#include "task.h"
#include "interrupts.h"
#include "gdt.h"
#include "../memory/heap.h"

#define MAX_TASKS 4

extern void task_switch(uint32_t** old_esp, uint32_t* new_esp);

// 태스크를 저장할 배열(테이블)
static Task g_tasks[MAX_TASKS];
static int g_count = 0;
static int g_current = -1;

// 메인 컨텍스트 저장용 변수
static uint32_t* g_main_esp = 0;

// 트램폴린에서 현재 태스크를 알아내기 위한 포인터
static Task* g_current_task = 0;

// 태스크 전용 스택의 top을 반환하는 내부 함수
static uint32_t task_kernel_stack_top(const Task* t)
{
    return (uint32_t)(t->stack + t->stack_size);
}

// 유휴 상태 루프
__attribute__((noreturn))
static void idle_loop(void)
{
    for (;;) __asm__ volatile("hlt");
}

__attribute__((noreturn))
static void task_trampoline(void)
{
    Task* t = g_current_task;
    if (!t || !t->fn) idle_loop();

    t->fn(t->arg);
    task_exit();
}

// 태스크 테이블 초기화 함수
void task_init(void)
{
    for (int i = 0; i < MAX_TASKS; ++i) {
        g_tasks[i].esp = 0;
        g_tasks[i].stack = 0;
        g_tasks[i].stack_size = 0;
        g_tasks[i].fn = 0;
        g_tasks[i].arg = 0;
        g_tasks[i].alive = false;
    }
    g_count = 0;
    g_current = -1;
    g_main_esp = 0;
    g_current_task = 0;
}

// 새 태스크가 처음 생성될 때 실행될 태스크의 초기 스택 구조 설정 함수
static uint32_t* build_initial_stack(uint8_t* stack, size_t stack_size)
{
    // 새 태스크의 시작 주소(top)
    uint32_t* sp = (uint32_t*)(stack + stack_size);
    
    // task_switch가 new esp로 바꾼 뒤:
    // pop edi, pop esi, pop ebx, pop ebp, ret 순서로 실행됨
    *(--sp) = (uint32_t)task_trampoline; // ret로 점프할 주소
    *(--sp) = 0; // ebp
    *(--sp) = 0; // ebx
    *(--sp) = 0; // esi
    *(--sp) = 0; // edi

    return sp;
}

// 새 태스크 생성 함수
int task_create(task_fn fn, void* arg, size_t stack_size)
{
    // 실행할 엔트리 함수가 없는 경우 실패
    if (!fn) { return -1; }
    // 태스크 수용 범위를 넘어선 경우 실패
    if (g_count >= MAX_TASKS) { return -1; }

    // 최소 스택 크기 보장(4KB)
    if (stack_size < 4096) stack_size = 4096;

    // 새 태스크 생성 후 태스크 테이블에 등록
    Task* t = &g_tasks[g_count];
    // 스택 크기 만큼의 공간을 kmalloc하여 스택 바닥 주소를 보관
    t->stack = (uint8_t*)kmalloc(stack_size);
    // 할당 실패 시 태스크 생성 실패
    if (!t->stack) { return -1; }

    // 태스크 구조체 정보 설정
    t->stack_size = stack_size;
    t->fn = fn;
    t->arg = arg;
    t->alive = true;
    t->state = TASK_RUNNABLE;
    t->wake_tick = 0;

    // 스택 기본 구조 생성
    t->esp = build_initial_stack(t->stack, t->stack_size);

    return g_count++;
}

// wake tick에 도달하였으면 태스크를 깨우는 함수
static void task_update_wakeup(Task* t)
{
    if (t->alive && t->state == TASK_SLEEPING)
    {
        if (g_timer_ticks >= t->wake_tick)
        {
            t->state = TASK_RUNNABLE;
        }
    }
}

// 다음에 실행할 alive 상태의 태스크 탐색 (Round-Robin 방식)
static int pick_next(int from)
{
    // 태스크 테이블이 비어 있는 경우 실패
    if (g_count == 0) { return -1; }

    for (int i = 1; i <= g_count; ++i)
    {
        int idx = (from + i) % g_count;

        Task* t = &g_tasks[idx];
        task_update_wakeup(t);

        if (t->alive && t->state == TASK_RUNNABLE)
        {
            return idx;
        }
    }
    return -1;
}

// 첫 번째 태스크로 컨텍스트 스위치하는 함수
void task_start(void)
{
    // 실행할 첫 태스크 탐색
    int next = pick_next(-1);
    // 실행할 태스크가 없는 경우 유휴 상태 돌입
    if (next < 0) { idle_loop(); }

    g_current = next;
    g_current_task = &g_tasks[g_current];

    // 현재 태스크의 전용 커널 스택 Top 주소로 TSS.esp0 갱신
    gdt_set_kernel_stack(task_kernel_stack_top(g_current_task));

    // 해당 태스크로 컨텍스트 스위칭
    task_switch(&g_main_esp, g_tasks[g_current].esp);

    idle_loop();
}

// 태스크가 CPU 점유를 양보하는 함수 (협력 스케줄링)
void task_yield(void)
{
    // 태스크가 1개 이하인 경우 양보하지 않음
    if (g_count <= 1) { return; }

    // 다음으로 실행 가능한 태스크 탐색
    int next = pick_next(g_current);
    // 다음으로 실행 가능한 태스크가 현재 태스크와 같거나 없는 경우 양보하지 않음
    if (next < 0 || next == g_current) { return; }

    int prev = g_current;
    g_current = next;
    g_current_task = &g_tasks[g_current];

    // 현재 태스크의 전용 커널 스택 Top 주소로 TSS.esp0 갱신
    gdt_set_kernel_stack(task_kernel_stack_top(g_current_task));

    // 해당 태스크로 컨텍스트 스위칭
    task_switch(&g_tasks[prev].esp, g_tasks[g_current].esp);
}

// 태스크 슬립 함수
void task_sleep(uint32_t ticks)
{
    if (ticks == 0)
    {
        task_yield();
        return;
    }

    if (g_current < 0 || g_current >= g_count)
    {
        return;
    }

    Task* t = &g_tasks[g_current];

    __asm__ volatile("cli");
    t->state = TASK_SLEEPING;
    t->wake_tick = g_timer_ticks + ticks;
    __asm__ volatile("sti");

    task_yield();
}

// 태스크 종료 함수
__attribute__((noreturn))
void task_exit(void)
{
    // 태스크의 alive 상태 false로 변경
    if (g_current >= 0 && g_current < g_count)
    {
        g_tasks[g_current].alive = false;
    }

    // 다음으로 실행할 태스크 탐색
    int next = pick_next(g_current);
    // 실행 가능한 태스크가 없는 경우 유휴 상태 돌입
    if (next < 0) { idle_loop(); }

    int prev = g_current;
    g_current = next;
    g_current_task = &g_tasks[g_current];

    // 현재 태스크의 전용 커널 스택 Top 주소로 TSS.esp0 갱신
    gdt_set_kernel_stack(task_kernel_stack_top(g_current_task));

    // 해당 태스크로 컨텍스트 스위칭
    task_switch(&g_tasks[prev].esp, g_tasks[g_current].esp);

    idle_loop();
}
#include "sched.h"

// 최대 task 수
#define MAX_TASKS 8

// task를 보관하는 배열을 정적으로 선언
static task_step_func g_tasks[MAX_TASKS];
static int g_count = 0;

// 스케줄러 내부 상태 초기화 함수
void sched_init(void)
{
    g_count = 0;
    for (int i = 0; i < MAX_TASKS; ++i)
    {
        g_tasks[i] = 0;
    }
}

// 실행할 task를 스케줄러에 등록하는 함수
int sched_add_task(task_step_func func)
{
    // func가 NULL인 경우 실패
    if (!func) return -1;
    
    // task 배열이 가득 찬 경우 실패
    if (g_count >= MAX_TASKS) return -1;
    
    // task를 스케줄러에 등록
    g_tasks[g_count] = func;
    
    // 등록한 task의 ID 반환
    return g_count++;
}

// 스케줄러에 등록된 task를 한 프레임에서 순서대로 한 번씩 호출하는 함수
void sched_run_one_frame(void)
{
    // task 배열 순회
    for (int i = 0; i < g_count; ++i)
    {
        g_tasks[i]();
    }
}
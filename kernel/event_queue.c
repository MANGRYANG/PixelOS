#include "event_queue.h"

// 이벤트 큐 최대 크기
#define EVENT_Q_SIZE 128

// 이벤트 큐는 원형 큐 형태로 정의
static volatile int q_head = 0;
static volatile int q_tail = 0;
static Event q_buf[EVENT_Q_SIZE];

static inline void irq_disable(void) { __asm__ volatile("cli"); }
static inline void irq_enable(void)  { __asm__ volatile("sti"); }

// 이벤트 큐 초기화 함수
void event_queue_init(void)
{
    irq_disable();
    q_head = 0;
    q_tail = 0;
    irq_enable();
}

bool event_push(const Event* e)
{
    // 이벤트가 올바르지 않은 경우 push 실패
    if (!e)
    {
        return false;
    }

    // IRQ 비활성화
    irq_disable();

    int next = q_head + 1;
    if (next >= EVENT_Q_SIZE) next = 0;

    // 이벤트 큐가 가득 찬 경우
    if (next == q_tail)
    {
        // IRQ 활성화 후 push 실패
        irq_enable();
        return false;
    }

    q_buf[q_head] = *e;
    q_head = next;

    // IRQ 활성화 후 push 성공
    irq_enable();
    return true;
}

bool event_pop(Event* out)
{
    // 호출자가 제공한 출력 버퍼가 NULL 포인터인 케이스 방지
    if (!out)
    {
        return false;
    }

    // IRQ 비활성화
    irq_disable();

    // 이벤트 큐가 비어 있는 경우
    if (q_tail == q_head)
    {
        // IRQ 활성화 후 pop 실패
        irq_enable();
        return false;
    }

    // 큐의 tail 위치에 있는 이벤트를 꺼내어 출력 버퍼에 복사
    *out = q_buf[q_tail];

    int next = q_tail + 1;
    if (next >= EVENT_Q_SIZE) next = 0;
    q_tail = next;

    // IRQ 활성화 후 pop 성공
    irq_enable();
    return true;
}
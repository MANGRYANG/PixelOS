#pragma once

// 인자가 없고 반환형이 void인 함수 포인터 타입의 별칭 정의
typedef void (*task_step_func)(void);

// 스케줄러 내부 상태 초기화 함수
void sched_init(void);
// 실행할 task를 스케줄러에 등록하는 함수
int sched_add_task(task_step_func func);
// 스케줄러에 등록된 task를 한 프레임에서 순서대로 한 번씩 호출하는 함수
void sched_run_one_frame(void);
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// 인자가 없고 반환형이 void인 함수 포인터 타입의 별칭 정의
typedef void (*task_fn)(void*);

// Task 구조체 정의
typedef struct Task {
    uint32_t* esp;          // 저장된 스택 포인터
    uint8_t* stack;         // 태스크 전용 스택 메모리 바닥 주소
    size_t stack_size;      // 태스크 전용 스택 크기
    task_fn fn;             // 태스크 시작 시점에서 실행될 함수 포인터
    void* arg;              // 해당 함수에 넘겨줄 인자
    bool alive;             // 태스크 실행 대상 여부
} Task;

// 태스크 테이블 초기화 함수
void task_init(void);
// 새 태스크 생성 함수
int task_create(task_fn fn, void* arg, size_t stack_size);

// 첫 태스크로 전환하는 함수
void task_start(void);
// 태스크가 CPU 점유를 양보하는 함수 (협력 스케줄링)
void task_yield(void);
// 태스크 종료 함수
__attribute__((noreturn)) void task_exit(void);

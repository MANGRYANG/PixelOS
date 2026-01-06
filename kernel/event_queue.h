#pragma once
#include <stdbool.h>
#include "event.h"

void event_queue_init(void);

// IRQ에서 push 작업, 메인루프에서 pop 작업 수행
bool event_push(const Event* e);
bool event_pop(Event* out);
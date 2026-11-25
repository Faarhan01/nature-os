#ifndef NATURE_RENDER_SCHEDULER_H
#define NATURE_RENDER_SCHEDULER_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

struct nature_scheduler {
    uint32_t target_hz;
    uint64_t frame_budget_ns;
    struct timespec last_present;
    bool input_priority_boost;
};

void nature_scheduler_init(struct nature_scheduler *sched, uint32_t hz);
bool nature_scheduler_should_present(const struct timespec *now);

#endif

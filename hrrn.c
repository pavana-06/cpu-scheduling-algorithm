#include "process.h"

#include <limits.h>

static long double response_ratio(const Process *process, long long current_time)
{
    long long waiting_time = current_time - process->arrival_time;

    return ((long double)waiting_time + (long double)process->burst_time) /
           (long double)process->burst_time;
}

static int hrrn_process_precedes(const Process *candidate,
                                const Process *current,
                                long long current_time)
{
    long double candidate_ratio = response_ratio(candidate, current_time);
    long double current_ratio = response_ratio(current, current_time);

    if (candidate_ratio != current_ratio) {
        return candidate_ratio > current_ratio;
    }
    if (candidate->arrival_time != current->arrival_time) {
        return candidate->arrival_time < current->arrival_time;
    }
    return candidate->id < current->id;
}

int schedule_hrrn(Process *processes, size_t count, size_t *schedule_order)
{
    long long current_time = 0;
    size_t completed_count = 0;

    /* An empty workload is a successful no-op, as in the other schedulers. */
    if (count == 0) {
        return 0;
    }
    if (processes == NULL || schedule_order == NULL) {
        return -1;
    }

    for (size_t i = 0; i < count; ++i) {
        if (processes[i].arrival_time < 0 || processes[i].burst_time <= 0 ||
            processes[i].id <= 0) {
            return -1;
        }
        for (size_t j = 0; j < i; ++j) {
            if (processes[i].id == processes[j].id) {
                return -1;
            }
        }
    }

    for (size_t i = 0; i < count; ++i) {
        processes[i].completed = 0;
        processes[i].start_time = 0;
        processes[i].completion_time = 0;
        processes[i].turnaround_time = 0;
        processes[i].waiting_time = 0;
        processes[i].response_time = 0;
    }

    while (completed_count < count) {
        size_t selected = count;
        size_t next_arrival = count;

        for (size_t i = 0; i < count; ++i) {
            if (processes[i].completed) {
                continue;
            }

            if (processes[i].arrival_time > current_time) {
                if (next_arrival == count ||
                    processes[i].arrival_time <
                        processes[next_arrival].arrival_time) {
                    next_arrival = i;
                }
                continue;
            }

            /* Among ready jobs, choose the highest response ratio. */
            if (selected == count ||
                hrrn_process_precedes(&processes[i], &processes[selected],
                                      current_time)) {
                selected = i;
            }
        }

        if (selected == count) {
            if (next_arrival == count) {
                return -1;
            }
            /* No process is ready, so advance to the next arrival. */
            current_time = processes[next_arrival].arrival_time;
            continue;
        }

        if (processes[selected].burst_time > LLONG_MAX - current_time) {
            return -1;
        }

        processes[selected].start_time = current_time;
        /* Non-preemptive execution runs the selected process to completion. */
        processes[selected].completion_time =
            current_time + processes[selected].burst_time;
        processes[selected].turnaround_time =
            processes[selected].completion_time -
            processes[selected].arrival_time;
        processes[selected].waiting_time =
            processes[selected].turnaround_time -
            processes[selected].burst_time;
        processes[selected].response_time =
            processes[selected].start_time -
            processes[selected].arrival_time;
        processes[selected].completed = 1;
        schedule_order[completed_count] = selected;
        ++completed_count;
        current_time = processes[selected].completion_time;
    }

    return 0;
}
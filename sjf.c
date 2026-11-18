#include "process.h"

#include <limits.h>

static int sjf_process_precedes(const Process *candidate,
                                const Process *current)
{
    if (candidate->burst_time != current->burst_time) {
        return candidate->burst_time < current->burst_time;
    }
    if (candidate->arrival_time != current->arrival_time) {
        return candidate->arrival_time < current->arrival_time;
    }
    return candidate->id < current->id;
}

int schedule_sjf(Process *processes, size_t count, size_t *schedule_order)
{
    long long current_time = 0;
    size_t completed_count = 0;

    /* An empty workload is a successful no-op; nonempty workloads need arrays. */
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
        size_t candidate = count;
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

            /* Among ready jobs, choose the shortest burst; ties use arrival
               time and then process ID. */
            if (candidate == count ||
                sjf_process_precedes(&processes[i], &processes[candidate])) {
                candidate = i;
            }
        }

        if (candidate == count) {
            if (next_arrival == count) {
                return -1;
            }
            /* No process is ready, so jump to the earliest unfinished arrival. */
            current_time = processes[next_arrival].arrival_time;
            continue;
        }

        if (processes[candidate].burst_time > LLONG_MAX - current_time) {
            return -1;
        }

        processes[candidate].start_time = current_time;
        /* SJF is non-preemptive: run this process through its full burst. */
        processes[candidate].completion_time =
            current_time + processes[candidate].burst_time;
        processes[candidate].turnaround_time =
            processes[candidate].completion_time -
            processes[candidate].arrival_time;
        processes[candidate].waiting_time =
            processes[candidate].turnaround_time -
            processes[candidate].burst_time;
        processes[candidate].response_time =
            processes[candidate].start_time -
            processes[candidate].arrival_time;
        processes[candidate].completed = 1;
        schedule_order[completed_count] = candidate;
        ++completed_count;
        current_time = processes[candidate].completion_time;
    }

    return 0;
}

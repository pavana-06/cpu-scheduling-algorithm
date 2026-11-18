
#include "process.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

/* Add an execution interval, merging adjacent intervals for the same process. */
static int add_timeline_segment(SRTFSegment *timeline,
                                size_t capacity,
                                size_t *timeline_count,
                                int process_id,
                                long long start_time,
                                long long end_time)
{
    if (start_time >= end_time) {
        return -1;
    }

    if (*timeline_count > 0) {
        SRTFSegment *last = &timeline[*timeline_count - 1];

        if (last->process_id == process_id &&
            last->end_time == start_time) {
            last->end_time = end_time;
            return 0;
        }
    }

    if (*timeline_count >= capacity) {
        return -1;
    }

    timeline[*timeline_count].process_id = process_id;
    timeline[*timeline_count].start_time = start_time;
    timeline[*timeline_count].end_time = end_time;

    ++(*timeline_count);
    return 0;
}

static int srtf_process_precedes(const Process *candidate,
                                 const Process *current,
                                 const long long *remaining_time,
                                 size_t candidate_index,
                                 size_t current_index)
{
    if (remaining_time[candidate_index] !=
        remaining_time[current_index]) {
        return remaining_time[candidate_index] <
               remaining_time[current_index];
    }

    if (candidate->arrival_time != current->arrival_time) {
        return candidate->arrival_time < current->arrival_time;
    }

    return candidate->id < current->id;
}

int schedule_srtf(Process *processes, size_t count,
                  size_t *schedule_order,
                  SRTFSegment *timeline,
                  size_t timeline_capacity,
                  size_t *timeline_count)
{
    long long *remaining_time;
    long long current_time = 0;
    size_t completed_count = 0;

    if (timeline_count == NULL) {
        return -1;
    }

    *timeline_count = 0;

    if (count == 0) {
        return 0;
    }

    if (processes == NULL || schedule_order == NULL ||
        timeline == NULL ||
        count > SIZE_MAX / sizeof(*remaining_time)) {
        return -1;
    }

    for (size_t i = 0; i < count; ++i) {
        if (processes[i].arrival_time < 0 ||
            processes[i].burst_time <= 0 ||
            processes[i].id <= 0) {
            return -1;
        }

        for (size_t j = 0; j < i; ++j) {
            if (processes[i].id == processes[j].id) {
                return -1;
            }
        }
    }

    remaining_time = malloc(count * sizeof(*remaining_time));

    if (remaining_time == NULL) {
        return -1;
    }

    for (size_t i = 0; i < count; ++i) {
        processes[i].completed = 0;
        processes[i].start_time = 0;
        processes[i].completion_time = 0;
        processes[i].turnaround_time = 0;
        processes[i].waiting_time = 0;
        processes[i].response_time = 0;

        remaining_time[i] = processes[i].burst_time;
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

            if (selected == count ||
                srtf_process_precedes(
                    &processes[i],
                    &processes[selected],
                    remaining_time,
                    i,
                    selected)) {
                selected = i;
            }
        }

        /* No process is ready: record CPU idle time. */
        if (selected == count) {
            if (next_arrival == count) {
                free(remaining_time);
                return -1;
            }

            long long idle_end =
                processes[next_arrival].arrival_time;

            if (add_timeline_segment(
                    timeline, timeline_capacity, timeline_count,
                    0, current_time, idle_end) != 0) {
                free(remaining_time);
                return -1;
            }

            current_time = idle_end;
            continue;
        }

        /* Record the first time this process receives the CPU. */
        if (remaining_time[selected] ==
            processes[selected].burst_time) {
            processes[selected].start_time = current_time;

            processes[selected].response_time =
                current_time - processes[selected].arrival_time;
        }

        /* Run until completion or the next process arrival. */
        long long run_time = remaining_time[selected];

        if (next_arrival != count) {
            long long until_arrival =
                processes[next_arrival].arrival_time - current_time;

            if (until_arrival < run_time) {
                run_time = until_arrival;
            }
        }

        if (run_time <= 0 ||
            run_time > LLONG_MAX - current_time) {
            free(remaining_time);
            return -1;
        }

        long long segment_start = current_time;
        long long segment_end = current_time + run_time;

        if (add_timeline_segment(
                timeline, timeline_capacity, timeline_count,
                processes[selected].id,
                segment_start, segment_end) != 0) {
            free(remaining_time);
            return -1;
        }

        current_time = segment_end;
        remaining_time[selected] -= run_time;

        /* Record completion when remaining time reaches zero. */
        if (remaining_time[selected] == 0) {
            processes[selected].completion_time = current_time;

            processes[selected].turnaround_time =
                processes[selected].completion_time -
                processes[selected].arrival_time;

            processes[selected].waiting_time =
                processes[selected].turnaround_time -
                processes[selected].burst_time;

            processes[selected].completed = 1;

            schedule_order[completed_count] = selected;
            ++completed_count;
        }
    }

    free(remaining_time);
    return 0;
}

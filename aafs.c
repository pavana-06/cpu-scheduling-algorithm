#include "process.h"

#include <limits.h>

static int calculate_aging_threshold(const Process *processes,
                                     size_t count,
                                     long long *aging_threshold)
{
    unsigned long long divisor = (unsigned long long)count;
    unsigned long long quotient_sum = 0;
    unsigned long long remainder_sum = 0;

    for (size_t i = 0; i < count; ++i) {
        unsigned long long burst_time =
            (unsigned long long)processes[i].burst_time;
        unsigned long long quotient = burst_time / divisor;
        unsigned long long remainder = burst_time % divisor;

        if (quotient_sum > ULLONG_MAX - quotient ||
            remainder_sum > ULLONG_MAX - remainder) {
            return -1;
        }
        quotient_sum += quotient;
        remainder_sum += remainder;
    }

    unsigned long long fractional_whole = remainder_sum / divisor;
    unsigned long long fractional_remainder = remainder_sum % divisor;

    if (quotient_sum > ULLONG_MAX - fractional_whole) {
        return -1;
    }
    unsigned long long whole_average = quotient_sum + fractional_whole;

    if (whole_average > (unsigned long long)LLONG_MAX) {
        return -1;
    }

    /* Round to nearest integer, with exact halves rounded upward. */
    if (fractional_remainder >= (divisor + 1) / 2) {
        if (whole_average == (unsigned long long)LLONG_MAX) {
            return -1;
        }
        ++whole_average;
    }

    *aging_threshold = (long long)whole_average;
    return 0;
}

static int aged_process_precedes(const Process *candidate,
                                 const Process *current,
                                 long long current_time)
{
    long long candidate_wait = current_time - candidate->arrival_time;
    long long current_wait = current_time - current->arrival_time;

    /* Among aged processes, prefer the longest wait, then earlier arrival
       and lower process ID. */
    if (candidate_wait != current_wait) {
        return candidate_wait > current_wait;
    }
    if (candidate->arrival_time != current->arrival_time) {
        return candidate->arrival_time < current->arrival_time;
    }
    return candidate->id < current->id;
}

static int sjf_process_precedes(const Process *candidate,
                                const Process *current)
{
    /* Before aging applies, prefer shorter original burst, then earlier
       arrival and lower process ID. */
    if (candidate->burst_time != current->burst_time) {
        return candidate->burst_time < current->burst_time;
    }
    if (candidate->arrival_time != current->arrival_time) {
        return candidate->arrival_time < current->arrival_time;
    }
    return candidate->id < current->id;
}

int schedule_aafs(Process *processes, size_t count,
                  size_t *schedule_order, long long *aging_threshold)
{
    long long current_time = 0;
    long long fixed_threshold;
    size_t completed_count = 0;

    if (processes == NULL || schedule_order == NULL ||
        aging_threshold == NULL || count == 0 ||
        count > (size_t)INT_MAX) {
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

    if (calculate_aging_threshold(
            processes, count, &fixed_threshold) != 0) {
        return -1;
    }
    *aging_threshold = fixed_threshold;

    for (size_t i = 0; i < count; ++i) {
        processes[i].completed = 0;
        processes[i].start_time = 0;
        processes[i].completion_time = 0;
        processes[i].turnaround_time = 0;
        processes[i].waiting_time = 0;
        processes[i].response_time = 0;
    }

    while (completed_count < count) {
        size_t aged_candidate = count;
        size_t sjf_candidate = count;
        size_t next_process = count;

        for (size_t i = 0; i < count; ++i) {
            if (processes[i].completed) {
                continue;
            }

            if (processes[i].arrival_time > current_time) {
                if (next_process == count ||
                    processes[i].arrival_time <
                        processes[next_process].arrival_time) {
                    next_process = i;
                }
                continue;
            }

            /* Classify ready processes using the fixed threshold for this run. */
            if (current_time - processes[i].arrival_time >=
                fixed_threshold) {
                if (aged_candidate == count ||
                    aged_process_precedes(&processes[i],
                                          &processes[aged_candidate],
                                          current_time)) {
                    aged_candidate = i;
                }
            } else if (sjf_candidate == count ||
                       sjf_process_precedes(&processes[i],
                                            &processes[sjf_candidate])) {
                sjf_candidate = i;
            }
        }

        /* Any aged process takes priority; its comparator picks the longest
           waiting one. Otherwise, use the shortest-burst candidate. */
        if (aged_candidate != count) {
            next_process = aged_candidate;
        } else if (sjf_candidate != count) {
            next_process = sjf_candidate;
        } else if (next_process != count) {
            /* No process is ready: advance directly to the next arrival. */
            current_time = processes[next_process].arrival_time;
            continue;
        } else {
            return -1;
        }

        if (processes[next_process].burst_time > LLONG_MAX - current_time) {
            return -1;
        }

        processes[next_process].start_time = current_time;
        /* Non-preemptive execution runs the selected process to completion. */
        processes[next_process].completion_time =
            current_time + processes[next_process].burst_time;
        /* Turnaround = completion - arrival; waiting = turnaround - burst;
           response = first start - arrival. */
        processes[next_process].turnaround_time =
            processes[next_process].completion_time -
            processes[next_process].arrival_time;
        processes[next_process].waiting_time =
            processes[next_process].turnaround_time -
            processes[next_process].burst_time;
        processes[next_process].response_time =
            processes[next_process].start_time -
            processes[next_process].arrival_time;
        processes[next_process].completed = 1;
        schedule_order[completed_count] = next_process;
        ++completed_count;
        current_time = processes[next_process].completion_time;
    }

    return 0;
}

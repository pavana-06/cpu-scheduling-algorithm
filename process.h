#ifndef PROCESS_H
#define PROCESS_H

#include <stddef.h>

typedef struct {
    int id;
    long long arrival_time;
    long long burst_time;
    long long start_time;
    long long completion_time;
    long long turnaround_time;
    long long waiting_time;
    long long response_time;
    int completed;
} Process;

/* Records one continuous SRTF execution interval. */
typedef struct {
    int process_id;
    long long start_time;
    long long end_time;
} SRTFSegment;

/* Proposed AAFS scheduling algorithm. */
int schedule_aafs(Process *processes, size_t count,
                  size_t *schedule_order, long long *aging_threshold);

/* Non-preemptive Shortest Job First. */
int schedule_sjf(Process *processes, size_t count,
                 size_t *schedule_order);

/* Preemptive Shortest Remaining Time First with execution timeline. */
int schedule_srtf(Process *processes, size_t count,
                  size_t *schedule_order,
                  SRTFSegment *timeline,
                  size_t timeline_capacity,
                  size_t *timeline_count);

/* Non-preemptive Highest Response Ratio Next. */
int schedule_hrrn(Process *processes, size_t count,
                  size_t *schedule_order);

#endif

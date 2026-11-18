#include "process.h"

#include <limits.h>
#include <stdio.h>

#define MAX_PROCESSES 5
#define MAX_SEGMENTS 10
#define ALGORITHM_COUNT 4

typedef struct {
    int id;
    long long arrival_time;
    long long burst_time;
} InputProcess;

typedef struct {
    size_t count;
    InputProcess processes[MAX_PROCESSES];
    long long threshold;
    int order[ALGORITHM_COUNT][MAX_PROCESSES];
    long long start[ALGORITHM_COUNT][MAX_PROCESSES];
    long long completion[ALGORITHM_COUNT][MAX_PROCESSES];
    SRTFSegment srtf_timeline[MAX_SEGMENTS];
    size_t srtf_timeline_count;
} Scenario;

static const Scenario scenarios[] = {
    {
        3,
        {{1, 0, 8}, {2, 1, 4}, {3, 2, 2}},
        5,
        {{1, 2, 3}, {1, 3, 2}, {3, 2, 1}, {1, 3, 2}},
        {{0, 8, 12}, {0, 10, 8}, {0, 1, 2}, {0, 10, 8}},
        {{8, 12, 14}, {8, 14, 10}, {14, 7, 4}, {8, 14, 10}},
        {{1, 0, 1}, {2, 1, 2}, {3, 2, 4}, {2, 4, 7}, {1, 7, 14}},
        5
    },
    {
        4,
        {{1, 0, 8}, {2, 0, 3}, {3, 0, 6}, {4, 0, 2}},
        5,
        {{4, 2, 1, 3}, {4, 2, 3, 1}, {4, 2, 3, 1}, {1, 4, 2, 3}},
        {{5, 2, 13, 0}, {11, 2, 5, 0}, {11, 2, 5, 0}, {0, 10, 13, 8}},
        {{13, 5, 19, 2}, {19, 5, 11, 2}, {19, 5, 11, 2}, {8, 13, 19, 10}},
        {{4, 0, 2}, {2, 2, 5}, {3, 5, 11}, {1, 11, 19}},
        4
    },
    {
        3,
        {{1, 0, 3}, {2, 10, 2}, {3, 15, 4}},
        3,
        {{1, 2, 3}, {1, 2, 3}, {1, 2, 3}, {1, 2, 3}},
        {{0, 10, 15}, {0, 10, 15}, {0, 10, 15}, {0, 10, 15}},
        {{3, 12, 19}, {3, 12, 19}, {3, 12, 19}, {3, 12, 19}},
        {{1, 0, 3}, {0, 3, 10}, {2, 10, 12}, {0, 12, 15}, {3, 15, 19}},
        5
    },
    {
        5,
        {{1, 0, 12}, {2, 1, 2}, {3, 2, 3}, {4, 3, 4}, {5, 4, 5}},
        5,
        {{1, 2, 3, 4, 5}, {1, 2, 3, 4, 5},
         {2, 3, 4, 5, 1}, {1, 2, 3, 4, 5}},
        {{0, 12, 14, 17, 21}, {0, 12, 14, 17, 21},
         {0, 1, 3, 6, 10}, {0, 12, 14, 17, 21}},
        {{12, 14, 17, 21, 26}, {12, 14, 17, 21, 26},
         {26, 3, 6, 10, 15}, {12, 14, 17, 21, 26}},
        {{1, 0, 1}, {2, 1, 3}, {3, 3, 6},
         {4, 6, 10}, {5, 10, 15}, {1, 15, 26}},
        6
    }
};

static int fail(const char *scenario, const char *message)
{
    fprintf(stderr, "FAIL (%s): %s\n", scenario, message);
    return 1;
}

static int run_scenario(const Scenario *scenario, size_t scenario_index)
{
    static const char *const names[ALGORITHM_COUNT] = {
        "AAFS", "SJF", "SRTF", "HRRN"
    };
    static const long double expected_averages[][ALGORITHM_COUNT][3] = {
        {
            {5.6666666667L, 10.3333333333L, 5.6666666667L},
            {5.0L, 9.6666666667L, 5.0L},
            {2.6666666667L, 7.3333333333L, 0.0L},
            {5.0L, 9.6666666667L, 5.0L}
        },
        {
            {5.0L, 9.75L, 5.0L},
            {4.5L, 9.25L, 4.5L},
            {4.5L, 9.25L, 4.5L},
            {7.75L, 12.5L, 7.75L}
        },
        {
            {0.0L, 3.0L, 0.0L},
            {0.0L, 3.0L, 0.0L},
            {0.0L, 3.0L, 0.0L},
            {0.0L, 3.0L, 0.0L}
        },
        {
            {10.8L, 16.0L, 10.8L},
            {10.8L, 16.0L, 10.8L},
            {4.8L, 10.0L, 2.0L},
            {10.8L, 16.0L, 10.8L}
        }
    };
    char scenario_name[32];
    Process workload[ALGORITHM_COUNT][MAX_PROCESSES];
    size_t orders[ALGORITHM_COUNT][MAX_PROCESSES];
    SRTFSegment timeline[MAX_SEGMENTS];
    size_t timeline_count = 0;
    long long threshold = -1;
    int status[ALGORITHM_COUNT];

    (void)snprintf(scenario_name, sizeof(scenario_name),
                   "scenario %zu", scenario_index + 1);

    for (size_t algorithm = 0; algorithm < ALGORITHM_COUNT; ++algorithm) {
        for (size_t i = 0; i < scenario->count; ++i) {
            workload[algorithm][i] = (Process) {
                .id = scenario->processes[i].id,
                .arrival_time = scenario->processes[i].arrival_time,
                .burst_time = scenario->processes[i].burst_time
            };
        }
    }

    status[0] = schedule_aafs(workload[0], scenario->count,
                              orders[0], &threshold);
    status[1] = schedule_sjf(workload[1], scenario->count, orders[1]);
    status[2] = schedule_srtf(workload[2], scenario->count, orders[2],
                              timeline, MAX_SEGMENTS, &timeline_count);
    status[3] = schedule_hrrn(workload[3], scenario->count, orders[3]);

    if (status[0] != 0 || status[1] != 0 ||
        status[2] != 0 || status[3] != 0) {
        return fail(scenario_name, "a scheduler returned an error");
    }
    if (threshold != scenario->threshold) {
        return fail(scenario_name, "automatic AAFS threshold mismatch");
    }

    for (size_t algorithm = 0; algorithm < ALGORITHM_COUNT; ++algorithm) {
        long long waiting_total = 0;
        long long turnaround_total = 0;
        long long response_total = 0;
        int seen[MAX_PROCESSES] = {0};

        for (size_t position = 0; position < scenario->count; ++position) {
            int process_id =
                workload[algorithm][orders[algorithm][position]].id;

            if (process_id < 1 || (size_t)process_id > scenario->count ||
                seen[process_id - 1]) {
                return fail(scenario_name,
                            "schedule order has a missing or duplicate process");
            }
            seen[process_id - 1] = 1;
            if (process_id != scenario->order[algorithm][position]) {
                fprintf(stderr, "FAIL (%s, %s): scheduling order mismatch\n",
                        scenario_name, names[algorithm]);
                return 1;
            }
        }

        for (size_t i = 0; i < scenario->count; ++i) {
            const Process *process = &workload[algorithm][i];
            long long expected_start = scenario->start[algorithm][i];
            long long expected_completion =
                scenario->completion[algorithm][i];
            long long expected_turnaround =
                expected_completion - process->arrival_time;
            long long expected_waiting =
                expected_turnaround - process->burst_time;
            long long expected_response =
                expected_start - process->arrival_time;

            if (!process->completed ||
                process->start_time != expected_start ||
                process->completion_time != expected_completion ||
                process->turnaround_time != expected_turnaround ||
                process->waiting_time != expected_waiting ||
                process->response_time != expected_response) {
                fprintf(stderr,
                        "FAIL (%s, %s): metrics mismatch for P%d\n",
                        scenario_name, names[algorithm], process->id);
                return 1;
            }

            waiting_total += process->waiting_time;
            turnaround_total += process->turnaround_time;
            response_total += process->response_time;
        }

        long double actual_averages[] = {
            (long double)waiting_total / (long double)scenario->count,
            (long double)turnaround_total / (long double)scenario->count,
            (long double)response_total / (long double)scenario->count
        };
        for (size_t metric = 0; metric < 3; ++metric) {
            long double expected =
                expected_averages[scenario_index][algorithm][metric];
            long double difference = actual_averages[metric] - expected;

            if (difference < -0.000001L || difference > 0.000001L) {
                fprintf(stderr,
                        "FAIL (%s, %s): average metric mismatch\n",
                        scenario_name, names[algorithm]);
                return 1;
            }
        }
    }

    if (timeline_count != scenario->srtf_timeline_count) {
        return fail(scenario_name, "SRTF Gantt chart segment count mismatch");
    }
    for (size_t i = 0; i < timeline_count; ++i) {
        if (timeline[i].process_id != scenario->srtf_timeline[i].process_id ||
            timeline[i].start_time != scenario->srtf_timeline[i].start_time ||
            timeline[i].end_time != scenario->srtf_timeline[i].end_time) {
            return fail(scenario_name, "SRTF execution timeline mismatch");
        }
    }

    return 0;
}

static int test_aafs_validation_and_overflow(void)
{
    Process process = {1, 0, 1, 0, 0, 0, 0, 0, 0};
    size_t order = 0;
    long long threshold = 0;

    if (schedule_aafs(&process, 0, &order, &threshold) == 0) {
        return fail("empty workload", "AAFS accepted an empty workload");
    }

    Process large_workload[] = {
        {1, 0, LLONG_MAX, 0, 0, 0, 0, 0, 0},
        {2, 0, LLONG_MAX, 0, 0, 0, 0, 0, 0}
    };
    size_t large_order[2];

    if (schedule_aafs(large_workload, 2, large_order, &threshold) == 0) {
        return fail("overflow workload",
                    "AAFS failed to report completion-time overflow");
    }
    if (threshold != LLONG_MAX) {
        return fail("overflow workload",
                    "threshold calculation overflowed for large bursts");
    }

    return 0;
}

int main(void)
{
    for (size_t i = 0; i < sizeof(scenarios) / sizeof(scenarios[0]); ++i) {
        if (run_scenario(&scenarios[i], i) != 0) {
            return 1;
        }
    }

    if (test_aafs_validation_and_overflow() != 0) {
        return 1;
    }

    printf("All scheduling scenarios and edge cases passed.\n");
    return 0;
}

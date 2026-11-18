#include "process.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>

#define ALGORITHM_COUNT 4

typedef struct {
    const char *name;
    Process *processes;
    size_t *order;
    long double avg_waiting;
    long double avg_turnaround;
    long double avg_response;
    long double cpu_utilization;
    long double throughput;
} AlgorithmResult;

static int read_integer(long long *value)
{
    char token[128];
    size_t length = 0;
    int too_long = 0;
    int character;

    do {
        character = getchar();
    } while (character != EOF && isspace((unsigned char)character));

    if (character == EOF) {
        return -1;
    }

    do {
        if (length < sizeof(token) - 1) {
            token[length++] = (char)character;
        } else {
            too_long = 1;
        }
        character = getchar();
    } while (character != EOF && !isspace((unsigned char)character));

    if (too_long) {
        return -1;
    }

    token[length] = '\0';
    errno = 0;
    char *end;
    long long parsed_value = strtoll(token, &end, 10);

    if (errno == ERANGE || end == token || *end != '\0') {
        return -1;
    }

    *value = parsed_value;
    return 0;
}

static void print_order(const AlgorithmResult *result, size_t count)
{
    printf("Process order: ");

    for (size_t i = 0; i < count; ++i) {
        printf("P%d%s",
               result->processes[result->order[i]].id,
               i + 1 == count ? "\n" : " -> ");
    }
}

/* Gantt chart for non-preemptive algorithms. */
static void print_gantt_chart(const AlgorithmResult *result,
                              size_t count)
{
    long long chart_time = 0;

    printf("Gantt chart:\n%lld", chart_time);

    for (size_t i = 0; i < count; ++i) {
        const Process *p = &result->processes[result->order[i]];

        if (p->start_time > chart_time) {
            printf(" -- Idle -- %lld", p->start_time);
        }

        printf(" -- P%d -- %lld", p->id, p->completion_time);
        chart_time = p->completion_time;
    }

    putchar('\n');
}

/* Gantt chart using the actual preemption timeline recorded by SRTF. */
static void print_srtf_gantt_chart(const SRTFSegment *timeline,
                                   size_t timeline_count)
{
    printf("Gantt chart:\n");

    if (timeline_count == 0) {
        printf("No execution intervals recorded.\n");
        return;
    }

    printf("%lld", timeline[0].start_time);

    for (size_t i = 0; i < timeline_count; ++i) {
        if (timeline[i].process_id == 0) {
            printf(" -- Idle -- %lld", timeline[i].end_time);
        } else {
            printf(" -- P%d -- %lld",
                   timeline[i].process_id,
                   timeline[i].end_time);
        }
    }

    putchar('\n');
}

static void print_results(AlgorithmResult *result,
                          size_t count,
                          const SRTFSegment *timeline,
                          size_t timeline_count)
{
    long double waiting_total = 0.0L;
    long double turnaround_total = 0.0L;
    long double response_total = 0.0L;
    long double total_burst_time = 0.0L;
    long long max_completion_time = 0;
    size_t completed_count = 0;

    printf("\n========================================\n");
    printf("             %s RESULTS\n", result->name);
    printf("========================================\n");

    print_order(result, count);

    if (timeline != NULL) {
        print_srtf_gantt_chart(timeline, timeline_count);
    } else {
        print_gantt_chart(result, count);
    }

    printf("\nProcess  AT      BT      CT      TAT     WT      RT\n");

    for (size_t i = 0; i < count; ++i) {
        Process p = result->processes[i];

        printf("P%-7d %-7lld %-7lld %-7lld %-7lld %-7lld %-7lld\n",
               p.id,
               p.arrival_time,
               p.burst_time,
               p.completion_time,
               p.turnaround_time,
               p.waiting_time,
               p.response_time);

        waiting_total += (long double)p.waiting_time;
        turnaround_total += (long double)p.turnaround_time;
        response_total += (long double)p.response_time;
        total_burst_time += (long double)p.burst_time;

        if (p.completion_time > max_completion_time) {
            max_completion_time = p.completion_time;
        }
        if (p.completed) {
            ++completed_count;
        }
    }

    result->avg_waiting = waiting_total / (long double)count;
    result->avg_turnaround = turnaround_total / (long double)count;
    result->avg_response = response_total / (long double)count;
    long double total_elapsed_time = (long double)max_completion_time;
    result->cpu_utilization = total_elapsed_time > 0.0L
        ? total_burst_time / total_elapsed_time * 100.0L
        : 0.0L;
    result->throughput = total_elapsed_time > 0.0L
        ? (long double)completed_count / total_elapsed_time
        : 0.0L;

    printf("\nAverage Waiting Time: %.2Lf\n",
           result->avg_waiting);

    printf("Average Turnaround Time: %.2Lf\n",
           result->avg_turnaround);

    printf("Average Response Time: %.2Lf\n",
           result->avg_response);

    printf("CPU Utilization: %.2Lf%%\n",
           result->cpu_utilization);

    printf("Throughput: %.4Lf processes/time unit\n",
           result->throughput);
}

static void print_comparison(const AlgorithmResult results[])
{
    printf("\n\n========================================\n");
    printf("          ALGORITHM COMPARISON\n");
    printf("========================================\n");

    printf("\n%-12s %-9s %-9s %-9s %12s  %s\n",
           "Algorithm",
           "Avg WT",
           "Avg TAT",
           "Avg RT",
           "CPU Util. (%)",
           "Throughput (P/unit)");

    for (size_t i = 0; i < ALGORITHM_COUNT; ++i) {
        printf("%-12s %-9.2Lf %-9.2Lf %-9.2Lf %12.2Lf%%  %.4Lf\n",
               results[i].name,
               results[i].avg_waiting,
               results[i].avg_turnaround,
               results[i].avg_response,
               results[i].cpu_utilization,
               results[i].throughput);
    }
}

int main(void)
{
    long long input_count;
    long long aging_threshold = 0;
    size_t count;

    Process *original = NULL;
    SRTFSegment *srtf_timeline = NULL;
    size_t srtf_timeline_count = 0;
    size_t timeline_capacity = 0;

    int exit_status = EXIT_FAILURE;

    AlgorithmResult results[ALGORITHM_COUNT] = {
        {"AAFS", NULL, NULL, 0, 0, 0, 0, 0},
        {"SJF", NULL, NULL, 0, 0, 0, 0, 0},
        {"SRTF", NULL, NULL, 0, 0, 0, 0, 0},
        {"HRRN", NULL, NULL, 0, 0, 0, 0, 0}
    };

    printf("Enter number of processes: ");

    if (read_integer(&input_count) != 0 ||
        input_count <= 0 || input_count > INT_MAX) {
        fprintf(stderr, "Invalid process count.\n");
        goto cleanup;
    }

    count = (size_t)input_count;

    if (count > SIZE_MAX / sizeof(Process) ||
        count > SIZE_MAX / sizeof(size_t) ||
        count > SIZE_MAX / 3) {
        fprintf(stderr, "Process count is too large.\n");
        goto cleanup;
    }

    original = malloc(count * sizeof(Process));

    if (original == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        goto cleanup;
    }

    for (size_t i = 0; i < count; ++i) {
        original[i] = (Process){0};
        original[i].id = (int)i + 1;

        printf("Enter arrival time and burst time for P%zu: ", i + 1);

        if (read_integer(&original[i].arrival_time) != 0 ||
            read_integer(&original[i].burst_time) != 0 ||
            original[i].arrival_time < 0 ||
            original[i].burst_time <= 0) {
            fprintf(stderr, "Invalid process input.\n");
            goto cleanup;
        }
    }

    /* Give each algorithm an identical copy of the workload. */
    for (size_t i = 0; i < ALGORITHM_COUNT; ++i) {
        results[i].processes = malloc(count * sizeof(Process));
        results[i].order = malloc(count * sizeof(size_t));

        if (results[i].processes == NULL ||
            results[i].order == NULL) {
            fprintf(stderr, "Memory allocation failed.\n");
            goto cleanup;
        }

        for (size_t j = 0; j < count; ++j) {
            results[i].processes[j] = original[j];
        }
    }

    free(original);
    original = NULL;

    /* Allocate storage for the SRTF execution timeline. */
    timeline_capacity = count * 3;

    if (timeline_capacity >
        SIZE_MAX / sizeof(SRTFSegment)) {
        fprintf(stderr, "Timeline capacity is too large.\n");
        goto cleanup;
    }

    srtf_timeline = malloc(
        timeline_capacity * sizeof(SRTFSegment));

    if (srtf_timeline == NULL) {
        fprintf(stderr, "Timeline allocation failed.\n");
        goto cleanup;
    }

    /* Execute all four scheduling algorithms. */
    int status[ALGORITHM_COUNT];

    status[0] = schedule_aafs(
        results[0].processes,
        count,
        results[0].order,
        &aging_threshold);

    status[1] = schedule_sjf(
        results[1].processes,
        count,
        results[1].order);

    status[2] = schedule_srtf(
        results[2].processes,
        count,
        results[2].order,
        srtf_timeline,
        timeline_capacity,
        &srtf_timeline_count);

    status[3] = schedule_hrrn(
        results[3].processes,
        count,
        results[3].order);

    for (size_t i = 0; i < ALGORITHM_COUNT; ++i) {
        if (status[i] != 0) {
            fprintf(stderr, "%s scheduling failed.\n",
                    results[i].name);
            goto cleanup;
        }
    }

    /* Display individual algorithm results. */
    printf("\nAAFS automatic aging threshold: %lld\n",
           aging_threshold);
    print_results(&results[0], count, NULL, 0);
    print_results(&results[1], count, NULL, 0);
    print_results(&results[2], count,
                  srtf_timeline, srtf_timeline_count);
    print_results(&results[3], count, NULL, 0);

    /* Display the final comparison. */
    print_comparison(results);

    exit_status = EXIT_SUCCESS;

cleanup:
    free(original);
    free(srtf_timeline);

    for (size_t i = 0; i < ALGORITHM_COUNT; ++i) {
        free(results[i].processes);
        free(results[i].order);
    }

    return exit_status;
}

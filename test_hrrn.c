
#include "process.h"
#include <stdio.h>

int main(void)
{
    Process processes[] = {
        {1, 0, 3, 0, 0, 0, 0, 0, 0},
        {2, 1, 8, 0, 0, 0, 0, 0, 0},
        {3, 2, 4, 0, 0, 0, 0, 0, 0},
        {4, 3, 2, 0, 0, 0, 0, 0, 0}
    };

    size_t count = 4;
    size_t order[4];

    if (schedule_hrrn(processes, count, order) != 0) {
        printf("HRRN scheduling failed.\n");
        return 1;
    }

    printf("HRRN Scheduling Order: ");

    for (size_t i = 0; i < count; i++) {
        printf("P%d", processes[order[i]].id);

        if (i < count - 1)
            printf(" -> ");
    }

    printf("\n\nProcess\tAT\tBT\tCT\tTAT\tWT\tRT\n");

    double total_wt = 0;
    double total_tat = 0;
    double total_rt = 0;

    for (size_t i = 0; i < count; i++) {
        Process p = processes[i];

        printf("P%d\t%lld\t%lld\t%lld\t%lld\t%lld\t%lld\n",
               p.id, p.arrival_time, p.burst_time,
               p.completion_time, p.turnaround_time,
               p.waiting_time, p.response_time);

        total_wt += p.waiting_time;
        total_tat += p.turnaround_time;
        total_rt += p.response_time;
    }

    printf("\nAverage Waiting Time: %.2f\n", total_wt / count);
    printf("Average Turnaround Time: %.2f\n", total_tat / count);
    printf("Average Response Time: %.2f\n", total_rt / count);

    return 0;
}

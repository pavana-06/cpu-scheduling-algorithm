#include "process.h"
#include <stdio.h>

int main(void)
{
    Process processes[] = {
        {1, 0, 3, 0, 0, 0, 0, 0, 0},
        {2, 10, 2, 0, 0, 0, 0, 0, 0},
        {3, 15, 4, 0, 0, 0, 0, 0, 0}
    };

    size_t count = 3;
    size_t order[3];

    if (schedule_sjf(processes, count, order) != 0) {
        printf("Scheduling failed\n");
        return 1;
    }

    printf("SJF Order: ");

    for (size_t i = 0; i < count; i++) {
        printf("P%d ", processes[order[i]].id);
    }

    printf("\n\nProcess\tCT\tTAT\tWT\tRT\n");

    for (size_t i = 0; i < count; i++) {
        Process p = processes[i];

        printf("P%d\t%lld\t%lld\t%lld\t%lld\n",
               p.id, p.completion_time,
               p.turnaround_time, p.waiting_time,
               p.response_time);
    }

    return 0;
}
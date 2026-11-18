
#include "process.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    Process processes[] = {
        {1, 0, 8, 0, 0, 0, 0, 0, 0},
        {2, 1, 4, 0, 0, 0, 0, 0, 0},
        {3, 2, 2, 0, 0, 0, 0, 0, 0}
    };

    size_t count = 3;
    size_t order[3];

    SRTFSegment timeline[9];
    size_t timeline_count = 0;

    if (schedule_srtf(
            processes, count, order,
            timeline, 9, &timeline_count) != 0) {
        printf("SRTF scheduling failed.\n");
        return 1;
    }

    printf("SRTF Completion Order: ");

    for (size_t i = 0; i < count; i++) {
        printf("P%d ", processes[order[i]].id);
    }

    printf("\n\nSRTF Gantt Chart:\n");

    if (timeline_count > 0) {
        printf("%lld", timeline[0].start_time);

        for (size_t i = 0; i < timeline_count; i++) {
            if (timeline[i].process_id == 0) {
                printf(" -- Idle -- %lld",
                       timeline[i].end_time);
            } else {
                printf(" -- P%d -- %lld",
                       timeline[i].process_id,
                       timeline[i].end_time);
            }
        }
    }

    printf("\n\nProcess\tCT\tTAT\tWT\tRT\n");

    for (size_t i = 0; i < count; i++) {
        Process p = processes[i];

        printf("P%d\t%lld\t%lld\t%lld\t%lld\n",
               p.id,
               p.completion_time,
               p.turnaround_time,
               p.waiting_time,
               p.response_time);
    }

    return 0;
}

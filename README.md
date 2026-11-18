# CPU Scheduling Algorithms

A command-line C program that runs four CPU scheduling policies over the same
process workload and compares their completion, waiting, turnaround, and
response metrics:

- **AAFS** — Adaptive Aging-Based Fair Scheduling (the proposed policy)
- **SJF** — non-preemptive Shortest Job First
- **SRTF** — preemptive Shortest Remaining Time First
- **HRRN** — non-preemptive Highest Response Ratio Next

## Problem statement

CPU scheduling decides which ready process receives the processor next.
Shortest-job policies can reduce waiting time, but may leave long jobs waiting
for an excessive amount of time. This project implements several policies on
identical input so their schedules and performance metrics can be compared.

## AAFS policy

AAFS is a **non-preemptive** policy with an automatically calculated, fixed
aging threshold. Before scheduling begins, it calculates:

```text
threshold = round(sum of all burst times / number of processes)
```

The average and rounding are computed with integer arithmetic; a half-way
fraction rounds upward. The threshold is based on the workload's average burst
time as a **heuristic** for choosing when to favor waiting processes. It is not
a mathematically optimal threshold.

At each dispatch:

1. Consider every arrived, unfinished process.
2. Compute its waiting time as `current time - arrival time`.
3. If any ready process has waited at least the threshold, select the one with
   the greatest waiting time.
4. If there are no aged ready processes, select the process with the shortest
   original burst time.
5. Resolve ties by earlier arrival time, then lower process ID.
6. Run the selected process to completion. AAFS never preempts a running job.
7. If no process is ready, advance the clock to the next arrival.

## Other algorithms

- **SJF:** chooses the ready process with the shortest original burst time and
  runs it to completion.
- **SRTF:** chooses the ready process with the shortest remaining time. It
  preempts when a process arrives with a shorter remaining time and records
  each execution interval for its Gantt chart.
- **HRRN:** chooses the ready process with the highest response ratio,
  `(waiting time + burst time) / burst time`, and runs it to completion.

## Scheduling metrics

For each process:

```text
Turnaround time (TAT) = completion time (CT) - arrival time (AT)
Waiting time (WT)     = turnaround time - burst time (BT)
Response time (RT)    = first start time - arrival time
```

Average waiting, turnaround, and response times are the corresponding
per-process totals divided by the number of processes. The program also reports
CPU utilization and throughput for each schedule.

## Project files

```text
main.c              Input validation, independent workload copies, and reports
process.h           Shared process/timeline types and scheduler declarations
aafs.c              Adaptive Aging-Based Fair Scheduling
sjf.c               Shortest Job First
srtf.c              Shortest Remaining Time First and execution timeline
hrrn.c              Highest Response Ratio Next
test_scheduling.c   Scenario and edge-case checks for all four schedulers
test_*.c            Standalone scheduler examples/tests
README.md           Project documentation
.gitignore          Generated executable and object-file exclusions
```

## Build and run

Compile the program with GCC:

```sh
gcc -std=c11 -Wall -Wextra -Wpedantic -g \
    main.c aafs.c sjf.c srtf.c hrrn.c -o cpu_scheduler
```

Run it:

```sh
./cpu_scheduler
```

Enter a positive process count, then one nonnegative arrival time and one
positive burst time for each process. The AAFS threshold is calculated and
displayed automatically; it is not requested as input.

### Sample

Input:

```text
3
0 8
1 4
2 2
```

AAFS output excerpt:

```text
AAFS automatic aging threshold: 5

             AAFS RESULTS
Process order: P1 -> P2 -> P3
Gantt chart:
0 -- P1 -- 8 -- P2 -- 12 -- P3 -- 14

Process  AT      BT      CT      TAT     WT      RT
P1      0       8       8       8       0       0
P2      1       4       12      11      7       7
P3      2       2       14      12      10      10
```

The program then prints full results and a comparison table for SJF, SRTF, and
HRRN using independent copies of the same workload.

## Tests

Build and run the focused regression tests:

```sh
gcc -std=c11 -Wall -Wextra -Wpedantic -g \
    test_scheduling.c aafs.c sjf.c srtf.c hrrn.c -o test_scheduling
./test_scheduling
```

The tests cover the normal workload (threshold 5), simultaneous arrivals,
idle CPU periods, a mixed long/short workload, all four algorithms' process
metrics and averages, process ordering, the SRTF execution timeline, empty
AAFS input, and large-burst threshold/completion-time overflow handling.
`test_hrrn.c`, `test_sjf.c`, and `test_srtf.c` are also retained as standalone
examples.

## Limitations and future improvements

- Times are integer time units, and each process must have a positive burst
  time and a nonnegative arrival time.
- Input is interactive and currently supports at most `INT_MAX` processes,
  subject to available memory.
- The average-burst AAFS threshold is a simple workload heuristic; different
  workloads may benefit from a separately tuned policy.
- Possible extensions include configurable tie-breaking, file-based workload
  input, and additional scheduling policies.

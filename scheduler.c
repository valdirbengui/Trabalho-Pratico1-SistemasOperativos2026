#include "scheduler.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "msg.h"

#define MLFQ_LEVELS 3
#define MLFQ_QUANTUM_MS 500
#define MLFQ_LEVEL_ALLOCATION_MS 1000
#define MLFQ_BOOST_INTERVAL_MS 5000

static const char *SCHED_NAMES[] = { "FIFO", "SJF", "RR", "MLFQ", NULL };
static sched_algo_en sched_algo = SCHED_FIFO;
static uint32_t last_mlfq_boost_ms = 0;

static const uint32_t mlfq_quantum_ms[MLFQ_LEVELS] = {
    MLFQ_QUANTUM_MS,
    MLFQ_QUANTUM_MS,
    MLFQ_QUANTUM_MS
};

int set_sched_algo(const char *name) {
    for (int i = 0; SCHED_NAMES[i] != NULL; i++) {
        if (strcasecmp(SCHED_NAMES[i], name) == 0) {
            sched_algo = (sched_algo_en)i;
            last_mlfq_boost_ms = 0;
            return sched_algo;
        }
    }

    return -1;
}

const char *get_sched_algo_str(void) {
    return SCHED_NAMES[sched_algo];
}

static void finish_burst(uint32_t current_time_ms, queue_t *cq, pcb_t *task) {
    msg_t msg = {
        .pid = task->pid,
        .request = PROCESS_REQUEST_DONE,
        .time_ms = current_time_ms
    };

    if (write(task->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t)) {
        perror("write");
    }

    enqueue_pcb(cq, task);
}

static pcb_t *dequeue_sjf(queue_t *rq) {
    if (!rq || !rq->head)
        return NULL;

    queue_elem_t *best = rq->head;
    queue_elem_t *elem = rq->head->next;

    while (elem) {
        if (elem->pcb->time_ms < best->pcb->time_ms) {
            best = elem;
        }

        elem = elem->next;
    }

    queue_elem_t *removed = remove_queue_elem(rq, best);

    if (!removed)
        return NULL;

    pcb_t *task = removed->pcb;
    free(removed);

    return task;
}

static pcb_t *dequeue_mlfq(queue_t *rq) {
    if (!rq || !rq->head)
        return NULL;

    queue_elem_t *best = rq->head;
    queue_elem_t *elem = rq->head->next;

    while (elem) {
        if (elem->pcb->mlfq_level < best->pcb->mlfq_level) {
            best = elem;
        }

        elem = elem->next;
    }

    queue_elem_t *removed = remove_queue_elem(rq, best);

    if (!removed)
        return NULL;

    pcb_t *task = removed->pcb;
    free(removed);

    return task;
}

static int mlfq_has_higher_priority(queue_t *rq, int current_level) {
    if (!rq)
        return 0;

    queue_elem_t *elem = rq->head;

    while (elem) {
        if (elem->pcb->mlfq_level < current_level) {
            return 1;
        }

        elem = elem->next;
    }

    return 0;
}

static void mlfq_boost(queue_t *rq, queue_t *cq, queue_t *bq,
                       pcb_t *cpu_task, uint32_t current_time_ms) {

    if (last_mlfq_boost_ms == 0) {
        last_mlfq_boost_ms = current_time_ms;
        return;
    }

    if (current_time_ms - last_mlfq_boost_ms < MLFQ_BOOST_INTERVAL_MS)
        return;

    /*
     * Rule 5:
     * Every process in the system is moved to the top queue.
     *
     * This includes:
     * - READY
     * - COMMAND
     * - BLOCKED
     * - CPU
     */

    queue_t *queues[] = { rq, cq, bq };

    for (size_t i = 0;
         i < sizeof(queues) / sizeof(queues[0]);
         i++) {

        queue_elem_t *elem = queues[i] ? queues[i]->head : NULL;

        while (elem) {
            elem->pcb->mlfq_level = 0;
            elem->pcb->mlfq_level_time_ms = 0;

            elem = elem->next;
        }
    }

    if (cpu_task) {
        cpu_task->mlfq_level = 0;
        cpu_task->mlfq_level_time_ms = 0;
    }

    last_mlfq_boost_ms = current_time_ms;

    printf("[MLFQ] Priority boost at %u ms: "
           "all processes moved to level 0\n",
           current_time_ms);
}

static void dispatch_task(uint32_t current_time_ms, pcb_t *task) {
    task->slice_start_ms = current_time_ms;

    if (task->first_run_time_ms == UINT32_MAX) {
        task->first_run_time_ms = current_time_ms;

        printf("[SCHED] PID %d first CPU at %u ms "
               "(response=%u ms)\n",
               task->pid,
               current_time_ms,
               current_time_ms - task->ready_since_ms);
    }
}

static void mlfq_demote_if_needed(pcb_t *task, uint32_t current_time_ms) {
    if (!task)
        return;

    if (task->mlfq_level_time_ms < MLFQ_LEVEL_ALLOCATION_MS)
        return;

    if (task->mlfq_level < MLFQ_LEVELS - 1) {

        task->mlfq_level++;

        /*
         * A new allocation starts at the new level.
         */
        task->mlfq_level_time_ms = 0;

        printf("[MLFQ] PID %d exhausted %u ms at level "
               "-> level %d at %u ms\n",
               task->pid,
               MLFQ_LEVEL_ALLOCATION_MS,
               task->mlfq_level,
               current_time_ms);

    } else {

        /*
         * The lowest level has nowhere else to go.
         * Start a new allocation period at the same level.
         */
        task->mlfq_level_time_ms = 0;

        printf("[MLFQ] PID %d exhausted allocation at "
               "lowest level %d at %u ms\n",
               task->pid,
               task->mlfq_level,
               current_time_ms);
    }
}

int scheduler(uint32_t current_time_ms,
              queue_t *rq,
              queue_t *cq,
              queue_t *bq,
              pcb_t **cpu_task) {

    /*
     * MLFQ priority boost.
     */
    if (sched_algo == SCHED_MLFQ) {
        mlfq_boost(
            rq,
            cq,
            bq,
            *cpu_task,
            current_time_ms
        );
    }

    /*
     * If there is a process running on the CPU.
     */
    if (*cpu_task) {

        /*
         * Update total CPU burst execution time.
         */
        (*cpu_task)->ellapsed_time_ms += TICKS_MS;

        /*
         * MLFQ:
         * accumulate the CPU time used at the current level.
         */
        if (sched_algo == SCHED_MLFQ) {

            (*cpu_task)->mlfq_level_time_ms += TICKS_MS;

            /*
             * IMPORTANT:
             *
             * Rule 4 says that when the allocation of a level
             * is exhausted, the process must immediately move
             * to the next level.
             *
             * Previously this was only checked when the quantum
             * expired or the burst ended, which could result in
             * values such as 1460/1000 ms.
             */
            if ((*cpu_task)->mlfq_level_time_ms
                >= MLFQ_LEVEL_ALLOCATION_MS) {

                mlfq_demote_if_needed(
                    *cpu_task,
                    current_time_ms
                );
            }
        }

        /*
         * Has the CPU burst finished?
         */
        if ((*cpu_task)->ellapsed_time_ms
            >= (*cpu_task)->time_ms) {

            /*
             * Rule 4 also applies when the process gives up
             * the CPU voluntarily at the end of a CPU burst.
             */
            if (sched_algo == SCHED_MLFQ) {
                mlfq_demote_if_needed(
                    *cpu_task,
                    current_time_ms
                );
            }

            finish_burst(
                current_time_ms,
                cq,
                *cpu_task
            );

            *cpu_task = NULL;
        }

        /*
         * Rule 1:
         * A lower-priority process cannot keep the CPU when
         * a higher-priority READY process exists.
         */
        else if (sched_algo == SCHED_MLFQ &&
                 mlfq_has_higher_priority(
                     rq,
                     (*cpu_task)->mlfq_level
                 )) {

            pcb_t *preempted = *cpu_task;

            mlfq_demote_if_needed(
                preempted,
                current_time_ms
            );

            printf(
                "[MLFQ] Higher-priority process ready: "
                "preempt PID %d at %u ms\n",
                preempted->pid,
                current_time_ms
            );

            enqueue_pcb(rq, preempted);

            *cpu_task = NULL;
        }

        /*
         * Rule 2:
         * Processes at the same priority use Round Robin.
         */
        else if ((sched_algo == SCHED_RR ||
                  sched_algo == SCHED_MLFQ) &&

                 current_time_ms -
                 (*cpu_task)->slice_start_ms >=

                 (sched_algo == SCHED_MLFQ
                      ? mlfq_quantum_ms[
                            (*cpu_task)->mlfq_level
                        ]
                      : TIME_SLICE_MS)) {

            pcb_t *preempted = *cpu_task;

            printf(
                "[SCHED] %s quantum expired for PID %d "
                "at %u ms",
                get_sched_algo_str(),
                preempted->pid,
                current_time_ms
            );

            if (sched_algo == SCHED_MLFQ) {

                printf(
                    " (level %d, level-used=%u/%u ms)",
                    preempted->mlfq_level,
                    preempted->mlfq_level_time_ms,
                    MLFQ_LEVEL_ALLOCATION_MS
                );
            }

            printf("\n");

            /*
             * Rule 4:
             * If the allocation of the current level was
             * exhausted, lower the priority before putting
             * the process back in READY.
             */
            if (sched_algo == SCHED_MLFQ) {
                mlfq_demote_if_needed(
                    preempted,
                    current_time_ms
                );
            }

            enqueue_pcb(rq, preempted);

            *cpu_task = NULL;
        }
    }

    /*
     * If the CPU is free, select the next process.
     */
    if (*cpu_task == NULL) {

        pcb_t *next = NULL;

        switch (sched_algo) {

            case SCHED_SJF:
                next = dequeue_sjf(rq);
                break;

            case SCHED_MLFQ:
                next = dequeue_mlfq(rq);
                break;

            case SCHED_FIFO:
            case SCHED_RR:
            default:
                next = dequeue_pcb(rq);
                break;
        }

        /*
         * Dispatch selected process.
         */
        if (next) {

            *cpu_task = next;

            dispatch_task(
                current_time_ms,
                next
            );

            printf(
                "[SCHED] %s dispatch PID %d at %u ms",
                get_sched_algo_str(),
                next->pid,
                current_time_ms
            );

            if (sched_algo == SCHED_MLFQ) {

                printf(
                    " (level %d, level-used=%u/%u ms)",
                    next->mlfq_level,
                    next->mlfq_level_time_ms,
                    MLFQ_LEVEL_ALLOCATION_MS
                );
            }

            printf("\n");

            return 1;
        }
    }

    return 0;
}
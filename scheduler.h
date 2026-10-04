#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "queue.h"

// Time-slice used by RR. MLFQ uses a 500 ms quantum per priority queue.
#define TIME_SLICE_MS 500

// Escalonadores disponíveis. No ponto de partida só FIFO está implementado;
// os alunos devem acrescentar SJF, RR e MLFQ.
typedef enum {
    SCHED_FIFO = 0,
    SCHED_SJF,
    SCHED_RR,
    SCHED_MLFQ,
} sched_algo_en;

int  set_sched_algo(const char *name);   // -1 se o nome for inválido
const char *get_sched_algo_str(void);

/**
 * @brief One scheduling step (called once per simulator tick).
 *
 * @param current_time_ms  current simulated time
 * @param rq   ready queue
 * @param cq   command queue (finished bursts go here to wait for the next instruction)
 * @param cpu_task  in/out: the PCB currently on the CPU (NULL if the CPU is idle)
 * @return 1 if a new task was placed on the CPU this step, 0 otherwise
 */
int scheduler(uint32_t current_time_ms, queue_t *rq, queue_t *cq, queue_t *bq,
               pcb_t **cpu_task);

#endif //SCHEDULER_H

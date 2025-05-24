#define _XOPEN_SOURCE 600 // Macro de compatibilidade para o uso de setitimer e sigaction
#include "ppos.h"
#include "ppos-core-globals.h"
#include "ppos-disk-manager.h"
#include <signal.h>    // Necessário para struct sigaction e sigaction()
#include <sys/time.h>  // Necessário para struct itimerval e setitimer()

// ****************************************************************************
// Adicione TUDO O QUE FOR NECESSARIO para realizar o seu trabalho
// Coloque as suas modificações aqui, 
// p.ex. includes, defines variáveis, 
// estruturas e funções
//
// ****************************************************************************

// #define DEBUG

unsigned int _systemTime = 0;

struct sigaction action;
struct itimerval timer;

int finalizadas = 0;
int numTasks = 0;
int flag = 0;

task_t * scheduler() { 
    if (!readyQueue) {
        if(!taskExec)
            return NULL;
        else
            return taskExec;
    }

    // if(readyQueue->id == 0){
    //     task_setprio(readyQueue, 20);
    // }
    task_t *aux = readyQueue->next;
    task_t *prioritaria = readyQueue;

    if(prioritaria->prio_din > aux->prio_din || (prioritaria->prio_din == aux->prio_din && task_getprio(aux) < task_getprio(prioritaria))){
        prioritaria = aux;
    }
    if(aux->id == 0){
        task_setprio(aux, 0);
    }
    while(aux != readyQueue) {
        aux = aux->next;
        if(prioritaria->prio_din > aux->prio_din || (prioritaria->prio_din == aux->prio_din && task_getprio(aux) < task_getprio(prioritaria))){
            prioritaria = aux;
        }
        if(aux->id == 0){
            task_setprio(aux, 0);
        }
    }

    // printf("Trocando de tarefa: %d -> %d\n", taskExec->id, prioritaria->id);
    // task_t *curr = readyQueue;
    // if (curr) {
    //     printf("Fila de prontas: ");
    //     do {
    //         printf("[id:%d prio_din:%d] ", curr->id, curr->prio_din);
    //         curr = curr->next;
    //     } while (curr && curr != readyQueue);
    // }
    // printf("\n");

    aux = readyQueue->next;

    if(aux != prioritaria && aux->prio_din > -20) {
        aux->prio_din--;
    }

    while(aux != readyQueue) {
        aux = aux->next;
        if(aux != prioritaria && aux->prio_din > -20) {
            aux->prio_din--;
        }
    }

    prioritaria->quantum = 40;
    prioritaria->prio_din = task_getprio(prioritaria);
    return prioritaria;
}

/**************************************************TIME*******************************************************************************/


void tratador_tick(int signum) {
    if(!taskExec)
        exit(-1);

    if(taskExec != taskDisp && taskExec != taskMain) {
        // Se houver uma tarefa em execução, decrementa seu quantum
        if (taskExec->quantum > 0) {
            taskExec->quantum--;
        }

        // Se o quantum chegar a zero, coloca a tarefa na fila de prontas
        if (taskExec->quantum == 0) {
            taskExec->state = PPOS_TASK_STATE_READY;
            taskExec->quantum = 40;
            taskExec->prio_din = task_getprio(taskExec);
            task_t* nextTask = scheduler();
            if(nextTask != taskExec) {
                queue_append((queue_t**)&readyQueue, (queue_t*)taskExec);
                nextTask = (task_t*)queue_remove((queue_t**)&readyQueue, (queue_t*)nextTask);
                task_switch(nextTask);
            }
        }
    }
    _systemTime++; // incrementa o relógio global a cada tick
}

unsigned int systime () {
    return _systemTime;
}

/**************************************************TIME*******************************************************************************/

/**************************************************PRIO*******************************************************************************/

void task_setprio (task_t *task, int prio) {
    if (task == NULL)
        task = taskExec;

    if (prio < -20 || prio > 20)
        exit(-1);

    task->prio_est = prio;
    task->prio_din = prio;
}

int task_getprio (task_t *task) {
    if (task == NULL)
        task = taskExec;

    return task->prio_est;
}

/**************************************************PRIO*******************************************************************************/



/*************************************************ppos_init*******************************************************************************/

void before_ppos_init () {
    // put your customization here
#ifdef DEBUG
    printf("\ninit - BEFORE");
#endif
    // registra a ação para o sinal de timer SIGALRM
    action.sa_handler = tratador_tick;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction(SIGALRM, &action, 0) < 0)
    {
        perror ("Erro em sigaction: ") ;
        exit (1) ;
    }

    // ajusta valores do temporizador para 1 ms
    timer.it_value.tv_usec = 1000;      // primeiro disparo, em mili-segundos (1 ms)
    timer.it_interval.tv_usec = 1000;   // disparos subsequentes, em mili-segundos (1 ms)

    // arma o temporizador ITIMER_REAL (vide man setitimer)
    if (setitimer(ITIMER_REAL, &timer, 0) < 0)
    {
        perror ("Erro em setitimer: ") ;
        exit (1) ;
    }
}

void after_ppos_init () {
    // put your customization here
#ifdef DEBUG
    printf("\ninit - AFTER");
#endif
    // printf("\ntaskDisp == NULL: %d", taskDisp == NULL);
    // printf("\ntaskExec == NULL: %d", taskExec == NULL);
    // printf("\ntaskMain == NULL: %d taskMain->id = %d\n", taskMain == NULL, taskMain->id);
    // // printf("\ntaskExec->id: %d\n", taskExec->id);
    // PRINT_READY_QUEUE
}

/*************************************************ppos_init*******************************************************************************/


/*************************************************task_create*****************************************************************************/

void before_task_create (task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_create - BEFORE - [%d]", task->id);
#endif
    
}

void after_task_create (task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_create - AFTER - [%d]", task->id);
#endif
    if(task->id == 1) {
        taskDisp = task;
    }
    if(task->id != 0 && task->id != 1) {
        if(flag != 0) {
            numTasks++;
        }
        else {
            flag = 1;
        }
    }
    task->start_time = (int)systime();
    if(task->id == 1) {
        printf("System time: %d ms task->start_time: %d\n", systime(), task->id);
    }
    task->end_time = 0;
    task->start_processor = 0;
    task->finished = 0;
    if(task->id != 1 && task->id != 0) {
        task->processor_time = 0;
        task->activations = 0;
        task->quantum = 40;
    }
    // printf("\ntask->start_processor: %d task->activations: %d task->quantum: %d", task->start_processor, task->activations, task->quantum); 
}

/*************************************************task_create*****************************************************************************/

/**************************************************task_exit******************************************************************************/

void before_task_exit () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_exit - BEFORE - [%d]", taskExec->id);
#endif
}

void after_task_exit () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_exit - AFTER- [%d]", taskExec->id);
#endif
    if(taskExec->id != 1) {
        taskExec->end_time = (int)systime();
        finalizadas++;
    }
    else {
        printf("Task->start_time: %d\n", taskExec->start_time);
    }
    taskExec->finished = 1;
    printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n", taskExec->id, taskExec->end_time - taskExec->start_time, taskExec->processor_time, taskExec->activations);
}

/**************************************************task_exit******************************************************************************/

/*************************************************task_switch*****************************************************************************/

void before_task_switch ( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_switch - BEFORE - [%d -> %d]", taskExec->id, task->id);
#endif
    if(taskExec->id == task->id) {
        task_switch(readyQueue);
    }
    if(taskExec->id != 1 && taskExec->id != 0 && taskExec->finished == 0) {
        // printf("Task %d switch: start processor %d, system time %d, processor time %d ms\n", taskExec->id, taskExec->start_processor, (int)systime(), taskExec->processor_time);
        taskExec->processor_time += ((int)systime() - taskExec->start_processor);
        task->start_processor = (int)systime();
        task->activations++;
        task->quantum = 40;
    }
}

void after_task_switch ( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_switch - AFTER - [%d -> %d]", taskExec->id, task->id);
#endif
    // PRINT_READY_QUEUE;
    // printf("finalizadas: %d numTasks: %d\n", finalizadas, numTasks);
    if(finalizadas > numTasks && queue_size((queue_t*)readyQueue) == 1 && taskExec->id == 1) {
        task_exit(0);
    }
}

/*************************************************task_switch*****************************************************************************/


/**************************************************task_yield*****************************************************************************/

void before_task_yield () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_yield - BEFORE - [%d]", taskExec->id);
#endif
}


void after_task_yield () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_yield - AFTER - [%d]", taskExec->id);
#endif
}


/**************************************************task_yield*****************************************************************************/


void before_task_suspend( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_suspend - BEFORE - [%d]", task->id);
#endif
}

void after_task_suspend( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_suspend - AFTER - [%d]", task->id);
#endif
}

void before_task_resume(task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_resume - BEFORE - [%d]", task->id);
#endif
}

void after_task_resume(task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_resume - AFTER - [%d]", task->id);
#endif
}

void before_task_sleep () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_sleep - BEFORE - [%d]", taskExec->id);
#endif
}

void after_task_sleep () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_sleep - AFTER - [%d]", taskExec->id);
#endif
}

int before_task_join (task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_join - BEFORE - [%d] espera tarefa %d", taskExec->id, task->id);
#endif
    
    return 0;
}

int after_task_join (task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_join - AFTER - [%d] espera tarefa %d", taskExec->id, task->id);
#endif
    return 0;
}


int before_sem_create (semaphore_t *s, int value) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_create (semaphore_t *s, int value) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_down (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_down - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_down (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_down - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_up (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_up - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_up (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_up - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_destroy (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_destroy (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_create (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_create (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_lock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_lock - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_lock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_lock - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_unlock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_unlock - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_unlock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_unlock - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_destroy (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_destroy (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_create (barrier_t *b, int N) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_create (barrier_t *b, int N) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_join (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_join - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_join (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_join - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_destroy (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_destroy (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_create (mqueue_t *queue, int max, int size) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_create (mqueue_t *queue, int max, int size) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_send (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_send - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_send (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_send - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_recv (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_recv - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_recv (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_recv - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_destroy (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_destroy (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_msgs (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_msgs - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_msgs (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_msgs - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}
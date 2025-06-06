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


unsigned int _systemTime = 0;

struct sigaction action;
struct itimerval timer;

int finalizadas = 0;

// Variáveis globais para os dados do dispatcher
int start_time_dispatcher = 0, end_time_dispatcher = 0, activations_disp = 0, processor_time_disp = 0, start_processor_disp = 0, finished_disp = 0;

// Variavel para saber se está usando o pingpong-scheduler.c
#ifdef SCHEDULER_MODE
    int modo_scheduler = SCHEDULER_MODE;
#else
    int modo_scheduler = 0;
#endif

task_t * scheduler() { 
    if (!readyQueue) {
        if(!taskExec)
            return NULL;
        else
            return taskExec;
    }

    // Escolhe a tarefa com menor prioridade dinâmica (mais importante), e se tiver duas tarefas com a menor prioridade dinamica, escolhe a de menor prioridade estática (mais importante)
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

    // Envelhece as tarefas na fila de prontas, tirando a escolhida, decrementando a prioridade dinâmica delas (desde que continue sendo >= -20)
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

    prioritaria->quantum = 20;  // reinicia o quantum da tarefa escolhida
    prioritaria->prio_din = task_getprio(prioritaria); // reinicia a prioridade da tarefa escolhida
    return prioritaria;
}

/**************************************************TIME*******************************************************************************/


void tratador_tick(int signum) {
    if(!taskExec)
        exit(-1);

    if(taskExec != taskDisp) {
        // Se houver uma tarefa em execução, decrementa seu quantum
        if (taskExec->quantum > 0) {
            taskExec->quantum--;
        }

        // Se o quantum chegar a zero, coloca a tarefa na fila de prontas e chama o dispatcher
        if (taskExec->quantum == 0) {
            task_yield();
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
    // printf("taskDisp == NULL: %d\n", taskDisp == NULL);
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
    // Inicializa os campos da tarefa criada
    if(task->id != 1 && task->id != 0) {
        task->start_time = (int)systime();
        task->end_time = 0;
        task->start_processor = 0;
        task->finished = 0;
        task->processor_time = 0;
        task->activations = 0;
        task->quantum = 20;
    }
    else if (task->id == 1) {
        start_processor_disp = (int)systime();
    }
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
    if (taskExec->id != 1 && taskExec->id != 0) {
        // Marca a tarefa como finalizada
        taskExec->finished = 1;
        
        // Conta o número de tarefas finalizadas e o tempo final delas (tirando o dispatcher e a main)
        taskExec->end_time = (int)systime();
        finalizadas++;
        
        // Imprime as contabilizações de cada tarefa (separado do dispatcher porque seus campos da TCB tem comportamento anômalo)
        if(modo_scheduler == 0) {
            printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n", taskExec->id, taskExec->end_time - taskExec->start_time, taskExec->processor_time, taskExec->activations);
        }
    }
    // Se a tarefa for o dispatcher, imprime as contabilizações dela (separado do resto porque seus campos da TCB tem comportamento anômalo)
    if(taskExec->id == 1) {
        end_time_dispatcher = (int)systime();
        finished_disp = 1;
        if(modo_scheduler == 0) {
            printf("Task 1 exit: execution time %d ms, processor time %d ms, %d activations\n", end_time_dispatcher - start_time_dispatcher , processor_time_disp, activations_disp);
        }
    }
}

/**************************************************task_exit******************************************************************************/

/*************************************************task_switch*****************************************************************************/

void before_task_switch ( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_switch - BEFORE - [%d -> %d]", taskExec->id, task->id);
#endif
    // Se a tarefa que está parando de executar e a que vai começar a executar não forem o dispatcher nem a main
    if(taskExec->id != 1 && taskExec->id != 0 && taskExec->finished == 0) {
        // Incrementa o tempo de processamento da tarefa usando o tempo que ela começou a executar e o tempo atual do processador
        taskExec->processor_time += ((int)systime() - taskExec->start_processor);
    }

    // Se a tarefa que está parando de executar e a que vai começar a executar não forem o dispatcher nem a main
    if(task->id != 1 && task->id != 0 && task->finished == 0) {
        // Guarda o tempo de início da tarefa no processador e incrementa o número de ativações dela
        task->start_processor = (int)systime();
        task->activations++;
    }

    // Se a tarefa atual for o dispatcher, incrementa o tempo de processamento dela
    if(taskExec->id == 1 && finished_disp == 0) {
        processor_time_disp += ((int)systime() - start_processor_disp);
    }

    // Se a tarefa que está sendo chamada for o dispatcher, inicializa o tempo de início dela no processador e o numero de ativações dela
    if(task->id == 1 && finished_disp == 0) {
        start_processor_disp = (int)systime();
        activations_disp++;
    }
}

void after_task_switch ( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_switch - AFTER - [%d -> %d]", taskExec->id, task->id);
#endif
    // Se a tarefa atual for o dispatcher e tiver só o dispatcher e a main na fila de prontas finaliza o dispatcher
    if(countTasks <= 1 && finalizadas > 0 && taskExec->id == 1) {
        task_exit(0);
    }
}

/*************************************************task_switch*****************************************************************************/

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
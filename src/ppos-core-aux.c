#define _XOPEN_SOURCE 600 // Macro de compatibilidade para o uso de setitimer e sigaction
#include "ppos.h"
#include "ppos-core-globals.h"
#include "ppos-disk-manager.h"
#include <signal.h>    // Necessário para struct sigaction e sigaction()
#include <sys/time.h>  // Necessário para struct itimerval e setitimer()
#include <limits.h> // Necessário para INT_MAX

// ****************************************************************************
// Adicione TUDO O QUE FOR NECESSARIO para realizar o seu trabalho
// Coloque as suas modificações aqui, 
// p.ex. includes, defines variáveis, 
// estruturas e funções
//
// ****************************************************************************

// #define DEBUG 1

unsigned int _systemTime = 0;

struct sigaction action;
struct itimerval timer;

/*************************************************PARTE A*****************************************************************************/

int finalizadas = 0;

// Variáveis globais para os dados do dispatcher
int start_time_dispatcher = 0, end_time_dispatcher = 0, activations_disp = 0, processor_time_disp = 0, start_processor_disp = 0, finished_disp = 0;
// Variáveis globais para os dados da main
int start_time_main = 0, end_time_main = 0, activations_main = 0, processor_time_main = 0, start_processor_main = 0, finished_main = 0;

int termina = 1;

// Variavel para saber se está usando o pingpong-scheduler.c
#ifdef SCHEDULER_MODE
    int modo_scheduler = SCHEDULER_MODE;
#else
    int modo_scheduler = 0;
#endif

#ifdef PARTE_A
    int modo_parte_a = 1;
#else
    int modo_parte_a = 0;
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

/*************************************************PARTE A*****************************************************************************/


/*************************************************PARTE B*****************************************************************************/

// Definições de políticas de disco como flags de compilação

#define MODO_FCFS   0
#define MODO_SSTF   1
#define MODO_CSCAN  2

#ifdef FCFS
int politica_disco = MODO_FCFS;
#elif defined(SSTF)
int politica_disco = MODO_SSTF;
#elif defined(CSCAN)
int politica_disco = MODO_CSCAN;
#else
int politica_disco = MODO_FCFS; // padrão FCFS se nenhuma flag for definida
#endif

int cabeca_do_disco = 1;

int blocos_percorridos = 0;

int anterior = 0;

diskrequest_t* disk_scheduler_fcfs(diskrequest_t* request) {
    return request;
}


diskrequest_t* disk_scheduler_sstf(diskrequest_t* request, int* dist) {
    int menor_dist = INT_MAX;
    diskrequest_t* retorno = NULL;
    for (diskrequest_t* r = request; r != request || retorno == NULL; r = r->next) {
        *dist = abs(r->block - anterior);
        if (*dist < menor_dist) {
            menor_dist = *dist;
            retorno = r;
        }
    }
    return retorno;
}


diskrequest_t* disk_scheduler_cscan(diskrequest_t* request) {
    diskrequest_t* escolhido = NULL;
    diskrequest_t* retorno = NULL;
    int menor_dist = INT_MAX;
    int encontrou = 0;

    diskrequest_t* r = request;
    int head = anterior;
    // printf("\nCSCAN, com head em %d", head);
    do {
        if (r->block >= head && (r->block - head) < menor_dist) {
            menor_dist = r->block - head;
            escolhido = r;
            encontrou = 1;
        }
        r = r->next;
    } while (r != request);

    if (!encontrou) {
        blocos_percorridos += 255 + (255 - head);
        menor_dist = INT_MAX;

        r = request;
        do {
            if (r->block < menor_dist) {
                menor_dist = r->block;
                escolhido = r;
            }
            r = r->next;
        } while (r != request);
        anterior = 0;
    }
    retorno = escolhido;

    return retorno;
}



diskrequest_t* disk_scheduler(diskrequest_t* request) {
    int dist;
    if (!request)
        return NULL;

    diskrequest_t* retorno = NULL;

    if (politica_disco == MODO_FCFS) {
        retorno = disk_scheduler_fcfs(request);
    } else if (politica_disco == MODO_SSTF) {
        retorno = disk_scheduler_sstf(request, &dist);
    } else if (politica_disco == MODO_CSCAN) {
        retorno = disk_scheduler_cscan(request);
    } else {
        printf("\nErro: politica de disco invalida.\n");
        exit(1);
    }

    dist = abs(retorno->block - anterior);
    anterior = retorno->block;
    blocos_percorridos += dist;

    return retorno;
}


/*************************************************PARTE B*****************************************************************************/


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
    } else if (task->id == 0) {
        start_time_main = (int)systime();
    } else if (task->id == 1) {
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
            if(modo_parte_a == 0 && taskExec->id != 2)
                printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n", taskExec->id, taskExec->end_time - taskExec->start_time, taskExec->processor_time, taskExec->activations);
        }
    }

    // Se a tarefa atual for a main, marca ela como finalizada e imprime as contabilizações dela
    if(taskExec->id == 0) {
        finished_main = 1;
        end_time_main = (int)systime();
        finalizadas++;
        if(modo_scheduler == 0) {
            printf("Task 0 exit: execution time %d ms, processor time %d ms, %d activations\n", end_time_main - start_time_main, processor_time_main, activations_main);
        }
    }

    // Se a tarefa for o dispatcher, imprime as contabilizações dela (separado do resto porque seus campos da TCB tem comportamento anômalo)
    if(taskExec->id == 1) {
        end_time_dispatcher = (int)systime();
        finished_disp = 1;
        if(modo_scheduler == 0) {
            printf("Task 1 exit: execution time %d ms, processor time %d ms, %d activations\n", end_time_dispatcher - start_time_dispatcher , processor_time_disp, activations_disp);
            printf("\nA quantidade de blocos percorridos foi: %d", blocos_percorridos);
        }
    }
}

/**************************************************task_exit******************************************************************************/

/*************************************************task_switch*****************************************************************************/

void before_task_switch ( task_t *task ) {
    // put your customization here
    if(task == NULL) {
        printf("\nErro: task_switch recebeu uma tarefa nula.\n");
        exit(1);
    }
#ifdef DEBUG
    printf("\ntask_switch - BEFORE - [%d -> %d]", taskExec->id, task->id);
#endif

/*****************************************TAREFA ATUAL QUE ESTÁ SAINDO********************************************************************/

    // Se a tarefa que está parando de executar não for o dispatcher nem a main
    if(taskExec->id != 1 && taskExec->id != 0 && taskExec->finished == 0) {
        taskExec->processor_time += ((int)systime() - taskExec->start_processor);
    } 
    // Se a tarefa que está parando de executar for a main
    else if(taskExec->id == 0 && finished_main == 0) {
        processor_time_main += ((int)systime() - start_processor_main);
    } 
    // Se a tarefa que está parando de executar for o dispatcher, 
    else if(taskExec->id == 1 && finished_disp == 0) {
        if(task->id != 2 || disco.sinal) {
            processor_time_disp += ((int)systime() - start_processor_disp);
        }
    }
/*****************************************TAREFA ATUAL QUE ESTÁ SAINDO********************************************************************/
// [<--bloco 0000

/*************************************TAREFA QUE ESTÁ COMEÇANDO A EXECUTAR****************************************************************/

    // Se a tarefa que está começando a executar não for o dispatcher nem a main
    if(task->id != 1 && task->id != 0 && task->finished == 0) {
        task->start_processor = (int)systime();
        task->activations++;
    }
    // Se a tarefa que está começando a executar for a main, guarda o tempo de início dela no processador e incrementa o número de ativações dela
    else if(task->id == 0 && finished_main == 0) {
        start_processor_main = (int)systime();
        activations_main++;
    }
    // Se a tarefa que está começando a executar for o dispatcher, guarda o tempo de início dela no processador e incrementa o número de ativações dela
    else if(task->id == 1 && finished_disp == 0) {
        if(taskExec->id != 2 || disco.sinal) {
            start_processor_disp = (int)systime();
            activations_disp++;
        }
    }

/*************************************TAREFA QUE ESTÁ COMEÇANDO A EXECUTAR****************************************************************/
}

void after_task_switch ( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_switch - AFTER - [%d -> %d]", taskExec->id, task->id);
#endif
    if(modo_parte_a) {
        // Se a tarefa atual for o dispatcher e tiver só o dispatcher e a main na fila de prontas finaliza o dispatcher
        if(countTasks <= 1 && finalizadas > 0 && taskExec->id == 1) {
            task_exit(0);
        }
    } else {
        if(countTasks <= 2 && taskExec->id == 1 && finalizadas > 0 && termina) {
            termina = 0;
            taskExec = &taskDiskMgr;
            task_exit(0);
        }
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

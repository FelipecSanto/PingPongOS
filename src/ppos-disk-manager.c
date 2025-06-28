#define _XOPEN_SOURCE 600 // Macro de compatibilidade para o uso de setitimer e sigaction
#include "ppos-disk-manager.h"

// Variáveis globais
disk_t disco;
task_t taskDiskMgr;

// Protótipos
void bodyDiskManager(void *arg);
void diskSignalHandler(int signum);
// void clean_exit_on_sig(int sig_num);

// Inicialização do gerente de disco
int disk_mgr_init(int *numBlocks, int *blockSize) {
    int qtdBlocos, tamBloco;

    if (disk_cmd(DISK_CMD_INIT, 0, 0) < 0)
        return -1;

    qtdBlocos = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
    tamBloco = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);

    if (qtdBlocos < 0 || tamBloco < 0)
        return -1;

    *numBlocks = qtdBlocos;
    *blockSize = tamBloco;

    // Inicializa disco
    disco.numBlocks = qtdBlocos;
    disco.blockSize = tamBloco;
    disco.diskQueue = NULL;
    disco.requestQueue = NULL;
    disco.livre = 1;
    disco.sinal = 0;

    sem_create(&disco.semaforo, 1);
    sem_create(&disco.semaforo_queue, 1);

    // taskDiskMgr é global
    task_create(&taskDiskMgr, bodyDiskManager, NULL);

    // Handler de sinal do disco
    struct sigaction diskAction;
    diskAction.sa_handler = diskSignalHandler;
    sigemptyset(&diskAction.sa_mask);
    diskAction.sa_flags = 0;
    if (sigaction(SIGUSR1, &diskAction, NULL) < 0) {
        perror("Erro em sigaction: ");
        exit(1);
    }
    // signal(SIGSEGV, clean_exit_on_sig);

    return 0;
}

// Leitura de bloco
int disk_block_read(int block, void *buffer) {
    if (sem_down(&disco.semaforo) < 0)
        return -1;

    diskrequest_t* req = malloc(sizeof(diskrequest_t));
    req->next = req->prev = NULL;
    req->task = taskExec;
    req->operation = 1; // DISK_REQUEST_READ
    req->block = block;
    req->buffer = buffer;

    sem_down(&disco.semaforo_queue);
    queue_append((queue_t**)&disco.requestQueue, (queue_t*)req);
    sem_up(&disco.semaforo_queue);

    if (taskDiskMgr.state == 'S')
        task_resume(&taskDiskMgr);

    sem_up(&disco.semaforo);

    task_suspend(taskExec, &disco.diskQueue);
    task_yield();

    return 0;
}

// Escrita de bloco
int disk_block_write(int block, void *buffer) {
    if (sem_down(&disco.semaforo) < 0)
        return -1;

    diskrequest_t* req = malloc(sizeof(diskrequest_t));
    req->next = req->prev = NULL;
    req->task = taskExec;
    req->operation = 2; // DISK_REQUEST_WRITE
    req->block = block;
    req->buffer = buffer;

    sem_down(&disco.semaforo_queue);
    queue_append((queue_t**)&disco.requestQueue, (queue_t*)req);
    sem_up(&disco.semaforo_queue);

    if (taskDiskMgr.state == 'S')
        task_resume(&taskDiskMgr);

    sem_up(&disco.semaforo);

    task_suspend(taskExec, &disco.diskQueue);
    task_yield();

    return 0;
}

// Corpo do gerente de disco
void bodyDiskManager(void *arg) {
    while (1) {
        sem_down(&disco.semaforo);

        if (disco.sinal) {
            disco.sinal = 0;
            task_resume(disco.diskQueue);
            disco.livre = 1;
        }

        if (disco.livre && disco.requestQueue) {
            diskrequest_t* req = disk_scheduler(disco.requestQueue);

            if (req) {
                sem_down(&disco.semaforo_queue);
                queue_remove((queue_t**)&disco.requestQueue, (queue_t*)req);
                sem_up(&disco.semaforo_queue);

                if (req->operation == 1) {
                    disk_cmd(DISK_CMD_READ, req->block, req->buffer);
                    disco.livre = 0;
                } else if (req->operation == 2) {
                    disk_cmd(DISK_CMD_WRITE, req->block, req->buffer);
                    disco.livre = 0;
                }
                free(req);
            }
        }

        sem_up(&disco.semaforo);
        task_yield();
    }
}

// Handler de sinal do disco
void diskSignalHandler(int signum) {
    disco.sinal = 1;
}

// // Handler para SIGSEGV
// void clean_exit_on_sig(int sig_num) {
//     int err = errno;
//     printf("\n[DEBUG] clean_exit_on_sig chamado!\n");
//     fprintf(stderr, "\n ERROR[Signal = %d]: %d \"%s\"\n", sig_num, err, strerror(err));
//     printf("\n[DEBUG] clean_exit_on_sig chamado!\n");
//     exit(err);
// }
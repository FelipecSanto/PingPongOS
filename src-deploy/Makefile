# Variáveis
# Define "main" como valor padrão, mas permite sobrescrever
TARGET ?= main
OBJS = ppos-all.o queue.o
CC = gcc
CFLAGS = -Wall -g
SRCS = ppos-core-aux.c

# Alvos para cada teste
scheduler: pingpong-scheduler.c $(SRCS) $(OBJS)
	$(CC) -DSCHEDULER_MODE=1 -o scheduler pingpong-scheduler.c $(SRCS) $(OBJS)
	./scheduler
	rm -f scheduler

preempcao: pingpong-preempcao.c $(SRCS) $(OBJS)
	$(CC) -DSCHEDULER_MODE=1 -o preempcao pingpong-preempcao.c $(SRCS) $(OBJS)
	./preempcao
	rm -f preempcao

preempcao-stress: pingpong-preempcao-stress.c $(SRCS) $(OBJS)
	$(CC) -DSCHEDULER_MODE=1 -o preempcao-stress pingpong-preempcao-stress.c $(SRCS) $(OBJS)
	./preempcao-stress
	rm -f preempcao-stress

contab-prio: pingpong-contab-prio.c $(SRCS) $(OBJS)
	$(CC) -o contab-prio pingpong-contab-prio.c $(SRCS) $(OBJS)
	./contab-prio
	rm -f contab-prio

# Regra padrão
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) pingpong-contab-prio.c $(SRCS) $(OBJS)
	./$(TARGET)
	rm -f $(TARGET)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

# Observações:
# - O nome padrão do alvo é "main", mas pode ser alterado sobrescrevendo a variável TARGET.
# - Para executar o Makefile:
#   - Apenas "make" compila e executa o alvo com o nome padrão.
#   - "make TARGET=nome-desejado" compila e executa um alvo com o nome especificado.
# - O alvo é um executável que é gerado a partir da compilação.

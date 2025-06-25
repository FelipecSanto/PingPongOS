# Variáveis
TARGET ?= main
CC = gcc
CFLAGS = -Wall -g -Iinclude/utils -Iinclude/parteB
SRCS = $(wildcard src/*.c)
OBJSDIR = objects
OBJS = $(patsubst src/%.c,$(OBJSDIR)/%.o,$(SRCS)) $(wildcard objectsbyProf/*.o)

# Garante que a pasta objects existe
$(shell mkdir -p objects)

# Alvos para cada teste
scheduler: src/parteA/pingpong-scheduler.c $(OBJS)
	@$(CC) -DSCHEDULER_MODE=1 -o scheduler src/parteA/pingpong-scheduler.c $(OBJS) $(CFLAGS)
	@./scheduler || (echo "Deu ruim"; rm -f scheduler)
	@rm -f scheduler

preempcao: src/parteA/pingpong-preempcao.c $(OBJS)
	@$(CC) -DSCHEDULER_MODE=1 -o preempcao src/parteA/pingpong-preempcao.c $(OBJS) $(CFLAGS)
	@./preempcao || (echo "Deu ruim"; rm -f preempcao)
	@rm -f preempcao

preempcao-stress: src/parteA/pingpong-preempcao-stress.c $(OBJS)
	@$(CC) -DSCHEDULER_MODE=1 -o preempcao-stress src/parteA/pingpong-preempcao-stress.c $(OBJS) $(CFLAGS)
	@./preempcao-stress || (echo "Deu ruim"; rm -f preempcao-stress)
	@rm -f preempcao-stress

contab-prio: src/parteA/pingpong-contab-prio.c $(OBJS)
	@$(CC) -o contab-prio src/parteA/pingpong-contab-prio.c $(OBJS) $(CFLAGS)
	@./contab-prio || (echo "Deu ruim"; rm -f contab-prio)
	@rm -f contab-prio

disc1: src/parteB/pingpong-disco1.c $(OBJS)
	@$(CC) -o disc1 src/parteB/pingpong-disco1.c $(OBJS) $(CFLAGS) -lrt
	@./disc1 || (echo "Deu ruim"; rm -f disc1)
	@rm -f disc1
	@cp disk_original.dat disk.dat

disc2: src/parteB/pingpong-disco2.c $(OBJS)
	@cp disk_original.dat disk.dat
	@$(CC) -o disc2 src/parteB/pingpong-disco2.c $(OBJS) $(CFLAGS) -lrt
	@./disc2 > saida_terminal.txt || (echo "Deu ruim"; rm -f disc2)
	@rm -f disc2
	@cp disk_original.dat disk.dat

restore-disk:
	@cp disk_original.dat disk.dat


# prodcons: src/parteB/pingpong-prodcons.c $(OBJS)
# 	@$(CC) -o prodcons src/parteB/pingpong-prodcons.c $(OBJS) $(CFLAGS) -lrt
# 	@./prodcons || (echo "Deu ruim"; rm -f prodcons)
# 	@rm -f prodcons

# racecond: src/parteB/pingpong-racecond.c $(OBJS)
# 	@$(CC) -o racecond src/parteB/pingpong-racecond.c $(OBJS) $(CFLAGS) -lrt
# 	@./racecond || (echo "Deu ruim"; rm -f racecond)
# 	@rm -f racecond

# Regra padrão
all: $(TARGET)

$(TARGET): $(OBJS)
	@$(CC) -o $(TARGET) src/parteA/pingpong-contab-prio.c $(OBJS) $(CFLAGS)
	@./$(TARGET)
	@rm -f $(TARGET)

objects/%.o: src/%.c
	@$(CC) -c $< -o $@ $(CFLAGS)

clean:
	@rm -f objects/*.o $(TARGET) scheduler preempcao preempcao-stress contab-prio

# Observações:
# - O nome padrão do alvo é "main", mas pode ser alterado sobrescrevendo a variável TARGET.
# - Para executar o Makefile:
#   - Apenas "make" compila e executa o alvo com o nome padrão.
#   - "make TARGET=nome-desejado" compila e executa um alvo com o nome especificado.
# - O alvo é um executável que é gerado a partir da compilação.
# Variáveis
# Define "main" como valor padrão, mas permite sobrescrever
TARGET ?= main
OBJS = ppos-all.o queue.o
CC = gcc
CFLAGS = -Wall -g
SRCS = pingpong-preempcao.c ppos-core-aux.c

# Regra padrão
all: $(TARGET)

# Regra para compilar o executável e executar automaticamente
$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(SRCS) $(OBJS)
	./$(TARGET)
	if [ -f $(TARGET) ]; then \
		rm -f $(TARGET); \
	fi

# Regra para compilar arquivos .o
%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

# Observações:
# - O nome padrão do alvo é "main", mas pode ser alterado sobrescrevendo a variável TARGET.
# - Para executar o Makefile:
#   - Apenas "make" compila e executa o alvo com o nome padrão.
#   - "make TARGET=nome-desejado" compila e executa um alvo com o nome especificado.
# - O alvo é um executável que é gerado a partir da compilação.

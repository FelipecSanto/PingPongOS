# Variáveis principais
CC      = gcc
CFLAGS  = -Wall -g -Iinclude/utils -Iinclude/parteB
OBJSDIR = objects
PROFOBJDIR = objectsbyProf
OBJS    = $(wildcard $(OBJSDIR)/*.o) $(wildcard $(PROFOBJDIR)/*.o)
PARTEA  = src/parteA
PARTEB  = src/parteB

# Garante que a pasta objects existe
$(shell mkdir -p $(OBJSDIR))

# Compilação dos objetos principais
$(OBJSDIR)/ppos-core-aux.o: src/ppos-core-aux.c
	@$(CC) -c $< -o $@ $(CFLAGS) $(DISKFLAG)

$(OBJSDIR)/ppos-disk-manager.o: src/ppos-disk-manager.c
	@$(CC) -c $< -o $@ $(CFLAGS) $(DISKFLAG)

# Alvos de testes de disco (Parte B)
d1-fcfs:   DISKFLAG = -DFCFS=1
d1-sstf:   DISKFLAG = -DSSTF=1
d1-cscan:  DISKFLAG = -DCSCAN=1
d2-fcfs:   DISKFLAG = -DFCFS=1
d2-sstf:   DISKFLAG = -DSSTF=1
d2-cscan:  DISKFLAG = -DCSCAN=1

d1-fcfs d1-sstf d1-cscan: clean $(OBJS)
	@cp disk_original.dat disk.dat
	@$(CC) $(DISKFLAG) -o disc1 $(PARTEB)/pingpong-disco1.c $(OBJS) $(CFLAGS) -lrt
	@./disc1 > saida_terminal_disk1.txt || (echo "Deu ruim"; rm -f disc1)
	@rm -f disc1
	@cp disk_original.dat disk.dat

d2-fcfs d2-sstf d2-cscan: clean $(OBJS)
	@cp disk_original.dat disk.dat
	@$(CC) $(DISKFLAG) -o disc2 $(PARTEB)/pingpong-disco2.c $(OBJS) $(CFLAGS) -lrt
	@./disc2 > saida_terminal_disk2.txt || (echo "Deu ruim"; rm -f disc2)
	@rm -f disc2
	@cp disk_original.dat disk.dat

# Alvos de testes de escalonador e preempção (Parte A)
scheduler:   DISKFLAG = -DSCHEDULER_MODE=1 -DPARTE_A=1
preempcao:   DISKFLAG = -DSCHEDULER_MODE=1 -DPARTE_A=1
preempcao-stress: DISKFLAG = -DSCHEDULER_MODE=1 -DPARTE_A=1
contab-prio: DISKFLAG = -DPARTE_A=1

scheduler preempcao preempcao-stress contab-prio: clean $(OBJS)
	@$(CC) $(DISKFLAG) -o $@ $(PARTEA)/pingpong-$@.c $(OBJS) $(CFLAGS)
	@./$@ || (echo "Deu ruim"; rm -f $@)
	@rm -f $@


# Restaurar disco
restore-disk:
	@cp disk_original.dat disk.dat

# Limpeza
clean:
	@rm -f $(OBJSDIR)/*.o

# Alvo padrão
all: contab-prio

.PHONY: clean restore-disk all


# # Debug alvo
# debug-d2-fcfs: DISKFLAG = -DFCFS=1
# debug-d2-fcfs: clean $(OBJS)
# 	@cp disk_original.dat disk.dat
# 	@$(CC) -g $(DISKFLAG) -o disc2 $(PARTEB)/pingpong-disco2.c $(OBJS) $(CFLAGS) -lrt
# 	@gdb ./disc2
#-include ../../common.mk
#CHARMC=../../../bin/charmc $(OPTS)
#CHARMC=/Users/kale/software/taskqCharm/charm/bin/charmc $(OPTS)

OBJS = tasksAndRing.o

all: tasksAndRing

tasksAndRing: $(OBJS)
	$(CHARMC) -O3 -language charm++ -tracemode projections -o tasksAndRing $(OBJS)

tasksAndRing.decl.h: tasksAndRing.ci
	$(CHARMC)  tasksAndRing.ci

clean:
	rm -f *.decl.h *.def.h conv-host *.o tasksAndRing charmrun *.log *.sum *.sts tasksAndRing.exe tasksAndRing.pdb tasksAndRing.ilk

tasksAndRing.o: tasksAndRing.C tasksAndRing.decl.h
	$(CHARMC) -O3 -c tasksAndRing.C

test: all
	$(call run, ./tasksAndRing +p4 10 )

testp: all
	$(call run, ./tasksAndRing +p$(P) $$(( $(P) * 10)) )


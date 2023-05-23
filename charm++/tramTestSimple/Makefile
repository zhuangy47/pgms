-include ../../common.mk
#CHARMC=/Users/kale/software/taskqCharm/charm/bin/charmc $(OPTS)
CHARMC=/Users/kale/software/charm/bin/charmc $(OPTS)

OBJS = tramTest1.o

all: tramTest1

tramTest1: $(OBJS)
	$(CHARMC) -language charm++ -tracemode projections -o tramTest1 $(OBJS)

tramTest1.decl.h: tramTest1.ci
	$(CHARMC)  tramTest1.ci

clean:
	rm -f *.decl.h *.def.h conv-host *.o tramTest1 charmrun *.log *.sum *.sts tramTest1.exe tramTest1.pdb tramTest1.ilk

tramTest1.o: tramTest1.C tramTest1.decl.h
	$(CHARMC) -c tramTest1.C

test: all
	$(call run, ./tramTest1 +p4 10 )

testp: all
	$(call run, ./tramTest1 +p$(P) $$(( $(P) * 10)) )

smptest: all
	$(call run, ./tramTest1 10 +p2 ++ppn 2 )
	$(call run, ./tramTest1 10 +p4 ++ppn 2 )

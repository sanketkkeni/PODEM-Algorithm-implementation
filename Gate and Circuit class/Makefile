CFLAGS = -x c++
CFLAGS = -x c++ -std=c++11 -Wno-deprecated-register
OPTLEVEL = -O3
SRCPP = main.cc ClassGate.cc ClassCircuit.cc 
SRCC = lex.yy.c parse_bench.tab.c
EXECNAME = atpg

#FLEXLOC = flex
#BISONLOC = bison
#LIBFLAGS = -ll

FLEXLOC = /home/home4/pmilder/ese549/tools/bin/flex 
BISONLOC = /home/home4/pmilder/ese549/tools/bin/bison 
LIBFLAGS = -L /home/home4/pmilder/ese549/tools/lib -lfl



all: bison flex
	g++ $(CFLAGS) $(SRCC) $(SRCPP) $(LIBFLAGS) -o $(EXECNAME) $(OPTLEVEL)

debug: bison flex
	g++ $(CFLAGS) $(SRCC) $(SRCPP) $(LIBFLAGS) -o $(EXECNAME) -g

bison:
	$(BISONLOC) -d parse_bench.y

flex:
	$(FLEXLOC) parse_bench.l

clean:
	rm -rf parse_bench.tab.c parse_bench.tab.h lex.yy.c $(EXECNAME) *~ atpg.dSYM

doc:
	doxygen doxygen.cfg
	cd docs/latex && make pdf

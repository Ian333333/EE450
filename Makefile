# build an executable named serverM from serverM.cpp
# TARGET = serverA serverB serverC serverM clientA clientB

all: serverA.cpp serverB.cpp serverC.cpp serverM.cpp clientA.cpp clientB.cpp
	g++ -o serverA serverA.cpp
	g++ -o serverB serverB.cpp
	g++ -o serverC serverC.cpp
	g++ -pthread -o serverM serverM.cpp
	g++ -o clientA clientA.cpp
	g++ -o clientB clientB.cpp

serverA: serverA.o
	./serverA

serverB: serverB.o
	./serverB

serverC: serverC.o
	./serverC

serverM: serverM.o
	./serverM

clean: 
	$(RM) serverA
	$(RM) serverB
	$(RM) serverC
	$(RM) serverM
	$(RM) clientA
	$(RM) clientB
	$(RM) serverA.o
	$(RM) serverB.o
	$(RM) serverC.o
	$(RM) serverM.o
	$(RM) clientA.o
	$(RM) clientB.o
	

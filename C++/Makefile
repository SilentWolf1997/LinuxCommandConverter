.PHONY: all clean

AR=ar
ARFLAGS=rv
CXX=g++
CXXFLAGS=-I./include
LDFLAGS=-L./lib -lds

all: app

app: obj lib bin obj/frontend.o obj/backend.o obj/converter.o lib/libds.a
	${CXX} -o bin/frontend obj/frontend.o obj/converter.o ${LDFLAGS}
	${CXX} -o bin/backend obj/backend.o ${LDFLAGS}

obj:
	mkdir obj

lib:
	mkdir lib

bin:
	mkdir bin

obj/frontend.o: src/frontend.cc
	${CXX} -o $@ ${CXXFLAGS} -c $<

obj/backend.o: src/backend.cc
	${CXX} -o $@ ${CXXFLAGS} -c $<

obj/converter.o: src/converter.cc
	${CXX} -o $@ ${CXXFLAGS} -c $<

lib/libds.a: src/message_queue.cc src/named_pipe.cc
	${CXX} -o obj/message_queue.o ${CXXFLAGS} -c src/message_queue.cc
	${CXX} -o obj/named_pipe.o ${CXXFLAGS} -c src/named_pipe.cc
	${AR} ${ARFLAGS} lib/libds.a obj/message_queue.o obj/named_pipe.o

remake: clean all

clean:
	-rm obj/*
	-rm lib/*
	-rm bin/*

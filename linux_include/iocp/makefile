

all : libIOCPEvent.a test.exe

libIOCPEvent.a : libnet.cpp libnet.h
	gcc -fPIC -c libnet.cpp -I/usr/local/include/lsm -g
	ar  -rcs libIOCPEvent.a libnet.o
	rm  -rf *.o

test.exe : test.cpp
	gcc -o test.exe test.cpp -I/usr/local/include/lsm -L. -lIOCPEvent -lstdc++ -lacl_cpp -lacl -pthread -g

clean:
	rm -rf test.exe libIOCPEvent.a



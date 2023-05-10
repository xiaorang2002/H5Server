

all : lib3des8bit.a test.exe

lib3des8bit.a : 3des8bit.cpp
	gcc -fPIC -c 3des8bit.cpp
	ar -rcs lib3des8bit.a 3des8bit.o
	rm -rf *.o

test.exe : test.cpp
	gcc -o test.exe test.cpp -I/usr/local/include/lsm -L. -lpthread -lstdc++ -l3des8bit -g

clean:
	rm -rf test.exe test.txt *.o lib3des8bit.a





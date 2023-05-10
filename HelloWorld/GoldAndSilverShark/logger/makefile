

all : test.exe


test.exe : test.cpp LogAsync.h
	gcc -o test.exe test.cpp -I/usr/local/include/lsm -lpthread -lstdc++ -g

clean:
	rm -rf test.exe test.txt



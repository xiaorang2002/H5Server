
all : test.exe

test.exe : test.cpp
	gcc -o test.exe test.cpp -I/usr/local/include/lsm -L. -lsqlite3 -lstdc++ -pthread -ldl -lm -g

clean:
	rm -rf test.exe



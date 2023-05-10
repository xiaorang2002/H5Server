

all : test.exe


test.exe : test.cpp
	gcc -o test.exe test.cpp -I/usr/local/include/lsm -lVMProtectSDK64 -lpthread -lstdc++ -g

clean:
	rm -rf test.exe test.txt





all : test.exe


test.exe : test.cpp IpAddress.h
	gcc -o test.exe test.cpp -I/usr/local/include/lsm -lpthread -lcrypto -lstdc++ -g

clean:
	rm -rf test.exe test.txt



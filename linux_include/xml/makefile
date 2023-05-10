

all : libtinyxml.a test.exe

libtinyxml.a : tinystr.cpp tinyxml.cpp tinyxmlerror.cpp tinyxmlparser.cpp
	gcc -fPIC -c tinystr.cpp tinyxml.cpp tinyxmlerror.cpp tinyxmlparser.cpp
	ar -rcs libtinyxml.a tinystr.o tinyxml.o tinyxmlerror.o tinyxmlparser.o
	rm -rf *.o

test.exe : test.cpp
	gcc -o test.exe test.cpp -I/usr/local/include/lsm -L. -lpthread -lstdc++ -ltinyxml -g

clean:
	rm -rf test.exe test.txt *.o libtinyxml.a





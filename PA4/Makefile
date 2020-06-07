CFLAGS = -g -O0

all:
	gcc  $(CFLAGS) test.c -pthread -o test
	gcc -shared -fPIC -o  ddmon.so ddmon.c -pthread -ldl 
	gcc -o ddchck ddchck.c -pthread

clean:
	rm test ddmon.so ddchck

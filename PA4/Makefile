CFLAGS = -g -O0

all:
	gcc  $(CFLAGS) abba.c -pthread -o abba
	gcc -shared -fPIC -o  ddmon.so ddmon.c -pthread -ldl 
	gcc -o ddchck ddchck.c -pthread
	gcc $(CFLAGS) diningtest.c -pthread -o dining
clean:
	rm abba ddchck dining
	rm .ddtrace
	rm *.so

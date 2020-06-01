all:
	gcc -o test test.c -pthread
	gcc -shared -fPIC -o ddmon.so ddmon.c -ldl
	gcc -o ddchck ddchck.c

clean:
	rm test ddmon.so ddchck

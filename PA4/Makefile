all:
	gcc -o test test.c -pthread
	gcc -shared -fPIC -o ddmon.so ddmon.c -pthread -ldl 
	gcc -o ddchck ddchck.c -pthread

clean:
	rm test ddmon.so ddchck

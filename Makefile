mythread.a: mythread.o
	ar rcs mythread.a mythread.o 
	
mythread.o: mythread.c mythread.h
	gcc -c mythread.c
	
//Just one semaphore is being used 


#include <sys/time.h>
#include <stdio.h>
#include "mythread.h" 
#include <errno.h>

//pthread_mutex_t produce_mutex = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t consume_mutex = PTHREAD_MUTEX_INITIALIZER;

MySemaphore consume_semaphore, produce_semaphore;

int b[10];  /* buffer size = 1; */
int n; 


void add_buffer(int i){
  b[i] = i;
  printf("\nAdded %d",b[i])	;
}
int get_buffer(int i){
  
	printf("\nConsumed %d",b[i]);	
	return b[i] ;
}
 

void func(void *dummy)
{
	printf ("XXX");
	int i = (int) dummy,v;
	MyThread  t;
	
	 //for (i=0;i<10;i++){
	   MySemaphoreWait(consume_semaphore);
		printf("I'm a consumer in %d\n",i);	
	//printf("\n In %d", i);  	   
	if(i==0)
		t = MyThreadCreate(func,(void *)1);
		
	  MyThreadJoin(t);
	// v = get_buffer(i);
		
	   MySemaphoreSignal(consume_semaphore);
	   printf("Exiting %d  ",i);
	//}
	MyThreadExit();
}

void temp(void *dummy)
{
	printf ("CCC");
	MyThread t_consumer;	
	t_consumer = MyThreadCreate(func,(void *)0);
	printf ("DDD");
//	MyThreadCreate(func,(void *)2);
	MyThreadJoin(t_consumer);
	printf ("EEE");


}

 
main()  
{
	int p,c;	
	n =10;	
	//produce_semaphore = MySemaphoreInit(2); 
	consume_semaphore = MySemaphoreInit(1);
 	printf ("AAA");
	MyThreadInit(temp,(void *)&n);
	printf ("BBB");	
	//if(MySemaphoreDestroy(produce_semaphore) == -1)
	//	printf("\n Error destroying producer semaphore");
	if(MySemaphoreDestroy(consume_semaphore))
		printf("\nError destroying consumer semaphore");
}

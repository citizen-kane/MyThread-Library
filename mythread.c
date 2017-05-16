#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include "mythread.h"

int thread_id = 1;

struct Thread;

typedef struct ListNode {
	struct Thread *thread;
	struct ListNode *next;
} ListNode;

typedef struct Queue {
	struct ListNode *front;
	struct ListNode *rear;
} Queue;

typedef struct Thread {
	ucontext_t context;
	int id;
	struct Thread *parent;
	int terminated;
	int blockedOn;
	struct Queue *childList;
} Thread;

typedef struct Semaphore {
	int val;
	struct Queue *semQ;
} Semaphore;

Queue *CreateQ() {
	Queue *Q;
	Q = malloc(sizeof(struct Queue));

	if (!Q)
		return NULL;

	Q->front = Q->rear = NULL;
	return Q;
}

Thread *runningThread;
Thread *originalThread;
ucontext_t *originalContext;

int isEmptyQueue(Queue *Q) {
	return (Q->front == NULL);
}

void EnQueue(Queue *Q, Thread *thread) {
	ListNode *newNode = (ListNode *)malloc(sizeof(ListNode));

	newNode->thread = thread;
	newNode->next = NULL;

	if (Q->rear != NULL)
		Q->rear->next = newNode;
	
	Q->rear = newNode;

	if (Q->front == NULL) 
		Q->front = Q->rear;

}

Thread *DeQueue(Queue *Q) {

	if (Q->front == NULL)
		return NULL;

	Thread *temp = Q->front->thread;

	if (Q->front == Q->rear) {
		Q->front = Q->rear = NULL;
	} else {
		Q->front = Q->front->next;
	}

	return temp;
}

int existInQueue(Queue *Q, Thread *thread) {
	if (isEmptyQueue(Q)) {
		return 0;
	}
	ListNode *temp = Q->front;
	while (temp){
		if (temp->thread == thread) {
			return 1;
		}
		temp = temp->next;
	}
	return 0;
}

void deleteFromQueue(Queue *Q, Thread *thread) {
	if (isEmptyQueue(Q)) {
		return;
	}
	ListNode *temp = Q->front;
	ListNode *prev;

	while (temp) {
		if (thread == temp->thread) {
			if ((temp == Q->front) && (Q->front == Q->rear)) {
				Q->front = NULL;
				Q->rear = NULL;
			} else if (temp == Q->front) {
				Q->front = Q->front->next;
			} else if (temp == Q->rear) {
				Q->rear = prev;
				Q->rear->next = NULL;
			} else {
				prev->next = temp->next;
			}
			return;
		}
		prev = temp;
		temp = temp->next;
	}
	return;
}

Queue *readyQ;
Queue *blockedQ;

void MyThreadInit(void(*start_funct)(void *), void *args) {
	Thread *thread = (Thread *)malloc(sizeof(Thread));
	thread->id = thread_id;
	thread_id++;

	thread->terminated = 0;
	thread->blockedOn = 0;
	thread->parent = NULL;
	thread->childList = CreateQ();

	readyQ = CreateQ();
	blockedQ = CreateQ();

	getcontext(&thread->context);
	thread->context.uc_stack.ss_sp = malloc(1024*8);
	thread->context.uc_stack.ss_size = 1024*8;
	makecontext(&thread->context, (void (*)(void)) start_funct, 1, args);

	runningThread = thread;
	originalThread = thread;
	
	originalContext = (ucontext_t *)malloc(sizeof(ucontext_t));
	swapcontext(originalContext, &thread->context);

}

void *MyThreadCreate (void (*start_funct)(void *), void *args) {
	Thread *thread = (Thread *)malloc(sizeof(Thread));
	thread->id = thread_id;
	thread_id++;

	thread->terminated = 0;
	thread->blockedOn = 0;
	thread->parent = runningThread;
	thread->childList = CreateQ();

	getcontext(&thread->context);
	thread->context.uc_stack.ss_sp = malloc(1024*8);
	thread->context.uc_stack.ss_size = 1024*8;
	makecontext(&thread->context, (void (*)(void)) start_funct, 1, args);

	EnQueue(readyQ, thread);
	EnQueue(runningThread->childList, thread);

	return (void *)thread;
}

void MyThreadYield(void) {
	Thread *temp2 = runningThread;
	EnQueue(readyQ, runningThread);

	Thread *temp = DeQueue(readyQ);
	if(temp != NULL) {
		runningThread = temp;
		swapcontext(&temp2->context, &temp->context);
	} else {
		setcontext (originalContext);
	}
}

int MyThreadJoin(void *thread) {
	Thread *childThread = (Thread *)thread;

	if (childThread->terminated != 1) {
		if (childThread->parent == runningThread) {
			Thread *temp = runningThread;
			runningThread->blockedOn = childThread->id;
			EnQueue(blockedQ, runningThread);
			Thread *temp2 = DeQueue(readyQ);
			if (temp2 != NULL) {

				runningThread = temp2;
				swapcontext(&temp->context, &temp2->context);
			} else {

				setcontext(originalContext);
			}
			return 0;
		} else {
			return -1;
		}
	}

	return 0;
}

void MyThreadJoinAll(void) {
	ListNode *temp = runningThread->childList->front;

	int flag = 0;
	while (temp != NULL) {
		if (existInQueue(readyQ, temp->thread)) {

			flag = 1;
			break;
		}

		temp = temp->next;
	}
	if (flag ==1){

		Thread *temp2 = runningThread;
		EnQueue(blockedQ, runningThread);
		Thread *temp3 = DeQueue(readyQ);
		if (temp3 != NULL) {

			runningThread = temp3;
			swapcontext(&temp2->context, &temp3->context);
		} else {

			setcontext(originalContext);
		}
	}

}

void MyThreadExit(void) {
	
	int flag = 0;

	Thread *tempThread = runningThread->parent;

	if (tempThread != NULL) {
		ListNode *temp2 = tempThread->childList->front;
		while (temp2 != NULL) {
			if (temp2->thread == runningThread) {
				deleteFromQueue(runningThread->parent->childList, runningThread);
				break;
			}
			temp2 = temp2->next;
		}
	}
	
	ListNode *temp = blockedQ->front;
	if (existInQueue(blockedQ, runningThread->parent)) {
		while (temp != NULL) {
			if (temp->thread == runningThread->parent) {
				flag = 1;
				break;
			}
			temp = temp->next;
		}
		if (flag == 1) {
			if (temp->thread->blockedOn == runningThread->id) {
				temp->thread->blockedOn = 0;
				deleteFromQueue(blockedQ, temp->thread);
				EnQueue(readyQ, temp->thread);
			} else if (temp->thread->childList->front == NULL) {
				deleteFromQueue(blockedQ, temp->thread);
				EnQueue(readyQ, temp->thread);
				
			}
		}
	}

	ListNode *temp3 = runningThread->childList->front;
	while (temp3 != NULL) {
		if (runningThread==originalThread) {
			temp3->thread->parent = NULL;
		} else {
			temp3->thread->parent = originalThread;
			EnQueue(originalThread->childList, temp3->thread);
		}
		temp3 = temp3->next;
	}
	runningThread->terminated = 1;
	Thread *temporary1 = runningThread;

	Thread *temporary2 = DeQueue(readyQ);
	if (temporary2 != NULL) {
		runningThread = temporary2;
		swapcontext(&temporary1->context, &temporary2->context);
	} else {
		swapcontext(&temporary1->context, originalContext);
	}


}

MySemaphore MySemaphoreInit(int initialValue) {
	Semaphore *s = (Semaphore *)malloc(sizeof(Semaphore));
	s->val = initialValue;
	s->semQ = CreateQ();

	return (MySemaphore) s;
}

void MySemaphoreSignal(MySemaphore sem){
	Semaphore *s = sem;
	s->val = s->val + 1;

	if (s->val<=0) {
		Thread *thread = DeQueue(s->semQ);
		if (thread != NULL){
			EnQueue(readyQ, thread);
		}
	}
}

void MySemaphoreWait(MySemaphore sem) {
	Semaphore *s = sem;
	s->val = s->val - 1;

	if(s->val<0) {
		EnQueue(s->semQ, runningThread);
		Thread *temp = runningThread;

		Thread *temp2 = DeQueue(readyQ);
		if (temp2 != NULL) {
			runningThread = temp2;
			swapcontext(&temp->context, &temp2->context);
		} else {
			swapcontext(&temp->context, originalContext);
		}
	}

}

int MySemaphoreDestroy(MySemaphore sem) {
	Semaphore *s  = sem;

	if (isEmptyQueue(s->semQ)) {
		free(s);
		return 0;
	}
	return -1;
}

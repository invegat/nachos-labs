// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustrate the inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.


//#ifdef HW1_SEMAPHORES
//#include <time.h>
//#include <ctime>
//#include <atomic>
//#endif

class Thread;

#include "copyright.h"
#include "system.h"
#if defined(HW1_SEMAPHORES) || defined(HW1_LOCKS)
#include "synch.h"
#ifdef HW1_SEMAPHORES
Semaphore * semaphore = new Semaphore("semaphoreTest", 1);
#endif
#ifdef HW1_LOCKS
Lock * lock = new Lock("lockTest");
#endif
#endif


// testnum is set in main.cc
//extern int testnum;
//extern int n;
//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------
#ifdef CHANGED
int SharedVariable;
int numThreadsActive; // used to implement barrier upon completion

void SimpleThread(int which) {
    int num, val;
    // printf("forked thread %d\n", which);
//    DEBUG('w', buf);
    for(num = 0; num < 5; num++) {
#ifdef HW1_SEMAPHORES
        semaphore->P();
#endif
#ifdef HW1_LOCKS
        lock->Acquire();
#endif
        val = SharedVariable;
        printf("*** thread %d sees value %d\n", which, val);
        currentThread->Yield();
        SharedVariable = val+1;
#ifdef HW1_SEMAPHORES
        semaphore->V();
#endif
#ifdef HW1_LOCKS
        lock->Release();
#endif
        currentThread->Yield();
    }
    numThreadsActive--;
    while (numThreadsActive > 0) {
        currentThread->Yield();
        numThreadsActive--;
    }
    val = SharedVariable;
    printf("Thread %d sees final value %d\n", which, val);
}
//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------
void
ThreadTest(int n) {
    DEBUG('t', "Entering SimpleTest");
    Thread *t;
    numThreadsActive = n;
    printf("numThreadsActive =%d\n", numThreadsActive);
    for (int i = 1;i<n;i++)
    {
        t = new Thread("forked thread");
        t->Fork(SimpleThread, i);
        DEBUG('i', "forked thread");
    }
    SimpleThread(0);

}
#else // CHANGED not defined
void
SimpleThread(int which)
{
    int num;

    for (num = 0; num < 5; num++) {
        printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
}
//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");
    Thread *t = new Thread("forked thread");
    t->Fork("SimpleThread", 1);
    SimpleThread(0);
}
//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest() {
    DEBUG('t', "Entering SimpleTest");
    switch (testnum) {
        case 1:
            ThreadTest1();
	        break;
        default:
	        printf("No test specified.\n");
	        break;
    }
}
#endif // CHANGED



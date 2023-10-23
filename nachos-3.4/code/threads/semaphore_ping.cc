// Alternate two threads, one that prints "ping" and another that
// prints "pong".

class Thread;

#include "copyright.h"
#include "system.h"
#include "synch.h"

Semaphore * pingSem = new Semaphore("ping", 1);

void SPingPong(int which)
{
    char *msg = (char *) which;
    int i;

    for (i = 0; i < 5; i++) {
		
	    pingSem->P();
	    printf("%s\n", msg);
	    currentThread->Yield();
	    pingSem->V();
     	currentThread->Yield();	
    }
     
}

void SemaphorePing()
{
    const char *ping = "ping";
    const char *pong = "pong";

    Thread *t = new Thread(ping);
    t->Fork(SPingPong, (int) ping);

    SPingPong((int) pong);

}
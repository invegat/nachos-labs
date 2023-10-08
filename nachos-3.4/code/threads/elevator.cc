#ifdef __JETBRAINS_IDE__
#define EXTERN_LOCKS
#define EXTERN_SEMAPHORES
#include "../machine/interrupt.h"
#define LOG
#define DEVELOPMENT
#define ELEVATOR_DELAY
#endif
#include "copyright.h"
#include "system.h"
#include "synch.h"
#include "elevator.h"
#ifdef LOG
#include <stdio.h>
extern FILE *log_file;
#endif
#include "elevatorDelay.h"

//static void freeElevatorLock(int pE) {
//    ELEVATOR * e = *(ELEVATOR **)pE;
//    e->elevatorLock->Release();
//}
bool ELEVATOR::removeFromPPFirst(Person * p[]) {
    int i = 0;
    while (p[i] != NULL) {
        if (p[i] != (Person *)-1) {
            removeFromPP(p, p[i]);
            return true;
        }
        i++;
    }
    return false;
}
static int personCount(Person *p[]) {
		int i = 0;
		int totalOn = 0;
		while (p[i] != NULL) {
			if (p[i] != (Person *)-1)
				totalOn++;
			i++;
		}		
	return totalOn;
}
static int active(int pE) {
    ELEVATOR * e = *(ELEVATOR **)pE;
    int totalWaiting = 0;
    for (int floor = 1;floor <= e->numFloors;floor++) {
        totalWaiting += e->personsWaiting[floor];
        totalWaiting += personCount(e->listLeaving[floor]);
        totalWaiting += personCount(e->listEntering[floor]);
    }

    totalWaiting += personCount(e->personsOn);
    return totalWaiting;

}
void ELEVATOR::addToPP(Person *pp[], Person * p) {
    personLock->Acquire();
    for (int i = 0;i<tp;i++) {
        if (pp[i] == (Person *)-1) {
            pp[i] = p;
//            printf("pp[i] == -1 personCount is %d\n", personCount(pp));
            break;
        }
        if (pp[i] == NULL) {
            pp[i]=p;
            pp[i+1] = (Person *)NULL;
            // printf("*pp == NULL personCount is %d\n", personCount(pp));
            break;
        }
    }
    personLock->Release();
}
void ELEVATOR::removeFromPP(Person * pp[], Person * p) {
    personLock->Acquire();
    for(int i=0;i<tp;i++) {
        if (pp[i] == p) {
            if (pp[i+1] == NULL)
                pp[i] = NULL;
            else
                pp[i] = (Person *)-1;
            break;
        }
    }
//    printf("removeFromPP personCount is %d\n", personCount(pp));
    personLock->Release();

}
bool ELEVATOR::onElevator(Person * target) {
    for (int i = 0;i<tp;i++) {
        if (target == personsOn[i])
            return true;
        if (personsOn[i] == NULL)
            return false;
    }
    return false;
}

int nextPersonID = 1;
Lock *personIDLock = new Lock("PersonIDLock");


int ELEVATOR::getFloor(Person **pp[], bool at) {
    for (int delta = 0;delta < numFloors;delta++) {
        if (currentFloor + delta <= numFloors) {
            Person **p = pp[currentFloor + delta];
            int i = 0;
            while (p[i] != NULL) {
                if (p[i] != (Person *) -1) {
                    if (at || onElevator(p[i]))
                        return (at ? p[i]->atFloor : p[i]->toFloor);
                }
                i++;
            }
        }
        if (delta && (currentFloor - delta >=1)) {
            Person **p = pp[currentFloor - delta];
            int i = 0;
            while (p[i] != NULL) {
                if (p[i] != (Person *) -1) {
                    if (at || onElevator(p[i]))
                        return (at ? p[i]->atFloor : p[i]->toFloor);
                }
                i++;
            }
        }
    }
    return -1;
}



int pf = -1;
static int totalOut = 0;
static int totalIn = 0;

#ifndef DISABLE_HALT
Semaphore * haltSem = new Semaphore("halt", 1);
static void haltTest(int pE) {
    ELEVATOR * e = *(ELEVATOR **)pE;
    while(1) {
        IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts

        if ((totalIn == totalOut) && (active(pE) == 0)) {
            bool ccf = true; // consistent currentFloor == targetFloor
            int targetFloor = (e->numFloors + 1) / 2;
            if (targetFloor == e->currentFloor) {
                for (int j = 0; j < 10000000; j++) {
                    currentThread->Yield();
                    if ((e->currentFloor != targetFloor) || (active(pE) > 0)  || (totalIn != totalOut)) {
                        ccf = false;
                        break;
                    }
                }

                if (ccf) {
#ifdef LOG
#ifdef DEVELOPMENT
                    printf("haltTest Halt\n");
#endif
                    fclose(log_file);
#endif
                    interrupt->Halt();
                }
            }
        }
//        for(int j =0 ; j<1000; j++) {
//            currentThread->Yield();
//        }
        haltSem->P();
        (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts

    }

}
#endif
ELEVATOR *e;

void ELEVATOR::start() {

//    printf("elevatorDelay: %d\n", elevatorDelay);
#if defined(LOG) && defined(DEVELOPMENT)
    fprintf(log_file, "elevatorDelay: %d\n", elevatorDelay);
#endif
    int from = -1;
    int to = -1;
    int lastActive = -1;
#ifndef DISABLE_HALT
    Thread* t = new Thread("Halt Thread");
    t->Fork(haltTest, (int)&e);
#endif
	
    while(1) {

        // A. Wait until hailed
		
        //      0. Acquire elevatorLock
//        bool elevatorCaptured = false;
        elevatorLock->Acquire();
//        elevatorCaptured = true;

#ifndef DISABLE_HALT
        haltSem->V();
#endif



        if (from < 1)
            from = getFloor(listEntering, true);
        if (to < 1)
            to = getFloor(listLeaving, false);

        int totalActive = active((int)&e);

        if (totalActive != lastActive) {
//            printf("totalActive %d\n", totalActive);
            lastActive = totalActive;
        }

        // B. While there are active persons, loop doing the following
        //      3. Spin for some time
//		if (occupancy > 0 || from > 0 || to > 0 || totalWaiting == 0 || totalWaiting > 0)
//            sleep(1);
        if (totalActive > 0)
            for(int j =0 ; j< elevatorDelay; j++) {
                currentThread->Yield();
            }
        //      4. Go to next 
		bool suppressPrint = false;
//        bool floorCaptured = false;

        if (from == -1 && to == -1) {
            if (currentFloor < (numFloors + 1) /2)
                currentFloor++;
            else if (currentFloor > (numFloors + 1) / 2)
                currentFloor--;
        }

		if ((from > 0) && (to == -1 || (abs(to - currentFloor) > abs(from - currentFloor))) && (occupancy < maxOccupancy)   ) {
            /*
			if (currentFloor > from)
				currentFloor--;
			else if (currentFloor < from)
				currentFloor++;
			else {
				suppressPrint = true;
//				from = 0;
                for(int j =0 ; j< 1000000; j++) {
                    currentThread->Yield();
                }				
			}
             */
//            while (currentFloor != from){
            if (from < currentFloor) currentFloor--;
            if (from > currentFloor) currentFloor++;

//             }
            //      2. Signal persons atFloor to get in, one at a time, checking occupancyLimit each time
            if (from == currentFloor) {
                Condition *condition = (Condition *) entering[currentFloor];

                int o = occupancy;
                while (o < maxOccupancy) {
                    Thread *thread = condition->Signal(elevatorLock);
                    if (thread == NULL) break;
                    bool found = removeFromPPFirst(listEntering[currentFloor]);
                    ASSERT(found);
                    o++;
                }
                from = -1;
            }
        }
		else if (to > 0) {
            /*
			if (currentFloor > to)
				currentFloor--;
			else if (currentFloor < to)
				currentFloor++;
			else {
				suppressPrint = true;
//				to = 0;
                for(int j =0 ; j< 1000000; j++) {
                    currentThread->Yield();
                }
			}
             */
//            while (currentFloor != to){
            if (to < currentFloor) currentFloor--;
            if (to > currentFloor) currentFloor++;
//            }
            if (to == currentFloor) {
//                printf("Broadcasting\n");
                //      1. Signal persons inside elevator to get off (leaving->broadcast(elevatorLock))
                int i = leaving[currentFloor]->Broadcast(elevatorLock);
                for (int j=0;j<i;j++) {
                    bool found = removeFromPPFirst(listLeaving[currentFloor]);
                    ASSERT(found);
                }
//                printf("%d dequed by Broadcast\n", i);
                to = -1;
            }
		}
//        if (floorCaptured)
//        floorLock->Release();
		if (!suppressPrint) {
			if (currentFloor != pf) {
	        	printf("Elevator arrives on floor %d\n", currentFloor );
#ifdef LOG
                fprintf(log_file, "Elevator arrives on floor %d\n", currentFloor );
#endif
				pf = currentFloor;
			}
		}
//        if (elevatorCaptured)
//            printf("Elevator Captured\n");
        //      2.5 Release elevatorLock
        elevatorLock->Release();
        for(int j =0 ; j< 50; j++);
        for(int j =0 ; j< elevatorDelay; j++) {
            currentThread->Yield();
        }
    }
}

void ElevatorThread(int numFloors) {

    printf("Elevator with %d floors was created!\n", numFloors);
#ifdef LOG
    fprintf(log_file, "Elevator with %d floors was created!\n", numFloors);
#endif

    e = new ELEVATOR(numFloors);

    e->start();


}

ELEVATOR::ELEVATOR(int lNumFloors) {
	numFloors = lNumFloors;
    currentFloor = 1;
    entering = new Condition*[numFloors + 1];
    leaving = new Condition*[numFloors + 1];
    listEntering = (Person ***)(new Person *[numFloors + 1][tp]);
    listLeaving = (Person ***)(new Person *[numFloors + 1][tp]);
    // Initialize entering
    for (int i = 1; i < numFloors + 1; i++) {
        entering[i] = new Condition("Entering " + i);
        listEntering[i] = new Person *[tp] ;
        listLeaving[i] = new Person *[tp];
        listEntering[i][0] = (Person *)NULL;
        listLeaving[i][0] = (Person *)NULL;
    }
    personsWaiting = new int[numFloors + 1];

    elevatorLock = new Lock("ElevatorLock");

    // Initialize 
    for (int j = 1; j < numFloors + 1; j++) {
        leaving[j] = new Condition("Leaving " + j);
    }
	//personsOn = new Person*[tp]; // limited to 100 student/teachers
	personsOn[0] = (Person *)NULL;
	up = true;
	maxOccupancy = 5;
	// eTimer = new Timer(freeElevatorLock, (int)(&e), false);
    personLock = new Lock("Person Lock");
    conditionLock = new Lock("ConditionLock");
//    floorLock = new Lock("Floor Lock");

}


void Elevator(int lNumFloors) {
    // Create Elevator Thread
    Thread *t = new Thread("Elevator");
    t->Fork(ElevatorThread, lNumFloors);
}

int getPersonID() {
	int personID = nextPersonID;
	personIDLock->Acquire();
	nextPersonID++;
	personIDLock->Release();
	return personID;
}
void ELEVATOR::hailElevator(Person *p) {
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    // 1. Increment waiting persons atFloor
    personsWaiting[p->atFloor]++;
//    printf("personsWaiting atFloor = %d\n", p->atFloor);
    addToPP(listEntering[p->atFloor],p);

//    for(int j =0 ; j< 10; j++) {
//        currentThread->Yield();
//    }
//    for(int j =0 ; j< 1000000; j++) {
//        currentThread->Yield();
//    }
    // 2. Hail Elevator
	// elevatorLock->Release();
    // 2.5 Acquire elevatorLock;
	elevatorLock->Acquire();
//    floorLock->Acquire();
    // 3. Wait for elevator to arrive atFloor [entering[p->atFloor]->wait(elevatorLock)]
	entering[p->atFloor]->Wait(elevatorLock);
//    printf("p->atFloor = %d\n", p->atFloor);
    // 5. Get into elevator
	addToPP(personsOn,p);
    ASSERT(p->atFloor == currentFloor);
//    printf("Person %d got into the elevator at floor %d.\n", p->id, currentFloor);
    printf("Person %d got into the elevator.\n", p->id);
#ifdef LOG
    fprintf(log_file, "Person %d got into the elevator.\n", p->id);
#endif
    // 6. Decrement persons waiting atFloor [personsWaiting[atFloor]++]
	personsWaiting[p->atFloor]--;
    addToPP(listLeaving[p->toFloor],p);
//    floorLock->Release();
//    sleep(1);
//    floorLock->Acquire();
    // 7. Increment persons inside elevator [occupancy++]
	occupancy++;
    // 8. Wait for elevator to reach toFloor [leaving[p->toFloor]->wait(elevatorLock)]
	leaving[p->toFloor]->Wait(elevatorLock);
    // 9. Get out of the elevator
	removeFromPP(personsOn,p);

    ASSERT(currentFloor == p->toFloor);
    printf("Person %d got out of the elevator.\n", p->id);
#ifdef LOG
    fprintf(log_file, "Person %d got out of the elevator.\n", p->id);
#endif
    totalOut++;

    // 10. Decrement persons inside elevator
	occupancy--;
    // 11. Release elevatorLock;
//    floorLock->Release();
	elevatorLock->Release();

//    testHalt((int)&e);
//    printf("total of %d out %d\n",totalIn, totalIn - totalOut);
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
//    for(int j =0 ; j< 10; j++) {
//        currentThread->Yield();
//    }
}

void PersonThread(int person) {

    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts

    Person *p = (Person *)person;

    printf("Person %d wants to go from floor %d to %d\n", p->id, p->atFloor, p->toFloor);
#ifdef LOG
    fprintf(log_file, "Person %d wants to go from floor %d to %d\n", p->id, p->atFloor, p->toFloor);
#endif
    e->hailElevator(p);
   (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

int getNextPersonID() {
    int personID = nextPersonID;
    personIDLock->Acquire();
    nextPersonID = nextPersonID + 1;
    personIDLock->Release();
    return personID;
}


void ArrivingGoingFromTo(int atFloor, int toFloor) {
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupt
    // Create Person struct
    Person *p = new Person;
    p->id = getNextPersonID();
    p->atFloor = atFloor;
    p->toFloor = toFloor;
    totalIn++;
    // Creates Person Thread
    Thread *t = new Thread("Person " + p->id);
    t->Fork(PersonThread, (int)p);
   (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts

}

#include "copyright.h"
#include "system.h"
#include "synch.h"
#include "elevator.h"
static bool inAction = true, haltEnabled = false;

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
static int lTotalWaiting = -1, lTotalOn = -1;
static bool firstTestHalt = true;
static void testHalt(int pE) {
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
	if (inAction) {
		haltEnabled = true;
    }
	if (haltEnabled && (!firstTestHalt)) {
		ELEVATOR * e = *(ELEVATOR **)pE;
		int totalWaiting = 0;
		for (int floor = 1;floor < e->numFloors;floor++) 
			totalWaiting += e->personsWaiting[floor];
		int totalOn = personCount(e->personsOn);
		if (totalOn != lTotalOn || lTotalWaiting != totalWaiting) {
//			printf("total On %d   total Waiting %d\n", totalOn, totalWaiting);
			lTotalOn = totalOn;
			lTotalWaiting = totalWaiting;
		}	
		// interrupt->Halt();		
//		if (e->occupancy == 0 && totalWaiting == 0) interrupt->Halt();
		if (totalOn == 0 && totalWaiting == 0) {
//            printf("testHalt Halt\n");
            interrupt->Halt();
        }
	}
   (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}
void ELEVATOR::addToPP(Person *pp[], Person * p) {
    personLock->Acquire();
    for (int i = 0;i<tp;i++) {
        if (pp[i] == (Person *)-1) {
            pp[1] = p;
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



ELEVATOR *e;
int pf = -1;

void ELEVATOR::start() {
    int from = -1;
    int to = -1;
	
    while(1) {

        // A. Wait until hailed
		
        //      0. Acquire elevatorLock
//        bool elevatorCaptured = false;
        elevatorLock->Acquire();
//        elevatorCaptured = true;



        int totalActive= 0;
        for (int floor = 1;floor < numFloors;floor++)
            totalActive += personsWaiting[floor];

        if (from < 1)
            from = getFloor(listEntering, true);
        if (to < 1)
            to = getFloor(listLeaving, false);
        if (from > 0 || to > 0)
            totalActive++;



        // B. While there are active persons, loop doing the following
        //      3. Spin for some time
//		if (occupancy > 0 || from > 0 || to > 0 || totalWaiting == 0 || totalWaiting > 0)
//            sleep(1);
//        if (totalActive > 0)
            for(int j =0 ; j< 1000000; j++) {
                currentThread->Yield();
            }
        //      4. Go to next 
		bool suppressPrint = false;
//        bool floorCaptured = false;

        if (from == -1 && to == -1) {
//            if (!firstTestHalt) interrupt->Halt();
            if (currentFloor < numFloors / 2)
                currentFloor++;
            if (currentFloor >= (numFloors + 1) / 2)
                currentFloor--;
//            sleep(1);
        }
        if (from > 0 || to > 0)
            firstTestHalt = false;

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
//	        	printf("Elevator arrives on floor %d\n", currentFloor );
				pf = currentFloor;
			}
		}
//        if (elevatorCaptured)
//            printf("Elevator Captured\n");
        //      2.5 Release elevatorLock
        elevatorLock->Release();
        for(int j =0 ; j< 1000000; j++) {
            currentThread->Yield();
        }

    }
}

void ElevatorThread(int numFloors) {

    printf("Elevator with %d floors was created!\n", numFloors);

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
static int totalOut = 0;
static int totalIn = 0;
void ELEVATOR::hailElevator(Person *p) {
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    for(int j =0 ; j< 1000000; j++) {
        currentThread->Yield();
    }
    // 1. Increment waiting persons atFloor
    personsWaiting[p->atFloor]++;
//    for(int j =0 ; j< 1000000; j++) {
//        currentThread->Yield();
//    }
    // 2. Hail Elevator
	// elevatorLock->Release();
    // 2.5 Acquire elevatorLock;
	elevatorLock->Acquire();
//    floorLock->Acquire();
    addToPP(listEntering[p->atFloor],p);
    // 3. Wait for elevator to arrive atFloor [entering[p->atFloor]->wait(elevatorLock)]
	entering[p->atFloor]->Wait(elevatorLock);
    // 5. Get into elevator
	addToPP(personsOn,p);
    ASSERT(p->atFloor == currentFloor);
//    printf("Person %d got into the elevator at floor %d.\n", p->id, currentFloor);
    printf("Person %d got into the elevator.\n", p->id);
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
    totalOut++;

    // 10. Decrement persons inside elevator
	occupancy--;
    // 11. Release elevatorLock;
//    floorLock->Release();
	elevatorLock->Release();
    for(int j =0 ; j< 1000000; j++) {
        currentThread->Yield();
    }
    testHalt((int)&e);
//    printf("total of %d out %d\n",totalIn, totalIn - totalOut);
//    if (totalIn == totalOut) interrupt->Halt();
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts

}

void PersonThread(int person) {

    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts

    Person *p = (Person *)person;

    printf("Person %d wants to go from floor %d to %d\n", p->id, p->atFloor, p->toFloor);

    e->hailElevator(p);
	inAction = false;
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
	inAction = true;	
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
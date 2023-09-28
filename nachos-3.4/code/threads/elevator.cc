#include "copyright.h"
#include "system.h"
#include "synch.h"
#include "elevator.h"
static bool inAction = true, haltEnabled = false;

static int from = 0, to = 0;
static const int tp = 100;
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
static void testHalt(int pE) {
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
	if (inAction) {
		haltEnabled = true;
		return;
	}
	if (haltEnabled) {
		ELEVATOR * e = *(ELEVATOR **)pE;
		int totalWaiting = 0;
		for (int floor = 1;floor < e->numFloors;floor++) 
			totalWaiting += e->personsWaiting[floor];
		int totalOn = personCount(e->personsOn);
		if (totalOn != lTotalOn || lTotalWaiting != totalWaiting) {
			printf("total On %d   total Waiting %d\n", totalOn, totalWaiting);
			lTotalOn = totalOn;
			lTotalWaiting = totalWaiting;
		}	
		// interrupt->Halt();		
//		if (e->occupancy == 0 && totalWaiting == 0) interrupt->Halt();
		if (totalOn == 0 && totalWaiting == 0) interrupt->Halt();
	}
   (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}
void addToPP(Person *pp[], Person * p) {
	for (int i = 0;i<tp;i++) {
		if (pp[i] == (Person *)-1) {
			pp[1] = p;
			printf("pp[i] == -1 personCount is %d\n", personCount(pp));
			break;
		}
		if (pp[i] == NULL) {
			pp[i]=p;
			pp[i+1] = (Person *)NULL;
			// printf("*pp == NULL personCount is %d\n", personCount(pp));
			break;
		}
	}
}
void removeFromPP(Person * pp[], Person * p) {
	for(int i;i<tp;i++) {
		if (pp[i] == p) {
			if (pp[i+1] == NULL) 
				pp[i] = NULL;
			else
				pp[i] = (Person *)-1;
			break;
		}
	}
	printf("removeFromPP personCount is %d\n", personCount(pp));

}

int nextPersonID = 1;
Lock *personIDLock = new Lock("PersonIDLock");


ELEVATOR *e;
int pf = -1;

void ELEVATOR::start() {
	
    while(1) {

        // A. Wait until hailed
		
        //      0. Acquire elevatorLock
		elevatorLock->Acquire();

        //      1. Signal persons inside elevator to get off (leaving->broadcast(elevatorLock))
		leaving[currentFloor]->Broadcast(elevatorLock);

		Condition * condition = (Condition *)entering[currentFloor];
        //      2. Signal persons atFloor to get in, one at a time, checking occupancyLimit each time
		while(occupancy < maxOccupancy) {
			Thread  * thread = condition->Signal(elevatorLock);
			if (thread == NULL) break;
			
			occupancy++;
		}

        // B. While there are active persons, loop doing the following
        //      3. Spin for some time
		if (occupancy > 0 || from > 0 || to > 0)
                for(int j =0 ; j< 1000000; j++) {
                    currentThread->Yield();
                }
        //      4. Go to next 
		bool suppressPrint = false;
		if (from > 0) {
			if (currentFloor > from)
				currentFloor--;
			else if (currentFloor < from)
				currentFloor++;
			else {
				suppressPrint = true;
				from = 0;
                for(int j =0 ; j< 1000000; j++) {
                    currentThread->Yield();
                }				
			}
		}
		else if (to > 0) {
			if (currentFloor > to)
				currentFloor--;
			else if (currentFloor < to)
				currentFloor++;
			else {
				suppressPrint = true;
				to = 0;
                for(int j =0 ; j< 1000000; j++) {
                    currentThread->Yield();
                }
			}
		}
        //      2.5 Release elevatorLock
		if (!suppressPrint) {
			if (currentFloor != pf) {
	        	printf("Elevator arrives on floor %d\n", currentFloor );
				pf = currentFloor;
			}
		}	
		elevatorLock->Release();			
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
    // Initialize entering
    for (int i = 1; i < numFloors + 1; i++) {
        entering[i] = new Condition("Entering " + i);
    }
    personsWaiting = new int[numFloors + 1];

    elevatorLock = new Lock("ElevatorLock");

    // Initialize 
    for (int j = 1; j < numFloors + 1; j++) {
        leaving[j] = new Condition("Leaving " + j);
    }
	personsOn = new Person*[tp]; // limited to 100 student/teachers
	personsOn[0] = (Person *)NULL;
	up = true;
	maxOccupancy = 5;
	eTimer = new Timer(testHalt, (int)(&e), false);
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
    // 1. Increment waiting persons atFloor
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts

	personsWaiting[currentFloor]++;
    // 2. Hail Elevator
	// elevatorLock->Release();
    // 2.5 Acquire elevatorLock;
	elevatorLock->Acquire();
    // 3. Wait for elevator to arrive atFloor [entering[p->atFloor]->wait(elevatorLock)]
	from = p->atFloor;
	to = p->toFloor;
	entering[p->atFloor]->Wait(elevatorLock);
    // 5. Get into elevator
	addToPP(personsOn,p);

    printf("Person %d got into the elevator.\n", p->id);
    // 6. Decrement persons waiting atFloor [personsWaiting[atFloor]++]
	personsWaiting[p->atFloor]--;
    // 7. Increment persons inside elevator [occupancy++]
	occupancy++;
    // 8. Wait for elevator to reach toFloor [leaving[p->toFloor]->wait(elevatorLock)]
	leaving[p->toFloor]->Wait(elevatorLock);
    // 9. Get out of the elevator
	removeFromPP(personsOn, p);
    printf("Person %d got out of the elevator.\n", p->id);
    // 10. Decrement persons inside elevator
	occupancy--;
    // 11. Release elevatorLock;
	elevatorLock->Release();
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

    // Creates Person Thread
    Thread *t = new Thread("Person " + p->id);
    t->Fork(PersonThread, (int)p);
   (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts

}
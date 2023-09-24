#include "copyright.h"
#include "system.h"
#include "synch.h"
#include "elevator.h"
/*
1. does the elevator go from floor 1 to the top unconditionally then back to floor 1 unconditionally 

*/

int nextPersonID = 1;
Lock *personIDLock = new Lock("PersonIDLock");


ELEVATOR *e;

Condition hailCondition = new Condition("hail");
Lock hailLock = new Lock("hailLock", false);
void ELEVATOR::start() {
	
    while(1) {

        // A. Wait until hailed
		hailCondition->wait(hailLock);
		
		elevatorLock->Acquire();
		leaving[currentFloor]->broadcast(elevatorLock
		while(occupancy <= maxOccupancy) {
			
		}


        // B. While there are active persons, loop doing the following
        //      0. Acquire elevatorLock
        //      1. Signal persons inside elevator to get off (leaving->broadcast(elevatorLock))
        //      2. Signal persons atFloor to get in, one at a time, checking occupancyLimit each time
        //      2.5 Release elevatorLock
        //      3. Spin for some time
                for(int j =0 ; j< 1000000; j++) {
                    currentThread->Yield();
                }
        //      4. Go to next floor
        //  printf("Elevator arrives on floor %d", )
    }
}

void ElevatorThread(int numFloors) {

    printf("Elevator with %d floors was created!\n", numFloors);

    e = new ELEVATOR(numFloors);

    e->start();


}

ELEVATOR::ELEVATOR(int numFloors) {
    currentFloor = 1;
    entering = new Condition*[numFloors];
    // Initialize entering
    for (int i = 0; i < numFloors; i++) {
        entering[i] = new Condition("Entering " + i);
    }
    personsWaiting = new int[numFloors];
    elevatorLock = new Lock("ElevatorLock");

    // Initialize 
    for (int j = 0; j < numFloors; j++) {
        leaving[j] = new Condition("Leaving " + j);
    }
}


void Elevator(int numFloors) {
    // Create Elevator Thread
    Thread *t = new Thread("Elevator");
    t->Fork(ElevatorThread, numFloors);
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
    // 2. Hail Elevator
    // 2.5 Acquire elevatorLock;
    // 3. Wait for elevator to arrive atFloor [entering[p->atFloor]->wait(elevatorLock)]
    // 5. Get into elevator
    printf("Person %d got into the elevator.\n", p->id);
    // 6. Decrement persons waiting atFloor [personsWaiting[atFloor]++]
    // 7. Increment persons inside elevator [occupancy++]
    // 8. Wait for elevator to reach toFloor [leaving[p->toFloor]->wait(elevatorLock)]
    // 9. Get out of the elevator
    printf("Person %d got out of the elevator.\n", p->id);
    // 10. Decrement persons inside elevator
    // 11. Release elevatorLock;
}

void PersonThread(int person) {

    Person *p = (Person *)person;

    printf("Person %d wants to go from floor %d to %d\n", p->id, p->atFloor, p->toFloor);

    e->hailElevator(p);

}

int getNextPersonID() {
    int personID = nextPersonID;
    personIDLock->Acquire();
    nextPersonID = nextPersonID + 1;
    personIDLock->Release();
    return personID;
}


void ArrivingGoingFromTo(int atFloor, int toFloor) {


    // Create Person struct
    Person *p = new Person;
    p->id = getNextPersonID();
    p->atFloor = atFloor;
    p->toFloor = toFloor;

    // Creates Person Thread
    Thread *t = new Thread("Person " + p->id);
    t->Fork(PersonThread, (int)p);

}
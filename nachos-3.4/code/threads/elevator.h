
#ifndef ELEVATOR_H
#define ELEVATOR_H

#include "copyright.h"

void Elevator(int numFloors);
void ArrivingGoingFromTo(int atFloor, int toFloor);

typedef struct Person {
    int id;
    int atFloor;
    int toFloor;
} Person;


class ELEVATOR {

public:
    ELEVATOR(int numFloors);
    ~ELEVATOR();
    void hailElevator(Person *p);
    void start();

private:
    int currentFloor;
    Condition **entering;
    Condition **leaving;
    int *personsWaiting;
	Person **personsOn;
    int occupancy;
    int maxOccupancy;
    Lock *elevatorLock;
	bool up;
	int numFloors; 
};

#endif
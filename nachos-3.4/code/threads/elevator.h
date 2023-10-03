
#ifndef ELEVATOR_H
#define ELEVATOR_H
#include<unistd.h>
#include "copyright.h"

const int tp = 100;

void Elevator(int numFloors);
void ArrivingGoingFromTo(int atFloor, int toFloor);

typedef struct Person {
    int id;
    int atFloor;
    int toFloor;
} Person;

const int maxOccupancy = 5;

class ELEVATOR {

public:
    ELEVATOR(int numFloors);
    ~ELEVATOR();
    void hailElevator(Person *p);
    void start();

    int *personsWaiting;
    int occupancy;
	int numFloors;
    Person *personsOn[tp];
    Lock *elevatorLock;

private:
    int currentFloor;
    Condition **entering;
    Condition **leaving;
    Person ***listEntering;
    Person ***listLeaving;
    int maxOccupancy;
	bool up;
	Timer * eTimer;
    void addToPP(Person * pp[], Person * p);
    void removeFromPP(Person * pp[], Person * p);
    Lock * personLock;
//    Lock *floorLock;
    int getFloor(Person ** pp[], bool at);
    bool onElevator(Person * p);
};

#endif
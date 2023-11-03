#include "pcb.h"
#include "system.h"
extern Thread *currentThread;			// the thread holding the CPU

PCB::PCB(int id) {

    pid = id;
    parent = NULL;
    children = new List();
    thread = NULL;
    exitStatus = -9999;
}


void decspn(int arg) {
    ASSERT(arg != 0);
    PCB* pcb = (PCB*)arg;
    if (pcb->HasExited()) pcbManager->DeallocatePCB(pcb);
    else pcb->parent = NULL;
}


PCB::~PCB() {
    // children->Mapcar(decspn);
    delete children;
    if (thread != NULL && thread != currentThread) {
        if (thread->space != NULL) {
            delete thread->space;
            thread->space = NULL;
        }
        scheduler->RemoveThread(thread);
        if (thread->getStatus() != DELETED)
            delete thread;
        thread = NULL;
    }

    // pcbManager->DeallocatePCB(this);
}



void PCB::AddChild(PCB* pcb) {

    children->Append(pcb);

}


int PCB::RemoveChild(PCB* pcb){
    return children->RemoveItem(pcb);
}


bool PCB::HasExited() {
    return exitStatus == -9999 ? false : true;
}




void PCB::DeleteExitedChildrenSetParentNull() {
    if (children != NULL)
        children->Mapcar(decspn);
}
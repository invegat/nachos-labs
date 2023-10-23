#include "pcb.h"


PCB::PCB(int id) {

    pid = id;
    parent = NULL;
    children = new List();
    thread = NULL;
    exitStatus = -9999;

}


void decspn(int arg) {
    PCB* pcb = (PCB*)arg;
    if (pcb == NULL) return;
    if (pcb->HasExited()) pcbManager->DeallocatePCB(pcb);
//    if (pcb->HasExited()) delete pcb;
    else pcb->parent = NULL;
}


PCB::~PCB() {
    // children->Mapcar(decspn);
    delete children;
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
    children->Mapcar(decspn);
}
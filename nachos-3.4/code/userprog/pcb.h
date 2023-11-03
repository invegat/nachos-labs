#ifndef PCB_H
#define PCB_H
class Thread;
class PCBManager;

#ifdef __JETBRAINS_IDE__
#include "../threads/list.h"
#else
#include "list.h"
#endif
#include "pcbmanager.h"
extern PCBManager* pcbManager;

class PCB {

    public:
        PCB(int id);
        ~PCB();
        int pid;
        PCB* parent;
        Thread* thread;
        int exitStatus;

        void AddChild(PCB* pcb);
        int RemoveChild(PCB* pcb);
        bool HasExited();
        void DeleteExitedChildrenSetParentNull();

    private:
        List* children;

};

#endif // PCB_H
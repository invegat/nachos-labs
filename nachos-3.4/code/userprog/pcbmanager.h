#ifndef PCBMANAGER_H
#define PCBMANAGER_H
#ifdef __JETBRAINS_IDE__
    #include "../threads/synch.h"
#else
    #include "synch.h"
#endif

#include "bitmap.h"
#include "pcb.h"

class PCB;

class PCBManager {

    public:
        PCBManager(int maxProcesses);
        ~PCBManager();

        PCB* AllocatePCB();
        int DeallocatePCB(PCB* pcb);
        PCB* GetPCB(int pid);
        PCB* GetLastPCB();

    private:
        BitMap* bitmap;
        PCB** pcbs;
        Lock* pcbManagerLock;
        int count;

};

#endif // PCBMANAGER_H
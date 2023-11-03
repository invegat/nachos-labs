#include "pcbmanager.h"


PCBManager::PCBManager(int maxProcesses) {

    bitmap = new BitMap(maxProcesses);
    pcbs = new PCB*[maxProcesses];
    pcbManagerLock = new Lock("PCB Manager Lock") ;
    for(int i = 0; i < maxProcesses; i++) {
        pcbs[i] = NULL;
    }
    count = maxProcesses;

}


PCBManager::~PCBManager() {

    delete bitmap;

    delete pcbs;

}


PCB* PCBManager::AllocatePCB() {

    // Aquire pcbManagerLock
    pcbManagerLock->Acquire();

    int pid = bitmap->Find();

    // Release pcbManagerLock

    ASSERT(pid != -1);

    pcbs[pid] = new PCB(pid);
    pcbManagerLock->Release();
    return pcbs[pid];
}


int PCBManager::DeallocatePCB(PCB* pcb) {

    // Check is pcb is valid -- check pcbs for pcb->pid
    if (pcb->pid < 0 || pcb->pid > bitmap->NumItems())
        return -1;

     // Aquire pcbManagerLock
    pcbManagerLock->Acquire();

    bitmap->Clear(pcb->pid);
//    pcb->exitStatus = -9999;

//    printf("pid %d %d\n", pcb->pid+1, bitmap->Test(pcb->pid)  );


    int pid = pcb->pid;

    delete pcb;

    pcbs[pid] = NULL;

    // Release pcbManagerLock
    pcbManagerLock->Release();

    return 0;

}

PCB* PCBManager::GetPCB(int pid) {
    return pcbs[pid];
}

PCB* PCBManager::GetLastPCB() {
    PCB * last = NULL;
    for (int i=0;i<count;i++)
        if (pcbs[i] != NULL)
            last = pcbs[i];
    return last;
}
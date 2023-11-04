// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() and Fork system calls.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.
#ifdef __JETBRAINS_IDE__
    #include "../threads/copyright.h"
    #include "../threads/system.h"
    #include "../threads/synch.h"
    #include "../threads/thread.h"
    #include "../filesys/filehdr.h"
#else
    #include "copyright.h"
    #include "system.h"
    #include "synch.h"
#endif
#include "syscall.h"
#undef SEMAPHORE

#ifdef SEMAPHORE
Semaphore* yieldSemaphore = new Semaphore("Yield Semaphore", false);
Semaphore* execSemaphore = new Semaphore("Exec Semaphore", false);
//Semaphore* forkSemaphore = new Semaphore("Fork Semaphore", false);
#endif
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions
//	are in machine.h.
//----------------------------------------------------------------------
#ifdef SEMAPHORE
Semaphore* childSync = new Semaphore("child sync", 0);
#endif


void doExit(int status) {
    if (currentThread->space->GetPageTable() == NULL)
        return; // space was previously deleted

    PCB* pcb = currentThread->space->pcb;

    int pid = pcb->pid;

    printf("System Call: [%d] invoked [Exit]\n", pid);


    // Manage PCB memory As a parent process

    // Delete exited children and set parent null for non-exited ones

    currentThread->space->pcb->exitStatus = status;
    scheduler->RemoveThread(pcb->thread);
    pcb->DeleteExitedChildrenSetParentNull();

    // Manage PCB memory As a child process
    if(pcb->parent == NULL)
        pcbManager->DeallocatePCB(pcb);
//     else {
//        pcb->parent->RemoveChild(pcb);
//        delete pcb;
//    }


    // Delete address space only after use is completed
    delete currentThread->space;
    printf ("Process [%d] exits with [%d]\n", pid, status);

    // Finish current thread only after all the cleanup is done
    // because currentThread marks itself to be destroyed (by a different thread)
    // and then puts itself to sleep -- thus anything after this statement will not be executed!
    currentThread->Finish();
//    currentThread->Yield();
    ASSERT(FALSE);

}

void incrementPC() {
    int oldPCReg = machine->ReadRegister(PCReg);

    machine->WriteRegister(PrevPCReg, oldPCReg);
    machine->WriteRegister(PCReg, oldPCReg + 4);
    machine->WriteRegister(NextPCReg, oldPCReg + 8);
}


void childFunction(int pid) {

//    forkSemaphore->P();
    // 1. Restore the state of registers
    currentThread->RestoreUserState();

    // 2. Restore the page table for child
    currentThread->space->RestoreState();

    int pCReg = machine->ReadRegister(PCReg);
    // print message for child creation (pid,  PCReg, currentThread->space->GetNumPages())
//    printf("pid %d  PCReg %d  Pages %d\n", pid, pCReg, currentThread->space->GetNumPages());
    printf("Process [%d] Fork: start at address [%#x] with [%d] pages memory\n", pid, pCReg, currentThread->space->GetNumPages());
    fflush(stdout);
#ifdef SEMAPHORE
    childSync->V();
    execSemaphore->V();
#endif

    machine->Run();

}

int doFork(int functionAddr) {
//    printf("System Call: [%d] invoked Fork.\n", currentThread->space->pcb->pid);
    // 1. Check if sufficient memory exists to create new process
    if (currentThread->space->GetNumPages() > mm->GetFreePageCount()) {
        printf("Not Enough Memory for Child Process %d", currentThread->space->pcb->pid );
        return -1;
    }
    // if check fails, return -1
    // 2. SaveUserState for the parent thread
    PCB* pcb = pcbManager->AllocatePCB();
    currentThread->SaveUserState();
    printf("System Call: [%d] invoked Fork.\n",pcb->pid);
    fflush(stdout);

    // 3. Create a new address space for child by copying parent address space
    // Parent: currentThread->space

    AddrSpace* childAddrSpace = new AddrSpace(currentThread->space);
    if (!childAddrSpace->valid) {
        printf("Could not create AddrSpace\n");
        return -1;
    }

    // 4. Create a new thread for the child and set its addrSpace
    Thread* childThread = new Thread("childThread");
    childThread->space = childAddrSpace;

    // 5. Create a PCB for the child and connect it all up
    pcb->thread = childThread;
    childThread->space->pcb = pcb;
    // set parent for child pcb
    pcb->parent = currentThread->space->pcb;
    // add child for parent pcb
    currentThread->space->pcb->AddChild(pcb);

    // 6. Set up machine registers for child and save it to child thread
    machine->WriteRegister(PCReg, functionAddr);
    // PCReg: functionAddr
    machine->WriteRegister(PrevPCReg, functionAddr-4);
    // PrevPCReg: functionAddr-4
    machine->WriteRegister(NextPCReg, functionAddr+4);
    childThread->SaveUserState();

    // 7. Restore register state of parent user-level process
    currentThread->RestoreUserState();
    // 8. Call thread->fork on Child
    childThread->Fork(childFunction, pcb->pid);
#ifdef SEMAPHORE
    childSync->P();
#endif

    return pcb->pid;

}

int doExec(char* filename) {

    // Use progtest.cc:StartProcess() as a guide
#ifdef SEMAPHORE
    yieldSemaphore->P(); // wait for Yield
    execSemaphore->P();  // wait for ChildTread of fork
#endif
    // 1. Open the file and check validity
     OpenFile *executable = fileSystem->Open(filename);
     AddrSpace *space;

     if (executable == NULL) {
         printf("Unable to open file %s\n", filename);
         return -1;
     }
    // 2. Delete current address space but store current PCB first if using in Step 5.
     PCB *pcb = NULL;
     if (currentThread->space != NULL) {
         pcb = currentThread->space->pcb;
         printf("Syscall Call: [%d] invoked Exec.\n", pcb->pid);
         delete currentThread->space;
         currentThread->space = NULL;
     }
     else {
         delete executable;			// close file
         return -1;
     }

    // 3. Create new address space
     space = new AddrSpace(executable, pcb);
     printf("Exec Program: [%d] loading [%s]\n", pcb->pid, filename);

    // 4.
     delete executable;			// close file

    // 5. Check if Addrspace creation was successful
     if(space->valid != true) {
         printf("Could not create AddrSpace\n");
         return -1;
     }

    // 6. space->pcb was set in the constructor
    // space->pcb = pcb;

    // 7. Set the addrspace for currentThread
     currentThread->space = space;

    // 8. Initialize registers for new addrspace
      space->InitRegisters();		// set the initial register values

    // 9. Initialize the page table
     space->RestoreState();		// load page table register

    // 10. Run the machine now that all is set up
     // machine->Run();			// jump to the user progam
     // ASSERT(FALSE); // Execution never reaches here

    return pcb->pid;
}


int doJoin(int pid) {

    // 1. Check if this is a valid pid and return -1 if not
    PCB* joinPCB = pcbManager->GetPCB(pid);

    // 2. Check if pid is a child of current process
    PCB* pcb = currentThread->space->pcb;
    if (pcb == NULL) return -1;
    if (joinPCB == NULL) return -1;
    if (pcb != joinPCB->parent) return -1;

    // 3. Yield until joinPCB has not exited
    while(!joinPCB->HasExited()) currentThread->Yield();

    // 4. Store status and delete joinPCB
    int status = joinPCB->exitStatus;
    delete joinPCB;
    pcb = currentThread->space->pcb;
    printf("System Call: [%d] invoked Join.\n", pcb->pid);
    // 5. return status;
    return status;

}


int doKill (int pid) {
#ifdef SEMAPHORE
    yieldSemaphore->P();
    execSemaphore->P();
#endif
    printf("System Call: [%d] invoked Kill.\n", pid);
    // 1. Check if the pid is valid and if not, return -1
     PCB* pcb = pcbManager->GetPCB(pid);
     if (pcb == NULL) return -1;

    // 2. IF pid is self, then just exit the process
     if (pcb == currentThread->space->pcb) {
             doExit(0);  // never returns
             ASSERT(false);
     }
     int pcbPid = pcb->pid;
    // 3. Valid kill, pid exists and not self - Set thread to be destroyed.
    scheduler->RemoveThread(pcb->thread);

    // 4. do cleanup similar to Exit
    // However, change references from currentThread to the target thread
    // pcb->thread is the target thread
//    pcb->exitStatus = 0;
//    if (pcb->parent != NULL)
//        pcb->parent->DeleteExitedChildrenSetParentNull();
//    else
//        pcbManager->DeallocatePCB(pcb);
//    delete pcb;
    if (pcb->parent != NULL)
        pcb->parent->RemoveChild(pcb);
    pcb->DeleteExitedChildrenSetParentNull();
    printf("Process [%d] killed process [%d]\n",currentThread->space->pcb->pid, pcbPid);
    pcb->thread->setStatus(BLOCKED);

//    pcb->thread->Finish();
    // 5. return 0 for success!
    return 0;
}



void doYield() {
//    PCB* pcb = pcbManager->GetLastPCB();
    PCB * pcb = currentThread->space->pcb;
    currentThread->Yield();
    printf("System Call: [%d] invoked Yield.\n", pcb->pid);
    fflush(stdout);
#ifdef SEMAPHORE
    yieldSemaphore->V();
#endif
}


// This implementation is correct!
// perform MMU translation to access physical memory
char* readString(int virtualAddr) {
    int i = 0;
    char* str = new char[256];
    unsigned int physicalAddr = currentThread->space->Translate(virtualAddr);

    // Need to get one byte at a time since the string may straddle multiple pages that are not guaranteed to be contiguous in the physicalAddr space
    bcopy(&(machine->mainMemory[physicalAddr]),&str[i],1);
    while(str[i] != '\0' && i != 256-1)
    {
        virtualAddr++;
        i++;
        physicalAddr = currentThread->space->Translate(virtualAddr);
        bcopy(&(machine->mainMemory[physicalAddr]),&str[i],1);
    }
    if(i == 256-1 && str[i] != '\0')
    {
        str[i] = '\0';
    }

    return str;
}

void doCreate(char* fileName)
{
    printf("Syscall Call: [%d] invoked Create.\n", currentThread->space->pcb->pid);
    fileSystem->Create(fileName, 0);
}

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException) && (type == SC_Halt)) {
	DEBUG('a', "Shutdown, initiated by user program.\n");
   	interrupt->Halt();
    } else  if ((which == SyscallException) && (type == SC_Exit)) {
        // Implement Exit system call
        doExit(machine->ReadRegister(4));
    } else if ((which == SyscallException) && (type == SC_Fork)) {
        int ret = doFork(machine->ReadRegister(4));
        machine->WriteRegister(2, ret);
        incrementPC();
    } else if ((which == SyscallException) && (type == SC_Exec)) {
        int virtAddr = machine->ReadRegister(4);
        char* fileName = readString(virtAddr);
        doExec(fileName);
        incrementPC();
        machine->Run();
    } else if ((which == SyscallException) && (type == SC_Join)) {
        int ret = doJoin(machine->ReadRegister(4));
        machine->WriteRegister(2, ret);
        incrementPC();
    } else if ((which == SyscallException) && (type == SC_Kill)) {
        int ret = doKill(machine->ReadRegister(4));
        machine->WriteRegister(2, ret);
        incrementPC();
//        machine->Run();
    } else if ((which == SyscallException) && (type == SC_Yield)) {
        doYield();
        incrementPC();
    } else if((which == SyscallException) && (type == SC_Create)) {
        int virtAddr = machine->ReadRegister(4);
        char* fileName = readString(virtAddr);
        doCreate(fileName);
        incrementPC();
    } else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }

}


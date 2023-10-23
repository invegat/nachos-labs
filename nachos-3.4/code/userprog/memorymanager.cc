
#ifdef __JETBRAINS_IDE__
    #include "../machine/machine.h"
#else
    #include "machine.h"
#endif

#include "memorymanager.h"


MemoryManager::MemoryManager() {

    bitmap = new BitMap(NumPhysPages);
    lock = new Lock("mm lock");

}


MemoryManager::~MemoryManager() {

    delete bitmap;
    delete lock;

}
int MemoryManager::getPage() {
    return this->AllocatePage();
}
void MemoryManager::clearPage(int pageId) {
    bitmap->Clear(pageId);
}
int MemoryManager::AllocatePage() {

    return bitmap->Find();

}

int MemoryManager::DeallocatePage(int which) {

    if(bitmap->Test(which) == false) return -1;
    else {
        bitmap->Clear(which);
        return 0;
    }

}


unsigned int MemoryManager::GetFreePageCount() {

    return bitmap->NumClear();

}



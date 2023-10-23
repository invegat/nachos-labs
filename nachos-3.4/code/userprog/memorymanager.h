#ifndef MEMORY_H
#define MEMORY_H
#ifdef __JETBRAINS_IDE__
    #include "../threads/synch.h"
#else
    class Threads;
    #include "synch.h"
#endif

#include "bitmap.h"

class MemoryManager {

    public:
        MemoryManager();
        ~MemoryManager();

        int AllocatePage();
        int DeallocatePage(int which);
        unsigned int GetFreePageCount();
        int getPage();
        void clearPage (int pageId);

    private:
        BitMap *bitmap;
        Lock *lock;


};



#endif // MEMORY_H
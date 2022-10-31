#include "conditional_variable.h"
#include <thread_internal.h>

void CondVariableInit(
    OUT     PCONDITIONAL_VARIABLE   CondVariable
) {

    InitializeListHead(&CondVariable->WaitingListHead);
}

void CondVariableWait(
    INOUT   PCONDITIONAL_VARIABLE   CondVariable,
    INOUT   PMUTEX                  Lock
) {

    PTHREAD Thread = GetCurrentThread();
    InsertHeadList(&CondVariable->WaitingListHead, &Thread->ReadyList);
    ThreadTakeBlockLock();
    INTR_STATE state;
    MutexAcquire(Lock);
    ThreadBlock();
    MutexRelease(Lock);
}

void CondVariableBroadcast(
    INOUT   PCONDITIONAL_VARIABLE   CondVariable,
    INOUT   PMUTEX                  Lock
) {

    PTHREAD Thread;
    while ((Thread = RemoveHeadList(&CondVariable->WaitingListHead)) != NULL)
    {
        INTR_STATE state;
        MutexRelease(Lock);
        ThreadBlock();
        MutexAcquire(Lock);
    }
}
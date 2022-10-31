#pragma once
#include <common_lib.h>
#include <lock_common.h>
#include <list.h>
#include <mutex.h>


typedef struct _CONDITIONAL_VARIABLE
{
    LIST_ENTRY      WaitingListHead;

} CONDITIONAL_VARIABLE, *PCONDITIONAL_VARIABLE;

void CondVariableInit(
    OUT     PCONDITIONAL_VARIABLE   CondVariable
);

void CondVariableWait(
    INOUT   PCONDITIONAL_VARIABLE   CondVariable,
    INOUT   PMUTEX                  Lock
);

void CondVariableBroadcast(
    INOUT   PCONDITIONAL_VARIABLE   CondVariable,
    INOUT   PMUTEX                  Lock
);
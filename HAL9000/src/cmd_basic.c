#include "HAL9000.h"
#include "cmd_fs_helper.h"
#include "display.h"
#include "dmp_io.h"
#include "print.h"
#include "iomu.h"
#include "test_common.h"
#include "strutils.h"
#include "thread_internal.h"
#include "thread.h"

void
CmdPrintVolumeInformation(
    IN      QWORD           NumberOfParameters
    )
{
    ASSERT(NumberOfParameters == 0);

    printColor(MAGENTA_COLOR, "%7s", "Letter|");
    printColor(MAGENTA_COLOR, "%17s", "Type|");
    printColor(MAGENTA_COLOR, "%10s", "Mounted|");
    printColor(MAGENTA_COLOR, "%10s", "Bootable|");
    printColor(MAGENTA_COLOR, "%17s", "Offset|");
    printColor(MAGENTA_COLOR, "%17s", "Size|");
    printColor(MAGENTA_COLOR, "\n");

    IomuExecuteForEachVpb(DumpVpb, NULL, FALSE);
}

#pragma warning(push)

// warning C4717: '_CmdInfiniteRecursion': recursive on all control paths, function will cause runtime stack overflow
#pragma warning(disable:4717)
void
CmdInfiniteRecursion(
    IN      QWORD           NumberOfParameters
    )
{
    ASSERT(NumberOfParameters == 0);

    CmdInfiniteRecursion(NumberOfParameters);
}
#pragma warning(pop)

void
CmdRtcFail(
    IN      QWORD           NumberOfParameters
    )
{
    char buffer[] = "Alex is a smart boy!\n";

    ASSERT(NumberOfParameters == 0);

    strcpy(buffer, "Alex is a very dumb boy!\n");
}

void
CmdRangeFail(
    IN      QWORD           NumberOfParameters
    )
{
    ASSERT(NumberOfParameters == 0);

    perror("Cannot implement! :(\n");
}

void
(__cdecl CmdBiteCookie)(
    IN      QWORD           NumberOfParameters
    )
{
    char buffer[] = "Alex is a smart boy!\n";

    ASSERT(NumberOfParameters == 0);

    strcpy(buffer + sizeof(buffer) + sizeof(PVOID), "Alex is a very dumb boy!\n");
}

void
(__cdecl CmdLogSetState)(
    IN      QWORD           NumberOfParameters,
    IN      char*           LogState
    )
{
    ASSERT(NumberOfParameters == 1);

    LogSetState(stricmp(LogState, "ON") == 0);
}

void
(__cdecl CmdSetLogLevel)(
    IN      QWORD           NumberOfParameters,
    IN      char*           LogLevelString
    )
{
    LOG_LEVEL logLevel;

    ASSERT(NumberOfParameters == 1);

    atoi32(&logLevel, LogLevelString, BASE_TEN);

    if (logLevel > LogLevelError)
    {
        perror("Invalid log level %u specified!\n", logLevel);
        return;
    }

    printf("Will set logging level to %u\n", logLevel);
    LogSetLevel(logLevel);
}

void
(__cdecl CmdSetLogComponents)(
    IN      QWORD           NumberOfParameters,
    IN      char*           LogComponentsString
    )
{
    LOG_COMPONENT logComponents;

    ASSERT(NumberOfParameters == 1);

    atoi32(&logComponents, LogComponentsString, BASE_HEXA);

    printf("Will set logging components to 0x%x\n", logComponents);

    LogSetTracedComponents(logComponents);
}

void
(__cdecl CmdClearScreen)(
    IN          QWORD       NumberOfParameters
    )
{
    ASSERT(NumberOfParameters == 0);

    DispClearScreen();
}

void
(__cdecl CmdRunAllFunctionalTests)(
    IN          QWORD       NumberOfParameters
    )
{
    ASSERT(NumberOfParameters == 0);

    TestRunAllFunctional();
}

void
(__cdecl CmdRunAllPerformanceTests)(
    IN          QWORD       NumberOfParameters
    )
{
    ASSERT(NumberOfParameters == 0);

    TestRunAllPerformance();
}

typedef struct _DATA_STRUCTURE {

    DWORD Value;

    LIST_ENTRY ListEntry;

} DATA_STRUCTURE, *PDATA_STRUCTURE;

static
STATUS
(__cdecl MyListFunc) (
    IN      PLIST_ENTRY     ListEntry,
    IN_OPT  PVOID           FunctionContext
    ) {

    UNREFERENCED_PARAMETER(FunctionContext);
    PDATA_STRUCTURE pDataStructure = CONTAINING_RECORD(ListEntry, DATA_STRUCTURE, ListEntry);
    LOG("%d ", pDataStructure->Value);
    return STATUS_SUCCESS;
}

void
(__cdecl CmdMyTest)(
    IN          QWORD       NumberOfParameters,
    IN          char*       Param1,
    IN          char*       Param2
    )
{
    DWORD dwMin, dwMax;
    LIST_ENTRY head;
    InitializeListHead(&head);
    dwMin = 1;
    dwMax = 1;
    if (NumberOfParameters == 1) {

        atoi(&dwMax, Param1, 10, FALSE);
        dwMin = 1;
    }
    else if (NumberOfParameters == 2) {

        atoi(&dwMin, Param1, 10, FALSE);
        atoi(&dwMax, Param2, 10, FALSE);
    }

    PDATA_STRUCTURE pDataStructure = NULL;
    for (DWORD i = dwMin; i <= dwMax; i++) {
        
        pDataStructure = ExAllocatePoolWithTag(PoolAllocateZeroMemory, sizeof(DATA_STRUCTURE), HEAP_TEST_TAG, PAGE_SIZE);
        if (pDataStructure == NULL) {

            LOG_FUNC_ERROR("ExAllocatePoolWithTag", STATUS_HEAP_INSUFFICIENT_RESOURCES);
            return;
        }
        pDataStructure->Value = i;
        InsertTailList(&head, &(pDataStructure->ListEntry));
    }

    for (PLIST_ENTRY pEntry = head.Flink; pEntry != &head; pEntry = pEntry->Flink) {

        pDataStructure = CONTAINING_RECORD(pEntry, DATA_STRUCTURE, ListEntry);
        LOG("%d ", pDataStructure->Value);
    }
    LOG("\n");

    ForEachElementExecute(&head, MyListFunc, NULL, FALSE);
    LOG("\n");

    while (!IsListEmpty(&head)) {

        PLIST_ENTRY pEntry = RemoveHeadList(&head);
        pDataStructure = CONTAINING_RECORD(pEntry, DATA_STRUCTURE, ListEntry);
        ExFreePoolWithTag(pDataStructure, HEAP_TEST_TAG);
    }
}


STATUS EmptyFunc(
    IN_OPT PVOID Context
) {

    UNREFERENCED_PARAMETER(Context);
    return STATUS_SUCCESS;
}
 
typedef struct _ARGS {

    DWORD value1;
    DWORD value2;

} ARGS, PARGS;

STATUS ThreadFunc1(
    IN_OPT PVOID Context
) {

    UNREFERENCED_PARAMETER(Context);

    for (DWORD i = 0; i < 10; i++) {

        char threadName[10];
        sprintf(threadName, "T_%d", i);

        PTHREAD pThread;
        ThreadCreate(threadName, ThreadPriorityDefault, EmptyFunc, NULL, &pThread);
    }

    return STATUS_SUCCESS;
}

void
(__cdecl CmdTestDescendents)(
    IN          QWORD       NumberOfParameters
    )
{
    ASSERT(NumberOfParameters == 0);

    PTHREAD pThread;
    ThreadCreate("a", ThreadPriorityDefault, ThreadFunc1, NULL, &pThread);
}

void
(__cdecl CmdShow)(
    IN          QWORD       NumberOfParameters
    )
{
    ASSERT(NumberOfParameters == 0);

    /*PLOCK pLock = GetAllThreadsByCreatedTimeLock();
    PLIST_ENTRY pHead = GetAllThreadsByCreatedTimeListHead();

    INTR_STATE state;
    LockAcquire(pLock, &state);

    for (PLIST_ENTRY pEntry = pHead->Flink; pEntry != pHead; pEntry = pEntry->Flink) {

        PTHREAD pThread = CONTAINING_RECORD(pEntry, THREAD, AllListByCreatedTime);
        LOG("Thread [tid = %d] was created at %d\n", pThread->Id, pThread->CreateTime);
    }

    LockRelease(pLock, state);*/
}
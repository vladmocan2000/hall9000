#include "HAL9000.h"
#include "cmd_fs_helper.h"
#include "display.h"
#include "dmp_io.h"
#include "print.h"
#include "iomu.h"
#include "test_common.h"
#include "strutils.h"

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


void
(__cdecl CmdTest)(
    IN          QWORD       NumberOfParameters,
    IN          char*       param1,
    IN          char*       param2
    )
{
    if (NumberOfParameters == 1) {

        LOG("The function has 1 parameter: %s.\n", param1);
    }
    else {

        LOG("The function has 2 parameters: %s, %s.\n", param1, param2);
    }
}

typedef struct _MY_ENTRY {

    LIST_ENTRY      ListEntry;
    DWORD           Value;

} MY_ENTRY, * PMY_ENTRY;

static
STATUS
(__cdecl _MyListFunction) (
    IN      PLIST_ENTRY     ListEntry,
    IN_OPT  PVOID           FunctionContext
    )
{
    UNREFERENCED_PARAMETER(FunctionContext);
    PMY_ENTRY pMyEntry = CONTAINING_RECORD(ListEntry, MY_ENTRY, ListEntry);
    LOG("%d, ", pMyEntry->Value);

    return STATUS_SUCCESS;
}

void 
(__cdecl CmdLists)(
    IN          QWORD       NumberOfParameters,
    IN          char*       firstIntervalLimit,
    IN          char*       secondIntervalLimit
    )
{
    if (NumberOfParameters == 0) {
        return;
    }
    DWORD dwMin = 0, dwMax = 0;
    atoi(&dwMin, firstIntervalLimit, 10, FALSE);
    atoi(&dwMax, secondIntervalLimit, 10, FALSE);

    if (dwMin > dwMax) {

        dwMax ^= dwMin;
        dwMin ^= dwMax;
        dwMax ^= dwMin;
    }

    LIST_ENTRY head;
    InitializeListHead(&head);
    for (DWORD dwIndex = dwMin; dwIndex <= dwMax; dwIndex++) {

        PMY_ENTRY pMyEntry = ExAllocatePoolWithTag(PoolAllocateZeroMemory, sizeof(MY_ENTRY), HEAP_TEST_TAG, PAGE_SIZE);
        if (pMyEntry == NULL) {

            LOG_FUNC_ERROR("ExAllocatePoolWithTag", STATUS_HEAP_INSUFFICIENT_RESOURCES);
        }

        pMyEntry->Value = dwIndex;
        InsertTailList(&head, &(pMyEntry->ListEntry));
    }


    for (PLIST_ENTRY pEntry = head.Flink; pEntry != &head; pEntry = pEntry->Flink) {

        PMY_ENTRY pMyEntry = CONTAINING_RECORD(pEntry, MY_ENTRY, ListEntry);
        LOG("%d, ", pMyEntry->Value);
    }
    LOG("\n");

    LIST_ITERATOR iterator;
    PLIST_ENTRY pListEntry;
    ListIteratorInit(&head, &iterator);
    while ((pListEntry = ListIteratorNext(&iterator)) != NULL) {

        PMY_ENTRY pMyEntry = CONTAINING_RECORD(pListEntry, MY_ENTRY, ListEntry);
        LOG("%d, ", pMyEntry->Value);
    }
    LOG("\n");

    ForEachElementExecute(&head, _MyListFunction, NULL, FALSE);
    LOG("\n");

    while(!IsListEmpty(&head)) {

        pListEntry = RemoveHeadList(&head);
        PMY_ENTRY pMyEntry = CONTAINING_RECORD(pListEntry, MY_ENTRY, ListEntry);
        ExFreePoolWithTag(pMyEntry, HEAP_TEST_TAG);
    }
}
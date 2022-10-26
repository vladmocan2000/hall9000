#include "test_lp.h"
#include <thread_internal.h>

static volatile DWORD gNumberOfThreads;

typedef struct _THREAD_LIST_ENTRY {

	LIST_ENTRY ListEntry;
	PTHREAD Thread;

} THREAD_LIST_ENTRY, * PTHREAD_LIST_ENTRY;

STATUS (__cdecl _ThreadLpTest)(QWORD Context)
{
	LIST_ENTRY head;
	InitializeListHead(&head);

	QWORD numberOfChildren = (QWORD)Context;
	QWORD i;
	Context--;
	for (i = 0; i < numberOfChildren; i++)
	{
		PTHREAD thread;
		char thName[MAX_PATH];
		snprintf(thName, MAX_PATH, "ThreadLp-%d ", _InterlockedIncrement(&gNumberOfThreads));
		STATUS status = ThreadCreate(thName,
			ThreadPriorityDefault,
			(PFUNC_ThreadStart)_ThreadLpTest,
			(PVOID)Context,
			&thread
		);
		if (!SUCCEEDED(status))
		{
			LOG_FUNC_ERROR("ThreadCreate", status);
		}
		else
		{
			PTHREAD_LIST_ENTRY pThread = ExAllocatePoolWithTag(PoolAllocateZeroMemory, sizeof(THREAD_LIST_ENTRY), HEAP_TEST_TAG, PAGE_SIZE);
			if (pThread == NULL) {

				LOG_FUNC_ERROR("ExAllocatePoolWithTag", STATUS_HEAP_INSUFFICIENT_RESOURCES);
			}

			pThread->Thread = thread;
			InsertTailList(&head, &(pThread->ListEntry));
		}
	}

	while(!IsListEmpty(&head)) {

		PLIST_ENTRY pListEntry = RemoveHeadList(&head);
		PTHREAD_LIST_ENTRY threadListEntry = CONTAINING_RECORD(pListEntry, THREAD_LIST_ENTRY, ListEntry);
		STATUS status;
		ThreadWaitForTermination(threadListEntry->Thread, &status);
		ThreadCloseHandle(threadListEntry->Thread);
		ExFreePoolWithTag(threadListEntry, HEAP_TEST_TAG);
	}

	return 0;
}
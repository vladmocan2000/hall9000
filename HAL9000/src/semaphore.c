#include "semaphore.h"
#include <thread_internal.h>

void SemaphoreInit(
	OUT			PSEMAPHORE		Semaphore,
	IN			DWORD			InitialValue
) {

	ASSERT(Semaphore != NULL);

	memzero(Semaphore, sizeof(SEMAPHORE));
	LockInit(&Semaphore->Lock);
	InitializeListHead(&Semaphore->WaitingListHead);

	INTR_STATE state;
	LockAcquire(&Semaphore->Lock, &state);
	Semaphore->Value = InitialValue;
	LockRelease(&Semaphore->Lock, state);
}

void SemaphoreDown(
	INOUT		PSEMAPHORE		Semaphore,
	IN			DWORD			Value
) {

	if (Semaphore->Value >= Value) {
		
		INTR_STATE state;
		LockAcquire(&Semaphore->Lock, &state);
		Semaphore->Value -= Value;
		LockRelease(&Semaphore->Lock, state);
	}
	else {

		PTHREAD Thread = GetCurrentThread();
		InsertTailList(&Semaphore->WaitingListHead, &Thread->ReadyList);
		ThreadTakeBlockLock();
		INTR_STATE state;
		LockRelease(&Semaphore->Lock, &state);
		ThreadBlock();
		LockAcquire(&Semaphore->Lock, state);
	}
}

void SemaphoreUp(
	INOUT		PSEMAPHORE		Semaphore,
	IN			DWORD			Value
) {

	INTR_STATE state;
	LockAcquire(&Semaphore->Lock, &state);
	Semaphore->Value += Value;
	LockRelease(&Semaphore->Lock, state);

	for (int i = 0; i < Value; i++) {

		PTHREAD Thread = CONTAINING_RECORD(RemoveHeadList(&Semaphore->WaitingListHead), THREAD, ReadyList);
		if (Thread != NULL) {
			
			ThreadUnblock(Thread);
		}
	}
}
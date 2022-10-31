#pragma once
#include <common_lib.h>
#include <lock_common.h>

typedef struct _SEMAPHORE {
	
	LOCK			Lock;
	DWORD			Value;
	LIST_ENTRY      WaitingListHead;

} SEMAPHORE, *PSEMAPHORE;

void SemaphoreInit(
	OUT			PSEMAPHORE		Semaphore,
	IN			DWORD			InitialValue
);

void SemaphoreDown(
	INOUT		PSEMAPHORE		Semaphore,
	IN			DWORD			Value
);

void SemaphoreUp(
	INOUT		PSEMAPHORE		Semaphore,
	IN			DWORD			Value
);
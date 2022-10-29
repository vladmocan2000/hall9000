#include "HAL9000.h"
#include "ex_timer.h"
#include "iomu.h"
#include "thread_internal.h"
#include "ex_event.h"

struct _GLOBAL_TIMER_LIST
{
    LOCK                TimerListLock;

    LIST_ENTRY          TimerListHead;
};

static struct  _GLOBAL_TIMER_LIST m_globalTimerList;

static
INT64
(__cdecl _TimerCompareFunction) (
    IN      PLIST_ENTRY     FirstTimer,
    IN      PLIST_ENTRY     SecondTimer,
    IN_OPT  PVOID           Context
    )
{
    ASSERT(NULL != FirstTimer);
    ASSERT(NULL != SecondTimer);
    ASSERT(Context == NULL);

    PEX_TIMER pFirstTimer = CONTAINING_RECORD(FirstTimer, EX_TIMER, TimerListElem);
    PEX_TIMER pSecondTimer = CONTAINING_RECORD(SecondTimer, EX_TIMER, TimerListElem);
    
    return ExTimerCompareTimers(pFirstTimer, pSecondTimer);
}

STATUS
ExTimerInit(
    OUT     PEX_TIMER       Timer,
    IN      EX_TIMER_TYPE   Type,
    IN      QWORD           Time
    )
{
    STATUS status;

    if (NULL == Timer)
    {
        return STATUS_INVALID_PARAMETER1;
    }

    if (Type > ExTimerTypeMax)
    {
        return STATUS_INVALID_PARAMETER2;
    }

    status = STATUS_SUCCESS;

    memzero(Timer, sizeof(EX_TIMER));

    Timer->Type = Type;
    if (Timer->Type != ExTimerTypeAbsolute)
    {
        // relative time

        // if the time trigger time has already passed the timer will
        // be signaled after the first scheduler tick
        Timer->TriggerTimeUs = IomuGetSystemTimeUs() + Time;
        Timer->ReloadTimeUs = Time;
    }
    else
    {
        // absolute
        Timer->TriggerTimeUs = Time;
    }

    ExEventInit(&Timer->TimerEvent, ExEventTypeNotification, FALSE);

    INTR_STATE oldState;
    LockAcquire(&m_globalTimerList.TimerListLock, &oldState);
    InsertOrderedList(&m_globalTimerList.TimerListHead, &Timer->TimerListElem, _TimerCompareFunction, NULL);
    LockRelease(&m_globalTimerList.TimerListLock, oldState);

    return status;
}

void
ExTimerStart(
    IN      PEX_TIMER       Timer
    )
{
    ASSERT(Timer != NULL);

    if (Timer->TimerUninited)
    {
        return;
    }

    Timer->TimerStarted = TRUE;
}

void
ExTimerStop(
    IN      PEX_TIMER       Timer
    )
{
    ASSERT(Timer != NULL);

    if (Timer->TimerUninited)
    {
        return;
    }

    Timer->TimerStarted = FALSE;
    ExEventSignal(&Timer->TimerEvent);
}

void
ExTimerWait(
    INOUT   PEX_TIMER       Timer
    )
{
    ASSERT(Timer != NULL);

    if (Timer->TimerUninited)
    {
        return;
    }

    ExEventWaitForSignal(&Timer->TimerEvent);
}

void
ExTimerUninit(
    INOUT   PEX_TIMER       Timer
    )
{
    ASSERT(Timer != NULL);

    ExTimerStop(Timer);

    INTR_STATE oldState;
    LockAcquire(&m_globalTimerList.TimerListLock, &oldState);
    if (Timer->TimerListElem.Flink->Blink == &Timer->TimerListElem && Timer->TimerListElem.Blink->Flink == &Timer->TimerListElem) {

        RemoveEntryList(&Timer->TimerListElem);
    }
    LockRelease(&m_globalTimerList.TimerListLock, oldState);

    Timer->TimerUninited = TRUE;
}

INT64
ExTimerCompareTimers(
    IN      PEX_TIMER     FirstElem,
    IN      PEX_TIMER     SecondElem
)
{
    return FirstElem->TriggerTimeUs - SecondElem->TriggerTimeUs;
}

void ExTimerSystemPreinit() {

    InitializeListHead(&m_globalTimerList.TimerListHead);
    LockInit(&m_globalTimerList.TimerListLock);
}

BOOLEAN ExTimerCheck (
    IN      PLIST_ENTRY     ListEntry
    )
{
    PEX_TIMER pTimer = CONTAINING_RECORD(ListEntry, EX_TIMER, TimerListElem);
    if (pTimer->TriggerTimeUs <= IomuGetSystemTimeUs()) {

        ExEventSignal(&pTimer->TimerEvent);
        RemoveEntryList(&pTimer->TimerListElem);
        return TRUE;
    }

    return FALSE;
}

void ExTimerCheckAll() {

    INTR_STATE oldState;
    LockAcquire(&m_globalTimerList.TimerListLock, &oldState);
    while (!IsListEmpty(&m_globalTimerList.TimerListHead) && ExTimerCheck(m_globalTimerList.TimerListHead.Flink)) {}
    LockRelease(&m_globalTimerList.TimerListLock, oldState);
}
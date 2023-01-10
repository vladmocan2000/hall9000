#include "HAL9000.h"
#include "syscall.h"
#include "gdtmu.h"
#include "syscall_defs.h"
#include "syscall_func.h"
#include "syscall_no.h"
#include "mmu.h"
#include "process_internal.h"
#include "dmp_cpu.h"
#include "thread.h"
#include "thread_internal.h"

extern void SyscallEntry();

#define SYSCALL_IF_VERSION_KM       SYSCALL_IMPLEMENTED_IF_VERSION

void
SyscallHandler(
    INOUT   COMPLETE_PROCESSOR_STATE    *CompleteProcessorState
    )
{
    SYSCALL_ID sysCallId;
    PQWORD pSyscallParameters;
    PQWORD pParameters;
    STATUS status;
    REGISTER_AREA* usermodeProcessorState;

    ASSERT(CompleteProcessorState != NULL);

    // It is NOT ok to setup the FMASK so that interrupts will be enabled when the system call occurs
    // The issue is that we'll have a user-mode stack and we wouldn't want to receive an interrupt on
    // that stack. This is why we only enable interrupts here.
    ASSERT(CpuIntrGetState() == INTR_OFF);
    CpuIntrSetState(INTR_ON);

    LOG_TRACE_USERMODE("The syscall handler has been called!\n");

    status = STATUS_SUCCESS;
    pSyscallParameters = NULL;
    pParameters = NULL;
    usermodeProcessorState = &CompleteProcessorState->RegisterArea;

    __try
    {
        if (LogIsComponentTraced(LogComponentUserMode))
        {
            DumpProcessorState(CompleteProcessorState);
        }

        // Check if indeed the shadow stack is valid (the shadow stack is mandatory)
        pParameters = (PQWORD)usermodeProcessorState->RegisterValues[RegisterRbp];
        status = MmuIsBufferValid(pParameters, SHADOW_STACK_SIZE, PAGE_RIGHTS_READ, GetCurrentProcess());
        if (!SUCCEEDED(status))
        {
            LOG_FUNC_ERROR("MmuIsBufferValid", status);
            __leave;
        }

        sysCallId = usermodeProcessorState->RegisterValues[RegisterR8];

        LOG_TRACE_USERMODE("System call ID is %u\n", sysCallId);

        // The first parameter is the system call ID, we don't care about it => +1
        pSyscallParameters = (PQWORD)usermodeProcessorState->RegisterValues[RegisterRbp] + 1;

        // Dispatch syscalls
        switch (sysCallId)
        {
        case SyscallIdIdentifyVersion:
            status = SyscallValidateInterface((SYSCALL_IF_VERSION)*pSyscallParameters);
            break;
        // STUDENT TODO: implement the rest of the syscalls

        case SyscallIdProcessExit:
            status = SyscallProcessExit((STATUS)*pSyscallParameters);
            break;
        case SyscallIdThreadExit:
            status = SyscallThreadExit((STATUS)*pSyscallParameters);
            break;
        case SyscallIdFileWrite:
            status = SyscallFileWrite(
                (UM_HANDLE)pSyscallParameters[0],
                (PVOID)pSyscallParameters[1],
                (QWORD)pSyscallParameters[2],
                (QWORD*)pSyscallParameters[3]
            );
            break;
        case SyscallIdThreadCreate:
            status = SyscallThreadCreate(
                (PFUNC_ThreadStart)pSyscallParameters[0],
                (PVOID)pSyscallParameters[1],
                (UM_HANDLE*)pSyscallParameters[2]
            );
            break;
        case SyscallIdThreadGetTid:
            status = SyscallThreadGetTid(
                (UM_HANDLE)pSyscallParameters[0],
                (TID*)pSyscallParameters[1]
            );
            break;
        case SyscallIdThreadWaitForTermination:
            status = SyscallThreadWaitForTermination(
                (UM_HANDLE)pSyscallParameters[0],
                (STATUS*)pSyscallParameters[1]
            );
            break;
        case SyscallIdThreadCloseHandle:
            status = SyscallThreadCloseHandle(
                (UM_HANDLE)*pSyscallParameters
            );
            break;

        default:
            LOG_ERROR("Unimplemented syscall called from User-space!\n");
            status = STATUS_UNSUPPORTED;
            break;
        }

    }
    __finally
    {
        LOG_TRACE_USERMODE("Will set UM RAX to 0x%x\n", status);

        usermodeProcessorState->RegisterValues[RegisterRax] = status;

        CpuIntrSetState(INTR_OFF);
    }
}

void
SyscallPreinitSystem(
    void
    )
{

}

STATUS
SyscallInitSystem(
    void
    )
{
    return STATUS_SUCCESS;
}

STATUS
SyscallUninitSystem(
    void
    )
{
    return STATUS_SUCCESS;
}

void
SyscallCpuInit(
    void
    )
{
    IA32_STAR_MSR_DATA starMsr;
    WORD kmCsSelector;
    WORD umCsSelector;

    memzero(&starMsr, sizeof(IA32_STAR_MSR_DATA));

    kmCsSelector = GdtMuGetCS64Supervisor();
    ASSERT(kmCsSelector + 0x8 == GdtMuGetDS64Supervisor());

    umCsSelector = GdtMuGetCS32Usermode();
    /// DS64 is the same as DS32
    ASSERT(umCsSelector + 0x8 == GdtMuGetDS32Usermode());
    ASSERT(umCsSelector + 0x10 == GdtMuGetCS64Usermode());

    // Syscall RIP <- IA32_LSTAR
    __writemsr(IA32_LSTAR, (QWORD) SyscallEntry);

    LOG_TRACE_USERMODE("Successfully set LSTAR to 0x%X\n", (QWORD) SyscallEntry);

    // Syscall RFLAGS <- RFLAGS & ~(IA32_FMASK)
    __writemsr(IA32_FMASK, RFLAGS_INTERRUPT_FLAG_BIT);

    LOG_TRACE_USERMODE("Successfully set FMASK to 0x%X\n", RFLAGS_INTERRUPT_FLAG_BIT);

    // Syscall CS.Sel <- IA32_STAR[47:32] & 0xFFFC
    // Syscall DS.Sel <- (IA32_STAR[47:32] + 0x8) & 0xFFFC
    starMsr.SyscallCsDs = kmCsSelector;

    // Sysret CS.Sel <- (IA32_STAR[63:48] + 0x10) & 0xFFFC
    // Sysret DS.Sel <- (IA32_STAR[63:48] + 0x8) & 0xFFFC
    starMsr.SysretCsDs = umCsSelector;

    __writemsr(IA32_STAR, starMsr.Raw);

    LOG_TRACE_USERMODE("Successfully set STAR to 0x%X\n", starMsr.Raw);
}

// SyscallIdIdentifyVersion
STATUS
SyscallValidateInterface(
    IN  SYSCALL_IF_VERSION          InterfaceVersion
)
{
    LOG_TRACE_USERMODE("Will check interface version 0x%x from UM against 0x%x from KM\n",
        InterfaceVersion, SYSCALL_IF_VERSION_KM);

    if (InterfaceVersion != SYSCALL_IF_VERSION_KM)
    {
        LOG_ERROR("Usermode interface 0x%x incompatible with KM!\n", InterfaceVersion);
        return STATUS_INCOMPATIBLE_INTERFACE;
    }

    return STATUS_SUCCESS;
}

// STUDENT TODO: implement the rest of the syscalls

STATUS
SyscallProcessExit(
    IN      STATUS                  ExitStatus
)
{
    PPROCESS Process;
    Process = GetCurrentProcess();
    Process->TerminationStatus = ExitStatus;
    ProcessTerminate(Process);
    return STATUS_SUCCESS;
}

STATUS
SyscallThreadExit(
    IN  STATUS                      ExitStatus
)
{
    PPROCESS pProcess = GetCurrentProcess();
    INTR_STATE state1, state2;
    LockAcquire(&pProcess->UsermodeThreadListLock, &state1);
    LockAcquire(&pProcess->NumberOfUsermodeThreadsLock, &state2);
    if (&GetCurrentThread()->ProcessUsermodeThreadListElem != NULL) {

        //RemoveEntryList(&GetCurrentThread()->ProcessUsermodeThreadListElem);
        //pProcess->NumberOfUsermodeThreads--;
    }
    LockRelease(&pProcess->NumberOfUsermodeThreadsLock, state2);
    LockRelease(&pProcess->UsermodeThreadListLock, state1);

    ThreadExit(ExitStatus);
    return STATUS_SUCCESS;
}

STATUS
SyscallFileWrite(
    IN  UM_HANDLE                   FileHandle,
    IN_READS_BYTES(BytesToWrite)
    PVOID                       Buffer,
    IN  QWORD                       BytesToWrite,
    OUT QWORD * BytesWritten
)
{
    if (BytesWritten == NULL) {
        return STATUS_UNSUCCESSFUL;    
    }
    
        if (FileHandle == UM_FILE_HANDLE_STDOUT) {
        *BytesWritten = BytesToWrite;
        LOG("[%s]:[%s]\n", ProcessGetName(NULL), Buffer);
        return STATUS_SUCCESS;
    }
    
    *BytesWritten = BytesToWrite;
    return STATUS_SUCCESS;
}

STATUS
SyscallThreadCreate(
    IN      PFUNC_ThreadStart       StartFunction,
    IN_OPT  PVOID                   Context,
    OUT     UM_HANDLE* ThreadHandle
) {
    PPROCESS pProcess = GetCurrentProcess();

    if (STATUS_SUCCESS != MmuIsBufferValid((PVOID)StartFunction, sizeof(PFUNC_ThreadStart), PAGE_RIGHTS_READ, pProcess)) {

        return STATUS_UNSUCCESSFUL;
    }
    if (Context != 0 && STATUS_SUCCESS != MmuIsBufferValid((PVOID)Context, sizeof(PVOID), PAGE_RIGHTS_READ, pProcess)) {

        return STATUS_UNSUCCESSFUL;
    }

    if (ThreadHandle != 0 && STATUS_SUCCESS != MmuIsBufferValid((PVOID)ThreadHandle, sizeof(PVOID), PAGE_RIGHTS_WRITE, pProcess)) {

        return STATUS_UNSUCCESSFUL;
    }

    PTHREAD pThread = NULL;

    INTR_STATE state1, state3;
    LockAcquire(&pProcess->NumberOfUsermodeThreadsLock, &state1);
    char threadName[1000];
    sprintf(threadName, "usermodeThread%d", ++pProcess->NumberOfUsermodeThreads);
    STATUS status = ThreadCreateEx(threadName, ThreadPriorityDefault, StartFunction, Context, &pThread, GetCurrentProcess());

    if (!SUCCEEDED(status)) {
        return STATUS_UNSUCCESSFUL;
    }

    LockAcquire(&pProcess->UsermodeThreadListLock, &state3);
    InsertTailList(&pProcess->HeadUsermodeThreadList, &pThread->ProcessUsermodeThreadListElem);

    LockRelease(&pProcess->UsermodeThreadListLock, state3);
    LockRelease(&pProcess->NumberOfUsermodeThreadsLock, state1);

    LockAcquire(&pThread->HandleValueLock, &state3);
    pThread->HandleValue = pProcess->NumberOfUsermodeThreads;
    *ThreadHandle = pThread->HandleValue;
    LockRelease(&pThread->HandleValueLock, state3);

    return STATUS_SUCCESS;
}

STATUS
SyscallThreadGetTid(
    IN_OPT  UM_HANDLE               ThreadHandle,
    OUT     TID* ThreadId
) {
    PPROCESS pProcess = GetCurrentProcess();

    if (ThreadHandle == UM_INVALID_HANDLE_VALUE) {
        PTHREAD cThread = GetCurrentThread();
        if (cThread == NULL) {
            return STATUS_UNSUCCESSFUL;
        }
        *ThreadId = cThread->Id;
        return STATUS_SUCCESS;
    }
    INTR_STATE state;
    LockAcquire(&pProcess->UsermodeThreadListLock, &state);
    LIST_ITERATOR it;
    ListIteratorInit(&pProcess->HeadUsermodeThreadList, &it);

    PLIST_ENTRY pEntry;
    while ((pEntry = ListIteratorNext(&it)) != NULL)
    {
        //for (PLIST_ENTRY pEntry = &pProcess->HeadUsermodeThreadList.Flink; pEntry != &pProcess->HeadUsermodeThreadList; pEntry = pEntry->Flink) {

        PTHREAD pThread = CONTAINING_RECORD(pEntry, THREAD, ProcessUsermodeThreadListElem);
        if (pThread->HandleValue == ThreadHandle) {

            *ThreadId = pThread->Id;
            LockRelease(&pProcess->UsermodeThreadListLock, state);
            return STATUS_SUCCESS;
        }
    }
    LockRelease(&pProcess->UsermodeThreadListLock, state);

    return STATUS_UNSUCCESSFUL;
}

STATUS
SyscallThreadWaitForTermination(
    IN      UM_HANDLE               ThreadHandle,
    OUT     STATUS* TerminationStatus
) {

    if (TerminationStatus == NULL || ThreadHandle == UM_INVALID_HANDLE_VALUE) {
        *TerminationStatus = STATUS_UNSUCCESSFUL;
        return STATUS_SUCCESS;
    }

    INTR_STATE state;
    PPROCESS pProcess = GetCurrentProcess();
    LockAcquire(&pProcess->UsermodeThreadListLock, &state);
    LIST_ITERATOR it;
    ListIteratorInit(&pProcess->HeadUsermodeThreadList, &it);

    PLIST_ENTRY pEntry;
    while ((pEntry = ListIteratorNext(&it)) != NULL)
    {
        PTHREAD pThread = CONTAINING_RECORD(pEntry, THREAD, ProcessUsermodeThreadListElem);
        if (pThread->HandleValue == ThreadHandle) {

            ThreadWaitForTermination(pThread, TerminationStatus);
            LockRelease(&pProcess->UsermodeThreadListLock, state);
            return STATUS_SUCCESS;
        }
    }
    LockRelease(&pProcess->UsermodeThreadListLock, state);

    return STATUS_UNSUCCESSFUL;
}

STATUS
SyscallThreadCloseHandle(
    IN      UM_HANDLE               ThreadHandle
) {
    UNREFERENCED_PARAMETER(ThreadHandle);
    INTR_STATE state;
    PPROCESS pProcess = GetCurrentProcess();
    LockAcquire(&pProcess->UsermodeThreadListLock, &state);
    LIST_ITERATOR it;
    ListIteratorInit(&pProcess->HeadUsermodeThreadList, &it);

    PLIST_ENTRY pEntry;
    while ((pEntry = ListIteratorNext(&it)) != NULL)
    {
        PTHREAD pThread = CONTAINING_RECORD(pEntry, THREAD, ProcessUsermodeThreadListElem);
        if (pThread->HandleValue == ThreadHandle) {
            //RemoveEntryList(&pThread->ProcessUsermodeThreadListElem);
            pThread->HandleValue = UM_INVALID_HANDLE_VALUE;
            ThreadCloseHandle(pThread);
            LockRelease(&pProcess->UsermodeThreadListLock, state);

            return STATUS_SUCCESS;
        }
    }
    LockRelease(&pProcess->UsermodeThreadListLock, state);

    return STATUS_UNSUCCESSFUL;
}

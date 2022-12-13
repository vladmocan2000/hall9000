#include "common_lib.h"
#include "syscall_if.h"
#include "um_lib_helper.h"

STATUS
__main(
    DWORD       argc,
    char**      argv
)
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    char endline = '\n';
    char space = ' ';
    QWORD bytesWritten;
    SyscallFileWrite(UM_FILE_HANDLE_STDOUT, (PVOID)(&endline), 1, &bytesWritten);//print  endline
    SyscallFileWrite(UM_FILE_HANDLE_STDOUT, (PVOID)(&endline), 1, &bytesWritten);//print  endline

    //print tid (we need to convert it into string using sprintf())
    TID tid;
    SyscallThreadGetTid(HANDLE_CURRENT_THREAD, &tid);
    char tid_s[1000];
    sprintf(tid_s, "%d", (int)tid);
    SyscallFileWrite(UM_FILE_HANDLE_STDOUT, (PVOID)tid_s, strlen(tid_s), &bytesWritten);

    SyscallFileWrite(UM_FILE_HANDLE_STDOUT, (PVOID)(&space), 1, &bytesWritten);//print  space

    //print name
    char name[10];
    SyscallThreadGetName(name, 10);
    SyscallFileWrite(UM_FILE_HANDLE_STDOUT, (PVOID)name, strlen(name), &bytesWritten);

    SyscallFileWrite(UM_FILE_HANDLE_STDOUT, (PVOID)(&endline), 1, &bytesWritten);//print  endline

    //9.1
    for (int i = 1; i <= (int)strlen(name) + 1; i++) {

        char trunc_name[100];
        SyscallThreadGetName(trunc_name, i);
        SyscallFileWrite(UM_FILE_HANDLE_STDOUT, (PVOID)trunc_name, strlen(trunc_name), &bytesWritten);
        SyscallFileWrite(UM_FILE_HANDLE_STDOUT, (PVOID)(&endline), 1, &bytesWritten);//print  endline
    }

    //9.2
    QWORD threads_no;
    SyscallGetTotalThreadNo(&threads_no);
    char threads_no_s[1000];
    sprintf(threads_no_s, "%d", (int)threads_no);
    SyscallFileWrite(UM_FILE_HANDLE_STDOUT, (PVOID)threads_no_s, strlen(threads_no_s), &bytesWritten);

    //9.3
    
    /*PVOID stackStartAddress;
    SyscallGetThreadUmStackAddress(&stackStartAddress);
    char stackStartAddress_s[1000];
    sprintf(stackStartAddress_s, "%lld", stackStartAddress);
    SyscallFileWrite(UM_FILE_HANDLE_STDOUT, (PVOID)stackStartAddress_s, strlen(stackStartAddress_s), &bytesWritten);
    SyscallFileWrite(UM_FILE_HANDLE_STDOUT, (PVOID)(&endline), 1, &bytesWritten);//print  endline


    QWORD stackStartSize;
    SyscallGetThreadUmStackSize(&stackStartSize);
    char stackStartSize_s[1000];
    sprintf(stackStartSize_s, "%d", (int)stackStartSize);
    SyscallFileWrite(UM_FILE_HANDLE_STDOUT, (PVOID)stackStartSize_s, strlen(stackStartSize_s), &bytesWritten);
    SyscallFileWrite(UM_FILE_HANDLE_STDOUT, (PVOID)(&endline), 1, &bytesWritten);//print  endline*/
    

    SyscallFileWrite(UM_FILE_HANDLE_STDOUT, (PVOID)(&endline), 1, &bytesWritten);//print  endline
    SyscallFileWrite(UM_FILE_HANDLE_STDOUT, (PVOID)(&endline), 1, &bytesWritten);//print  endline
    return STATUS_SUCCESS;
}
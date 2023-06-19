/*
 * fork.c
 * Experimental fork() on Windows.  Requires NT 6 subsystem or
 * newer.
 *
 * Copyright (c) 2012 William Pitcock <nenolod@dereferenced.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * This software is provided 'as is' and without any warranty, express or
 * implied.  In no event shall the authors be liable for any damages arising
 * from the use of this software.
 */

#include "../lib/windoze.h"

#if WINDOWS_NATIVE
#define _WIN32_WINNT 0x0600
#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <errno.h>
#include <process.h>
#include <windows.h>

typedef struct _CLIENT_ID {
  PVOID UniqueProcess;
  PVOID UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef struct _SECTION_IMAGE_INFORMATION {
  PVOID EntryPoint;
  ULONG StackZeroBits;
  ULONG StackReserved;
  ULONG StackCommit;
  ULONG ImageSubsystem;
  WORD SubSystemVersionLow;
  WORD SubSystemVersionHigh;
  ULONG Unknown1;
  ULONG ImageCharacteristics;
  ULONG ImageMachineType;
  ULONG Unknown2[3];
} SECTION_IMAGE_INFORMATION, *PSECTION_IMAGE_INFORMATION;

typedef struct _RTL_USER_PROCESS_INFORMATION {
  ULONG Size;
  HANDLE Process;
  HANDLE Thread;
  CLIENT_ID ClientId;
  SECTION_IMAGE_INFORMATION ImageInformation;
} RTL_USER_PROCESS_INFORMATION, *PRTL_USER_PROCESS_INFORMATION;

#define RTL_CLONE_PROCESS_FLAGS_CREATE_SUSPENDED 0x00000001
#define RTL_CLONE_PROCESS_FLAGS_INHERIT_HANDLES 0x00000002
#define RTL_CLONE_PROCESS_FLAGS_NO_SYNCHRONIZE 0x00000004

#define RTL_CLONE_PARENT 0
#define RTL_CLONE_CHILD 297

typedef long (*RtlCloneUserProcess_f)(
    ULONG ProcessFlags,
    PSECURITY_DESCRIPTOR ProcessSecurityDescriptor /* optional */,
    PSECURITY_DESCRIPTOR ThreadSecurityDescriptor /* optional */,
    HANDLE DebugPort /* optional */,
    PRTL_USER_PROCESS_INFORMATION ProcessInformation);
typedef int get_process_id_function(HANDLE);

#ifndef ENOSYS
#define ENOSYS 88
#endif

int
fork(void) {
  HMODULE mod;
  RtlCloneUserProcess_f clone_p;
  RTL_USER_PROCESS_INFORMATION process_info;
  long result;

  mod = GetModuleHandle("ntdll.dll");
  if(!mod)
    return -ENOSYS;

  clone_p = GetProcAddress(mod, "RtlCloneUserProcess");
  if(clone_p == NULL)
    return -ENOSYS;

  /* lets do this */
  result = clone_p(RTL_CLONE_PROCESS_FLAGS_CREATE_SUSPENDED |
                       RTL_CLONE_PROCESS_FLAGS_INHERIT_HANDLES,
                   NULL,
                   NULL,
                   NULL,
                   &process_info);

  if(result == RTL_CLONE_PARENT) {
    HANDLE me = GetCurrentProcess();
    HANDLE kern32;
    get_process_id_function* get_process_id;
    int child_pid;

    if((kern32 = GetModuleHandle("kernel32.dll")) == NULL)
      return -ENOSYS;
    if((get_process_id = GetProcAddress(kern32, "GetProcessId")) == NULL)
      return -ENOSYS;

    child_pid = get_process_id(process_info.Process);

    ResumeThread(process_info.Thread);
    CloseHandle(process_info.Process);
    CloseHandle(process_info.Thread);

    return child_pid;
  } else if(result == RTL_CLONE_CHILD) {
    /* fix stdio */
    AllocConsole();
    return 0;
  } else
    return -1;

  /* NOTREACHED */
  return -1;
}

#endif

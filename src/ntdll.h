#pragma once

#include <winternl.h>

using SECTION_INHERIT = int;

NTSTATUS NTAPI NtMapViewOfSection
(
    _In_        HANDLE           SectionHandle,
    _In_        HANDLE           ProcessHandle,
    _Inout_     PVOID           *BaseAddress,
    _In_        ULONG_PTR        ZeroBits,
    _In_        SIZE_T           CommitSize,
    _Inout_opt_ PLARGE_INTEGER   SectionOffset,
    _Inout_     PSIZE_T          ViewSize,
    _In_        SECTION_INHERIT  InheritDisposition,
    _In_        ULONG            AllocationType,
    _In_        ULONG            Win32Protect
);
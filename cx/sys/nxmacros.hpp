/*++

Copyright (C) Microsoft Corporation. All rights reserved.

Module Name:

    Nxmacros.hpp





    
Environment:

    kernel mode only

Revision History:

--*/

#pragma once

#define NETEXPORT(a) imp_ ## a
#define NETADAPTERCX_TAG 'xCdN'
#define NETADAPTERCX_TAG_PTR ((PVOID)(PULONG_PTR) NDISCX_TAG)

FORCEINLINE
VOID
SetCompletionRoutineSmart(
    _In_     PDEVICE_OBJECT         DeviceObject,
    _In_     PIRP                   Irp,
    _In_     PIO_COMPLETION_ROUTINE CompletionRoutine,
    _In_opt_ PVOID                  Context,
    _In_     BOOLEAN                InvokeOnSuccess,
    _In_     BOOLEAN                InvokeOnError,
    _In_     BOOLEAN                InvokeOnCancel
    )
/*++ 
 
Routine Description: 
    This routine first calls the IoSetCompletionRoutineEx to set the completion
    routine on the Irp, and if it fails then it calls the old
    IoSetCompletionRoutine.
 
    Using IoSetCompletionRoutine can result in a rare issue where
    the driver might get unloaded prior to the IoSetCompletionRoutine returns.
 
    Leveraging IoSetCompletionRoutineEx first and if it fails using
    IoSetCompletionRoutine shrinks the window in which that issue can happen
    to negligible. This is a common practice used across several other inbox
    drivers.
--*/
{
    if (!NT_SUCCESS(IoSetCompletionRoutineEx(DeviceObject, 
                                             Irp, 
                                             CompletionRoutine, 
                                             Context, 
                                             InvokeOnSuccess, 
                                             InvokeOnError, 
                                             InvokeOnCancel))) {
        IoSetCompletionRoutine(Irp,
                               CompletionRoutine,
                               Context, 
                               InvokeOnSuccess, 
                               InvokeOnError,
                               InvokeOnCancel);
    }
}

/*++
Macro Description:
    This method loops through each entry in a doubly linked list (LIST_ENTRY).
    It assumes that the doubly linked list has a dedicated list head
 
Arguments: 
    Type - The type of each entry of the linked list.
 
    Head - A Pointer to the list head
 
    Field - The name of LIST_ENTRY filed in the stucture.
 
    Current - A pointer to the current entry (to be used in the body of the
        FOR_ALL_IN_LIST
 
Usage: 
 
    typedef struct _MYENTRY {
        ULONG Version;
        ULONG SubVersion;
        LIST_ENTRY Link;
        UCHAR Data;
    } MYENTRY, *PMYENTRY;
 
    typedef struct _MYCONTEXT {
        ULONG Size;
        LIST_ENTRY MyEntryListHead;
        ...
    } *PMYCONTEXT;
 
    PMYENTRY FindMyEntry(PMYCONTEXT Context,
                         UCHAR Data) {
        PMYENTRY entry;
 
        FOR_ALL_IN_LIST(MYENTRY,
                        &Context->MyEntryListHead,
                        Link,
                        entry) {
            if (entry->Data == Data) { return entry }
    }
 
Remarks : 
    While using the FOR_ALL_IN_LIST, you must not change the
    structure of the list. In case you want to remove the current element
    and continue interating the list, use FOR_ALL_IN_LIST_SAFE
--*/
#define FOR_ALL_IN_LIST(Type, Head, Field, Current)                 \
    for((Current) = CONTAINING_RECORD((Head)->Flink, Type, Field);  \
       (Head) != &(Current)->Field;                                 \
       (Current) = CONTAINING_RECORD((Current)->Field.Flink,        \
                                     Type,                          \
                                     Field)                         \
       )

/*++
Macro Description:
    This method loops through each entry in a doubly linked list (LIST_ENTRY).
    It assumes that the doubly linked list has a dedicated list head.
    In each iteration of the loop it is safe to remove the current element from
    the list. 
 
Arguments: 
    Type - The type of each entry of the linked list.
 
    Head - A Pointer to the list head
 
    Field - The name of LIST_ENTRY filed in the stucture.
 
    Current - A pointer to the current entry (to be used in the body of the
        FOR_ALL_IN_LIST
 
    Next - A pointer to the next entry in the list, that the user must not touch
 
Usage: 
 
    typedef struct _MYENTRY {
        ULONG Version;
        ULONG SubVersion;
        LIST_ENTRY Link;
        UCHAR Data;
    } MYENTRY, *PMYENTRY;
 
    typedef struct _MYCONTEXT {
        ULONG Size;
        LIST_ENTRY MyEntryListHead;
        ...
    } *PMYCONTEXT;
 
    VOID DeleteEntries(PMYCONTEXT Context,
                           UCHAR Data) {
        PMYENTRY entry, nextEntry;
 
        FOR_ALL_IN_LIST_SAFE(MYENTRY,
                            &Context->MyEntryListHead,
                            Link,
                            entry,
                            nextEntry) {
            if (entry->Data == Data) {
                RemoveEntryList(&entry->Link);
                ExFreePool(entry);
            }
    }
 --*/
#define FOR_ALL_IN_LIST_SAFE(Type, Head, Field, Current, Next)          \
    for((Current) = CONTAINING_RECORD((Head)->Flink, Type, Field),      \
            (Next) = CONTAINING_RECORD((Current)->Field.Flink,          \
                                       Type, Field);                    \
       (Head) != &(Current)->Field;                                     \
       (Current) = (Next),                                              \
            (Next) = CONTAINING_RECORD((Current)->Field.Flink,          \
                                     Type, Field)                       \
       )

FORCEINLINE
VOID
InitializeListEntry(
    _Out_ PLIST_ENTRY ListEntry
    )
/*++

Routine Description:

    Initialize a list entry to NULL.

    - Using this improves catching list manipulation errors
    - This should not be called on a list head
    - Callers may depend on use of NULL value

--*/
{
    ListEntry->Flink = ListEntry->Blink = NULL;
}

FORCEINLINE
LONG
NxInterlockedIncrementFloor(
    __inout LONG  volatile *Target,
    __in LONG Floor
    )
{
    LONG startVal;
    LONG currentVal;

    currentVal = *Target;

    do {
        if (currentVal <= Floor) {
            return currentVal;
        }

        startVal = currentVal;

        //
        // currentVal will be the value that used to be Target if the exchange was made
        // or its current value if the exchange was not made.
        //
        currentVal = InterlockedCompareExchange(Target, startVal+1, startVal);

        //
        // If startVal == currentVal, then no one updated Target in between the deref at the top
        // and the InterlockedCompareExchange afterward.
        //
    } while (startVal != currentVal);

    //
    // startVal is the old value of Target. Since InterlockedIncrement returns the new
    // incremented value of Target, we should do the same here.
    //
    return startVal+1;
}


FORCEINLINE
LONG
NxInterlockedDecrementFloor(
    __inout LONG  volatile *Target,
    __in LONG Floor
    )
{
    LONG startVal;
    LONG currentVal;

    currentVal = *Target;

    do {
        if (currentVal <= Floor) {
            //
            // This value cannot be returned in the success path
            //
            return Floor-1;
        }

        startVal = currentVal;

        //
        // currentVal will be the value that used to be Target if the exchange was made
        // or its current value if the exchange was not made.
        //
        currentVal = InterlockedCompareExchange(Target, startVal -1, startVal);

        //
        // If startVal == currentVal, then no one updated Target in between the deref at the top
        // and the InterlockedCompareExchange afterward.
        //
    } while (startVal != currentVal);

    //
    // startVal is the old value of Target. Since InterlockedDecrement returns the new
    // decremented value of Target, we should do the same here.
    //
    return startVal -1;
}

FORCEINLINE
LONG
NxInterlockedIncrementGTZero(
    __inout LONG  volatile *Target
    )
{
    return NxInterlockedIncrementFloor(Target, 0);
}

class DispatchLock {
private: 
    volatile LONG m_Count;
    KEVENT        m_Event;

    //
    // For performance reasons this lock may not enabled. 
    // In that case the member of this Lock just fake success
    //
    BOOLEAN       m_Enabled;

public:
    DispatchLock(
        BOOLEAN Enabled
        ):
        m_Count(0),
        m_Enabled(Enabled)
    {
        if (!Enabled) { return; }
        KeInitializeEvent(&m_Event, NotificationEvent, TRUE);
    }

    VOID
    InitAndAcquire(
        VOID
        ) {
        if (!m_Enabled) { return; }
        NT_ASSERT(m_Count == 0);
        m_Count = 1;
        KeClearEvent(&m_Event);
    }

    BOOLEAN 
    Acquire(
        VOID
        ) {
        if (!m_Enabled) { return TRUE; }
        return (NxInterlockedIncrementGTZero(&m_Count) != 0);
    }

    VOID
    Release(
        VOID
        ) {
        if (!m_Enabled) { return; }
        if (0 == InterlockedDecrement(&m_Count)) {
            KeSetEvent(&m_Event, IO_NO_INCREMENT, FALSE);
        }
    }

    VOID
    ReleaseAndWait(
        VOID
        ){
        if (!m_Enabled) { return; }
        Release();
        KeWaitForSingleObject(&m_Event, Executive, KernelMode, FALSE, NULL);
    }

};

#define WHILE(a) \
__pragma(warning(suppress:4127)) while(a)

#define POINTER_WITH_HIDDEN_BITS_MASK 0x7
class PointerWithHiddenBits {

public:

    static
    PVOID
    _GetPtr(
        PVOID Ptr
        ) {
        return (PVOID)(((ULONG_PTR)Ptr) & ~POINTER_WITH_HIDDEN_BITS_MASK);
    }

    static
    VOID
    _SetBit0(
        PVOID* Ptr
        )
    {
        ULONG_PTR ptrVal = (ULONG_PTR)*Ptr;
        ptrVal |= 0x1;        
        *Ptr = (PVOID)(ptrVal);
    }

    static
    BOOLEAN
    _IsBit0Set(
        PVOID Ptr
        )
    {
        ULONG_PTR ptrVal;
        ptrVal = (ULONG_PTR)Ptr;
        ptrVal &= 0x1;
        return (ptrVal != 0);
    }

};

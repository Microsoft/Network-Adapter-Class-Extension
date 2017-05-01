/*++

Copyright (C) Microsoft Corporation. All rights reserved.

Module Name:

    NxDriver.hpp

Abstract:

    This is the definition of the NxDriver object.





Environment:

    kernel mode only

Revision History:

--*/

#pragma once

//
// The NxDriver is an object that represents a NetAdapterCx Client Driver
//

PNxDriver
FORCEINLINE
GetNxDriverFromWdfDriver(
    _In_ WDFDRIVER Driver
    );

typedef class NxDriver *PNxDriver;
class NxDriver : public CFxObject<WDFDRIVER,
                                  NxDriver,
                                  GetNxDriverFromWdfDriver,
                                  false> 
{
//friend class NxAdapter;

private:
    WDFDRIVER    m_Driver;
    RECORDER_LOG m_RecorderLog;
    NDIS_HANDLE  m_NdisMiniportDriverHandle;

    NxDriver(
        _In_ WDFDRIVER                Driver,
        _In_ PNX_PRIVATE_GLOBALS      NxPrivateGlobals
        );

public:
    static
    NTSTATUS
    _CreateAndRegisterIfNeeded(
        _In_ WDFDRIVER                      Driver,
        _In_ NET_ADAPTER_DRIVER_TYPE        DriverType,
        _In_ PNX_PRIVATE_GLOBALS            NxPrivateGlobals
        );

    static
    NTSTATUS
    _CreateIfNeeded(
        _In_ WDFDRIVER           Driver,
        _In_ PNX_PRIVATE_GLOBALS NxPrivateGlobals
        );

    NTSTATUS
    Register(
        _In_ NET_ADAPTER_DRIVER_TYPE        DriverType
        );

    static 
    NDIS_STATUS
    _EvtNdisSetOptions(
        _In_  NDIS_HANDLE  NdisDriverHandle,
        _In_  NDIS_HANDLE  NxDriverAsContext
        );

    static
    VOID
    _EvtWdfCleanup(
        _In_  WDFOBJECT Driver
    );

    RECORDER_LOG
    GetRecorderLog() { 
        return m_RecorderLog;
    }

    NDIS_HANDLE
    GetNdisMiniportDriverHandle() {
        return m_NdisMiniportDriverHandle;
    }

    ~NxDriver();

};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(NxDriver, _GetNxDriverFromWdfDriver);

PNxDriver
FORCEINLINE
GetNxDriverFromWdfDriver(
    _In_ WDFDRIVER Driver
    )
/*++
Routine Description:

    This routine is just a wrapper around the _GetNxDriverFromWdfDriver function.
    To be able to define a the NxDriver class above, we need a forward declaration of the
    accessor function. Since _GetNxDriverFromWdfDriver is defined by Wdf, we dont want to
    assume a prototype of that function for the foward declaration. 

--*/

{
    return _GetNxDriverFromWdfDriver(Driver);
}

/*++

Copyright (C) Microsoft Corporation. All rights reserved.

Module Name:

    NxConfiguration.hpp

Abstract:

    This is the definition of the NxAdapter Configuration object.





Environment:

    kernel mode only

Revision History:

--*/

#pragma once

//
// The NxConfiguration is an object that represents a Net Configuration
//

PNxConfiguration
FORCEINLINE
GetNxConfigurationFromHandle(
    _In_ NETCONFIGURATION Configuration
    );

typedef class NxConfiguration *PNxConfiguration;
class NxConfiguration : public CFxObject<NETCONFIGURATION,
                                   NxConfiguration,
                                   GetNxConfigurationFromHandle,
                                   false> 
{

private:
    PNxConfiguration             m_ParentNxConfiguration;

public: 

    PNxAdapter                   m_NxAdapter;

    //
    // Opaque handle returned by ndis.sys for this adapter.
    //
    NDIS_HANDLE                  m_NdisConfigurationHandle;

private:
    NxConfiguration(
        _In_ PNX_PRIVATE_GLOBALS      NxPrivateGlobals,
        _In_ NETCONFIGURATION         Configuration,
        _In_ PNxConfiguration         ParentNxConfiguration,
        _In_ PNxAdapter               NxAdapter
        );

public:

    ~NxConfiguration();

    static
    NTSTATUS
    _Create(
        _In_     PNX_PRIVATE_GLOBALS      PrivateGlobals,
        _In_     PNxAdapter               NxAdapter,
        _In_opt_ PNxConfiguration         ParentNxConfiguration,
        _Out_    PNxConfiguration*        NxConfiguration
        );

    static
    VOID
    _EvtCleanup(
        _In_  WDFOBJECT Configuration
        );

    NTSTATUS
    Open(
        VOID
        );

    NTSTATUS
    OpenAsSubConfiguration(
        PCUNICODE_STRING  SubConfigurationName
        );

    VOID
    DeleteFromFailedOpen(
        VOID
        );

    VOID
    Close(
        VOID
        );

    NTSTATUS
    AddAttributes(
        _In_ PWDF_OBJECT_ATTRIBUTES Attributes
        );

    RECORDER_LOG
    GetRecorderLog(
        VOID
        );

    _Must_inspect_result_
    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS
    QueryUlong(
        _In_  NET_CONFIGURATION_QUERY_ULONG_FLAGS   Flags,
        _In_  PCUNICODE_STRING                      ValueName,
        _Out_ PULONG                                Value
        );

    _Must_inspect_result_
    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS
    QueryString(
        _In_     PCUNICODE_STRING                      ValueName,
        _In_opt_ PWDF_OBJECT_ATTRIBUTES                StringAttributes,
        _Out_    WDFSTRING*                            WdfString
        );

    _Must_inspect_result_
    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS
    QueryMultiString(
        _In_     PCUNICODE_STRING                      ValueName,
        _In_opt_ PWDF_OBJECT_ATTRIBUTES                StringsAttributes,
        _In_     WDFCOLLECTION                         Collection
        );

    _Must_inspect_result_
    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS
    QueryBinary(
        _In_     PCUNICODE_STRING                      ValueName,
        _Strict_type_match_ _In_
                 POOL_TYPE                             PoolType,
        _In_opt_ PWDF_OBJECT_ATTRIBUTES                MemoryAttributes,
        _Out_    WDFMEMORY*                            WdfMemory
    );

    _Must_inspect_result_
    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS
    QueryNetworkAddress(
        _In_     ULONG                                 BufferLength,
        _Out_writes_bytes_to_(BufferLength,*ResultLength)
                 PVOID                                 NetworkAddressBuffer,
        _Out_    PULONG                                ResultLength
        );

    _Must_inspect_result_
    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS
    AssignUlong(
        _In_  PCUNICODE_STRING                      ValueName,
        _In_  ULONG                                 Value
    );

    _Must_inspect_result_
    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS
    AssignUnicodeString(
        _In_  PCUNICODE_STRING                      ValueName,
        _In_  PCUNICODE_STRING                      Value
        );

    _Must_inspect_result_
    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS
    AssignBinary(
        _In_                                PCUNICODE_STRING    ValueName,
        _In_reads_bytes_(BufferLength)      PVOID               Buffer,
        _In_                                ULONG               BufferLength
        );

    _Must_inspect_result_
    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS
    AssignMultiString(
        _In_  PCUNICODE_STRING                      ValueName,
        _In_  WDFCOLLECTION                         StringsCollection
        );

};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(NxConfiguration, _GetNxConfigurationFromHandle);

PNxConfiguration
FORCEINLINE
GetNxConfigurationFromHandle(
    _In_ NETCONFIGURATION           Configuration
    )
/*++
Routine Description:

    This routine is just a wrapper around the _GetNxConfigurationFromHandle function.
    To be able to define a the NxConfiguration class above, we need a forward declaration of the
    accessor function. Since _GetNxConfigurationFromHandle is defined by Wdf, we dont want to
    assume a prototype of that function for the foward declaration. 

--*/

{
    return _GetNxConfigurationFromHandle(Configuration);
}

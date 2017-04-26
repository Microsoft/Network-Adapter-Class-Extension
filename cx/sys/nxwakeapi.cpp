/*++

Copyright (C) Microsoft Corporation. All rights reserved.

Module Name:

    NxWakeApi.cpp

Abstract:

    This module contains the "C" interface for the NxWake object.





Environment:

    kernel mode only

Revision History:

--*/

#include "Nx.hpp"

// Tracing support
extern "C" {
#include "NxWakeApi.tmh"
}

//
// extern the whole file
//
extern "C" {

WDFAPI
_IRQL_requires_(DISPATCH_LEVEL)
ULONG
NETEXPORT(NetPowerSettingsGetEnabledWakePatterns)(
    _In_     PNET_DRIVER_GLOBALS    Globals,
    _In_     NETPOWERSETTINGS       NetPowerSettings
    )
/*++
Routine Description:
    Obtain the EnabledWoLPacketPatterns associated with the adapater. This
    API must only be called during a power transition.

Arguments:
     NetPowerSettings - The NetPowerSettings object

Returns: 
    A bitmap flags represting which Wake patterns need
    to be enabled in the hardware for arming the device for wake.
    Refer to the documentation of NDIS_PM_PARAMETERS for more details

    This API must only be called during a power transition.
--*/
{
    FuncEntry(FLAG_POWER);

    PNxWake          nxWake;
    PNX_PRIVATE_GLOBALS pNxPrivateGlobals = GetPrivateGlobals(Globals);

    Verifier_VerifyPrivateGlobals(pNxPrivateGlobals);

    nxWake = GetNxWakeFromHandle(NetPowerSettings);

    Verifier_VerifyNetPowerSettingsAccessible(pNxPrivateGlobals, nxWake);

    FuncExit(FLAG_POWER);
    return nxWake->GetEnabledWakePacketPatterns();
}

WDFAPI
_IRQL_requires_(DISPATCH_LEVEL)
ULONG
NETEXPORT(NetPowerSettingsGetEnabledProtocolOffloads)(
    _In_     PNET_DRIVER_GLOBALS                         Globals,
    _In_     NETPOWERSETTINGS                             Adapter
    )
/*++
Routine Description:
    Returns a bitmap flag representing the enabled protocol offloads.

    Refer to the documentation of NDIS_PM_PARAMETERS EnabledProtocolOffloads
    for more details

Arguments:
    NetPowerSettings - The NetPowerSettings object
 
Returns: 
    A bitmap flag representing EnabledProtocolOffloads 
    Refer to the documentation of NDIS_PM_PARAMETERS for more details

    This API must only be called during a power transition.
--*/
{
    FuncEntry(FLAG_POWER);

    PNxWake nxWake;
    PNX_PRIVATE_GLOBALS pNxPrivateGlobals = GetPrivateGlobals(Globals);

    Verifier_VerifyPrivateGlobals(pNxPrivateGlobals);

    nxWake = GetNxWakeFromHandle(Adapter);





    Verifier_VerifyNetPowerSettingsAccessible(pNxPrivateGlobals, nxWake);

    FuncExit(FLAG_POWER);
    return  nxWake->GetEnabledProtocolOffloads();
}

WDFAPI
_IRQL_requires_(DISPATCH_LEVEL)
ULONG
NETEXPORT(NetPowerSettingsGetEnabledMediaSpecificWakeUpEvents)(
    _In_     PNET_DRIVER_GLOBALS    Globals,
    _In_     NETPOWERSETTINGS        NetPowerSettings
    )
/*++
Routine Description:
    Returns a ULONG value that contains a bitwise OR of flags. 
    These flags specify the media-specific wake-up events that a network adapter 
    supports. Refer to the documentation of NDIS_PM_PARAMETERS for more details

    This API must only be called during a power transition.
Arguments:
    NetPowerSettings - The NetPowerSettings object
 
Returns: 
    A bitmap flags represting which media-specific wake-up events need
    to be enabled in the hardware for arming the device for wake.

    Refer to the documentation of NDIS_PM_PARAMETERS for more details

--*/
{
    FuncEntry(FLAG_POWER);

    PNxWake nxWake;
    PNX_PRIVATE_GLOBALS pNxPrivateGlobals = GetPrivateGlobals(Globals);

    Verifier_VerifyPrivateGlobals(pNxPrivateGlobals);

    nxWake = GetNxWakeFromHandle(NetPowerSettings);
    Verifier_VerifyNetPowerSettingsAccessible(pNxPrivateGlobals, nxWake);

    FuncExit(FLAG_POWER);
    return  nxWake->GetEnabledMediaSpecificWakeUpEvents();
}

WDFAPI
_IRQL_requires_(DISPATCH_LEVEL)
ULONG
NETEXPORT(NetPowerSettingsGetEnabledWakeUpFlags)(
    _In_     PNET_DRIVER_GLOBALS    Globals,
    _In_     NETPOWERSETTINGS        NetPowerSettings
    )
/*++
Routine Description:
    Returns a ULONG value that contains a bitwise OR of NDIS_PM_WAKE_ON_ Xxx flags.
    This API must only be called during a power transition.

Arguments:
    NetPowerSettings - The NetPowerSettings object
 
Returns: 
    A bitmap flags represting the WakeUp flags need
    to be enabled in the hardware for arming the device for wake.
    Refer to the documentation of WakeUpFlags field of NDIS_PM_PARAMETERS
--*/
{
    FuncEntry(FLAG_POWER);
    PNxWake nxWake;
    PNX_PRIVATE_GLOBALS pNxPrivateGlobals = GetPrivateGlobals(Globals);

    Verifier_VerifyPrivateGlobals(pNxPrivateGlobals);

    nxWake = GetNxWakeFromHandle(NetPowerSettings);

    Verifier_VerifyNetPowerSettingsAccessible(pNxPrivateGlobals, nxWake);

    FuncExit(FLAG_POWER);
    return  nxWake->GetEnabledWakeUpFlags();
}

WDFAPI
_IRQL_requires_(DISPATCH_LEVEL)
PNDIS_PM_WOL_PATTERN
NETEXPORT(NetPowerSettingsGetWakePattern)(
    _In_     PNET_DRIVER_GLOBALS    Globals,
    _In_     NETPOWERSETTINGS       NetPowerSettings,
    _In_     ULONG                  Index
    )
/*++
Routine Description:
    Returns a PNDIS_PM_WOL_PATTERN structure at Index (0 based).

    This API must only be called during a power transition or from the 
    EvtPreviewWolPattern callback. In both cases, the driver should only access/examine 
    the  PNDIS_PM_WOL_PATTERN (obtained from this API) and should NOT cache or retain 
    a reference to the WoL pattern(s). This is because the Cx will automatically
    release it while handling WOL pattern removal.

Arguments:
    NetPowerSettings - The NetPowerSettings object
    Index - 0 based index. This value must be < NetPowerSettingsGetWoLPatternCount

Returns: 
    PNDIS_PM_WOL_PATTERN structure at Index. Returns NULL if Index is invalid.

--*/
{
    FuncEntry(FLAG_POWER);
    PNxWake nxWake;
    PNX_PRIVATE_GLOBALS pNxPrivateGlobals;
    PNX_NET_POWER_ENTRY nxWakePatternEntry;

    pNxPrivateGlobals = GetPrivateGlobals(Globals);

    Verifier_VerifyPrivateGlobals(pNxPrivateGlobals);

    nxWake = GetNxWakeFromHandle(NetPowerSettings);
    Verifier_VerifyNetPowerSettingsAccessible(pNxPrivateGlobals, nxWake);

    nxWakePatternEntry = nxWake->GetEntryAtIndex(Index, 
                                                NxPowerEntryTypeWakePattern);
    Verifier_VerifyNotNull(pNxPrivateGlobals, nxWakePatternEntry);

    FuncExit(FLAG_POWER);
    return nxWakePatternEntry ? &(nxWakePatternEntry->NdisWoLPattern) : NULL;
}

WDFAPI
_IRQL_requires_(DISPATCH_LEVEL)
ULONG
NETEXPORT(NetPowerSettingsGetWakePatternCount)(
    _In_     PNET_DRIVER_GLOBALS    Globals,
    _In_     NETPOWERSETTINGS       NetPowerSettings
    )
/*++
Routine Description:
    Returns the number of WoL Patterns stored in the NETPOWERSETTINGS object.

    IMPORTANT: This includes both, wake patterns that are enabled and 
    disabled. The driver can use the  NetPowerSettingsIsWakePatternEnabled 
    API to check if a particular wake pattern is enabled.

    This API must only be called during a power transition or from the 
    EvtPreviewWolPattern callback.

Arguments:
    NetPowerSettings - The NetPowerSettings object
 
Returns: 
    Returns the number of WoL Patterns stored in the NETPOWERSETTINGS object.
--*/
{
    FuncEntry(FLAG_POWER);
    PNxWake nxWake;
    PNX_PRIVATE_GLOBALS pNxPrivateGlobals = GetPrivateGlobals(Globals);

    Verifier_VerifyPrivateGlobals(pNxPrivateGlobals);

    nxWake = GetNxWakeFromHandle(NetPowerSettings);

    Verifier_VerifyNetPowerSettingsAccessible(pNxPrivateGlobals, nxWake);

    FuncExit(FLAG_POWER);
    return nxWake->GetWakePatternCount();
}

WDFAPI
_IRQL_requires_(DISPATCH_LEVEL)
BOOLEAN
NETEXPORT(NetPowerSettingsIsWakePatternEnabled)(
    _In_     PNET_DRIVER_GLOBALS    Globals,
    _In_     NETPOWERSETTINGS       NetPowerSettings,
    _In_     PNDIS_PM_WOL_PATTERN   NdisPmWolPattern
    )
/*++
Routine Description:
    This API can be used to determine if the PNDIS_PM_WOL_PATTERN obtained from
    a prior call to NetPowerSettingsGetWoLPattern is enabled. If it is enabled 
    the driver must program its hardware to enable the wake pattern during a 
    power down transition.

    This API must only be called during a power transition or from the 
    EvtPreviewWolPattern callback.

Arguments:
    NetPowerSettings - The NetPowerSettings object
    NdisPmWolPattern - Pointer to NDIS_PM_WOL_PATTERN structure that must be 
                obtained by a prior call to NetPowerSettingsGetWoLPattern

Returns: 
    Returns TRUE if the WoL pattern has been enabled and driver must enable it
        in its hardware. Returns FALSE if the pattern is not enabled.
--*/
{
    FuncEntry(FLAG_POWER);
    PNxWake nxWake;
    PNX_PRIVATE_GLOBALS pNxPrivateGlobals = GetPrivateGlobals(Globals);
    PNX_NET_POWER_ENTRY nxWakeEntry;

    Verifier_VerifyPrivateGlobals(pNxPrivateGlobals);

    nxWake = GetNxWakeFromHandle(NetPowerSettings);

    Verifier_VerifyNetPowerSettingsAccessible(pNxPrivateGlobals, nxWake);

    nxWakeEntry = CONTAINING_RECORD(NdisPmWolPattern,
                                NX_NET_POWER_ENTRY,
                                NdisWoLPattern);
    FuncExit(FLAG_POWER);
    return nxWakeEntry->Enabled;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
ULONG
NETEXPORT(NetPowerSettingsGetWakePatternCountForType)(
    _In_ PNET_DRIVER_GLOBALS    DriverGlobals,
    _In_ NETPOWERSETTINGS       NetPowerSettings,
    _In_ NDIS_PM_WOL_PACKET     WakePatternType
    )
/*++
Routine Description:
    Returns the number of WoL Patterns stored in the NETPOWERSETTINGS object for 
    a particular Wol Pattern Type

    IMPORTANT: This includes both, wake patterns that are enabled and 
    disabled. The driver can use the  NetPowerSettingsIsWakePatternEnabled 
    API to check if a particular wake pattern is enabled.

    This API must only be called during a power transition or from the
    EvtPreviewWolPattern callback.

Arguments:
    NetPowerSettings - The NetPowerSettings object
    WakePatternType - The WakePatternType that needs to be looked up

Returns:
    Returns the number of WoL Patterns stored in the NETPOWERSETTINGS object for 
    the WakePatternType specified
--*/
{
    FuncEntry(FLAG_POWER);
    PNxWake nxWake;
    PNX_PRIVATE_GLOBALS pNxPrivateGlobals = GetPrivateGlobals(DriverGlobals);

    Verifier_VerifyPrivateGlobals(pNxPrivateGlobals);

    nxWake = GetNxWakeFromHandle(NetPowerSettings);

    Verifier_VerifyNetPowerSettingsAccessible(pNxPrivateGlobals, nxWake);

    FuncExit(FLAG_POWER);
    return nxWake->GetWakePatternCountForType(WakePatternType);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
ULONG
NETEXPORT(NetPowerSettingsGetProtocolOffloadCount)(
    _In_ PNET_DRIVER_GLOBALS    DriverGlobals,
    _In_ NETPOWERSETTINGS       NetPowerSettings
    )
/*++
Routine Description:
    Returns the number of protocol offloads stored in the NETPOWERSETTINGS object.
    
    IMPORTANT: This includes both, offloads that are enabled and disabled.
    The driver can use the NetPowerSettingsIsProtocolOffloadEnabled API to check
    if a particular protocol offload is enabled.

    This API must only be called during a power transition or from the 
    EvtPreviewWolPattern callback.

Arguments:
    NetPowerSettings - The NetPowerSettings object
 
Returns: 
    Returns the number of protocol offloads stored in the NETPOWERSETTINGS object.
--*/
{
    FuncEntry(FLAG_POWER);
    PNxWake nxWake;
    PNX_PRIVATE_GLOBALS pNxPrivateGlobals = GetPrivateGlobals(DriverGlobals);

    Verifier_VerifyPrivateGlobals(pNxPrivateGlobals);

    nxWake = GetNxWakeFromHandle(NetPowerSettings);

    Verifier_VerifyNetPowerSettingsAccessible(pNxPrivateGlobals, nxWake);

    FuncExit(FLAG_POWER);
    return nxWake->GetProtocolOffloadCount();
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
ULONG
NETEXPORT(NetPowerSettingsGetProtocolOffloadCountForType)(
    _In_ PNET_DRIVER_GLOBALS                DriverGlobals,
    _In_ NETPOWERSETTINGS                   NetPowerSettings,
    _In_ NDIS_PM_PROTOCOL_OFFLOAD_TYPE      ProtocolOffloadType
    )
/*++
Routine Description:
    Returns the number of protocol offloads in the NETPOWERSETTINGS object for 
    the particular offload type

    IMPORTANT: This includes both, offloads that are enabled and disabled.
    The driver can use the NetPowerSettingsIsProtocolOffloadEnabled API to check
    if a particular protocol offload is enabled.

    This API must only be called during a power transition or from the
    EvtPreviewWolPattern callback.

Arguments:
    NetPowerSettings - The NetPowerSettings object
    ProtocolOffloadType - The offload type that needs to be looked up

Returns:
    Returns the number of protocol offloads in the NETPOWERSETTINGS object for 
    the specified protocol offload type
--*/
{
    FuncEntry(FLAG_POWER);
    PNxWake nxWake;
    PNX_PRIVATE_GLOBALS pNxPrivateGlobals = GetPrivateGlobals(DriverGlobals);

    Verifier_VerifyPrivateGlobals(pNxPrivateGlobals);

    nxWake = GetNxWakeFromHandle(NetPowerSettings);

    Verifier_VerifyNetPowerSettingsAccessible(pNxPrivateGlobals, nxWake);

    FuncExit(FLAG_POWER);
    return nxWake->GetProtocolOffloadCountForType(ProtocolOffloadType);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PNDIS_PM_PROTOCOL_OFFLOAD
NETEXPORT(NetPowerSettingsGetProtocolOffload)(
    _In_ PNET_DRIVER_GLOBALS    DriverGlobals,
    _In_ NETPOWERSETTINGS       NetPowerSettings,
    _In_ ULONG                  Index
    )
/*++
Routine Description:
    Returns a PNDIS_PM_PROTOCOL_OFFLOAD structure at the provided Index.
    Note that the Index is 0 based.

    This API must only be called during a power transition or from the 
    EvtPreviewProtocolOffload callback. In both cases, the driver should only 
    access/examine the  PNDIS_PM_PROTOCOL_OFFLOAD (obtained from this API) and
    should NOT cache or retain a reference to the protocol offload. 
    This is because the Cx will automatically release it while handling 
    offload removal without notifying the driver

Arguments:
    NetPowerSettings - The NetPowerSettings object
    Index - 0 based index. This value must be < NetPowerSettingsGetProtocolOffloadCount

Returns: 
    PNDIS_PM_PROTOCOL_OFFLOAD structure at Index. Returns NULL if Index is invalid.

--*/
{
    FuncEntry(FLAG_POWER);
    PNxWake nxWake;
    PNX_PRIVATE_GLOBALS pNxPrivateGlobals;
    PNX_NET_POWER_ENTRY nxPowerEntry;

    pNxPrivateGlobals = GetPrivateGlobals(DriverGlobals);

    Verifier_VerifyPrivateGlobals(pNxPrivateGlobals);

    nxWake = GetNxWakeFromHandle(NetPowerSettings);
    Verifier_VerifyNetPowerSettingsAccessible(pNxPrivateGlobals, nxWake);

    nxPowerEntry = nxWake->GetEntryAtIndex(Index,  NxPowerEntryTypeProtocolOffload);
    Verifier_VerifyNotNull(pNxPrivateGlobals, (PVOID)nxPowerEntry);

    FuncExit(FLAG_POWER);
    return nxPowerEntry ? &(nxPowerEntry->NdisProtocolOffload) : NULL;
}
 

WDFAPI
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN
NETEXPORT(NetPowerSettingsIsProtocolOffloadEnabled)(
    _In_ PNET_DRIVER_GLOBALS       DriverGlobals,
    _In_ NETPOWERSETTINGS          NetPowerSettings,
    _In_ PNDIS_PM_PROTOCOL_OFFLOAD ProtocolOffload
    )
/*++
Routine Description:
    This API can be used to determine if the PNDIS_PM_PROTOCOL_OFFLOAD obtained 
    from a prior call to NetPowerSettingsGetProtocolOffload is enabled. 

    This API must only be called during a power transition or from the 
    EvtPreviewProtocolOffload callback.

Arguments:
    NetPowerSettings - The NetPowerSettings object
    NdisOffload - Pointer to PNDIS_PM_PROTOCOL_OFFLOAD structure that must be 
                obtained by a prior call to NetPowerSettingsGetProtocolOffload

Returns: 
    Returns TRUE if the protocol offload has been enabled.
    Returns FALSE if the offload is not enabled.
--*/
{
    FuncEntry(FLAG_POWER);
    PNxWake nxWake;
    PNX_PRIVATE_GLOBALS pNxPrivateGlobals = GetPrivateGlobals(DriverGlobals);
    PNX_NET_POWER_ENTRY nxPowerEntry;

    Verifier_VerifyPrivateGlobals(pNxPrivateGlobals);

    nxWake = GetNxWakeFromHandle(NetPowerSettings);

    Verifier_VerifyNetPowerSettingsAccessible(pNxPrivateGlobals, nxWake);

    nxPowerEntry = CONTAINING_RECORD(ProtocolOffload,
                                NX_NET_POWER_ENTRY,
                                NdisProtocolOffload);
    FuncExit(FLAG_POWER);
    return nxPowerEntry->Enabled;
}


}

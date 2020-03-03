// Copyright (C) Microsoft Corporation. All rights reserved.
#include "precompiled.hpp"

#include "NxPowerPolicy.tmh"
#include "NxPowerPolicy.hpp"

NxPowerPolicy::NxPowerPolicy(
    _In_ WDFDEVICE Device,
    _In_ NET_DEVICE_POWER_POLICY_EVENT_CALLBACKS const & PowerPolicyCallbacks,
    _In_ NETADAPTER Adapter,
    _In_ RECORDER_LOG RecorderLog
)
    : m_device(Device)
    , m_adapter(Adapter)
    , m_recorderLog(RecorderLog)
    , m_wakeOnMediaChange(Adapter)
    , m_wakeOnPacketFilterMatch(Adapter)
{
    if (PowerPolicyCallbacks.Size != 0)
    {
        RtlCopyMemory(
            &m_powerPolicyEventCallbacks,
            &PowerPolicyCallbacks,
            PowerPolicyCallbacks.Size);
    }
}

NxPowerPolicy::~NxPowerPolicy(
    void
)
{
    while (m_WakeListHead.Next != nullptr)
    {
        auto listEntry = PopEntryList(&m_WakeListHead);
        auto powerEntry = CONTAINING_RECORD(listEntry, NxWakeSource, m_powerPolicyLinkage);

        powerEntry->~NxWakeSource();
        ExFreePoolWithTag(powerEntry, NETCX_POWER_TAG);
    }

    while (m_ProtocolOffloadListHead.Next != nullptr)
    {
        auto listEntry = PopEntryList(&m_ProtocolOffloadListHead);
        auto powerEntry = CONTAINING_RECORD(listEntry, NxPowerOffload, m_powerPolicyLinkage);

        powerEntry->~NxPowerOffload();
        ExFreePoolWithTag(powerEntry, NETCX_POWER_TAG);
    }
}

_Use_decl_annotations_
void
NxPowerPolicy::InitializeNdisCapabilities(
    ULONG MediaSpecificWakeUpEvents,
    NDIS_PM_CAPABILITIES * NdisPowerCapabilities
)
{
    *NdisPowerCapabilities = {};
    NdisPowerCapabilities->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
    NdisPowerCapabilities->Header.Size = NDIS_SIZEOF_NDIS_PM_CAPABILITIES_REVISION_2;
    NdisPowerCapabilities->Header.Revision = NDIS_PM_CAPABILITIES_REVISION_2;

    NdisPowerCapabilities->MediaSpecificWakeUpEvents = MediaSpecificWakeUpEvents;

    if (m_wakeMediaChangeCapabilities.MediaConnect)
    {
        NdisPowerCapabilities->SupportedWakeUpEvents |= NDIS_PM_WAKE_ON_MEDIA_CONNECT_SUPPORTED;
    }

    if (m_wakeMediaChangeCapabilities.MediaDisconnect)
    {
        NdisPowerCapabilities->SupportedWakeUpEvents |= NDIS_PM_WAKE_ON_MEDIA_DISCONNECT_SUPPORTED;
    }

    if (m_magicPacketCapabilities.MagicPacket)
    {
        NdisPowerCapabilities->SupportedWoLPacketPatterns |= NDIS_PM_WOL_MAGIC_PACKET_SUPPORTED;
    }

    if (m_wakeBitmapCapabilities.BitmapPattern)
    {
        NdisPowerCapabilities->SupportedWoLPacketPatterns |= NDIS_PM_WOL_BITMAP_PATTERN_SUPPORTED;
    }

    NdisPowerCapabilities->MaxWoLPatternSize = static_cast<ULONG>(m_wakeBitmapCapabilities.MaximumPatternSize);
    NdisPowerCapabilities->NumTotalWoLPatterns = static_cast<ULONG>(m_wakeBitmapCapabilities.MaximumPatternCount);

    if (m_wakePacketFilterCapabilities.PacketFilterMatch)
    {
        NdisPowerCapabilities->Flags |= NDIS_PM_SELECTIVE_SUSPEND_SUPPORTED;
    }

    if (m_powerOffloadArpCapabilities.ArpOffload)
    {
        NdisPowerCapabilities->SupportedProtocolOffloads |= NDIS_PM_PROTOCOL_OFFLOAD_ARP_SUPPORTED;
        NdisPowerCapabilities->NumArpOffloadIPv4Addresses = static_cast<ULONG>(m_powerOffloadArpCapabilities.MaximumOffloadCount);
    }

    if (m_powerOffloadNSCapabilities.NSOffload)
    {
        NdisPowerCapabilities->SupportedProtocolOffloads |= NDIS_PM_PROTOCOL_OFFLOAD_NS_SUPPORTED;
        NdisPowerCapabilities->NumNSOffloadIPv6Addresses = static_cast<ULONG>(m_powerOffloadNSCapabilities.MaximumOffloadCount);
    }

    //
    // WDF will request the D-IRPs. Doing this allows NDIS to treat this
    // device as wake capable and send appropriate PM parameters.
    //
    if (NdisPowerCapabilities->SupportedWoLPacketPatterns & NDIS_PM_WOL_BITMAP_PATTERN_SUPPORTED)
    {
        NdisPowerCapabilities->MinPatternWakeUp = NdisDeviceStateD2;
    }
    if (NdisPowerCapabilities->SupportedWoLPacketPatterns & NDIS_PM_WOL_MAGIC_PACKET_ENABLED)
    {
        NdisPowerCapabilities->MinMagicPacketWakeUp = NdisDeviceStateD2;
    }
    if (NdisPowerCapabilities->SupportedWakeUpEvents & NDIS_PM_WAKE_ON_MEDIA_CONNECT_SUPPORTED)
    {
        NdisPowerCapabilities->MinLinkChangeWakeUp = NdisDeviceStateD2;
    }
}

_Use_decl_annotations_
void
NxPowerPolicy::SetPowerOffloadArpCapabilities(
    NET_ADAPTER_POWER_OFFLOAD_ARP_CAPABILITIES const * Capabilities
)
{
    RtlCopyMemory(&m_powerOffloadArpCapabilities, Capabilities, Capabilities->Size);
}

_Use_decl_annotations_
void
NxPowerPolicy::SetPowerOffloadNSCapabilities(
    NET_ADAPTER_POWER_OFFLOAD_NS_CAPABILITIES const * Capabilities
)
{
    RtlCopyMemory(&m_powerOffloadNSCapabilities, Capabilities, Capabilities->Size);
}

_Use_decl_annotations_
void
NxPowerPolicy::SetWakeBitmapCapabilities(
    NET_ADAPTER_WAKE_BITMAP_CAPABILITIES const * Capabilities
)
{
    RtlCopyMemory(&m_wakeBitmapCapabilities, Capabilities, Capabilities->Size);
}

_Use_decl_annotations_
void
NxPowerPolicy::SetMagicPacketCapabilities(
    NET_ADAPTER_WAKE_MAGIC_PACKET_CAPABILITIES const * Capabilities
)
{
    RtlCopyMemory(&m_magicPacketCapabilities, Capabilities, Capabilities->Size);
}

_Use_decl_annotations_
void
NxPowerPolicy::SetWakeMediaChangeCapabilities(
    NET_ADAPTER_WAKE_MEDIA_CHANGE_CAPABILITIES const * Capabilities
)
{
    RtlCopyMemory(&m_wakeMediaChangeCapabilities, Capabilities, Capabilities->Size);
}

_Use_decl_annotations_
void
NxPowerPolicy::SetWakePacketFilterCapabilities(
    NET_ADAPTER_WAKE_PACKET_FILTER_CAPABILITIES const * Capabilities
)
{
    RtlCopyMemory(&m_wakePacketFilterCapabilities, Capabilities, Capabilities->Size);
}

_Use_decl_annotations_
void
NxPowerPolicy::UpdatePowerOffloadList(
    bool IsInPowerTransition,
    NxPowerOffloadList * List
) const
{
    for (auto link = m_ProtocolOffloadListHead.Next; link != nullptr; link = link->Next)
    {
        auto entry = CONTAINING_RECORD(link, NxPowerOffload, m_powerPolicyLinkage);

        auto const addToList = !IsInPowerTransition || entry->IsEnabled();

        if (addToList)
        {
            List->PushEntry(entry);
        }
    }
}

_Use_decl_annotations_
void
NxPowerPolicy::UpdateWakeSourceList(
    bool IsInPowerTransition,
    NxWakeSourceList * List
) const
{
    for (auto link = m_WakeListHead.Next; link != nullptr; link = link->Next)
    {
        auto entry = CONTAINING_RECORD(link, NxWakeSource, m_powerPolicyLinkage);

        auto const addToList = !IsInPowerTransition || entry->IsEnabled();

        if (addToList)
        {
            List->PushEntry(entry);
        }
    }

    //
    // Only add a NETWAKESOURCE entry for media change to the list if:
    //     1) We're not in a power transition *and* the client declared support for wake on media change (either connect or disconnect)
    //     2) We are in a power transition and wake on media change is enabled by NDIS
    //
    auto const clientSupportsWakeOnMediaChange = m_wakeMediaChangeCapabilities.MediaConnect || m_wakeMediaChangeCapabilities.MediaDisconnect;

    auto const addMediaChange = (!IsInPowerTransition && clientSupportsWakeOnMediaChange) || m_wakeOnMediaChange.IsEnabled();

    if (addMediaChange)
    {
        List->PushEntry(const_cast<NxWakeMediaChange *>(&m_wakeOnMediaChange));
    }

    //
    // Only add a NETWAKESOURCE entry for wake on packet filter match if:
    //     1) We're not in a power transition *and* the client declared support for it
    //     2) We are in a power transition and selective suspend or NAPS is enabled by NDIS
    //
    auto const addPacketFilterMatch = (!IsInPowerTransition && m_wakePacketFilterCapabilities.PacketFilterMatch) || m_wakeOnPacketFilterMatch.IsEnabled();

    if (addPacketFilterMatch)
    {
        List->PushEntry(const_cast<NxWakePacketFilterMatch *>(&m_wakeOnPacketFilterMatch));
    }
}

NDIS_STATUS
NxPowerPolicy::AddProtocolOffload(
    _In_ NDIS_OID_REQUEST::_REQUEST_DATA::_SET const * SetInformation
)
{
    if (SetInformation->InformationBufferLength < sizeof(NDIS_PM_PROTOCOL_OFFLOAD))
    {
        LogError(GetRecorderLog(), FLAG_POWER,
            "Invalid InformationBufferLength (%u) for OID_PM_ADD_PROTOCOL_OFFLOAD", SetInformation->InformationBufferLength);

        return NDIS_STATUS_INVALID_PARAMETER;
    }

    auto ndisProtocolOffload = static_cast<NDIS_PM_PROTOCOL_OFFLOAD const *>(SetInformation->InformationBuffer);

    KPoolPtr<NxPowerOffload> powerOffload;
    PFN_NET_DEVICE_PREVIEW_POWER_OFFLOAD pfnPreview;

    switch (ndisProtocolOffload->ProtocolOffloadType)
    {
        case NdisPMProtocolOffloadIdIPv4ARP:
        {
            if (!m_powerOffloadArpCapabilities.ArpOffload)
            {
                return NDIS_STATUS_NOT_SUPPORTED;
            }

            auto const ndisStatus = NxArpOffload::CreateFromNdisPmOffload(
                m_adapter,
                ndisProtocolOffload,
                wil::out_param(powerOffload));

            if (ndisStatus != NDIS_STATUS_SUCCESS)
            {
                return ndisStatus;
            }

            pfnPreview = m_powerPolicyEventCallbacks.EvtDevicePreviewArpOffload;
            break;
        }
        case NdisPMProtocolOffloadIdIPv6NS:
        {
            if (!m_powerOffloadNSCapabilities.NSOffload)
            {
                return NDIS_STATUS_NOT_SUPPORTED;
            }

            auto const ndisStatus = NxNSOffload::CreateFromNdisPmOffload(
                m_adapter,
                ndisProtocolOffload,
                wil::out_param(powerOffload));

            if (ndisStatus != NDIS_STATUS_SUCCESS)
            {
                return ndisStatus;
            }

            pfnPreview = m_powerPolicyEventCallbacks.EvtDevicePreviewNSOffload;
            break;
        }
        default:
        {
            return NDIS_STATUS_NOT_SUPPORTED;
        }
    }

    if (pfnPreview != nullptr)
    {
        auto const previewStatus = pfnPreview(m_device, powerOffload->GetHandle());

        if (previewStatus == STATUS_NDIS_PM_PROTOCOL_OFFLOAD_LIST_FULL)
        {
            return NDIS_STATUS_PM_PROTOCOL_OFFLOAD_LIST_FULL;
        }
        else if (previewStatus != STATUS_SUCCESS)
        {
            return NDIS_STATUS_FAILURE;
        }
    }

    //
    // Change powerEntry to see if it is enabled.
    //
    UpdateProtocolOffloadEntryEnabledField(powerOffload.get());

    PushEntryList(&m_ProtocolOffloadListHead, &powerOffload->m_powerPolicyLinkage);

    powerOffload.release();

    return NDIS_STATUS_SUCCESS;
}

_Use_decl_annotations_
NxPowerOffload *
NxPowerPolicy::RemovePowerOffloadByID(
    _In_ ULONG OffloadID
)
{
    auto prevEntry = &m_ProtocolOffloadListHead;
    auto listEntry = prevEntry->Next;

    while (listEntry != nullptr)
    {
        auto powerOffload = CONTAINING_RECORD(listEntry, NxPowerOffload, m_powerPolicyLinkage);

        if (powerOffload->GetID() == OffloadID)
        {
            PopEntryList(prevEntry);
            return powerOffload;
        }
        prevEntry = listEntry;
        listEntry = listEntry->Next;
    }

    return nullptr;
}

NDIS_STATUS
NxPowerPolicy::RemoveProtocolOffload(
    _In_ NDIS_OID_REQUEST::_REQUEST_DATA::_SET const * SetInformation
)
/*++
Routine Description:
    Processes removal of protocol offloads.
--*/
{
    if (SetInformation->InformationBufferLength < sizeof(ULONG))
    {
        LogError(GetRecorderLog(), FLAG_POWER,
            "Invalid InformationBufferLength (%u) for OID_PM_REMOVE_PROTOCOL_OFFLOAD", SetInformation->InformationBufferLength);

        return NDIS_STATUS_INVALID_PARAMETER;
    }

    auto const & offloadID = *(static_cast<ULONG *>(SetInformation->InformationBuffer));

    auto removedEntry = RemovePowerOffloadByID(offloadID);

    if (removedEntry == nullptr)
    {
        return NDIS_STATUS_FILE_NOT_FOUND;
    }

    removedEntry->~NxPowerOffload();
    ExFreePoolWithTag(removedEntry, NETCX_POWER_TAG);

    return NDIS_STATUS_SUCCESS;
}

RECORDER_LOG
NxPowerPolicy::GetRecorderLog(
    void
)
{
    return m_recorderLog;
}

NDIS_STATUS
NxPowerPolicy::AddWakePattern(
    _In_ NDIS_OID_REQUEST::_REQUEST_DATA::_SET const * SetInformation
)
{
    if (SetInformation->InformationBufferLength < sizeof(NDIS_PM_WOL_PATTERN))
    {
        LogError(GetRecorderLog(), FLAG_POWER,
            "Invalid InformationBufferLength (%u) for OID_PM_ADD_WOL_PATTERN", SetInformation->InformationBufferLength);

        return NDIS_STATUS_INVALID_PARAMETER;
    }

    auto ndisWolPattern = static_cast<NDIS_PM_WOL_PATTERN const *>(SetInformation->InformationBuffer);

    KPoolPtr<NxWakePattern> wakePattern;
    PFN_NET_DEVICE_PREVIEW_WAKE_SOURCE pfnPreview;

    switch (ndisWolPattern->WoLPacketType)
    {
        case NdisPMWoLPacketBitmapPattern:
        {
            if (!m_wakeBitmapCapabilities.BitmapPattern)
            {
                return NDIS_STATUS_NOT_SUPPORTED;
            }

            auto const ndisStatus = NxWakeBitmapPattern::CreateFromNdisWoLPattern(
                m_adapter,
                ndisWolPattern,
                wil::out_param(wakePattern));

            if (ndisStatus != NDIS_STATUS_SUCCESS)
            {
                return ndisStatus;
            }

            pfnPreview = m_powerPolicyEventCallbacks.EvtDevicePreviewBitmapPattern;
            break;
        }
        case NdisPMWoLPacketMagicPacket:
        {
            if (!m_magicPacketCapabilities.MagicPacket)
            {
                return NDIS_STATUS_NOT_SUPPORTED;
            }

            auto const ndisStatus = NxWakeMagicPacket::CreateFromNdisWoLPattern(
                m_adapter,
                ndisWolPattern,
                wil::out_param(wakePattern));

            if (ndisStatus != NDIS_STATUS_SUCCESS)
            {
                return ndisStatus;
            }

            // No preview for magic packet
            pfnPreview = nullptr;
            break;
        }
        default:
        {
            return NDIS_STATUS_NOT_SUPPORTED;
        }
    }

    if (pfnPreview != nullptr)
    {
        auto const previewStatus = pfnPreview(m_device, wakePattern->GetHandle());

        if (previewStatus == STATUS_NDIS_PM_WOL_PATTERN_LIST_FULL)
        {
            return NDIS_STATUS_PM_WOL_PATTERN_LIST_FULL;
        }
        else if (previewStatus != STATUS_SUCCESS)
        {
            return NDIS_STATUS_FAILURE;
        }
    }

    //
    // Change powerEntry to see if it is enabled.
    //
    UpdatePatternEntryEnabledField(wakePattern.get());

    PushEntryList(&m_WakeListHead, &wakePattern->m_powerPolicyLinkage);

    wakePattern.release();

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
NxPowerPolicy::RemoveWakePattern(
    _In_ NDIS_OID_REQUEST::_REQUEST_DATA::_SET const * SetInformation
)
/*++
Routine Description:
    Processes removal of Wake patterns.
--*/
{
    if (SetInformation->InformationBufferLength < sizeof(ULONG))
    {
        LogError(GetRecorderLog(), FLAG_POWER,
            "Invalid InformationBufferLength (%u) for OID_PM_REMOVE_WOL_PATTERN", SetInformation->InformationBufferLength);

        return NDIS_STATUS_INVALID_PARAMETER;
    }

    auto const & patternID = *(static_cast<ULONG *>(SetInformation->InformationBuffer));

    auto removedEntry = RemoveWakePatternByID(patternID);

    if (removedEntry == nullptr)
    {
        return NDIS_STATUS_FILE_NOT_FOUND;
    }

    removedEntry->~NxWakePattern();
    ExFreePoolWithTag(removedEntry, NETCX_POWER_TAG);

    return NDIS_STATUS_SUCCESS;
}

NxWakePattern *
NxPowerPolicy::RemoveWakePatternByID(
    _In_ ULONG PatternID
)
{
    auto prevEntry = &m_WakeListHead;
    auto listEntry = prevEntry->Next;

    while (listEntry != nullptr)
    {
        auto wakePattern = CONTAINING_RECORD(listEntry, NxWakePattern, m_powerPolicyLinkage);

        if (wakePattern->GetID() == PatternID)
        {
            PopEntryList(prevEntry);
            return wakePattern;
        }
        prevEntry = listEntry;
        listEntry = listEntry->Next;
    }

    return nullptr;
}

NDIS_STATUS
NxPowerPolicy::SetParameters(
    _In_ NDIS_OID_REQUEST::_REQUEST_DATA::_SET const * Set
)
/*++
Routine Description:
    Stores the incoming the NDIS_PM_PARAMETERS and if the Wake pattern
    have changed then it updates the Wake patterns to reflect the change
--*/
{
    if (Set->InformationBufferLength < sizeof(NDIS_PM_PARAMETERS))
    {
        LogError(GetRecorderLog(), FLAG_POWER,
            "Invalid InformationBufferLength (%u) for OID_PM_PARAMETERS", Set->InformationBufferLength);

        return NDIS_STATUS_INVALID_PARAMETER;
    }

    auto const PmParams = static_cast<NDIS_PM_PARAMETERS *>(Set->InformationBuffer);

    LogInfo(GetRecorderLog(), FLAG_POWER,
        "Received NDIS_PM_PARAMETERS: EnabledWoLPacketPatterns=0x%08x, EnabledProtocolOffloads=0x%08x, WakeUpFlags=0x%08x",
        PmParams->EnabledWoLPacketPatterns, PmParams->EnabledProtocolOffloads, PmParams->WakeUpFlags);

    if (PmParams->WakeUpFlags & (NDIS_PM_SELECTIVE_SUSPEND_ENABLED | NDIS_PM_AOAC_NAPS_ENABLED))
    {
        //
        // To conform with MSDN documentation, if NDIS SS flag is present:
        //    1) Enable all supported control path wake sources
        //    2) Make sure EnabledWoLPacketPatterns is zero
        //

        if (m_wakeMediaChangeCapabilities.MediaConnect)
        {
            PmParams->WakeUpFlags |= NDIS_PM_WAKE_ON_LINK_CHANGE_ENABLED;
        }

        if (m_wakeMediaChangeCapabilities.MediaDisconnect)
        {
            PmParams->WakeUpFlags |= NDIS_PM_WAKE_ON_MEDIA_DISCONNECT_ENABLED;
        }

        PmParams->EnabledWoLPacketPatterns = 0;
    }

    LogInfo(GetRecorderLog(), FLAG_POWER,
        "Saved NDIS_PM_PARAMETERS: EnabledWoLPacketPatterns=0x%08x, EnabledProtocolOffloads=0x%08x, WakeUpFlags=0x%08x",
        PmParams->EnabledWoLPacketPatterns, PmParams->EnabledProtocolOffloads, PmParams->WakeUpFlags);

    auto const updateWakeUpEvents = m_PMParameters.WakeUpFlags != PmParams->WakeUpFlags;
    auto const updateWakePatterns = m_PMParameters.EnabledWoLPacketPatterns != PmParams->EnabledWoLPacketPatterns;
    auto const updateProtocolOffload = m_PMParameters.EnabledProtocolOffloads != PmParams->EnabledProtocolOffloads;

    RtlCopyMemory(
        &m_PMParameters,
        PmParams,
        sizeof(m_PMParameters));

    if (updateWakeUpEvents)
    {
        //
        // Make sure we filter out anything the client driver does not support
        //

        ULONG supportedWakeUpFlags = 0;

        if (m_wakeMediaChangeCapabilities.MediaConnect)
        {
            supportedWakeUpFlags |= NDIS_PM_WAKE_ON_LINK_CHANGE_ENABLED;
        }

        if (m_wakeMediaChangeCapabilities.MediaDisconnect)
        {
            supportedWakeUpFlags |= NDIS_PM_WAKE_ON_MEDIA_DISCONNECT_ENABLED;
        }

        if (m_wakePacketFilterCapabilities.PacketFilterMatch)
        {
            supportedWakeUpFlags |= NDIS_PM_SELECTIVE_SUSPEND_ENABLED | NDIS_PM_AOAC_NAPS_ENABLED;
        }

        LogInfo(GetRecorderLog(), FLAG_POWER,
            "Supported WakeUpFlags=0x%08x", supportedWakeUpFlags);

        auto const effectiveWakeUpFlags = m_PMParameters.WakeUpFlags & supportedWakeUpFlags;

        LogInfo(GetRecorderLog(), FLAG_POWER,
            "Effective WakeUpFlags=0x%08x", effectiveWakeUpFlags);

        m_wakeOnMediaChange.SetWakeUpFlags(effectiveWakeUpFlags);
        m_wakeOnPacketFilterMatch.SetWakeUpFlags(effectiveWakeUpFlags);
    }

    if (updateWakePatterns)
    {
        auto listEntry = m_WakeListHead.Next;
        while (listEntry != nullptr)
        {
            auto powerEntry = CONTAINING_RECORD(listEntry, NxWakePattern, m_powerPolicyLinkage);

            UpdatePatternEntryEnabledField(powerEntry);
            listEntry = listEntry->Next;
        }
    }

    if (updateProtocolOffload)
    {
        auto listEntry = m_ProtocolOffloadListHead.Next;
        while (listEntry != nullptr)
        {
            auto powerEntry = CONTAINING_RECORD(listEntry, NxPowerOffload, m_powerPolicyLinkage);

            UpdateProtocolOffloadEntryEnabledField(powerEntry);
            listEntry = listEntry->Next;
        }
    }

    return NDIS_STATUS_SUCCESS;
}

void
NxPowerPolicy::UpdateProtocolOffloadEntryEnabledField(
    _In_ NxPowerOffload * Entry
)
{
    bool enabled = false;

    switch (Entry->GetType())
    {
        case NetPowerOffloadTypeArp:
            enabled = !!(m_PMParameters.EnabledProtocolOffloads & NDIS_PM_PROTOCOL_OFFLOAD_ARP_ENABLED);
            break;
        case NetPowerOffloadTypeNS:
            enabled = !!(m_PMParameters.EnabledProtocolOffloads & NDIS_PM_PROTOCOL_OFFLOAD_NS_ENABLED);
            break;
        default:
            NT_ASSERTMSG("Unexpected protocol offload type", 0);
            break;
    }

    Entry->SetEnabled(enabled);
}

void
NxPowerPolicy::UpdatePatternEntryEnabledField(
    _In_ NxWakePattern * Entry
)
{
    bool enabled = false;

    switch (Entry->GetType())
    {
        case NetWakeSourceTypeBitmapPattern:
            enabled = !!(m_PMParameters.EnabledWoLPacketPatterns & NDIS_PM_WOL_BITMAP_PATTERN_ENABLED);
            break;
        case NetWakeSourceTypeMagicPacket:
            enabled = !!(m_PMParameters.EnabledWoLPacketPatterns & NDIS_PM_WOL_MAGIC_PACKET_ENABLED);
            break;
    }

    Entry->SetEnabled(enabled);
}

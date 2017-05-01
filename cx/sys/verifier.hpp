/*++

Copyright (C) Microsoft Corporation. All rights reserved.

Module Name:
    Verifier.hpp

Abstract:
    Contains definitions used by NetAdapterCx to detect and report violations




Environment:
    kernel mode only

Revision History:

--*/

#pragma once

//
// NET_*_SUPPORTED_FLAGS are used to check if a client is passing valid flags to NetAdapterCx APIs
//

// There is not yet a public NDIS_PM or NET_ADAPTER_POWER flag for NDIS_PM_AOAC_NAPS_SUPPORTED, but it's used in
// test code. So for now, just using that private define, but at some point we need to figure out the right thing to do here.
#define NET_ADAPTER_POWER_CAPABILITIES_SUPPORTED_FLAGS (NET_ADAPTER_POWER_WAKE_PACKET_INDICATION                    | \
                                                        NET_ADAPTER_POWER_SELECTIVE_SUSPEND                         | \
                                                        NDIS_PM_AOAC_NAPS_SUPPORTED)

#define NET_ADAPTER_PROTOCOL_OFFLOADS_SUPPORTED_FLAGS  (NET_ADAPTER_PROTOCOL_OFFLOAD_ARP                            | \
                                                        NET_ADAPTER_PROTOCOL_OFFLOAD_NS                             | \
                                                        NET_ADAPTER_PROTOCOL_OFFLOAD_80211_RSN_REKEY)

#define NET_ADAPTER_WAKEUP_SUPPORTED_FLAGS             (NET_ADAPTER_WAKE_ON_MEDIA_CONNECT                           | \
                                                        NET_ADAPTER_WAKE_ON_MEDIA_DISCONNECT)

#define NET_ADAPTER_WAKEUP_MEDIA_SPECIFIC_SUPPORTED_FLAGS (NET_ADAPTER_WLAN_WAKE_ON_NLO_DISCOVERY                   | \
                                                           NET_ADAPTER_WLAN_WAKE_ON_AP_ASSOCIATION_LOST             | \
                                                           NET_ADAPTER_WLAN_WAKE_ON_GTK_HANDSHAKE_ERROR             | \
                                                           NET_ADAPTER_WLAN_WAKE_ON_4WAY_HANDSHAKE_REQUEST          | \
                                                           NET_ADAPTER_WWAN_WAKE_ON_REGISTER_STATE                  | \
                                                           NET_ADAPTER_WWAN_WAKE_ON_SMS_RECEIVE                     | \
                                                           NET_ADAPTER_WWAN_WAKE_ON_USSD_RECEIVE                    | \
                                                           NET_ADAPTER_WWAN_WAKE_ON_PACKET_STATE                    | \
                                                           NET_ADAPTER_WWAN_WAKE_ON_UICC_CHANGE)

#define NET_ADAPTER_WAKE_SUPPORTED_FLAGS               (NET_ADAPTER_WAKE_BITMAP_PATTERN                              | \
                                                        NET_ADAPTER_WAKE_MAGIC_PACKET                                | \
                                                        NET_ADAPTER_WAKE_IPV4_TCP_SYN                                | \
                                                        NET_ADAPTER_WAKE_IPV6_TCP_SYN                                | \
                                                        NET_ADAPTER_WAKE_IPV4_DEST_ADDR_WILDCARD                     | \
                                                        NET_ADAPTER_WAKE_IPV6_DEST_ADDR_WILDCARD                     | \
                                                        NET_ADAPTER_WAKE_EAPOL_REQUEST_ID_MESSAGE)

#define NET_ADAPTER_STATISTICS_SUPPORTED_FLAGS         (NET_ADAPTER_STATISTICS_XMIT_OK                              | \
                                                        NET_ADAPTER_STATISTICS_RCV_OK                               | \
                                                        NET_ADAPTER_STATISTICS_XMIT_ERROR                           | \
                                                        NET_ADAPTER_STATISTICS_RCV_ERROR                            | \
                                                        NET_ADAPTER_STATISTICS_RCV_NO_BUFFER                        | \
                                                        NET_ADAPTER_STATISTICS_DIRECTED_BYTES_XMIT                  | \
                                                        NET_ADAPTER_STATISTICS_DIRECTED_FRAMES_XMIT                 | \
                                                        NET_ADAPTER_STATISTICS_MULTICAST_BYTES_XMIT                 | \
                                                        NET_ADAPTER_STATISTICS_MULTICAST_FRAMES_XMIT                | \
                                                        NET_ADAPTER_STATISTICS_BROADCAST_BYTES_XMIT                 | \
                                                        NET_ADAPTER_STATISTICS_BROADCAST_FRAMES_XMIT                | \
                                                        NET_ADAPTER_STATISTICS_DIRECTED_BYTES_RCV                   | \
                                                        NET_ADAPTER_STATISTICS_DIRECTED_FRAMES_RCV                  | \
                                                        NET_ADAPTER_STATISTICS_MULTICAST_BYTES_RCV                  | \
                                                        NET_ADAPTER_STATISTICS_MULTICAST_FRAMES_RCV                 | \
                                                        NET_ADAPTER_STATISTICS_BROADCAST_BYTES_RCV                  | \
                                                        NET_ADAPTER_STATISTICS_BROADCAST_FRAMES_RCV                 | \
                                                        NET_ADAPTER_STATISTICS_RCV_CRC_ERROR                        | \
                                                        NET_ADAPTER_STATISTICS_TRANSMIT_QUEUE_LENGTH                | \
                                                        NET_ADAPTER_STATISTICS_BYTES_RCV                            | \
                                                        NET_ADAPTER_STATISTICS_BYTES_XMIT                           | \
                                                        NET_ADAPTER_STATISTICS_RCV_DISCARDS                         | \
                                                        NET_ADAPTER_STATISTICS_GEN_STATISTICS                       | \
                                                        NET_ADAPTER_STATISTICS_XMIT_DISCARDS)                         \

#define NET_PACKET_FILTER_SUPPORTED_FLAGS              (NET_PACKET_FILTER_TYPE_DIRECTED                             | \
                                                        NET_PACKET_FILTER_TYPE_MULTICAST                            | \
                                                        NET_PACKET_FILTER_TYPE_ALL_MULTICAST                        | \
                                                        NET_PACKET_FILTER_TYPE_BROADCAST                            | \
                                                        NET_PACKET_FILTER_TYPE_SOURCE_ROUTING                       | \
                                                        NET_PACKET_FILTER_TYPE_PROMISCUOUS                          | \
                                                        NET_PACKET_FILTER_TYPE_ALL_LOCAL                            | \
                                                        NET_PACKET_FILTER_TYPE_MAC_FRAME                            | \
                                                        NET_PACKET_FILTER_TYPE_NO_LOCAL)

#define NDIS_AUTO_NEGOTIATION_SUPPORTED_FLAGS          (NET_ADAPTER_AUTO_NEGOTIATION_NO_FLAGS                       | \
                                                        NET_ADAPTER_LINK_STATE_XMIT_LINK_SPEED_AUTO_NEGOTIATED      | \
                                                        NET_ADAPTER_LINK_STATE_RCV_LINK_SPEED_AUTO_NEGOTIATED       | \
                                                        NET_ADAPTER_LINK_STATE_DUPLEX_AUTO_NEGOTIATED               | \
                                                        NET_ADAPTER_LINK_STATE_PAUSE_FUNCTIONS_AUTO_NEGOTIATED)       \

#define NET_CONFIGURATION_QUERY_ULONG_SUPPORTED_FLAGS  (NET_CONFIGURATION_QUERY_ULONG_NO_FLAGS                      | \
                                                        NET_CONFIGURATION_QUERY_ULONG_MAY_BE_STORED_AS_HEX_STRING)    \

//
// This macro is used to check if an input flag mask contains only allowed flag values, as defined by _supported
//
#define VERIFIER_CHECK_FLAGS(_flags, _supported) (((_flags) & ~(_supported)) == 0)

//
// NetAdapterCx failure codes
//
typedef enum _FailureCode : ULONG_PTR
{
    FailureCode_CorruptedPrivateGlobals = 0,
    FailureCode_IrqlIsNotPassive,
    FailureCode_IrqlNotLessOrEqualDispatch,
    FailureCode_EvtSetCapabilitiesNotInProgress,
    FailureCode_EvtArmDisarmWakeNotInProgress,
    FailureCode_CompletingNetRequestWithPendingStatus,
    FailureCode_InvalidNetRequestType,
    FailureCode_DefaultRequestQueueAlreadyExists,
    FailureCode_InvalidStructTypeSize,
    FailureCode_InvalidQueueConfiguration,
    FailureCode_InvalidPowerCapabilities,
    FailureCode_MacAddressLengthTooLong,
    FailureCode_InvalidLinkLayerCapabilities,
    FailureCode_InvalidLinkState,
    FailureCode_ObjectIsNotCancelable,
    FailureCode_ParameterCantBeNull,
    FailureCode_InvalidQueryUlongFlag,
    FailureCode_QueryNetworkAddressInvalidParameter,
    FailureCode_QueueConfigurationHasError,
    FailureCode_InvalidRequestQueueType,
    FailureCode_NetPacketContextTypeMismatch,
    FailureCode_NetPacketDoesNotHaveContext,
    FailureCode_MtuMustBeGreaterThanZero,
    FailureCode_BadQueueInitContext,
    FailureCode_CreatingNetQueueFromWrongThread,
    FailureCode_InvalidDatapathCapabilities,
    FailureCode_NetQueueInvalidConfiguration,
    FailureCode_ParentObjectNotNull,
    FailureCode_InvalidNetAdapterConfig,
    FailureCode_QueueAlreadyCreated,
    FailureCode_ObjectAttributesContextSizeTooLarge,
    FailureCode_IllegalObjectAttributes,
} FailureCode;

//
// Verifier_ReportViolation uses a value from this enum to decide what to do in case of a violation
//
// Verifier_Verify* functions that use only VerifierAction_BugcheckAlways should not return any value.
// Verifier_Verify* functions that use VerifierAction_DbgBreakIfDebuggerPresent at least one time should return NTSTATUS, and the caller should deal with the status
//
typedef enum _VerifierAction
{
    VerifierAction_BugcheckAlways,
    VerifierAction_DbgBreakIfDebuggerPresent
} VerifierAction;

VOID
NetAdapterCxBugCheck(
    _In_ PNX_PRIVATE_GLOBALS PrivateGlobals,
    _In_ FailureCode         FailureCode,
    _In_ ULONG_PTR           Parameter2,
    _In_ ULONG_PTR           Parameter3
    );

VOID
Verifier_ReportViolation(
    _In_ PNX_PRIVATE_GLOBALS PrivateGlobals,
    _In_ VerifierAction      Action,
    _In_ FailureCode         FailureCode,
    _In_ ULONG_PTR           Parameter2,
    _In_ ULONG_PTR           Parameter3
    );

VOID
FORCEINLINE
Verifier_VerifyPrivateGlobals(
    PNX_PRIVATE_GLOBALS PrivateGlobals
    )
{
    if (PrivateGlobals->Signature != NX_PRIVATE_GLOBALS_SIG)
    {
        Verifier_ReportViolation(
            PrivateGlobals,
            VerifierAction_BugcheckAlways,
            FailureCode_CorruptedPrivateGlobals,
            0,
            0);
    }
}

VOID
Verifier_VerifyIrqlPassive(
    _In_ PNX_PRIVATE_GLOBALS PrivateGlobals
    );

VOID
Verifier_VerifyIrqlLessThanOrEqualDispatch(
    _In_ PNX_PRIVATE_GLOBALS PrivateGlobals
    );

VOID
Verifier_VerifyEvtAdapterSetCapabilitiesInProgress(
    _In_ PNX_PRIVATE_GLOBALS PrivateGlobals,
    _In_ PNxAdapter          pNxAdapter
    );

VOID
Verifier_VerifyNetPowerSettingsAccessible(
    _In_ PNX_PRIVATE_GLOBALS PrivateGlobals,
    _In_ PNxWake             NetWake
    );

VOID
Verifier_VerifyObjectSupportsCancellation(
    _In_ PNX_PRIVATE_GLOBALS PrivateGlobals,
    _In_ WDFOBJECT Object
    );

VOID
Verifier_VerifyNetRequestCompletionStatusNotPending(
    _In_ PNX_PRIVATE_GLOBALS PrivateGlobals,
    _In_ NETREQUEST NetRequest,
    _In_ NTSTATUS   CompletionStatus
    );

VOID
Verifier_VerifyNetRequestType(
    _In_ PNX_PRIVATE_GLOBALS PrivateGlobals,
    _In_ PNxRequest NxRequest,
    _In_ NDIS_REQUEST_TYPE Type
    );

VOID
Verifier_VerifyNetRequestIsQuery(
    _In_ PNX_PRIVATE_GLOBALS PrivateGlobals,
    _In_ PNxRequest NxRequest
    );

VOID
Verifier_VerifyNetRequest(
    _In_ PNX_PRIVATE_GLOBALS PrivateGlobals,
    _In_ PNxRequest          pNxRequest
    );

template <typename T>
VOID
Verifier_VerifyTypeSize(
    _In_ PNX_PRIVATE_GLOBALS PrivateGlobals,
    _In_ T *Input)
{
    ULONG uInputSize = Input->Size;
    ULONG uExpectedSize = sizeof(T);

    if (uInputSize != uExpectedSize)
    {
        Verifier_ReportViolation(
            PrivateGlobals,
            VerifierAction_BugcheckAlways,
            FailureCode_InvalidStructTypeSize,
            uInputSize,
            uExpectedSize);
    }
}

VOID
Verifier_VerifyNotNull(
    _In_ PNX_PRIVATE_GLOBALS PrivateGlobals,
    _In_ PVOID Ptr
    );

NTSTATUS
Verifier_VerifyQueueConfiguration(
    _In_ PNX_PRIVATE_GLOBALS PrivateGlobals,
    _In_ PNET_REQUEST_QUEUE_CONFIG QueueConfig
    );

VOID
Verifier_VerifyPowerCapabilities(
    _In_ PNX_PRIVATE_GLOBALS PrivateGlobals,
    _In_ PNET_ADAPTER_POWER_CAPABILITIES PowerCapabilities,
    _In_ BOOLEAN SetAttributesInProgress,
    _In_ PNET_ADAPTER_POWER_CAPABILITIES PreviouslyReportedCapabilities
    );

VOID
Verifier_VerifyLinkLayerCapabilities(
    _In_ PNX_PRIVATE_GLOBALS PrivateGlobals,
    _In_ PNET_ADAPTER_LINK_LAYER_CAPABILITIES LinkLayerCapabilities
    );

VOID
Verifier_VerifyCurrentLinkState(
    _In_ PNX_PRIVATE_GLOBALS PrivateGlobals,
    _In_ PNET_ADAPTER_LINK_STATE LinkState
    );

VOID
Verifier_VerifyQueryAsUlongFlags(
    _In_ PNX_PRIVATE_GLOBALS PrivateGlobals,
    _In_ NET_CONFIGURATION_QUERY_ULONG_FLAGS Flags
    );

NTSTATUS
Verifier_VerifyQueryNetworkAddressParameters(
    _In_ PNX_PRIVATE_GLOBALS PrivateGlobals,
    _In_ ULONG BufferLength,
    _In_ PVOID NetworkAddressBuffer
    );

VOID
Verifier_VerifyNetPacketUniqueType(
    _In_ PNX_PRIVATE_GLOBALS PrivateGlobals,
    _In_ NET_PACKET* NetPacket,
    _In_ PCNET_CONTEXT_TYPE_INFO UniqueType
    );

VOID
Verifier_VerifyMtuSize(
    _In_ PNX_PRIVATE_GLOBALS PrivateGlobals,
         ULONG MtuSize
    );

VOID
Verifier_VerifyQueueInitContext(
    _In_ PNX_PRIVATE_GLOBALS PrivateGlobals,
    _In_ struct QUEUE_CREATION_CONTEXT *NetQueueInit
    );

VOID
Verifier_VerifyNetTxQueueConfiguration(
    _In_ PNX_PRIVATE_GLOBALS PrivateGlobals,
    _In_ PNET_TXQUEUE_CONFIG Configuration
    );

VOID
Verifier_VerifyNetRxQueueConfiguration(
    _In_ PNX_PRIVATE_GLOBALS PrivateGlobals,
    _In_ PNET_RXQUEUE_CONFIG Configuration
    );

VOID
Verifier_VerifyObjectAttributesParentIsNull(
    _In_ PNX_PRIVATE_GLOBALS PrivateGlobals,
    _In_ PWDF_OBJECT_ATTRIBUTES ObjectAttributes
    );

VOID
Verifier_VerifyObjectAttributesContextSize(
    _In_ PNX_PRIVATE_GLOBALS PrivateGlobals,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES ObjectAttributes,
    _In_ SIZE_T MaximumContextSize
    );

VOID
Verifier_VerifyDatapathCapabilities(
    _In_ PNX_PRIVATE_GLOBALS PrivateGlobals,
    _In_ PNET_ADAPTER_DATAPATH_CAPABILITIES DataPathCapabilities
    );

VOID
Verifier_VerifyNetAdapterConfig(
    _In_ PNX_PRIVATE_GLOBALS PrivateGlobals,
    _In_ PNET_ADAPTER_CONFIG AdapterConfig
    );

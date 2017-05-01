/*++
 
Copyright (C) Microsoft Corporation. All rights reserved.

Module Name:

    NxRequestQueue.cpp

Abstract:

    This is the main NetAdapterCx driver framework.





Environment:

    kernel mode only

Revision History:

--*/

#include "Nx.hpp"

// Tracing support
extern "C" {
#include "NxRequestQueue.tmh"
}

NxRequestQueue::NxRequestQueue(
    _In_ PNX_PRIVATE_GLOBALS       NxPrivateGlobals,
    _In_ NETREQUESTQUEUE           NetRequestQueue,
    _In_ PNxAdapter                NxAdapter,
    _In_ PNET_REQUEST_QUEUE_CONFIG Config
    ) : 
    CFxObject(NetRequestQueue), 
    m_NxPrivateGlobals(NxPrivateGlobals),
    m_NxAdapter(NxAdapter)
/*++
Routine Description: 
    Constructor for the NxRequestQueue object.  
--*/
{
    FuncEntry(FLAG_REQUEST_QUEUE);

    RtlCopyMemory(&m_Config, Config, Config->Size);

    //
    // All the handlers are backed by WDF memory and are parented to the 
    // NetAdapter object. In the event that a NetRequestQueue is not deleted
    // explicilty both the handlers and the queue will be disposed when the 
    // NetAdapter is getting deleted. Thus the handlers might get disposed
    // prior to the queue. 
    // 
    // The queue's d'tor tries to delete the handlers explicitly. Thus we need 
    // to acquire a reference on all handlers so that it is safe to touch them
    // from the Queue's D'tor. 
    //
    ReferenceHandlers();

    KeInitializeSpinLock(&m_RequestsListLock);

    InitializeListHead(&m_RequestsListHead);

    FuncExit(FLAG_REQUEST_QUEUE);
}

NxRequestQueue::~NxRequestQueue()
/*++
Routine Description: 
    D'tor for the NxRequestQueue object.  
--*/
{
    FuncEntry(FLAG_REQUEST_QUEUE);

    _FreeHandlers(&m_Config);

    FuncExit(FLAG_REQUEST_QUEUE);
}

#define HANDLER_REF_TAG ((PVOID)(ULONG_PTR)('rldH'))

VOID
NxRequestQueue::ReferenceHandlers(
    VOID
    )
/*++
Routine Description: 
    This routine references all the custom handlers.
 
--*/
{

    PNET_REQUEST_QUEUE_SET_DATA_HANDLER setDataHandler;
    PNET_REQUEST_QUEUE_QUERY_DATA_HANDLER queryDataHandler;
    PNET_REQUEST_QUEUE_METHOD_HANDLER methodHandler;

    setDataHandler = m_Config.SetDataHandlers;
    while (setDataHandler != NULL) {
        WdfObjectReferenceWithTag(setDataHandler->Memory, HANDLER_REF_TAG);
        setDataHandler = setDataHandler->Next;
    }

    queryDataHandler = m_Config.QueryDataHandlers;
    while (queryDataHandler != NULL) {
        WdfObjectReferenceWithTag(queryDataHandler->Memory, HANDLER_REF_TAG);
        queryDataHandler = queryDataHandler->Next;
    }

    methodHandler = m_Config.MethodHandlers;
    while (methodHandler != NULL) {
        WdfObjectReferenceWithTag(methodHandler->Memory, HANDLER_REF_TAG);
        methodHandler = methodHandler->Next;
    }
}

VOID
NxRequestQueue::_FreeHandlers(
    _In_ PNET_REQUEST_QUEUE_CONFIG QueueConfig
    )
/*++
Routine Description: 
    Static method
    
    This routine frees the memory that was allocated to
    add a hander for the client.
 
    The memory is allocated by the NET_REQUEST_QUEUE_CONFIG_ADD_xxx_HANDLER APIs. 
 
Arguments: 
    QueueConfig - The pointer to the NET_REQUEST_QUEUE_CONFIG structure for which
    we need to free the Handlers
 
--*/
{

    PNET_REQUEST_QUEUE_SET_DATA_HANDLER setDataHandler, nextSetDataHandler;
    PNET_REQUEST_QUEUE_QUERY_DATA_HANDLER queryDataHandler, nextQueryDataHandler;
    PNET_REQUEST_QUEUE_METHOD_HANDLER methodHandler, nextMethodHandler;

    setDataHandler = QueueConfig->SetDataHandlers;
    while (setDataHandler != NULL) {
        nextSetDataHandler = setDataHandler->Next;
        WdfObjectDelete(setDataHandler->Memory);
        WdfObjectDereferenceWithTag(setDataHandler->Memory, HANDLER_REF_TAG);
        setDataHandler = nextSetDataHandler;
    }
    QueueConfig->SetDataHandlers = NULL;

    queryDataHandler = QueueConfig->QueryDataHandlers;
    while (queryDataHandler != NULL) {
        nextQueryDataHandler = queryDataHandler->Next;
        WdfObjectDelete(queryDataHandler->Memory);
        WdfObjectDereferenceWithTag(queryDataHandler->Memory, HANDLER_REF_TAG);
        queryDataHandler = nextQueryDataHandler;
    }
    QueueConfig->QueryDataHandlers = NULL;

    methodHandler = QueueConfig->MethodHandlers;
    while (methodHandler != NULL) {
        nextMethodHandler = methodHandler->Next;
        WdfObjectDelete(methodHandler->Memory);
        WdfObjectDereferenceWithTag(methodHandler->Memory, HANDLER_REF_TAG);
        methodHandler = nextMethodHandler;
    }
    QueueConfig->MethodHandlers = NULL;
}

NTSTATUS
NxRequestQueue::_Create(
    _In_     PNX_PRIVATE_GLOBALS       PrivateGlobals,
    _In_     PNxAdapter                NxAdapter,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES    ClientAttributes,
    _In_     PNET_REQUEST_QUEUE_CONFIG Config,
    _Out_    PNxRequestQueue*          Queue
)
/*++
Routine Description: 
    Static method that creates the NETREQUESTQUEUE object.
 
    This is the internal implementation of the NetRequestQueueCreate public API.
 
    Please refer to the NetAdapterRequestQueueCreate API for more description on this
    function and the arguments.
 
Arguments: 
    NxAdapter - Pointer to the NxAdapter for which the queue is being
        created.
 
    ClientAttributes - optional - Pointer to a WDF_OBJECT_ATTRIBUTES allocated
        and initialized by the caller for the request queue being created.
 
    Config - Pointer to the NET_REQUEST_QUEUE_CONFIG strcuture allocated and initialized
        by the caller.
 
    Queue - Ouput - Address of a location that recieves the pointer to the
        created NxRequestQueue object
 
Remarks: 
    Currently for a NxAdapter only 2 request queues (default and direct default)
    maybe created. 
 
--*/
{
    FuncEntry(FLAG_REQUEST_QUEUE);
    NTSTATUS              status;
    WDF_OBJECT_ATTRIBUTES attributes;
    NETREQUESTQUEUE       netRequestQueue;
    PNxRequestQueue       nxRequestQueue;
    PVOID                 nxRequestQueueMemory;

    //
    // Create a WDFOBJECT for the NxRequestQueue
    //

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, NxRequestQueue);
    attributes.ParentObject = NxAdapter->GetFxObject();

    //
    // Ensure that the destructor would be called when this object is destroyed.
    //
    NxRequestQueue::_SetObjectAttributes(&attributes);
    
    status = WdfObjectCreate(&attributes, (WDFOBJECT*)&netRequestQueue);
    if (!NT_SUCCESS(status)) {
        LogError(NxAdapter->GetRecorderLog(), FLAG_REQUEST_QUEUE,
                 "WdfObjectCreate for NetRequestQueue failed %!STATUS!", status);
        FuncExit(FLAG_REQUEST_QUEUE);
        return status;
    }

    //
    // Since we just created the netRequestQueue, the NxRequestQueue object has 
    // yet not been constructed. Get the NxRequestQueue's memory. 
    //
    nxRequestQueueMemory = (PVOID) GetNxRequestQueueFromHandle(netRequestQueue);

    //
    // Use the inplacement new and invoke the constructor on the 
    // NxRequestQueue's memory
    //
    nxRequestQueue = new (nxRequestQueueMemory) NxRequestQueue(PrivateGlobals,
                                                               netRequestQueue,
                                                               NxAdapter,
                                                               Config);

    __analysis_assume(nxRequestQueue != NULL);

    NT_ASSERT(nxRequestQueue);

    //
    // Now ~NxRequestQueue will free Handlers.
    // To ensure that we dont accidently try to free the handler 
    // memory twice, clear those pointers from the Config structure
    //
    Config->SetDataHandlers = NULL;
    Config->QueryDataHandlers = NULL;
    Config->MethodHandlers = NULL;

    if (ClientAttributes != WDF_NO_OBJECT_ATTRIBUTES) {
        status = WdfObjectAllocateContext(netRequestQueue, ClientAttributes, NULL);
        if (!NT_SUCCESS(status)) {
            LogError(nxRequestQueue->GetRecorderLog(), FLAG_REQUEST_QUEUE,
                     "WdfObjectAllocateContext for ClientAttributes failed %!STATUS!", status);
            WdfObjectDelete(netRequestQueue);
            FuncExit(FLAG_REQUEST_QUEUE);
            return status;
        }
    }

    //
    // Dont Fail after this point or else the client's Cleanup / Destroy
    // callbacks can get called. Also since we set the 
    // NxAdapter->m_DefaultRequestQueue, NxAdapter->m_DefaultDirectRequestQueue 
    // below, and for now they can only be set once. 
    //

    WdfObjectReferenceWithTag(netRequestQueue, (PVOID)NxAdapter::_EvtCleanup);

    //
    // Store the nxRequestQueue pointer in NxAdapter
    //
    switch (Config->Type) {
    case NetRequestQueueDefaultSequential:
        NxAdapter->m_DefaultRequestQueue = nxRequestQueue;
        break;

    case NetRequestQueueDefaultParallel: 
        NxAdapter->m_DefaultDirectRequestQueue = nxRequestQueue;
        break;

    default:
        NT_ASSERTMSG("UnExpected Type. We already should have validated the type", FALSE);
    }

    *Queue = nxRequestQueue;
    
    FuncExit(FLAG_REQUEST_QUEUE);
    return status;
}

/*++
Macro Description: 
    This Macro scans a singly Linked list of
    handlers, for a one that matches a given request 
 
Arguments: 
    Type - One of the handlers this macro supports 
        NET_REQUEST_QUEUE_SET_DATA_HANDLER
        NET_REQUEST_QUEUE_QUERY_DATA_HANDLER
        NET_REQUEST_QUEUE_METHOD_HANDLER
 
    First - Pointer to the first entry in the handler list. It Maybe NULL.
 
    Oid - The NDIS_OID Id for which the handler is being searched.
 
    InputBufferLength - The minimum required input buffer length. If a handler
        is found with matching Oid, but insufficient InputBufferLength
        this macro returns a failure.
 
    OutputBufferLength - The minimum required Output buffer length. If a handler
        is found with matching Oid, but insufficient OutputBufferLength
        this macro returns a failure.
 
    Status - Address of a location that takes in the resulting status from this
        macro
 
    Handler - Address that accepts a pointer to the hanlder if the search is
        successful. 
--*/
#define FIND_REQUEST_HANDLER(                                                  \
    _Type,                                                                     \
    _First,                                                                    \
    _Oid,                                                                      \
    _InputBufferLength,                                                        \
    _OutputBufferLength,                                                       \
    _Status,                                                                   \
    _Handler)                                                                  \
{                                                                              \
    _Type * entry = _First;                                                    \
    *(_Status) = STATUS_NOT_FOUND;                                             \
    *(_Handler) = NULL;                                                        \
                                                                               \
    while (entry != NULL) {                                                    \
                                                                               \
        if (entry->Oid == (_Oid)) {                                            \
            if (entry->MinimumInputLength > (_InputBufferLength)) {            \
                *(_Status) = STATUS_BUFFER_TOO_SMALL;                          \
            } else if (entry->MinimumOutputLength > (_OutputBufferLength)) {   \
                *(_Status) = STATUS_BUFFER_TOO_SMALL;                          \
            } else {                                                           \
                *(_Handler) = entry;                                           \
                *(_Status) = STATUS_SUCCESS;                                   \
            }                                                                  \
            break;                                                             \
        }                                                                      \
        entry = entry->Next;                                                   \
    }                                                                          \
}

VOID
NxRequestQueue::DispatchRequest(
    _In_ PNxRequest NxRequest
    )
/*++
Routine Description: 
    This routine dispatches a request to the client.
 
Arguments: 
    NxRequest - The request to be dipatched
 
Remarks: 
    This routine first tries to find a maching handler for the input
    NxRequest.
 
    If one is not found, then it tries to dispatch the request using one of the 
    EvtRequestDefaultSetData/EvtRequestDefaultQueryData/EvtRequestDefaultMethod
    callbacks, assuming the client registered for those.
 
    Lastly it tries to dispatch the request to the client using the EvtDefaultRequest.
 
    If no handler is found for the request (based on the aforementioned steps), this
    routine fails the request using STATUS_NOT_SUPPORTED. 
--*/
{
    PNET_REQUEST_QUEUE_SET_DATA_HANDLER setDataHandler;
    PNET_REQUEST_QUEUE_QUERY_DATA_HANDLER queryDataHandler;
    PNET_REQUEST_QUEUE_METHOD_HANDLER methodHandler;
    NTSTATUS status;

    FuncEntry(FLAG_REQUEST_QUEUE);

    switch (NxRequest->m_NdisOidRequest->RequestType) {
    case NdisRequestSetInformation:

        //
        // First Try to find a handler for this request
        //
        //
        FIND_REQUEST_HANDLER(NET_REQUEST_QUEUE_SET_DATA_HANDLER, 
                             m_Config.SetDataHandlers, 
                             NxRequest->m_Oid,
                             NxRequest->m_InputBufferLength,
                             NxRequest->m_OutputBufferLength,
                             &status,
                             &setDataHandler);

        if (NT_SUCCESS(status)) {
            //
            // Found a handler, call it.
            //
            setDataHandler->EvtRequestSetData(GetFxObject(), 
                                              NxRequest->GetFxObject(),
                                              NxRequest->m_InputOutputBuffer,
                                              NxRequest->m_InputBufferLength);
            goto Exit;
        } else if (status == STATUS_NOT_FOUND) {
            //
            // There was no handler found for this request, check if the 
            // client registered for a EvtRequestDefaultSetData callback. 
            // If yes, then call that particular callback. 
            //
            if (m_Config.EvtRequestDefaultSetData != NULL) {
                m_Config.EvtRequestDefaultSetData(GetFxObject(), 
                                                  NxRequest->GetFxObject(),
                                                  NxRequest->m_Oid,
                                                  NxRequest->m_InputOutputBuffer,
                                                  NxRequest->m_InputBufferLength);
                goto Exit;
            }
        } else if (!NT_SUCCESS(status)) {
            //
            // There was an error (other than STATUS_NOT_FOUND) while trying 
            // to find the handler for the request. Most likely the error is
            // that the input buffers are smaller than that the client
            // is expecting. Fail this request. 
            //
            LogError(GetRecorderLog(), FLAG_REQUEST_QUEUE,         
                     "Oid %!NDIS_OID!, Failed %!STATUS!",                 
                     NxRequest->m_Oid, status);
            NxRequest->Complete(status);
            goto Exit;
        } 
        break;

    case NdisRequestQueryInformation:
    case NdisRequestQueryStatistics:

        //
        // First Try to find a handler for this request
        //
        FIND_REQUEST_HANDLER(NET_REQUEST_QUEUE_QUERY_DATA_HANDLER, 
                             m_Config.QueryDataHandlers, 
                             NxRequest->m_Oid,
                             NxRequest->m_InputBufferLength,
                             NxRequest->m_OutputBufferLength,
                             &status,
                             &queryDataHandler);

        if (NT_SUCCESS(status)) {
            //
            // Found a handler, call it. 
            //
            queryDataHandler->EvtRequestQueryData(GetFxObject(), 
                                                  NxRequest->GetFxObject(),
                                                  NxRequest->m_InputOutputBuffer,
                                                  NxRequest->m_OutputBufferLength);
            goto Exit;
        } else if (status == STATUS_NOT_FOUND) {
            //
            // There was no handler found for this request, check if the 
            // client registered for a EvtRequestDefaultQueryData callback. 
            // If yes, then call that particular callback. 
            //
            if (m_Config.EvtRequestDefaultQueryData != NULL) {
                m_Config.EvtRequestDefaultQueryData(GetFxObject(), 
                                                    NxRequest->GetFxObject(),
                                                    NxRequest->m_Oid,
                                                    NxRequest->m_InputOutputBuffer,
                                                    NxRequest->m_OutputBufferLength);
                goto Exit;
            }
        } else if (!NT_SUCCESS(status)) {
            //
            // There was an error (other than STATUS_NOT_FOUND) while trying 
            // to find the handler for the request. Most likely the error is
            // that the input buffers are smaller than that the client
            // is expecting. Fail this request. 
            //
            LogError(GetRecorderLog(), FLAG_REQUEST_QUEUE,         
                     "Oid %!NDIS_OID!, Failed %!STATUS!",                 
                     NxRequest->m_Oid, status);
            NxRequest->Complete(status);
            goto Exit;
        } 
        break;

    case NdisRequestMethod:

        //
        // First Try to find a handler for this request
        //
        FIND_REQUEST_HANDLER(NET_REQUEST_QUEUE_METHOD_HANDLER, 
                             m_Config.MethodHandlers, 
                             NxRequest->m_Oid,
                             NxRequest->m_InputBufferLength,
                             NxRequest->m_OutputBufferLength,
                             &status,
                             &methodHandler);

        if (NT_SUCCESS(status)) {
            //
            // Found a handler, call it. 
            //
            methodHandler->EvtRequestMethod(GetFxObject(), 
                                            NxRequest->GetFxObject(),
                                            NxRequest->m_InputOutputBuffer,
                                            NxRequest->m_InputBufferLength,
                                            NxRequest->m_OutputBufferLength);
            goto Exit;
        } else if (status == STATUS_NOT_FOUND) {
            //
            // There was no handler found for this request, check if the 
            // client registered for a EvtOidMethod callback. If yes, then call 
            // that particular callback. 
            //
            if (m_Config.EvtRequestDefaultMethod != NULL) {
                m_Config.EvtRequestDefaultMethod(GetFxObject(), 
                                                 NxRequest->GetFxObject(),
                                                 NxRequest->m_Oid,
                                                 NxRequest->m_InputOutputBuffer,
                                                 NxRequest->m_InputBufferLength,
                                                 NxRequest->m_OutputBufferLength);
                goto Exit;
            }
        } else if (!NT_SUCCESS(status)) {
            //
            // There was an error (other than STATUS_NOT_FOUND) while trying 
            // to find the handler for the request. Most likely the error is
            // that the input buffers are smaller than that the client
            // is expecting. Fail this request. 
            //
            LogError(GetRecorderLog(), FLAG_REQUEST_QUEUE,         
                     "Oid %!NDIS_OID!, Failed %!STATUS!",                 
                     NxRequest->m_Oid, status);
            NxRequest->Complete(status);
            goto Exit;
        } 

        break;

    default: 

        LogError(GetRecorderLog(), FLAG_REQUEST_QUEUE,     
                 "NetRequest 0x%p Type %!NDIS_REQUEST_TYPE!, STATUS_NOT_SUPPORTED", 
                 NxRequest->GetFxObject(), NxRequest->m_NdisOidRequest->RequestType);
        NxRequest->Complete(STATUS_NOT_SUPPORTED);
        goto Exit;
    }

    //
    // So far for this request we have not found any appropiate handler. 
    // If the client registered for the EvtRequestDefault handler call it, 
    // else fail the request.
    //
    if (m_Config.EvtRequestDefault == NULL) {

        LogError(GetRecorderLog(), FLAG_REQUEST_QUEUE,     
                 "NetRequest 0x%p, Id %!NDIS_OID!, Type %!NDIS_REQUEST_TYPE!, STATUS_NOT_SUPPORTED", 
                 NxRequest->GetFxObject(), NxRequest->m_Oid, NxRequest->m_NdisOidRequest->RequestType);
        NxRequest->Complete(STATUS_NOT_SUPPORTED);
        goto Exit;

    } 
    
    m_Config.EvtRequestDefault(GetFxObject(),
                               NxRequest->GetFxObject(),
                               NxRequest->m_NdisOidRequest->RequestType,
                               NxRequest->m_Oid,
                               NxRequest->m_InputOutputBuffer,
                               NxRequest->m_InputBufferLength,
                               NxRequest->m_OutputBufferLength);
    
Exit:
    FuncExit(FLAG_REQUEST_QUEUE);
    return;
}

VOID
NxRequestQueue::QueueNdisOidRequest(
    _In_ PNDIS_OID_REQUEST NdisOidRequest
    )
/*++
Routine Description: 
    This routine queues a NIDS_OID_REQUEST recieved from NDIS.sys to the
    queue. 
 
Arguments: 
    NdisOidRequest - Pointer to the traditional NDIS_OID_REQUEST structure.
 
Remarks: 
    This routine first creates a NxRequest wrapper object around the the input
    NDIS_OID_REQUEST and then queues it to the 'this' NxRequestQueue
 
--*/
{
    NTSTATUS status;
    PNxRequest nxRequest;
    KIRQL irql;

    FuncEntry(FLAG_REQUEST_QUEUE);

    //
    // Create the NxRequest object from the tranditional NDIS_OID_REQUEST
    //
    status = NxRequest::_Create(m_NxPrivateGlobals,
                                m_NxAdapter,
                                NdisOidRequest,
                                &nxRequest);

    if (!NT_SUCCESS(status)) {

        //
        // _Create failed, so fail the NDIS request.
        //
        NdisMOidRequestComplete(m_NxAdapter->m_NdisAdapterHandle,
                                NdisOidRequest, 
                                NdisConvertNtStatusToNdisStatus(status));
        FuncExit(FLAG_REQUEST_QUEUE);
        return;
    }

    //
    // Add the NxRequest to a Queue level list. This list may be leveraged in 
    // the following situations: 
    //  * cancellation
    //  * power transitions
    //
    KeAcquireSpinLock(&m_RequestsListLock, &irql);
    InsertTailList(&m_RequestsListHead, &nxRequest->m_QueueListEntry);
    KeReleaseSpinLock(&m_RequestsListLock, irql);

    nxRequest->m_NxQueue = this;

    //
    // For now we leverage the NDIS functionality that already serializes the
    // request anyway for us.  
    //
    DispatchRequest(nxRequest);
    FuncExit(FLAG_REQUEST_QUEUE);

}

VOID
NxRequestQueue::DisconnectRequest(
    _In_ PNxRequest NxRequest
    )
/*++
Routine Description: 
    This routine disassoicates an request from a Queue. This routine is called prior
    to completing an request. 
 
Arguments: 
    NxRequest - The NxRequest that is being dequeued from the queue. 
 
--*/
{
    KIRQL irql;

    FuncEntry(FLAG_REQUEST_QUEUE);

    KeAcquireSpinLock(&m_RequestsListLock, &irql);

    NT_ASSERT(NxRequest->m_QueueListEntry.Flink != NULL);
    NT_ASSERT(NxRequest->m_QueueListEntry.Blink != NULL);

    RemoveEntryList(&NxRequest->m_QueueListEntry);

    KeReleaseSpinLock(&m_RequestsListLock, irql);

    InitializeListEntry(&NxRequest->m_QueueListEntry);

    NxRequest->m_NxQueue = NULL;
    
    FuncExit(FLAG_REQUEST_QUEUE);
}

#define CANCEL_REQUEST_TAG ((PVOID)(ULONG_PTR)('CdiO'))
VOID
NxRequestQueue::CancelRequests(
    _In_ PVOID RequestId
    )
/*++
Routine Description: 
    This routine cancels all the requests with a matching RequestId.
 
Arguments: 
    RequestId - Any requests with matching with the RequestId field must be
        canceled.
 
--*/
{
    PNxRequest currNxRequest, nextNxRequest;
    KIRQL irql;
    LIST_ENTRY tmpCancelList;

    FuncEntry(FLAG_REQUEST_QUEUE);

    InitializeListHead(&tmpCancelList);

    KeAcquireSpinLock(&m_RequestsListLock, &irql);

    //
    // Loop through each entry request associated with the queue and 
    // see if it needs to be canceled.
    //
    FOR_ALL_IN_LIST(NxRequest, &m_RequestsListHead, m_QueueListEntry, currNxRequest) {

        if (currNxRequest->m_NdisOidRequest->RequestId != RequestId) {
            //
            // Not a matching request
            //
            continue;
        }

        if (currNxRequest->m_CancellationStarted) {
            //
            // Though we found a maching Ndis Oid Request, it has already been
            // canceled, or being canceled on a different thread
            //
            continue;
        }

        //
        // Found a request that we need to cancel. Since we would be 
        // touching the request outside of a lock, add a reference to it. 
        // ALso add the request to temporary cancel list. 
        //
        currNxRequest->m_CancellationStarted = TRUE;

        WdfObjectReferenceWithTag(currNxRequest->GetFxObject(), CANCEL_REQUEST_TAG);
        
        InsertTailList(&tmpCancelList, &currNxRequest->m_CancelTempListEntry);

    }

    KeReleaseSpinLock(&m_RequestsListLock, irql);

    //
    // Loop through all the requests that need to be cancelled and cancel them
    //
    FOR_ALL_IN_LIST_SAFE(NxRequest, 
                         &tmpCancelList, 
                         m_CancelTempListEntry,
                         currNxRequest,
                         nextNxRequest) {
        currNxRequest->Cancel();
        RemoveEntryList(&currNxRequest->m_CancelTempListEntry);
        InitializeListEntry(&currNxRequest->m_CancelTempListEntry);

        WdfObjectDereferenceWithTag(currNxRequest->GetFxObject(), CANCEL_REQUEST_TAG);

    }

    FuncExit(FLAG_REQUEST_QUEUE);
}

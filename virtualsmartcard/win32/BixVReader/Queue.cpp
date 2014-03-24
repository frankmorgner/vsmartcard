#include "internal.h"
#include "Device.h"
#include "Queue.h"
#include "SectionLocker.h"

/////////////////////////////////////////////////////////////////////////
//
// CMyQueue::CMyQueue
//
// Object constructor function
//
// Initialize member variables
//
/////////////////////////////////////////////////////////////////////////
CMyQueue::CMyQueue() :
    m_pParentDevice(NULL)
{
}

/////////////////////////////////////////////////////////////////////////
//
// CMyQueue::~CMyQueue
//
// Object destructor function
//
//
/////////////////////////////////////////////////////////////////////////
CMyQueue::~CMyQueue()
{
    // Release the reference that was incremented in CreateInstance()
    SAFE_RELEASE(m_pParentDevice);   
}

/////////////////////////////////////////////////////////////////////////
//
// CMyQueue::CreateInstance
//
// This function supports the COM factory creation system
//
// Parameters:
//      parentDevice    - A pointer to the CWSSDevice object
//      ppUkwn          - pointer to a pointer to the queue to be returned
//
// Return Values:
//      S_OK: The queue was created successfully
//
/////////////////////////////////////////////////////////////////////////
HRESULT CMyQueue::CreateInstance(__in IWDFDevice*  pWdfDevice, CMyDevice* pMyDevice)
{
	inFunc
	SectionLogger a(__FUNCTION__);
    CComObject<CMyQueue>* pMyQueue = NULL;

    if(NULL == pMyDevice)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = CComObject<CMyQueue>::CreateInstance(&pMyQueue);

    if(SUCCEEDED(hr))
    {
        // AddRef the object
        pMyQueue->AddRef();

        // Store the parent device object
        pMyQueue->m_pParentDevice = pMyDevice;

        // Increment the reference for the lifetime of the CMyQueue object.
        pMyQueue->m_pParentDevice->AddRef();

        CComPtr<IUnknown> spIUnknown;
        hr = pMyQueue->QueryInterface(IID_IUnknown, (void**)&spIUnknown);

        if(SUCCEEDED(hr))
        {
            // Create the framework queue
            CComPtr<IWDFIoQueue> spDefaultQueue;
            hr = pWdfDevice->CreateIoQueue( spIUnknown,
                                            TRUE,                        // DefaultQueue
                                            WdfIoQueueDispatchParallel,  // Parallel queue handling 
                                            FALSE,                       // PowerManaged
                                            TRUE,                        // AllowZeroLengthRequests
                                            &spDefaultQueue 
                                            );
            if (FAILED(hr))
            {
				OutputDebugString (L"[BixVReader]IoQueue NOT Created\n");
            }
			else
				OutputDebugString (L"[BixVReader]IoQueue Created\n");

        }

        // Release the pMyQueue pointer when done. Note: UMDF holds a reference to it above
        SAFE_RELEASE(pMyQueue);
    }

    return hr;
}
/////////////////////////////////////////////////////////////////////////
//
// CMyQueue::OnDeviceIoControl
//
// This method is called when an IOCTL is sent to the device
//
// Parameters:
//      pQueue            - pointer to an IO queue
//      pRequest          - pointer to an IO request
//      ControlCode       - The IOCTL to process
//      InputBufferSizeInBytes - the size of the input buffer
//      OutputBufferSizeInBytes - the size of the output buffer
//
/////////////////////////////////////////////////////////////////////////
STDMETHODIMP_ (void) CMyQueue::OnDeviceIoControl(
    __in IWDFIoQueue*     pQueue,
    __in IWDFIoRequest*   pRequest,
    __in ULONG            ControlCode,
         SIZE_T           InputBufferSizeInBytes,
         SIZE_T           OutputBufferSizeInBytes
    )
{
	m_pParentDevice->ProcessIoControl(pQueue,pRequest,ControlCode,InputBufferSizeInBytes,OutputBufferSizeInBytes);    
}

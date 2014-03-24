#pragma once

#include <vector>
#include "reader.h"

//
// Class for the iotrace driver.
//
class CMyDevice;


class ATL_NO_VTABLE CMyDevice :
    public CComObjectRootEx<CComMultiThreadModel>,
    public IPnpCallbackHardware,
	public IPnpCallback,
	public IRequestCallbackCancel
{
public:
	~CMyDevice() {
        DeleteCriticalSection(&m_RequestLock);
	}

    DECLARE_NOT_AGGREGATABLE(CMyDevice)

    BEGIN_COM_MAP(CMyDevice)
        COM_INTERFACE_ENTRY(IPnpCallbackHardware)
        COM_INTERFACE_ENTRY(IPnpCallback)
		COM_INTERFACE_ENTRY(IRequestCallbackCancel)
    END_COM_MAP()

    CMyDevice(VOID)
    {
        InitializeCriticalSection(&m_RequestLock);
        m_pWdfDevice = NULL;
    }

	//
// Private data members.
//
private:

    CComPtr<IWDFDevice>     m_pWdfDevice;
	std::vector<Reader*> readers;
	int numInstances;

//
// Private methods.
//

private:


	HRESULT ConfigureQueue();

	//
// Public methods
//
public:

    CRITICAL_SECTION        m_RequestLock;

    //
    // The factory method used to create an instance of this driver.
    //
    
    static
    HRESULT CreateInstance(
        __in IWDFDriver *FxDriver,
        __in IWDFDeviceInitialize *FxDeviceInit
        );

//
// COM methods
//
public:

    //
    // IUnknown methods.
    //

	void  ProcessIoControl(__in IWDFIoQueue*     pQueue,
                                    __in IWDFIoRequest*   pRequest,
                                    __in ULONG            ControlCode,
                                         SIZE_T           InputBufferSizeInBytes,
                                         SIZE_T           OutputBufferSizeInBytes);

    STDMETHOD_ (HRESULT, OnPrepareHardware)(__in IWDFDevice* pWdfDevice);
    STDMETHOD_ (HRESULT, OnReleaseHardware)(__in IWDFDevice* pWdfDevice);
    STDMETHOD_ (HRESULT, OnD0Entry)(IN IWDFDevice*  pWdfDevice,IN WDF_POWER_DEVICE_STATE  previousState);
    STDMETHOD_ (HRESULT, OnD0Exit )(IN IWDFDevice*  pWdfDevice,IN WDF_POWER_DEVICE_STATE  newState);
    STDMETHOD_ (HRESULT, OnQueryRemove )(IN IWDFDevice*  pWdfDevice);
    STDMETHOD_ (HRESULT, OnQueryStop)(IN IWDFDevice*  pWdfDevice);
    STDMETHOD_ (void, OnSurpriseRemoval)(IN IWDFDevice*  pWdfDevice);
    STDMETHOD_ (void, OnCancel)(IN IWDFIoRequest*  pWdfRequest);

	void shutDown();
};


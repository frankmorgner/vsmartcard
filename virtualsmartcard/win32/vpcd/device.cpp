#include "vpcd.h"
#include "internal.h"
#include "VirtualSCReader.h"
#include "queue.h"
#include "device.h"
#include "driver.h"
#include <stdio.h>
#include <winscard.h>
#include "memory.h"
#include <Sddl.h>

HRESULT
CMyDevice::CreateInstance(
    __in IWDFDriver *FxDriver,
    __in IWDFDeviceInitialize * FxDeviceInit
    )
/*++
 
  Routine Description:

    This method creates and initializs an instance of the virtual smart card reader driver's 
    device callback object.

  Arguments:

    FxDeviceInit - the settings for the device.

    Device - a location to store the referenced pointer to the device object.

  Return Value:

    Status

--*/
{
	inFunc;
    CComObject<CMyDevice>* device = NULL;
    HRESULT hr;

    //
    // Allocate a new instance of the device class.
    //
	hr = CComObject<CMyDevice>::CreateInstance(&device);

    if (device==NULL)
    {
        return E_OUTOFMEMORY;
    }

    //
    // Initialize the instance.
    //
	device->AddRef();
    FxDeviceInit->SetLockingConstraint(WdfDeviceLevel);

    CComPtr<IUnknown> spCallback;
    hr = device->QueryInterface(IID_IUnknown, (void**)&spCallback);

    CComPtr<IWDFDevice> spIWDFDevice;
    if (SUCCEEDED(hr))
    {
        hr = FxDriver->CreateDevice(FxDeviceInit, spCallback, &spIWDFDevice);
    }

	if (spIWDFDevice->CreateDeviceInterface(&SmartCardReaderGuid,NULL)!=0)
		OutputDebugString(L"CreateDeviceInterface Failed\n");

	SAFE_RELEASE(device);

    return hr;
}

void CMyDevice::IoSmartCardIsPresent(IWDFIoRequest* pRequest,SIZE_T inBufSize,SIZE_T outBufSize) {
	UNREFERENCED_PARAMETER(inBufSize);
	UNREFERENCED_PARAMETER(outBufSize);
	inFunc;
	if (vicc_present(ctx) == 1)
		// there's a smart card present, so complete the request
		pRequest->CompleteWithInformation(STATUS_SUCCESS, 0);
	else {
		// there's no smart card present, so leave the request pending; it will be completed later
		waitInsertIpr=pRequest;
		IRequestCallbackCancel *callback;
		QueryInterface(__uuidof(IRequestCallbackCancel),(void**)&callback);
		pRequest->MarkCancelable(callback);
		callback->Release();
	}
}

void CMyDevice::IoSmartCardGetState(IWDFIoRequest* pRequest,SIZE_T inBufSize,SIZE_T outBufSize) {
	UNREFERENCED_PARAMETER(inBufSize);
	UNREFERENCED_PARAMETER(outBufSize);
	inFunc;
	if (vicc_present(ctx) == 1) {
		OutputDebugString(L"[GSTA]SCARD_SPECIFIC\n");
		completeWithInteger(pRequest, SCARD_SPECIFIC);
	} else {
		OutputDebugString(L"SCARD_ABSENT\n");
		completeWithInteger(pRequest, SCARD_ABSENT);
	}
}
void CMyDevice::IoSmartCardIsAbsent(IWDFIoRequest* pRequest,SIZE_T inBufSize,SIZE_T outBufSize) {
	UNREFERENCED_PARAMETER(inBufSize);
	UNREFERENCED_PARAMETER(outBufSize);
	inFunc;
	if (vicc_present(ctx) == 0)
		// there's no smart card present, so complete the request
		pRequest->CompleteWithInformation(STATUS_SUCCESS, 0);
	else {
		// there's a smart card present, so leave the request pending; it will be completed later
		waitRemoveIpr=pRequest;
		IRequestCallbackCancel *callback;

		QueryInterface(__uuidof(IRequestCallbackCancel),(void**)&callback);
		pRequest->MarkCancelable(callback);
		callback->Release();
	}
}

void CMyDevice::IoSmartCardPower(IWDFIoRequest* pRequest,SIZE_T inBufSize,SIZE_T outBufSize) {
	UNREFERENCED_PARAMETER(inBufSize);
	UNREFERENCED_PARAMETER(outBufSize);
	inFunc;
	DWORD code;
	if (!getInteger(pRequest, &code)) {
		OutputDebugString(L"error getting code\n");
		pRequest->CompleteWithInformation(STATUS_NO_MEDIA, 0);
		return;
	}
	switch (code) {
		case SCARD_COLD_RESET:
			OutputDebugString(L"[POWR]SCARD_COLD_RESET\n");
			vicc_poweroff(ctx);
			vicc_poweron(ctx);
			break;
		case SCARD_WARM_RESET:
			OutputDebugString(L"[POWR]SCARD_WARM_RESET\n");
			vicc_reset(ctx);
			break;
		case SCARD_POWER_DOWN:
			OutputDebugString(L"[POWR]SCARD_POWER_DOWN\n");
			vicc_poweroff(ctx);
	}

	if (code==SCARD_COLD_RESET || code==SCARD_WARM_RESET) {
		int ATRsize = vicc_getatr(ctx, &outdata);
		if (ATRsize <= 0) {
			pRequest->CompleteWithInformation(STATUS_NO_MEDIA, 0);					
			return;
		}
		outlength = ATRsize;
		completeWithBuffer(pRequest,outdata,outlength);
	} else {
		pRequest->CompleteWithInformation(STATUS_SUCCESS, 0);
	}
}

void CMyDevice::IoSmartCardSetAttribute(IWDFIoRequest* pRequest,SIZE_T inBufSize,SIZE_T outBufSize) {
	UNREFERENCED_PARAMETER(inBufSize);
	UNREFERENCED_PARAMETER(outBufSize);
	inFunc;
	
	IWDFMemory *inmem=NULL;
	pRequest->GetInputMemory(&inmem);
	
	SIZE_T size;
	BYTE *data=(BYTE *)inmem->GetDataBuffer(&size);

	DWORD minCode=*(DWORD*)(data);
	bool handled=false;
	if (minCode==SCARD_ATTR_DEVICE_IN_USE) {
		OutputDebugString(L"[SATT]SCARD_ATTR_DEVICE_IN_USE\n");
		pRequest->CompleteWithInformation(STATUS_SUCCESS, 0);
		handled=true;
	}
	inmem->Release();

	if (!handled) {
		OutputDebugString(L"[SATT]ERROR_NOT_SUPPORTED\n");
		pRequest->CompleteWithInformation(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED), 0);
	}
}

void CMyDevice::IoSmartCardTransmit(IWDFIoRequest* pRequest,SIZE_T inBufSize,SIZE_T outBufSize) {
	UNREFERENCED_PARAMETER(inBufSize);
	UNREFERENCED_PARAMETER(outBufSize);
	inFunc;
	if (!getBuffer(pRequest,&indata,&inlength)) {
        pRequest->CompleteWithInformation(STATUS_INVALID_DEVICE_STATE, 0);
		return;
	}
	if (((SCARD_IO_REQUEST*)indata)->dwProtocol!=SCARD_PROTOCOL_T1) {
        pRequest->CompleteWithInformation(STATUS_INVALID_DEVICE_STATE, 0);
		return;
	}
	int RespSize = vicc_transmit(ctx, inlength - sizeof(SCARD_IO_REQUEST), indata + sizeof(SCARD_IO_REQUEST), &outdata);
	if (RespSize < 0)
	{
		pRequest->CompleteWithInformation(STATUS_NO_MEDIA, 0);					
		return;
	}
	outlength = RespSize;
	BYTE *Resp = (BYTE *) malloc(outlength+sizeof(SCARD_IO_REQUEST));
	if (!Resp) {
        pRequest->CompleteWithInformation(STATUS_NO_MEDIA, 0);
		return;
	}
	((SCARD_IO_REQUEST*)Resp)->cbPciLength=sizeof(SCARD_IO_REQUEST);
	((SCARD_IO_REQUEST*)Resp)->dwProtocol=SCARD_PROTOCOL_T1;
	memcpy(Resp + sizeof(SCARD_IO_REQUEST), outdata, outlength);
	completeWithBuffer(pRequest,Resp,RespSize+sizeof(SCARD_IO_REQUEST));
}

void CMyDevice::IoSmartCardGetAttribute(IWDFIoRequest* pRequest,SIZE_T inBufSize,SIZE_T outBufSize) {
	UNREFERENCED_PARAMETER(inBufSize);
	DWORD code;
	int ATRsize;

	if (!getInteger(pRequest, &code)) {
		OutputDebugString(L"error getting code\n");
		pRequest->CompleteWithInformation(STATUS_NO_MEDIA, 0);
		return;
	}

	switch(code) {
		case SCARD_ATTR_CHARACTERISTICS:
			// 0x00000000 No special characteristics
			OutputDebugString(L"[GATT]SCARD_ATTR_CHARACTERISTICS\n");
			completeWithInteger(pRequest,0);
			return;
		case SCARD_ATTR_VENDOR_NAME:
			OutputDebugString(L"[GATT]SCARD_ATTR_VENDOR_NAME\n");
			completeWithString(pRequest,"Virtual Smart Card Architecture",(int)outBufSize);
			return;
		case SCARD_ATTR_VENDOR_IFD_TYPE:
			OutputDebugString(L"[GATT]SCARD_ATTR_VENDOR_IFD_TYPE\n");
			completeWithString(pRequest,"Virtual PCD",(int)outBufSize);
			return;
		case SCARD_ATTR_DEVICE_UNIT:
			OutputDebugString(L"[GATT]SCARD_ATTR_DEVICE_UNIT\n");
			completeWithInteger(pRequest,0);
			return;
		case SCARD_ATTR_ATR_STRING:
			OutputDebugString(L"[GATT]SCARD_ATTR_ATR_STRING\n");
			ATRsize = vicc_getatr(ctx, &outdata);
			if (ATRsize <= 0)
			{
				pRequest->CompleteWithInformation(STATUS_NO_MEDIA, 0);					
				return;
			}
			outlength = ATRsize;
			completeWithBuffer(pRequest,outdata,outlength);
			return;
		case SCARD_ATTR_CURRENT_PROTOCOL_TYPE:
			OutputDebugString(L"[GATT]SCARD_ATTR_CURRENT_PROTOCOL_TYPE\n");
			completeWithInteger(pRequest,SCARD_PROTOCOL_T1); // T=1
			return;
		default:
			OutputDebugString(L"[GATT]ERROR_NOT_SUPPORTED\n");
	        pRequest->CompleteWithInformation(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED), 0);
	}
}

void CMyDevice::ProcessIoControl(__in IWDFIoQueue*     pQueue,
                                    __in IWDFIoRequest*   pRequest,
                                    __in ULONG            ControlCode,
                                         SIZE_T           inBufSize,
                                         SIZE_T           outBufSize)
{
	inFunc;
	UNREFERENCED_PARAMETER(pQueue);

	if (ControlCode==IOCTL_SMARTCARD_GET_ATTRIBUTE) {
		IoSmartCardGetAttribute(pRequest,inBufSize,outBufSize);
		return;
	}
	else if (ControlCode==IOCTL_SMARTCARD_IS_PRESENT) {
		IoSmartCardIsPresent(pRequest,inBufSize,outBufSize);
		return;
	}
	else if (ControlCode==IOCTL_SMARTCARD_GET_STATE) {
		IoSmartCardGetState(pRequest,inBufSize,outBufSize);
		return;
	}
	else if (ControlCode==IOCTL_SMARTCARD_IS_ABSENT) {
		IoSmartCardIsAbsent(pRequest,inBufSize,outBufSize);
		return;
	}
	else if (ControlCode==IOCTL_SMARTCARD_POWER) {
		IoSmartCardPower(pRequest,inBufSize,outBufSize);
		return;
	}	
	else if (ControlCode==IOCTL_SMARTCARD_SET_ATTRIBUTE) {
		IoSmartCardSetAttribute(pRequest,inBufSize,outBufSize);
		return;
	}
	else if (ControlCode==IOCTL_SMARTCARD_TRANSMIT) {
		IoSmartCardTransmit(pRequest,inBufSize,outBufSize);
		return;
	}
	OutputDebugString(L"[IOCT]ERROR_NOT_SUPPORTED\n");
    pRequest->CompleteWithInformation(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED), 0);

    return;
}

/////////////////////////////////////////////////////////////////////////
//
// CMyDevice::OnPrepareHardware
//
//  Called by UMDF to prepare the hardware for use. 
//
// Parameters:
//      pWdfDevice - pointer to an IWDFDevice object representing the
//      device
//
// Return Values:
//      S_OK: success
//
/////////////////////////////////////////////////////////////////////////
HRESULT CMyDevice::OnPrepareHardware(
        __in IWDFDevice* pWdfDevice
        )
{
	inFunc;
    // Store the IWDFDevice pointer
    m_pWdfDevice = pWdfDevice;

    // Configure the default IO Queue
    HRESULT hr = CMyQueue::CreateInstance(m_pWdfDevice, this);

	ctx = vicc_init(NULL, VPCDPORT);

    return hr;
}


/////////////////////////////////////////////////////////////////////////
//
// CMyDevice::OnReleaseHardware
//
// Called by WUDF to uninitialize the hardware.
//
// Parameters:
//      pWdfDevice - pointer to an IWDFDevice object for the device
//
// Return Values:
//      S_OK:
//
/////////////////////////////////////////////////////////////////////////
HRESULT CMyDevice::OnReleaseHardware(
        __in IWDFDevice* pWdfDevice
        )
{
	inFunc;
    UNREFERENCED_PARAMETER(pWdfDevice);
    
    // Release the IWDFDevice handle, if it matches
    if (pWdfDevice == m_pWdfDevice.p)
    {
		vicc_eject(ctx);
		vicc_exit(ctx);
		ctx = NULL;
        m_pWdfDevice.Release();
    }

    return S_OK;
}


HRESULT CMyDevice::OnD0Entry(IN IWDFDevice*  pWdfDevice,IN WDF_POWER_DEVICE_STATE  previousState) {
	UNREFERENCED_PARAMETER(pWdfDevice);
	UNREFERENCED_PARAMETER(previousState);
	inFunc;
	vicc_poweroff(ctx);
	return S_OK;
}
HRESULT CMyDevice::OnD0Exit(IN IWDFDevice*  pWdfDevice,IN WDF_POWER_DEVICE_STATE  newState) {
	UNREFERENCED_PARAMETER(pWdfDevice);
	UNREFERENCED_PARAMETER(newState);
	inFunc;
	vicc_poweron(ctx);
	return S_OK;
}

void CMyDevice::shutDown(){
	if (waitRemoveIpr!=NULL) {
		if (waitRemoveIpr->UnmarkCancelable()==S_OK)
			waitRemoveIpr->Complete(HRESULT_FROM_WIN32(ERROR_CANCELLED));
		waitRemoveIpr=NULL;
	}
	if (waitInsertIpr!=NULL) {
		if (waitInsertIpr->UnmarkCancelable()==S_OK)
			waitInsertIpr->Complete(HRESULT_FROM_WIN32(ERROR_CANCELLED));
		waitInsertIpr=NULL;
	}
	free(outdata);
	outdata = NULL;
	outlength = 0;
	free(indata);
	indata = NULL;
	inlength = 0;
}
HRESULT CMyDevice::OnQueryRemove(IN IWDFDevice*  pWdfDevice) {
	UNREFERENCED_PARAMETER(pWdfDevice);
	inFunc;
	shutDown();
	return S_OK;
}
HRESULT CMyDevice::OnQueryStop(IN IWDFDevice*  pWdfDevice) {
	UNREFERENCED_PARAMETER(pWdfDevice);
	inFunc;
	shutDown();
	return S_OK;
}
void CMyDevice::OnSurpriseRemoval(IN IWDFDevice*  pWdfDevice) {
	UNREFERENCED_PARAMETER(pWdfDevice);
	inFunc;
	shutDown();
}

STDMETHODIMP_ (void) CMyDevice::OnCancel(IN IWDFIoRequest*  pWdfRequest) {
	inFunc;
	if (pWdfRequest==waitRemoveIpr) {
		OutputDebugString(L"Cancel Remove\n");
		waitRemoveIpr=NULL;
	} else if (pWdfRequest==waitInsertIpr) {
		OutputDebugString(L"Cancel Insert\n");
		waitInsertIpr=NULL;
	}
	pWdfRequest->CompleteWithInformation(HRESULT_FROM_WIN32(ERROR_CANCELLED), 0);
}

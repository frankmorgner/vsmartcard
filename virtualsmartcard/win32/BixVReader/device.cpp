#include "internal.h"
#include "VirtualSCReader.h"
#include "queue.h"
#include "device.h"
#include "driver.h"
#include <stdio.h>
#include <winscard.h>
#include "memory.h"
#include "SectionLocker.h"

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
    inFunc
        SectionLogger a(__FUNCTION__);
    CComObject<CMyDevice>* device = NULL;
    HRESULT hr;

    OutputDebugString(L"[BixVReader]CreateInstance");    //
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

    OutputDebugString(L"[BixVReader]SetLockingConstraint");    //
    FxDeviceInit->SetLockingConstraint(WdfDeviceLevel);

    CComPtr<IUnknown> spCallback;
    OutputDebugString(L"[BixVReader]QueryInterface");    //
    hr = device->QueryInterface(IID_IUnknown, (void**)&spCallback);

    CComPtr<IWDFDevice> spIWDFDevice;
    if (SUCCEEDED(hr))
    {
        OutputDebugString(L"[BixVReader]CreateDevice");    //
        hr = FxDriver->CreateDevice(FxDeviceInit, spCallback, &spIWDFDevice);
    }

    device->numInstances=GetPrivateProfileInt(L"Driver",L"NumReaders",1,L"BixVReader.ini");

    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
        OutputDebugString(L"[BixVReader]Error at WSAStartup()\n");
    }

    for (int i=0;i<device->numInstances;i++) {
        OutputDebugString(L"[BixVReader]CreateDeviceInterface");    //
        wchar_t name[10];
        swprintf(name,L"DEV%i",i);

        if (spIWDFDevice->CreateDeviceInterface(&SmartCardReaderGuid,name)!=0)
            OutputDebugString(L"[BixVReader]CreateDeviceInterface Failed");
    }

    SAFE_RELEASE(device);
    return hr;
}

void CMyDevice::ProcessIoControl(__in IWDFIoQueue*     pQueue,
                                 __in IWDFIoRequest*   pRequest,
                                 __in ULONG            ControlCode,
                                 SIZE_T           inBufSize,
                                 SIZE_T           outBufSize)
{
    inFunc
        SectionLogger a(__FUNCTION__);
    UNREFERENCED_PARAMETER(pQueue);
    wchar_t log[300];
    swprintf(log,L"[BixVReader][IOCT]IOCTL %08X - In %i Out %i",ControlCode,inBufSize,outBufSize);
    OutputDebugString(log);

    //SectionLocker lock(m_RequestLock);
    int instance=0;
    {
        CComPtr<IWDFFile>   pFileObject;
        pRequest->GetFileObject(&pFileObject);

        if (pFileObject != NULL)
        {
            DWORD logLen=300;
            pFileObject->RetrieveFileName(log,&logLen);
            instance=_wtoi(log+(logLen-2));
        }
    }
    Reader &reader=*readers[instance];

    if (ControlCode==IOCTL_SMARTCARD_GET_ATTRIBUTE) {
        reader.IoSmartCardGetAttribute(pRequest,inBufSize,outBufSize);
        return;
    }
    else if (ControlCode==IOCTL_SMARTCARD_IS_PRESENT) {
        reader.IoSmartCardIsPresent(pRequest,inBufSize,outBufSize);
        return;
    }
    else if (ControlCode==IOCTL_SMARTCARD_GET_STATE) {
        reader.IoSmartCardGetState(pRequest,inBufSize,outBufSize);
        return;
    }
    else if (ControlCode==IOCTL_SMARTCARD_IS_ABSENT) {
        reader.IoSmartCardIsAbsent(pRequest,inBufSize,outBufSize);
        return;
    }
    else if (ControlCode==IOCTL_SMARTCARD_POWER) {
        reader.IoSmartCardPower(pRequest,inBufSize,outBufSize);
        return;
    }
    else if (ControlCode==IOCTL_SMARTCARD_SET_ATTRIBUTE) {
        reader.IoSmartCardSetAttribute(pRequest,inBufSize,outBufSize);
        return;
    }
    else if (ControlCode==IOCTL_SMARTCARD_SET_PROTOCOL) {
        reader.IoSmartCardSetProtocol(pRequest,inBufSize,outBufSize);
        return;
    }
    else if (ControlCode==IOCTL_SMARTCARD_TRANSMIT) {
        reader.IoSmartCardTransmit(pRequest,inBufSize,outBufSize);
        return;
    }
    swprintf(log,L"[BixVReader][IOCT]ERROR_NOT_SUPPORTED:%08X",ControlCode);
    OutputDebugString(log);
    pRequest->CompleteWithInformation(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED), 0);

    return;
}


DWORD WINAPI ServerFunc(Reader *reader) {
    return reader->startServer();
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
    inFunc
        SectionLogger a(__FUNCTION__);
    // Store the IWDFDevice pointer
    m_pWdfDevice = pWdfDevice;

    // Configure the default IO Queue
    HRESULT hr = CMyQueue::CreateInstance(m_pWdfDevice, this);

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
    inFunc
        SectionLogger a(__FUNCTION__);
    UNREFERENCED_PARAMETER(pWdfDevice);

    // Release the IWDFDevice handle, if it matches
    if (pWdfDevice == m_pWdfDevice.p)
    {
        m_pWdfDevice.Release();
    }

    return S_OK;
}


HRESULT CMyDevice::OnD0Entry(IN IWDFDevice*  pWdfDevice,IN WDF_POWER_DEVICE_STATE  previousState) {
    SectionLogger a(__FUNCTION__);
    UNREFERENCED_PARAMETER(pWdfDevice);
    UNREFERENCED_PARAMETER(previousState);

    numInstances=GetPrivateProfileInt(L"Driver",L"NumReaders",1,L"BixVReader.ini");
    readers.resize(numInstances);

    for (int i=0;i<numInstances;i++) {
        wchar_t section[300];
        char sectionA[300];
        swprintf(section,L"Reader%i",i);
        sprintf(sectionA,"Reader%i",i);

        int rpcType=GetPrivateProfileInt(section,L"RPC_TYPE",0,L"BixVReader.ini");
        if (rpcType==0)
            readers[i]=new PipeReader();
        else if (rpcType==1)
            readers[i]=new TcpIpReader();
        else if (rpcType==2)
            readers[i]=new VpcdReader();

        readers[i]->instance=i;
        readers[i]->device=this;
        GetPrivateProfileStringA(sectionA,"VENDOR_NAME","Bix",readers[i]->vendorName,sizeof readers[i]->vendorName,"BixVReader.ini");
        GetPrivateProfileStringA(sectionA,"VENDOR_IFD_TYPE","VIRTUAL_CARD_READER",readers[i]->vendorIfdType,sizeof readers[i]->vendorIfdType,"BixVReader.ini");
        readers[i]->deviceUnit=GetPrivateProfileInt(section,L"DEVICE_UNIT",i,L"BixVReader.ini");
        readers[i]->protocol=0;

        readers[i]->init(section);

        DWORD pipeThreadID;
        readers[i]->serverThread=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)ServerFunc,readers[i],0,&pipeThreadID);
    }

    return S_OK;
}
HRESULT CMyDevice::OnD0Exit(IN IWDFDevice*  pWdfDevice,IN WDF_POWER_DEVICE_STATE  newState) {
    SectionLogger a(__FUNCTION__);
    UNREFERENCED_PARAMETER(pWdfDevice);
    UNREFERENCED_PARAMETER(newState);
    // dovrei fermare tutti i thread in ascolto dei lettori
    shutDown();
    return S_OK;
}

void CMyDevice::shutDown() {
    SectionLogger a(__FUNCTION__);
    for (int i=0;i<numInstances;i++) {
        readers[i]->shutdown();
        delete readers[i];
    }
    numInstances=0;
}
HRESULT CMyDevice::OnQueryRemove(IN IWDFDevice*  pWdfDevice) {
    SectionLogger a(__FUNCTION__);
    UNREFERENCED_PARAMETER(pWdfDevice);
    OutputDebugString(L"[BixVReader]OnQueryRemove");
    shutDown();
    return S_OK;
}
HRESULT CMyDevice::OnQueryStop(IN IWDFDevice*  pWdfDevice) {
    SectionLogger a(__FUNCTION__);
    UNREFERENCED_PARAMETER(pWdfDevice);
    OutputDebugString(L"[BixVReader]OnQueryStop");
    shutDown();
    return S_OK;
}
void CMyDevice::OnSurpriseRemoval(IN IWDFDevice*  pWdfDevice) {
    SectionLogger a(__FUNCTION__);
    UNREFERENCED_PARAMETER(pWdfDevice);
    OutputDebugString(L"[BixVReader]OnSurpriseRemoval");
    shutDown();
}

STDMETHODIMP_ (void) CMyDevice::OnCancel(IN IWDFIoRequest*  pWdfRequest) {
    SectionLogger a(__FUNCTION__);

    wchar_t log[300];
    SectionLocker lock(m_RequestLock);

    int instance=0;
    {
        CComPtr<IWDFFile>   pFileObject;
        pWdfRequest->GetFileObject(&pFileObject);

        if (pFileObject != NULL)
        {
            DWORD logLen=300;
            pFileObject->RetrieveFileName(log,&logLen);
            instance=_wtoi(log+(logLen-2));
        }
    }
    Reader &reader=*readers[instance];

    for (std::vector< CComPtr<IWDFIoRequest> >::iterator it = reader.waitRemoveIpr.begin();
            it != reader.waitRemoveIpr.end(); it++) {
        if (pWdfRequest == *it) {
            OutputDebugString(L"[BixVReader]Cancel Remove");
            reader.waitRemoveIpr.erase(it);
        }
    }
    for (std::vector< CComPtr<IWDFIoRequest> >::iterator it = reader.waitInsertIpr.begin();
            it != reader.waitInsertIpr.end(); it++) {
        if (pWdfRequest == *it) {
            OutputDebugString(L"[BixVReader]Cancel Insert");
            reader.waitInsertIpr.erase(it);
        }
    }
    pWdfRequest->CompleteWithInformation(HRESULT_FROM_WIN32(ERROR_CANCELLED), 0);
}


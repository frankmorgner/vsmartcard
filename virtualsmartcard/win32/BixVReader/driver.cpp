#include "internal.h"
#include "VirtualSCReader.h"
#include "driver.h"
#include "device.h"
#include <stdio.h>
#include "SectionLocker.h"

CMyDriver::CMyDriver()
{
	inFunc
}

/////////////////////////////////////////////////////////////////////////
//
// CMyDriver::OnDeviceAdd
//
// The framework call this function when device is detected. This driver
// creates a device callback object
//
// Parameters:
//      pDriver     - pointer to an IWDFDriver object
//      pDeviceInit - pointer to a device initialization object
//
// Return Values:
//      S_OK: device initialized successfully
//
/////////////////////////////////////////////////////////////////////////
HRESULT CMyDriver::OnDeviceAdd(
    __in IWDFDriver* pDriver,
    __in IWDFDeviceInitialize* pDeviceInit
    )
{
	inFunc
	SectionLogger a(__FUNCTION__);
    HRESULT hr = CMyDevice::CreateInstance(pDriver, pDeviceInit);

    return hr;
}
/////////////////////////////////////////////////////////////////////////
//
// CMyDriver::OnInitialize
//
//  The framework calls this function just after loading the driver. The driver
//  can perform any global, device independent intialization in this routine.
//
/////////////////////////////////////////////////////////////////////////
HRESULT CMyDriver::OnInitialize(
    __in IWDFDriver* pDriver
    )
{
	inFunc
	SectionLogger a(__FUNCTION__);
    UNREFERENCED_PARAMETER(pDriver);
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////
//
// CMyDriver::OnDeinitialize
//
//  The framework calls this function just before de-initializing itself. All
//  WDF framework resources should be released by driver before returning
//  from this call.
//
/////////////////////////////////////////////////////////////////////////
void CMyDriver::OnDeinitialize(
    __in IWDFDriver* pDriver
    )
{
	inFunc
 	SectionLogger a(__FUNCTION__);
   UNREFERENCED_PARAMETER(pDriver);
    return;
}


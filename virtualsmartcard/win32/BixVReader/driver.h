

#pragma once

//
// This class handles driver events for the skeleton sample.  In particular
// it supports the OnDeviceAdd event, which occurs when the driver is called
// to setup per-device handlers for a new device stack.
//

class ATL_NO_VTABLE CMyDriver :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CMyDriver, &CLSID_VirtualSCReaderDriver>,
    public IDriverEntry
{
public:
    CMyDriver();

    DECLARE_NO_REGISTRY()
    DECLARE_CLASSFACTORY()
    DECLARE_NOT_AGGREGATABLE(CMyDriver)

    BEGIN_COM_MAP(CMyDriver)
        COM_INTERFACE_ENTRY(IDriverEntry)
    END_COM_MAP()

public:
    //
    // IDriverEntry
    //
    STDMETHOD  (OnInitialize)(__in IWDFDriver* pDriver);
    STDMETHOD  (OnDeviceAdd)(__in IWDFDriver* pDriver, __in IWDFDeviceInitialize* pDeviceInit);
    STDMETHOD_ (void, OnDeinitialize)(__in IWDFDriver* pDriver);
};

OBJECT_ENTRY_AUTO(__uuidof(VirtualSCReaderDriver), CMyDriver)


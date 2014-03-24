#include "internal.h"

class CVirtualSmartCardDriverModule : public CAtlDllModuleT<CVirtualSmartCardDriverModule> {} _AtlModule;

/////////////////////////////////////////////////////////////////////////
//
// DllMain
//
// This is the main DLL Entry Point.
//
// Parameters:
//      hInstance  - Handle to the DLL module
//      dwReason   - Indicates why the DLL entry point is being called
//      lpReserved - Additional information based on dwReason
//
// Return Values:
//      TRUE  = initialization succeeds
//      FALSE = initialization fails
//
/////////////////////////////////////////////////////////////////////////

extern "C" BOOL WINAPI DllMain(HINSTANCE    hInstance,
                               DWORD        dwReason,
                               LPVOID       lpReserved)
{
	inFunc
    (lpReserved);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstance);

			break;

        case DLL_PROCESS_DETACH:
            _AtlModule.Term();
            break;

        default:
            break;
    }

    // Call the ATL module class so it can initialize
    return _AtlModule.DllMain(dwReason, lpReserved); 
}

/////////////////////////////////////////////////////////////////////////
//
// DllCanUnloadNow
//
// Used to determine whether the DLL can be unloaded by OLE
//
// Parameters:
//      void - (unused argument)
//
// Return Values:
//      S_OK: DLL can be unloaded
//      S_FALSE: DLL cannot be unloaded now
//
/////////////////////////////////////////////////////////////////////////
STDAPI DllCanUnloadNow(void)
{
	inFunc
    return _AtlModule.DllCanUnloadNow();
}

/////////////////////////////////////////////////////////////////////////
//
// DllGetClassObject
//
// Returns a class factory to create an object of the requested type
//
// Parameters:
//      rclsid - CLSID that will associate the correct data and code
//      riid   - Reference to the IID the caller will use
//      ppv    - pointer to an interface pointer requested in riid
//
// Return Values:
//      S_OK: The object was retrieved successfully.
//      CLASS_E_CLASSNOTAVAILABLE: The DLL does not support the class
//
/////////////////////////////////////////////////////////////////////////

STDAPI DllGetClassObject(__in REFCLSID rclsid, __in REFIID riid, __deref_out LPVOID* ppv)
{
	inFunc
    return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////
//
// DllRegisterServer
//
// Adds entries to the system registry
//
// Parameters:
//      void - (unused argument)
//
// Return Values:
//      S_OK: The registry entries were created successfully
//      SELFREG_E_TYPELIB: The server was unable to complete the
//          registration of all the type libraries used by its classes
//      SELFREG_E_CLASS: The server was unable to complete the
//          registration of all the object classes
//
/////////////////////////////////////////////////////////////////////////
STDAPI DllRegisterServer(void)
{
	inFunc
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////
//
// DllUnregisterServer
//
// Removes entries from the system registry
//
// Parameters:
//      void - (unused argument)
//
// Return Values:
//      S_OK: The registry entries were removed successfully
//      S_FALSE: Unregistration of known entries was successful, but
//          other entries still exist for this server's classes
//      SELFREG_E_TYPELIB: The server was unable to remove the entries
//          of all the type libraries used by its classes
//      SELFREG_E_CLASS: The server was unable to to remove the entries
//          of all the object classes
//
/////////////////////////////////////////////////////////////////////////
STDAPI DllUnregisterServer(void)
{
	inFunc
    return S_OK;
}


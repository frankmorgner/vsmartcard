
#pragma once

#include <windows.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlstr.h>

//__user_driver;  // Macro letting the compiler know this is not a kernel driver (this will help surpress needless warnings)

// Common WPD and WUDF headers

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)     {if ((p)) { (p)->Release(); (p) = NULL; }}
#endif

#define STATUS_SUCCESS                          ((NTSTATUS)0x00000000L) // ntsubauth
#define STATUS_NO_MEDIA							((NTSTATUS)0xC0000178L) 
#define STATUS_INVALID_DEVICE_STATE				((NTSTATUS)0xC0000184L)


//
// Include the WUDF DDI 
//
#include <devioctl.h>
#include <initguid.h>
#include <propkeydef.h>
#include <propvarutil.h>
#include "PortableDeviceTypes.h"
#include "PortableDeviceClassExtension.h"
#include "PortableDevice.h"

#include "wudfddi.h"

//
// Use specstrings for in/out annotation of function parameters.
//

#include "specstrings.h"

//
// Forward definitions of classes in the other header files.
//

typedef class CMyDriver *PCMyDriver;
typedef class CMyDevice *PCMyDevice;

//
DEFINE_GUID(SmartCardReaderGuid, 0x50DD5230, 0xBA8A, 0x11D1, 0xBF,0x5D,0x00,0x00,0xF8,0x05,0xF5,0x30);
//
// Include the type specific headers.
//

//class funcTrace {
//public:
//	TCHAR funcN[500];
//	funcTrace (char *func) {
//		TCHAR funcName[500];
//		wsprintf(funcN, _T("%S"), func);
//		wsprintf(funcName, _T("[BixVReader]IN -> %s"), funcN);
//		OutputDebugString(funcName);
//	}
//	~funcTrace () {
//		TCHAR funcName[500];
//		wsprintf(funcName, _T("[BixVReader]OUT -> %s"), funcN);
//		OutputDebugString(funcName);
//	}
//};

//#define inFunc funcTrace _ftrace(__FUNCTION__);
#define inFunc
/*
 * MUSCLE SmartCard Development ( http://www.linuxnet.com )
 *
 * Copyright (C) 1999
 *  David Corcoran <corcoran@linuxnet.com>
 * Copyright (C) 2002-2011
 *  Ludovic Rousseau <ludovic.rousseau@free.fr>
 *
 * $Id: wintypes.h 5869 2011-07-09 12:04:18Z rousseau $
 */

/**
 * @file
 * @brief This keeps a list of Windows(R) types.
 */

#ifndef __wintypes_h__
#define __wintypes_h__

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __APPLE__

#include <stdint.h>

#ifndef BYTE
    typedef uint8_t BYTE;
#endif
    typedef uint8_t UCHAR;
    typedef UCHAR *PUCHAR;
    typedef uint16_t USHORT;

#ifndef __COREFOUNDATION_CFPLUGINCOM__
    typedef uint32_t ULONG;
    typedef void *LPVOID;
    typedef int16_t BOOL;
#endif

    typedef ULONG *PULONG;
    typedef const void *LPCVOID;
    typedef uint32_t DWORD;
    typedef DWORD *PDWORD;
    typedef uint16_t WORD;
    typedef int32_t LONG;
    typedef const char *LPCSTR;
    typedef const BYTE *LPCBYTE;
    typedef BYTE *LPBYTE;
    typedef DWORD *LPDWORD;
    typedef char *LPSTR;

#else

#ifndef BYTE
	typedef unsigned char BYTE;
#endif
	typedef unsigned char UCHAR;
	typedef UCHAR *PUCHAR;
	typedef unsigned short USHORT;

#ifndef __COREFOUNDATION_CFPLUGINCOM__
	typedef unsigned long ULONG;
	typedef void *LPVOID;
#endif

	typedef const void *LPCVOID;
	typedef unsigned long DWORD;
	typedef DWORD *PDWORD;
	typedef long LONG;
	typedef const char *LPCSTR;
	typedef const BYTE *LPCBYTE;
	typedef BYTE *LPBYTE;
	typedef DWORD *LPDWORD;
	typedef char *LPSTR;

	/* these types were deprecated but still used by old drivers and
	 * applications. So just declare and use them. */
	typedef LPSTR LPTSTR;
	typedef LPCSTR LPCTSTR;

	/* types unused by pcsc-lite */
	typedef short BOOL;
	typedef unsigned short WORD;
	typedef ULONG *PULONG;

#endif

#ifdef __cplusplus
}
#endif

#endif

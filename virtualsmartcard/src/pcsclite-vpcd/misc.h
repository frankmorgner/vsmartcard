/*
 * This handles GCC attributes
 *
 * MUSCLE SmartCard Development ( http://pcsclite.alioth.debian.org/pcsclite.html )
 *
 * Copyright (C) 2005-2010
 *  Ludovic Rousseau <ludovic.rousseau@free.fr>
 *
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id$
 */

#ifndef __misc_h__
#define __misc_h__

/*
 * Declare the function as internal to the library: the function name is
 * not exported and can't be used by a program linked to the library
 *
 * see http://gcc.gnu.org/onlinedocs/gcc-3.3.5/gcc/Function-Attributes.html#Function-Attributes
 * see http://www.nedprod.com/programs/gccvisibility.html
 */
#if defined __GNUC__ && (! defined (__sun)) && (__GNUC__ >= 4 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3))
#define INTERNAL __attribute__ ((visibility("hidden")))
#define PCSC_API __attribute__ ((visibility("default")))
#elif (! defined __GNUC__ ) && defined (__sun)
/* http://wikis.sun.com/display/SunStudio/Macros+for+Shared+Library+Symbol+Visibility */
#define INTERNAL __hidden
#define PCSC_API __global
#else
#define INTERNAL
#define PCSC_API
#endif
#define EXTERNAL PCSC_API

#if defined __GNUC__

/* GNU Compiler Collection (GCC) */
#define CONSTRUCTOR __attribute__ ((constructor))
#define DESTRUCTOR __attribute__ ((destructor))

#else

/* SUN C compiler does not use __attribute__ but #pragma init (function)
 * We can't use a # inside a #define so it is not possible to use
 * #define CONSTRUCTOR_DECLARATION(x) #pragma init (x)
 * The #pragma is used directly where needed */

/* any other */
#define CONSTRUCTOR
#define DESTRUCTOR

#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef COUNT_OF
#define COUNT_OF(arr) (sizeof(arr)/sizeof(arr[0]))
#endif

#endif /* __misc_h__ */

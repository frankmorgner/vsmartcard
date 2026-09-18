#pragma once
#include "internal.h"
#include "device.h"


bool getBuffer(IWDFIoRequest* pRequest,void **buffer,SIZE_T *bufferLen,SIZE_T inBufSize);
void setString(CMyDevice *device,IWDFIoRequest* pRequest,char *result,SIZE_T outSize);
void setBuffer(CMyDevice *device,IWDFIoRequest* pRequest,void *result,SIZE_T inSize,SIZE_T outSize);
void setInt(CMyDevice *device,IWDFIoRequest* pRequest,DWORD result,SIZE_T outSize);
DWORD getInt(IWDFIoRequest* pRequest,SIZE_T inBufSize);

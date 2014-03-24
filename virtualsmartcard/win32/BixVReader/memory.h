#pragma once
#include "internal.h"
#include "device.h"


bool getBuffer(IWDFIoRequest* pRequest,BYTE *buffer,int *bufferLen);
void setString(CMyDevice *device,IWDFIoRequest* pRequest,char *result,int outSize);
void setBuffer(CMyDevice *device,IWDFIoRequest* pRequest,BYTE *result,int inSize);
void setInt(CMyDevice *device,IWDFIoRequest* pRequest,DWORD result);
DWORD getInt(IWDFIoRequest* pRequest);

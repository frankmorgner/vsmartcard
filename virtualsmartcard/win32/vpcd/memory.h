#pragma once
#include "internal.h"


bool getBuffer(IWDFIoRequest *pRequest, BYTE **buffer, SIZE_T *bufferLen);
void completeWithString(IWDFIoRequest* pRequest,char *result,int outSize);
void completeWithBuffer(IWDFIoRequest* pRequest,BYTE *result,int inSize);
void completeWithInteger(IWDFIoRequest* pRequest,DWORD result);
bool getInteger(IWDFIoRequest *pRequest, DWORD *integer);

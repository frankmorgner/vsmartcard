#include "memory.h"

bool getBuffer(IWDFIoRequest *pRequest, BYTE **buffer, SIZE_T *bufferLen)
{
	IWDFMemory *inmem=NULL;

	if (!pRequest || !buffer || !bufferLen) {
		OutputDebugString(L"wrong arguments");
		return false;
	}

	pRequest->GetInputMemory(&inmem);
	if (inmem==NULL) {
		OutputDebugString(L"GetInputMemory failed");
		return false;
	} else {
		SIZE_T size;
		void *data=inmem->GetDataBuffer(&size);
		if (*bufferLen < size) {
			BYTE *p = (BYTE *) realloc(*buffer, size);
			if (!p) {
				OutputDebugString(L"realloc failed");
				inmem->Release();
				return false;
			}
			*buffer = p;
		}

		memcpy(*buffer, data, size);
		(*bufferLen) = size;

		inmem->Release();
		return true;
	}
}

void completeWithBuffer(IWDFIoRequest* pRequest,BYTE *result,int inSize) {
	IWDFMemory *outmem=NULL;
	pRequest->GetOutputMemory (&outmem);
	if (outmem==NULL) {
		OutputDebugString(L"GetOutputMemory failed");
        pRequest->Complete(HRESULT_FROM_WIN32(ERROR_GEN_FAILURE));
	}
	else {
		outmem->CopyFromBuffer(0,result,inSize);
		outmem->Release();
        pRequest->CompleteWithInformation(0,(SIZE_T)inSize);
	}
}

void completeWithString(IWDFIoRequest* pRequest,char *result,int outSize) {
	IWDFMemory *outmem=NULL;
	pRequest->GetOutputMemory (&outmem);
	if (outmem==NULL) {
		OutputDebugString(L"GetOutputMemory failed");
        pRequest->Complete(HRESULT_FROM_WIN32(ERROR_GEN_FAILURE));
	}
	else {
		int size=min(outSize,(int)strlen(result)+1);
		outmem->CopyFromBuffer(0,result,size);
		outmem->Release();
        pRequest->CompleteWithInformation(0,(SIZE_T)size);
	}
}
void completeWithInteger(IWDFIoRequest* pRequest,DWORD result) {
	IWDFMemory *outmem=NULL;
	pRequest->GetOutputMemory (&outmem);
	if (outmem==NULL) {
		OutputDebugString(L"GetOutputMemory failed");
        pRequest->Complete(HRESULT_FROM_WIN32(ERROR_GEN_FAILURE));
	}
	else {
		outmem->CopyFromBuffer(0,&result,sizeof(result));
		outmem->Release();
        pRequest->CompleteWithInformation(0,sizeof(result));
	}
}

bool getInteger(IWDFIoRequest *pRequest, DWORD *integer)
{
	IWDFMemory *inmem=NULL;

	if (!pRequest || !integer) {
		OutputDebugString(L"wrong arguments");
		return false;
	}

	pRequest->GetInputMemory(&inmem);
	if (inmem==NULL) {
		OutputDebugString(L"GetInputMemory failed");
		return false;
	} else {
		SIZE_T size;
		void *data=inmem->GetDataBuffer(&size);
		if (size != sizeof(*integer)) {
			OutputDebugString(L"not enough memory failed");
			return false;
		}
		*integer=*(DWORD *)data;
		inmem->Release();
		return true;
	}
}

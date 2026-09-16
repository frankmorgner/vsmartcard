#include "memory.h"
#include "SectionLocker.h"

bool getBuffer(IWDFIoRequest* pRequest,void **buffer,SIZE_T *bufferLen) {
	IWDFMemory *inmem=NULL;
	pRequest->GetInputMemory(&inmem);
	if (inmem==NULL) {
		OutputDebugString(L"GetInputMemory failed");
		return false;
	}
	else {
		SIZE_T size;
		void *data=inmem->GetDataBuffer(&size);
		if (0 != size) {
			void *out = realloc(*buffer, size);
			if (out==NULL) {
				OutputDebugString(L"realloc failed");
				return false;
			}
			memcpy(out,data,size);
			(*buffer)=out;
		}
		(*bufferLen)=size;
		inmem->Release();
		return true;
	}
}

void setBuffer(CMyDevice *device,IWDFIoRequest* pRequest,void *result,SIZE_T inSize,SIZE_T outSize) {
	IWDFMemory *outmem=NULL;
	pRequest->GetOutputMemory (&outmem);
	if (outSize < inSize || outmem == NULL) {
		SectionLocker lock(device->m_RequestLock);
		if (outmem) outmem->Release();
		pRequest->CompleteWithInformation(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), inSize);
	}
	else {
		SectionLocker lock(device->m_RequestLock);
		outmem->CopyFromBuffer(0,result,inSize);
		outmem->Release();
        pRequest->CompleteWithInformation(0,inSize);
	}
}

void setString(CMyDevice *device,IWDFIoRequest* pRequest,char *result,SIZE_T outSize) {
	IWDFMemory *outmem=NULL;
	pRequest->GetOutputMemory (&outmem);
	SIZE_T requiredSize = strlen(result) + 1;
	if (outSize < requiredSize || outmem == NULL) {
		SectionLocker lock(device->m_RequestLock);
		if (outmem) outmem->Release();
		pRequest->CompleteWithInformation(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), requiredSize);
	}
	else {
		SectionLocker lock(device->m_RequestLock);
		outmem->CopyFromBuffer(0,result,requiredSize);
		outmem->Release();
        pRequest->CompleteWithInformation(0,requiredSize);
	}
}

void setInt(CMyDevice *device,IWDFIoRequest* pRequest,DWORD result,SIZE_T outSize) {
	IWDFMemory *outmem=NULL;
	pRequest->GetOutputMemory (&outmem);
	if (outSize < sizeof(result) || outmem == NULL) {
		SectionLocker lock(device->m_RequestLock);
		if (outmem) outmem->Release();
		pRequest->CompleteWithInformation(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), sizeof(result));
	}
	else {
		SectionLocker lock(device->m_RequestLock);
		outmem->CopyFromBuffer(0,&result,sizeof(result));
		outmem->Release();
        pRequest->CompleteWithInformation(0,sizeof(result));
	}
}

DWORD getInt(IWDFIoRequest* pRequest) {

	IWDFMemory *inmem=NULL;
	pRequest->GetInputMemory(&inmem);
	if (inmem==NULL) {
		OutputDebugString(L"GetInputMemory failed");
		return 0xffffffff;
	}
	else {
		SIZE_T size;
		void *data=inmem->GetDataBuffer(&size);
		if (size<sizeof(DWORD)) {
			OutputDebugString(L"Invalid input");
			return 0xffffffff;
		}
		DWORD d=*(DWORD *)data;
		inmem->Release();
		return d;
	}
}

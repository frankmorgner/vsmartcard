#include "memory.h"
#include "SectionLocker.h"

bool getBuffer(IWDFIoRequest* pRequest,void **buffer,int *bufferLen) {
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
		(*bufferLen)=(int)size;
		inmem->Release();
		return true;
	}
}

void setBuffer(CMyDevice *device,IWDFIoRequest* pRequest,void *result,int inSize) {
	IWDFMemory *outmem=NULL;
	pRequest->GetOutputMemory (&outmem);
	if (outmem==NULL) {
		SectionLocker lock(device->m_RequestLock);
		OutputDebugString(L"GetOutputMemory failed");
        pRequest->Complete(HRESULT_FROM_WIN32(ERROR_GEN_FAILURE));
	}
	else {
		SectionLocker lock(device->m_RequestLock);
		outmem->CopyFromBuffer(0,result,inSize);
		outmem->Release();
        pRequest->CompleteWithInformation(0,(SIZE_T)inSize);
	}
}

void setString(CMyDevice *device,IWDFIoRequest* pRequest,char *result,int outSize) {
	IWDFMemory *outmem=NULL;
	pRequest->GetOutputMemory (&outmem);
	if (outmem==NULL) {
		SectionLocker lock(device->m_RequestLock);
		OutputDebugString(L"GetOutputMemory failed");
        pRequest->Complete(HRESULT_FROM_WIN32(ERROR_GEN_FAILURE));
	}
	else {
		SectionLocker lock(device->m_RequestLock);
		int size=min(outSize,(int)strlen(result)+1);
		outmem->CopyFromBuffer(0,result,size);
		outmem->Release();
        pRequest->CompleteWithInformation(0,(SIZE_T)size);
	}
}
void setInt(CMyDevice *device,IWDFIoRequest* pRequest,DWORD result) {
	IWDFMemory *outmem=NULL;
	pRequest->GetOutputMemory (&outmem);
	if (outmem==NULL) {
		SectionLocker lock(device->m_RequestLock);
		OutputDebugString(L"GetOutputMemory failed");
        pRequest->Complete(HRESULT_FROM_WIN32(ERROR_GEN_FAILURE));
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
		DWORD d=*(DWORD *)data;
		inmem->Release();
		return d;
	}
}

#include "internal.h"
#include "VirtualSCReader.h"
#include "reader.h"
#include "device.h"
#include <winscard.h>
#include "memory.h"
#include <Sddl.h>
#include "SectionLocker.h"

void Reader::init(wchar_t *section) {
	section;
	state=SCARD_ABSENT;
}

void Reader::IoSmartCardIsPresent(IWDFIoRequest* pRequest,SIZE_T inBufSize,SIZE_T outBufSize) {
	UNREFERENCED_PARAMETER(inBufSize);
	UNREFERENCED_PARAMETER(outBufSize);

	OutputDebugString(L"[BixVReader][IPRE]IOCTL_SMARTCARD_IS_PRESENT");
	if (CheckATR()) {
		// there's a smart card present, so complete the request
		SectionLocker lock(device->m_RequestLock);
		pRequest->CompleteWithInformation(STATUS_SUCCESS, 0);
	}
	else {
		// there's no smart card present, so leave the request pending; it will be completed later
		SectionLocker lock(device->m_RequestLock);
		waitInsertIpr.push_back(pRequest);
		IRequestCallbackCancel *callback;
		device->QueryInterface(__uuidof(IRequestCallbackCancel),(void**)&callback);
		pRequest->MarkCancelable(callback);
		callback->Release();
	}
}

void Reader::IoSmartCardGetState(IWDFIoRequest* pRequest,SIZE_T inBufSize,SIZE_T outBufSize) {
	UNREFERENCED_PARAMETER(inBufSize);
	UNREFERENCED_PARAMETER(outBufSize);
	OutputDebugString(L"[BixVReader][GSTA]IOCTL_SMARTCARD_GET_STATE");
	wchar_t log[300];
	swprintf(log,L"[BixVReader]STATE:%08X",state);
	OutputDebugString(log);
	setInt(device,pRequest,state);
}
void Reader::IoSmartCardIsAbsent(IWDFIoRequest* pRequest,SIZE_T inBufSize,SIZE_T outBufSize) {
	UNREFERENCED_PARAMETER(inBufSize);
	UNREFERENCED_PARAMETER(outBufSize);
	OutputDebugString(L"[BixVReader][IABS]IOCTL_SMARTCARD_IS_ABSENT");
	if (!CheckATR()) {
		// there's no smart card present, so complete the request
		SectionLocker lock(device->m_RequestLock);
		pRequest->CompleteWithInformation(STATUS_SUCCESS, 0);
	}
	else {
		SectionLocker lock(device->m_RequestLock);
		// there's a smart card present, so leave the request pending; it will be completed later
		waitRemoveIpr.push_back(pRequest);
		IRequestCallbackCancel *callback;

		device->QueryInterface(__uuidof(IRequestCallbackCancel),(void**)&callback);
		pRequest->MarkCancelable(callback);
		callback->Release();
	}
}

void Reader::IoSmartCardPower(IWDFIoRequest* pRequest,SIZE_T inBufSize,SIZE_T outBufSize) {
	UNREFERENCED_PARAMETER(inBufSize);
	UNREFERENCED_PARAMETER(outBufSize);
	OutputDebugString(L"[BixVReader][POWR]IOCTL_SMARTCARD_POWER");
	DWORD code=getInt(pRequest);
	if (code==SCARD_COLD_RESET) {
		OutputDebugString(L"[BixVReader][POWR]SCARD_COLD_RESET");
		protocol=0;
		powered=1;
		state=SCARD_NEGOTIABLE;
	}
	else if (code==SCARD_WARM_RESET) {
		OutputDebugString(L"[BixVReader][POWR]SCARD_WARM_RESET");
		protocol=0;
		powered=1;
		state=SCARD_NEGOTIABLE;
	}
	else if (code==SCARD_POWER_DOWN) {
		OutputDebugString(L"[BixVReader][POWR]SCARD_POWER_DOWN");
		protocol=0;
		powered=0;
		state=SCARD_SWALLOWED;
	}
	if (code==SCARD_COLD_RESET || code==SCARD_WARM_RESET) {
		BYTE ATR[100];
		DWORD ATRsize;
		if (!QueryATR(ATR,&ATRsize,true))
		{
			pRequest->CompleteWithInformation(STATUS_NO_MEDIA, 0);
			return;
		}
		setBuffer(device,pRequest,ATR,ATRsize);
	}
	else {
		SectionLocker lock(device->m_RequestLock);
		pRequest->CompleteWithInformation(STATUS_SUCCESS, 0);
	}

}

void Reader::IoSmartCardSetProtocol(IWDFIoRequest* pRequest,SIZE_T inBufSize,SIZE_T outBufSize) {
	UNREFERENCED_PARAMETER(inBufSize);
	UNREFERENCED_PARAMETER(outBufSize);
	UNREFERENCED_PARAMETER(instance);

	DWORD requestedProtocol=getInt(pRequest);
	wchar_t log[300];
	swprintf(log,L"[BixVReader][SPRT]IOCTL_SMARTCARD_SET_PROTOCOL:%08X",requestedProtocol);
	OutputDebugString(log);

	BYTE ATR[100];
	DWORD ATRsize;
	state=SCARD_SPECIFIC;
	if (!QueryATR(ATR,&ATRsize,true))
	{
		SectionLocker lock(device->m_RequestLock);
		pRequest->CompleteWithInformation(STATUS_NO_MEDIA, 0);
		return;
	}

	if (((requestedProtocol & SCARD_PROTOCOL_T1) != 0) &&
		((availableProtocol & SCARD_PROTOCOL_T1) != 0)) {
		protocol = SCARD_PROTOCOL_T1;
		SectionLocker lock(device->m_RequestLock);
		pRequest->CompleteWithInformation(STATUS_SUCCESS, 0);
		OutputDebugString(L"[BixVReader]PROTOCOL SET: T1");
		return;
	}

	if (((requestedProtocol & SCARD_PROTOCOL_T0) != 0) &&
		((availableProtocol & SCARD_PROTOCOL_T0) != 0)) {
		protocol = SCARD_PROTOCOL_T0;
		SectionLocker lock(device->m_RequestLock);
		pRequest->CompleteWithInformation(STATUS_SUCCESS, 0);
		OutputDebugString(L"[BixVReader]PROTOCOL SET: T0");
		return;
	}

	if (((requestedProtocol & (SCARD_PROTOCOL_DEFAULT)) != 0) &&
		((availableProtocol & SCARD_PROTOCOL_T1) != 0)) {
		protocol = SCARD_PROTOCOL_T1;
		SectionLocker lock(device->m_RequestLock);
		pRequest->CompleteWithInformation(STATUS_SUCCESS, 0);
		OutputDebugString(L"[BixVReader]PROTOCOL SET: T1");
		return;
	}

	if (((requestedProtocol & (SCARD_PROTOCOL_DEFAULT)) != 0) &&
		((availableProtocol & SCARD_PROTOCOL_T0) != 0)) {
		protocol = SCARD_PROTOCOL_T0;
		SectionLocker lock(device->m_RequestLock);
		pRequest->CompleteWithInformation(STATUS_SUCCESS, 0);
		OutputDebugString(L"[BixVReader]PROTOCOL SET: T0");
		return;
	}
	{
		state=SCARD_NEGOTIABLE;
		SectionLocker lock(device->m_RequestLock);
		pRequest->CompleteWithInformation(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED), 0);
	}
}

	void Reader::IoSmartCardSetAttribute(IWDFIoRequest* pRequest,SIZE_T inBufSize,SIZE_T outBufSize) {
	UNREFERENCED_PARAMETER(inBufSize);
	UNREFERENCED_PARAMETER(outBufSize);
	UNREFERENCED_PARAMETER(instance);
	OutputDebugString(L"[BixVReader][SATT]IOCTL_SMARTCARD_SET_ATTRIBUTE");

	IWDFMemory *inmem=NULL;
	pRequest->GetInputMemory(&inmem);

	SIZE_T size;
	BYTE *data=(BYTE *)inmem->GetDataBuffer(&size);

	DWORD minCode=*(DWORD*)(data);
	bool handled=false;
	if (minCode==SCARD_ATTR_DEVICE_IN_USE) {
		SectionLocker lock(device->m_RequestLock);
		OutputDebugString(L"[BixVReader][SATT]SCARD_ATTR_DEVICE_IN_USE");
		pRequest->CompleteWithInformation(STATUS_SUCCESS, 0);
		handled=true;
	}
	inmem->Release();
	if (!handled) {
		SectionLocker lock(device->m_RequestLock);
		wchar_t log[300];
		swprintf(log,L"[BixVReader][SATT]ERROR_NOT_SUPPORTED:%08X",minCode);
		OutputDebugString(log);
		pRequest->CompleteWithInformation(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED), 0);
	}
}

void Reader::IoSmartCardTransmit(IWDFIoRequest* pRequest,SIZE_T inBufSize,SIZE_T outBufSize) {
	UNREFERENCED_PARAMETER(inBufSize);
	UNREFERENCED_PARAMETER(outBufSize);
	OutputDebugString(L"[BixVReader][TRSM]IOCTL_SMARTCARD_TRANSMIT");
	SCARD_IO_REQUEST *scardRequest=NULL;
	int scardRequestSize=0;
	BYTE *RAPDU=NULL;
	int RAPDUSize=0;
	if (!getBuffer(pRequest,(void **)&scardRequest,&scardRequestSize)
			|| scardRequestSize<sizeof *scardRequest
			|| scardRequest->dwProtocol!=protocol) {
		SectionLocker lock(device->m_RequestLock);
        pRequest->CompleteWithInformation(STATUS_INVALID_DEVICE_STATE, 0);
		goto end;
	}
	if (!QueryTransmit((BYTE *)(scardRequest+1),
				scardRequestSize-sizeof(SCARD_IO_REQUEST),
				&RAPDU,&RAPDUSize))
	{
		SectionLocker lock(device->m_RequestLock);
		pRequest->CompleteWithInformation(STATUS_NO_MEDIA, 0);					
		goto end;
	}
	SCARD_IO_REQUEST *p=(SCARD_IO_REQUEST *)realloc(scardRequest,RAPDUSize+sizeof(SCARD_IO_REQUEST));
	if (p==NULL) {
		SectionLocker lock(device->m_RequestLock);
        pRequest->CompleteWithInformation(STATUS_INVALID_DEVICE_STATE, 0);
		goto end;
	}
	scardRequest = p;
	scardRequest->cbPciLength=sizeof(SCARD_IO_REQUEST);
	scardRequest->dwProtocol=protocol;
	memcpy(scardRequest+1,RAPDU,RAPDUSize);
	setBuffer(device,pRequest,scardRequest,RAPDUSize+sizeof(SCARD_IO_REQUEST));
end:
	free(scardRequest);
	free(RAPDU);
}

void Reader::IoSmartCardGetAttribute(IWDFIoRequest* pRequest,SIZE_T inBufSize,SIZE_T outBufSize) {
	UNREFERENCED_PARAMETER(inBufSize);

	wchar_t log[300]=L"";
	char temp[300];

	DWORD code=getInt(pRequest);
	swprintf(log,L"[BixVReader][GATT]  - code %0X",code);
	OutputDebugString(log);

	switch(code) {
		case SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED, 0xA009):
			// custom attribute; RPC_TYPE
			OutputDebugString(L"[BixVReader][GATT]RPC_TYPE");
			setInt(device,pRequest,rpcType);
			return;
		case SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED, 0xA00a):
			// custom attribute; PipeName
			if (rpcType==0) {
				PipeReader *pipe=(PipeReader *)this;
				OutputDebugString(L"[BixVReader][GATT]PIPE_NAME");
				sprintf(temp,"%S",pipe->pipeName);
				setString(device,pRequest,(char*)temp,(int)outBufSize);
			}
			else {
				SectionLocker lock(device->m_RequestLock);
				pRequest->CompleteWithInformation(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED), 0);
			}
			return;
		case SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED, 0xA00b):
			// custom attribute; EventPipeName
			if (rpcType==0) {
				PipeReader *pipe=(PipeReader *)this;
				OutputDebugString(L"[BixVReader][GATT]EVENT_PIPE_NAME");
				sprintf(temp,"%S",pipe->pipeEventName);
				setString(device,pRequest,(char*)temp,(int)outBufSize);
			}
			else {
				SectionLocker lock(device->m_RequestLock);
				pRequest->CompleteWithInformation(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED), 0);
			}
			return;
		case SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED, 0xA00c):
			// custom attribute; TCP port
			if (rpcType==1) {
				TcpIpReader *tcpIp=(TcpIpReader *)this;
				OutputDebugString(L"[BixVReader][GATT]PORT");
				setInt(device,pRequest,tcpIp->port);
			}
			else {
				SectionLocker lock(device->m_RequestLock);
				pRequest->CompleteWithInformation(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED), 0);
			}
			return;
		case SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED, 0xA00d):
			// custom attribute; TCP event port
			if (rpcType==1) {
				TcpIpReader *tcpIp=(TcpIpReader *)this;
				OutputDebugString(L"[BixVReader][GATT]EVENT_PORT");
				setInt(device,pRequest,tcpIp->eventPort);
			}
			else {
				SectionLocker lock(device->m_RequestLock);
				pRequest->CompleteWithInformation(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED), 0);
			}
			return;
		case SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED, 0xA00e):
			// custom attribute; TCP base port
			if (rpcType==1) {
				TcpIpReader *tcpIp=(TcpIpReader *)this;
				OutputDebugString(L"[BixVReader][GATT]BASE_PORT");
				setInt(device,pRequest,tcpIp->portBase);
				tcpIp;
			}
			else {
				SectionLocker lock(device->m_RequestLock);
				pRequest->CompleteWithInformation(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED), 0);
			}
			return;
		case SCARD_ATTR_CHARACTERISTICS:
			// 0x00000000 No special characteristics
			OutputDebugString(L"[BixVReader][GATT]SCARD_ATTR_CHARACTERISTICS");
			setInt(device,pRequest,0);
			return;
		case SCARD_ATTR_VENDOR_NAME:
			OutputDebugString(L"[BixVReader][GATT]SCARD_ATTR_VENDOR_NAME");
			setString(device,pRequest,vendorName,(int)outBufSize);
			return;
		case SCARD_ATTR_VENDOR_IFD_TYPE:
			OutputDebugString(L"[BixVReader][GATT]SCARD_ATTR_VENDOR_IFD_TYPE");
			setString(device,pRequest,vendorIfdType,(int)outBufSize);
			return;
		case SCARD_ATTR_DEVICE_UNIT:
			OutputDebugString(L"[BixVReader][GATT]SCARD_ATTR_DEVICE_UNIT");
			setInt(device,pRequest,deviceUnit);
			return;
		case SCARD_ATTR_ATR_STRING:
			OutputDebugString(L"[BixVReader][GATT]SCARD_ATTR_ATR_STRING");
			BYTE ATR[100];
			DWORD ATRsize;
			if (!QueryATR(ATR,&ATRsize))
			{
				SectionLocker lock(device->m_RequestLock);
				pRequest->CompleteWithInformation(STATUS_NO_MEDIA, 0);
				return;
			}
			setBuffer(device,pRequest,ATR,ATRsize);
			return;
		case SCARD_ATTR_CURRENT_PROTOCOL_TYPE:
			OutputDebugString(L"[BixVReader][GATT]SCARD_ATTR_CURRENT_PROTOCOL_TYPE");
			setInt(device,pRequest,protocol); // T=0 or T=1
			return;
		default: {
			swprintf(log,L"[BixVReader][GATT]ERROR_NOT_SUPPORTED:%08X",code);
			OutputDebugString(log);
			SectionLocker lock(device->m_RequestLock);
	        pRequest->CompleteWithInformation(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED), 0);
		}
	}
}

bool Reader::CheckATR() {
	return false;
}

bool Reader::QueryTransmit(BYTE *APDU,int APDUlen,BYTE **Resp,int *Resplen) {
	APDU;
	APDUlen;
	Resp;
	Resplen;
	return false;
}

bool Reader::QueryATR(BYTE *ATR,DWORD *ATRsize,bool reset) {
	ATR;
	ATRsize;
	reset;
	return false;
}

bool Reader::initProtocols() {
	// ask ATR to determine available protocols
	BYTE ATR[100];
	DWORD ATRsize=100;
	availableProtocol=0;
	if (QueryATR(ATR,&ATRsize,true))
	{
		availableProtocol=0;
		int iNumHistBytes=0;
		int iTotHistBytes=0;
		int y=0;
		int block=1;
		bool isHist=false;
		char let='A';
		for (unsigned int i=0;i<ATRsize && ! isHist;i++) {
			if (i==0) {
				// we are in TS
			}
			else if (i==1) {
				// we are in T0; read number of historical bytes
				let='A';
				y=ATR[i] >> 4;
				iTotHistBytes=ATR[i] & 0xF;
			}
			else {
				while ((y & 1)==0) {
					let++;
					y>>=1;
					if (y==0) {
						isHist=true;
						iNumHistBytes=1;
						i--;
						goto end;
					}
				}
				// we are in T(let)(block)
				y=y & 0xE;
				if (let=='D') {
					// check available protocol(s)
					protocol=(ATR[i] & 0x0f);
					availableProtocol|=1 << protocol;
					// prepare for next block
					let='A';
					y=ATR[i] >> 4;
					block++;
				}
			}
end:
			;
		}
		//support at least T=0
		if (availableProtocol==0)
			availableProtocol=1;
		return true;
	}
	return false;
}

DWORD Reader::startServer() {
	return 0;
}

void Reader::shutdown() {
}

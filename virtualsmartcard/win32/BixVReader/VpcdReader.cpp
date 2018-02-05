#include "internal.h"
#include "VirtualSCReader.h"
#include "reader.h"
#include "device.h"
#include <winscard.h>
#include "memory.h"
#include <Sddl.h>
#include "sectionLocker.h"
#include "../../src/vpcd/vpcd.h"


int VpcdReader::portBase;

VpcdReader::VpcdReader() {
	rpcType=2;
	state = SCARD_ABSENT;
	cardPresent = false;
	InitializeCriticalSection(&ioSection);
}

VpcdReader::~VpcdReader() {
	DeleteCriticalSection(&ioSection);
}
void VpcdReader::init(wchar_t *section) {
	portBase=GetPrivateProfileInt(L"Driver",L"RPC_PORT_BASE",VPCDPORT,L"BixVReader.ini");

	port=(short) GetPrivateProfileInt(section,L"TCP_PORT",portBase+instance,L"BixVReader.ini");
}

bool VpcdReader::CheckATR() {
	bool r = false;

	if (vicc_present((struct vicc_ctx *) ctx) == 1) {
		r = true;
	}


	if (r) {
		signalInsertion();
	}
	else {
		signalRemoval();
	}

	return r;
}
bool VpcdReader::QueryTransmit(BYTE *APDU,int APDUlen,BYTE **Resp,int *Resplen) {
	bool r = false;

	if (APDU && APDUlen && Resplen) {
		ssize_t rapdu_len = vicc_transmit((struct vicc_ctx *) ctx, APDUlen, APDU, Resp);
		if (rapdu_len > 0) {
			*Resplen = rapdu_len;
			r = true;
		} else {
			signalRemoval();
		}
	}

	return r;
}

bool VpcdReader::QueryATR(BYTE *ATR,DWORD *ATRsize,bool reset) {
	unsigned char *atr = NULL;
	int atr_len;
	bool r = false;

	if (ATR && ATRsize) {
		atr_len = vicc_getatr((struct vicc_ctx *) ctx, &atr);
		if (atr_len > 0) {
			/* TODO do length checking on length of ATR when ATRsize is
			 * correctly initialized by Reader.cpp */
			memcpy(ATR, atr, atr_len);
			*ATRsize = atr_len;
			free(atr);
			r = true;
			if (reset) {
				vicc_reset((struct vicc_ctx *) ctx);
			}
		} else {
			signalRemoval();
		}
	}

	return r;
}

DWORD VpcdReader::startServer() {
	breakSocket = false;
	ctx = vicc_init(NULL, port);
	while (!breakSocket) {
		CheckATR();
		Sleep(1000);
	}
	return 0;
}

void VpcdReader::shutdown() {
	breakSocket=true;
	WaitForSingleObject(serverThread,10000);
	serverThread = NULL;
	vicc_exit((struct vicc_ctx *) ctx);
	state=SCARD_ABSENT;
	ctx = NULL;
	if (!waitRemoveIpr.empty()) {
		SectionLocker lock(device->m_RequestLock);
		while (!waitRemoveIpr.empty()) {
			CComPtr<IWDFIoRequest> ipr = waitRemoveIpr.back();
			if (ipr->UnmarkCancelable()==S_OK)
				ipr->Complete(HRESULT_FROM_WIN32(ERROR_CANCELLED));
			waitRemoveIpr.pop_back();
		}
	}
	while (!waitInsertIpr.empty()) {
		SectionLocker lock(device->m_RequestLock);
		CComPtr<IWDFIoRequest> ipr = waitInsertIpr.back();
		if (ipr->UnmarkCancelable()==S_OK)
			ipr->Complete(HRESULT_FROM_WIN32(ERROR_CANCELLED));
		waitInsertIpr.pop_back();
	}
}

void VpcdReader::signalRemoval(void) {
	if (cardPresent) {
		cardPresent = false;
		state=SCARD_ABSENT;
		if (!waitRemoveIpr.empty()) {
			SectionLocker lock(device->m_RequestLock);
			while (!waitRemoveIpr.empty()) {
				CComPtr<IWDFIoRequest> ipr = waitRemoveIpr.back();
				if (ipr->UnmarkCancelable()==S_OK) {
					ipr->CompleteWithInformation(STATUS_SUCCESS, 0);
				}
				waitRemoveIpr.pop_back();
			}
		}
		if (!waitInsertIpr.empty()) {
			SectionLocker lock(device->m_RequestLock);
			while (!waitInsertIpr.empty()) {
				CComPtr<IWDFIoRequest> ipr = waitInsertIpr.back();
				if (ipr->UnmarkCancelable()==S_OK) {
					ipr->CompleteWithInformation(HRESULT_FROM_WIN32(ERROR_CANCELLED), 0);
				}
				waitInsertIpr.pop_back();
			}
		}
	}
}

void VpcdReader::signalInsertion(void) {
	if (!cardPresent) {
		cardPresent = true;
		while (!waitInsertIpr.empty()) {
			if (initProtocols()) {
				SectionLocker lock(device->m_RequestLock);
				while (!waitInsertIpr.empty()) {
					CComPtr<IWDFIoRequest> ipr = waitInsertIpr.back();
					if (ipr->UnmarkCancelable()==S_OK) {
						ipr->CompleteWithInformation(STATUS_SUCCESS, 0);
					}
					waitInsertIpr.pop_back();
				}
				state=SCARD_SWALLOWED;
			}
		}
	}
}

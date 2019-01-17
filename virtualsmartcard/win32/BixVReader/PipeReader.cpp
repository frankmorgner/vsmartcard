#include "internal.h"
#include "VirtualSCReader.h"
#include "reader.h"
#include "device.h"
#include <winscard.h>
#include "memory.h"
#include <Sddl.h>
#include "sectionLocker.h"

BOOL CreateMyDACL(SECURITY_ATTRIBUTES * pSA)
{
     TCHAR * szSD = TEXT("D:")       // Discretionary ACL
        TEXT("(D;OICI;GA;;;BG)")     // Deny access to
                                     // built-in guests
        TEXT("(D;OICI;GA;;;AN)")     // Deny access to
                                     // anonymous logon
        TEXT("(A;OICI;GRGWGX;;;AU)") // Allow
                                     // read/write/execute
                                     // to authenticated
                                     // users
        TEXT("(A;OICI;GA;;;BA)");    // Allow full control
                                     // to administrators

    if (NULL == pSA)
        return FALSE;

     return ConvertStringSecurityDescriptorToSecurityDescriptor(
                szSD,
                SDDL_REVISION_1,
                &(pSA->lpSecurityDescriptor),
                NULL);
}

PipeReader::PipeReader() {
	rpcType=0;
	state=SCARD_ABSENT;

	InitializeCriticalSection(&eventSection);
	InitializeCriticalSection(&dataSection);
}

void PipeReader::init(wchar_t *section) {
	wchar_t temp[300];
	swprintf(temp,L"SCardSimulatorDriver%i",instance);
	GetPrivateProfileStringW(section,L"PIPE_NAME",temp,pipeName,300,L"BixVReader.ini");
	swprintf(temp,L"SCardSimulatorDriverEvents%i",instance);
	GetPrivateProfileStringW(section,L"PIPE_EVENT_NAME",temp,pipeEventName,300,L"BixVReader.ini");

}

bool PipeReader::CheckATR() {
	if (pipe==NULL)
		return false;
	DWORD read=0;
	DWORD command=1;
	if (!WriteFile(pipe,&command,sizeof(DWORD),&read,NULL)) {
		return false;
	}
	FlushFileBuffers(pipe);
	DWORD size=0;
	if (!ReadFile(pipe,&size,sizeof(DWORD),&read,NULL)) {
		return false;
	}
	if (size==0)
		return false;
	BYTE ATR[100];
	if (!ReadFile(pipe,ATR,size,&read,NULL)) {
		return false;
	}
	return true;
}
bool PipeReader::QueryTransmit(BYTE *APDU,int APDUlen,BYTE **Resp,int *Resplen) {
	if (pipe==NULL)
		return false;
	DWORD command=2;
	DWORD read=0;
	if (!WriteFile(pipe,&command,sizeof(DWORD),&read,NULL)) {
		pipe=NULL;
		return false;
	}
	DWORD dwAPDUlen=(DWORD)APDUlen;
	if (!WriteFile(pipe,&dwAPDUlen,sizeof(DWORD),&read,NULL)) {
		pipe=NULL;
		return false;
	}
	if (!WriteFile(pipe,APDU,APDUlen,&read,NULL)) {
		pipe=NULL;
		return false;
	}
	FlushFileBuffers(pipe);
	DWORD dwRespLen=0;
	if (!ReadFile(pipe,&dwRespLen,sizeof(DWORD),&read,NULL)) {
		pipe=NULL;
		return false;
	}
	if (0 != dwRespLen) {
		BYTE *p=(BYTE *)realloc(*Resp, dwRespLen);
		if (p==NULL) {
			pipe=NULL;
			return false;
		}
		*Resp=p;
	}
	if (!ReadFile(pipe,*Resp,dwRespLen,&read,NULL)) {
		pipe=NULL;
		return false;
	}
	(*Resplen)=(int)dwRespLen;
	return true;
}

bool PipeReader::QueryATR(BYTE *ATR,DWORD *ATRsize,bool reset) {
	if (pipe==NULL)
		return false;
	DWORD command=reset ? 0 : 1;
	DWORD read=0;
	if (!WriteFile(pipe,&command,sizeof(DWORD),&read,NULL)) {
		pipe=NULL;
		return false;
	}
	FlushFileBuffers(pipe);
	DWORD size=0;
	if (!ReadFile(pipe,&size,sizeof(DWORD),&read,NULL)) {
		pipe=NULL;
		return false;
	}
	if (size==0)
		return false;
	if (!ReadFile(pipe,ATR,size,&read,NULL)) {
		pipe=NULL;
		return false;
	}
	(*ATRsize)=size;
	return true;
}

DWORD PipeReader::startServer() {
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = FALSE;
	CreateMyDACL(&sa);

	wchar_t temp[300];
	swprintf(temp,L"\\\\.\\pipe\\%s",pipeName);
	HANDLE _pipe=CreateNamedPipe(temp,PIPE_ACCESS_DUPLEX|FILE_FLAG_OVERLAPPED,PIPE_TYPE_BYTE,PIPE_UNLIMITED_INSTANCES,0,0,0,&sa);
	swprintf(temp,L"\\\\.\\pipe\\%s",pipeEventName);
	HANDLE _eventpipe=CreateNamedPipe(temp,PIPE_ACCESS_DUPLEX|FILE_FLAG_OVERLAPPED,PIPE_TYPE_BYTE,PIPE_UNLIMITED_INSTANCES,0,0,0,&sa);
	wchar_t log[300];
	swprintf(log,L"[BixVReader]Pipe created:%s:%p",pipeName,_pipe);
	OutputDebugString(log);

	while (true) {
		//__try {
			BOOL ris=ConnectNamedPipe(_pipe,NULL);
			if (ris==0) {
				swprintf(log,L"[BixVReader]Pipe NOT connected:%x",GetLastError());
				OutputDebugString(log);
			}
			else {
				swprintf(log,L"[BixVReader]Pipe connected");
				OutputDebugString(log);
				}
			ris=ConnectNamedPipe(_eventpipe,NULL);
			if (ris==0) {
				swprintf(log,L"[BixVReader]Event Pipe NOT connected:%x",GetLastError());
				OutputDebugString(log);
			}
			else {
				swprintf(log,L"[BixVReader]Event Pipe connected");
				OutputDebugString(log);
			}
			pipe=_pipe;
			eventpipe=_eventpipe;
			if (!waitInsertIpr.empty()) {
				// if I'm waiting for card insertion, verify if there's a card present
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

			while (true) {
				// wait for a command
				DWORD command=0;
				DWORD read=0;
				if (!ReadFile(eventpipe,&command,sizeof(DWORD),&read,NULL)) {
					state=SCARD_ABSENT;
					OutputDebugString(L"[BixVReader]Pipe error");
					powered=0;
					pipe=NULL;
					eventpipe=NULL;
					if (!waitRemoveIpr.empty()) {
						// card inserted
						SectionLocker lock(device->m_RequestLock);
						while (!waitRemoveIpr.empty()) {
							CComPtr<IWDFIoRequest> ipr = waitRemoveIpr.back();
							OutputDebugString(L"[BixVReader]complete Wait Remove");
							if (ipr->UnmarkCancelable()==S_OK) {
								OutputDebugString(L"[BixVReader]Wait Remove Unmarked");
								ipr->CompleteWithInformation(STATUS_SUCCESS, 0);
								OutputDebugString(L"[BixVReader]Wait Remove Completed");
							}
							waitRemoveIpr.pop_back();
						}
					}
					if (!waitInsertIpr.empty()) {
						// card removed
						SectionLocker lock(device->m_RequestLock);
						while (!waitInsertIpr.empty()) {
							CComPtr<IWDFIoRequest> ipr = waitInsertIpr.back();
							OutputDebugString(L"[BixVReader]cancel Wait Remove");
							if (ipr->UnmarkCancelable()==S_OK) {
								OutputDebugString(L"[BixVReader]Wait Insert Unmarked");
								ipr->CompleteWithInformation(HRESULT_FROM_WIN32(ERROR_CANCELLED), 0);
								OutputDebugString(L"[BixVReader]Wait Insert Cancelled");
							}
							waitInsertIpr.pop_back();
						}
					}
					DisconnectNamedPipe(_pipe);
					DisconnectNamedPipe(_eventpipe);
					break;
				}
				OutputDebugString(L"[BixVReader]Pipe data");
				if (command==0)
					powered=0;
				if (command==0 && !waitRemoveIpr.empty()) {
					// card removed
					SectionLocker lock(device->m_RequestLock);
					state=SCARD_ABSENT;
					while (!waitRemoveIpr.empty()) {
						CComPtr<IWDFIoRequest> ipr = waitRemoveIpr.back();
						if (ipr->UnmarkCancelable()==S_OK)
							ipr->CompleteWithInformation(STATUS_SUCCESS, 0);
						waitRemoveIpr.pop_back();
					}
				}
				else if (command==1 && !waitInsertIpr.empty()) {
					// card inserted
					SectionLocker lock(device->m_RequestLock);
					state=SCARD_SWALLOWED;
					initProtocols();
					while (!waitInsertIpr.empty()) {
						CComPtr<IWDFIoRequest> ipr = waitInsertIpr.back();
						if (ipr->UnmarkCancelable()==S_OK)
							ipr->CompleteWithInformation(STATUS_SUCCESS, 0);
						waitInsertIpr.pop_back();
					}
				}
			}
		//}
		//__except(EXCEPTION_EXECUTE_HANDLER) {
		//	wchar_t log[300];
		//	DWORD err=GetExceptionCode();
		//	swprintf(log,L"Exception:%08X",err);
		//	OutputDebugString(log);
		//}
	}
	OutputDebugString(L"[BixVReader]Pipe quit!!!");
	return 0;
}

void PipeReader::shutdown() {
	state=SCARD_ABSENT;
	while (!waitRemoveIpr.empty()) {
		SectionLocker lock(device->m_RequestLock);
		CComPtr<IWDFIoRequest> ipr = waitRemoveIpr.back();
		if (ipr->UnmarkCancelable()==S_OK)
			ipr->Complete(HRESULT_FROM_WIN32(ERROR_CANCELLED));
		waitRemoveIpr.pop_back();
	}
	while (!waitInsertIpr.empty()) {
		SectionLocker lock(device->m_RequestLock);
		CComPtr<IWDFIoRequest> ipr = waitRemoveIpr.back();
		if (ipr->UnmarkCancelable()==S_OK)
			ipr->Complete(HRESULT_FROM_WIN32(ERROR_CANCELLED));
		waitInsertIpr.pop_back();
	}
}

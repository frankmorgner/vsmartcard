#include "internal.h"
#include "VirtualSCReader.h"
#include "reader.h"
#include "device.h"
#include <winscard.h>
#include "memory.h"
#include <Sddl.h>
#include "sectionLocker.h"


int TcpIpReader::portBase;

TcpIpReader::TcpIpReader() {
	rpcType=1;
	state=SCARD_ABSENT;
}
void TcpIpReader::init(wchar_t *section) {
	portBase=GetPrivateProfileInt(L"Driver",L"RPC_PORT_BASE",29500,L"BixVReader.ini");

	port=GetPrivateProfileInt(section,L"TCP_PORT",portBase+(instance<<1),L"BixVReader.ini");
	eventPort=GetPrivateProfileInt(section,L"TCP_EVENT_PORT",portBase+1+(instance<<1),L"BixVReader.ini");

	InitializeCriticalSection(&eventSection);
	InitializeCriticalSection(&dataSection);
}

bool TcpIpReader::CheckATR() {
	if (AcceptSocket==NULL)
		return false;
	int read=0;
	DWORD command=1;
	if ((read=send(AcceptSocket,(char*)&command,sizeof(DWORD),NULL))<=0) {
		return false;
	}
	DWORD size=0;
	if ((read=recv(AcceptSocket,(char*)&size,sizeof(DWORD),MSG_WAITALL))<=0) {
		return false;
	}
	if (size==0)
		return false;
	BYTE ATR[100];
	if ((read=recv(AcceptSocket,(char*)ATR,size,MSG_WAITALL))<=0) {
		return false;
	}
	return true;
}
bool TcpIpReader::QueryTransmit(BYTE *APDU,int APDUlen,BYTE **Resp,int *Resplen) {
	if (AcceptSocket==NULL)
		return false;
	DWORD command=2;
	DWORD read=0;
	if ((read=send(AcceptSocket,(char*)&command,sizeof(DWORD),NULL))<=0) {
		::shutdown(AcceptSocket,SD_BOTH);
		AcceptSocket=NULL;
		return false;
	}
	DWORD dwAPDUlen=(DWORD)APDUlen;
	if ((read=send(AcceptSocket,(char*)&dwAPDUlen,sizeof(DWORD),NULL))<=0) {
		::shutdown(AcceptSocket,SD_BOTH);
		AcceptSocket=NULL;
		return false;
	}
	if ((read=send(AcceptSocket,(char*)APDU,APDUlen,NULL))<=0) {
		::shutdown(AcceptSocket,SD_BOTH);
		AcceptSocket=NULL;
		return false;
	}
	DWORD dwRespLen=0;
	if ((read=recv(AcceptSocket,(char*)&dwRespLen,sizeof(DWORD),MSG_WAITALL))<=0) {
		::shutdown(AcceptSocket,SD_BOTH);
		AcceptSocket=NULL;
		return false;
	}
	if (0 != dwRespLen) {
		BYTE *p=(BYTE *)realloc(*Resp, dwRespLen);
		if (p==NULL) {
			::shutdown(AcceptSocket,SD_BOTH);
			AcceptSocket=NULL;
			return false;
		}
		*Resp=p;
	}
	if ((read=recv(AcceptSocket,(char*)*Resp,dwRespLen,MSG_WAITALL))<=0) {
		::shutdown(AcceptSocket,SD_BOTH);
		AcceptSocket=NULL;
		return false;
	}
	(*Resplen)=(int)dwRespLen;
	return true;
}

bool TcpIpReader::QueryATR(BYTE *ATR,DWORD *ATRsize,bool reset) {
	if (AcceptSocket==NULL)
		return false;
	int read=0;
	DWORD command=reset ? 0 : 1;
	if ((read=send(AcceptSocket,(char*)&command,sizeof(DWORD),NULL))<=0) {
		::shutdown(AcceptSocket,SD_BOTH);
		AcceptSocket=NULL;
		return false;
	}
	DWORD size=0;
	if ((read=recv(AcceptSocket,(char*)&size,sizeof(DWORD),MSG_WAITALL))<=0) {
		::shutdown(AcceptSocket,SD_BOTH);
		AcceptSocket=NULL;
		return false;
	}
	if (size==0)
		return false;
	if ((read=recv(AcceptSocket,(char*)ATR,size,MSG_WAITALL))<=0) {
		::shutdown(AcceptSocket,SD_BOTH);
		AcceptSocket=NULL;
		return false;
	}
	(*ATRsize)=size;
	return true;
}

DWORD TcpIpReader::startServer() {
	breakSocket=false;

	wchar_t log[300];
	while (true) {
		//__try {
			fd_set readfds;

			FD_ZERO(&readfds);
			// Set server socket to set
			FD_SET(socket, &readfds);

			// Timeout parameter
			timeval tv = { 0 };
			tv.tv_sec = 5;

			while(true) {
				if (breakSocket)
					return 0;
				FD_SET(socket, &readfds);
				int ret = select(0, &readfds, NULL, NULL, &tv);
				if (ret > 0)
					break;
				if (ret<0) {
					DWORD err=WSAGetLastError();
					swprintf(log,L"[BixVReader]wsa err:%x",err);
					OutputDebugString(log);
					if (err==0x2736) {
						socket=WSASocket(AF_INET,SOCK_STREAM,IPPROTO_TCP,NULL,0,0);
						sockaddr_in Service;
						Service.sin_family = AF_INET;
						Service.sin_addr.s_addr = inet_addr("127.0.0.1");
						Service.sin_port = htons((u_short)(port));
						bind(socket, (SOCKADDR *) &Service, sizeof (Service));
						listen(socket, 1);

						FD_ZERO(&readfds);
						// Set server socket to set
						FD_SET(socket, &readfds);
					}
				}
			}

			SOCKET AcceptEventSocket;

			AcceptSocket = accept(socket, NULL, NULL);
			closesocket(socket);
			socket=NULL;
			if (AcceptSocket == INVALID_SOCKET)
				return 0;

			swprintf(log,L"[BixVReader]Socket connected:%i",AcceptSocket);
			OutputDebugString(log);

			FD_ZERO(&readfds);
			// Set server socket to set
			FD_SET(eventsocket, &readfds);

			while(true) {
				if (breakSocket)
					return 0;
				FD_SET(eventsocket, &readfds);
				int ret = select(0, &readfds, NULL, NULL, &tv);
				if (ret > 0)
					break;
				if (ret<0) {
					DWORD err=WSAGetLastError();
					swprintf(log,L"[BixVReader]wsa err:%x",err);
					OutputDebugString(log);
					if (err==0x2736) {
						eventsocket=WSASocket(AF_INET,SOCK_STREAM,IPPROTO_TCP,NULL,0,0);
						sockaddr_in eventService;
						eventService.sin_family = AF_INET;
						eventService.sin_addr.s_addr = inet_addr("127.0.0.1");
						eventService.sin_port = htons((u_short)(eventPort));
						bind(eventsocket, (SOCKADDR *) &eventService, sizeof (eventService));
						listen(eventsocket, 1);

						FD_ZERO(&readfds);
						// Set server socket to set
						FD_SET(eventsocket, &readfds);
					}
				}
			}

			AcceptEventSocket = accept(eventsocket, NULL, NULL);
			closesocket(eventsocket);
			eventsocket=NULL;
			if (AcceptEventSocket == INVALID_SOCKET)
				return 0;

			swprintf(log,L"[BixVReader]Event Socket connected:%i",AcceptEventSocket);
			OutputDebugString(log);

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
				int read=0;
				if ((read=recv(AcceptEventSocket,(char*)&command,sizeof(DWORD),MSG_WAITALL))<=0) {
					state=SCARD_ABSENT;
					OutputDebugString(L"[BixVReader]Socket error");
					powered=0;
					::shutdown(AcceptSocket,SD_BOTH);
					::shutdown(AcceptEventSocket,SD_BOTH);
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
					break;
				}
				OutputDebugString(L"[BixVReader]Socket data");
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
	OutputDebugString(L"[BixVReader]Socket quit!!!");
	return 0;
}

void TcpIpReader::shutdown() {
	state=SCARD_ABSENT;
	breakSocket=true;
	{
		closesocket(eventsocket);
		closesocket(socket);
	}
	WaitForSingleObject(serverThread,10000);
	serverThread=NULL;
	eventsocket=NULL;
	socket=NULL;
	while (!waitRemoveIpr.empty()) {
		SectionLocker lock(device->m_RequestLock);
		CComPtr<IWDFIoRequest> ipr = waitRemoveIpr.back();
		if (ipr->UnmarkCancelable()==S_OK)
			ipr->Complete(HRESULT_FROM_WIN32(ERROR_CANCELLED));
		waitRemoveIpr.pop_back();
	}
	while (!waitInsertIpr.empty()) {
		SectionLocker lock(device->m_RequestLock);
		CComPtr<IWDFIoRequest> ipr = waitInsertIpr.back();
		if (ipr->UnmarkCancelable()==S_OK)
			ipr->Complete(HRESULT_FROM_WIN32(ERROR_CANCELLED));
		waitInsertIpr.pop_back();
	}
}

#pragma once

#include <Winsock2.h>
#include <vector>

class CMyDevice;

class Reader {
public:
	CMyDevice *device;
	std::vector< CComPtr<IWDFIoRequest> > waitRemoveIpr;
	std::vector< CComPtr<IWDFIoRequest> > waitInsertIpr;
	inbuf;
	outbuf;

	HANDLE serverThread;

	char vendorName[300];
	char vendorIfdType[300];
	int deviceUnit;
	int powered;
	int instance;
	int rpcType;
	int state;
	DWORD protocol; // T0 or T1 - protocol in use
	DWORD availableProtocol; // T0, T1 or Both - protocols available

	void IoSmartCardGetAttribute(IWDFIoRequest* pRequest,SIZE_T inBufSize,SIZE_T outBufSize);
	void IoSmartCardIsPresent(IWDFIoRequest* pRequest,SIZE_T inBufSize,SIZE_T outBufSize);
	void IoSmartCardGetState(IWDFIoRequest* pRequest,SIZE_T inBufSize,SIZE_T outBufSize);
	void IoSmartCardIsAbsent(IWDFIoRequest* pRequest,SIZE_T inBufSize,SIZE_T outBufSize);
	void IoSmartCardPower(IWDFIoRequest* pRequest,SIZE_T inBufSize,SIZE_T outBufSize);
	void IoSmartCardSetAttribute(IWDFIoRequest* pRequest,SIZE_T inBufSize,SIZE_T outBufSize);
	void IoSmartCardSetProtocol(IWDFIoRequest* pRequest,SIZE_T inBufSize,SIZE_T outBufSize);
	void IoSmartCardTransmit(IWDFIoRequest* pRequest,SIZE_T inBufSize,SIZE_T outBufSize);

	bool initProtocols();
	virtual bool QueryTransmit(BYTE *APDU,int APDUlen,BYTE **Resp,int *Resplen);
	virtual bool QueryATR(BYTE *ATR,DWORD *ATRsize,bool reset=false);
	virtual bool CheckATR();
	virtual DWORD startServer();
	virtual void shutdown();
	virtual void init(wchar_t *section);

};

class PipeReader : public Reader {
public:
	wchar_t pipeName[300];
	wchar_t pipeEventName[300];
	HANDLE pipe;
	HANDLE eventpipe;

	PipeReader();
	bool QueryTransmit(BYTE *APDU,int APDUlen,BYTE **Resp,int *Resplen);
	bool QueryATR(BYTE *ATR,DWORD *ATRsize,bool reset=false);
	bool CheckATR();
	DWORD startServer();
	void shutdown();
	void init(wchar_t *section);

	CRITICAL_SECTION eventSection;
	CRITICAL_SECTION dataSection;
};

class TcpIpReader : public Reader {
public:
	static int portBase;
	int port;
	int eventPort;
	SOCKET socket;
	SOCKET AcceptSocket;
	SOCKET eventsocket;
	bool breakSocket;

	TcpIpReader();
	bool QueryTransmit(BYTE *APDU,int APDUlen,BYTE **Resp,int *Resplen);
	bool QueryATR(BYTE *ATR,DWORD *ATRsize,bool reset=false);
	bool CheckATR();
	DWORD startServer();
	void shutdown();
	void init(wchar_t *section);

	CRITICAL_SECTION eventSection;
	CRITICAL_SECTION dataSection;
};

class VpcdReader : public Reader {
public:
	static int portBase;
	short port;
	void *ctx;
	bool breakSocket;
	bool cardPresent;

	VpcdReader();
	~VpcdReader();
	bool QueryTransmit(BYTE *APDU,int APDUlen,BYTE **Resp,int *Resplen);
	bool QueryATR(BYTE *ATR,DWORD *ATRsize,bool reset=false);
	bool CheckATR();
	DWORD startServer();
	void shutdown();
	void init(wchar_t *section);
	void signalRemoval(void);
	void signalInsertion(void);

	CRITICAL_SECTION ioSection;
};

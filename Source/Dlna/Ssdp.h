#ifndef __SSDP_H__
#define __SSDP_H__


#include <Neptune.h>
#include "HttpServer.h"


class CSsdpInitMulticastIterator
{
public:
	CSsdpInitMulticastIterator(NPT_UdpMulticastSocket* socket)
		: m_Socket(socket)
	{}

	NPT_Result operator()(NPT_IpAddress& if_addr) const
	{
		NPT_IpAddress addr;
		addr.ResolveName("239.255.255.250");
		m_Socket->SetInterface(if_addr);
		m_Socket->LeaveGroup(addr, if_addr);
		return m_Socket->JoinGroup(addr, if_addr);
	}

private:
	NPT_UdpMulticastSocket* m_Socket;
};

class CSsdpPacketListener
{
public:
	CSsdpPacketListener() {}
	virtual ~CSsdpPacketListener() {}
	virtual NPT_Result OnSsdpPacket(const NPT_DataBuffer& buf, const NPT_SocketAddress& from_addr) = 0;
};

class CSsdpSearchResponseListener
{
public:
	CSsdpSearchResponseListener() {}
	virtual ~CSsdpSearchResponseListener() {}
	virtual NPT_Result OnSsdpSerchResponse(NPT_Result res, const NPT_HttpRequestContext& conext, NPT_HttpResponse* response) = 0;
};


class CSsdpPacketListenerIterator
{
public:
	CSsdpPacketListenerIterator(const NPT_DataBuffer& buf, const NPT_SocketAddress& from_addr)
		: m_Buf(buf)
		, m_FromAddr(from_addr)
	{}

	NPT_Result operator()(CSsdpPacketListener*& listener) const
	{
		return listener->OnSsdpPacket(m_Buf, m_FromAddr);
	}

private:
	const NPT_DataBuffer& m_Buf;
	const NPT_SocketAddress& m_FromAddr;
};


class CSsdpListenTask : public CThreadTask
{
public:
	CSsdpListenTask(NPT_UdpMulticastSocket* socket, bool stay_alive_forever = false);

	NPT_Result AddListerner(CSsdpPacketListener* listener);
	NPT_Result RemoveListener(CSsdpPacketListener* listener);

protected:
	virtual ~CSsdpListenTask();

	virtual void DoAbort() { m_Socket->Cancel(); }
	virtual void DoRun();

protected:
	NPT_List<CSsdpPacketListener*> m_Listeners;
	NPT_Mutex m_Mutex;
	bool m_StayAliveForever;
	NPT_UdpMulticastSocket* m_Socket;
	NPT_UInt16 m_Port;
};


//////////////////////////////////////////////////////////////////////////
// class CSsdpSearchTask
class CSsdpSearchTask : public CThreadTask
{
public:
	CSsdpSearchTask(NPT_UdpSocket* socket, CSsdpSearchResponseListener* listener, NPT_HttpRequest* request, NPT_TimeInterval frequency = NPT_TimeInterval(0.0));

protected:
	virtual ~CSsdpSearchTask();

	// CThreadTask methods
	virtual void DoAbort() { m_Socket->Cancel(); }
	virtual void DoRun();

private:
	void Report(NPT_Result rt);
	void Report(const NPT_DataBuffer& data);

private:
	CSsdpSearchResponseListener* m_Listener;
	NPT_HttpRequest* m_Request;
	NPT_UdpSocket* m_Socket;
	NPT_IpAddress m_Addr;
	NPT_TimeInterval m_Frequency;
	bool m_Repeat;
};

#endif	// __SSDP_H__
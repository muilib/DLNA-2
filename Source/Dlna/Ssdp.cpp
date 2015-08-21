#include "Ssdp.h"



//////////////////////////////////////////////////////////////////////////
// class CSsdpListenTask
CSsdpListenTask::CSsdpListenTask(NPT_UdpMulticastSocket* socket, bool stay_alive_forever /* = false */)
	: m_Socket(socket)
	, m_StayAliveForever(stay_alive_forever)
{
	// Change read time out for UDP because iPhone 3.0 seems to hang
	// after reading everything from the socket even though
	// more stuff arrived
#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
	m_Socket->SetReadTimeout(10000);
#endif
}

CSsdpListenTask::~CSsdpListenTask()
{
	if (m_Socket) delete m_Socket;
}

NPT_Result CSsdpListenTask::AddListerner(CSsdpPacketListener* listener)
{
	NPT_AutoLock lock(m_Mutex);
	m_Listeners.Add(listener);
	return NPT_SUCCESS;
}

NPT_Result CSsdpListenTask::RemoveListener(CSsdpPacketListener* listener)
{
	NPT_AutoLock lock(m_Mutex);
	m_Listeners.Remove(listener);
	return NPT_SUCCESS;
}


void CSsdpListenTask::DoRun()
{
	NPT_Result result;
	NPT_DataBuffer packet(4096 * 4);
	NPT_SocketAddress from_addr;

	while (!IsAborting(0))
	{
		result = m_Socket->Receive(packet, &from_addr);
		if (!m_StayAliveForever)
			break;

		if (NPT_SUCCEEDED(result))
		{
			//char* data = reinterpret_cast<char*>(packet.UseData());
			//*(data + packet.GetDataSize()) = 0;
			m_Listeners.Apply(CSsdpPacketListenerIterator(packet, from_addr));
		}
		else
		{
			NPT_System::Sleep(1.0);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
// CSsdpSearchTask
CSsdpSearchTask::CSsdpSearchTask(NPT_UdpSocket* socket, CSsdpSearchResponseListener* listener, NPT_HttpRequest* request, NPT_TimeInterval frequency /* = NPT_TimeInterval */) 
	: m_Socket(socket)
	, m_Listener(listener)
	, m_Frequency(frequency ? frequency : NPT_TimeInterval(3.0))
	, m_Repeat(frequency.ToSeconds() != 0)
{
	m_Socket->SetReadTimeout((NPT_Timeout)m_Frequency.ToMillis());
	m_Socket->SetWriteTimeout(10000);
}

CSsdpSearchTask::~CSsdpSearchTask()
{
	delete m_Socket;
	delete m_Request;
}

void CSsdpSearchTask::DoRun()
{
	NPT_Result res;
	NPT_DataBuffer packet(4096 * 4);
	NPT_SocketAddress from_addr;
	res = m_Socket->Bind(NPT_SocketAddress(m_Addr, 0));
	if (NPT_FAILED(res))
		return Report(res);
	
	NPT_SocketAddress target_addr(NPT_IpAddress(239, 255, 255, 250), 1900);
	//NPT_String req = NPT_String::Format("M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nMAN: \"ssdp:discover\"\r\nMX: %d\r\nST: %s\r\nXMPP:%s:5222\r\n\r\n", m_mx, m_st.GetChars(), m_Addr.ToString());

	NPT_Timeout timeout = 30000;
	NPT_HttpResponse* response = nullptr;
	NPT_HttpRequestContext context;

	while (!IsAborting(0))
	{
		// read response
		//NPT_InputStreamReference stream
		NPT_Result res;
		m_Listener->OnSsdpSerchResponse(res, context, response);
	}
}

void CSsdpSearchTask::Report(NPT_Result rt)
{
}

void CSsdpSearchTask::Report(const NPT_DataBuffer& data)
{

}
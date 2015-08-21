#include "UPnP.h"



//////////////////////////////////////////////////////////////////////////
// CUPnP_CtrlPointStartIter
class CUPnP_CtrlPointStartIter
{
public:
	CUPnP_CtrlPointStartIter(CSsdpListenTask* listen_task) 
		: m_ListenTask(listen_task)
	{}
	virtual ~CUPnP_CtrlPointStartIter()
	{}
	NPT_Result operator()(CControlPoint* ctrl_point) const
	{
		ctrl_point->Start(m_ListenTask);
		return NPT_SUCCESS;
	}

private:
	CSsdpListenTask* m_ListenTask;
};


//////////////////////////////////////////////////////////////////////////
// CUPnP
CUPnP::CUPnP()
	: m_Started(false)
	, m_IgnoreLocalUUID(false)
	, m_SsdpListenTask(NULL)
{
}

CUPnP::~CUPnP()
{
	Stop();
	m_CtrlPoints.Clear();
}


NPT_Result CUPnP::Start()
{
	NPT_AutoLock lock(m_Lock);
	if (m_Started) return NPT_SUCCESS;

	NPT_List<NPT_IpAddress> ips;
	UPNPMessageHelper::GetIPAddresses(ips);
	NPT_UdpMulticastSocket* socket = new NPT_UdpMulticastSocket();
	socket->Bind(NPT_SocketAddress(NPT_IpAddress::Any, 1900), true);
	ips.ApplyUntil(CSsdpInitMulticastIterator(socket), NPT_UntilResultNotEquals(NPT_SUCCESS));

	m_SsdpListenTask = new CSsdpListenTask(socket, true);
	m_TaskManager = new CThreadManager();
	if (NPT_FAILED(m_TaskManager->StartTask(m_SsdpListenTask))) return NPT_FAILURE;

	// start devices & control points
	m_CtrlPoints.Apply(CUPnP_CtrlPointStartIter(m_SsdpListenTask));

	m_Started =true;
	return NPT_SUCCESS;
}

NPT_Result CUPnP::Stop()
{
	NPT_AutoLock lock(m_Lock);
	if (m_Started == false) return NPT_ERROR_INVALID_STATE;

	m_TaskManager->StopAllTasks();
	m_TaskManager = nullptr;
	m_SsdpListenTask = nullptr;

	m_Started = false;
	return NPT_SUCCESS;
}

NPT_Result CUPnP::AddDevice()
{
	return NPT_SUCCESS;
}

NPT_Result CUPnP::AddCtrlPoint(CControlPoint* ctrlpoint)
{
	NPT_AutoLock lock(m_Lock);

	if (m_Started)
		ctrlpoint->Start(m_SsdpListenTask);

	m_CtrlPoints.Add(ctrlpoint);
	return NPT_SUCCESS;
}

NPT_Result CUPnP::RemoveDevice()
{
	return NPT_SUCCESS;
}

NPT_Result CUPnP::RemoveCtrlPoint(CControlPoint* ctrlpoint)
{
	NPT_AutoLock lock(m_Lock);
	if (m_Started)
		ctrlpoint->Stop(m_SsdpListenTask);

	return m_CtrlPoints.Remove(ctrlpoint);
}
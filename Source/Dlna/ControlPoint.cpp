#include "ControlPoint.h"
#include "Utilities.h"


//////////////////////////////////////////////////////////////////////////
// CControlPoint
CControlPoint::CControlPoint(const char* search_criteria /* = "upnp:rootdevice" */)
	: m_TaskManager(nullptr)
	, m_SearchCriteria(search_criteria)
	, m_Started(false)
{

}

CControlPoint::~CControlPoint()
{

}



NPT_Result CControlPoint::Start(CSsdpListenTask* task)
{
	m_TaskManager = new CThreadManager();
	m_TaskManager->StartTask(new CControlPointHouseKeeping(this));

	task->AddListerner(this);

	m_Started = true;
	return SsdpSearch(m_SearchCriteria, 15);
}

NPT_Result CControlPoint::Stop(CSsdpListenTask* task)
{
	m_Started = false;

	task->RemoveListener(this);
	m_TaskManager->StopAllTasks();
	m_TaskManager = nullptr;

	return NPT_SUCCESS;
}

NPT_Result CControlPoint::SsdpSearch(const char* target /* = "upnp:rootdevice" */, NPT_UInt32 mx /* = 5 */)
{
	if (!m_Started)
		return NPT_ERROR_INVALID_STATE;

	NPT_Result res;
	NPT_List<NPT_NetworkInterface*> if_list;
	NPT_List<NPT_NetworkInterface*>::Iterator if_iter;
	NPT_List<NPT_NetworkInterfaceAddress>::Iterator if_addr_iter;

	res = UPNPMessageHelper::GetNetworkInterfaces(if_list, true);
	if (NPT_FAILED(res)) return res;

	return NPT_SUCCESS;
}

NPT_Result CControlPoint::DoHouseKeeping()
{
	return NPT_SUCCESS;
}

NPT_Result CControlPoint::OnSsdpPacket(const NPT_DataBuffer& buf, const NPT_SocketAddress& from_addr)
{
	printf("%s\n", from_addr.ToString());
	return NPT_SUCCESS;
}

NPT_Result CControlPoint::OnSsdpSerchResponse(NPT_Result res, const NPT_HttpRequestContext& conext, NPT_HttpResponse* response)
{
	return NPT_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////
// CControlPointHouseKeeping
CControlPointHouseKeeping::CControlPointHouseKeeping(CControlPoint* ctrlpoint, NPT_TimeInterval timer /* = NPT_TimeInterval */)
	: m_CtrlPoint(ctrlpoint)
	, m_Timer(timer)
{
}

CControlPointHouseKeeping::~CControlPointHouseKeeping()
{
}

void CControlPointHouseKeeping::DoRun()
{
	while (!IsAborting((NPT_Timeout)m_Timer.ToSeconds()*1000))
	{
		if (m_CtrlPoint)
			m_CtrlPoint->DoHouseKeeping();
	}
}
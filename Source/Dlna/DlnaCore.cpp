#include "DlnaCore.h"

CDlnaCore::CDlnaCore(CDlnaCoreProxy* pCallback)
	: m_Callback(pCallback)
	, m_UPnp(new CUPnP())
	, m_CtrlPoint(new CControlPoint())
{
}

CDlnaCore::~CDlnaCore()
{
}

void CDlnaCore::Start()
{
	m_UPnp->AddCtrlPoint(m_CtrlPoint);
	m_UPnp->Start();
}

void CDlnaCore::Stop()
{
	m_UPnp->RemoveCtrlPoint(m_CtrlPoint);
	m_UPnp->Stop();
}

void CDlnaCore::SetProperty(const NPT_String& name, const NPT_String& value)
{

}

bool CDlnaCore::GetProperty(const NPT_String& name, NPT_String& value) const
{
	return true;
}

void CDlnaCore::ImportFileToMediaServer(const NPT_List<NPT_String>& dirs, const NPT_List<NPT_String>& names, bool ignoreDot /* = false */)
{

}

void CDlnaCore::ClearMediaServerContent()
{

}

void CDlnaCore::EnableFuntion(Function fun, bool enable)
{

}

void CDlnaCore::FlushDeviceList(FlushMode fm /* = FM_ALL */)
{

}

void CDlnaCore::SerchDevices(NPT_UInt32 mx /* = 3 */)
{

}
#include "DlnaProxy.h"

CDlnaProxy* CDlnaProxy::m_pInstance = NULL;

CDlnaProxy::CDlnaProxy()
{
	m_Dlna = new CDlnaCore(this);
	m_Dlna->Start();
}

CDlnaProxy::~CDlnaProxy()
{
	m_Dlna->Stop();
	delete m_Dlna;
	m_Dlna = NULL;
}

CDlnaProxy* CDlnaProxy::GetInstance()
{
	if (!m_pInstance)
	{
		m_pInstance = new CDlnaProxy();
	}
	return m_pInstance;
}

void CDlnaProxy::Stop()
{
	m_Dlna->Stop();
}


void CDlnaProxy::ImportFileToMediaServer(const NPT_List<NPT_String>& dirs, const NPT_List<NPT_String>& names)
{
	m_Dlna->ClearMediaServerContent();
	m_Dlna->ImportFileToMediaServer(dirs, names, false);
}

void CDlnaProxy::SetControlPoint(bool enable /* = true */)
{
	m_Dlna->EnableFuntion(Fun_ControlPoint, enable);
}

void CDlnaProxy::SetMediaServer(bool enable /* = true */)
{
	m_Dlna->EnableFuntion(Fun_MediaServer, enable);
}

void CDlnaProxy::SetMediaRender(bool enable /* = true */)
{
	m_Dlna->EnableFuntion(Fun_MediaRender, enable);
}


//////////////////////////////////////////////////////////////////////////
// CDlnaCore Callback methond
void CDlnaProxy::OnMediaServerListChanged()
{

}

void CDlnaProxy::OnMediaRenderListChanged()
{

}

void CDlnaProxy::DmrOpen(const NPT_String& url, const NPT_String& mimeType, const NPT_String& metaData)
{

}

void CDlnaProxy::DmrPlay()
{

}


void CDlnaProxy::DmrPause()
{

}

void CDlnaProxy::DmrStop()
{

}

void CDlnaProxy::DmrSeekTo(NPT_Int64 timeInMillis)
{

}

void CDlnaProxy::DmrSetMute(bool mute)
{

}

void CDlnaProxy::DmrSetVolume(int volume)
{

}
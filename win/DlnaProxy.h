#ifndef __DLNAPROXY_H__
#define __DLNAPROXY_H__

#include "DlnaCore.h"

class CDlnaProxy : public CDlnaCoreProxy
{
public:
	static CDlnaProxy* GetInstance();

	void Start();
	void Stop();
	void ImportFileToMediaServer(const NPT_List<NPT_String>& dirs, const NPT_List<NPT_String>& names);
	void SetMediaServer(bool enable = true);
	void SetMediaRender(bool enable = true);
	void SetControlPoint(bool enable = true);

private:
	CDlnaProxy();
	~CDlnaProxy();

protected:
	// Callback
	virtual void OnMediaServerListChanged();
	virtual void OnMediaRenderListChanged();
	virtual void DmrOpen(const NPT_String& url, const NPT_String& mimeType, const NPT_String& metaData);
	virtual void DmrPlay();
	virtual void DmrPause();
	virtual void DmrStop();
	virtual void DmrSeekTo(NPT_Int64 timeInMillis);
	virtual void DmrSetMute(bool mute);
	virtual void DmrSetVolume(int volume);

private:
	static CDlnaProxy* m_pInstance;
	CDlnaCore* m_Dlna;
};

#endif	// __DLNAPROXY_H__
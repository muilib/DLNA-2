#ifndef __DLNACORE_H__
#define __DLNACORE_H__

#include "Global.h"
#include "UPnP.h"
#include "ControlPoint.h"


class CDlnaCoreProxy
{
public:
	virtual void OnMediaServerListChanged() = 0;
	virtual void OnMediaRenderListChanged() = 0;
	virtual void DmrOpen(const NPT_String& url, const NPT_String& mimeType, const NPT_String& metaData) = 0;
	virtual void DmrPlay() = 0;
	virtual void DmrPause() = 0;
	virtual void DmrStop() = 0;
	virtual void DmrSeekTo(NPT_Int64 timeInMillis) = 0;
	virtual void DmrSetMute(bool mute) = 0;
	virtual void DmrSetVolume(int volume) = 0;
};


class CDlnaCore
{
public:
	CDlnaCore(CDlnaCoreProxy* pCallback);
	~CDlnaCore();

	void SetProperty(const NPT_String& name, const NPT_String& value);
	bool GetProperty(const NPT_String& name, NPT_String& value) const;

	void ImportFileToMediaServer(const NPT_List<NPT_String>& dirs, const NPT_List<NPT_String>& names, bool ignoreDot = false);
	void ClearMediaServerContent();
	void Start();
	void Stop();
	void EnableFuntion(Function fun, bool enable);
	void FlushDeviceList(FlushMode fm = FM_ALL);
	void SerchDevices(NPT_UInt32 mx = 3);

private:
	CDlnaCoreProxy* m_Callback;
	CUPnP* m_UPnp;
	CControlPoint* m_CtrlPoint;
};



#endif	// __DLNACORE_H__
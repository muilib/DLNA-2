#ifndef __UPNP_H__
#define __UPNP_H__


#include "ThreadTask.h"
#include "ControlPoint.h"
#include "Ssdp.h"
#include "Utilities.h"


class CUPnP
{
public:
	CUPnP();
	~CUPnP();

	NPT_Result AddDevice();
	NPT_Result AddCtrlPoint(CControlPoint* ctrlpoint);
	NPT_Result RemoveDevice();
	NPT_Result RemoveCtrlPoint(CControlPoint* ctrlpoint);

	NPT_Result Start();
	NPT_Result Stop();

	bool IsRunning() { return m_Started; }
	void SetIgnoreLocalUUID(bool ignore) { m_IgnoreLocalUUID = ignore; }


private:
	NPT_Mutex m_Lock;
	NPT_List<CControlPoint*> m_CtrlPoints;
	CThreadManager* m_TaskManager;
	CSsdpListenTask* m_SsdpListenTask;
	bool m_Started;
	bool m_IgnoreLocalUUID;
};

#endif	// __UPNP_H__
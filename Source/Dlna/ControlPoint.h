#ifndef __CONTROL_POINT_H__
#define __CONTROL_POINT_H__


#include "Neptune.h"
#include "Ssdp.h"


//////////////////////////////////////////////////////////////////////////
// forward declarations
class CControlPointHouseKeeping;
class CSsdpListenTask;
class CSsdpSearchTask;


//////////////////////////////////////////////////////////////////////////
// class CControlPoint
class CControlPoint 
	: public CSsdpPacketListener
	, public CSsdpSearchResponseListener
{
public:
	CControlPoint(const char* search_criteria = "upnp:rootdevice");
	virtual ~CControlPoint();

	// delegation
	
	// discovery
	void IgnoreUUID(const char* uuid);

	NPT_Result SsdpSearch(const char* target = "upnp:rootdevice", NPT_UInt32 mx = 5);
	NPT_Result Discover(const char* target = "ssdp:all", NPT_UInt32 mx = 5);
	
	// actions

	// events

	// listener method
	virtual NPT_Result OnSsdpPacket(const NPT_DataBuffer& buf, const NPT_SocketAddress& from_addr);
	virtual NPT_Result OnSsdpSerchResponse(NPT_Result res, const NPT_HttpRequestContext& conext, NPT_HttpResponse* response);

protected:
	// methods
	virtual NPT_Result Start(CSsdpListenTask* task);
	virtual NPT_Result Stop(CSsdpListenTask* task);

	NPT_Result DoHouseKeeping();

private:
	friend class CUPnP;
	friend class CUPnP_CtrlPointStartIter;
	friend class CUPnP_CtrlPointStopIter;
	friend class CControlPointHouseKeeping;

	CThreadManager* m_TaskManager;
	NPT_Mutex m_Lock;
	NPT_String m_SearchCriteria;
	bool m_Started;
};


//////////////////////////////////////////////////////////////////////////
// class CControlPointHouseKeeping
class CControlPointHouseKeeping : public CThreadTask
{
public:
	CControlPointHouseKeeping(CControlPoint* ctrlpoint, NPT_TimeInterval timer = NPT_TimeInterval(5.0));

protected:
	~CControlPointHouseKeeping();

	virtual void DoRun();

protected:
	CControlPoint* m_CtrlPoint;
	NPT_TimeInterval m_Timer;
};

#endif	// __CONTROL_POINT_H__
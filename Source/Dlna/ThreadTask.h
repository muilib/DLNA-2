#ifndef __THREADTASK_H__
#define __THREADTASK_H__

#include <Neptune.h>

class CThreadTask;
class CThreadManager;


class CThreadTask : public NPT_Runnable
{
public:
	friend class CThreadManager;

	NPT_Result Kill();

protected:
	virtual bool IsAborting(NPT_Timeout timeout) { return NPT_SUCCEEDED(m_Abort.WaitUntilEquals(1, timeout)); }
	NPT_Result Start(CThreadManager* manager = NULL, NPT_TimeInterval* delay = NULL, bool auto_destroy = true);
	NPT_Result Stop(bool blocking = true);

	virtual void DoInit() {}
	virtual void DoAbort() {}
	virtual void DoRun() {}

	CThreadTask();
	virtual ~CThreadTask();

private:
	NPT_Result StartThread();
	void Run();

protected:
	CThreadManager* m_Manager;

private:
	NPT_SharedVariable m_Started;
	NPT_SharedVariable m_Abort;
	NPT_Thread* m_Thread;
	NPT_TimeInterval m_Delay;
	bool m_AutoDestroy;
};


class CThreadManager
{
public:
	CThreadManager(NPT_Cardinal max_tasks = 0);
	virtual ~CThreadManager();

	virtual NPT_Result StartTask(CThreadTask* task, NPT_TimeInterval* delay = NULL, bool auto_destroy = true);
	NPT_Result StopAllTasks();
	NPT_Cardinal GetMaxTasks() { return m_MaxTasks; }

private:
	friend class CThreadTask;
	NPT_Result AddTask(CThreadTask* task);
	NPT_Result RemoveTask(CThreadTask* task);

private:
	NPT_List<CThreadTask*> m_Tasks;
	NPT_Queue<int>* m_Queue;
	NPT_Mutex m_TasksLock;
	NPT_Mutex m_CallbackLock;
	NPT_Cardinal m_MaxTasks;
	NPT_Cardinal m_RunningTasks;
	bool m_bStopping;
};


#endif	// __THREADTASK_H__
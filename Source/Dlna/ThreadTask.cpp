#include "ThreadTask.h"

//////////////////////////////////////////////////////////////////////////
// class CThreadManager
CThreadManager::CThreadManager(NPT_Cardinal max_tasks /* = 0 */)
	: m_Queue(NULL)
	, m_MaxTasks(max_tasks)
	, m_RunningTasks(0)
	, m_bStopping(false)
{

}

CThreadManager::~CThreadManager()
{
	StopAllTasks();
}

NPT_Result CThreadManager::StartTask(CThreadTask* task, NPT_TimeInterval* delay /* = NULL */, bool auto_destroy /* = true */)
{
	//NPT_CHECK_POINTER_SEVERE(task);
	return task->Start(this, delay, auto_destroy);
}

NPT_Result CThreadManager::StopAllTasks()
{
	NPT_Cardinal num_running_tasks;
	do 
	{
		// delee queue
		{
			NPT_AutoLock lock(m_TasksLock);
			m_bStopping = true;
			if (m_Queue)
			{
				int * val = NULL;
				while (NPT_SUCCEEDED(m_Queue->Pop(val, 0)))
					delete val;

				delete m_Queue;
				m_Queue = NULL;
			}
		}

		// abort all running tasks
		{
			NPT_AutoLock lock(m_TasksLock);
			NPT_List<CThreadTask*>::Iterator task = m_Tasks.GetFirstItem();
			while (task)
			{
				if (!(*task)->IsAborting(0))
					(*task)->Stop(false);
				++task;
			}
			num_running_tasks = m_Tasks.GetItemCount();
		}

		if (num_running_tasks == 0)
			break;

		NPT_System::Sleep(NPT_TimeInterval(0.05));
	} while (1);

	m_bStopping = false;
	return NPT_SUCCESS;
}

NPT_Result CThreadManager::AddTask(CThreadTask* task)
{
	NPT_Result result = NPT_SUCCESS;
	int * val = NULL;

	do 
	{
		m_TasksLock.Lock();
		if (m_bStopping)
		{
			m_TasksLock.Unlock();
			delete val;
			if (task->m_AutoDestroy) delete task;
			//NPT_CHECK_WARNING(NPT_ERROR_INTERRUPTED);
		}

		if (m_MaxTasks)
		{
			val = val ? val : new int;
			if (!m_Queue)
				m_Queue = new NPT_Queue<int>(m_MaxTasks);

			result = m_Queue->Push(val, 20);
			if (NPT_SUCCEEDED(result)) break;

			m_TasksLock.Unlock();

			if (result != NPT_ERROR_TIMEOUT)
			{
				delete val;
				if (task->m_AutoDestroy) delete task;
				//NPT_CHECK_WARNING(result);
			}
		}
	} while (result == NPT_ERROR_TIMEOUT);

	// start task now
	if (NPT_FAILED(result = task->StartThread()))
	{
		m_TasksLock.Unlock();
		RemoveTask(task);
		return result;
	}

	result = m_Tasks.Add(task);
	m_TasksLock.Unlock();
	return result;
}


NPT_Result CThreadManager::RemoveTask(CThreadTask* task)
{
	NPT_Result result = NPT_SUCCESS;

	NPT_AutoLock lock(m_TasksLock);
	if (m_Queue)
	{
		int * val = NULL;
		result = m_Queue->Pop(val, 100);
		if (NPT_SUCCEEDED(result))
			delete val;
	}

	m_Tasks.Remove(task);

	if (task->m_AutoDestroy) delete task;

	return NPT_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////
// class CThreadTask
CThreadTask::CThreadTask()
	: m_Manager(NULL)
	, m_Thread(NULL)
	, m_AutoDestroy(false)
{
}

CThreadTask::~CThreadTask()
{
	if (!m_AutoDestroy) delete m_Thread;
}


NPT_Result CThreadTask::Start(CThreadManager* manager /* = NULL */, NPT_TimeInterval* delay /* = NULL */, bool auto_destroy /* = true */)
{
	m_Abort.SetValue(0);
	m_AutoDestroy = auto_destroy;
	m_Delay = delay ? *delay : NPT_TimeStamp(0.0);
	m_Manager = manager;

	if (m_Manager)
	{
		m_Manager->AddTask(this);
		return NPT_SUCCESS;
	}
	else
	{
		NPT_Result result = StartThread();
		if (NPT_FAILED(result) && m_AutoDestroy) delete this;
		return result;
	}
}

NPT_Result CThreadTask::StartThread()
{
	m_Started.SetValue(0);
	m_Thread = new NPT_Thread((NPT_Runnable &)*this, m_AutoDestroy);
	NPT_Result result = m_Thread->Start();
	if (NPT_FAILED(result))
	{
		if (m_AutoDestroy)
		{
			delete m_Thread;
			m_Thread = NULL;
		}
		//NPT_CHECK_FATAL(result);
	}

	return m_Started.WaitUntilEquals(1, NPT_TIMEOUT_INFINITE);
}

NPT_Result CThreadTask::Stop(bool blocking /* = true */)
{
	bool auto_destroy = m_AutoDestroy;
	m_Abort.SetValue(1);
	DoAbort();

	if (!blocking || !m_Thread) return NPT_SUCCESS;

	return auto_destroy ? NPT_FAILURE : m_Thread->Wait();
}

NPT_Result CThreadTask::Kill()
{
	Stop();
	NPT_ASSERT(m_AutoDestroy == false);
	if (!m_AutoDestroy) delete this;

	return NPT_SUCCESS;
}

void CThreadTask::Run()
{
	m_Started.SetValue(1);

	if ((float)m_Delay > 0.f)
	{
		// more than 100ms, loop so we can abort it
		if ((float)m_Delay > 0.1f)
		{
			NPT_TimeStamp start, now;
			NPT_System::GetCurrentTimeStamp(start);
			do 
			{
				NPT_System::GetCurrentTimeStamp(now);
				if (now >= start + m_Delay) break;
			} while (!IsAborting(100));
		}
		else
		{
			NPT_System::Sleep(m_Delay);
		}
	}

	// loop
	if (!IsAborting(0))
	{
		DoInit();
		DoRun();
	}

	if (m_Manager)
		m_Manager->RemoveTask(this);
	else if (m_AutoDestroy)
		delete this;
}
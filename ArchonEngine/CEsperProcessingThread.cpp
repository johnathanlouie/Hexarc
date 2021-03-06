//	CEsperProcessingThread.cpp
//
//	CEsperProcessingThread class
//	Copyright (c) 2013 by Kronosaur Productions, LLC. All Rights Reserved.

#include "stdafx.h"

DECLARE_CONST_STRING(IOCP_SOCKET_OP,					"IOCP.socketOp")

DECLARE_CONST_STRING(MSG_LOG_DEBUG,						"Log.debug")
DECLARE_CONST_STRING(MSG_LOG_ERROR,						"Log.error")

DECLARE_CONST_STRING(WORKER_SIGNAL_SHUTDOWN,			"Worker.shutdown")
DECLARE_CONST_STRING(WORKER_SIGNAL_PAUSE,				"Worker.pause")

class CPauseThreadEvent : public IIOCPEntry
	{
	public:
		CPauseThreadEvent (CEsperProcessingThread *pThread) : IIOCPEntry(WORKER_SIGNAL_PAUSE),
				m_pThread(pThread)
			{ }

	protected:

		//	IIOCPEntry overrides

		virtual void OnProcess (void) {	m_pThread->Stop(); }

	private:
		CEsperProcessingThread *m_pThread;
	};

class CQuitThreadEvent : public IIOCPEntry
	{
	public:
		CQuitThreadEvent (CEsperProcessingThread *pThread) : IIOCPEntry(WORKER_SIGNAL_SHUTDOWN),
				m_pThread(pThread)
			{ }

	protected:

		//	IIOCPEntry overrides

		virtual void OnProcess (void) { m_pThread->SetQuit(); }

	private:
		CEsperProcessingThread *m_pThread;
	};

void CEsperProcessingThread::Mark (void)

//	Mark
//
//	Mark all AEON data in use

	{
	}

void CEsperProcessingThread::Run (void)

//	Run
//
//	Runs

	{
	try
		{
		//	Create a couple of special entries for stopping the thread
		//	and quitting the app.

		m_pPauseSignal = new CPauseThreadEvent(this);
		m_pQuitSignal = new CQuitThreadEvent(this);

		//	Process Events

		while (true)
			{
			try
				{
				m_pEngine->ProcessConnections();
				}
			catch (...)
				{
				m_pEngine->Log(MSG_LOG_ERROR, CString("Crash in processing IO completion port."));
				}

			//	If we need to quit, do it

			if (m_bQuit)
				break;
			}

		//	Done

		delete m_pPauseSignal;
		m_pPauseSignal = NULL;
		delete m_pQuitSignal;
		m_pQuitSignal = NULL;
		}
	catch (...)
		{
		m_pEngine->Log(MSG_LOG_ERROR, CString("Crash in Esper processing thread."));
		}
	}

void CEsperProcessingThread::Stop (void)

//	Stop
//
//	Stop the thread while we garbage collect

	{
	m_PausedEvent.Set();

	CWaitArray Events;
	int iQUIT_EVENT = Events.Insert(m_pEngine->GetQuitEvent());
	int iRUN_EVENT = Events.Insert(m_pEngine->GetRunEvent());

	int iEvent = Events.WaitForAny();
	if (iEvent == iQUIT_EVENT)
		{
		m_bQuit = true;
		return;
		}
	else
		m_PausedEvent.Reset();
	}

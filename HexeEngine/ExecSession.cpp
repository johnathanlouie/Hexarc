//	ExecSession.cpp
//
//	ExecSession class
//	Copyright (c) 2011 by George Moromisato. All Rights Reserved.

#include "stdafx.h"

DECLARE_CONST_STRING(PORT_HEXE_COMMAND,					"Hexe.command")

DECLARE_CONST_STRING(MSG_ERROR_UNABLE_TO_COMPLY,		"Error.unableToComply")
DECLARE_CONST_STRING(MSG_REPLY_DATA,					"Reply.data")

DECLARE_CONST_STRING(LIBRARY_CORE,						"core")

DECLARE_CONST_STRING(ERR_COMPILER,						"HexeLisp compiler error: %s.")
DECLARE_CONST_STRING(ERR_UNABLE_TO_PARSE_CODE,			"Unable to parse input.")

const DWORD MESSAGE_TIMEOUT =							30 * 1000;

class CRunSession : public ISessionHandler
	{
	public:
		CRunSession (void) : m_iState(stateNone) { }

	protected:
		//	ISessionHandler virtuals
		virtual void OnMark (void);
		virtual bool OnProcessMessage (const SArchonMessage &Msg);
		virtual bool OnStartSession (const SArchonMessage &Msg, DWORD dwTicket);

	private:
		enum States
			{
			stateNone,
			stateWaitingForHexarcReply,
			};

		bool HandleResult (CHexeProcess::ERunCodes iRun, CDatum dResult);

		States m_iState;
		CHexeProcess m_Process;
	};

void CHexeEngine::MsgRun (const SArchonMessage &Msg, const CHexeSecurityCtx *pSecurityCtx)

//	MsgRun
//
//	Hexe.run {code}

	{
	StartSession(Msg, new CRunSession);
	}

//	CRunSession ----------------------------------------------------------------

bool CRunSession::HandleResult (CHexeProcess::ERunCodes iRun, CDatum dResult)

//	HandleResult
//
//	Handles the result from a run

	{
	switch (iRun)
		{
		case CHexeProcess::runOK:
		case CHexeProcess::runError:
			SendMessageReply(MSG_REPLY_DATA, dResult);
			//	FALSE means we're done with the session
			return false;

		case CHexeProcess::runAsyncRequest:
			{
			SendMessageCommand(dResult.GetElement(0), 
					dResult.GetElement(1), 
					GenerateAddress(PORT_HEXE_COMMAND),
					dResult.GetElement(2),
					MESSAGE_TIMEOUT);

			m_iState = stateWaitingForHexarcReply;

			return true;
			}

		default:
			//	LATER:
			SendMessageReplyError(MSG_ERROR_UNABLE_TO_COMPLY, CString("LATER"));
			return false;
		}
	}

void CRunSession::OnMark (void)

//	OnMark
//
//	Mark data in use

	{
	m_Process.Mark();
	}

bool CRunSession::OnProcessMessage (const SArchonMessage &Msg)

//	OnProcessMessage
//
//	Handle a message

	{
	//	If we a reply, then process it

	if (m_iState == stateWaitingForHexarcReply)
		{
		//	continue with execution

		CDatum dResult;
		CHexeProcess::ERunCodes iRun = m_Process.RunContinues(CSimpleEngine::MessageToHexeResult(Msg), &dResult);

		//	Handle it

		return HandleResult(iRun, dResult);
		}

	//	Otherwise we are done.

	return false;
	}

bool CRunSession::OnStartSession (const SArchonMessage &Msg, DWORD dwTicket)

//	OnStartSession
//
//	Start the session

	{
	CDatum dCode = Msg.dPayload.GetElement(0);

	//	Initialize the process

	m_Process.LoadLibrary(LIBRARY_CORE);

	//	Parse into an expression (depending on the type of input)

	CDatum dExpression;
	if (dCode.GetBasicType() == CDatum::typeString)
		{
		CString sError;
		if (!CHexeDocument::ParseLispExpression(dCode, &dExpression, &sError))
			{
			SendMessageReplyError(MSG_ERROR_UNABLE_TO_COMPLY, strPattern(ERR_COMPILER, sError));
			return false;
			}
		}

	//	Otherwise we don't know how to parse the input

	else
		{
		SendMessageReplyError(MSG_ERROR_UNABLE_TO_COMPLY, ERR_UNABLE_TO_PARSE_CODE);
		return false;
		}

	//	Run the code

	CDatum dResult;
	CHexeProcess::ERunCodes iRun = m_Process.Run(dExpression, &dResult);

	//	Deal with the result

	return HandleResult(iRun, dResult);
	}

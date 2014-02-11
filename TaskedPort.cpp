// TaskedPort.cpp: implementation of the CTaskedPort class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TaskedPort.h"
#include "Logger.h"
#include "Stringtokenizer.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTaskedPort::CTaskedPort()
{
}

CTaskedPort::~CTaskedPort()
{
	if(m_pFGTask)
	{
		m_pFGTask->AbortTask();
		m_pFGTask = NULL;
	}
	
	if(m_pBGTask)
	{
		m_pBGTask->AbortTask();
		m_pBGTask = NULL;
	}
	
}

void CTaskedPort::OnEvRxChar()
{
	CSingleLock sl(&m_Mutex);
	sl.Lock();
	//�����ַ�
	
	CString sInput;
	sInput = ReadAll();
	TRACE("\r\nreceived<<%s\r\n", sInput);
	TRACE("\r\n��ǰTASK %s...\r\n", m_pFGTask ? m_pFGTask->m_sTaskName : "<��>");
	TRACE("m_pFGTask == %p\r\n", m_pFGTask);
	
	//���ȴ���ǰ̨����
	if(m_pFGTask)
	{
		TRACE("\r\n����TASK %s...\r\n", m_pFGTask->m_sTaskName);
		if(m_pFGTask->Catch(sInput) != -1)
		{
			//����Ȩת����̨����
			m_pFGTask = NULL;
			
			if(m_pBGTask)
				m_pBGTask->ResetTask();
		}
		return;
	}
	
	//�����̨����
	if(m_pBGTask)
	{
		if(m_pBGTask->Catch(sInput) != -1)
		{
			//����������
			m_pBGTask->ResetTask();
		}
	}
}

CPortTask::CPortTask(CString sTaskName)
{
	m_hCompleteEvent = NULL;
	m_pOwner = NULL;
	m_pOwnerPort = NULL;
	m_nLimit = 0xFFFF;
	m_sTaskName = sTaskName;

	ResetTask();
}

CPortTask::~CPortTask()
{
	EndTask();
}

BOOL CTaskedPort::Open(CString sPort)
{
	if(!CSerialPort::Open(sPort))
		return FALSE;
	
	StartListener();
	return TRUE;
}

void CTaskedPort::UnloadTask(CPortTask *pTask)
{
	CSingleLock sl(&m_Mutex);
	sl.Lock();
	
	if(pTask == m_pFGTask)
		m_pFGTask = NULL;
	
	if(pTask == m_pBGTask)
		m_pBGTask = NULL;
}

int CPortTask::AddMatch(CString sMatch, int nValue)
{
	m_sMatches.Add(sMatch);
	int nWhich = m_nMatchCodes.Add(nValue);
	
	return nWhich;
}

int CPortTask::AddMatch(CString sMatch)
{
	return AddMatch(sMatch, m_nMatchCodes.GetSize());
}

int CPortTask::AddMatches(CString sMatches)
{
	CStringTokenizer st(sMatches, "|");
	CStringArray * psa = st.GetTokens();
	int nCount = 0;
	
	for(int i = 0; i < psa->GetSize(); i++)
	{
		if(AddMatch(psa->GetAt(i)))
			nCount++;
	}
	
	return nCount;
}

DWORD CTaskedPort::SendCommand(CString sCommand, DWORD nIdleTimeLimit)
{
	//ASSERT(sCommand.Right(1) == "\r" || sCommand.Right(1) == 26);
	TRACE("\r\nsend>>" + sCommand + "\r\n");

	::Logger.Log("��������>>%s", sCommand);

	return Write(sCommand, nIdleTimeLimit);
}

int CTaskedPort::Request(CString sRequest, CString & sResponse, int nEndCondition, CWaiting * pWaiting, char * sFind, CString sTaskName, DWORD nIdleTimeLimit)
{
	AssertConnected();
	Purge(PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	///////////////////////////////
	////����TASK
	//////////////////////////////

	if(sTaskName.IsEmpty())
		sTaskName = sRequest;

	CPortTask Task(sTaskName);
	
	if(nEndCondition & ecOkOrError)
		Task.AddMatches("ERROR|OK");
	
	if(nEndCondition & ecCrlf)
		Task.AddMatch("\r");
	
	if(sFind)
		Task.AddMatches(sFind);
	
	//����ֹͣ��־
	if(Task.GetMatches()->GetSize())
		nEndCondition |= ecWaiting;
	else
		nEndCondition &= ~ecWaiting;
	
	if(nEndCondition & ecDirty)
		nEndCondition |= ecWaiting;
	
	///////////////////////////////
	////����TASK
	//////////////////////////////

	LoadFGTask(&Task);

	//��������
	if(SendCommand(sRequest, nIdleTimeLimit) <= 0)
	{
		return -1;
	}

	WaitForTask(&Task, nEndCondition, pWaiting, nIdleTimeLimit);

	sResponse = Task.GetInputBuffer();
	::Logger.Log("��������<<%s", sResponse);

	return Task.GetMatchCode();
}

CString CTaskedPort::Request(CString sRequest, int nEndCondition, CWaiting * pWaiting, char * sFind, CString sTaskName, DWORD nIdleTimeLimit)
{
	CString sResponse;
	Request(sRequest, sResponse, nEndCondition, pWaiting, sFind, sTaskName, nIdleTimeLimit);
	
	return sResponse;
}

void CTaskedPort::LoadBGTask(CPortTask *pTask)
{
	CSingleLock sl(&m_Mutex);
	sl.Lock();
	
	if(m_pBGTask)
	{
		TRACE("������:%s\r\n", m_pBGTask->m_sTaskName);
		TRACE("������:%s\r\n", pTask->m_sTaskName);
		
		m_pBGTask->AbortTask();
	}
	
	m_pBGTask = pTask;
	
	if(pTask)
	{
		pTask->SetOwner(this);
		pTask->ResetTask();
	}
}

void CTaskedPort::LoadFGTask(CPortTask *pTask)
{
	CSingleLock sl(&m_Mutex);
	sl.Lock();
	
	if(m_pFGTask)
	{
		TRACE("������:%s\r\n", m_pFGTask->m_sTaskName);
		TRACE("������:%s\r\n", pTask->m_sTaskName);

		::Logger.Log("���棺������[%s]��������[%s] ������ͻ��", 
			m_pFGTask->m_sTaskName, 
			pTask->m_sTaskName);
		
		m_pFGTask->AbortTask();
	}
	
	m_pFGTask = pTask;
	TRACE("����ǰ̨TASK: %s\r\n", pTask->m_sTaskName);
	TRACE("m_pFGTask == %p\r\n", m_pFGTask);
	
	if(pTask)
	{
		pTask->SetOwner(this);
		pTask->ResetTask();
	}
}

void CPortTask::OnTaskCompleted()
{
	//do nothing...
}

BOOL CPortTask::IsCompleted()
{
	CSingleLock sl(&m_Mutex);
	sl.Lock();

	return m_bCompleted;
}

void CPortTask::ResetTask()
{
	m_bCompleted = FALSE;
	m_sFullInput.Empty();
	
	if(m_hCompleteEvent)
		::ResetEvent(m_hCompleteEvent);
	
	m_nMatchIndex = -1;
	m_nDirty = 0;
	
}

int CPortTask::Catch(CString sInput)
{
	TRACE("Catch %s\r\n", sInput);

	CSingleLock sl(&m_Mutex);
	sl.Lock();

	m_nDirty++;

	if(m_nDirty >= 0x8000)
	{
		m_nDirty = 0;
	}
	
	CString sLastInput = m_sFullInput;
	m_sFullInput += sInput;
	
	//�ӵ�����Ϣ
	if(m_nLimit > 0 && m_sFullInput.GetLength() > m_nLimit)
	{
		m_sFullInput.Delete(0, m_sFullInput.GetLength() - m_nLimit);
	}
	
	for(int i = 0; i < m_sMatches.GetSize(); i++)
	{
		//����ָ�����ַ���
		CString sMatch = m_sMatches.GetAt(i);
		CString sCatch = sLastInput.Right(sMatch.GetLength()) + sInput;
		
		//�ҵ�ƥ��
		if(sCatch.Find(sMatch) != -1)
		{
			{
				m_nMatchIndex = i;
				break;;
			}
		}
	}
	
	//����ƥ����
	int nCode = GetMatchCode();
	if(nCode != -1)
	{
		CompleteTask();
	}
	
	return nCode;
}

void CPortTask::AttachEvent(HANDLE hEvent)
{
	m_hCompleteEvent = hEvent;
}

void CPortTask::AttachEvent(CWnd *pOwner, UINT nMessageID)
{
	m_pOwner = pOwner;
	m_nNotifyMessageID = nMessageID;
}

int CPortTask::GetMatchCode()
{
	if(!m_sMatches.GetSize())
		return 0;
	
	if(m_nMatchIndex == -1)
		return -1;
	
	return m_nMatchCodes.GetAt(m_nMatchIndex);
}

void CPortTask::CompleteTask()
{
	TRACE("TASK completed!\r\n");
	m_bCompleted = TRUE;
	OnTaskCompleted();
	
	//�����������������Է�ֹCPortTask������ǰ����
	if(m_pOwner)
	{
		m_pOwner->PostMessage(m_nNotifyMessageID, (WPARAM)this, (LPARAM)GetMatchCode());
	}
	
	if(m_hCompleteEvent)
	{
		::SetEvent(m_hCompleteEvent);
	}	
}

void CPortTask::SetOwner(CTaskedPort *pOwnerPort)
{
	m_pOwnerPort = pOwnerPort;
}

void CPortTask::EndTask()
{
	if(m_pOwnerPort)
	{
		m_pOwnerPort->UnloadTask(this);
	}
}

void CPortTask::AbortTask()
{
	EndTask();
}

void CPortTask::SetBufferSize(int nLimit)
{
	m_nLimit = nLimit;
}

int CPortTask::GetDirty()
{
	return m_nDirty;
}

CString CPortTask::GetInputBuffer()
{
	return m_sFullInput;
}

CStringArray * CPortTask::GetMatches()
{
	return &m_sMatches;
}

void CTaskedPort::Close()
{
	UnloadTask(m_pBGTask);
	UnloadTask(m_pFGTask);

	CSerialPort::Close();
}

void CTaskedPort::WaitForTask(CPortTask *pTask, int nEndCondition, CWaiting * pWaiting, DWORD nIdleTimeLimit)
{
	int nDirty = 0;
	DWORD nStartTime = GetTickCount();
	
	while(!pTask->IsCompleted())
	{
		AssertConnected();

		//����Ҫ�ȴ�
		if(!(nEndCondition & ecWaiting))
			break;
		
		//������
		int nNewDirty = pTask->GetDirty();
		if(nDirty != nNewDirty)
		{
			nDirty = nNewDirty;
			nStartTime = GetTickCount();
			
			//��ʾ����
			pWaiting->OnWaitChip(true);
			
			//�������ݼ�����
			if(nEndCondition & ecDirty)
				break;
		}

		//û������
		else
		{
			pWaiting->OnWaitChip(false);
			
			//�Ƿ���ʱ�ޣ�
			if(nIdleTimeLimit != -1)
			{
				DWORD nEndTime = GetTickCount();
				
				//��ʱ
				if(nEndTime - nStartTime > nIdleTimeLimit)
				{
					//��ʾ�Ƿ�����ȴ�
					if(nEndCondition & ecRetryPrompt)
					{
						if(::MessageBox(NULL, "���ݵȴ�ʱ��������Ƿ�����ȴ���", 
							"�ȴ���ʱ", 
							MB_YESNO | MB_ICONQUESTION) == IDNO)
						{
							throw new CTimeoutException();
						}
						//�����ȴ�
						else
						{
							continue;
						}
					}
					
					//������ʱ
					if(!(nEndCondition & ecRetryAlways))
					{
						::Logger.Log("�ȴ���ʱ����");
						throw new CTimeoutException();
					}
				} // if(nEndTime - nStartTime > nIdleTimeLimit)
			} //if(nIdleTimeLimit != -1)
		} //if(nDirty == nNewDirty)
	} //while
	
	pWaiting->OnWaitChip(true);
}

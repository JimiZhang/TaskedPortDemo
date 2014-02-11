// TaskedPort.h: interface for the CTaskedPort class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TASKEDPORT_H__55962DCB_4955_4ACA_A605_74F41A464656__INCLUDED_)
#define AFX_TASKEDPORT_H__55962DCB_4955_4ACA_A605_74F41A464656__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define IDU_TASK_COMPLETE	(WM_USER + 101)

#include "SerialPort.h"
#include "RichException.h"
#include "Waiting.h"

/*
CTaskedPort������
�û�����ָ�������Ŀ�꣬�������ҵ�һ��ƥ���ַ����Լ���Ӧ��ƥ����
�������ʱ����ͨ������;��֪ͨ�û���

  1. �����ڷ�����Ϣ��
  2. �����¼�Ϊ���źţ�
  3. ����OnTaskCompleted()������
  4. �򵥵����ó�Ա����m_bCompleted��
  
*/
class CTaskedPort;

class CPortTask
{
protected:
	
	CStringArray m_sMatches;
	CByteArray m_nMatchCodes;
	int m_nLimit;
	HANDLE m_hCompleteEvent;
	BOOL m_bCompleted;
	CWnd * m_pOwner;
	UINT m_nNotifyMessageID;

	CMutex m_Mutex;
	
	int m_nMatchIndex;
	int m_nDirty; //�����������ÿ�ν��յ����ݣ��ü����Զ���1
	
	CTaskedPort * m_pOwnerPort;
	CString m_sFullInput; //�������������ַ�
	
	void EndTask(); //������ǰ����
	void CompleteTask(); //��ɵ�ǰ����
	
public:
	CStringArray * GetMatches();
	
	CString m_sTaskName;
	CString GetInputBuffer();
	
	int GetDirty();
	
	void SetBufferSize(int nLimit); //���û���������
	void AbortTask(); //ȡ����ǰ����
	void ResetTask(); //���õ�ǰ����
	
	void SetOwner(CTaskedPort * pOwnerPort); //��������CTaskedPort
	int GetMatchCode(); //��ȡƥ����
	
	CPortTask(CString sTaskName);
	~CPortTask();
	
	void AttachEvent(CWnd * pOwner, UINT nMessageID = IDU_TASK_COMPLETE); //������Ϣ֪ͨ��ʽ
	void AttachEvent(HANDLE hEvent); //�����¼�֪ͨ��ʽ
	
	int Catch(CString sInput); //�����ַ���
	
	BOOL IsCompleted(); //ָ���������Ƿ������
	
	virtual void OnTaskCompleted();
	
	int AddMatches(CString sMatches); //��Ӷ��ƥ���ƥ����֮����|�ָ�
	int AddMatch(CString sMatch); //���ƥ����
	int AddMatch(CString sMatch, int nValue); //���ƥ�����ָ��ƥ����
	
};

/*
֧������Ĵ��ڲ�����
���������̨�����ǰ̨����
ͬʱֻ����һ�����񱻼���
�������ǰ̨���񣬺�̨�����Զ�����
�����µ����񣬻��Զ����Ǿɵ�����
��̨����һֱ����
*/

class CTaskedPort : public CSerialPort  
{
private:
	
	CPortTask * m_pBGTask; //��̨����
	CPortTask * m_pFGTask; //ǰ̨����

	
public:

	void WaitForTask(CPortTask *pTask, int nEndCondition, CWaiting * pWaiting, DWORD nIdleTimeLimit);
	void Close();
	
	//�ȴ�ʱ��
	enum
	{
			tmShort = 20,
			tmNormal = 1000,
			tmInitialize = 3000,
			tmLong = 6000,
			tmLonger = 20000,
	};
	
	//ָ��GetInput��������Ϊ
	enum
	{
		//ָ��GetInput���õ���ֹ����
		
		//0-3: ƥ������
		
		ecDirty = 0x0001, //��ֵ������
			ecWaiting = 0x0002, //���ȴ�
			ecOkOrError = 0x0004, //��OK����ERROR������
			ecCrlf = 0x0008, //�лس�������
			
			//4-7: ָ�������ȴ���ʱʱGetInput�Ķ���ѡ��
			
			ecRetryAlways = 0x0100, //һֱ�ȴ�
			ecRetryPrompt = 0x0200, //��ʾ�û��Ƿ����
	};
	
	CMutex m_Mutex;
	
	BOOL Open(CString sPort);
	int Request(CString sRequest, CString & sResponse, int nEndCondition, CWaiting * pWaiting, char * sFind = NULL, CString sTaskName = "", DWORD nIdleTimeLimit = tmNormal);
	DWORD SendCommand(CString sCommand, DWORD nIdleTimeLimit = tmNormal);
	CString Request(CString sRequest, int nEndCondition, CWaiting * pWaiting, char * sFind = NULL, CString sTaskName = "", DWORD nIdleTimeLimit = tmNormal);
	
	void UnloadTask(CPortTask * pTask);
	
	void LoadFGTask(CPortTask * pTask);
	void LoadBGTask(CPortTask * pTask);
	
	CString m_sInputBuffer;
	
	void OnEvRxChar();
	CTaskedPort();
	virtual ~CTaskedPort();
};

#endif // !defined(AFX_TASKEDPORT_H__55962DCB_4955_4ACA_A605_74F41A464656__INCLUDED_)

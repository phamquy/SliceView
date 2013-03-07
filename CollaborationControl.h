#pragma once

#include <string>
#include <vector>
#include <stack>
#include "cvsp.h"


#define SOCKET_MESSAGE	  				700					// 
#define SM_NOTIFICATION	  				SOCKET_MESSAGE + 5	// FD_READ, FD_WRITE, FD_CLOSE, FD_OOB
#define SM_ACCEPT						SOCKET_MESSAGE + 6	// FD_ACCEPT
#define WM_COLLABORATION_CONNECTSUCCESS	SOCKET_MESSAGE + 7	// FD_ACCEPT
#define WM_COLLABORATION_FILEDONE		SOCKET_MESSAGE + 8	// FD_ACCEPT
#define WM_COLLABORATION_DIRECTORY		SOCKET_MESSAGE + 9	// FD_ACCEPT
#define WM_COLLABORATION_CONTROLRESULT	SOCKET_MESSAGE + 10	// FD_ACCEPT
#define WM_COLLABORATION_OPERATION_START	SOCKET_MESSAGE + 11	// FD_ACCEPT
#define WM_COLLABORATION_OPERATION_RUN		SOCKET_MESSAGE + 12	// FD_ACCEPT
#define WM_COLLABORATION_OPERATION_GETSTART	SOCKET_MESSAGE + 13	// FD_ACCEPT
#define WM_COLLABORATION_OPERATION_GETRUN	SOCKET_MESSAGE + 14	// FD_ACCEPT



//#define WM_COLLABORATION_CONNECT	SOCKET_MESSAGE + 7	// FD_ACCEPT



typedef struct _controlinfo_t
{
	CWnd	*pWnd ;
	bool	bControl ;
	char	szControlUser[50] ;
	char	szControlID[MAX_PATH] ;
}ControlInfo ;



class CCollaborationClient //: CWnd
{
public:
	CCollaborationClient(CWnd*			pwnd);
	~CCollaborationClient(void);

	bool				JoinServer(LPCSTR ID, LPCSTR IPAddress) ;
	static DWORD WINAPI ListenThread(LPVOID temp) ;

protected:

private:
	int					InitSocketLayer() ;
	void				CloseSocketLayer() ;
	void				Receive() ;
	void				Close() ;
	
	
public:
	// Operation Method
	bool				IsControl() ;
	bool				IsConnect() ;
	bool				IsStart() ;
	bool				FloorControRequest(int index) ;
	bool				FloorControRelease(int index) ;
	void				OperationStart(int index, long x, long y) ;
	void				OperationRun(int index, long x, long y) ;
	void				OperationRun(int index, double xRoatae, double yRotate) ;
	void				OperationEnd() ;
	void				Leave() ;
	void				Start(LPCSTR fileinfo) ;
	// Handle Messge
	void				SetParentHandle(HWND hWnd) ;
	void 				GetOperation(Operation* pOperation) ;
	LPCSTR				GetFileName() ;
	std::vector<std::string> &GetFileInfoLists() ;
	std::string ReadZIP(std::string UserName, std::string FilePath, std::string FileExtension, std::string Password) ;
	std::string GetUserName() ;
	std::vector<ControlInfo> &GetControlInfoLists() ;
	FileInfo			&GetFileInfo() ;
	bool				StartDownload() ;
	bool				NextDownload() ;
	void				SetControlInfo(CWnd *pView) ;
	


private:
	// Connection States.
	SOCKET			m_Sock ;
	bool			m_Connect ;
	bool			m_Authenticated ;
	bool			m_ControlQuestion ;
	bool			m_Control ;
	bool			m_Start ;
	std::string		m_ID ;
	HANDLE			m_ListenHandle ;
	DWORD			m_ListenID ;
	std::vector<std::string> m_FilInfoLists; 
	std::vector<FileDownInfo> m_DownLoadLists; 
	std::vector<FileDownInfo>::iterator m_DownLoadIter ;
	std::vector<ControlInfo> m_ControlLists ;
	std::string		m_Directory ;
	bool			m_bDownLoad ;
	
	FileInfo		m_FileInfo ;
	FILE			*m_fpOSG ;

	// handle for CTRL class.
	HWND			m_hWnd ;
	CWnd*			m_pwnd ;
	Operation		m_Operation ;

};

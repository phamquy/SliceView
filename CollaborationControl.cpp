#include "StdAfx.h"
#include "CollaborationControl.h"
#include "cvsp.h"
#include "unzip.h"



class CCollaborationClient ;

CCollaborationClient::CCollaborationClient(CWnd* pwnd)
{
	m_pwnd = pwnd ;
	m_Connect = false ;
	m_Control = false ;
	m_Authenticated = false ;
	m_ID	  = "" ;
	m_ControlQuestion = true ;
	m_Start = false ;

	m_bDownLoad = false ;
	
		
	InitSocketLayer() ;
}


CCollaborationClient::~CCollaborationClient(void)
{
	
}

bool CCollaborationClient::JoinServer(LPCSTR ID, LPCSTR IPAddress)
{
	if(m_Connect)
	{
		return true ;
	}
	
	m_Sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) ;
	
	if(m_Sock == INVALID_SOCKET)
	{
		WSACleanup() ;
		return false ;
	}

	sockaddr_in service ;
	service.sin_family = AF_INET ;
	service.sin_addr.s_addr = inet_addr((const char*)IPAddress) ;
	service.sin_port = htons(CVSP_PORT) ;

	/*if(bind(m_Sock, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR)
	{
		closesocket(pServer->m_Sock) ;
		return -1 ;
	}*/

	if(connect(m_Sock, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR)
	{
		closesocket(m_Sock) ;
		return false ;
	}

	m_ID = (const char*)ID ;
	
	// Send to server the "JoinReq" message.
	unsigned char	cmd ;
	unsigned char	option ;

	cmd = CVSP_JOINREQ ;
	option = CVSP_SUCCESS ;
	
	sendCVSP(m_Sock, cmd, option, (void*)m_ID.c_str(), strlen(m_ID.c_str())) ;
	m_Connect = true ;    
	
	//WSAAsyncSelect(m_Sock, m_hWnd, SM_NOTIFICATION, FD_READ|FD_CLOSE) ;
	m_ListenHandle = CreateThread(NULL, 0, CCollaborationClient::ListenThread, this, 0, &m_ListenID) ;
	return true ;
}

int CCollaborationClient::InitSocketLayer()
{
	int retval = 0 ;
	// Initialize the winsock environment
	WORD ver_request = MAKEWORD( 2, 2 );
	WSADATA wsa_data;

	// Initialize the winsock environment.
	if ( WSAStartup( ver_request, &wsa_data ) )
	{
		// Failed to startup WinSock
		return -1;
	}
	// Confirm the winsock environment supports at least version 2.2.
	long nMajor = LOBYTE( wsa_data.wVersion );
	long nMinor = HIBYTE( wsa_data.wVersion );
	if ( 2 < nMajor )
	{
		// We can't support anything with a major value under 2.  Goodbye.
		retval = -1;
	}
	else if ( 2 == nMajor )
	{
		// Make sure that the minor value is at least 2
		if ( 2 > nMinor )
		{
			retval = -1;
		}
	}
	// All other versions should work.  Let's hope that future versions don't break
	// the application.
	else
	{
		//std::cerr << "The version of winsock is newer than what we expected." << std::endl;
	}
	if ( 0 < retval )
	{
		WSACleanup();
	}

	return retval; 
}

void CCollaborationClient::CloseSocketLayer()
{
	WSACleanup() ;
}

void CCollaborationClient::Receive()
{
/*
	static char 	extraPacket[CVSP_STANDARD_PAYLOAD_LENGTH];
	unsigned char	cmd ;
	unsigned char	option ;
	int				len ;

	if( (len = recvCVSP(m_Sock, &cmd, &option, extraPacket, CVSP_STANDARD_PAYLOAD_LENGTH)) < 0)
	{
		return;
	}

	switch(cmd)
	{
		case CVSP_JoinRes:
			if(option == CVSP_SUCCESS)
			{
				
				AfxMessageBox("Success!") ;
			}
			else if(option == CVSP_FAILE)
			{

			}
		break ;
		case CVSP_FlowControlRes:
			
		break ;
		case CVSP_MonitoringMSG:
			
		break ;
	}
*/
}

void CCollaborationClient::Close()
{
	closesocket(m_Sock) ;
	m_Connect = false ;
}

// Thread for Listen thread of CactiVTO_Client class. It likes java or c# threads.
DWORD WINAPI CCollaborationClient::ListenThread(LPVOID temp)
{
	CCollaborationClient *pClient = (CCollaborationClient *)temp ;

	// Protocol Options.
	char 			extraPacket[CVSP_STANDARD_PAYLOAD_LENGTH - sizeof(CVSPHeader_t)];
	unsigned char	cmd ;
	unsigned char	option ;
	int				len ;

	// Select varaiables.
	fd_set	fdReadSet, fdErrorSet, fdMaster ;
	struct	timeval	tvs ;
	
	FD_ZERO(&fdMaster) ;
	FD_SET(pClient->m_Sock, &fdMaster) ;
	tvs.tv_sec = 0;
	tvs.tv_usec = 0;

	// Authenticate manage
	fdReadSet = fdMaster ;
	fdErrorSet = fdMaster ;
	select((int)pClient->m_Sock + 1, &fdReadSet, NULL, &fdErrorSet, NULL) ;
	
	CString tmp ;


	if(FD_ISSET(pClient->m_Sock, &fdReadSet))
	{
		// Read Operation
		// so, use the CVSP Function
		if( (len = recvCVSP((unsigned int)pClient->m_Sock, &cmd, &option, extraPacket, sizeof(extraPacket)))< 0)
		{
			pClient->m_Connect = false ;
			//;
		}

		switch(cmd)
		{
			case CVSP_JOINRES:
				if(option == CVSP_SUCCESS)
				{
					// 서버와의 연결이 성공함 여기서 파일 정보를 추출 한다.
					if(!pClient->m_FilInfoLists.empty())
						pClient->m_FilInfoLists.clear() ;

					char fileinfodata[1024] ;
					memset(fileinfodata, 0, sizeof(fileinfodata)) ;
					strncpy_s(fileinfodata, sizeof(fileinfodata), extraPacket, len) ;
					char seps[] = "|" ;
					char *token = NULL ;
					char *next_token = NULL ;
					
					token = strtok_s(fileinfodata, seps, &next_token) ;
					while(token != NULL)
					{
						pClient->m_FilInfoLists.push_back(token) ;
						token = strtok_s(NULL, seps, &next_token) ;
					}
					
					pClient->m_Authenticated = true ;
					// 서버와 연결이 이루어 졌음을 알려준다.
					//PostMessage(pClient->m_hWnd, WM_COLLABORATION_CONNECTSUCCESS, NULL, NULL) ; 
					
					pClient->m_pwnd->PostMessage(WM_COLLABORATION_CONNECTSUCCESS, NULL) ;
					
					//AfxMessageBox("Connect Success!") ;
				}
				else if(option == CVSP_FAILE)
				{
					// login failed, so close connection.
					pClient->m_Connect = false ;
					closesocket(pClient->m_Sock) ;
				}
			break ;
			
			default:
			break ;
		}
	}
	else
	{
		// error;
		AfxMessageBox((LPCTSTR)"Connection Error!") ;
		return 0;
	}
	
	//cmd = CVSP_VTOFILEREQ ;
	//option = CVSP_SUCCESS ;
	//int id = 0 ;
	//sendCVSP((unsigned int)pClient->m_Sock, cmd, option, &id, sizeof(id)) ;

	
	int recvSize =  0 ;
	// Listen meesage from server
	tvs.tv_usec = 200;
	while(pClient->m_Authenticated)
	{
		fdReadSet = fdMaster ;
		fdErrorSet = fdMaster ;
		select((int)pClient->m_Sock + 1, &fdReadSet, NULL, &fdErrorSet, &tvs) ;
		if(FD_ISSET(pClient->m_Sock, &fdReadSet))
		{	// Read Operation
			// so, use the CVSP Function
			if( (len = recvCVSP((unsigned int)pClient->m_Sock, &cmd, &option, extraPacket, CVSP_STANDARD_PAYLOAD_LENGTH))< 0)
			{
				// error
				continue ;
			}
			switch(cmd)
			{
				case CVSP_VTODIRECTORYRES:
					if(option == CVSP_SUCCESS)
					{
						char fileinfodata[2048] ;
						memset(fileinfodata, 0, sizeof(fileinfodata)) ;
						strncpy_s(fileinfodata, sizeof(fileinfodata), extraPacket, len) ;
						char seps[] = "|" ;
						char *token = NULL ;
						char *next_token = NULL ;

						pClient->m_DownLoadLists.clear() ;

						while(token != NULL)
						{
							FileDownInfo down ;
							memset(down.szFileName, 0, sizeof(down.szFileName)) ;
							strncpy_s(down.szFileName, sizeof(down.szFileName), token, strlen(token)) ;
							down.status = CVSP_FILE_NOT ;

							pClient->m_DownLoadLists.push_back(down) ;
							token = strtok_s(NULL, seps, &next_token) ;
						}
						
						pClient->m_pwnd->PostMessage(WM_COLLABORATION_DIRECTORY, NULL) ;

	
					}
				break;
			
			
				case CVSP_VTOFILERES:
					if(option == CVSP_FILE_START)
					{
						//memset(&pClient->m_FileInfo, 0, sizeof(pClient->m_FileInfo)) ;
						memcpy(&pClient->m_FileInfo, extraPacket, sizeof(pClient->m_FileInfo) );
						char temp[1024] ;
						

						// 데이터 파일을 받은 정보에서 마지막 데이터만 읽어 낸다.
						char seps[] = "\\" ;
						char *token ;
						char *nexttoken ;
						std::stack<std::string> filestack ;
							
				

						//pClient->m_FileInfo.szCollaborationID ;
						//pClient->m_FileInfo.szFileName ;
						memset(temp, 0, sizeof(temp)) ;
						strncpy_s(temp, sizeof(temp), pClient->m_FileInfo.szFileName, strlen(pClient->m_FileInfo.szFileName) ) ;
												
						token = strtok_s(temp, seps, &nexttoken) ;
						while(token != NULL)
						{
							
							//pClient->m_FilInfoLists.push_back(token) ;
							token = strtok_s(NULL, seps, &nexttoken) ;
							if(token == NULL)
								break ;
							
							filestack.push(token) ;
						}
						
						char temptail[5] ;		
						//memset(temp, 0, sizeof(temp)) ;
						//strncpy_s(temp, sizeof(temp), filestack.top().c_str(), strlen(filestack.top().c_str()) - 4) ;
						//memset(temptail, 0, sizeof(temptail)) ;
						//strncpy_s(temptail, sizeof(temptail), filestack.top().c_str() + (strlen(filestack.top().c_str()) - 4), 4) ;
						//sprintf_s(pClient->m_FileInfo.szFileName, sizeof(pClient->m_FileInfo.szFileName), "%s_%s%s", temp, pClient->m_ID.c_str(), temptail) ;
						sprintf_s(pClient->m_FileInfo.szFileName, sizeof(pClient->m_FileInfo.szFileName), "%s", filestack.top().c_str()) ;
						

						while(filestack.empty() != false)
						{
							filestack.pop() ;
						}
						//AfxMessageBox(pClient->m_FileInfo.szFileName) ;

						fopen_s(&pClient->m_fpOSG, pClient->m_FileInfo.szFileName, "wb") ;
						
						if(pClient->m_fpOSG == NULL)
						{
							AfxMessageBox((LPCTSTR)"File Open Error!") ;
						}
					}
					else if(option == CVSP_FILE_RUN)
					{
						if(len > 0)
						{
							//AfxMessageBox(temp) ;
							//recvSize += len ;
							recvSize += fwrite(extraPacket, sizeof(char), len, pClient->m_fpOSG) ;
						}

					}
					else if(option == CVSP_FILE_END)
					{
						//recvSize += len ;
						recvSize += fwrite(extraPacket, sizeof(char), len, pClient->m_fpOSG) ;
						fclose(pClient->m_fpOSG) ;

						//CString temp ;
						//temp.Format("size[%d]", recvSize) ;
						//AfxMessageBox(temp) ;
						pClient->m_pwnd->PostMessage(WM_COLLABORATION_FILEDONE, NULL) ;


						// Now send a message to Ctrl class for OSG loading.
						//SendMessage(pClient->m_hWnd, CVSP_MONITORING_LOAD, NULL, NULL) ; 
					}

				break ;
				case CVSP_FLOORCONTROLRES:
				{	
					CollaborationInfo info ;
					int i = 0  ;
					int index = 0  ;
					memcpy(&info, extraPacket, sizeof(info) );
					bool find = false ;
					std::vector<ControlInfo>::iterator controlitr ;


					if(option == CVSP_SUCCESS || option == CVSP_REQUEST)
					{
						//pClient->m_Control = true ;
						
						controlitr = pClient->m_ControlLists.begin() ;


						while(controlitr != pClient->m_ControlLists.end())
						{
							if(strcmp(controlitr->szControlID, info.szCollaborationID) == 0)
							{
								if(strcmp(pClient->m_ID.c_str(), info.szUserID) == 0)
								{
									controlitr->bControl = true ;
								}
								//
								strcpy(controlitr->szControlUser, info.szUserID) ;
								find = true ;
								index = i ;
							}
							
							++i ;
							++controlitr ; 
						}

						
						
						//AfxMessageBox((LPCTSTR)"Get to the authority successfuly") ;
					}
					else if(option == CVSP_FAILE)
					{
						find = true ;
						//pClient->m_Control = false ;
						//AfxMessageBox((LPCTSTR)"Get to the authority is faield ") ;
					}
					else if(option == CVSP_RELEASE)
					{
						controlitr = pClient->m_ControlLists.begin() ;


						while(controlitr != pClient->m_ControlLists.end())
						{
							if(strcmp(controlitr->szControlID, info.szCollaborationID) == 0)
							{
								controlitr->bControl = false ;
								sprintf(controlitr->szControlID, "None") ;
								find = true ;
								index = i ;
							}
							
							++i ;
							++controlitr ; 
						}


						
						//pClient->m_Control = false; 	
					}
					else if(option == CVSP_RELEASE_BYSERVER)
					{
						
						controlitr = pClient->m_ControlLists.begin() ;


						while(controlitr != pClient->m_ControlLists.end())
						{
							if(strcmp(controlitr->szControlID, info.szCollaborationID) == 0)
							{
								controlitr->bControl = false ;
								sprintf(controlitr->szControlID, "None") ;
								find = true ;
								index = i ;
							}
							
							++i ;
							++controlitr ; 
						}
						/*if(pClient->m_Control)
						{
							pClient->m_Control = false; 
							AfxMessageBox((LPCTSTR)"This client lose the control authority!") ;
						}
						else
						{
							AfxMessageBox((LPCTSTR)"We can control authority!") ;
						}*/
					} 
					// 해당 데이터의 값.

					if(find == false) assert(false) ;
					pClient->m_pwnd->PostMessage(WM_COLLABORATION_CONTROLRESULT, index, option) ;
					pClient->m_ControlQuestion = false ;
				}

				break ;
				case CVSP_MONITORINGMSG:
					{	
					memcpy(&pClient->m_Operation, extraPacket, sizeof(Operation)) ;
					
					// 어떤 협업정보인지 받아서 체크를 한다.
					std::vector<ControlInfo>::iterator controlitr ;
					controlitr = pClient->m_ControlLists.begin() ;
					bool bFound = false ;
					int index = 0 ;

					while(controlitr != pClient->m_ControlLists.end())
					{
						if(strcmp(pClient->m_Operation.szCollaborationID, controlitr->szControlID) == 0)
						{
							bFound = true ;
							break ;

						}


						++controlitr ;
						++index ;
					}


					if(bFound)
					{
						if(option == CVSP_OPERATION_START)
							pClient->m_pwnd->PostMessage(WM_COLLABORATION_OPERATION_GETSTART, index, option) ;
						else if(option == CVSP_ORTHOGONAL_RUN || option == CVSP_VOLUME_RUN)
							pClient->m_pwnd->PostMessage(WM_COLLABORATION_OPERATION_GETRUN, index, option) ;
						
					}

					//pClient->m_TrackBall->resetTrackball() ;
					//pClient->m_TrackBall->setViewMatrix(osg::Matrixd(pClient->m_Operation.matrix[0][0], pClient->m_Operation.matrix[0][1], pClient->m_Operation.matrix[0][2], pClient->m_Operation.matrix[0][3],
					//											pClient->m_Operation.matrix[1][0], pClient->m_Operation.matrix[1][1], pClient->m_Operation.matrix[1][2], pClient->m_Operation.matrix[1][3],
					//											pClient->m_Operation.matrix[2][0], pClient->m_Operation.matrix[2][1], pClient->m_Operation.matrix[2][2], pClient->m_Operation.matrix[2][3],
					//											pClient->m_Operation.matrix[3][0], pClient->m_Operation.matrix[3][1], pClient->m_Operation.matrix[3][2], pClient->m_Operation.matrix[3][3])
					//											);

					/*switch(option)
					{
						case CVSP_ROTATE_START:
							//pClient->m_TrackBall->buttonPress(pClient->m_Operation.m_X, pClient->m_Operation.m_Y,1);
						break ;
						case CVSP_ROTATE_RUN:
							//pClient->m_TrackBall->mouseMotion(pClient->m_Operation.m_X, pClient->m_Operation.m_Y);								
						break ;
						case CVSP_ROTATE_END:
							//pClient->m_TrackBall->buttonRelease(pClient->m_Operation.m_X, pClient->m_Operation.m_Y, 1);
						break ;
						case CVSP_MOVE_START:
							//pClient->m_TrackBall->buttonPress(pClient->m_Operation.m_X, pClient->m_Operation.m_Y, 2);
						break ;
						case CVSP_MOVE_RUN:
							//pClient->m_TrackBall->mouseMotion(pClient->m_Operation.m_X, pClient->m_Operation.m_Y);
						break ;
						case CVSP_MOVE_END:
							//pClient->m_TrackBall->buttonRelease(pClient->m_Operation.m_X, pClient->m_Operation.m_Y,2);
						break ;
						case CVSP_ZOOM_START:
							//pClient->m_TrackBall->buttonPress(pClient->m_Operation.m_X, pClient->m_Operation.m_Y, 3);
						break ;
						case CVSP_ZOOM_RUN:
							//pClient->m_TrackBall->mouseMotion(pClient->m_Operation.m_X, pClient->m_Operation.m_Y) ;							
						break ;
						case CVSP_ZOOM_END:
							//pClient->m_TrackBall->buttonRelease(pClient->m_Operation.m_X, pClient->m_Operation.m_Y, 3);
						break ;
						default:
						break ;
					}*/
					//PostMessage(pClient->m_hWnd, CVSP_MONITORING_MESSAGE, NULL, NULL) ; 
					}
				break ;
				default:
					// default case is error!
					AfxMessageBox((LPCTSTR)"Default Case Error!") ;
				break ;
			}
			
		}
		
		if(FD_ISSET(pClient->m_Sock, &fdErrorSet))
		{
			// Error Occured. So, close connection, and check flow control
			AfxMessageBox((LPCTSTR)"Read Error!") ;
		}
	}

	return 0 ;
}

bool CCollaborationClient::FloorControRequest(int index)
{
	if(!m_Authenticated)
		return false;

	unsigned char	cmd ;
	unsigned char	option ;

	cmd = CVSP_FLOORCONTROLREQ ;
	option = CVSP_REQUEST ;
	
	CollaborationInfo info ;
	sprintf(info.szCollaborationID, "%s", m_ControlLists[index].szControlID) ;
	sprintf(info.szUserID, "%s", m_ID.c_str()) ;
	
	//nt temp = strlen(m_ControlLists[index].szControlID) ;



	m_ControlQuestion = true ;

	//AfxMessageBox("send a CVSP_FloorControlReq message!") ;
	sendCVSP(m_Sock, cmd, option, &info, sizeof(info)) ;

	while(m_ControlQuestion)
	{
		Sleep(50) ;
	}

	return m_Control; 
}

bool CCollaborationClient::FloorControRelease(int index)
{
	if(!m_ControlLists[index].bControl)
		return false ;
	
	//if(!m_Control)
	//	return false;

	unsigned char	cmd ;
	unsigned char	option ;

	cmd = CVSP_FLOORCONTROLREQ ;
	option = CVSP_RELEASE ;
	
	CollaborationInfo info ;
	sprintf(info.szCollaborationID, "%s", m_ControlLists[index].szControlID) ;
	sprintf(info.szUserID, "%s", m_ID.c_str()) ;

	//m_ControlQuestion = true ;
	//AfxMessageBox("send a CVSP_FloorControlReq message!") ;
	sendCVSP(m_Sock, cmd, option, &info, sizeof(info)) ;
	m_Control = false ;


	return true ;
}



void CCollaborationClient::OperationStart(int index, long x, long y)
{
	//if(!m_Control)
	//	return ;
	

	
	unsigned char	cmd ;
	unsigned char	option ;
	Operation operation ;

	cmd = CVSP_OPERATIONREQ ;
	option = CVSP_OPERATION_START ;
	operation.x = x ;
	operation.y = y ;
	sprintf(operation.szCollaborationID, "%s", m_ControlLists[index].szControlID) ;



	sendCVSP(m_Sock, cmd, option, &operation, sizeof(operation)) ;

	
}

void CCollaborationClient::OperationRun(int index, long x, long y)
{
	//if(!m_Control)
	//	return ;
	unsigned char	cmd ;
	unsigned char	option ;
	cmd = CVSP_OPERATIONREQ ;
	option = CVSP_ORTHOGONAL_RUN ;
	Operation operation ;

	operation.x = x ;
	operation.y = y ;
	sprintf(operation.szCollaborationID, "%s", m_ControlLists[index].szControlID) ;

	sendCVSP(m_Sock, cmd, option, &operation, sizeof(operation)) ;
}

void CCollaborationClient::OperationRun(int index, double xRotate, double yRotate)
{
	unsigned char	cmd ;
	unsigned char	option ;
	cmd = CVSP_OPERATIONREQ ;
	option = CVSP_VOLUME_RUN ;
	Operation operation ;

	operation.xRotate = xRotate ;
	operation.yRotate = yRotate ;
	sprintf(operation.szCollaborationID, "%s", m_ControlLists[index].szControlID) ;

	sendCVSP(m_Sock, cmd, option, &operation, sizeof(operation)) ;

}

void CCollaborationClient::OperationEnd()
{
	//if(!m_Control)
	//	return ;
	
	unsigned char	cmd ;
	unsigned char	option ;
	cmd = CVSP_OPERATIONREQ ;
	Operation operation ;

	/*
	int index = m_TrackBall->mbutton();
    switch(index)
	{
		case 1: //ROTATE
			option = CVSP_ROTATE_END ;
		break ;
		case 2:	//MOVE
			option = CVSP_MOVE_END ;
		break ;
		//case 3:	//ZOOM
		case 4:	//ZOOM
			option = CVSP_ZOOM_END ;
		break ;

	}	

	Operation operation ;
	//operation.m_X = m_TrackBall->mx() ;
	//operation.m_Y = m_TrackBall->my() ;

	osg::Matrixd matrix = m_TrackBall->getViewMatrix() ;
	osg::Matrixd::value_type *temp = matrix.ptr() ;

	operation.matrix[0][0] = temp[0] ;
	operation.matrix[0][1] = temp[1] ;
	operation.matrix[0][2] = temp[2] ;
	operation.matrix[0][3] = temp[3] ;

	operation.matrix[1][0] = temp[4] ;
	operation.matrix[1][1] = temp[5] ;
	operation.matrix[1][2] = temp[6] ;
	operation.matrix[1][3] = temp[7] ;

	operation.matrix[2][0] = temp[8] ;
	operation.matrix[2][1] = temp[9] ;
	operation.matrix[2][2] = temp[10] ;
	operation.matrix[2][3] = temp[11] ;

	operation.matrix[3][0] = temp[12] ;
	operation.matrix[3][1] = temp[13] ;
	operation.matrix[3][2] = temp[14] ;
	operation.matrix[3][3] = temp[15] ;
	*/
	
	sendCVSP(m_Sock, cmd, option, &operation, sizeof(operation)) ;

}

void	CCollaborationClient::Leave()
{
	unsigned char	cmd ;
	unsigned char	option ;
	cmd = CVSP_LEAVEREQ ;

	
	if(!m_Connect)
		return ;

	sendCVSP(m_Sock, cmd, option, NULL, 0) ;
	m_Connect = false ;
	m_Authenticated = false ;
	m_Start = false ;
}

void	CCollaborationClient::Start(LPCSTR fileinfo)
{
	unsigned char	cmd ;
	unsigned char	option ;

	if(!m_Connect)
		return  ;

	if(m_Start)
		return ;

	cmd = CVSP_VTOFILEREQ ;
	option = CVSP_SUCCESS ;
	m_Directory = fileinfo ;
	//int id = 0 ;
	sendCVSP((unsigned int)m_Sock, cmd, option, (void*)fileinfo, strlen(fileinfo)) ;
}


bool CCollaborationClient::IsControl()
{
	return m_Control ;
}

bool CCollaborationClient::IsConnect()
{
	return m_Authenticated ;
}

bool CCollaborationClient::IsStart()
{
	return m_Start ;
}

void CCollaborationClient::SetParentHandle(HWND hWnd)
{
	m_hWnd = hWnd ;
}

void CCollaborationClient::GetOperation(Operation *pOperation)
{
	pOperation->x = m_Operation.x ;
	pOperation->y = m_Operation.y ;
	pOperation->xRotate = m_Operation.xRotate ;
	pOperation->yRotate = m_Operation.yRotate ;
	// = &m_Operation ;
}

LPCSTR	CCollaborationClient::GetFileName()
{
	return m_FileInfo.szFileName ;
}


std::vector<std::string> &CCollaborationClient::GetFileInfoLists()
{
	return m_FilInfoLists ;
}


bool CCollaborationClient::StartDownload()
{
	if(!m_Connect)
		return  false;

	if(m_DownLoadLists.empty())
		return false;;

	m_bDownLoad = true ;
	//std::vector<FileDownInfo> m_DownLoadLists; 
	m_DownLoadIter = m_DownLoadLists.begin() ;
	m_DownLoadIter->status = CVSP_FILE_NOT ;
	unsigned char	cmd ;
	unsigned char	option ;
	char temp[1024] ;
	
	cmd = CVSP_VTOFILEREQ ;
	option = CVSP_REQUEST ;

	sprintf(temp, "%s\\%s", m_Directory, m_DownLoadIter->szFileName) ;
	
	
	m_ControlQuestion = true ;
	sendCVSP(m_Sock, cmd, option, temp, sizeof(temp)) ;

	return true ;
	
}


bool CCollaborationClient::NextDownload()
{
	if(!m_Connect)
		return  false;

	if(m_DownLoadLists.empty())
		return false;;

	if(m_DownLoadIter == m_DownLoadLists.end())
		return false ;

	m_DownLoadIter++ ;
	m_DownLoadIter->status = CVSP_FILE_NOT ;
	unsigned char	cmd ;
	unsigned char	option ;
	char temp[1024] ;

	cmd = CVSP_VTOFILEREQ ;
	option = CVSP_REQUEST ;

	sprintf(temp, "%s\\%s", m_Directory, m_DownLoadIter->szFileName) ;
	
	m_ControlQuestion = true ;
	sendCVSP(m_Sock, cmd, option, temp, sizeof(temp)) ;

	return true ;

}


std::string CCollaborationClient::ReadZIP(std::string UserName, std::string FilePath, std::string FileExtension, std::string Password)
{
	std::string Path ,Folder,File;
	Path = FilePath;
	int    x;
	//x = Path.find_last_of('\\', Path.length());
	Folder = Path.substr(0, Path.length() - 4);
	//int y = Path.length()- x-4;


	WCHAR temppath[MAX_PATH] ;
	WCHAR tempfolder[MAX_PATH] ;

	MultiByteToWideChar( CP_ACP, 0, FilePath.c_str(), -1,  temppath, MAX_PATH) ;
	
	
	HZIP hz;
    if(Password.compare("") != 0)
	{
	    hz = OpenZip(temppath,Password.c_str());//verifier pour le password
    }
    else
	{
        hz = OpenZip(temppath,0);
    }

    ZIPENTRY ze;
    GetZipItem(hz,-1,&ze);
    int numitems = ze.index;
    
	MultiByteToWideChar( CP_ACP, 0, Folder.c_str(), -1,  tempfolder, MAX_PATH) ;
	SetUnzipBaseDir(hz, tempfolder);
    // -1 gives overall information about the zipfile
    for (int zi=0; zi<numitems; zi++)
	{
        ZIPENTRY ze;

		char temp1[MAX_PATH] ;
		std::string temp ;
		std::string extension ;
		std::string filename ;
		GetZipItem(hz,zi,&ze);                          // fetch individual details
		

		WideCharToMultiByte( CP_ACP, 0, ze.name, -1, temp1, MAX_PATH, NULL, NULL );

		temp =  temp1;
		
		if(FileExtension.compare(temp.substr(temp.length() - 3, temp.length())) == 0)
		{
			WCHAR tempzip[MAX_PATH] ;

			extension = temp.substr(temp.length() - 4, temp.length()) ;
			filename = temp.substr(0, temp.length() - 4) ;
			filename += "_" ;
			filename += UserName ;
			filename += extension ;
			File = filename ;
			MultiByteToWideChar( CP_ACP, 0, filename.c_str(), -1,  tempzip, MAX_PATH) ;

			UnzipItem(hz, zi, tempzip);                     // e.g. the item's name.
			File = "\\" ;
			File += filename ;
		}
		else
		{
			UnzipItem(hz, zi, ze.name);                     // e.g. the item's name.
		}
		
    }

    CloseZip(hz);

	return Folder ;
}

std::string CCollaborationClient::GetUserName()
{
	return m_ID ;
}

FileInfo	&CCollaborationClient::GetFileInfo()
{
	return m_FileInfo ;
}

void CCollaborationClient::SetControlInfo(CWnd *pView)
{
	ControlInfo info ;
	info.bControl = false ;
	info.pWnd = pView ;
	

	sprintf(info.szControlID, "%s", m_FileInfo.szCollaborationID) ;
	sprintf(info.szControlUser, "None") ;

	m_ControlLists.push_back(info) ;

}


std::vector<ControlInfo> &CCollaborationClient::GetControlInfoLists()
{
	return m_ControlLists ;
}



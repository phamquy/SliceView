#include "stdAfx.h"	//class wizard for winddows.
#include "CVSP.h"

int sendCVSP(unsigned int sockfd, unsigned char cmd, unsigned char option, void *payload, unsigned short len)
{
	char 			*CVSPPacket ;
	CVSPHeader_t	CVSPHeader ;
	u_int 			packetSize ;
	int				result ;
	//sockaddr_in		recvAddr ;
	
	// set packetsize;
	packetSize = len + sizeof(CVSPHeader_t);
	
	// make header
	CVSPHeader.cmd 				=	cmd ;
	CVSPHeader.option			=	option ;
	CVSPHeader.packetLength		=	packetSize ;
	
	CVSPPacket = (char *)malloc(packetSize);
	
	assert(CVSPPacket);	//check empty packet.
	
	// init packet
	memset(CVSPPacket, 0, packetSize);
	
	// make packet
	memcpy(CVSPPacket, &CVSPHeader, sizeof(CVSPHeader_t));

	// Set address
	//recvAddr.sin_family ;
	//recvAddr.sin_port = htons(VSSP_CPORT) ;
	//recvAddr.sin_addr.s_addr = inet_addr(szIPAddr) ;

	if(payload != NULL)
		memcpy(CVSPPacket + sizeof(CVSPHeader_t), payload, len);
	
	result = send(sockfd, CVSPPacket, packetSize, 0);

	if(result <0)
	{
		return result ;
	}
	
	/*if(result <0)
	{
		int errorCode = WSAGetLastError() ;
		switch(errorCode)
		{
		case WSANOTINITIALISED:
			AfxMessageBox("WSANOTINITIALISED") ;
		break ;
		case WSAENETDOWN:
			AfxMessageBox("WSAENETDOWN") ;
		break ;
		case WSAEACCES:
			AfxMessageBox("WSAEACCES") ;
		break ;
		case WSAEFAULT:
			AfxMessageBox("WSAEFAULT") ;
		break ;
		case WSAEINTR:
			AfxMessageBox("WSAEINTR") ;
		break ;
		case WSAEINPROGRESS:
			AfxMessageBox("WSAEINPROGRESS") ;
		break ;
		case WSAEINVAL:
			AfxMessageBox("WSAEINVAL") ;
		break ;
		case WSAEISCONN:
			AfxMessageBox("WSAEISCONN") ;
		break ;
		case WSAENETRESET:
			AfxMessageBox("WSAENETRESET") ;
		break ;
		case WSAENOTSOCK:
			AfxMessageBox("WSAENOTSOCK") ;
		break;
		case WSAEOPNOTSUPP:
			AfxMessageBox("WSAEOPNOTSUPP") ;
		break ;
		case WSAESHUTDOWN:
			AfxMessageBox("WSAESHUTDOWN") ;
		break ;
		case WSAEWOULDBLOCK:
			AfxMessageBox("WSAEWOULDBLOCK") ;
		break ;
		case WSAEMSGSIZE:
			AfxMessageBox("WSAEMSGSIZE") ;
		break ;
		case WSAETIMEDOUT:
			AfxMessageBox("WSAETIMEDOUT") ;s
		break ;
		case WSAECONNRESET:
			AfxMessageBox("WSAECONNRESET") ;
		break ;
		case WSAENOBUFS:
			AfxMessageBox("WSAENOBUFS") ;
		break ;
		case WSAENOTCONN:
			AfxMessageBox("WSAENOTCONN") ;
		break ;
		case WSAEHOSTUNREACH:
			AfxMessageBox("WSAEHOSTUNREACH") ;
		break ;
		case WSAECONNABORTED:
			AfxMessageBox("WSAECONNABORTED") ;
		break ;
		case WSAEADDRNOTAVAIL:
			AfxMessageBox("WSAEADDRNOTAVAIL") ;
		break ;
		case WSAEAFNOSUPPORT:
			AfxMessageBox("WSAEAFNOSUPPORT") ;
		break ;
		case WSAEDESTADDRREQ:
			AfxMessageBox("WSAEDESTADDRREQ") ;
		break ;
		case WSAENETUNREACH:
			AfxMessageBox("WSAENETUNREACH") ;
		break ;
					
		}

		return result;
	}*/
			
	free(CVSPPacket);
	return result;
	
} 

int recvCVSP(unsigned int sockfd, unsigned char *cmd, unsigned char *option, void *payload, unsigned short len)
{
	CVSPHeader_t		CVSPHeader;
	char 				extraPacket[CVSP_STANDARD_PAYLOAD_LENGTH];
	int					recvSize;
	int 				payloadSize;
	int 				payloadCopySize;

	assert(payload != NULL);
		
	//init
	memset(extraPacket , 0, sizeof(extraPacket));
	
	int last_readn, cur_readn;
	int ret = 0 ;
	recvSize = recv(sockfd, (char*)&CVSPHeader, sizeof(CVSPHeader), MSG_PEEK);
	
	if(recvSize <0)
	{
		return recvSize ;
	}
	
	last_readn = CVSPHeader.packetLength;
	cur_readn = 0;

	for( ; cur_readn != CVSPHeader.packetLength; )	// read packet determined size.
	{		
		ret = recv(sockfd, extraPacket + cur_readn, last_readn, 0);		
		if(ret < 0)
		{
			return -1 ;
		}
		last_readn -= ret;
		cur_readn += ret;
	}

	// header copy
	memcpy(&CVSPHeader, extraPacket, sizeof(CVSPHeader));
	payloadSize = CVSPHeader.packetLength - sizeof(CVSPHeader_t);
	*cmd = CVSPHeader.cmd;
	*option = CVSPHeader.option;
	payloadCopySize = payloadSize;
			
	// payload copy
	if(payloadCopySize != 0)
		memcpy(payload, extraPacket + sizeof(CVSPHeader), payloadCopySize);

	return payloadCopySize;	
}



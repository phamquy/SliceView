// 2007.02.14
// Author: Jun Lee
// COllaborative Virtual Tryon Service Protocol(CVSP)
// This is an application level protocol using TCP.

#ifndef _CVSP_1_0_0_H_
#define _CVSP_1_0_0_H_

#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>


//#define	DEBUG

#ifdef WIN32

#pragma comment(lib, "ws2_32.lib")
#include <winsock.h>
#include <io.h>
//Class wizard for windows(MFC)
//#include "stdAfx.h" 
#endif

#ifndef WIN32

#include <sys/socket.h>
#include <unistd.h>

#endif

//#define MFCSOCK

#define CVSP_MONITORING_MESSAGE			700
#define CVSP_MONITORING_LOAD			701

// version 
#define CVSP_VER						0x01
// Port	
#define CVSP_PORT						9000
// payload size 
#define CVSP_STANDARD_PAYLOAD_LENGTH	4096
// Command 
#define CVSP_JOINREQ					0x01
#define CVSP_JOINRES					0x02
#define CVSP_FLOORCONTROLREQ			0x03
#define CVSP_FLOORCONTROLRES			0x04
#define CVSP_OPERATIONREQ				0x05
#define CVSP_MONITORINGMSG				0x06
#define CVSP_LEAVEREQ					0x07
#define CVSP_VTOFILEREQ					0x08
#define CVSP_VTOFILERES					0x09
#define CVSP_VTOFILEINFOREQ				0x10
#define CVSP_VTOFILEINFORES				0x11
#define CVSP_VTODIRECTORYREQ			0x12
#define CVSP_VTODIRECTORYRES			0x13


// option  
#define CVSP_SUCCESS					0x01
#define CVSP_FAILE						0x02
#define CVSP_RELEASE					0x03
#define CVSP_RELEASE_BYSERVER			0x04
#define CVSP_REQUEST					0x05

// operation
#define CVSP_OPERATION_START			0x06
#define CVSP_ORTHOGONAL_RUN				0x07
#define CVSP_ROTATE_END					0x08
#define CVSP_VOLUME_START				0x09
#define CVSP_VOLUME_RUN					0x10
#define CVSP_MOVE_END					0x11
#define CVSP_ZOOM_START					0x12
#define CVSP_ZOOM_RUN					0x13
#define CVSP_ZOOM_END					0x14
#define CVSP_FILE_START					0x15
#define CVSP_FILE_RUN					0x16
#define CVSP_FILE_END					0x17
#define CVSP_FILE_NOT					0x18

/* variable style */
#ifdef WIN32
typedef unsigned char  u_char;
typedef unsigned int   u_int;
typedef unsigned short u_short;
typedef unsigned long  u_long;
#endif	


/*	Header(32bit)
		|0   |8	   |12	|16      |24     |
		|____|_____|______|______|_______|____________________|
		|Ver  |cmd |Option|Packet|Length |PayLoad			  |
		|__________|______|______|_______|____________________|
*/ 
typedef struct CVSPHeader_t{
	u_char			cmd;
	u_char			option;
	u_short			packetLength;
} CVSPHeader;

// #define MFCSOCK

// Version
#define CVSP_VER			0x01

// API interface
int sendCVSP(unsigned int sockfd, unsigned char cmd, unsigned char option, void *payload, unsigned short len);
int recvCVSP(unsigned int sockfd, unsigned char *cmd, unsigned char *option, void *payload, unsigned short len);


struct Operation
{
	char szCollaborationID[MAX_PATH] ;
	long x, y ;
	double xRotate, yRotate ;
	
	Operation()
	{
		memset(szCollaborationID, 0, sizeof(szCollaborationID)) ;
		x = 0;
		y = 0 ;
		xRotate = 0.0 ;
		yRotate = 0.0 ;

	}

} ;


struct _Data
{
	char	szData[10] ;
	int		iValue ;
};

struct FileDownInfo
{
	char szFileName[MAX_PATH] ;
	char status ;
};

struct FileInfo
{
	char	szFileName[255] ;
	unsigned long	lFileSize ;
	char	szCollaborationID[MAX_PATH] ;
	
	FileInfo()
	{
		memset(szFileName, 0, sizeof(szFileName)) ;
		memset(szCollaborationID, 0, sizeof(szCollaborationID)) ;
		lFileSize = 0 ;
	}
};

struct CollaborationInfo
{
	char	szCollaborationID[MAX_PATH];
	char	szUserID[50] ;

	CollaborationInfo()
	{
		memset(szCollaborationID, 0, sizeof(szCollaborationID)) ;
		memset(szUserID, 0, sizeof(szUserID)) ;
	}
};

#endif

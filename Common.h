// ***************************************************************
//  common   version:  1.0   ¡¤  date: 01/31/2008
//  -------------------------------------------------------------
//  General definition for this project
//  -------------------------------------------------------------
//  Copyright (C) 2008 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************
#pragma once
#include <stack>
#include <vector>

/*
 *	DEFINITION OF PROGRAM RETURN ERROR CODE
 */

#define SV_NORMAL_ABORT		 1
#define SV_NORMAL			 0		//Running OK
#define SV_FILEIO_ERROR		-1
#define SV_MEMORY_ERR		-2
#define SV_UNSUPPORT_FORMAT	-3
#define SV_SIZE_OVERLOAD	-4
#define SV_INVALID_PARAM	-5
#define SV_SYSTEM_ERR		-6
#define SV_VOLPACK_ERR		-7

/*
 *	DEFINITION CONSTANT VALUE 
 */

// Definition of number of bit per sample [3/16/2008 QUYPS]
#define SV_DCM_OUTPUTBITS	8


// Define width of control on child frame toolbar [2/9/2008 QUYPS]
#define SV_TB_LABEL		70
#define SV_TB_COMBO		120

#define SV_STR_LAYOUT	_T("Layout:")

// Definition of default raw volume size
#define SV_DEFAUL_VOLSIZE_X	64
#define SV_DEFAUL_VOLSIZE_Y	64
#define SV_DEFAUL_VOLSIZE_Z	64

#define SV_PI				3.1415926535

// Limit value [2/11/2008 QUYPS]
#define	 SV_GRAYLEVELS		256
#define  SV_VOL_MAXX		512
#define  SV_VOL_MAXY		512
#define  SV_VOL_MAXZ		512
#define	 SV_VOL_MAXSIZE		512*512*512


// Define type of layout [2/9/2008 QUYPS]
#define SV_LO_MIN		0
#define SV_LO_CUBE		0
#define SV_LO_HORZ		1
#define SV_LO_VERT		2
#define SV_LO_MAX		2
#define SV_LO_COUNT		(SV_LO_MAX - SV_LO_MIN)

#define SV_LOSTR_CUBE	_T("Cube")
#define SV_LOSTR_HORZ	_T("Horizontal")
#define SV_LOSTR_VERT	_T("Vertical")


#define SV_LO_BORDER	2


// Slider range [2/17/2008 QUYPS]
#define SV_CONS_MIN		   -100
#define SV_CONS_ADJUST_MIN	5
#define SV_CONS_MAX			100
#define SV_CONS_ADJUST_MAX	85
#define SV_CONS_DEFAULT		0

#define SV_BRIG_MIN		   -100
#define SV_BRIG_ADJUST_MIN -128
#define SV_BRIG_MAX			100
#define SV_BRIG_ADJUST_MAX	127
#define SV_BRIG_DEFAULT		0

#define SV_SLC_BORDER		1
#define SV_SLC_BORDER_CLR	RGB(255,255,255)
#define SV_SLC_FRMBACK		RGB(0,0,0)
#define SV_VIEW_BGND		RGB(0,0,0)
#define SV_CTRLLINE_SPC		5

// Define the constant for the segmentation
#define SV_SEG_DEF_THRESHOLD	128


// Define custom message [3/4/2008 QUYPS]
#define	WM_ADJUST_IMAGE			(WM_USER + 100)
#define WM_BRIGHTNESS_CHANGE	(WM_USER + 101)
#define WM_CONTRAST_CHANGE		(WM_USER + 102)

// Define string name for each slice type
#define	SV_STR_AXIAL	_T("Axial")
#define SV_STR_CORONAL	_T("Coronal")
#define SV_STR_SAGITTAL	_T("Sagittal")
#define SV_STR_VOLUME	_T("3D View")


// Define file name extension
#define SV_EXT_DICOM	_T("dcm")
#define SV_EXT_DCM_LEN	3
#define SV_EXT_RAW		_T("raw")
#define SV_EXT_RAW_LEN	3
#define SV_EXT_TIFF		_T("tiff")
#define SV_EXT_TIFF_LEN	4
#define SV_EXT_TIF		_T("tif")
#define SV_EXT_TIF_LEN	3


// Default exporting name [2/2/2009 QUYPS]
#define SV_EXP_DEFAULTNAME _T("Exported")



// Image processing constant
#define SV_MEANINGLESS_MARK		128
#define SV_MEANNING_MARK		255

#define SV_INV_DIAG				0
#define SV_INV_ROW				1
#define SV_INV_COLUME			2

/*
 *	DECLARATION OF STRUCT AND ENUM
 */

enum fileopenmode
{
	eRead,
	eWrite
};

enum layoutmode
{
	eAuto,
	eEqual
};
enum slicetype
{
                eUndef = -1,
	eAxial,
	eCoronal,
	eSagittal,
	eVolume
};


enum frameidx
{
	eFrame1,
	eFrame2,
	eFrame3,
	eFrame4
};

enum filetype
{
	eUnKnown,
	eRaw,
	eDicom,
	eTiff
};
// LUMINANCE [2/9/2008 QUYPS]
typedef struct  t_LUT
{
	INT nBirght;
	INT	nContrast;
} LUT, *PLUT;

enum lutproctype
{
	eCustom,
	eLinear
};


typedef struct t_3DSize
{
	INT ndx;
	INT ndy;
	INT ndz;
} VOLSIZE, *PVOLSIZE;


typedef struct t_LAYOUT
{
	RECT rcbound;
	INT ntype;
	RECT rcFrame1;
	RECT rcFrame2;
	RECT rcFrame3;
	RECT rcFrame4;
} LAYOUT, *PLAYOUT;

typedef struct t_3DPOS
{
	INT nX;
	INT nY;
	INT nZ;	
} VOLPOS, *PVOLPOS;

class CVolPos: public VOLPOS
{
public:
	CVolPos(INT x, INT y, INT z)
	{
		nX = x; 
		nY = y;
		nZ = z;
	}
	CVolPos()
	{
		nX = nY = nZ;
	}
protected:
private:
};

typedef struct t_SLCFRAME
{
	RECT rcBound;
	RECT rcImage;
	COLORREF clrBound;
	COLORREF clrBgrnd;
} SLCFRAME, *PSLCFRAME;


typedef struct t_DISPLAYINFO
{
	INT nlayout;
	INT nMode;
	BOOL bLines;
	BOOL bInfo;
	INT nContrast;
	INT nBright;
} DISPLAYTINFO;

typedef std::vector<CVolPos> Point3DVector;
typedef std::stack<CVolPos> Point3DStack;
typedef std::stack<CPoint> Point2DStack;
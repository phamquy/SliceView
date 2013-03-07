#pragma once
#include "sliceframe.h"
#include "linecontrol.h"
#include "PatientInfo.h"
#include "SliceProc.h"
#include "ViewObj.h"

class CSliceProc;

class CViewSliceObj: public CViewObj
{
public:
	CViewSliceObj(void);
	~CViewSliceObj(void);
	slicetype m_nType;

protected:
	BITMAPINFOHEADER m_bmpInfo;
	CSliceFrame m_objSlcFrame;
	BOOL m_bReDrawFrame;
	CPatientInfo* m_pPatInfo;
	CLineControl m_objCtrlLine;
	INT m_nSlcIndex;

private:
	//Image data 
	UCHAR* m_pOrgData;
	UCHAR* m_pProcessedData;

public:
	CSliceFrame GetFrame()
	{
		return m_objSlcFrame;
	}
	
	BITMAPINFOHEADER GetInfoHeader()
	{
		return m_bmpInfo;
	}
	int Draw(CDC* pDC);
	int UpdateFrame(RECT in_rcBoundary);
	int UpdateResultSlice(CSliceProc* in_pProc);

	INT GetIndex()
	{
		return m_nSlcIndex;
	}

	void SetIndex(INT in_idx)
	{
			m_nSlcIndex = in_idx;
	}

	void SetBitmapInfo(BITMAPINFOHEADER in_bmpInfo)
	{
		m_bmpInfo = in_bmpInfo;
	}

	CSize GetSize();
	UCHAR* GetOrgBuffer()
	{
		return m_pOrgData;
	}

	UCHAR* GetProcessedBuff()
	{
		return m_pProcessedData;
	}
	int PrepareMem(CSize in_size);
	int UpdateLineCtrl(VOLPOS in_3dPos);
	BOOL IsOnMouse(CPoint in_ptMousePos);
	CPoint ImagePos2FramePos(CPoint in_imgpoint);
	CPoint FramePos2ImagePos(CPoint in_FrmPoint);
	CPoint ViewPos2ImagePos(CPoint in_viewpos);
};

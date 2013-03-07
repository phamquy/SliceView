#pragma once

#include "ImageDataSet.h"
#include "afxcmn.h"
#include "afxwin.h"
#include "ImgCutter.h"

// CAutoSegment dialog

class CAutoSegment : public CDialog
{
	DECLARE_DYNAMIC(CAutoSegment)

public:
	CAutoSegment(CImageDataSet *pDataSet, CWnd* pParent = NULL);   // standard constructor
	CAutoSegment(CWnd* pParent /*=NULL*/);
	virtual ~CAutoSegment();

// Dialog Data
	enum { IDD = IDD_DLG_AUTOSEGMENT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

private:
	// Internal data parameter
	CImageDataSet*		m_pDataSet;	
	UCHAR*				m_pSourceByteImg;
	UCHAR*				m_pSourceImg;
	UCHAR*				m_pCuttingMask;
	UCHAR*				m_pResByteImg;
	UCHAR*				m_pResultImg;


	BITMAPINFOHEADER	bmInfo;
	int					m_nSlicePos;
	int					m_nThresholdVal;

	UCHAR				m_nLUT[SV_GRAYLEVELS];
	// Layout information
	CRect		m_OrgRegion;
	CRect		m_ResRegion;

	// Control variable
	CSliderCtrl m_sldBright;
	CSliderCtrl m_sldContrast;
	CSliderCtrl m_sldSliceNo;
	CSliderCtrl m_sldThreshold;
	CButton		m_chkPreproc;
	CButton		m_chkDefaultThres;	
	BOOL		m_bDefaultThres;
	CComboBox	m_cbbCutMethod;
	int			m_nMode;
	int			m_nPreMode;
	// Drawing variable
	CImgCutter	m_oCutter;
	int			m_nSegMethod;

	//Temp variables
	BOOL		m_bInitDone;
	CRect		m_rThres;
	
	void AutoSegment();
	void DisplayImage(void);
	void UpdateSlices(void);
	void UpdateLayout(int cx, int cy);
	void UpdateLUT();
	CPoint ScreenToImage(CPoint in_screenPoint, CRect in_Rect);
	void GetRawSliceFromDataSet( int nPos );
	
public:
	afx_msg void OnBnClickedRad();
	afx_msg void OnBnClickedAuChkDef();
	afx_msg void OnBnClickedAuChkPreproc();
	afx_msg void OnBnClickedAuCmdSaveall();
	afx_msg void OnBnClickedAuCmdSavesingle();
	afx_msg void OnBnClickedAuCmdSavevol();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	virtual BOOL OnInitDialog();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnPaint();
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnCbnSelchangeAuCbCmode();
	
};

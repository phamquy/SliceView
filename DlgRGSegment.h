#pragma once
#include "afxcmn.h"
#include "afxwin.h"
#include "ImageDataSet.h"
#include "ImgCutter.h"

// CDlgRGSegment dialog

class CDlgRGSegment : public CDialog
{
	DECLARE_DYNAMIC(CDlgRGSegment)

public:
	CDlgRGSegment(CWnd* pParent = NULL);   // standard constructor
	CDlgRGSegment(CImageDataSet* in_pDataSet, CWnd* pParent = NULL);
	virtual ~CDlgRGSegment();

// Dialog Data
	enum { IDD = IDD_DLG_REGIONGROW };

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


	UCHAR**				m_ppResultSet;
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
	int			m_nMode;
	int			m_nPreMode;
	CComboBox m_cbbCutMethod;

	// Drawing variable
	CImgCutter	m_oCutter;

	//Temp variables
	BOOL		m_bInitDone;
	CRect		m_rThres;



private:
	void DisplayImage(void);
	void UpdateSlices(void);
	void UpdateLayout(int cx, int cy);
	void UpdateLUT();
	CPoint ScreenToImage(CPoint in_screenPoint, CRect in_Rect);
	void GetRawSliceFromDataSet( int nPos );

public:
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedRgChkDef();
	afx_msg void OnBnClickedRgChkPreproc();
	afx_msg void OnBnClickedRgCmdSaveall();
	afx_msg void OnBnClickedRgCmdSavesingle();
	afx_msg void OnBnClickedRgCmdSavevol();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnBnClickedRad();
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnCbnSelchangeRgCbCutmode();
	
	
};

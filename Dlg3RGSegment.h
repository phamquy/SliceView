#pragma once
#include "afxcmn.h"
#include "afxwin.h"
#include "ImageDataSet.h"
#include "ImgCutter.h"
#include "ImgLib.h"
#include "Common.h"

// Define the sub volume
typedef struct SubVol_t
{
	UCHAR* pData;		//data
	VOLSIZE	size3D;
	VOLPOS pos3D;		//position in parent volume coordinate
};

// CDlg3RGSegment dialog

class CDlg3RGSegment : public CDialog
{
	DECLARE_DYNAMIC(CDlg3RGSegment)

public:
	CDlg3RGSegment(CWnd* pParent = NULL);   // standard constructor
	CDlg3RGSegment(CImageDataSet* in_pDataSet, CWnd* pParent = NULL);
	virtual ~CDlg3RGSegment();

// Dialog Data
	enum { IDD = IDD_DLG_3DRG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

private:
	// Internal data parameter
	CImageDataSet*		m_pDataSet;	
	UCHAR*				m_pSourceByteImg;
	UCHAR*				m_pSourceImg;
	UCHAR*				m_pCuttingMask;
//	UCHAR*				m_pResByteImg;
	UCHAR*				m_pBoundaryMask;

	UCHAR*				m_pResultImg;
	Point3DVector		m_seedPoints;

	UCHAR**				m_ppResultSet;
	BITMAPINFOHEADER	bmInfo;
	int					m_nSlicePos;
	int					m_nThresholdVal;
	UCHAR				m_nLUT[SV_GRAYLEVELS];

	SubVol_t			m_tSubVol;

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
	CComboBox	m_cbbCutMethod;
	int			m_nStartSlc;
	int			m_nEndSlc;
	BOOL		m_bBoundDetect;
	CSliderCtrl m_sldBoundThres;

	// Drawing variable
	CImgCutter	m_oCutter;

	//Temp variables
	BOOL		m_bInitDone;
	CRect		m_rThres;
	CRect		m_rBthres;

private:
	void DrawSeedPoints(void);
	void DisplayImage(void);
	void UpdateSlices(void);
	void UpdateLayout(int cx, int cy);
	void UpdateLUT();
	CPoint ScreenToImage(CPoint in_screenPoint, CRect in_Rect);
	CPoint ImageToScreen(CPoint in_imagePoint, CRect in_Rect);
	void GetRawSliceFromDataSet( int nPos );
	void Cutout( UCHAR* pSource, UCHAR* pResult, int width, int height );

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
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnBnClickedRad();
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnCbnSelchangeRgCbCutmode();
	afx_msg void OnBnClicked3drgStart();
	
};
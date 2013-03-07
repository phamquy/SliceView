#pragma once
#include "afxcmn.h"
#include "ImageDataSet.h"
#include "LutProc.h"

// CCutoutSegDlg dialog

class CCutoutSegDlg : public CDialog
{
	DECLARE_DYNAMIC(CCutoutSegDlg)

public:
	CCutoutSegDlg(CWnd* pParent = NULL);   // standard constructor
	CCutoutSegDlg(CImageDataSet* in_pData, CWnd* pParent = NULL);
	virtual ~CCutoutSegDlg();

// Dialog Data
	enum { IDD = IDD_SEG_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:	
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);


private:

	// Internal data parameter
	CImageDataSet*		m_pDataSet;	
	LUT					m_CurLut;
	CLutProc			m_objLutProc;
	UCHAR*				m_pSourceByteImg;
	UCHAR*				m_pSourceImg;
	UCHAR*				m_pResultByteImg;
	UCHAR*				m_pResultImg;
	BITMAPINFOHEADER	bmInfo;
	int					m_nSlicePos;

#ifdef _DEBUG
	UCHAR*				m_pDebugBinImage;
	UCHAR*				m_pDebugBinByte;
#endif

	
	// Dialog parameters
	CSliderCtrl m_ctlBrightSlider;		// Brightness slider control
	CSliderCtrl m_ctlContrastSlider;	// Contrast slider control	
	CSliderCtrl m_ctlThresholdSlider;	// Threshold value slider control	
	CSliderCtrl m_ctlSlicePos;			// Position of slice slider control
	BOOL m_bEnablePreprocess;			// Enable brightness and contrast preprocessing
	BOOL m_bUseDefaultThreshold;		// Use default value for thresholding
	int m_nThresholdVal;

public:	
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedSegPreproc();
	afx_msg void OnBnClickedSegBindefault();
	afx_msg void OnBnClickedCmdSaveslice();
	afx_msg void OnBnClickedCmdSavevol();
	afx_msg void OnPaint();	

private:
	int UpdateSourceSlice(void);
	int UpdatePreproc(void);
	int CutOut(UCHAR* pSource, UCHAR* pResult, int width, int height);
	void DisplayImage();
	//void BoundaryRG(UCHAR* pSource, UCHAR* pDest, int width, int height);

public:
	void GetRawSliceFromDataSet(int nPos);
	afx_msg void OnBnClickedCmdSaveallslices();
	afx_msg void OnBnClickedCmdRg3d();
};

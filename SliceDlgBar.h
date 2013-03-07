#pragma once
#include "Common.h"

// CSliceDlgBar dialog

class CSliceDlgBar : public CDialogBar
{
DECLARE_DYNAMIC(CSliceDlgBar)
public:
	CSliceDlgBar();   // standard constructor
	virtual ~CSliceDlgBar();

	// Dialog Data
	enum { IDD = IDD_DIALOGBAR };
	int m_nLayoutMode;
	int m_nCurLayoutIndex;
	BOOL m_bLine;
	BOOL m_bInfo;
	CSliderCtrl m_wndSldContrast;
	CSliderCtrl m_wndSldBright;
	CComboBox m_wndCmbLayout;
	//CCheckBox 
protected:
	
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg LONG OnInitDialog ( UINT, LONG );
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	virtual BOOL Create(CWnd* pParentWnd, UINT nIDTemplate, UINT nStyle, UINT nID);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);

	int GetLayoutType();
	int GetLayoutMode()
	{
		if(m_nLayoutMode == 0)
			return	eAuto;
		else
			return eEqual;
	};

	BOOL IsDisplayLines()
	{
		return m_bLine;
	}

	BOOL IsDisplayInfo()
	{
		return m_bInfo;
	}

	int GetContrast()
	{
		return m_wndSldContrast.GetPos();
	}

	int GetBright()
	{
		return m_wndSldBright.GetPos();
	}
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};

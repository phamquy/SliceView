#pragma once
#include "resource.h"

// CVolSizeDlg dialog

class CVolSizeDlg : public CDialog
{
	DECLARE_DYNAMIC(CVolSizeDlg)

public:
	CVolSizeDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CVolSizeDlg();

// Dialog Data
	enum { IDD = IDD_DLG_VOLSIZE};

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	UINT m_nCX;
	UINT m_nCY;
	UINT m_nCZ;
	virtual BOOL OnInitDialog();
};

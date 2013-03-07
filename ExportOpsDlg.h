#pragma once


// CExportOpsDlg dialog

class CExportOpsDlg : public CDialog
{
	DECLARE_DYNAMIC(CExportOpsDlg)

public:
	CExportOpsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CExportOpsDlg();

// Dialog Data
	enum { IDD = IDD_DLG_EXP_OPT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	// Select between raw format or TIFF format
	BOOL m_bRawFormat;
	virtual BOOL OnInitDialog();
};

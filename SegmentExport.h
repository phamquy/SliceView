#pragma once


// CSegmentExport dialog

class CSegmentExportDlg : public CDialog
{
	DECLARE_DYNAMIC(CSegmentExportDlg)

public:
	CSegmentExportDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSegmentExportDlg();

// Dialog Data
	enum { IDD = IDD_DLD_SEGEX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString m_sDirPath;
	CString m_sExt;
	int m_nType;
	afx_msg void OnBnClickedSegexBrow();
	afx_msg void OnCbnSelchangeSegexFiletype();
protected:
	virtual void OnOK();
};

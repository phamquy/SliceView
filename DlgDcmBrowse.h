#pragma once
#include "afxwin.h"
#include "afxcoll.h"


// CDlgDcmBrowse dialog

class CDlgDcmBrowse : public CDialog
{
	DECLARE_DYNAMIC(CDlgDcmBrowse)

public:
	CDlgDcmBrowse(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgDcmBrowse();

// Dialog Data
	enum { IDD = IDD_DLG_BROWSE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCmdBrowse();
	CListBox m_ctlFileList;
	CString m_sFolderPath;
	afx_msg void OnBnClickedCmdSelectall();
	afx_msg void OnBnClickedCmdDeselectall();
	int GetFileList(CStringList* out_pFileList);
	void BrowseDownloads() ;
	virtual BOOL OnInitDialog();
protected:
	virtual void OnOK();
private:
	CStringList m_oFileList;
};

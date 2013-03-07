// VolSizeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SliceView.h"
#include "VolSizeDlg.h"
#include "Common.h"

// CVolSizeDlg dialog

IMPLEMENT_DYNAMIC(CVolSizeDlg, CDialog)

CVolSizeDlg::CVolSizeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CVolSizeDlg::IDD, pParent)
	, m_nCX(0)
	, m_nCY(0)
	, m_nCZ(0)
{

}

CVolSizeDlg::~CVolSizeDlg()
{

}

void CVolSizeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_CX, m_nCX);
	DDV_MinMaxUInt(pDX, m_nCX, 1, 1024);
	DDX_Text(pDX, IDC_CY, m_nCY);
	DDV_MinMaxUInt(pDX, m_nCY, 1, 1024);
	DDX_Text(pDX, IDC_CZ, m_nCZ);
	DDV_MinMaxUInt(pDX, m_nCZ, 1, 1024);
}


BEGIN_MESSAGE_MAP(CVolSizeDlg, CDialog)
END_MESSAGE_MAP()


// CVolSizeDlg message handlers

BOOL CVolSizeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	m_nCX = SV_DEFAUL_VOLSIZE_X;
	m_nCY = SV_DEFAUL_VOLSIZE_Y;
	m_nCZ =	SV_DEFAUL_VOLSIZE_Z;

	UpdateData(false);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

// SliceDlgBar.cpp : implementation file
//

#include "stdafx.h"
#include "SliceView.h"
#include "SliceDlgBar.h"
#include "ChildFrm.h"

IMPLEMENT_DYNAMIC(CSliceDlgBar, CDialogBar)
// CSliceDlgBar dialog

CSliceDlgBar::CSliceDlgBar()
{

}

CSliceDlgBar::~CSliceDlgBar()
{
}

void CSliceDlgBar::DoDataExchange(CDataExchange* pDX)
{
	CDialogBar::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHK_LINE, m_bLine);
	DDX_Check(pDX, IDC_CHK_INFO, m_bInfo);
	DDX_Radio(pDX, IDC_RA_AUTO, m_nLayoutMode);
	DDX_CBIndex(pDX, IDC_CMB_LAYOUT, m_nCurLayoutIndex);
}


BEGIN_MESSAGE_MAP(CSliceDlgBar, CDialogBar)
	ON_MESSAGE(WM_INITDIALOG, OnInitDialog )
	ON_WM_PAINT()	
	ON_WM_CREATE()
	//ON_BN_CLICKED(IDC_BUTTON2, &CSliceDlgBar::OnBnClickedButton)		
	ON_WM_HSCROLL()
END_MESSAGE_MAP()


// CSliceDlgBar message handlers
LONG CSliceDlgBar::OnInitDialog ( UINT wParam, LONG lParam)
{
	if (!UpdateData(FALSE))
	{
		TRACE0("Warning: UpdateData failed during dialog init.\n");
	}
	
	//Init value for control
	m_nLayoutMode = 1;
	m_bLine = TRUE;
	m_bInfo = FALSE;
	UpdateData(FALSE);
	BOOL bRet = HandleInitDialog(wParam, lParam);
	return bRet;

}
void CSliceDlgBar::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: Add your message handler code here
	
}


BOOL CSliceDlgBar::Create(CWnd* pParentWnd, UINT nIDTemplate, UINT nStyle, UINT nID)
{
	// TODO: Add your specialized code here and/or call the base class

	BOOL bReturn  = CDialogBar::Create(pParentWnd, nIDTemplate, nStyle, nID);
	BOOL re;
	if(bReturn)
	{
		re = m_wndSldContrast.SubclassDlgItem(IDC_SLD_CONTRAST, this);
		if(re)
		{
			m_wndSldContrast.SetRange(SV_CONS_MIN, SV_CONS_MAX, TRUE);
			m_wndSldContrast.SetPos(SV_CONS_DEFAULT);
		}

		if(re) 
		{
			re = m_wndSldBright.SubclassDlgItem(IDC_SLD_BRIGHT, this);
			if(re) 
			{
				m_wndSldBright.SetRange(SV_BRIG_MIN,SV_BRIG_MAX, TRUE);
				m_wndSldBright.SetPos(SV_BRIG_DEFAULT);
			}

		}
		if(re) 
		{
			re = m_wndCmbLayout.SubclassDlgItem(IDC_CMB_LAYOUT, this);
			if (re)	
			{
				m_wndCmbLayout.SetCurSel(0);				
			}
		}
	}
	return bReturn;
}

int CSliceDlgBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialogBar::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	// TODO:  Add your specialized creation code here
	
	return 0;
}

int CSliceDlgBar::GetLayoutType()
{
	int layout = -1;
	switch(m_wndCmbLayout.GetCurSel())
	{
	case 0:
		layout = SV_LO_CUBE;
		break;
	case 1:
		layout = SV_LO_HORZ;
		break;
	case 2:
		layout = SV_LO_VERT;
	    break;
	default:
	    break;
	}
	return layout;
}

void CSliceDlgBar::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default
	CXSliceFrame* pParentFrm = (CXSliceFrame*)GetParentFrame();
	CView* pAcView = pParentFrm->GetActiveView();
	INT nBright = m_wndSldBright.GetPos();
	INT nContrast = m_wndSldContrast.GetPos();
	pAcView->PostMessage(WM_ADJUST_IMAGE,nBright,nContrast);
}

// SliceView.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "SliceView.h"
#include "MainFrm.h"

#include "ChildFrm.h"
#include "XSliceDoc.h"
#include "XSliceView.h"
#include "DlgDcmBrowse.h"
#include "CutoutSegDlg.h"
#include "DlgRGSegment.h"
#include "AutoSegment.h"
#include "Dlg3RGSegment.h"

#include "gdiplus.h"
using namespace Gdiplus;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CSliceViewApp

BEGIN_MESSAGE_MAP(CSliceViewApp, CWinApp)

ON_COMMAND(ID_APP_ABOUT, &CSliceViewApp::OnAppAbout)
	// Standard file based document commands
	//ON_COMMAND(ID_FILE_NEW, &CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, &CWinApp::OnFilePrintSetup)
	//Import dicom file
	ON_COMMAND(ID_FILE_IMPORTFILES, &CSliceViewApp::OnDcmImport)	

	ON_COMMAND(ID_FILE_EXPORTFILES, &CSliceViewApp::OnFileExport)
	ON_COMMAND(ID_SEGMENTTOOLS_CUTOUT, &CSliceViewApp::OnSegmenttoolsCutout)
	ON_COMMAND(ID_SEGMENTTOOLS_REGIONGROWING, &CSliceViewApp::OnSegmenttoolsRegiongrowing)
	ON_COMMAND(ID_SEGMENTTOOLS_AUTODETECTION, &CSliceViewApp::OnSegmenttoolsAutodetection)
	ON_COMMAND(ID_SEGMENTTOOLS_3DRESGIONGROWING, &CSliceViewApp::OnSegmenttools3dresgiongrowing)
END_MESSAGE_MAP()


// CSliceViewApp construction

CSliceViewApp::CSliceViewApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	m_bDownload = false ;
	m_sFolderPath = "" ;
}


// The one and only CSliceViewApp object

CSliceViewApp theApp;


// CSliceViewApp initialization

BOOL CSliceViewApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}
	AfxEnableControlContainer();
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));
	LoadStdProfileSettings(4);  // Load standard INI file options (including MRU)
	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(IDR_SliceViewTYPE,
		RUNTIME_CLASS(CXSliceDoc),
		RUNTIME_CLASS(CXSliceFrame), // custom MDI child frame
		RUNTIME_CLASS(CXSliceView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame || !pMainFrame->LoadFrame(IDR_MAINFRAME))
	{
		delete pMainFrame;
		return FALSE;
	}
	m_pMainWnd = pMainFrame;
	// call DragAcceptFiles only if there's a suffix
	//  In an MDI app, this should occur immediately after setting m_pMainWnd


	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;
	ParseCommandLine(cmdInfo);


	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;
	// The main window has been initialized, so show and update it
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();


	//////////////////////////////////////////////////////////////////////////	
	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);


	return TRUE;
}



// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// App command to run the dialog
void CSliceViewApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}




//************************************
// Method:    OnDcmImport
// FullName:  CSliceViewApp::OnDcmImport
// Access:    public 
// Returns:   void
// Qualifier:
// Purpose:   
//************************************
void CSliceViewApp::OnDcmImport()
{
/*
	CDlgDcmBrowse imPortDlg;
	CStringList oFileList;
	if (imPortDlg.DoModal() == IDOK)
	{
		imPortDlg.GetFileList(&oFileList);
	}

#ifdef _DEBUG //---[3/18/2008 QUYPS]---
	POSITION pos = NULL;
	for (INT i=0; i<oFileList.GetCount(); i++)
	{
		pos = oFileList.FindIndex(i);
		if (pos)
		{
			TRACE(_T("\n%s"), oFileList.GetAt(pos));
		}		
	}
#endif //------------------------------

*/
	
// TEST [3/18/2008 QUYPS]	

	POSITION posTemplate = GetFirstDocTemplatePosition();
	CDocTemplate* pDocTemplate;
	if (posTemplate != NULL)
	{
		pDocTemplate = GetNextDocTemplate(posTemplate);
		pDocTemplate->OpenDocumentFile(NULL);
	}
}


// ADDED [1/22/2009 QUYPS]
//************************************
// Method:    OnFileExport
// FullName:  CSliceViewApp::OnFileExport
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void CSliceViewApp::OnFileExport()
{
	//Obtain the current active document
	CMDIFrameWnd* pMainFrame = (CMDIFrameWnd*)AfxGetApp()->GetMainWnd();
	CMDIChildWnd* pAcitiveChildFrame = pMainFrame->MDIGetActive();	
	CXSliceView* pActiveView = (CXSliceView*) pAcitiveChildFrame->GetActiveView();
	CXSliceDoc* pActiveDoc = pActiveView->GetDocument();	
	
	//Call the exporting function
	pActiveDoc->OnExportDocument();
}

// CSliceViewApp message handlers


int CSliceViewApp::ExitInstance()
{
	// TODO: Add your specialized code here and/or call the base class
	GdiplusShutdown(gdiplusToken);
	return CWinApp::ExitInstance();
}

void CSliceViewApp::OnSegmenttoolsCutout()
{
	// TODO: Add your command handler code here
	//Obtain the current active document
	CMDIFrameWnd* pMainFrame = (CMDIFrameWnd*)AfxGetApp()->GetMainWnd();
	CMDIChildWnd* pAcitiveChildFrame = pMainFrame->MDIGetActive();	
	CXSliceView* pActiveView = (CXSliceView*) pAcitiveChildFrame->GetActiveView();
	CXSliceDoc* pActiveDoc = pActiveView->GetDocument();	
	
	CCutoutSegDlg dlgSegCutout(pActiveDoc->GetDataSet(), pAcitiveChildFrame); 
	
	dlgSegCutout.DoModal();	
}

void CSliceViewApp::OnSegmenttoolsRegiongrowing()
{
	// TODO: Add your command handler code here	
	//Obtain the current active document
	CMDIFrameWnd* pMainFrame = (CMDIFrameWnd*)AfxGetApp()->GetMainWnd();
	CMDIChildWnd* pAcitiveChildFrame = pMainFrame->MDIGetActive();	
	CXSliceView* pActiveView = (CXSliceView*) pAcitiveChildFrame->GetActiveView();
	CXSliceDoc* pActiveDoc = pActiveView->GetDocument();	
	CDlgRGSegment dlgRGSeg(pActiveDoc->GetDataSet(), pAcitiveChildFrame); 

	dlgRGSeg.DoModal();	
}

void CSliceViewApp::OnSegmenttoolsAutodetection()
{
	// TODO: Add your command handler code here	
	//Obtain the current active document
	CMDIFrameWnd* pMainFrame = (CMDIFrameWnd*)AfxGetApp()->GetMainWnd();
	CMDIChildWnd* pAcitiveChildFrame = pMainFrame->MDIGetActive();	
	CXSliceView* pActiveView = (CXSliceView*) pAcitiveChildFrame->GetActiveView();
	CXSliceDoc* pActiveDoc = pActiveView->GetDocument();	
	CAutoSegment dlgAutoSeg(pActiveDoc->GetDataSet(), pAcitiveChildFrame); 
	dlgAutoSeg.DoModal();	
}

void CSliceViewApp::OnSegmenttools3dresgiongrowing()
{
	// TODO: Add your command handler code here	
	//Obtain the current active document
	CMDIFrameWnd* pMainFrame = (CMDIFrameWnd*)AfxGetApp()->GetMainWnd();
	CMDIChildWnd* pAcitiveChildFrame = pMainFrame->MDIGetActive();	
	CXSliceView* pActiveView = (CXSliceView*) pAcitiveChildFrame->GetActiveView();
	CXSliceDoc* pActiveDoc = pActiveView->GetDocument();	
	CDlg3RGSegment dlg3DRG(pActiveDoc->GetDataSet(), pAcitiveChildFrame);
	dlg3DRG.DoModal();	
}

// SliceView.h : main header file for the SliceView application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols


// CSliceViewApp:
// See SliceView.cpp for the implementation of this class
//

class CSliceViewApp : public CWinApp
{
public:
	CSliceViewApp();

private:
	ULONG_PTR gdiplusToken;

// Overrides
public:


	bool m_bDownload ;
	CString m_sFolderPath;


/*
	//Save the first slice to the file
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
*/

	
	virtual BOOL InitInstance();

// Implementation
	afx_msg void OnAppAbout();
	afx_msg void OnDcmImport();
	afx_msg void OnFileExport();

	DECLARE_MESSAGE_MAP()
	virtual int ExitInstance();
	afx_msg void OnSegmenttoolsCutout();
	afx_msg void OnSegmenttoolsRegiongrowing();
	afx_msg void OnSegmenttoolsAutodetection();
	afx_msg void OnSegmenttools3dresgiongrowing();
};

extern CSliceViewApp theApp;
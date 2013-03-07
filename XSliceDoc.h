// XSliceDoc.h : interface of the CXSliceDoc class
//
#pragma once
#include "PatientInfo.h"
#include "ImageDataSet.h"
#include "SliceObj.h"
#include "ViewLayout.h"
#include "Common.h"
#include "LutProc.h"
#include "ShearWarpRender.h"



class CXSliceDoc : public CDocument
{
protected: // create from serialization only
	CXSliceDoc();
	DECLARE_DYNCREATE(CXSliceDoc)

// Overrides
public:
	virtual BOOL OnNewDocument();
	virtual BOOL OnExportDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);

// Implementation
public:
	virtual ~CXSliceDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()


private:
	ShearWarpRender	m_objRenderer;
	CPatientInfo	m_objPatInfo;
	CImageDataSet	m_objImgDataSet;
	CViewSliceObj		m_objVolumeImg;
	CViewSliceObj		m_objSlcAxial;
	CViewSliceObj		m_objSlcCoronal;
	CViewSliceObj		m_objSlcSagittal;
	CViewLayout		m_objLayout;
	VOLPOS			m_Cur3Dpos;
	LUT				m_CurLut;
	CLutProc		m_objLutProc;
public:

	//////////////////////////////////////////////////////////////////////////
	// VOLUME TEST [4/17/2008 QUYPS]
	INT m_RotateAxis;	//0:X, 1:Y, 2:Z
	DOUBLE m_XRotate;
	DOUBLE m_YRotate;
	DOUBLE m_ZRotate;
	//////////////////////////////////////////////////////////////////////////

	int Draw(CDC* pDC);
	int UpdateLuminance(LUT in_lu, BOOL in_flgForced=FALSE);
	int UpdateViewLayout();
	int UpdateSlices(VOLPOS in_SlicePosition);	
	int UpdateSlicesFrame(void);
	int UpdateLineControl();
	CViewSliceObj* GetActiveSlice(CPoint in_MousePos);
	int UpdateSlices(CPoint in_pt2Dpos);
	int UpdateSlices(int in_nChange, CPoint in_MousePos);

	int ReRenderVol(CDC* pDC);

public:
	CImageDataSet*	GetDataSet() {
		return &m_objImgDataSet;
	}

};
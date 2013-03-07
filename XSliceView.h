// XSliceView.h : interface of the CXSliceView class
//
#pragma once




class CXSliceView : public CView
{
protected: // create from serialization only
	CXSliceView();
	DECLARE_DYNCREATE(CXSliceView)

// Attributes
public:
	CXSliceDoc* GetDocument() const;
	CPoint	m_CollborationPoint ;
	double	m_XRotate ;
	double	m_YRotate ;
	char	m_chOption ;

private:
	POINT prePoint;

	// 2009.03.14 Jun Lee
	bool	m_bCollaboration ;
	bool	m_bControl ;
	char	m_szCollaborationID[MAX_PATH] ;
	int		m_nCollaborationIndex ;
	
	
	
	CWnd	*m_pwnd ;
// Overrides
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// Implementation
public:
	virtual ~CXSliceView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	

	void SetCollaboration(const char *szCollaborationID, int nIndex, CWnd *pF) ;
	void SetControl(bool bcontrol) ;
	void OperationStart(CPoint pt) ;
	void OperationRun(CPoint pt) ;
	void OperationRun(double xRotate, double yRRotate) ;

protected:

// Generated message map functions
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnCbnSelchangeCmbLayout();
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	LRESULT OnBrightnessChange(WPARAM, LPARAM);
	LRESULT	OnContrastChange(WPARAM, LPARAM);
	LRESULT OnAdjustColor(WPARAM, LPARAM);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
protected:
	DECLARE_MESSAGE_MAP()

private:
	int updateVolumeRotation(POINT in_prePoint, POINT in_point);
	int updateOrthoSlices(CPoint in_ptMousePos);
	int updateOrthoSlices(int in_nDifferential, CPoint in_ptMousePos);
	int updateGrayScale(int in_nBright, int in_nContrast);
};

#ifndef _DEBUG  // debug version in XSliceView.cpp
inline CXSliceDoc* CXSliceView::GetDocument() const
   { return reinterpret_cast<CXSliceDoc*>(m_pDocument); }
#endif


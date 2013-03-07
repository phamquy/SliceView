#pragma once
#include "stdafx.h"
#include "CurvePath.h"

#define	SV_CUT_NOTSTARTED	 0
#define	SV_CUT_STARTED		 1	
#define	SV_CUT_CUTTING		 2
#define SV_CUT_FINISHED		 3

#define	SV_CUT_LINESEGMENTS		0
#define SV_CUT_RECTANGLE		1
#define SV_CUT_CURVES			2

class CImgCutter
{
public:
	CImgCutter(void);
	~CImgCutter(void);
	
	CRect GetCutRectangle();

	void SetFrame(CRect rcRect);

	void SetCutStatus(int nStatus)
	{
		m_nCutStatus = nStatus;
	}

	int GetCutStatus()
	{
		return m_nCutStatus;
	}

	void AddPoint(CPoint point)
	{
// 		Point addpoint;
// 		addpoint.X = point.x - m_CutFrame.left;
// 		addpoint.Y = point.y - m_CutFrame.top;
// 		m_oCurve.AddPoints(addpoint);
		
		m_oCurve.AddPoints(Point(point.x, point.y));

	}

	Status GenerateMask(UCHAR* pMask, int width, int height);

	void AddCandidatePoint(CPoint point)
	{
		m_oCurve.AddCandidate(Point(point.x, point.y));
	}

	void SetMode(int nCutMode)
	{
		m_nCutMode = nCutMode;
	}

	int GetNumOfPoint()
	{
		return m_oCurve.GetNumOfPoint();
	}

	int GetMode()
	{
		return m_nCutMode;
	}

	void EnableDrawPoints(BOOL bDrawpoint)
	{
		m_bDrawPoint = bDrawpoint;
	}

	void ResetCut()
	{
		m_nCutStatus = SV_CUT_NOTSTARTED;		
		m_oCurve.DeletePath();
	}


	Status DrawResult(Graphics* graph);

private:
	CCurvePath	m_oCurve;
	CRect		m_CutFrame;
	
	int			m_nCutStatus;
	int			m_nCutMode;
	int			m_bDrawPoint;
};

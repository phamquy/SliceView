

#pragma once

#include <Windows.h>
#include <GdiPlus.h>
using namespace Gdiplus;


class CCurvePath
{
public:
	CCurvePath(void);
	~CCurvePath(void);

	int AddPoints(Point inPoint)
	{
		m_pPoints[m_nNumOfPoints] = inPoint;
		m_nNumOfPoints++;
		return m_nNumOfPoints;
	}
	Point* GetPoints()
	{
		return m_pPoints;
	}

	Point GetPointAt(int i)
	{
		return m_pPoints[i];
	}

	int AddCandidate(Point inPoint)
	{
		m_pPoints[m_nNumOfPoints] = inPoint;
		return m_nNumOfPoints +1;
	}

	int GetNumOfPoint()
	{
		return m_nNumOfPoints;
	}

	void DeletePath()
	{
		m_nNumOfPoints = 0;

	}
	void Scale(DOUBLE dVertical, DOUBLE dHorizontal);

	Status DrawOpenCurve(Graphics* pGraph, int nNumOfPoints, BOOL bDrawPoint);
	Status DrawClosedCurve(Graphics* pGraph, int nNumOfPoints, BOOL bDrawPoint);
	Status DrawOpenLines(Graphics* pGraph, int nNumOfPoints, BOOL bDrawPoint);
	Status DrawCloseLines(Graphics* pGraph, int nNumOfPoints, BOOL bDrawPoint);
	Status DrawCutRect(Graphics* pGraph, BOOL bDrawPoint);
private:
	Point m_pPoints[256];
	int m_nNumOfPoints;

};

#include "CurvePath.h"

CCurvePath::CCurvePath(void)
{
	m_nNumOfPoints = 0;
	//m_nMode = 0;
}

CCurvePath::~CCurvePath(void)
{
}

Status CCurvePath::DrawOpenCurve( Graphics* pGraph, int nNumOfPoints, BOOL bDrawPoint )
{
	Pen greenPen(Color(0, 0, 255), 2);
	SolidBrush redBrush(Color::Red);
	Status sts = Ok;

	sts = pGraph->DrawCurve(&greenPen, m_pPoints,  nNumOfPoints);
	if(bDrawPoint)
	{
		for(int i=0; i< nNumOfPoints; i++)
		{
			sts = pGraph->FillEllipse(&redBrush, Rect(m_pPoints[i].X-4, m_pPoints[i].Y-4, 8, 8));		
		}
	}

	return sts;
}

Status CCurvePath::DrawClosedCurve( Graphics* pGraph, int nNumOfPoints, BOOL bDrawPoint )
{
	Pen greenPen(Color(0, 0, 255), 2);
	SolidBrush redBrush(Color::Red);
	Status sts = Ok;
	sts = pGraph->DrawClosedCurve(&greenPen, m_pPoints,  nNumOfPoints);
	
	if (bDrawPoint)
	{
		for(int i=0; i< nNumOfPoints; i++)
		{
			sts = pGraph->FillEllipse(&redBrush, Rect(m_pPoints[i].X-4, m_pPoints[i].Y-4, 8, 8));		
		}
	}

	return sts;
}

Status CCurvePath::DrawOpenLines( Graphics* pGraph, int nNumOfPoints, BOOL bDrawPoint )
{
	Pen greenPen(Color(0, 0, 255), 2);
	SolidBrush redBrush(Color::Red);
	Status sts = Ok;

	sts = pGraph->DrawCurve(&greenPen, m_pPoints,  nNumOfPoints, 0.0f);
	if(bDrawPoint)
	{
		for(int i=0; i< nNumOfPoints; i++)
		{
			sts = pGraph->FillEllipse(&redBrush, Rect(m_pPoints[i].X-4, m_pPoints[i].Y-4, 8, 8));		
		}
	}

	return sts;
}

Status CCurvePath::DrawCloseLines( Graphics* pGraph, int nNumOfPoints, BOOL bDrawPoint )
{
	Pen greenPen(Color(0, 0, 255), 2);
	SolidBrush redBrush(Color::Red);
	Status sts = Ok;
	sts = pGraph->DrawClosedCurve(&greenPen, m_pPoints,  nNumOfPoints, 0.0f);

	if (bDrawPoint)
	{
		for(int i=0; i< nNumOfPoints; i++)
		{
			sts = pGraph->FillEllipse(&redBrush, Rect(m_pPoints[i].X-4, m_pPoints[i].Y-4, 8, 8));		
		}
	}

	return sts;
}

Status CCurvePath::DrawCutRect( Graphics* pGraph, BOOL bDrawPoint )
{
	Pen greenPen(Color(0, 0, 255), 2);
	SolidBrush redBrush(Color::Red);
	Status sts = Ok;
	Rect cutRect;
	cutRect.X = min(m_pPoints[0].X, m_pPoints[1].X);
	cutRect.Y = min(m_pPoints[0].Y, m_pPoints[1].Y);;
	cutRect.Height = abs(m_pPoints[1].Y - m_pPoints[0].Y);
	cutRect.Width = abs(m_pPoints[1].X - m_pPoints[0].X);

	
//	sts = pGraph->DrawClosedCurve(&greenPen, m_pPoints,  nNumOfPoints, 0.0f);
	sts = pGraph->DrawRectangle(&greenPen, cutRect);
	if (bDrawPoint)
	{
		sts = pGraph->FillEllipse(&redBrush, Rect(m_pPoints[0].X-4, m_pPoints[0].Y-4, 8, 8));		
		sts = pGraph->FillEllipse(&redBrush, Rect(m_pPoints[1].X-4, m_pPoints[1].Y-4, 8, 8));		
	}

	return sts;
}

void CCurvePath::Scale( DOUBLE dVertical, DOUBLE dHorizontal )
{
	for (int i=0; i< m_nNumOfPoints; i++)
	{
		m_pPoints[i].X = m_pPoints[i].X * dHorizontal;
		m_pPoints[i].Y = m_pPoints[i].Y * dHorizontal;
	}
}
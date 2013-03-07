#pragma once
#include "Common.h"

class CSliceFrame
{
public:
	CSliceFrame(void);
	~CSliceFrame(void);
protected:
	SLCFRAME m_SlcFrm;
public:
	int Draw(CDC* pDC);
	int Update(SLCFRAME in_SlcFrame);
	//void SetBound(RECT in_rect);
	RECT GetBoundary() const
	{
		return m_SlcFrm.rcBound;
	}

	RECT GetImageRegion() const
	{
		return m_SlcFrm.rcImage;
	}
	BOOL IsOnImage(CPoint in_ptPos);
};

#pragma once
#include "atltypes.h"

class CLineControl
{
public:
	CLineControl(void);
	~CLineControl(void);

protected:
	RECT m_rcFrame;
	POINT m_ptPos;
	COLORREF m_crColor;
	BYTE m_nLineWidth;

public:
	COLORREF GetColor(void);
	POINT GetPos(void);
	RECT GetFrame();
	BYTE GetLineWidth();
	void SetColor(COLORREF in_color);
	void SetPos(POINT in_ptPos);
	void SetFrame(RECT in_rcFrm);
	void SetLineWidth(BYTE in_width);
	int Draw(CDC* pDC);

	
};

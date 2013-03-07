#pragma once
#include "Common.h"

class CViewLayout
{
public:
	CViewLayout(void);
	~CViewLayout(void);
protected:
	LAYOUT m_Layout;
	int makeCubeLayout(RECT, VOLSIZE, INT);
	int makeHorzLayout(RECT, VOLSIZE, INT);
	int makeVertLayout(RECT, VOLSIZE, INT);

public:
	RECT GetViewRect(int in_nIndex);
	int GetLayoutType(void);
	int MakeLayout(RECT, VOLSIZE,DISPLAYTINFO*);
	int Draw(CDC* pDC);
};

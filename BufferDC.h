#pragma once
#include "afxwin.h"

class CBufferDC :
	public CDC
{
	DECLARE_DYNAMIC(CBufferDC)

private:
	HDC m_hOutputDC;
	HDC m_hAttributeDC;
	HDC m_hMemoryDC;

	HBITMAP  m_hPaintBitmap;
	HBITMAP  m_hOldBitmap;

	RECT m_ClientRect;

	BOOL m_bBoundsUpdated;

public:
	CBufferDC(CDC* pDC);
	~CBufferDC(void);

private:
	void Flush();

public:
	UINT SetBoundsRect(LPCRECT lpRectBounds, UINT flags);
	virtual BOOL RestoreDC(int nSavedDC);
};

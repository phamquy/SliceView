#include "StdAfx.h"
#include "BufferDC.h"

IMPLEMENT_DYNAMIC(CBufferDC, CDC)

/*
CBufferDC::CBufferDC(CWnd* pWnd) : CDC(pWnd)
{
	if (pWnd != NULL && CDC::m_hDC != NULL)
	{
		m_hOutputDC    = CDC::m_hDC;
		m_hAttributeDC = CDC::m_hAttribDC;

		pWnd->GetClientRect(&m_ClientRect);

		m_hMemoryDC = ::CreateCompatibleDC(m_hOutputDC);

		m_hPaintBitmap =
			::CreateCompatibleBitmap(
					m_hOutputDC,
					m_ClientRect.right  - m_ClientRect.left,
					m_ClientRect.bottom - m_ClientRect.top);

		m_hOldBitmap = (HBITMAP)::SelectObject(m_hMemoryDC, m_hPaintBitmap);

		CDC::m_hDC       = m_hMemoryDC;
		CDC::m_hAttribDC = m_hMemoryDC;
	}

	m_bBoundsUpdated = FALSE;
}*/


CBufferDC::CBufferDC(CDC* pDC) : CDC()
{
	if (pDC != NULL)
	{
		m_hOutputDC    = pDC->m_hDC;
		m_hAttributeDC = pDC->m_hAttribDC;

		pDC->GetClipBox(&m_ClientRect);

		m_hMemoryDC = ::CreateCompatibleDC(m_hOutputDC);

		m_hPaintBitmap =
		::CreateCompatibleBitmap(
		m_hOutputDC,
		m_ClientRect.right  - m_ClientRect.left,
		m_ClientRect.bottom - m_ClientRect.top);

		m_hOldBitmap = (HBITMAP)::SelectObject(m_hMemoryDC, m_hPaintBitmap);

		CDC::m_hDC       = m_hMemoryDC;
		CDC::m_hAttribDC = m_hMemoryDC;
	}

	m_bBoundsUpdated = FALSE;
}



CBufferDC::~CBufferDC(void)
{
	Flush();

	::SelectObject(m_hMemoryDC, m_hOldBitmap);
	::DeleteObject(m_hPaintBitmap);

	CDC::m_hDC		  = m_hOutputDC;
	CDC::m_hAttribDC = m_hAttributeDC;

	::DeleteDC(m_hMemoryDC);
}

void CBufferDC::Flush()
{
	::BitBlt(
		m_hOutputDC,
		m_ClientRect.left, m_ClientRect.top,
		m_ClientRect.right  - m_ClientRect.left, 
		m_ClientRect.bottom - m_ClientRect.top,
		m_hMemoryDC,
		0, 0,
		SRCCOPY);
}

UINT CBufferDC::SetBoundsRect( LPCRECT lpRectBounds, UINT flags )
{
	if (lpRectBounds != NULL)
	{
		if (m_ClientRect.right  - m_ClientRect.left > lpRectBounds->right  - lpRectBounds->left ||
			m_ClientRect.bottom - m_ClientRect.top  > lpRectBounds->bottom - lpRectBounds->top)
		{
			lpRectBounds = &m_ClientRect;
		}

		HBITMAP bmp =
			::CreateCompatibleBitmap(
					m_hOutputDC, 
					lpRectBounds->right - lpRectBounds->left, 
					lpRectBounds->bottom - lpRectBounds->top);

		HDC tmpDC  = ::CreateCompatibleDC(m_hOutputDC);
		
		HBITMAP oldBmp = (HBITMAP)::SelectObject(tmpDC, bmp);

		::BitBlt(
			tmpDC,
			m_ClientRect.left, m_ClientRect.top,
			m_ClientRect.right  - m_ClientRect.left, 
			m_ClientRect.bottom - m_ClientRect.top,
			m_hMemoryDC,
			0, 0,
			SRCCOPY);

		::SelectObject(tmpDC, oldBmp);
		::DeleteDC(tmpDC);

		HBITMAP old = (HBITMAP)::SelectObject(m_hMemoryDC, bmp);

		if (old != NULL && old != m_hPaintBitmap)
		{
			::DeleteObject(old);
		}

		if (m_hPaintBitmap != NULL)
		{
			::DeleteObject(m_hPaintBitmap);
		}

		m_hPaintBitmap = bmp;

		m_ClientRect = *lpRectBounds;
		m_bBoundsUpdated = TRUE;
	}

	return CDC::SetBoundsRect(lpRectBounds, flags);
}

BOOL CBufferDC::RestoreDC( int nSavedDC )
{
	BOOL ret = CDC::RestoreDC(nSavedDC);

	if (m_bBoundsUpdated)
	{
		SelectObject(m_hPaintBitmap);
	}

	return ret;
}
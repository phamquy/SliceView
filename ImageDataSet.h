#pragma once
#include "Common.h"
#include "SliceObj.h"

class CImageDataSet
{
public:
	CImageDataSet(void);
	~CImageDataSet(void);
	// Size of volume
	VOLSIZE m_tSize;

protected:
	// Pointer to data
	UCHAR* m_pPixelsData;
public:

	VOLSIZE GetSize() const
	{
		return m_tSize;
	}
	// return number of pixel
	unsigned long GetPixelCount(void) const
	{
		return m_tSize.ndx * m_tSize.ndy * m_tSize.ndz;
	}
	
	PUCHAR GetDataBuff() const
	{
		return m_pPixelsData;
	}

	int GetSlice(CViewSliceObj* out_pSlcObj, slicetype in_nSliceType, INT in_nIndex=0 );
	UCHAR GetPixelValueAt(INT in_X, INT in_Y, INT in_Z);

protected:
	int clearVolume(void);
	int resetSize(void);

public:
	int PrepareBuffer(long in_buffSize);

};

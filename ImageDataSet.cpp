#include "StdAfx.h"
#include "ImageDataSet.h"

CImageDataSet::CImageDataSet(void)
{
	m_tSize.ndx = m_tSize.ndy = m_tSize.ndz = 0;
	m_pPixelsData = NULL;
}

CImageDataSet::~CImageDataSet(void)
{
	clearVolume();
}

 
//************************************
// Method:    GetSlice
// FullName:  CImageDataSet::GetSlice
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: CSliceObj * out_pSlcObj: slice object
// Parameter: INT in_nSliceType	: type of plane (XY, YZ, or ZX )
// Parameter: INT in_nIndex:Slice index
// Purpose:   Extract a slice and encapsulate in CSliceObj
//************************************
int CImageDataSet::GetSlice( CViewSliceObj* out_pSlcObj, slicetype in_nSliceType, INT in_nIndex/*=0 */ )
{
	ASSERT(out_pSlcObj != NULL);
	ASSERT(m_pPixelsData != NULL);

	int retcode = SV_NORMAL;
		//the way to extract depend on what kind of slice's plane 
	UCHAR* pBmpData = NULL;
	CSize imgSize(0,0); //in pixel
	INT rowPadder = 0;  //in byte

	switch(in_nSliceType)
	{

	case eAxial:		
		imgSize.cx = m_tSize.ndx;
		imgSize.cy = m_tSize.ndy;
		ASSERT(in_nIndex < m_tSize.ndz);
		

		// Allocate memory for image
		retcode = out_pSlcObj->PrepareMem(imgSize);
		pBmpData = out_pSlcObj->GetOrgBuffer();		//XY plane
		
		//padding bytes for each row
		rowPadder = (sizeof(DWORD)-(imgSize.cx * 3) % sizeof(DWORD)) % sizeof(DWORD);

		if(pBmpData != NULL)
		{
			for (int y=0; y<m_tSize.ndy; y++)
			{

				for (int x=0; x<m_tSize.ndx; x++)
				{
					//because line order in bitmap is inverted
					pBmpData[(y*m_tSize.ndx + x)*3 + rowPadder*y] = GetPixelValueAt(x,m_tSize.ndy-y-1,in_nIndex);		
					//pBmpData[(y*m_tSize.ndx + x)*3] = GetPixelValueAt(x,y,in_nIndex);		//NO INVERT
					pBmpData[(y*m_tSize.ndx + x)*3 + rowPadder*y +1] = GetPixelValueAt(x,m_tSize.ndy-y-1,in_nIndex);
					pBmpData[(y*m_tSize.ndx + x)*3 + rowPadder*y +2] = GetPixelValueAt(x,m_tSize.ndy-y-1,in_nIndex);
				}				
			}
		}
		else
		{
			retcode = SV_MEMORY_ERR;
		}
		break;
	case eCoronal:
		imgSize.cx = m_tSize.ndx;
		imgSize.cy = m_tSize.ndz;
		ASSERT(in_nIndex < m_tSize.ndy);

		//Allocate memory for image
		retcode = out_pSlcObj->PrepareMem(imgSize);
		pBmpData = out_pSlcObj->GetOrgBuffer();		//XZ plane

		//Compute the row's padding bytes
		rowPadder = (sizeof(DWORD)-(imgSize.cx * 3) % sizeof(DWORD)) % sizeof(DWORD);
		if (pBmpData != NULL)
		{
			for (int z=0; z<m_tSize.ndz; z++)
			for (int x=0; x<m_tSize.ndx; x++)
			{
				pBmpData[(z*m_tSize.ndx + x)*3 + rowPadder*z] = GetPixelValueAt(x,in_nIndex,m_tSize.ndz-z-1);
				//pBmpData[(z*m_tSize.ndx + x)*3] = GetPixelValueAt(x,in_nIndex,z);	//no inverted
				pBmpData[(z*m_tSize.ndx + x)*3 + rowPadder*z +1] = GetPixelValueAt(x,in_nIndex,m_tSize.ndz-z-1);
				pBmpData[(z*m_tSize.ndx + x)*3 + rowPadder*z +2] = GetPixelValueAt(x,in_nIndex,m_tSize.ndz-z-1);
			}
		}
		else
		{
			retcode = SV_MEMORY_ERR;
		}
		
		break;
	case eSagittal:
		imgSize.cx = m_tSize.ndy;
		imgSize.cy = m_tSize.ndz;
		ASSERT(in_nIndex < m_tSize.ndx);

		//Allocate memory for image
		retcode = out_pSlcObj->PrepareMem(imgSize);
		pBmpData = out_pSlcObj->GetOrgBuffer();		//YZ plane

		//Compute the row's padding bytes
		rowPadder = (sizeof(DWORD)-(imgSize.cx * 3) % sizeof(DWORD)) % sizeof(DWORD);

		if(pBmpData != NULL)
		{
			for (int z=0; z<m_tSize.ndz; z++)
			for (int y=0; y<m_tSize.ndy; y++)
			{
				pBmpData[(z*m_tSize.ndy + y)*3 + rowPadder*z] = GetPixelValueAt(in_nIndex,y,m_tSize.ndz-z-1);
				//pBmpData[(z*m_tSize.ndy + y)*3] = GetPixelValueAt(in_nIndex,y,z); //no inverted
				pBmpData[(z*m_tSize.ndy + y)*3 + rowPadder*z +1] = GetPixelValueAt(in_nIndex,y,m_tSize.ndz-z-1);
				pBmpData[(z*m_tSize.ndy + y)*3 + rowPadder*z +2] = GetPixelValueAt(in_nIndex,y,m_tSize.ndz-z-1);
			}
		}
		else
		{
			retcode = SV_MEMORY_ERR;
		}
		
	    break;
	default:
			retcode = SV_INVALID_PARAM;
	    break;
	}

	// Make the header for the image
	BITMAPINFOHEADER bmpInfo;
	FillMemory(&bmpInfo, sizeof(BITMAPINFOHEADER), 0x0);
	if(retcode == SV_NORMAL)
	{
		//set slice type
		out_pSlcObj->m_nType = in_nSliceType;
		//make header
		bmpInfo.biSize = sizeof(BITMAPINFOHEADER);
		bmpInfo.biWidth = imgSize.cx;
		bmpInfo.biHeight = imgSize.cy;
		bmpInfo.biPlanes = 1;	
		bmpInfo.biCompression = BI_RGB;
		bmpInfo.biBitCount = 24;		
		out_pSlcObj->SetBitmapInfo(bmpInfo);
		out_pSlcObj->SetIndex(in_nIndex);
		//CopyMemory(out_pSlcObj->GetProcessedBuff(), out_pSlcObj->GetOrgBuffer(), bmpInfo.biSizeImage);
	}

	
	return retcode;	
}


//************************************
// Method:    GetPixelValueAt
// FullName:  CImageDataSet::GetPixelValueAt
// Access:    public 
// Returns:   char
// Qualifier:
// Parameter: INT in_X	 -----	pixel in line
// Parameter: INT in_Y	 |-----|-----|-----|	line in frame
// Parameter: INT in_Z	 #-----|-----|-----#-----|-----|-----#-----|-----|-----# frame in volume
// Purpose:   get value of pixel at specified position
//************************************
UCHAR CImageDataSet::GetPixelValueAt( INT in_X, INT in_Y, INT in_Z )
{
	ASSERT(m_pPixelsData != NULL);
	long nFrameSize = m_tSize.ndx * m_tSize.ndy;
	long nLineSize	= m_tSize.ndx;

	return m_pPixelsData[in_Z*nFrameSize + in_Y*nLineSize + in_X];
}


//************************************
// Method:    clearVolume
// FullName:  CImageDataSet::clearVolume
// Access:    protected 
// Returns:   int
// Qualifier:
// Parameter: void
// Purpose:   Clear buffer of this object
//************************************
int CImageDataSet::clearVolume(void)
{
	if (m_pPixelsData != NULL)
	{
		delete[] m_pPixelsData;
		m_pPixelsData = NULL;
	}
	return 0;
}

//************************************
// Method:    PrepareBuffer
// FullName:  CImageDataSet::PrepareBuffer
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: long in_buffSize
// Purpose:   
//************************************
int CImageDataSet::PrepareBuffer(long in_buffSize)
{
	int retcode = SV_NORMAL;
	clearVolume();
	m_pPixelsData = new UCHAR[in_buffSize];
	if(m_pPixelsData == NULL)
	{
		retcode = SV_MEMORY_ERR;
	}

	return retcode;
}

//************************************
// Method:    resetSize
// FullName:  CImageDataSet::resetSize
// Access:    protected 
// Returns:   int
// Qualifier:
// Parameter: void
// Purpose:   
//************************************
int CImageDataSet::resetSize(void)
{
	m_tSize.ndx = m_tSize.ndy = m_tSize.ndz = 0;
	return 0;
}

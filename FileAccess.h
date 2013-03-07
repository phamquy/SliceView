#pragma once
#include "Common.h"
#include "ImageDataSet.h"

class CFileAccess
{
public:
	CFileAccess(void);
	virtual ~CFileAccess(void);
	virtual int ReadPatientInfo(CPatientInfo* out_pPatInfo)
	{
		ASSERT(FALSE);
		return SV_INVALID_PARAM;
	};
	virtual int ReadImageDataSet(CImageDataSet* out_pImgDataSet)
	{
		ASSERT(FALSE);
		return SV_INVALID_PARAM;
	};

	virtual int Open(CString in_szFilePath)
	{
		ASSERT(FALSE);
		return SV_INVALID_PARAM;
	};
	virtual int Close(void)
	{
		ASSERT(FALSE);
		return SV_INVALID_PARAM;
	};

protected:
	CString m_szFilePath;
};

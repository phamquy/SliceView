#pragma once
#include "Common.h"
#include "ImageDataSet.h"

class CFileExporter
{
public:
	CFileExporter(void);
	virtual ~CFileExporter(void);

protected:
	CString m_szFilePath;

public:	
	virtual int Open(CString in_szFilePath)	
	{ 
		ASSERT(FALSE); 
		return SV_INVALID_PARAM;
	};
	
	virtual int ExportImageDataSet(CImageDataSet* in_pImgDataSet)
	{ 
		ASSERT(FALSE); 
		return SV_INVALID_PARAM;
	};

	virtual int Close()
	{ 
		ASSERT(FALSE); 
		return SV_INVALID_PARAM;
	};
};

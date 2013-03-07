#pragma once
#include "afx.h"
#include "ImageDataSet.h"
#include "FileAccess.h"

class CRAWAccess: public CFileAccess
{
public:
	CRAWAccess(void);
	~CRAWAccess(void);
protected:
	
	CFile m_oFile;
public:
	virtual int ReadImageDataSet(CImageDataSet* out_pDataSet);
	virtual int Open(CString in_szFilePath);
	virtual int Close(void);
};

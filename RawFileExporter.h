#pragma once
#include "fileexporter.h"

class CRawFileExporter :
	public CFileExporter
{
public:
	CRawFileExporter(void);
	virtual ~CRawFileExporter(void);

protected:
	CFile m_oFile;

public:
	virtual int Open(CString in_szFilePath);
	virtual int ExportImageDataSet(CImageDataSet* in_pImgDataSet);
	virtual int Close();
};

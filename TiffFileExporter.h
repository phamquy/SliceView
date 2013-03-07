#pragma once
#include "fileexporter.h"

class CTiffFileExporter :
	public CFileExporter
{
public:
	CTiffFileExporter(void);
	virtual ~CTiffFileExporter(void);
	
public:	
	virtual int Open(CString in_szFilePath);
	virtual int ExportImageDataSet(CImageDataSet* in_pImgDataSet);
	virtual int Close();
};

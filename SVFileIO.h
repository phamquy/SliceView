#pragma once
#include "FileAccess.h"
#include "FileExporter.h"


class CSVFileIO
{
public:
	CSVFileIO(void);
	//CSVFileIO(CString)
	~CSVFileIO(void);
	int ReadPatientInfo(CPatientInfo* out_pPatInfo)
	{
		ASSERT(m_pAccessor != NULL);
		return m_pAccessor->ReadPatientInfo(out_pPatInfo);
	};
	
	int ReadImageDataSet(CImageDataSet* out_pImgDataSet)
	{
		ASSERT(m_pAccessor != NULL);
		return m_pAccessor->ReadImageDataSet(out_pImgDataSet);
	};

	int ExportImageDataSet(CImageDataSet* in_pImgDataSet)
	{
		ASSERT(m_pExporter != NULL);
		return m_pExporter->ExportImageDataSet(in_pImgDataSet);
	}
	
	int Open(CString in_szFilePath, fileopenmode in_eMode = eRead);
	int Close(void);

protected:
	CFileAccess* m_pAccessor;
	CFileExporter* m_pExporter;

};

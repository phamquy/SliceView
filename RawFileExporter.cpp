#include "StdAfx.h"
#include "RawFileExporter.h"

CRawFileExporter::CRawFileExporter(void)
{
}

CRawFileExporter::~CRawFileExporter(void)
{
}


//************************************
// Method:    Open
// FullName:  CRawFileExporter::Open
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: CString in_szFilePath
//************************************
int CRawFileExporter::Open( CString in_szFilePath )
{
	int ret = SV_NORMAL;
	m_szFilePath = in_szFilePath;
	CFileException ex;
	BOOL bflagOpen = m_oFile.Open(m_szFilePath, CFile::modeWrite | CFile::modeCreate, &ex);
	if(!bflagOpen)
	{
		ret = SV_FILEIO_ERROR;
		ex.ReportError();
		m_szFilePath = _T("");
	}

	return ret;
}

//************************************
// Method:    ExportImageDataSet
// FullName:  CRawFileExporter::ExportImageDataSet
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: CImageDataSet * in_pImgDataSet
//************************************
int CRawFileExporter::ExportImageDataSet( CImageDataSet* in_pImgDataSet )
{
	if(in_pImgDataSet == NULL)
	{
		ASSERT(FALSE);
		return SV_SYSTEM_ERR;
	}	

	int retcode = SV_NORMAL;	
	UINT nLength = in_pImgDataSet->GetSize().ndx*in_pImgDataSet->GetSize().ndy*in_pImgDataSet->GetSize().ndz;
	try
	{
		m_oFile.Write(in_pImgDataSet->GetDataBuff(), nLength);		
	}	
	catch (CException* ex)
	{	
		retcode = SV_SYSTEM_ERR;
		ex->ReportError();
	}

	return retcode;
}

//************************************
// Method:    Close
// FullName:  CRawFileExporter::Close
// Access:    public 
// Returns:   int
// Qualifier:
//************************************
int CRawFileExporter::Close()
{
	int ret = SV_NORMAL;
	m_szFilePath = _T("");
	m_oFile.Close();
	return ret;
}

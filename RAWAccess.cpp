#include "StdAfx.h"
#include "RAWAccess.h"
#include "Common.h"
#include "VolSizeDlg.h"

CRAWAccess::CRAWAccess(void)
{
}

CRAWAccess::~CRAWAccess(void)
{
}

//************************************
// Method:    ReadImageDataSet
// FullName:  CRAWAccess::ReadImageDataSet
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: CImageDataSet * out_pDataSet
// Purpose:   
//************************************
int CRAWAccess::ReadImageDataSet(CImageDataSet* out_pDataSet)
{
	ASSERT(out_pDataSet != NULL);
	int retcode = SV_NORMAL;

	//check pointer
	if (out_pDataSet == NULL)	return SV_MEMORY_ERR;

	CVolSizeDlg szDlg;
	if(szDlg.DoModal() == IDOK)
	{
		out_pDataSet->m_tSize.ndx = szDlg.m_nCX;
		out_pDataSet->m_tSize.ndy = szDlg.m_nCY;
		out_pDataSet->m_tSize.ndz = szDlg.m_nCZ;
	}

	//Copy file content to imageDataSet
	ULONG nFileLength = m_oFile.GetLength();
	VOLSIZE VolSize = out_pDataSet->m_tSize;

	if(nFileLength > SV_VOL_MAXSIZE) retcode =  SV_SIZE_OVERLOAD;
	if(nFileLength != VolSize.ndx*VolSize.ndy*VolSize.ndz) retcode = SV_INVALID_PARAM;

	if(retcode == SV_NORMAL)
	{
		out_pDataSet->PrepareBuffer(nFileLength);		
		m_oFile.Seek(0, CFile::begin);
		m_oFile.Read(out_pDataSet->GetDataBuff(), nFileLength);
	}

	return retcode;
}

//************************************
// Method:    Open
// FullName:  CRAWAccess::Open
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: CString in_szFilePath
// Purpose:   
//************************************
int CRAWAccess::Open(CString in_szFilePath)
{
	int ret = SV_NORMAL;
	m_szFilePath = in_szFilePath;
	CFileException ex;
	BOOL bflagOpen = m_oFile.Open(m_szFilePath, CFile::modeRead | CFile::shareDenyWrite, &ex);
	if(!bflagOpen)
	{
		ret = SV_FILEIO_ERROR;
		ex.ReportError();
		m_szFilePath = _T("");
	}

	return ret;
}

//************************************
// Method:    Close
// FullName:  CRAWAccess::Close
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: void
// Purpose:   
//************************************
int CRAWAccess::Close(void)
{
	int ret = SV_NORMAL;
	m_szFilePath = _T("");
	m_oFile.Close();
	return ret;
}

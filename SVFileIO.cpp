#include "StdAfx.h"
#include "SVFileIO.h"
#include "Utility.h"
#include "DCMAccess.h"
#include "RAWAccess.h"
#include "FileExporter.h"
#include "RawFileExporter.h"
#include "TiffFileExporter.h"

CSVFileIO::CSVFileIO(void)
: m_pAccessor(NULL), m_pExporter(NULL)
{
}

CSVFileIO::~CSVFileIO(void)
{

}

//************************************
// Method:    Open
// FullName:  CSVFileIO::Open
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: CString in_szFilePath
// Purpose:   
//************************************
int CSVFileIO::Open( CString in_szFilePath, fileopenmode in_eMode )
{
	int retcode = SV_NORMAL;
	
	filetype eType  = (filetype)CUtility::GetFileType(in_szFilePath);

	switch (in_eMode)
	{
	case eRead:
		switch(eType)
		{
		case eDicom:
			m_pAccessor = new CDCMAccess();		
			break;

		case eRaw:		
			m_pAccessor = new CRAWAccess();
			break;
		default:
			retcode = SV_INVALID_PARAM;
			break;
		}

		if (retcode == SV_NORMAL)
		{
			if(m_pAccessor == NULL) retcode = SV_MEMORY_ERR;
			else retcode = m_pAccessor->Open(in_szFilePath);
		}
		break;
	case eWrite:
		switch(eType)
		{
		case eRaw:
			m_pExporter = new CRawFileExporter();
			break;
		case eTiff:
			m_pExporter = new CTiffFileExporter();
			break;
		default:
			retcode = SV_INVALID_PARAM;
			break;
		}
		if (retcode == SV_NORMAL)
		{
			if(m_pExporter == NULL) retcode = SV_MEMORY_ERR;
			else retcode = m_pExporter->Open(in_szFilePath);
		}
		break;
	default:
		retcode = SV_INVALID_PARAM;
		break;
	}

	return retcode;	
}

//************************************
// Method:    Close
// FullName:  CSVFileIO::Close
// Access:    public 
// Returns:   int
// Qualifier:
// Purpose:   
//************************************
/*
In case of the exporting function and input is independence
this design have a disadvantage:The close function need to 
know the mode of closing
*/

int CSVFileIO::Close()
{
	int retcode = SV_NORMAL;
	
	if (m_pAccessor != NULL)
	{
		retcode = m_pAccessor->Close();
		delete m_pAccessor;
		m_pAccessor = NULL; 
	}

	if(m_pExporter != NULL)
	{
		retcode = m_pExporter->Close();
		delete m_pExporter;
		m_pExporter = NULL;
	}

	return retcode;
};


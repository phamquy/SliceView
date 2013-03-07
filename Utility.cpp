#include "StdAfx.h"
#include "Utility.h"
#include "Common.h"
#include "gdiplus.h"
using namespace Gdiplus;

CUtility::CUtility(void)
{
}

CUtility::~CUtility(void)
{
}


//************************************
// Method:    UnicodeToAnsi
// FullName:  CUtility::UnicodeToAnsi
// Access:    public 
// Returns:   DWORD
// Qualifier:
// Parameter: LPCWSTR pszW
// Parameter: LPSTR * ppszA
// Purpose:   
//************************************
DWORD CUtility::UnicodeToAnsi( LPCWSTR pszW, LPSTR* ppszA )
{
	ULONG cbAnsi, cCharacters;
	DWORD dwError;

	// If input is null then just return the same.
	if (pszW == NULL)
	{
		*ppszA = NULL;
		return NOERROR;
	}

	cCharacters = (ULONG)wcslen(pszW)+1;
	// Determine number of bytes to be allocated for ANSI string. An
	// ANSI string can have at most 2 bytes per character (for Double
	// Byte Character Strings.)
	cbAnsi = cCharacters*2;

	*ppszA = (LPSTR) malloc(cbAnsi);
	if (NULL == *ppszA)
		return ERROR_NOT_ENOUGH_MEMORY;

	// Convert to ANSI.
	if (0 == WideCharToMultiByte(CP_ACP, 0, pszW, cCharacters, *ppszA,
		cbAnsi, NULL, NULL))
	{
		dwError = GetLastError();
		free(*ppszA);
		*ppszA = NULL;
		return dwError;
	}
	return ERROR_SUCCESS;
}

//************************************
// Method:    GetFileType
// FullName:  CUtility::GetFileType
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: CString in_pzFileName
// Purpose:   
//************************************
int CUtility::GetFileType(CString  in_pzFileName)
{	
	filetype eType = eUnKnown;

	if (IsDCM(in_pzFileName))
	{
		eType = eDicom;
	}
	else if(IsRAW(in_pzFileName))
	{
		eType = eRaw;
	}
	else if(IsTIF(in_pzFileName))
	{
		eType = eTiff;
	}
	return eType;
}


//************************************
// Method:    IsDCM
// FullName:  CUtility::IsDCM
// Access:    public 
// Returns:   filetype
// Qualifier:
// Parameter: CString in_pzFileName
// Purpose:   
//************************************
BOOL CUtility::IsDCM(CString in_pzFileName)
{
//	ASSERT(FALSE);
	CString Ext = in_pzFileName.Right(SV_EXT_DCM_LEN);
	if (!Ext.CompareNoCase(SV_EXT_DICOM))
		return TRUE;
	else
		return FALSE;
}

//************************************
// Method:    IsRAW
// FullName:  CUtility::IsRAW
// Access:    public 
// Returns:   filetype
// Qualifier:
// Parameter: CString in_pzFileName
// Purpose:   
//************************************
BOOL CUtility::IsRAW(CString in_pzFileName)
{
//	ASSERT(FALSE);
	CString Ext = in_pzFileName.Right(SV_EXT_RAW_LEN);
	if (!Ext.CompareNoCase(SV_EXT_RAW))
		return TRUE;
	else
		return FALSE;
}

//************************************
// Method:    IsTIF
// FullName:  CUtility::IsTIF
// Access:    public 
// Returns:   BOOL
// Qualifier:
// Parameter: CString in_pzFileName
//************************************
BOOL CUtility::IsTIF( CString in_pzFileName )
{
	BOOL isTIFF = FALSE;
	in_pzFileName.Right(SV_EXT_TIFF_LEN);
	if (!in_pzFileName.Right(SV_EXT_TIFF_LEN).CompareNoCase(SV_EXT_TIFF))
		isTIFF = TRUE;

	if (!in_pzFileName.Right(SV_EXT_TIF_LEN).CompareNoCase(SV_EXT_TIF))
		isTIFF = TRUE;

	return isTIFF;
}

//************************************
// Method:    DeleteMem
// FullName:  CUtility::DeleteMem
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: UCHAR * in_pmem
// Purpose:   
//************************************
void CUtility::DeleteMem(UCHAR* in_pmem)
{
	if (in_pmem != NULL)
	{
		delete[] in_pmem;
		in_pmem = NULL;
	}
}

//************************************
// Method:    ReportError
// FullName:  CUtility::ReportError
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: int in_errCode
// Purpose:   
//************************************
void CUtility::ReportError(int in_errCode)
{
	AfxMessageBox(L"Loi me no roi!");
}

//************************************
// Method:    GetSliceName
// FullName:  CUtility::GetSliceName
// Access:    public 
// Returns:   CString
// Qualifier:
// Parameter: int in_nType
// Purpose:   
//************************************
CString CUtility::GetSliceName(int in_nType)
{
	CString str = _T("");
	switch(in_nType)
	{
	case eAxial:
		str = SV_STR_AXIAL;
		break;
	case eCoronal:
		str = SV_STR_CORONAL;
		break;
	case eSagittal:
		str = SV_STR_SAGITTAL;
	    break;
	case eVolume:
		str = SV_STR_VOLUME;
		break;
	default:
	    break;
	}
	return str;
}

//************************************
// Method:    AnsiToUnicode
// FullName:  CUtility::AnsiToUnicode
// Access:    public 
// Returns:   DWORD
// Qualifier:
// Parameter: LPCSTR pszA
// Parameter: LPWSTR * ppszW
// Purpose:   
//************************************
DWORD CUtility::AnsiToUnicode(LPCSTR pszA, LPWSTR* ppszW)
{

	ULONG cCharacters;
	DWORD dwError;

	// If input is null then just return the same.
	if (NULL == pszA)
	{
		*ppszW = NULL;
		return NOERROR;
	}

	// Determine number of wide characters to be allocated for the
	// Unicode string.
	cCharacters =  (ULONG)strlen(pszA)+1;

	*ppszW = (LPWSTR) malloc(cCharacters*2);
	if (NULL == *ppszW)
		return ERROR_NOT_ENOUGH_MEMORY;

	// Covert to Unicode.
	if (0 == MultiByteToWideChar(CP_ACP, 0, pszA, cCharacters,
		*ppszW, cCharacters))
	{
		dwError = GetLastError();
		free(*ppszW);
		*ppszW = NULL;
		return dwError;
	}

	return ERROR_SUCCESS;
}

//************************************
// Method:    GetEncoderClsid
// FullName:  CUtility::GetEncoderClsid
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: const WCHAR * format
// Parameter: CLSID * pClsid
//************************************
int CUtility::GetEncoderClsid( const WCHAR* format, CLSID* pClsid )
{	
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if(size == 0)
		return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if(pImageCodecInfo == NULL)
		return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for(UINT j = 0; j < num; ++j)
	{
		if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}    
	}

	free(pImageCodecInfo);

	return -1;  // Failure	
}



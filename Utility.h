#pragma once

class CUtility
{
public:
	CUtility(void);
	~CUtility(void);
	// Convert from WCHAR to CHAR 
	static DWORD UnicodeToAnsi(LPCWSTR pszW, LPSTR* ppszA);
	static int GetFileType(CString  in_pzFileName);
	static BOOL IsDCM(CString  in_pzFileName);
	static BOOL IsRAW(CString  in_pzFileName);
	static BOOL IsTIF(CString  in_pzFileName);
	static void DeleteMem(UCHAR* in_pmem);
	static void ReportError(int in_errCode);
	static CString GetSliceName(int in_nType);
	static DWORD AnsiToUnicode(LPCSTR pszA, LPWSTR* ppszW);
	static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
	
};

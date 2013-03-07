#include "StdAfx.h"
#include "LutProc.h"
#include "FunctionGenerator.h"
#include <math.h>

CLutProc::CLutProc(void)
{
	LUT lut;
	lut.nBirght = 0;
	lut.nContrast= 0;
	m_funcType = eLinear;
	MakeFunctionTable(lut);
}

CLutProc::CLutProc(LUT lut)
: m_funcType(0)
{
	m_funcType = eLinear;
	MakeFunctionTable(lut);
}

CLutProc::~CLutProc(void)
{

}

//************************************
// Method:    ApplyProc
// FullName:  CLutProc::ApplyProc
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: CSliceObj * in_pSlice
// Purpose:   
//************************************
int CLutProc::ApplyProc(CViewSliceObj*  in_pSlice)
{
	INT retcode = SV_NORMAL;
	UCHAR* pOrgData = in_pSlice->GetOrgBuffer();
	UCHAR* pProcData = in_pSlice->GetProcessedBuff();

	ASSERT(pOrgData!=NULL);
	ASSERT(pProcData!=NULL);
	
	if((pOrgData == NULL)||(pProcData == NULL))
	{
		retcode = SV_MEMORY_ERR;
	}

	if(retcode == SV_NORMAL)
	{
		// TEST [3/4/2008 QUYPS]
		CSize imgSize = in_pSlice->GetSize();
		for (int i=0;i<(imgSize.cy*imgSize.cx*3); i++)
		{		
			*(pProcData + i) = m_Func[*(pOrgData + i)];
		}
	}	
	return retcode;
}

//************************************
// Method:    MakeFunctionTable
// FullName:  CLutProc::MakeFunctionTable
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: LUT in_lut
// Purpose:   
//************************************
int CLutProc::MakeFunctionTable(LUT in_lut)
{
/*
	DOUBLE maxPow = pow(SV_GRAYLEVELS-1,0.5-1);
	for (INT i=0; i< SV_GRAYLEVELS;i++)
	{		
		DOUBLE temp = (2 * pow(i,0.5))/maxPow;
		
		if (temp < 0)
			m_Func[i]= 0;
		else
			m_Func[i] = min(temp, 255);		
	}
	return 0;
*/

/*
	for (INT i=0; i< SV_GRAYLEVELS;i++)
	{	
		//int temp = i+ 64;
		m_Func[i] = i;
	}
	return 0;
*/

/*
	for (INT i=0; i< SV_GRAYLEVELS;i++)
	{
		LONG temp = (log(DOUBLE(1 + i)) * 255) / log(DOUBLE(256));
		m_Func[i] = min(temp, 255);
	}
	return 0;
*/

	int retcode = SV_NORMAL;

	CFunctionGenerator objGenerator;
	retcode = objGenerator.GenerateTable(m_funcType, m_Func, in_lut);
	
#ifdef _DEBUG
// 	for(int i=0;i< SV_GRAYLEVELS;i++)
// 	{
// 		TRACE2("\nEntry[%d]=%d",i,m_Func[i]);
// 	}
#endif

	return retcode;


// 	int retcode = SV_NORMAL;
// 	
// 
// 	return retcode;
}



void CLutProc::SetType(int in_nType)
{
	m_funcType = in_nType;		
}

int CLutProc::GetType(void)
{
	return m_funcType;
}

#include "StdAfx.h"
#include "FunctionGenerator.h"
#include <math.h>

CFunctionGenerator::CFunctionGenerator(void)
{
}

CFunctionGenerator::~CFunctionGenerator(void)
{
}

//************************************
// Method:    GenerateTable
// FullName:  CFunctionGenerator::GenerateTable
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: INT in_nType
// Parameter: UCHAR * out_Table
// Parameter: LUT in_lu
// Purpose:   
//************************************
int CFunctionGenerator::GenerateTable( INT in_nType, UCHAR* out_Table, LUT in_lu )
{
	int retcode = SV_NORMAL;
	//Convert brightness from percent measure to gray value
	int nBrightness = ((in_lu.nBirght - SV_BRIG_MIN)*(SV_BRIG_ADJUST_MAX-SV_BRIG_ADJUST_MIN))/(SV_BRIG_MAX - SV_BRIG_MIN) +  SV_BRIG_ADJUST_MIN +1;
	//Convert contrast from percent to angle
	int nContrast = ((in_lu.nContrast - SV_CONS_MIN)*(SV_CONS_ADJUST_MAX-SV_CONS_ADJUST_MIN))/(SV_CONS_MAX - SV_CONS_MIN) +  SV_CONS_ADJUST_MIN;

	//establish line: Y = Ax + B
	DOUBLE A = tan(((DOUBLE)nContrast* SV_PI)/180);
	DOUBLE B = SV_BRIG_ADJUST_MAX+1 - (SV_BRIG_ADJUST_MAX  - nBrightness)*A;

	TRACE3("\nBrightness:%d\tContrast:%d\tContrastaAngl:%f", nBrightness, nContrast, A);
	//generate table
	DOUBLE temp = 0;
	for (int i=0; i< SV_GRAYLEVELS; i++)
	{
		temp = A*(DOUBLE)i + B;

		if (temp < 0)
			temp = 0;
		
		if (temp >= SV_GRAYLEVELS)
			temp = SV_GRAYLEVELS-1;
		
		out_Table[i] = temp;
 	}
	return retcode;
}

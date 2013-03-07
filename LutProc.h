#pragma once

#include "SliceProc.h"
#include "Common.h"

class CLutProc: public CSliceProc
{
public:
	CLutProc(LUT);
	CLutProc(void);
	virtual ~CLutProc(void);
	virtual int ApplyProc(CViewSliceObj*  in_pSlice);
private:
	UCHAR m_Func[SV_GRAYLEVELS];
public:
	int MakeFunctionTable(LUT in_lut);
protected:
	int m_funcType;
public:
	void SetType(int in_nType);
	int GetType(void);
};

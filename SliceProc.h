#pragma once
#include "SliceObj.h"

class CViewSliceObj;

class CSliceProc
{
public:
	CSliceProc(void);
	virtual ~CSliceProc(void);
	virtual int ApplyProc(CViewSliceObj* in_pSlice);
};

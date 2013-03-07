#pragma once
#include "Common.h"

class CFunctionGenerator
{
public:
	CFunctionGenerator(void);
	~CFunctionGenerator(void);

protected:
	
private:


public:
	virtual int GenerateTable( INT in_nType, UCHAR* out_Table, LUT in_lu);

};

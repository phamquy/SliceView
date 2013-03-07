#pragma once
#include "infolayout.h"

class CPatientInfo
{
public:
	CPatientInfo(void);
	~CPatientInfo(void);
protected:
	CInfoLayout m_objInfoLayout;
public:
	int Draw(CDC* pDC);
	int UpdateInfoLayout(CInfoLayout in_infLayout);
};

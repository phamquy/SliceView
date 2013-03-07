#pragma once
#include "ImageDataSet.h"

class ShearWarpRender
{
public:
	
	//define the size of volume
	int m_Xlen;
	int m_YLen;
	int m_ZLen;
	unsigned char* m_pRawData;
	VOID *m_vpcRender;
	BOOL m_UseOctree;
	BOOL m_UseClassified;
	BOOL m_UseRawData;

	void SetRawSource(CImageDataSet* in_pDataSource);
	
	ShearWarpRender(void)
	{ 
		m_Xlen = m_YLen = m_ZLen = 0;
		m_pRawData = NULL;
		m_vpcRender = NULL;
		m_UseClassified = TRUE;
		m_UseOctree = FALSE;
		m_UseRawData = TRUE;		
	};

	ShearWarpRender(int xlen, int ylen, int zlen, unsigned char* pRawData)
	{
		m_Xlen = xlen;
		m_YLen = ylen;
		m_ZLen = zlen;
		m_pRawData = pRawData;
	}

	~ShearWarpRender(void);

	int InitRender(CImageDataSet* in_pDataSource, BOOL flgUseRawData, BOOL flgUseOctree,BOOL flgUseClassifiedVol);
	int Make_Volume();
	int Make_Octree();
	int Classify();
	int Render(CViewSliceObj* outSlice, int nWidth, int nHeight ,DOUBLE nRotateX, DOUBLE nRotateY, DOUBLE nRotateZ);
	int SetRenderParam();

};

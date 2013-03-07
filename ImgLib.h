#pragma once
#include "afxwin.h"
#include "afxcoll.h"
#include "Common.h"
#include <math.h>

class CImgLib
{
public:
	CImgLib(void);
	~CImgLib(void);

	static int Binary(UCHAR* pSource, UCHAR* pDest, int width, int height, int theshold);
	static int Binary(UCHAR* pSource, int size, int theshold);
	static int BinBoundaryDetect(UCHAR* pSource, UCHAR* pDest, int width, int height);
	static int Invert(UCHAR* pSource, UCHAR* pDest, int width, int height, int mode = 0);
	static int Mask(UCHAR* pSource, UCHAR* pDest, UCHAR* pMask, int width, int height,int mode=0, int nMaskVal = SV_MEANNING_MARK);
	static int Mask(UCHAR* io_pSource, UCHAR* pMask, int size, int nMaskVal = SV_MEANNING_MARK);
	static int Mask(UCHAR* pSource, UCHAR* pDest, UCHAR* pMask, int size, int nMaskVal = SV_MEANNING_MARK);
	
	static int BinRGBoundary(UCHAR* pSource, UCHAR* pDest, int width, int height);
	static void RG3D(UCHAR* pVol, VOLSIZE volSize, VOLPOS volPos);
	static int RG2D(UCHAR* pSource, UCHAR* pResult, int width, int hieght, POINT initPos, UCHAR nThreshold);

	static void GrayAdjust(UCHAR* pSource, UCHAR* pResult, UCHAR* pLUT, int width, int height);
	static void GenerateLinearLUT(UCHAR* out_Table, int nBright, int contrast , INT in_nType = 0);
	static void BinBoundaryDetectEx( UCHAR* pSource, UCHAR* pDest, int imgwidth, int imgheight, int x, int y, int reWidth, int reHeight);
	static CRect GetRegion2DBB(UCHAR* pSource, int width, int height, UCHAR value = 255);

	static void RG3DEx( UCHAR* pVolSrc, VOLSIZE srcSize, UCHAR* pCutMask, UCHAR* pVolRes, VOLPOS pos3D, VOLSIZE resSize, Point3DVector& seedPoints, int nDiffValue,int nMode = 0);

private:
	static BOOL isFriend(UCHAR nNeighbor, UCHAR nAverageValOfFriendship, UCHAR nBoundaryOfFriendship );
	
};

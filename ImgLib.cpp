#include "stdafx.h"
#include "ImgLib.h"
#include "Common.h"



CImgLib::CImgLib(void)
{
}

CImgLib::~CImgLib(void)
{
}

int CImgLib::Binary( UCHAR* pSource, UCHAR* pDest, int width, int height, int theshold )
{
	ASSERT((pSource != NULL)&&(pDest != NULL));
	if (theshold <= 0) memset(pDest, 255, width * height);
	if (theshold >= 255) memset(pDest, 0, width * height);

	UCHAR thes = (UCHAR) theshold;
	for (int i=0; i< width * height; i++)
	{
		pDest[i]= (pSource[i] >= thes)?255:0;
	}			

	return 0;
}

int CImgLib::Binary( UCHAR* pSource, int size, int theshold )
{
	ASSERT((pSource != NULL));
	for (int i=0; i< size; i++)
	{
		pSource[i]= (pSource[i] >= theshold)?255:0;		
	}			

	return 0;
}

//************************************
// Method:    BinBoundaryDetectEx
// FullName:  CImgLib::BinBoundaryDetectEx
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: UCHAR * pSource : pointer to the original data image
// Parameter: UCHAR * pDest : pointer to the result 
// Parameter: int imgwidth : width of the image
// Parameter: int imgheight : height of the image
// Parameter: int x : toplef x 
// Parameter: int y
// Parameter: int reWidth
// Parameter: int reHeight
//************************************
void CImgLib::BinBoundaryDetectEx( 
	UCHAR* pSource, 
	UCHAR* pDest, 
	int imgwidth, 
	int imgheight, 
	int x,
	int y,
	int reWidth,
	int reHeight)
{
	BinBoundaryDetect(pSource, pDest, imgwidth, imgheight);
	
	for (int row = y; row < reHeight + y; row++) 
	{ 
		for (int column = x; column < reWidth + x; column ++) 
		{ 
			if( (pDest[row * imgwidth + column]== SV_MEANNING_MARK) && (pSource[row* imgwidth + column]==SV_MEANINGLESS_MARK))
					pDest[row * imgwidth  + column] = 255; 
			else 
					pDest[row * imgwidth  + column] = 0; 
		} 
	}  
}

//************************************
// Method:    BinBoundaryDetect
// FullName:  CImgLib::BinBoundaryDetect
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: UCHAR * pSource
// Parameter: UCHAR * pDest
// Parameter: int width
// Parameter: int height
//************************************
int CImgLib::BinBoundaryDetect( UCHAR* pSource, UCHAR* pDest, int width, int height )
{
	ASSERT((pSource != NULL)&&(pDest != NULL));
	// Make them both the same
	//CopyMemory(pDest,pSource, width * height);

	//Top-left ---> bottom-right
	for (int row = 1; row < height; row ++)
	{
		for (int column = 1; column < width; column ++)
		{

			if(( pSource[(row-1)* width + column]>0) && (pSource[row*width + column-1]>0))
			{
				if (pSource[row * width + column]==0)
				{	
					pSource[row * width + column]= SV_MEANINGLESS_MARK;					
					pDest[row * width + column]=  SV_MEANNING_MARK;
				}
				else
				{
					pSource[row * width + column]= SV_MEANNING_MARK;
					pDest[row * width + column]= SV_MEANNING_MARK;
				}    				
			}
			else 
			{
				pDest[row * width + column]=  pSource[row * width + column];			
			}
		}
	}

	//bottom-right ---->Top-left 
	for(int row = height-1; row > 0; row --)
	{
		for (int column = width-1; column >0; column --)
		{
			if( (pDest [(row+1)*width + column]==0 || pDest [row* width + column+1]==0) 
				&&	pSource[row * width + column]== SV_MEANINGLESS_MARK)
				pDest [row * width + column]=0;
		}
	}
	return 0;
}
int CImgLib::Invert( UCHAR* pSource, UCHAR* pDest, int width, int height, int mode /*= 0*/ )
{	
	ASSERT((pSource != NULL)&&(pDest != NULL));
	int memsize = width*height;
	switch (mode)
	{
	case SV_INV_DIAG:
		//Diagonal revert
		for (int i=0; i< memsize; i++)
			pDest[memsize-i-1] = pSource[i];			
		break;
	
	case SV_INV_ROW:
		//Vertical revert
		for (int line=0; line<height; line++)
			CopyMemory(pDest + line*width, pSource + (height - line -1)*width, width);			
 		break;

	case SV_INV_COLUME:
		//Horizontal revert
		for (int row=0; row<height; row++)
		{
			UCHAR* pSrcLine = pSource + row*width;
			UCHAR* pDesLine = pDest + row *width;
 			for (int col=0; col <width; col++)
			{
				pDesLine[col] = pSrcLine[width - col -1];
			}
		}
		break;
	}
	return 0;
}

int CImgLib::Mask( UCHAR* pSource, UCHAR* pDest, UCHAR* pMask, int width, int height, int mode, int nMaskVal /*= SV_MEANNING_MARK*/ )
{
	ASSERT((pSource != NULL)&&(pDest != NULL)&&(pMask != NULL));

	for (int i=0; i< width*height;i++)
		if (mode == 1)
		{
			pDest[i] = (pMask[i]==nMaskVal) ? pSource[i] : pDest[i];
		}
		else
		{
			pDest[i] = (pMask[i]==nMaskVal) ? pSource[i] : 0;
		}
		
	return 0;
}

int CImgLib::Mask( UCHAR* io_pSource, UCHAR* pMask, int size, int nMaskVal /*= SV_MEANNING_MARK*/ )
{
	ASSERT((io_pSource != NULL)&&(pMask != NULL));

	for (int i=0; i< size; i++)
		io_pSource[i] = (pMask[i] == nMaskVal) ? io_pSource[i] : 0;
	return 0;	
}

int CImgLib::Mask( UCHAR* pSource, UCHAR* pDest, UCHAR* pMask, int size, int nMaskVal /*= SV_MEANNING_MARK*/ )
{
	ASSERT((pSource != NULL)&&(pDest != NULL)&&(pMask != NULL));

	for (int i=0; i< size; i++)
		pDest[i] = (pMask[i]==nMaskVal) ? pSource[i] : 0;
	return 0;	
}

int CImgLib::BinRGBoundary( UCHAR* pSource, UCHAR* pDest, int width, int height )
{
ASSERT((pSource != NULL)&&(pDest != NULL));

#define MARKER 0
#define SrcAt(x,y)   ((UCHAR)(*((pSource) + (((INT)(y))*((INT)width)) + ((INT)(x)))))
#define DestAt(x,y)  ((UCHAR)(*((pDest) + (((INT)(y))*((INT)width)) + ((INT)(x)))))
#define MaskAt(x,y)  ((UCHAR)(*((pMask  ) + (((INT)(y))*((INT)width)) + ((INT)(x)))))

#define RefSrcAt(x,y)   ((pSource) + (((INT)(y))*((INT)width)) + ((INT)(x)))
#define RefDestAt(x,y)  ((pDest) + (((INT)(y))*((INT)width)) + ((INT)(x)))
#define RefMaskAt(x,y)  ((pMask  ) + (((INT)(y))*((INT)width)) + ((INT)(x)))

#define inBound(x,y)	(((x)<width) && ((y)<height))&&(((x)>=0)&((y)>=0))

		Point2DStack stkRemains;
		//	PointStack stkneighbor;
		int imgSize = width * height;

		UCHAR* pMask = new UCHAR[imgSize];		

		FillMemory(pMask,imgSize,255);

		CPoint first(0,0);
		stkRemains.push(first);

		while (!stkRemains.empty())
		{
			CPoint cur;
			cur = stkRemains.top();
			stkRemains.pop();

			if ( SrcAt(cur.x, cur.y) == 0)
			{
				MaskAt(cur.x, cur.y) = MARKER;

				if ((inBound(cur.x-1, cur.y-1))	&& (SrcAt(cur.x-1, cur.y-1) == 0) && (MaskAt(cur.x-1, cur.y-1) != MARKER))
				{
					MaskAt(cur.x-1, cur.y-1) = MARKER;
					stkRemains.push(CPoint(cur.x-1, cur.y-1));
				}

				if ((inBound(cur.x, cur.y-1)) && (SrcAt(cur.x, cur.y-1) == 0) && (MaskAt(cur.x, cur.y-1) != MARKER))
				{
					MaskAt(cur.x, cur.y-1) = MARKER;
					stkRemains.push(CPoint(cur.x, cur.y-1));
				}

				if ((inBound(cur.x+1, cur.y-1)) && (SrcAt(cur.x+1, cur.y-1) == 0) && (MaskAt(cur.x+1, cur.y-1) != MARKER))
				{
					MaskAt(cur.x+1, cur.y-1) = MARKER;
					stkRemains.push(CPoint(cur.x+1, cur.y-1));
				}

				if ((inBound(cur.x-1, cur.y)) && (SrcAt(cur.x-1, cur.y) == 0) && (MaskAt(cur.x-1, cur.y) != MARKER))
				{
					MaskAt(cur.x-1, cur.y) = MARKER;
					stkRemains.push(CPoint(cur.x-1, cur.y));
				}

				if ((inBound(cur.x+1, cur.y)) && (SrcAt(cur.x+1, cur.y) == 0) && (MaskAt(cur.x+1, cur.y)!= MARKER))
				{
					MaskAt(cur.x+1, cur.y) = MARKER;
					stkRemains.push(CPoint(cur.x+1, cur.y));
				}

				if ((inBound(cur.x-1, cur.y+1)) && (SrcAt(cur.x-1, cur.y+1) == 0) && (MaskAt(cur.x-1, cur.y+1)!= MARKER))
				{
					MaskAt(cur.x-1, cur.y+1) = MARKER;
					stkRemains.push(CPoint(cur.x-1, cur.y+1));
				}

				if ((inBound(cur.x, cur.y+1)) && (SrcAt(cur.x, cur.y+1) == 0)&& (MaskAt(cur.x, cur.y+1)!= MARKER))
				{
					MaskAt(cur.x, cur.y+1) = MARKER;
					stkRemains.push(CPoint(cur.x, cur.y+1));
				}
				if ((inBound(cur.x+1, cur.y+1)) && (SrcAt(cur.x+1, cur.y+1) == 0) && (MaskAt(cur.x+1, cur.y+1)!= MARKER))
				{
					MaskAt(cur.x+1, cur.y+1) = MARKER;
					stkRemains.push(CPoint(cur.x+1, cur.y+1));
				}
			}
		}
		CopyMemory(pDest, pMask, imgSize);
		delete[] pMask;

		return 0;
}

/////////////////////////////////////////////////////////////////////////
#define  VOLMARK  255
#define	 EPSILON  64

void CImgLib::RG3D( UCHAR* pVol, VOLSIZE volSize, VOLPOS initPos)
{
	ASSERT(pVol != NULL);

	UINT totalVoxels = volSize.ndx * volSize.ndy * volSize.ndz;
	UINT framePixels = volSize.ndx * volSize.ndy;
	UINT linePoints	 = volSize.ndx;

#define VolAt(X,Y,Z)						(UCHAR)(*(pVol +(((Z) * framePixels) + (Y) * linePoints + (X))))
#define VolMaskAt(X,Y,Z)					(UCHAR)(*(pMask +(((Z) * framePixels) + (Y) * linePoints + (X))))
#define AtNeighborVol(POS, DX, DY, DZ)		VolAt((POS).nX + (DX), (POS).nY + (DY) , (POS).nZ + (DZ) )
#define AtNeighbor3DMask(POS, DX, DY, DZ)	VolMaskAt((POS).nX + (DX), (POS).nY + (DY) , (POS).nZ + (DZ) )
#define InVolBound(X,Y,Z)					(((X)<volSize.ndx) && ((Y)<volSize.ndy) && ((Z)<volSize.ndz)) && ( ((X)>=0) && ((Y)>=0) && ((Z) >=10))
#define Is3DNeighborInBound(POS,DX,DY,DZ)	InVolBound((POS).nX + (DX), (POS).nY + (DY) , (POS).nZ + (DZ) )
#define NeighborVol(POS, DX, DY, DZ)		CVolPos(((POS).nX + (DX))  ,  ((POS).nY + (DY))  ,  ((POS).nZ + (DZ)))

	Point3DStack remainVoxels;
	LONG numOfFriends = 0;
	UCHAR averageOfFriends = 0;
	CVolPos curPos(0,0,0);
	// Init the mask
	//UCHAR* pMask = new UCHAR[totalVoxels];
	//UCHAR* pMask = (UCHAR*)GlobalAlloc(GPTR, totalVoxels);

	UCHAR* pMask = (UCHAR*)malloc(totalVoxels);
	
	// Init the loop

	remainVoxels.push(CVolPos(initPos.nX, initPos.nY, initPos.nZ));
	numOfFriends = 1;	
	averageOfFriends = VolAt(initPos.nX, initPos.nY, initPos.nZ);
	VolMaskAt(initPos.nX, initPos.nY, initPos.nZ) = VOLMARK;

	// test [4/4/2009 QUYPS]
	UCHAR initValue = averageOfFriends;

	// Growing the region
	while (!remainVoxels.empty())
	{
		// Get the top voxel
		curPos = remainVoxels.top();
		remainVoxels.pop();

		// Check all the neighbor of current voxel
		for (int dx =-1; dx <= 1; dx++)
		for (int dy =-1; dy <= 1; dy++)
		for (int dz =-1; dz <= 1; dz++)
		{
			if ((dx == 0) && (dy == 0) && ( dz == 0)) continue;			
			// For each neighbor check:	
			if(Is3DNeighborInBound(curPos, dx,dy,dz))
			{
				UCHAR neighborval =	AtNeighborVol(curPos, dx,dy,dz);
				//if it in boundary and "good to be a friend" and not "being a friend" yet
				if( ( neighborval == initValue) && (AtNeighbor3DMask(curPos, dx,dy,dz) != VOLMARK))
				{
					//then mark it as a friends
					AtNeighbor3DMask(curPos, dx,dy,dz) = VOLMARK;
					//and push it in to friends list
					remainVoxels.push(NeighborVol(curPos, dx, dy, dz));

					//Update the friendship
					numOfFriends++;
					averageOfFriends = averageOfFriends - ((DOUBLE)(neighborval-averageOfFriends))/(DOUBLE)numOfFriends;
				}
			}			
		}		
	}

	//Apply the mask to volume
	CopyMemory(pVol, pMask, totalVoxels);

	//GlobalFree(pMask);
	free(pMask);
} // End RG3D 


///////////////////////////////////////////////////////////////////////////////////////////////////////
int CImgLib::RG2D(UCHAR* pSource, UCHAR* pDest, int width, int height, POINT initPos, UCHAR nThreshold)
{
	ASSERT((pSource != NULL)&&(pDest != NULL));

#define RG2DMARKER 255
#define RG2DUNMARK 0

#define SrcAt(x,y)   ((UCHAR)(*((pSource) + (((INT)(y))*((INT)width)) + ((INT)(x)))))
#define DestAt(x,y)  ((UCHAR)(*((pDest) + (((INT)(y))*((INT)width)) + ((INT)(x)))))
#define MaskAt(x,y)  ((UCHAR)(*((pMask  ) + (((INT)(y))*((INT)width)) + ((INT)(x)))))
#define inBound(x,y)	(((x)<width) && ((y)<height))&&(((x)>=0)&((y)>=0))

#define AtNeighbor2DSrc(POS, DX, DY)		SrcAt((POS).x + (DX), (POS).y + (DY))
#define AtNeighbor2DMask(POS, DX, DY)		MaskAt((POS).x + (DX), (POS).y + (DY))

#define Is2DNeighborInBound(POS,DX,DY)		inBound((POS).x + (DX), (POS).y + (DY))
#define NeighborPixel(POS, DX, DY)		CPoint( ((POS).x + (DX))  ,  ((POS).y + (DY)) )

	Point2DStack remainPixels;
	int imgSize = width * height;
	UCHAR* pMask = new UCHAR[imgSize];		
	FillMemory(pMask,imgSize,RG2DUNMARK);

	//Init the loop
	CPoint first(initPos);
	remainPixels.push(first);
	MaskAt(first.x, first.y) = RG2DMARKER;
	int numOfFriends = 1;
	UCHAR averageOfFriends = SrcAt(first.x, first.y); 

	// Growing the friendship
	while (!remainPixels.empty())
	{
		CPoint curPixel;
		//Get the next pixel in stack
		curPixel = remainPixels.top();
		remainPixels.pop();

		// Check all the neighbor of current pixel
		for (int dx =-1; dx <= 1; dx++)
			for (int dy =-1; dy <= 1; dy++)
			{
				// ignore the current pixel
				if ((dx == 0) && (dy == 0)) continue;			

				// For each neighbor check:	
				if(Is2DNeighborInBound(curPixel, dx,dy))
				{
					UCHAR neighborval =	AtNeighbor2DSrc(curPixel, dx,dy);

					//if this neighbor in boundary and "good to be a friend" and not "being a fiend" yet
					if( (isFriend(neighborval, SrcAt(first.x, first.y), nThreshold)) && (AtNeighbor2DMask(curPixel, dx,dy) != RG2DMARKER))
					{
						//then mark it as a friends
						AtNeighbor2DMask(curPixel, dx,dy) = RG2DMARKER;

						//and push it in to friends list
						remainPixels.push(NeighborPixel(curPixel, dx, dy));

						//Update the friendship
						averageOfFriends = (averageOfFriends*numOfFriends + neighborval)/(numOfFriends+1);
						numOfFriends++;
					}
				}		
			}
	}
	Mask(pSource, pDest, pMask, width, height, 1, RG2DMARKER);
	delete[] pMask;

	return 0;	
}


BOOL CImgLib::isFriend( UCHAR nNeighborValue, UCHAR nAverageValueOfFriendship, UCHAR nBoundaryOfFriendship )
{
	return (abs(nNeighborValue-nAverageValueOfFriendship) < nBoundaryOfFriendship);
}

void CImgLib::GenerateLinearLUT( UCHAR* out_Table, int Bright, int Contrast , INT in_nType )
{
	ASSERT(out_Table != NULL);
	//Convert brightness from percent measure to gray value
	int nBrightness = ((Bright - SV_BRIG_MIN)*(SV_BRIG_ADJUST_MAX-SV_BRIG_ADJUST_MIN))/(SV_BRIG_MAX - SV_BRIG_MIN) +  SV_BRIG_ADJUST_MIN +1;
	//Convert contrast from percent to angle
	int nContrast = ((Contrast - SV_CONS_MIN)*(SV_CONS_ADJUST_MAX-SV_CONS_ADJUST_MIN))/(SV_CONS_MAX - SV_CONS_MIN) +  SV_CONS_ADJUST_MIN;

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
}

void CImgLib::GrayAdjust( UCHAR* pSource, UCHAR* pResult, UCHAR* pLUT, int width, int height )
{
	ASSERT((pSource != NULL)&&(pResult != NULL)&&(pLUT != NULL));
	for (int i=0; i< width*height; i++ )
	{		
		pResult[i] = pLUT[pSource[i]];
	}
}


/****************************************************
This function detect the boundary rectangle of the region which is collection of pixels
in binary image, the pixels that have a specific gray value

//***************************************************/
CRect CImgLib::GetRegion2DBB( UCHAR* pSource, int width, int height, UCHAR value /*= 255*/ )
{
	CRect rect(0,0,0,0);
	// Init the boundary size equal to the whole source's size
	rect.left = width-1;
	rect.right = 0;
	rect.top = height-1;
	rect.bottom = 0;

	for (int row = 0; row  < height; row++)
		for (int col = 0; col  < width; col++)
	{
		if (pSource[row*width + col] == value)
		{
			rect.left = min(col, rect.left);
			rect.right = max(col, rect.right);

			rect.top = min(row, rect.top);
			rect.bottom = max(row, rect.bottom);			
		}
	}

		return rect;
}


//************************************
// Method:    RG3DEx
// FullName:  CImgLib::RG3DEx
// Access:    public 
// Returns:   void
// Qualifier: 
// Parameter: UCHAR * pVolSrc : Pointer to the memory area of source volume
// Parameter: VOLSIZE srcSize : Size of source volume
// Parameter: UCHAR * pCutMask: the Cutting mask which is defined by user have size is CX, CY of the volume
// Parameter: UCHAR * pVolRes : Pointer to the memory area of the result volume, it is so called sub volume
// Parameter: VOLPOS pos3D    : Position of subvolume in side the source volume
// Parameter: VOLSIZE resSize : Size of result volume
// Parameter: Point3DVector & seedPoints: The set of seed point which is defined by user
// Parameter: int nMode : The parameter determines the mode of the constraint method.
/************************************************************************
NOTE:
- nMode determines the way the constraint the voxel to the interest criterion
- All the volume traveling will be in sub-volume space also being result volume
//*/

#define SV_RG3D_DIFFVAL		32

void CImgLib::RG3DEx( UCHAR* pVolSrc, VOLSIZE srcSize, UCHAR* pCutMask, 
					 UCHAR* pVolRes, VOLPOS pos3D, VOLSIZE resSize, 
					 Point3DVector& seedPoints, int nDiffValue,int nMode /*= 0*/ )
{	
	// Get start time

#define _PERFORMANCE_CHECK
#ifdef _PERFORMANCE_CHECK
	DWORD nStartTime_ms = GetTickCount();
	DWORD nEndTime_ms;
#endif // _PERFORMANCE_CHECK
	
	ASSERT(pVolSrc != NULL);
	ASSERT(pVolRes != NULL);
	ASSERT(!seedPoints.empty());
	
	UINT totalSubVoxels = resSize.ndx * resSize.ndy * resSize.ndz;
	UINT frameSubVoxels = resSize.ndx * resSize.ndy;
	UINT lineSubVoxels	= resSize.ndx;
	
	UINT totalVolVoxels = srcSize.ndx * srcSize.ndy * srcSize.ndz;
	UINT frameVolVoxels = srcSize.ndx * srcSize.ndy;
	UINT lineVolVoxels	= srcSize.ndx;

// Define some macro for the algorithm
// All the X, Y, Z is in sub-volume space coordinate: value from 0 to sub-volume size
#define SubVolAt(X, Y, Z)						(UCHAR)	pVolRes[(Z)*frameSubVoxels + (Y)*lineSubVoxels + (X)]
#define	SubToVol3DAdjust						(pos3D.nZ* frameVolVoxels + pos3D.nY *lineVolVoxels + pos3D.nX)		
#define VolOfSubAt(X, Y, Z)						(UCHAR)	pVolSrc[(Z)*frameVolVoxels + (Y)*lineVolVoxels + (X) + SubToVol3DAdjust]									
#define	SubToVol2DAdjust						(pos3D.nY *lineVolVoxels + pos3D.nX)		
#define CutMaskAt(X, Y)							(UCHAR) pCutMask[(Y)*srcSize.ndx + (X) + SubToVol2DAdjust]
#define AtNeighborVolEx(POS, DX, DY, DZ)		VolOfSubAt((POS).nX + (DX), (POS).nY + (DY) , (POS).nZ + (DZ))
#define AtNeighborSub(POS, DX, DY, DZ)			SubVolAt((POS).nX + (DX), (POS).nY + (DY) , (POS).nZ + (DZ))
#define InSubVolBound(X,Y,Z)					(((X) < resSize.ndx) && ((Y) < resSize.ndy) && ((Z) < resSize.ndz)) && ( ((X) >= 0) && ((Y) >= 0) && ((Z) >= 0))
#define Is3DNeighborInSubBound(POS,DX,DY,DZ)	InSubVolBound((POS).nX + (DX), (POS).nY + (DY) , (POS).nZ + (DZ) )
#define NeighborSubVol(POS, DX, DY, DZ)			CVolPos(((POS).nX + (DX))  ,  ((POS).nY + (DY))  ,  ((POS).nZ + (DZ)))


	Point3DStack remainVoxels;
	LONG numOfFriends = 0;
	DOUBLE averageOfFriends = 0;
	DOUBLE constraintVal =  SV_RG3D_DIFFVAL;
	UCHAR maxSeedVal = 0; 
	UCHAR minSeedVal = 255;
	
	// Init the seed point, transform to the sub-volume space
	for(int i=0; i < seedPoints.size(); i++)
	{
		CVolPos& voxelRef = seedPoints.at(i);
		remainVoxels.push(CVolPos(voxelRef.nX - pos3D.nX, voxelRef.nY - pos3D.nY, voxelRef.nZ - pos3D.nZ));
		CVolPos& newVoxelRef = remainVoxels.top();

/*
#ifdef _DEBUG
		TRACE1("\nTransformed seed points-%d:", i);
		TRACE3("[%d, %d, %d]", newVoxelRef.nX, newVoxelRef.nY, newVoxelRef.nZ);
		TRACE1("\t Value: %d", VolOfSubAt(newVoxelRef.nX , newVoxelRef.nY , newVoxelRef.nZ ));
#endif // _DEBUG
//*/
		numOfFriends++;
		averageOfFriends += VolOfSubAt(newVoxelRef.nX, newVoxelRef.nY, newVoxelRef.nZ);
		maxSeedVal = max(maxSeedVal, VolOfSubAt(newVoxelRef.nX, newVoxelRef.nY, newVoxelRef.nZ));
		minSeedVal = min(minSeedVal, VolOfSubAt(newVoxelRef.nX, newVoxelRef.nY, newVoxelRef.nZ));
	}

	//Compute the constraint of connectivity
	//Compared value
	averageOfFriends = averageOfFriends/numOfFriends;
	//Different standard
	constraintVal = max(abs(maxSeedVal - averageOfFriends), abs(minSeedVal - averageOfFriends));


/*
#ifdef _DEBUG
	TRACE2("\nNum of Seed: %d \t Average: %0.3f", numOfFriends, averageOfFriends);
#endif // _DEBUG
//*/

///*
	// Growing the region
	while (!remainVoxels.empty())
	{
		// Get the top voxel
		CVolPos& curRef = remainVoxels.top();
		remainVoxels.pop();

		// Check all the neighbor of current voxel
		for (int dx =-1; dx <= 1; dx++)
		for (int dy =-1; dy <= 1; dy++)
		for (int dz =-1; dz <= 1; dz++)
		{
			if ((dx == 0) && (dy == 0) && ( dz == 0)) continue;			

			//----------------------------------------------------------------------------------
			// For each neighbor check:	
			// 1. If neighbor is in sub boundary AND inside the mask 
			// 2. YES: If the neighbor is not marked
			// 3.	   YES: Check if the neighbor value is SATISFIED
			// 4.			YES: 1. Mark the neighbor, 2. push in the remainVoxels, 3. Update the friendship
			//----------------------------------------------------------------------------------
			// #1
 			if(Is3DNeighborInSubBound(curRef, dx,dy,dz) && (CutMaskAt(curRef.nX + dx , curRef.nY + dy) == 255))
 			{
/*
#ifdef _DEBUG
				BOOL isNeiInboudary = Is3DNeighborInSubBound(curRef, dx,dy,dz);
				UCHAR NeiMaskAt = CutMaskAt(curRef.nX + dx , curRef.nY + dy);
				UCHAR neivaldbg = AtNeighborVolEx(curRef, dx,dy,dz);
				UCHAR neigmark = AtNeighborSub(curRef, dx,dy,dz);
#endif // _DEBUG
//*/

				UCHAR neighborVal = AtNeighborVolEx(curRef, dx,dy,dz);
				//#2 #3
				if ((AtNeighborSub(curRef, dx,dy,dz) != VOLMARK)  &&  
					//(isFriend(neighborVal, averageOfFriends, SV_RG3D_CONSTRAINT)))
					(isFriend(neighborVal, averageOfFriends, nDiffValue)))
				{
					// #4.1
					AtNeighborSub(curRef, dx, dy, dz) = 255;	

					// #4.2
					remainVoxels.push(NeighborSubVol(curRef, dx, dy, dz));

					numOfFriends ++;
				}
 			}			
		}		
	}
//*/
	// Do the filter to get all the filtered voxels
//*
	for (int z=0; z< resSize.ndz; z++)
	for (int y=0; y< resSize.ndy; y++)
	for (int x=0; x< resSize.ndx; x++)
	{
		if (SubVolAt(x,y,z) == 255)
			SubVolAt(x,y,z) = VolOfSubAt(x,y,z);
	}
//*/	
	
#ifdef _PERFORMANCE_CHECK
	nEndTime_ms = GetTickCount();
	DWORD nTotalTime_s = (nEndTime_ms - nStartTime_ms) / 1000;
	TRACE2("\nTotal time for 3DRG: %d ms = %d s", nEndTime_ms - nStartTime_ms, nTotalTime_s);
	CString str;
	str.Format(L"Total time: %d seconds", nTotalTime_s);
	AfxMessageBox(str);
#endif // _PERFORMANCE_CHECK
	

//#define _EXPORT
#ifdef _EXPORT

	CString strOutputFile;	
	strOutputFile.Format(_T("%s_%dx%dx%d"), SV_EXP_DEFAULTNAME, resSize.ndx, resSize.ndy, resSize.ndz);
	CString strFilters = _T("Raw files (*.raw)|*.raw||");
	CFileDialog dlgSave(FALSE, _T(".raw"), strOutputFile, OFN_OVERWRITEPROMPT, strFilters);

	CFile oFile;
	if (dlgSave.DoModal() == IDOK)
	{
		CFileException ex;
		BOOL bflagOpen = oFile.Open(dlgSave.GetPathName(), CFile::modeWrite | CFile::modeCreate, &ex);
		if(bflagOpen)
		{
			UCHAR temp;
			UINT nLength = resSize.ndx * resSize.ndy * resSize.ndz;
			oFile.Write(pVolRes, nLength);	
			oFile.Flush();
			oFile.Close();
		}	
	}
#endif // _DEBUG
}


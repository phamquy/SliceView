#include "StdAfx.h"
#include "ShearWarpRender.h"
#include "Common.h"
#include "volpack/volume.h"

#define VOLUME_FILE	"vol.rv"
#define OCTREE_FILE	"octree.oct"
#define CLVOLUME_FILE "classvol.cv"

ShearWarpRender::~ShearWarpRender(void)
{
	if(m_vpcRender != NULL)
	{
		vpContext* vpc = (vpContext*) m_vpcRender;
		vpDestroyContext(vpc);
	}
}


//************************************
// Method:    Make_Volume
// FullName:  ShearWarpRender::Make_Volume
// Access:    public 
// Returns:   int
// Qualifier:
// Purpose:   
//************************************
int ShearWarpRender::Make_Volume()
{
	int retCode = SV_NORMAL;
	vpContext *vpc;	/* rendering context */
	unsigned density_size;/* size of density data */
	char *volume;	/* volume data */
	unsigned volume_size;/* size of volume */
	FILE *volume_fd = NULL;

	/* create a context */
	vpc = vpCreateContext();
	
	/* describe the layout of the volume */
	vpSetVolumeSize(vpc, m_Xlen, m_YLen, m_ZLen);
	vpSetVoxelSize(vpc, BYTES_PER_VOXEL, VOXEL_FIELDS, SHADE_FIELDS, CLSFY_FIELDS);
	vpSetVoxelField(vpc, NORMAL_FIELD, NORMAL_SIZE, NORMAL_OFFSET, NORMAL_MAX);
	vpSetVoxelField(vpc, DENSITY_FIELD, DENSITY_SIZE, DENSITY_OFFSET, DENSITY_MAX);
	vpSetVoxelField(vpc, GRADIENT_FIELD, GRADIENT_SIZE, GRADIENT_OFFSET, GRADIENT_MAX);

	/* allocate space for the raw data and the volume */
	density_size = m_Xlen * m_YLen * m_ZLen;

	volume_size = m_Xlen * m_YLen * m_ZLen * BYTES_PER_VOXEL;
	volume = (char*)malloc(volume_size);

	if (volume == NULL) {
		retCode = SV_MEMORY_ERR;
	}

	if (!retCode)
	{
		vpSetRawVoxels(vpc, volume, volume_size, BYTES_PER_VOXEL, 
			m_Xlen * BYTES_PER_VOXEL, 
			m_YLen * m_Xlen * BYTES_PER_VOXEL);
		/* compute surface normals (for shading) and gradient magnitudes (for classification) */
		if (vpVolumeNormals(vpc, m_pRawData, density_size, DENSITY_FIELD,GRADIENT_FIELD, NORMAL_FIELD) != VP_OK)
			retCode = SV_VOLPACK_ERR;
	}
	
	//store volume to file
	if(!retCode)
	{
		volume_fd = fopen(VOLUME_FILE, WRITE_MODE);
		if(volume_fd == NULL) retCode = SV_FILEIO_ERROR;
		if (vpStoreRawVolume(vpc, volume_fd) != VP_OK)
		{
			fprintf(stderr, "VolPack error: %s\n", vpGetErrorString(vpGetError(vpc)));
			retCode = SV_VOLPACK_ERR;
		}
	}	
	fclose(volume_fd);	
	
	// volume memory free is in Destroy task of volpack [3/5/2009 QUYPS]
	//delete[] volume;

	vpDestroyContext(vpc);
	return retCode;
}



//************************************
// Method:    Make_Octree
// FullName:  ShearWarpRender::Make_Octree
// Access:    public 
// Returns:   int
// Qualifier:
// Purpose:   
//************************************
int ShearWarpRender::Make_Octree()
{
 	int retcode = SV_NORMAL;
	vpContext *vpc;	/* rendering context */
	FILE *volume_fd;
	FILE *octree_fd;

// 	/* create a context */
 	vpc = vpCreateContext();

	if ((volume_fd = fopen(VOLUME_FILE, READ_MODE)) == NULL) 
		retcode = SV_FILEIO_ERROR;
	else if (vpLoadRawVolume(vpc, volume_fd) != VP_OK) 
	{
		fprintf(stderr, "VolPack error: %s\n", vpGetErrorString(vpGetError(vpc)));
		fprintf(stderr, "could not load the aa volume from file %s\n", VOLUME_FILE);
		retcode = SV_FILEIO_ERROR;
	}
	fclose(volume_fd);
	
	if(!retcode)
	{
		/* compute the octree */
		vpSetClassifierTable(vpc, DENSITY_PARAM, DENSITY_FIELD, NULL, 0);
		vpSetClassifierTable(vpc, GRADIENT_PARAM, GRADIENT_FIELD, NULL, 0);
		vpMinMaxOctreeThreshold(vpc, DENSITY_PARAM, OCTREE_DENSITY_THRESH);
		vpMinMaxOctreeThreshold(vpc, GRADIENT_PARAM, OCTREE_GRADIENT_THRESH);
		if (vpCreateMinMaxOctree(vpc, 0, OCTREE_BASE_NODE_SIZE) != VP_OK) {
			fprintf(stderr, "VolPack error: %s\n",vpGetErrorString(vpGetError(vpc)));
			retcode = SV_VOLPACK_ERR;
		}
	}

	if(!retcode)
	{
		/*Store octree in a file*/
		if((octree_fd = fopen(OCTREE_FILE, WRITE_MODE)) == NULL)
			retcode = SV_FILEIO_ERROR;
		else if (vpStoreMinMaxOctree(vpc, octree_fd) != VP_OK) {
			fprintf(stderr, "VolPack error: %s\n",	vpGetErrorString(vpGetError(vpc)));
			retcode = SV_VOLPACK_ERR;
		}
		fclose(octree_fd);
	}	
	vpDestroyContext(vpc);	
	return retcode;
}

//************************************
// Method:    Classify
// FullName:  ShearWarpRender::Classify
// Access:    public 
// Returns:   int
// Qualifier:
// Purpose:   
//************************************
int ShearWarpRender::Classify()
{
	int retcode = SV_NORMAL;

	vpContext *vpc;	/* rendering context */
	FILE *volume_fd;	/* file descriptor for volume (input) */
	FILE *octree_fd;	/* file descriptor for octree (input) */
	FILE *output_fd;	/* file descriptor for classified volume (output) */

	int use_rawdata;	/* if true, use raw data instead of volume */
	int use_octree;	/* if true, use octree with the volume */
	unsigned density_size;/* size of density data */
	float density_ramp[DENSITY_MAX+1];	/* opacity as a function of density */
	float gradient_ramp[GRADIENT_MAX+1];/* opacity as a function of gradient magnitude */

	/* check command-line arguments */
	use_octree = m_UseOctree;
	use_rawdata = m_UseRawData;


	/* create a context */
	vpc = vpCreateContext();
	density_size = m_Xlen * m_YLen * m_ZLen;

	/* load input data: either raw data, or an unclassified volume with no
	octree, or an unclassified volume with an octree */
	if (use_rawdata) {
		/* describe the layout of the volume */
		vpSetVolumeSize(vpc, m_Xlen, m_YLen, m_ZLen);
		vpSetVoxelSize(vpc, BYTES_PER_VOXEL, VOXEL_FIELDS, SHADE_FIELDS, CLSFY_FIELDS);
		vpSetVoxelField(vpc, NORMAL_FIELD, NORMAL_SIZE, NORMAL_OFFSET, NORMAL_MAX);
		vpSetVoxelField(vpc, DENSITY_FIELD, DENSITY_SIZE, DENSITY_OFFSET, DENSITY_MAX);
		vpSetVoxelField(vpc, GRADIENT_FIELD, GRADIENT_SIZE, GRADIENT_OFFSET, GRADIENT_MAX);

	} else {
		if ((volume_fd = fopen(VOLUME_FILE, READ_MODE)) == NULL) 
			retcode = SV_FILEIO_ERROR;
		else if (vpLoadRawVolume(vpc, volume_fd) != VP_OK) {
			fprintf(stderr, "VolPack error: %s\n",vpGetErrorString(vpGetError(vpc)));
			fprintf(stderr, "could not load the volume from file %s\n", VOLUME_FILE);
			retcode = SV_VOLPACK_ERR;
		}
		fclose(volume_fd);
	}

	
	if (!retcode)
	{
		/* set the classification function */
		vpRamp(density_ramp, sizeof(float), DENSITY_RAMP_POINTS, DensityRampX, DensityRampY);
		vpSetClassifierTable(vpc, DENSITY_PARAM, DENSITY_FIELD, density_ramp, sizeof(density_ramp));
		vpRamp(gradient_ramp, sizeof(float), GRADIENT_RAMP_POINTS, GradientRampX, GradientRampY);
		vpSetClassifierTable(vpc, GRADIENT_PARAM, GRADIENT_FIELD, gradient_ramp, sizeof(gradient_ramp));
		vpSetd(vpc, VP_MIN_VOXEL_OPACITY, 0.05);
	

		/* load the octree */
		if (use_octree) {
			/* load the octree */
			if ((octree_fd = fopen(OCTREE_FILE, READ_MODE)) == NULL) retcode = SV_FILEIO_ERROR;
			else if (vpLoadMinMaxOctree(vpc, octree_fd) != VP_OK) {
				fprintf(stderr, "VolPack error: %s\n",vpGetErrorString(vpGetError(vpc)));
				fprintf(stderr, "could not load the octree from file %s\n", OCTREE_FILE);
				retcode = SV_VOLPACK_ERR;
			}
			fclose(octree_fd);
		}
	}

	if (!retcode)
	{
		/* classify */
		if (use_rawdata) {
			if (vpClassifyScalars(vpc, m_pRawData, density_size, DENSITY_FIELD, GRADIENT_FIELD, NORMAL_FIELD) != VP_OK) {
				fprintf(stderr, "VolPack error: %s\n", vpGetErrorString(vpGetError(vpc)));
				retcode = SV_VOLPACK_ERR;
			}
		} else {
			if (vpClassifyVolume(vpc) != VP_OK) {
				fprintf(stderr, "VolPack error: %s\n", vpGetErrorString(vpGetError(vpc)));
				retcode = SV_VOLPACK_ERR;
			}
		}
	}

	if (!retcode)
	{
		/* store the classified volume */
		if ((output_fd = fopen(CLVOLUME_FILE, WRITE_MODE)) == NULL) retcode = SV_FILEIO_ERROR;
		else if (vpStoreClassifiedVolume(vpc, output_fd) != VP_OK) {
			fprintf(stderr, "VolPack error: %s\n",	vpGetErrorString(vpGetError(vpc)));
			retcode = SV_VOLPACK_ERR;
		}
		fclose(output_fd);
	}
	vpDestroyContext(vpc);
	return retcode;
}

//************************************
// Method:    Render
// FullName:  ShearWarpRender::Render
// Access:    public 
// Returns:   int
// Qualifier:
// Purpose:   
//************************************
int ShearWarpRender::Render( CViewSliceObj* outSlice, int nWidth, int nHeight ,DOUBLE nRotateX, DOUBLE nRotateY, DOUBLE nRotateZ )
{
	int retcode = SV_NORMAL;
	UCHAR* pImage = NULL;
	vpContext* vpc = (vpContext*)m_vpcRender;

	/* set the initial viewing parameters */
	vpSeti(vpc, VP_CONCAT_MODE, VP_CONCAT_LEFT);
	vpRotate(vpc, VP_X_AXIS, nRotateX);
	vpRotate(vpc, VP_Y_AXIS, nRotateY);	
	vpRotate(vpc, VP_X_AXIS, nRotateZ);
	vpCurrentMatrix(vpc, VP_PROJECT);
	vpIdentityMatrix(vpc);
	vpWindow(vpc, VP_PARALLEL, -0.5, 0.5, -0.5, 0.5, -0.5, 0.5);
	vpCurrentMatrix(vpc, VP_MODEL);

	// set the image buffer 
	pImage = new UCHAR[nWidth* nHeight];
	vpSetImage(vpc, pImage, nWidth, nHeight, nWidth, VP_LUMINANCE);
	
	vpSetd(vpc, VP_MAX_RAY_OPACITY, 0.95);

	/* compute shading lookup table */
	if (vpShadeTable(vpc) != VP_OK){
		fprintf(stderr, "VolPack error: %s\n", vpGetErrorString(vpGetError(vpc)));
		retcode = SV_VOLPACK_ERR;
	}

	BOOL use_clvolume = m_UseClassified;

	//render 
	if (use_clvolume) 
	{
		if (vpRenderClassifiedVolume(vpc) != VP_OK) 
		{
			fprintf(stderr, "VolPack error: %s\n", vpGetErrorString(vpGetError(vpc)));
			retcode = SV_VOLPACK_ERR;
		}
	} else 
	{
		if (vpRenderRawVolume(vpc) != VP_OK)
		{
			char* err = vpGetErrorString(vpGetError(vpc));
			fprintf(stderr, "VolPack error: %s\n", vpGetErrorString(vpGetError(vpc)));
			retcode = SV_VOLPACK_ERR;
		}
	} 
	
	
//	outSlice->PrepareMem(CSize(nWidth, nHeight));
	UCHAR* pOutImg = outSlice->GetProcessedBuff();
	ASSERT(pOutImg != NULL);
	
	// Row padding
	INT rowPadder = (sizeof(DWORD) -(nWidth * 3) % sizeof(DWORD)) % sizeof(DWORD);

/*
	for (int i =0; i<nWidth * nHeight; i++)
	{
		*(pOutImg + 3*i) = *(pImage + i);
		*(pOutImg + 3*i + 1) = *(pImage + i);
		*(pOutImg + 3*i + 2) = *(pImage + i);
	}
*/

	for (int row=0; row < nHeight; row++)
	for (int col=0; col < nWidth; col++)
	{
		pOutImg[(row*nWidth + col)*3 + rowPadder*row] = pImage[row*nWidth + col];
		pOutImg[(row*nWidth + col)*3 + rowPadder*row +1] = pImage[row*nWidth + col];
		pOutImg[(row*nWidth + col)*3 + rowPadder*row +2] = pImage[row*nWidth + col];
	}

	delete[] pImage;
	//vpDestroyContext(vpc);
	return retcode;
}

//************************************
// Method:    SetRawSource
// FullName:  ShearWarpRender::SetRawSource
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: CImageDataSet * in_pDataSource
// Purpose:   
//************************************
void ShearWarpRender::SetRawSource( CImageDataSet* in_pDataSource )
{
	m_Xlen = in_pDataSource->m_tSize.ndx;	
	m_YLen = in_pDataSource->m_tSize.ndy;
	m_ZLen = in_pDataSource->m_tSize.ndz;
	m_pRawData = in_pDataSource->GetDataBuff();
}



//************************************
// Method:    PrepareToRender
// FullName:  ShearWarpRender::PrepareToRender
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: CSliceObj * outSlice
// Parameter: int nWidth
// Parameter: int nHeight
// Parameter: int nRotateX
// Parameter: int nRotateY
// Parameter: int nRotateZ
// Purpose:   
//************************************
int ShearWarpRender::SetRenderParam()
{
	int retcode = SV_NORMAL;
	vpContext *vpc;									/* rendering context */
	FILE *volume_fd;	
	FILE *octree_fd;	
	FILE *clvolume_fd;	
	int use_octree;									/* if true, use octree with the unclassified volume */
	int use_clvolume;								/* if true, use the classified volume */
	float density_ramp[DENSITY_MAX+1];				/* opacity as a function of density */
	float gradient_ramp[GRADIENT_MAX+1];			/* opacity as a function of gradient magnitude */
	float shade_table[NORMAL_MAX+1];				/* shading lookup table */
	int n;

	/* check command-line arguments */
	use_octree = m_UseOctree;
	use_clvolume = m_UseClassified;


	/* create a context */
	m_vpcRender = vpCreateContext();
	vpc = (vpContext*)m_vpcRender;	

	/* load input data: either a classified volume, or an unclassified
	volume with no octree, or an unclassified volume with an octree */
	if (use_clvolume)
	{
		if ((clvolume_fd = fopen(CLVOLUME_FILE, READ_MODE)) == NULL) retcode = SV_FILEIO_ERROR;
		/* load the classified volume data */
		else if (vpLoadClassifiedVolume(vpc, clvolume_fd) != VP_OK)
		{
			fprintf(stderr, "VolPack error: %s\n", vpGetErrorString(vpGetError(vpc)));
			fprintf(stderr, "could not load the volume from file %s\n", CLVOLUME_FILE);
			retcode = SV_VOLPACK_ERR;
		}
		fclose(clvolume_fd);
	} else {
		/* load the unclassified volume data */
		if ((volume_fd = fopen(VOLUME_FILE, READ_MODE)) < 0) retcode = SV_FILEIO_ERROR;
		else if (vpLoadRawVolume(vpc, volume_fd) != VP_OK)
		{
			fprintf(stderr, "VolPack error: %s\n", vpGetErrorString(vpGetError(vpc)));
			fprintf(stderr, "could not load the volume from file %s\n", VOLUME_FILE);
			retcode = SV_VOLPACK_ERR;
		}
		fclose(volume_fd);

		/* set the classification function */
		vpRamp(density_ramp, sizeof(float), DENSITY_RAMP_POINTS, DensityRampX, DensityRampY);
		vpSetClassifierTable(vpc, DENSITY_PARAM, DENSITY_FIELD, density_ramp, sizeof(density_ramp));
		vpRamp(gradient_ramp, sizeof(float), GRADIENT_RAMP_POINTS, GradientRampX, GradientRampY);
		vpSetClassifierTable(vpc, GRADIENT_PARAM, GRADIENT_FIELD, gradient_ramp, sizeof(gradient_ramp));
		vpSetd(vpc, VP_MIN_VOXEL_OPACITY, 0.05);

		if (use_octree)
		{
			/* load the octree */
			if ((octree_fd = fopen(OCTREE_FILE, READ_MODE)) < 0) retcode = SV_FILEIO_ERROR;
			else if (vpLoadMinMaxOctree(vpc, octree_fd) != VP_OK) 
			{
				fprintf(stderr, "VolPack error: %s\n", vpGetErrorString(vpGetError(vpc)));
				fprintf(stderr, "could not load the octree from file %s\n",	OCTREE_FILE);
				retcode = SV_VOLPACK_ERR;
			}
			fclose(octree_fd);
		}
	}

	if(!retcode)
	{
		/* set the shading parameters */
		vpSetLookupShader(vpc, 1, 1, NORMAL_FIELD, shade_table, sizeof(shade_table), 0, NULL, 0);
		vpSetMaterial(vpc, VP_MATERIAL0, VP_AMBIENT, VP_BOTH_SIDES, 0.18, 0.18, 0.18);
		vpSetMaterial(vpc, VP_MATERIAL0, VP_DIFFUSE, VP_BOTH_SIDES, 0.35, 0.35, 0.35);
		vpSetMaterial(vpc, VP_MATERIAL0, VP_SPECULAR, VP_BOTH_SIDES, 0.39, 0.39, 0.39);
		vpSetMaterial(vpc, VP_MATERIAL0, VP_SHINYNESS, VP_BOTH_SIDES,10.0,0.0,0.0);
		vpSetLight(vpc, VP_LIGHT0, VP_DIRECTION, 0.3, 0.3, 1.0);
		vpSetLight(vpc, VP_LIGHT0, VP_COLOR, 1.0, 1.0, 1.0);
		vpEnable(vpc, VP_LIGHT0, 1);
		vpSetDepthCueing(vpc, 1.4, 1.5);
		vpEnable(vpc, VP_DEPTH_CUE, 1);
	}
	return retcode;
}

//************************************
// Method:    InitRender
// FullName:  ShearWarpRender::InitRender
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: CImageDataSet * in_pDataSource
// Parameter: BOOL flgUseOctree
// Parameter: BOOL flgUseClassifiedVol
// Purpose:   
//************************************
int ShearWarpRender::InitRender( CImageDataSet* in_pDataSource, BOOL flgUseRawData, BOOL flgUseOctree,BOOL flgUseClassifiedVol )
{
	ASSERT(in_pDataSource != NULL);
	

	if ((flgUseRawData && flgUseOctree) || (flgUseRawData && (!flgUseClassifiedVol)))
		return SV_INVALID_PARAM;

	int retcode = SV_NORMAL;
	SetRawSource(in_pDataSource);
	m_UseRawData = flgUseRawData;
	m_UseClassified = flgUseClassifiedVol;
	m_UseOctree = flgUseOctree;
	
 	if (!m_UseRawData)	 retcode = Make_Volume();
 	if (m_UseOctree && (!retcode)) retcode = Make_Octree(); // DUMP [3/5/2009 QUYPS]
 	if (m_UseClassified && (!retcode)) retcode = Classify(); // DUMP [3/5/2009 QUYPS]
 	if (!retcode) retcode = SetRenderParam();

	return retcode;
}
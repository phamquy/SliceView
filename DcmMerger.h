#pragma once

#include "PatientInfo.h"
#include "ImageDataSet.h"

#include "dcmtk/dcmdata/dctk.h"			/* for various dcmdata headers */
#include "dcmtk/dcmdata/dcdebug.h"		/* for SetDebugLevel */
#include "dcmtk/dcmdata/cmdlnarg.h"		/* for prepareCmdLineArgs */
#include "dcmtk/dcmdata/dcuid.h"		/* for dcmtk version name */
#include "dcmtk/dcmdata/dcrledrg.h"		/* for DcmRLEDecoderRegistration */
#include "dcmtk/dcmimgle/dcmimage.h"	/* for DicomImage */
#include "dcmtk/dcmimgle/digsdfn.h"		/* for DiGSDFunction */
#include "dcmtk/dcmimgle/diciefn.h"		/* for DiCIELABFunction */
#include "dcmtk/ofstd/ofconapp.h"		/* for OFConsoleApplication */
#include "dcmtk/ofstd/ofcmdln.h"		/* for OFCommandLine */
#include "dcmtk/dcmimage/diregist.h"	/* include to support color images */
#include "dcmtk/ofstd/ofstd.h"			/* for OFStandard */


typedef CTypedPtrList<CPtrList, DcmFileFormat*> DcmFileList;
typedef CTypedPtrList<CPtrList, DicomImage*> DcmImageList;

class CDcmMerger
{
public:
	CDcmMerger(void);
	virtual ~CDcmMerger(void);

public:
	DcmFileList m_FileList;
	DcmImageList m_ImageList;

//	CStringList m_fileList;
	virtual int ReadPatientInfo(CPatientInfo* out_pPatInfo);
	virtual int ReadImageDataSet(CImageDataSet* out_pImgDataSet);
	virtual int Open(CStringList* in_pFileList);
	virtual int Close(void);
};

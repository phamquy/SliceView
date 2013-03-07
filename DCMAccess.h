#pragma once
#include "PatientInfo.h"
#include "ImageDataSet.h"
#include "afx.h"
#include "FileAccess.h"

//Include DCMTK
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

class CDCMAccess: public CFileAccess
{
public:
	CDCMAccess(void);
	~CDCMAccess(void);
	// Read patient information from current opened DCM file
	virtual int ReadPatientInfo(CPatientInfo* out_pPatInfo);
	virtual int ReadImageDataSet(CImageDataSet* out_pImgDataSet);
	virtual int Open(CString in_szFilePath);
	virtual int Close(void);
protected:
	//WCHAR* m_szFilePath;
	DicomImage* m_pDCMImage;	
	DcmFileFormat* m_pDcmFile;
};

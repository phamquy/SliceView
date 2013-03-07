/*
 * volume.h
 *
 * Global definitions for VolPack example programs.
 *
 * Copyright (c) 1994 The Board of Trustees of The Leland Stanford
 * Junior University.  All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice and this permission notice appear in
 * all copies of this software and that you do not sell the software.
 * Commercial licensing is available by contacting the author.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author:
 *    Phil Lacroute
 *    Computer Systems Laboratory
 *    Electrical Engineering Dept.
 *    Stanford University
 */

/*
 * $Date: 1994/12/31 19:53:03 $
 * $Revision: 1.5 $
 */
#pragma once
#include <stdio.h>
#include "volpack.h"
#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include <sys/types.h>
#include "denfile.h"


#define WRITE_MODE "wb"
#define READ_MODE "rb"

typedef struct {		/* contents of a voxel */
    short normal;		/*   encoded surface normal vector */
    unsigned char density;	/*   original density */
    unsigned char gradient;	/*   original gradient */
} RawVoxel;

RawVoxel *dummy_voxel;

#define BYTES_PER_VOXEL	sizeof(RawVoxel)	/* voxel size in bytes */
#define VOXEL_FIELDS	3	/* number of fields in voxel */
#define SHADE_FIELDS	2	/* number of fields used for shading
				   (normal and density); must be the
				   1st fields of RawVoxel */
#define CLSFY_FIELDS	2	/* number of fields used for classifying
				   (density and gradient); can be any fields
				   in the RawVoxel */

#define NORMAL_FIELD	0
#define NORMAL_OFFSET	vpFieldOffset(dummy_voxel, normal)
#define NORMAL_SIZE	sizeof(short)
#define NORMAL_MAX	VP_NORM_MAX

#define DENSITY_FIELD	1
#define DENSITY_OFFSET	vpFieldOffset(dummy_voxel, density)
#define DENSITY_SIZE	sizeof(unsigned char)
#define DENSITY_MAX	255

#define GRADIENT_FIELD	2
#define GRADIENT_OFFSET	vpFieldOffset(dummy_voxel, gradient)
#define GRADIENT_SIZE	sizeof(unsigned char)
#define GRADIENT_MAX	VP_GRAD_MAX

#define DENSITY_PARAM		0		/* classification parameters */
#define OCTREE_DENSITY_THRESH	4
#define GRADIENT_PARAM		1
#define OCTREE_GRADIENT_THRESH	4
#define OCTREE_BASE_NODE_SIZE	4

#define DENSITY_RAMP_POINTS 3			/* classification ramps */
int DensityRampX[] =    {  0,  20, 255};
float DensityRampY[] =  {0.0, 1.0, 1.0};

#define GRADIENT_RAMP_POINTS 4
int GradientRampX[] =   {  0,   5,  20, 221};
float GradientRampY[] = {0.0, 0.0, 1.0, 1.0};
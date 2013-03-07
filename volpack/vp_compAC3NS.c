/*
 * vp_compA.m4
 *
 * Compositing routine for affine viewing transformations.
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
 * $Date: 1994/12/30 23:52:38 $
 * $Revision: 1.7 $
 */

#include "vp_global.h"



    
    
    
    
    

    
    
    
#define USE_SHADOW_BUFFER
	    
    
    


#undef UNROLL_RUN_LOOP




















#define MULTIPLE_MATERIALS































#define RGB
    
    
    
    



#define RLEVOLUME
	
	
	
	
	
	
	




#undef INDEX_VOLUME


    
    








/* codes indicating the types of a pair of runs in adjacent scanlines */
#define ALL_ZERO	0	/* both runs are runs of zeros */
#define TOP_NONZERO	1	/* run for top scanline has nonzero data */
#define BOT_NONZERO	2	/* run for bottom scanline has nonzero data */
#define ALL_NONZERO	3	/* both runs have nonzero data */

/* codes indicating the types for the current left and right voxel pairs */
#define ALL_ZERO__ALL_ZERO		((ALL_ZERO << 2) | ALL_ZERO)
#define ALL_ZERO__TOP_NONZERO		((ALL_ZERO << 2) | TOP_NONZERO)
#define ALL_ZERO__BOT_NONZERO		((ALL_ZERO << 2) | BOT_NONZERO)
#define ALL_ZERO__ALL_NONZERO		((ALL_ZERO << 2) | ALL_NONZERO)
#define TOP_NONZERO__ALL_ZERO		((TOP_NONZERO << 2) | ALL_ZERO)
#define TOP_NONZERO__TOP_NONZERO	((TOP_NONZERO << 2) | TOP_NONZERO)
#define TOP_NONZERO__BOT_NONZERO	((TOP_NONZERO << 2) | BOT_NONZERO)
#define TOP_NONZERO__ALL_NONZERO	((TOP_NONZERO << 2) | ALL_NONZERO)
#define BOT_NONZERO__ALL_ZERO		((BOT_NONZERO << 2) | ALL_ZERO)
#define BOT_NONZERO__TOP_NONZERO	((BOT_NONZERO << 2) | TOP_NONZERO)
#define BOT_NONZERO__BOT_NONZERO	((BOT_NONZERO << 2) | BOT_NONZERO)
#define BOT_NONZERO__ALL_NONZERO	((BOT_NONZERO << 2) | ALL_NONZERO)
#define ALL_NONZERO__ALL_ZERO		((ALL_NONZERO << 2) | ALL_ZERO)
#define ALL_NONZERO__TOP_NONZERO	((ALL_NONZERO << 2) | TOP_NONZERO)
#define ALL_NONZERO__BOT_NONZERO	((ALL_NONZERO << 2) | BOT_NONZERO)
#define ALL_NONZERO__ALL_NONZERO	((ALL_NONZERO << 2) | ALL_NONZERO)

#ifdef SKIP_ERT
#define PIXEL_IS_OPAQUE(ipixel)	0
#else
#define PIXEL_IS_OPAQUE(ipixel)	((ipixel)->lnk != 0)
#endif

#ifdef STATISTICS
extern int vpResampleCount;
extern int vpCompositeCount;
extern int vpERTSkipCount;
extern int vpERTSkipAgainCount;
extern int vpERTUpdateCount;
extern int vpSpecialZeroSkipCount;
extern int vpRunFragmentCount;
#define COUNT_RESAMPLE		vpResampleCount++
#define COUNT_COMPOSITE		vpCompositeCount++
#define COUNT_ERT_SKIP		vpERTSkipCount++
#define COUNT_ERT_SKIP_AGAIN	vpERTSkipAgainCount++
#define COUNT_ERT_UPDATE	vpERTUpdateCount++
#define COUNT_SPECIAL_ZERO_SKIP	vpSpecialZeroSkipCount++
#define COUNT_RUN_FRAGMENT	vpRunFragmentCount++
#else
#define COUNT_RESAMPLE
#define COUNT_COMPOSITE
#define COUNT_ERT_SKIP
#define COUNT_ERT_SKIP_AGAIN
#define COUNT_ERT_UPDATE
#define COUNT_SPECIAL_ZERO_SKIP
#define COUNT_RUN_FRAGMENT
#endif /* STATISTICS */

/*
 * VPCompAC3NS
 *
 * Compositing routine for run-length encoded volume data slices.
 * Decode and resample one slice of volume data, and composite
 * it into the intermediate image.  The resampling filter is a bilirp.
 */

void
VPCompAC3NS (vpc, icount, jcount, k, slice_depth_cueing_dbl, intimage,
	  weightTLdbl, weightBLdbl, weightTRdbl, weightBRdbl,
	  run_lengths, voxel_data , shadow_buffer)
vpContext *vpc;			/* context */
int icount;			/* slice size */
int jcount;
int k;				/* slice number */
double slice_depth_cueing_dbl;	/* depth cueing factor for slice */
RGBIntPixel *intimage;		/* intermediate image pixels */
double weightTLdbl;		/* resampling weights */
double weightBLdbl;
double weightTRdbl;
double weightBRdbl;

		unsigned char *run_lengths;	/* run lengths for slice */
		void *voxel_data;		/* voxel data for slice */
GrayIntPixel *shadow_buffer;
{
    int i, j;			/* voxel index in rotated object space */
    RGBIntPixel *ipixel;	/* current intermediate image pixel */
    RGBIntPixel *ipixel2;	/* another intermediate image pixel */
    int update_interval;	/* # of pixels to skip when updating links */
    float iopc;			/* intermediate pixel opacity (0-1) */
    float iopc_inv;		/* 1-iopc */
    float acc_opc;		/* accumulator for resampled voxel opacity */
    float top_opc, bot_opc;	/* voxel opacity (top and bottom scanlines) */
#ifdef NO_REUSE_VOXEL
#define voxels_loaded	0
#define CLEAR_VOXELS_LOADED
#define SET_VOXELS_LOADED
#else
    int voxels_loaded;		/* if true, top/bot_opc contain valid
				   data loaded during the last resample */
#define CLEAR_VOXELS_LOADED	voxels_loaded = 0
#define SET_VOXELS_LOADED	voxels_loaded = 1
#endif
    float wgtTL, wgtBL,		/* weights in the range 0..1 which give the */
	  wgtTR, wgtBR;		/*   fractional contribution of the */
    				/*   neighboring voxels to the current */
    			        /*   intermediate image pixel */
    unsigned char *topRLElen;	/* length of current run in top scanline */
    unsigned char *botRLElen;	/* length of current run in bottom scanline */
    char *topRLEdata;		/* data for current run in top scanline */
    char *botRLEdata;		/* data for current run in bottom scanline */
    int toprun_count;		/* number of voxels left in top run */
    int botrun_count;		/* number of voxels left in bottom run */
    int last_run_state;		/* run state code for last resample */
    int run_state;		/* run state code for this resample */
    int final_run_state;	/* run state code for end of scanline */
    float min_opacity;		/* low opacity threshold */
    float max_opacity;		/* high opacity threshold */
    float slice_depth_cueing;	/* depth cueing factor for slice */
    float *opac_correct;	/* opacity correction table */
    int ert_skip_count;		/* number of pixels to skip for ERT */
    int intermediate_width;	/* width of intermediate image in pixels */
    int count;			/* voxels left in current run */
    float *shade_table;		/* shade lookup table */
    int norm_offset;		/* byte offset to shade table index in voxel */
    int shade_index;		/* shade index */
    float shade_factor;		/* attenuation factor for color
				   (voxel opacity * depth cueing) */

#ifdef MULTIPLE_MATERIALS
    float *weight_table;	/* weight lookup table */
    int wgt_offset;		/* byte offset to weight table index */
    int weight_index;		/* weight index */
    int m, num_materials;
    float weight1, weight2;
#endif /* MULTIPLE_MATERIALS */

#ifdef GRAYSCALE
    float acc_clr;		/* accumulator for resampled color */
    float top_clr, bot_clr;	/* voxel color (top and bottom scanlines) */
#endif /* GRAYSCALE */

#ifdef RGB
    float acc_rclr;		/* accumulator for resampled color */
    float acc_gclr;
    float acc_bclr;
    float top_rclr;		/* voxel color (top and bottom scanlines) */
    float bot_rclr;
    float top_gclr;
    float bot_gclr;
    float top_bclr;
    float bot_bclr;
#endif

#ifdef RLEVOLUME
    int voxel_istride;		/* size of a voxel in bytes */
#endif

#ifdef RAWVOLUME
    int use_octree;		/* if true then use the min-max octree */
    MMOctreeLevel level_stack[VP_MAX_OCTREE_LEVELS];
				/* stack for traversal of min-max octree */
    int scans_left;		/* scanlines until next octree traversal */
    int best_view_axis;		/* viewing axis */
    unsigned char runlen_buf1[VP_MAX_VOLUME_DIM]; /* buffers for run lengths */
    unsigned char runlen_buf2[VP_MAX_VOLUME_DIM];
    unsigned char *top_len_base;/* first run length for top scanline */
    unsigned char *bot_len_base;/* first run length for bottom scanline */
    int opac_param;		/* parameter to opacity transfer function */
    float opacity;		/* voxel opacity */
    int opacity_int;		/* voxel opacity truncated to an integer */
    int param0_offset;		/* offset to first parameter in voxel */
    int param0_size;		/* size of first parameter in bytes */
    float *param0_table;	/* lookup table for first parameter */
    int param1_offset;		/* offset to second parameter in voxel */
    int param1_size;		/* size of second parameter in bytes */
    float *param1_table;	/* lookup table for second parameter */
    int param2_offset;		/* offset to third parameter in voxel */
    int param2_size;		/* size of third parameter in bytes */
    float *param2_table;	/* lookup table for third parameter */
#endif /* RAWVOLUME */

#ifdef INDEX_VOLUME
    unsigned char *scanline_topRLElen; /* first topRLElen in scanline */
    unsigned char *scanline_botRLElen; /* first botRLElen in scanline */
    char *scanline_topRLEdata;	/* first topRLEdata in scanline */
    char *scanline_botRLEdata;	/* first botRLEdata in scanline */
    VoxelLocation *top_voxel_index; /* voxel indexes for top scanline */
    VoxelLocation *bot_voxel_index; /* voxel indexes for bot scanline */
    VoxelLocation *vindex;
    int next_i;			/* i coordinate of voxel to skip to */
    int next_scan;		/* true if skipped to next scanline */
#endif /* INDEX_VOLUME */

#ifdef CALLBACK
				/* shading callback function */
#ifdef GRAYSCALE
    void (*shade_func) ANSI_ARGS((void *, float *, void *));
#endif
#ifdef RGB
    void (*shade_func) ANSI_ARGS((void *, float *, float *, float *, void *));
#endif
    void *client_data;		/* client data handle */
#endif /* CALLBACK */

#ifdef USE_SHADOW_BUFFER
    float *shadow_table;	/* color lookup table for shadows */
    int shadow_width;		/* width of shadow buffer */
    GrayIntPixel *shadow_pixel; /* current shadow buffer pixel */
#ifdef GRAYSCALE
    float top_sclr, bot_sclr;	/* shadow color (top and bottom scanlines) */
#endif /* GRAYSCALE */
#ifdef RGB
    float top_rsclr;		/* shadow color (top and bottom scanlines) */
    float bot_rsclr;
    float top_gsclr;
    float bot_gsclr;
    float top_bsclr;
    float bot_bsclr;
#endif
#endif /* SHADOW_BUFFER */

#ifdef DEBUG
    float trace_opcTL, trace_opcBL, trace_opcTR, trace_opcBR;
    float trace_rsclrTL, trace_rsclrBL, trace_rsclrTR, trace_rsclrBR;
    float trace_rclrTL, trace_rclrBL, trace_rclrTR, trace_rclrBR;
    float trace_gclrTL, trace_gclrBL, trace_gclrTR, trace_gclrBR;
    float trace_bclrTL, trace_bclrBL, trace_bclrTR, trace_bclrBR;
    RGBIntPixel *trace_pixel_ptr;

#ifdef COMPUTE_SHADOW_BUFFER
    int slice_u_int, shadow_slice_u_int;
    int slice_v_int, shadow_slice_v_int;
#endif
#endif /* DEBUG */

    DECLARE_HIRES_TIME(t0);
    DECLARE_HIRES_TIME(t1);

    /*******************************************************************
     * Copy parameters from the rendering context into local variables.
     *******************************************************************/

    GET_HIRES_TIME(vpc, t0);

    wgtTL = weightTLdbl;
    wgtBL = weightBLdbl;
    wgtTR = weightTRdbl;
    wgtBR = weightBRdbl;
    slice_depth_cueing = slice_depth_cueing_dbl;
    min_opacity = vpc->min_opacity;
    max_opacity = vpc->max_opacity;
#ifdef USE_SHADOW_BUFFER
    opac_correct = vpc->shadow_opac_correct;
#else
    opac_correct = vpc->affine_opac_correct;
#endif
#ifdef COMPUTE_SHADOW_BUFFER
    intermediate_width = vpc->shadow_width;
#else
    intermediate_width = vpc->intermediate_width;
#endif
#ifdef USE_SHADOW_BUFFER
    shadow_table = vpc->shadow_color_table;
    shadow_width = vpc->shadow_width;
    shadow_pixel = shadow_buffer;
#endif
    ipixel = intimage;
    shade_table = vpc->shade_color_table;
    norm_offset = vpc->field_offset[vpc->color_field];

#ifdef MULTIPLE_MATERIALS
    weight_table = vpc->shade_weight_table;
    wgt_offset = vpc->field_offset[vpc->weight_field];
    num_materials = vpc->num_materials;
#endif /* MULTIPLE_MATERIALS */

#ifdef RLEVOLUME
    topRLEdata = voxel_data;
    botRLEdata = voxel_data;
    topRLElen = run_lengths;
    botRLElen = run_lengths;
    voxel_istride = vpc->rle_bytes_per_voxel;
#endif /* RLEVOLUME */

#ifdef RAWVOLUME
    ASSERT(vpc->num_clsfy_params > 0);
    ASSERT(vpc->num_clsfy_params < 3);
    param0_offset = vpc->field_offset[vpc->param_field[0]];
    param0_size = vpc->field_size[vpc->param_field[0]];
    param0_table = vpc->clsfy_table[0];
    if (vpc->num_clsfy_params > 1) {
	param1_offset = vpc->field_offset[vpc->param_field[1]];
	param1_size = vpc->field_size[vpc->param_field[1]];
	param1_table = vpc->clsfy_table[1];
    } else {
	param1_offset = 0;
	param1_size = 0;
	param1_table = NULL;
    }
    if (vpc->num_clsfy_params > 2) {
	param2_offset = vpc->field_offset[vpc->param_field[2]];
	param2_size = vpc->field_size[vpc->param_field[2]];
	param2_table = vpc->clsfy_table[2];
    } else {
	param2_offset = 0;
	param2_size = 0;
	param2_table = NULL;
    }
    if (vpc->mm_octree == NULL) {
	use_octree = 0;
    } else {
	use_octree = 1;
	best_view_axis = vpc->best_view_axis;
	VPInitOctreeLevelStack(vpc, level_stack, best_view_axis, k);
	scans_left = 0;
	bot_len_base = runlen_buf1;
    }
#endif /* RAWVOLUME */

#ifdef CALLBACK
    shade_func = vpc->shade_func;
    client_data = vpc->client_data;
    ASSERT(shade_func != NULL);
#endif

#ifdef DEBUG
    trace_pixel_ptr = 0;
    if (vpc->trace_u >= 0 && vpc->trace_v >= 0) {
#ifdef GRAYSCALE
	trace_pixel_ptr = &vpc->int_image.gray_intim[vpc->trace_u + 
		      vpc->trace_v*vpc->intermediate_width];
#endif
#ifdef RGB
	trace_pixel_ptr = &vpc->int_image.rgb_intim[vpc->trace_u + 
		      vpc->trace_v*vpc->intermediate_width];
#endif
#ifdef COMPUTE_SHADOW_BUFFER
	slice_u_int = (int)ceil(vpc->shear_i * vpc->trace_shadow_k +
				vpc->trans_i) - 1;
	shadow_slice_u_int = (int)ceil(vpc->shadow_shear_i * 
			     vpc->trace_shadow_k + vpc->shadow_trans_i) - 1;
	slice_v_int = (int)ceil(vpc->shear_j * vpc->trace_shadow_k
				+ vpc->trans_j) - 1;
	shadow_slice_v_int = (int)ceil(vpc->shadow_shear_j *
			     vpc->trace_shadow_k + vpc->shadow_trans_j) - 1;
	trace_pixel_ptr = &vpc->shadow_buffer[vpc->trace_u +
		shadow_slice_u_int - slice_u_int + 
		(vpc->trace_v + shadow_slice_v_int -
		slice_v_int)*vpc->shadow_width];
#endif
    }
#endif /* DEBUG */

    /*******************************************************************
     * Loop over voxel scanlines.
     *******************************************************************/

    for (j = 0; j <= jcount; j++) {

	/***************************************************************
	 * Initialize counters and flags.
	 ***************************************************************/

	i = icount;
	CLEAR_VOXELS_LOADED;
	last_run_state = ALL_ZERO;

#ifdef RAWVOLUME
	botRLEdata = (char *)voxel_data + j*voxel_jstride;
	topRLEdata = botRLEdata - voxel_jstride;
	if (!use_octree) {
	    if (j == 0) {
		run_state = BOT_NONZERO;
		toprun_count = icount+2;
		botrun_count = icount;
	    } else if (j == jcount) {
		run_state = TOP_NONZERO;
		toprun_count = icount;
		botrun_count = icount+2;
	    } else {
		run_state = ALL_NONZERO;
		toprun_count = icount;
		botrun_count = icount;
	    }
	} else
#endif /* RAWVOLUME */
	if (j == 0) {
	    run_state = BOT_NONZERO;
	    toprun_count = icount+2;
	    botrun_count = 0;
	} else if (j == jcount) {
	    run_state = TOP_NONZERO;
	    toprun_count = 0;
	    botrun_count = icount+2;
	} else {
	    run_state = ALL_NONZERO;
	    toprun_count = 0;
	    botrun_count = 0;
	}

#ifdef INDEX_VOLUME
	scanline_topRLElen = topRLElen;
	scanline_botRLElen = botRLElen;
	scanline_topRLEdata = topRLEdata;
	scanline_botRLEdata = botRLEdata;
	if (j == 0) {
	    top_voxel_index = voxel_index;
	    bot_voxel_index = voxel_index;
	} else {
	    top_voxel_index = bot_voxel_index;
	    bot_voxel_index += icount;
	}
#endif /* INDEX_VOLUME */

	/***************************************************************
	 * If the volume is not run-length encoded, use the min-max
	 * to find run lengths for the current voxel scanline.
	 ***************************************************************/

#ifdef RAWVOLUME
	if (use_octree) {
	    top_len_base = bot_len_base;
	    if (scans_left == 0) {
		if (bot_len_base == runlen_buf1)
		    bot_len_base = runlen_buf2;
		else
		    bot_len_base = runlen_buf1;

		GET_HIRES_TIME(vpc, t1);
		STORE_HIRES_TIME(vpc, VPTIMER_TRAVERSE_RUNS, t0, t1);
		COPY_HIRES_TIME(t0, t1);

		scans_left = VPComputeScanRuns(vpc, level_stack, bot_len_base,
					       best_view_axis, j, icount);

		GET_HIRES_TIME(vpc, t1);
		STORE_HIRES_TIME(vpc, VPTIMER_TRAVERSE_OCTREE, t0, t1);
		COPY_HIRES_TIME(t0, t1);
	    }
#ifdef DEBUG
	    if (j > 0)
		VPCheckRuns(vpc, top_len_base, best_view_axis, k, j-1);
	    if (j < jcount)
		VPCheckRuns(vpc, bot_len_base, best_view_axis, k, j);
#endif
	    scans_left--;
	    topRLElen = top_len_base;
	    botRLElen = bot_len_base;
	}
#endif /* RAWVOLUME */

	/***************************************************************
	 * Loop over runs in the voxel scanline.
	 ***************************************************************/

	Debug((vpc, VPDEBUG_COMPOSITE, "StartIScan(u=%d,v=%d)\n",
	       (((int)ipixel - (int)vpc->int_image.gray_intim) /
		sizeof(RGBIntPixel)) % vpc->intermediate_width,
	       (((int)ipixel - (int)vpc->int_image.gray_intim) /
		sizeof(RGBIntPixel)) / vpc->intermediate_width));

#ifdef UNROLL_RUN_LOOP
	while (i > 0) {
#else
	while (i >= 0) {
#endif
	    /***********************************************************
	     * Skip over any empty runs at beginning of scanline.
	     ***********************************************************/

	    if (last_run_state == ALL_ZERO) {
#ifndef UNROLL_RUN_LOOP
		if (i == 0) {
		    Debug((vpc, VPDEBUG_COMPOSITE, "ZeroSkip(1)End\n"));
		    ipixel += 1; shadow_pixel += 1;
		    final_run_state = ALL_ZERO;
		    i = -1;
		    break;	/* scanline is done */
		}
#endif

		/* check if this is the start of a new run */
		while (toprun_count == 0) {
		    toprun_count = *topRLElen++;
		    run_state ^= 1;
		}
		while (botrun_count == 0) {
		    botrun_count = *botRLElen++;
		    run_state ^= 2;
		}
		if (run_state == ALL_ZERO) {
		    COUNT_SPECIAL_ZERO_SKIP;

		    /* find the union of the two runs of voxels */
		    count = MIN(toprun_count, botrun_count);
		    toprun_count -= count;
		    botrun_count -= count;
		    ipixel += count; shadow_pixel += count;
		    ;
		    ;
		    i -= count;
		    ASSERT(i >= 0);
		    Debug((vpc, VPDEBUG_COMPOSITE, "ZeroSkip(%d)\n", count));
		    continue;
		}
	    }

#ifndef SKIP_ERT
	    /***********************************************************
	     * Skip over opaque pixels (early-ray termination).
	     ***********************************************************/

	    if ((ert_skip_count = ipixel->lnk) != 0) {

		GET_HIRES_TIME(vpc, t1);
		STORE_HIRES_TIME(vpc, VPTIMER_TRAVERSE_RUNS, t0, t1);
		COPY_HIRES_TIME(t0, t1);

		COUNT_ERT_SKIP;

#ifndef UNROLL_RUN_LOOP
		if (i == 0) {
		    ipixel += 1; shadow_pixel += 1;
		    final_run_state = last_run_state;
		    i = -1;
		    Debug((vpc, VPDEBUG_COMPOSITE, "ERTSkip(1)End\n"));
		    break;	/* scanline is done */
		}
#endif

		/* find out how many pixels to skip */
		if (ert_skip_count < i &&
				(count = ipixel[ert_skip_count].lnk) != 0) {
		    /* follow pointer chain */
		    do {
			COUNT_ERT_SKIP_AGAIN;
			ert_skip_count += count;
		    } while (ert_skip_count < i &&
				 (count = ipixel[ert_skip_count].lnk) != 0);

		    /* update some of the lnk pointers in the run of opaque
		       pixels; the more links we update the longer it will
		       take to perform the update, but we will potentially
		       save time in future slices by not having to follow
		       long pointer chains */
		    ipixel2 = ipixel;
		    update_interval = 1;
		    count = ert_skip_count - 1;
		    while (count > 0) {
			COUNT_ERT_UPDATE;
			ipixel2 += update_interval;
			if (count > 255)
			    ipixel2->lnk = 255;
			else
			    ipixel2->lnk = count;
			update_interval *= 2;
			count -= update_interval;
		    }

		    /* update the current link */
		    COUNT_ERT_UPDATE;
		    if (ert_skip_count > 255)
			ert_skip_count = 255;
		    ipixel->lnk = ert_skip_count;
		}

		/* skip over the opaque pixels */
		if (ert_skip_count > i)
		    ert_skip_count = i;
		Debug((vpc, VPDEBUG_COMPOSITE,"ERTSkip(%d)\n",ert_skip_count));
		ipixel += ert_skip_count; shadow_pixel += ert_skip_count;
		CLEAR_VOXELS_LOADED;

#ifdef INDEX_VOLUME
		/* compute i coordinate of voxel to skip to */
		next_i = icount - i + ert_skip_count;
		if (next_i == icount) {
		    next_i--;
		    next_scan = 1;
		} else {
		    next_scan = 0;
		}

		/* skip over voxels in top scanline */
		vindex = &top_voxel_index[next_i];
		toprun_count = vindex->run_count;
		topRLElen = scanline_topRLElen + vindex->len_offset;
		if (vindex->data_offset & INDEX_RUN_IS_ZERO) {
		    run_state &= ~1;
		    topRLEdata = scanline_topRLEdata +
			(vindex->data_offset & ~INDEX_RUN_IS_ZERO);
		} else {
		    run_state |= 1;
		    topRLEdata = scanline_topRLEdata + vindex->data_offset;
		}

		/* skip over voxels in bottom scanline */
		vindex = &bot_voxel_index[next_i];
		botrun_count = vindex->run_count;
		botRLElen = scanline_botRLElen + vindex->len_offset;
		if (vindex->data_offset & INDEX_RUN_IS_ZERO) {
		    run_state &= ~2;
		    botRLEdata = scanline_botRLEdata +
			(vindex->data_offset & ~INDEX_RUN_IS_ZERO);
		} else {
		    run_state |= 2;
		    botRLEdata = scanline_botRLEdata + vindex->data_offset;
		}

		/* special case to skip over last voxel in scanline */
		if (next_scan) {
		    /* advance to beginning of next top scanline */
		    while (toprun_count == 0) {
			toprun_count = *topRLElen++;
			run_state ^= 1;
		    }
		    toprun_count--;
		    if (run_state & 1) {
			topRLEdata += 1 * voxel_istride;
		    } else {
			;
		    }

		    /* advance to beginning of next bottom scanline */
		    while (botrun_count == 0) {
			botrun_count = *botRLElen++;
			run_state ^= 2;
		    }
		    botrun_count--;
		    if (run_state & 2) {
			botRLEdata += 1 * voxel_istride;
		    } else {
			;
		    }
		}

#else /* !INDEX_VOLUME */
		/* skip over voxels in top scanline */
		count = ert_skip_count;
		for (;;) {
		    if (toprun_count >= count) {
			toprun_count -= count;
			if (run_state & 1) {
			    topRLEdata += count * voxel_istride;
			} else {
			    ;
			}
			break;
		    } else {
			count -= toprun_count;
			if (run_state & 1) {
			    topRLEdata += toprun_count * voxel_istride;
			} else {
			    ;
			}
			toprun_count = *topRLElen++;
			if (toprun_count == 0)
			    toprun_count = *topRLElen++;
			else
			    run_state ^= 1;
		    }
		}

		/* skip over voxels in bottom scanline */
		count = ert_skip_count;
		for (;;) {
		    if (botrun_count >= count) {
			botrun_count -= count;
			if (run_state & 2) {
			    botRLEdata += count * voxel_istride;
			} else {
			    ;
			}
			break;
		    } else {
			count -= botrun_count;
			if (run_state & 2) {
			    botRLEdata += botrun_count * voxel_istride;
			} else {
			    ;
			}
			botrun_count = *botRLElen++;
			if (botrun_count == 0)
			    botrun_count = *botRLElen++;
			else
			    run_state ^= 2;
		    }
		}
#endif /* INDEX_VOLUME */
		i -= ert_skip_count;
		last_run_state = run_state;
		if (i == 0) {
#ifdef UNROLL_RUN_LOOP
		    break;
#else
		    if (last_run_state == ALL_ZERO) {
			ipixel += 1; shadow_pixel += 1;
			final_run_state = ALL_ZERO;
			i = -1;
			Debug((vpc, VPDEBUG_COMPOSITE, "ZeroSkip(1)End\n"));
			break;	/* scanline is done */
		    }
		    if (ipixel->lnk != 0) {
			ipixel += 1; shadow_pixel += 1;
			final_run_state = last_run_state;
			i = -1;
			Debug((vpc, VPDEBUG_COMPOSITE, "ERTSkip(1)End\n"));
			break;	/* scanline is done */
		    }
#endif /* UNROLL_RUN_LOOP */
		}

		GET_HIRES_TIME(vpc, t1);
		STORE_HIRES_TIME(vpc, VPTIMER_ERT, t0, t1);
		COPY_HIRES_TIME(t0, t1);
	    }
	    ASSERT(ipixel->opcflt < max_opacity);
#endif /* SKIP_ERT */

	    /***********************************************************
	     * Compute the length of the current run.
	     ***********************************************************/

#ifndef UNROLL_RUN_LOOP
	    if (i == 0) {
		final_run_state = last_run_state;
		run_state = ALL_ZERO;
		i = -1;
		count = 1;
		Debug((vpc, VPDEBUG_COMPOSITE, "Run(1)End\n"));
	    } else {
#endif
		/* check if this is the start of a new run */
		while (toprun_count == 0) {
		    toprun_count = *topRLElen++;
		    run_state ^= 1;
		}
		while (botrun_count == 0) {
		    botrun_count = *botRLElen++;
		    run_state ^= 2;
		}

		/* find the union of the two runs of voxels */
		count = MIN(toprun_count, botrun_count);
		toprun_count -= count;
		botrun_count -= count;
		i -= count;
		Debug((vpc, VPDEBUG_COMPOSITE, "Run(%d)\n", count));
		ASSERT(i >= 0);
#ifndef UNROLL_RUN_LOOP
	    }
#endif
	    COUNT_RUN_FRAGMENT;

	    /***********************************************************
	     * composite the voxels in the current run.
	     ***********************************************************/

	    GET_HIRES_TIME(vpc, t1);
	    STORE_HIRES_TIME(vpc, VPTIMER_TRAVERSE_RUNS, t0, t1);
	    COPY_HIRES_TIME(t0, t1);

#ifdef SKIP_COMPOSITE
	    switch (run_state) {
	    case ALL_ZERO:
		ipixel += count; shadow_pixel += count;
		;
		;
		count = 0;
		break;
	    case TOP_NONZERO:
		ipixel += count; shadow_pixel += count;
		topRLEdata += count * voxel_istride;
		;
		count = 0;
		break;
	    case BOT_NONZERO:
		ipixel += count; shadow_pixel += count;
		;
		botRLEdata += count * voxel_istride;
		count = 0;
		break;
	    case ALL_NONZERO:
		ipixel += count; shadow_pixel += count;
		topRLEdata += count * voxel_istride;
		botRLEdata += count * voxel_istride;
		count = 0;
		break;
	    }
#else /* !SKIP_COMPOSITE */
#ifdef UNROLL_RUN_LOOP
	    /* this run contains pixels, so process them */
	    switch ((last_run_state << 2) | run_state) {
	    case ALL_ZERO__ALL_ZERO:
		/* no voxels contribute to the pixels in this run */
		ipixel += count; shadow_pixel += count;
		;
		;
		count = 0;
		break;
	    case TOP_NONZERO__ALL_ZERO:
		/* only the top-left voxel contributes to the first
		   pixel of the run, and the rest are zero */
		if (!voxels_loaded) {
		    
    top_opc = opac_correct[ByteField(topRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata - voxel_istride, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
		}
		
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = 0.; trace_opcBL = 0.; trace_opcTR = 0.; trace_opcBR = 0.;
	trace_rsclrTL=0.; trace_rsclrBL=0.; trace_rsclrTR=0.; trace_rsclrBR=0.;
	trace_rclrTL= 0.; trace_rclrBL= 0.; trace_rclrTR= 0.; trace_rclrBR= 0.;
	trace_gclrTL= 0.; trace_gclrBL= 0.; trace_gclrTR= 0.; trace_gclrBR= 0.;
	trace_bclrTL= 0.; trace_bclrBL= 0.; trace_bclrTR= 0.; trace_bclrBR= 0.;
    }
#endif
;
		
       acc_opc = top_opc * wgtTL;
       
	    acc_rclr = (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_gclr = (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_bclr = (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = top_opc;
        trace_rclrTL = top_rclr;
                     trace_gclrTL = top_gclr;
                     trace_bclrTL = top_bclr;
	
    }
#endif
;
		
	COUNT_RESAMPLE;
	if (acc_opc > min_opacity) {
	    COUNT_COMPOSITE;
	    iopc = ipixel->opcflt;
#           ifndef SKIP_ERT
	        ASSERT(iopc < max_opacity);
#           endif
	    iopc_inv = (float)1. - iopc;
	    
	ipixel->rclrflt += acc_rclr * iopc_inv;
	ipixel->gclrflt += acc_gclr * iopc_inv;
	ipixel->bclrflt += acc_bclr * iopc_inv;
	    iopc += acc_opc * iopc_inv;
	    ipixel->opcflt = iopc;
	    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
#ifdef COMPUTE_SHADOW_BUFFER
	printf("{%3d}  %3d %3d", k, icount-i-count, j);
#else
	printf("[%3d]  %3d %3d", k, icount-i-count, j);
#endif
	printf("  %3.0f %3.0f %3.0f",trace_opcTL*255.,trace_rclrTL,wgtTL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBL*255.,trace_rclrBL,wgtBL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcTR*255.,trace_rclrTR,wgtTR*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBR*255.,trace_rclrBR,wgtBR*100.);
	printf("  %3.0f %3.0f\n", iopc*255., 
	       ipixel->rclrflt);
        
	printf("              ");
	printf("      %3.0f    ",trace_rsclrTL);
	printf("      %3.0f    ",trace_rsclrBL);
	printf("      %3.0f    ",trace_rsclrTR);
	printf("      %3.0f    ",trace_rsclrBR);
	printf("  %3.0f\n", shadow_pixel->opcflt * 255.);
        
	printf("              ");
	printf("      %3.0f    ",trace_gclrTL);
	printf("      %3.0f    ",trace_gclrBL);
	printf("      %3.0f    ",trace_gclrTR);
	printf("      %3.0f    ",trace_gclrBR);
	printf("      %3.0f\n", ipixel->gclrflt);
	printf("              ");
	printf("      %3.0f    ",trace_bclrTL);
	printf("      %3.0f    ",trace_bclrBL);
	printf("      %3.0f    ",trace_bclrTR);
	printf("      %3.0f    ",trace_bclrBR);
	printf("      %3.0f\n", ipixel->bclrflt);
    }
#endif /* DEBUG */
;
#           ifndef SKIP_ERT
	        if (iopc >= max_opacity) {
		    ASSERT(ipixel->lnk == 0);
		    ipixel->lnk = 1;
	        }
#           endif
	};
		ipixel += count; shadow_pixel += count;
		;
		;
		count = 0;
		break;
	    case BOT_NONZERO__ALL_ZERO:
		/* only the bottom left voxel contributes to the first
		   pixel of the run, and the rest are zero */
		if (!voxels_loaded) {
		    
    bot_opc = opac_correct[ByteField(botRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata - voxel_istride, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		}
		
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = 0.; trace_opcBL = 0.; trace_opcTR = 0.; trace_opcBR = 0.;
	trace_rsclrTL=0.; trace_rsclrBL=0.; trace_rsclrTR=0.; trace_rsclrBR=0.;
	trace_rclrTL= 0.; trace_rclrBL= 0.; trace_rclrTR= 0.; trace_rclrBR= 0.;
	trace_gclrTL= 0.; trace_gclrBL= 0.; trace_gclrTR= 0.; trace_gclrBR= 0.;
	trace_bclrTL= 0.; trace_bclrBL= 0.; trace_bclrTR= 0.; trace_bclrBR= 0.;
    }
#endif
;
		
       acc_opc = bot_opc * wgtBL;
       
	    acc_rclr = (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_gclr = (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_bclr = (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBL = bot_opc;
        trace_rclrBL = bot_rclr;
                     trace_gclrBL = bot_gclr;
                     trace_bclrBL = bot_bclr;
	
    }
#endif
;
		
	COUNT_RESAMPLE;
	if (acc_opc > min_opacity) {
	    COUNT_COMPOSITE;
	    iopc = ipixel->opcflt;
#           ifndef SKIP_ERT
	        ASSERT(iopc < max_opacity);
#           endif
	    iopc_inv = (float)1. - iopc;
	    
	ipixel->rclrflt += acc_rclr * iopc_inv;
	ipixel->gclrflt += acc_gclr * iopc_inv;
	ipixel->bclrflt += acc_bclr * iopc_inv;
	    iopc += acc_opc * iopc_inv;
	    ipixel->opcflt = iopc;
	    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
#ifdef COMPUTE_SHADOW_BUFFER
	printf("{%3d}  %3d %3d", k, icount-i-count, j);
#else
	printf("[%3d]  %3d %3d", k, icount-i-count, j);
#endif
	printf("  %3.0f %3.0f %3.0f",trace_opcTL*255.,trace_rclrTL,wgtTL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBL*255.,trace_rclrBL,wgtBL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcTR*255.,trace_rclrTR,wgtTR*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBR*255.,trace_rclrBR,wgtBR*100.);
	printf("  %3.0f %3.0f\n", iopc*255., 
	       ipixel->rclrflt);
        
	printf("              ");
	printf("      %3.0f    ",trace_rsclrTL);
	printf("      %3.0f    ",trace_rsclrBL);
	printf("      %3.0f    ",trace_rsclrTR);
	printf("      %3.0f    ",trace_rsclrBR);
	printf("  %3.0f\n", shadow_pixel->opcflt * 255.);
        
	printf("              ");
	printf("      %3.0f    ",trace_gclrTL);
	printf("      %3.0f    ",trace_gclrBL);
	printf("      %3.0f    ",trace_gclrTR);
	printf("      %3.0f    ",trace_gclrBR);
	printf("      %3.0f\n", ipixel->gclrflt);
	printf("              ");
	printf("      %3.0f    ",trace_bclrTL);
	printf("      %3.0f    ",trace_bclrBL);
	printf("      %3.0f    ",trace_bclrTR);
	printf("      %3.0f    ",trace_bclrBR);
	printf("      %3.0f\n", ipixel->bclrflt);
    }
#endif /* DEBUG */
;
#           ifndef SKIP_ERT
	        if (iopc >= max_opacity) {
		    ASSERT(ipixel->lnk == 0);
		    ipixel->lnk = 1;
	        }
#           endif
	};
		ipixel += count; shadow_pixel += count;
		;
		;
		count = 0;
		break;
	    case ALL_NONZERO__ALL_ZERO:
		/* the top and bottom left voxels contribute to the
		   first pixel of the run, and the rest are zero */
		if (!voxels_loaded) {
		    
    top_opc = opac_correct[ByteField(topRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata - voxel_istride, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
		    
    bot_opc = opac_correct[ByteField(botRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata - voxel_istride, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		}
		
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = 0.; trace_opcBL = 0.; trace_opcTR = 0.; trace_opcBR = 0.;
	trace_rsclrTL=0.; trace_rsclrBL=0.; trace_rsclrTR=0.; trace_rsclrBR=0.;
	trace_rclrTL= 0.; trace_rclrBL= 0.; trace_rclrTR= 0.; trace_rclrBR= 0.;
	trace_gclrTL= 0.; trace_gclrBL= 0.; trace_gclrTR= 0.; trace_gclrBR= 0.;
	trace_bclrTL= 0.; trace_bclrBL= 0.; trace_bclrTR= 0.; trace_bclrBR= 0.;
    }
#endif
;
		
       acc_opc = top_opc * wgtTL;
       
	    acc_rclr = (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_gclr = (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_bclr = (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = top_opc;
        trace_rclrTL = top_rclr;
                     trace_gclrTL = top_gclr;
                     trace_bclrTL = top_bclr;
	
    }
#endif
;
		
       acc_opc += bot_opc * wgtBL;
       
	    acc_rclr += (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_gclr += (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_bclr += (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBL = bot_opc;
        trace_rclrBL = bot_rclr;
                     trace_gclrBL = bot_gclr;
                     trace_bclrBL = bot_bclr;
	
    }
#endif
;
		
	COUNT_RESAMPLE;
	if (acc_opc > min_opacity) {
	    COUNT_COMPOSITE;
	    iopc = ipixel->opcflt;
#           ifndef SKIP_ERT
	        ASSERT(iopc < max_opacity);
#           endif
	    iopc_inv = (float)1. - iopc;
	    
	ipixel->rclrflt += acc_rclr * iopc_inv;
	ipixel->gclrflt += acc_gclr * iopc_inv;
	ipixel->bclrflt += acc_bclr * iopc_inv;
	    iopc += acc_opc * iopc_inv;
	    ipixel->opcflt = iopc;
	    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
#ifdef COMPUTE_SHADOW_BUFFER
	printf("{%3d}  %3d %3d", k, icount-i-count, j);
#else
	printf("[%3d]  %3d %3d", k, icount-i-count, j);
#endif
	printf("  %3.0f %3.0f %3.0f",trace_opcTL*255.,trace_rclrTL,wgtTL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBL*255.,trace_rclrBL,wgtBL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcTR*255.,trace_rclrTR,wgtTR*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBR*255.,trace_rclrBR,wgtBR*100.);
	printf("  %3.0f %3.0f\n", iopc*255., 
	       ipixel->rclrflt);
        
	printf("              ");
	printf("      %3.0f    ",trace_rsclrTL);
	printf("      %3.0f    ",trace_rsclrBL);
	printf("      %3.0f    ",trace_rsclrTR);
	printf("      %3.0f    ",trace_rsclrBR);
	printf("  %3.0f\n", shadow_pixel->opcflt * 255.);
        
	printf("              ");
	printf("      %3.0f    ",trace_gclrTL);
	printf("      %3.0f    ",trace_gclrBL);
	printf("      %3.0f    ",trace_gclrTR);
	printf("      %3.0f    ",trace_gclrBR);
	printf("      %3.0f\n", ipixel->gclrflt);
	printf("              ");
	printf("      %3.0f    ",trace_bclrTL);
	printf("      %3.0f    ",trace_bclrBL);
	printf("      %3.0f    ",trace_bclrTR);
	printf("      %3.0f    ",trace_bclrBR);
	printf("      %3.0f\n", ipixel->bclrflt);
    }
#endif /* DEBUG */
;
#           ifndef SKIP_ERT
	        if (iopc >= max_opacity) {
		    ASSERT(ipixel->lnk == 0);
		    ipixel->lnk = 1;
	        }
#           endif
	};
		ipixel += count; shadow_pixel += count;
		;
		;
		count = 0;
		break;
	    case ALL_ZERO__TOP_NONZERO:
		/* first pixel: only the top-right voxel contributes */
		
    top_opc = opac_correct[ByteField(topRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
		
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = 0.; trace_opcBL = 0.; trace_opcTR = 0.; trace_opcBR = 0.;
	trace_rsclrTL=0.; trace_rsclrBL=0.; trace_rsclrTR=0.; trace_rsclrBR=0.;
	trace_rclrTL= 0.; trace_rclrBL= 0.; trace_rclrTR= 0.; trace_rclrBR= 0.;
	trace_gclrTL= 0.; trace_gclrBL= 0.; trace_gclrTR= 0.; trace_gclrBR= 0.;
	trace_bclrTL= 0.; trace_bclrBL= 0.; trace_bclrTR= 0.; trace_bclrBR= 0.;
    }
#endif
;
		
       acc_opc = top_opc * wgtTR;
       
	    acc_rclr = (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_gclr = (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_bclr = (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTR = top_opc;
        trace_rclrTR = top_rclr;
                     trace_gclrTR = top_gclr;
                     trace_bclrTR = top_bclr;
	
    }
#endif
;
		
	COUNT_RESAMPLE;
	if (acc_opc > min_opacity) {
	    COUNT_COMPOSITE;
	    iopc = ipixel->opcflt;
#           ifndef SKIP_ERT
	        ASSERT(iopc < max_opacity);
#           endif
	    iopc_inv = (float)1. - iopc;
	    
	ipixel->rclrflt += acc_rclr * iopc_inv;
	ipixel->gclrflt += acc_gclr * iopc_inv;
	ipixel->bclrflt += acc_bclr * iopc_inv;
	    iopc += acc_opc * iopc_inv;
	    ipixel->opcflt = iopc;
	    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
#ifdef COMPUTE_SHADOW_BUFFER
	printf("{%3d}  %3d %3d", k, icount-i-count, j);
#else
	printf("[%3d]  %3d %3d", k, icount-i-count, j);
#endif
	printf("  %3.0f %3.0f %3.0f",trace_opcTL*255.,trace_rclrTL,wgtTL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBL*255.,trace_rclrBL,wgtBL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcTR*255.,trace_rclrTR,wgtTR*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBR*255.,trace_rclrBR,wgtBR*100.);
	printf("  %3.0f %3.0f\n", iopc*255., 
	       ipixel->rclrflt);
        
	printf("              ");
	printf("      %3.0f    ",trace_rsclrTL);
	printf("      %3.0f    ",trace_rsclrBL);
	printf("      %3.0f    ",trace_rsclrTR);
	printf("      %3.0f    ",trace_rsclrBR);
	printf("  %3.0f\n", shadow_pixel->opcflt * 255.);
        
	printf("              ");
	printf("      %3.0f    ",trace_gclrTL);
	printf("      %3.0f    ",trace_gclrBL);
	printf("      %3.0f    ",trace_gclrTR);
	printf("      %3.0f    ",trace_gclrBR);
	printf("      %3.0f\n", ipixel->gclrflt);
	printf("              ");
	printf("      %3.0f    ",trace_bclrTL);
	printf("      %3.0f    ",trace_bclrBL);
	printf("      %3.0f    ",trace_bclrTR);
	printf("      %3.0f    ",trace_bclrBR);
	printf("      %3.0f\n", ipixel->bclrflt);
    }
#endif /* DEBUG */
;
#           ifndef SKIP_ERT
	        if (iopc >= max_opacity) {
		    ASSERT(ipixel->lnk == 0);
		    ipixel->lnk = 1;
	        }
#           endif
	};
		ipixel += 1; shadow_pixel += 1;
		topRLEdata += 1 * voxel_istride;
		;
		count--;
		SET_VOXELS_LOADED;

		/* do the rest of the pixels in this run;
		   the top-left and top-right voxels contribute */
		while (count > 0) {
		    if (PIXEL_IS_OPAQUE(ipixel))
			break;
		    if (!voxels_loaded) {
			
    top_opc = opac_correct[ByteField(topRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata - voxel_istride, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
		    }
		    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = 0.; trace_opcBL = 0.; trace_opcTR = 0.; trace_opcBR = 0.;
	trace_rsclrTL=0.; trace_rsclrBL=0.; trace_rsclrTR=0.; trace_rsclrBR=0.;
	trace_rclrTL= 0.; trace_rclrBL= 0.; trace_rclrTR= 0.; trace_rclrBR= 0.;
	trace_gclrTL= 0.; trace_gclrBL= 0.; trace_gclrTR= 0.; trace_gclrBR= 0.;
	trace_bclrTL= 0.; trace_bclrBL= 0.; trace_bclrTR= 0.; trace_bclrBR= 0.;
    }
#endif
;
		    
       acc_opc = top_opc * wgtTL;
       
	    acc_rclr = (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_gclr = (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_bclr = (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = top_opc;
        trace_rclrTL = top_rclr;
                     trace_gclrTL = top_gclr;
                     trace_bclrTL = top_bclr;
	
    }
#endif
;
		    
    top_opc = opac_correct[ByteField(topRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
		    
       acc_opc += top_opc * wgtTR;
       
	    acc_rclr += (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_gclr += (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_bclr += (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTR = top_opc;
        trace_rclrTR = top_rclr;
                     trace_gclrTR = top_gclr;
                     trace_bclrTR = top_bclr;
	
    }
#endif
;
		    
	COUNT_RESAMPLE;
	if (acc_opc > min_opacity) {
	    COUNT_COMPOSITE;
	    iopc = ipixel->opcflt;
#           ifndef SKIP_ERT
	        ASSERT(iopc < max_opacity);
#           endif
	    iopc_inv = (float)1. - iopc;
	    
	ipixel->rclrflt += acc_rclr * iopc_inv;
	ipixel->gclrflt += acc_gclr * iopc_inv;
	ipixel->bclrflt += acc_bclr * iopc_inv;
	    iopc += acc_opc * iopc_inv;
	    ipixel->opcflt = iopc;
	    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
#ifdef COMPUTE_SHADOW_BUFFER
	printf("{%3d}  %3d %3d", k, icount-i-count, j);
#else
	printf("[%3d]  %3d %3d", k, icount-i-count, j);
#endif
	printf("  %3.0f %3.0f %3.0f",trace_opcTL*255.,trace_rclrTL,wgtTL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBL*255.,trace_rclrBL,wgtBL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcTR*255.,trace_rclrTR,wgtTR*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBR*255.,trace_rclrBR,wgtBR*100.);
	printf("  %3.0f %3.0f\n", iopc*255., 
	       ipixel->rclrflt);
        
	printf("              ");
	printf("      %3.0f    ",trace_rsclrTL);
	printf("      %3.0f    ",trace_rsclrBL);
	printf("      %3.0f    ",trace_rsclrTR);
	printf("      %3.0f    ",trace_rsclrBR);
	printf("  %3.0f\n", shadow_pixel->opcflt * 255.);
        
	printf("              ");
	printf("      %3.0f    ",trace_gclrTL);
	printf("      %3.0f    ",trace_gclrBL);
	printf("      %3.0f    ",trace_gclrTR);
	printf("      %3.0f    ",trace_gclrBR);
	printf("      %3.0f\n", ipixel->gclrflt);
	printf("              ");
	printf("      %3.0f    ",trace_bclrTL);
	printf("      %3.0f    ",trace_bclrBL);
	printf("      %3.0f    ",trace_bclrTR);
	printf("      %3.0f    ",trace_bclrBR);
	printf("      %3.0f\n", ipixel->bclrflt);
    }
#endif /* DEBUG */
;
#           ifndef SKIP_ERT
	        if (iopc >= max_opacity) {
		    ASSERT(ipixel->lnk == 0);
		    ipixel->lnk = 1;
	        }
#           endif
	};
		    ipixel += 1; shadow_pixel += 1;
		    topRLEdata += 1 * voxel_istride;
		    ;
		    count--;
		    SET_VOXELS_LOADED;
		}
		break;
	    case TOP_NONZERO__TOP_NONZERO:
		/* do the pixels in this run; the top-left and
		   top-right voxels contribute */
		while (count > 0) {
		    if (PIXEL_IS_OPAQUE(ipixel))
			break;
		    if (!voxels_loaded) {
			
    top_opc = opac_correct[ByteField(topRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata - voxel_istride, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
		    }
		    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = 0.; trace_opcBL = 0.; trace_opcTR = 0.; trace_opcBR = 0.;
	trace_rsclrTL=0.; trace_rsclrBL=0.; trace_rsclrTR=0.; trace_rsclrBR=0.;
	trace_rclrTL= 0.; trace_rclrBL= 0.; trace_rclrTR= 0.; trace_rclrBR= 0.;
	trace_gclrTL= 0.; trace_gclrBL= 0.; trace_gclrTR= 0.; trace_gclrBR= 0.;
	trace_bclrTL= 0.; trace_bclrBL= 0.; trace_bclrTR= 0.; trace_bclrBR= 0.;
    }
#endif
;
		    
       acc_opc = top_opc * wgtTL;
       
	    acc_rclr = (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_gclr = (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_bclr = (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = top_opc;
        trace_rclrTL = top_rclr;
                     trace_gclrTL = top_gclr;
                     trace_bclrTL = top_bclr;
	
    }
#endif
;
		    
    top_opc = opac_correct[ByteField(topRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
		    
       acc_opc += top_opc * wgtTR;
       
	    acc_rclr += (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_gclr += (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_bclr += (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTR = top_opc;
        trace_rclrTR = top_rclr;
                     trace_gclrTR = top_gclr;
                     trace_bclrTR = top_bclr;
	
    }
#endif
;
		    
	COUNT_RESAMPLE;
	if (acc_opc > min_opacity) {
	    COUNT_COMPOSITE;
	    iopc = ipixel->opcflt;
#           ifndef SKIP_ERT
	        ASSERT(iopc < max_opacity);
#           endif
	    iopc_inv = (float)1. - iopc;
	    
	ipixel->rclrflt += acc_rclr * iopc_inv;
	ipixel->gclrflt += acc_gclr * iopc_inv;
	ipixel->bclrflt += acc_bclr * iopc_inv;
	    iopc += acc_opc * iopc_inv;
	    ipixel->opcflt = iopc;
	    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
#ifdef COMPUTE_SHADOW_BUFFER
	printf("{%3d}  %3d %3d", k, icount-i-count, j);
#else
	printf("[%3d]  %3d %3d", k, icount-i-count, j);
#endif
	printf("  %3.0f %3.0f %3.0f",trace_opcTL*255.,trace_rclrTL,wgtTL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBL*255.,trace_rclrBL,wgtBL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcTR*255.,trace_rclrTR,wgtTR*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBR*255.,trace_rclrBR,wgtBR*100.);
	printf("  %3.0f %3.0f\n", iopc*255., 
	       ipixel->rclrflt);
        
	printf("              ");
	printf("      %3.0f    ",trace_rsclrTL);
	printf("      %3.0f    ",trace_rsclrBL);
	printf("      %3.0f    ",trace_rsclrTR);
	printf("      %3.0f    ",trace_rsclrBR);
	printf("  %3.0f\n", shadow_pixel->opcflt * 255.);
        
	printf("              ");
	printf("      %3.0f    ",trace_gclrTL);
	printf("      %3.0f    ",trace_gclrBL);
	printf("      %3.0f    ",trace_gclrTR);
	printf("      %3.0f    ",trace_gclrBR);
	printf("      %3.0f\n", ipixel->gclrflt);
	printf("              ");
	printf("      %3.0f    ",trace_bclrTL);
	printf("      %3.0f    ",trace_bclrBL);
	printf("      %3.0f    ",trace_bclrTR);
	printf("      %3.0f    ",trace_bclrBR);
	printf("      %3.0f\n", ipixel->bclrflt);
    }
#endif /* DEBUG */
;
#           ifndef SKIP_ERT
	        if (iopc >= max_opacity) {
		    ASSERT(ipixel->lnk == 0);
		    ipixel->lnk = 1;
	        }
#           endif
	};
		    ipixel += 1; shadow_pixel += 1;
		    topRLEdata += 1 * voxel_istride;
		    ;
		    count--;
		    SET_VOXELS_LOADED;
		}
		break;
	    case BOT_NONZERO__TOP_NONZERO:
		/* first pixel: bottom-left and top-right voxels
		   contribute */
		if (!voxels_loaded) {
		    
    bot_opc = opac_correct[ByteField(botRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata - voxel_istride, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		}
		
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = 0.; trace_opcBL = 0.; trace_opcTR = 0.; trace_opcBR = 0.;
	trace_rsclrTL=0.; trace_rsclrBL=0.; trace_rsclrTR=0.; trace_rsclrBR=0.;
	trace_rclrTL= 0.; trace_rclrBL= 0.; trace_rclrTR= 0.; trace_rclrBR= 0.;
	trace_gclrTL= 0.; trace_gclrBL= 0.; trace_gclrTR= 0.; trace_gclrBR= 0.;
	trace_bclrTL= 0.; trace_bclrBL= 0.; trace_bclrTR= 0.; trace_bclrBR= 0.;
    }
#endif
;
		
       acc_opc = bot_opc * wgtBL;
       
	    acc_rclr = (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_gclr = (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_bclr = (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBL = bot_opc;
        trace_rclrBL = bot_rclr;
                     trace_gclrBL = bot_gclr;
                     trace_bclrBL = bot_bclr;
	
    }
#endif
;
		
    top_opc = opac_correct[ByteField(topRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
		
       acc_opc += top_opc * wgtTR;
       
	    acc_rclr += (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_gclr += (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_bclr += (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTR = top_opc;
        trace_rclrTR = top_rclr;
                     trace_gclrTR = top_gclr;
                     trace_bclrTR = top_bclr;
	
    }
#endif
;
		
	COUNT_RESAMPLE;
	if (acc_opc > min_opacity) {
	    COUNT_COMPOSITE;
	    iopc = ipixel->opcflt;
#           ifndef SKIP_ERT
	        ASSERT(iopc < max_opacity);
#           endif
	    iopc_inv = (float)1. - iopc;
	    
	ipixel->rclrflt += acc_rclr * iopc_inv;
	ipixel->gclrflt += acc_gclr * iopc_inv;
	ipixel->bclrflt += acc_bclr * iopc_inv;
	    iopc += acc_opc * iopc_inv;
	    ipixel->opcflt = iopc;
	    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
#ifdef COMPUTE_SHADOW_BUFFER
	printf("{%3d}  %3d %3d", k, icount-i-count, j);
#else
	printf("[%3d]  %3d %3d", k, icount-i-count, j);
#endif
	printf("  %3.0f %3.0f %3.0f",trace_opcTL*255.,trace_rclrTL,wgtTL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBL*255.,trace_rclrBL,wgtBL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcTR*255.,trace_rclrTR,wgtTR*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBR*255.,trace_rclrBR,wgtBR*100.);
	printf("  %3.0f %3.0f\n", iopc*255., 
	       ipixel->rclrflt);
        
	printf("              ");
	printf("      %3.0f    ",trace_rsclrTL);
	printf("      %3.0f    ",trace_rsclrBL);
	printf("      %3.0f    ",trace_rsclrTR);
	printf("      %3.0f    ",trace_rsclrBR);
	printf("  %3.0f\n", shadow_pixel->opcflt * 255.);
        
	printf("              ");
	printf("      %3.0f    ",trace_gclrTL);
	printf("      %3.0f    ",trace_gclrBL);
	printf("      %3.0f    ",trace_gclrTR);
	printf("      %3.0f    ",trace_gclrBR);
	printf("      %3.0f\n", ipixel->gclrflt);
	printf("              ");
	printf("      %3.0f    ",trace_bclrTL);
	printf("      %3.0f    ",trace_bclrBL);
	printf("      %3.0f    ",trace_bclrTR);
	printf("      %3.0f    ",trace_bclrBR);
	printf("      %3.0f\n", ipixel->bclrflt);
    }
#endif /* DEBUG */
;
#           ifndef SKIP_ERT
	        if (iopc >= max_opacity) {
		    ASSERT(ipixel->lnk == 0);
		    ipixel->lnk = 1;
	        }
#           endif
	};
		ipixel += 1; shadow_pixel += 1;
		topRLEdata += 1 * voxel_istride;
		;
		count--;
		SET_VOXELS_LOADED;

		/* do the rest of the pixels in this run;
		   the top-left and top-right voxels contribute */
		while (count > 0) {
		    if (PIXEL_IS_OPAQUE(ipixel))
			break;
		    if (!voxels_loaded) {
			
    top_opc = opac_correct[ByteField(topRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata - voxel_istride, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
		    }
		    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = 0.; trace_opcBL = 0.; trace_opcTR = 0.; trace_opcBR = 0.;
	trace_rsclrTL=0.; trace_rsclrBL=0.; trace_rsclrTR=0.; trace_rsclrBR=0.;
	trace_rclrTL= 0.; trace_rclrBL= 0.; trace_rclrTR= 0.; trace_rclrBR= 0.;
	trace_gclrTL= 0.; trace_gclrBL= 0.; trace_gclrTR= 0.; trace_gclrBR= 0.;
	trace_bclrTL= 0.; trace_bclrBL= 0.; trace_bclrTR= 0.; trace_bclrBR= 0.;
    }
#endif
;
		    
       acc_opc = top_opc * wgtTL;
       
	    acc_rclr = (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_gclr = (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_bclr = (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = top_opc;
        trace_rclrTL = top_rclr;
                     trace_gclrTL = top_gclr;
                     trace_bclrTL = top_bclr;
	
    }
#endif
;
		    
    top_opc = opac_correct[ByteField(topRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
		    
       acc_opc += top_opc * wgtTR;
       
	    acc_rclr += (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_gclr += (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_bclr += (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTR = top_opc;
        trace_rclrTR = top_rclr;
                     trace_gclrTR = top_gclr;
                     trace_bclrTR = top_bclr;
	
    }
#endif
;
		    
	COUNT_RESAMPLE;
	if (acc_opc > min_opacity) {
	    COUNT_COMPOSITE;
	    iopc = ipixel->opcflt;
#           ifndef SKIP_ERT
	        ASSERT(iopc < max_opacity);
#           endif
	    iopc_inv = (float)1. - iopc;
	    
	ipixel->rclrflt += acc_rclr * iopc_inv;
	ipixel->gclrflt += acc_gclr * iopc_inv;
	ipixel->bclrflt += acc_bclr * iopc_inv;
	    iopc += acc_opc * iopc_inv;
	    ipixel->opcflt = iopc;
	    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
#ifdef COMPUTE_SHADOW_BUFFER
	printf("{%3d}  %3d %3d", k, icount-i-count, j);
#else
	printf("[%3d]  %3d %3d", k, icount-i-count, j);
#endif
	printf("  %3.0f %3.0f %3.0f",trace_opcTL*255.,trace_rclrTL,wgtTL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBL*255.,trace_rclrBL,wgtBL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcTR*255.,trace_rclrTR,wgtTR*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBR*255.,trace_rclrBR,wgtBR*100.);
	printf("  %3.0f %3.0f\n", iopc*255., 
	       ipixel->rclrflt);
        
	printf("              ");
	printf("      %3.0f    ",trace_rsclrTL);
	printf("      %3.0f    ",trace_rsclrBL);
	printf("      %3.0f    ",trace_rsclrTR);
	printf("      %3.0f    ",trace_rsclrBR);
	printf("  %3.0f\n", shadow_pixel->opcflt * 255.);
        
	printf("              ");
	printf("      %3.0f    ",trace_gclrTL);
	printf("      %3.0f    ",trace_gclrBL);
	printf("      %3.0f    ",trace_gclrTR);
	printf("      %3.0f    ",trace_gclrBR);
	printf("      %3.0f\n", ipixel->gclrflt);
	printf("              ");
	printf("      %3.0f    ",trace_bclrTL);
	printf("      %3.0f    ",trace_bclrBL);
	printf("      %3.0f    ",trace_bclrTR);
	printf("      %3.0f    ",trace_bclrBR);
	printf("      %3.0f\n", ipixel->bclrflt);
    }
#endif /* DEBUG */
;
#           ifndef SKIP_ERT
	        if (iopc >= max_opacity) {
		    ASSERT(ipixel->lnk == 0);
		    ipixel->lnk = 1;
	        }
#           endif
	};
		    ipixel += 1; shadow_pixel += 1;
		    topRLEdata += 1 * voxel_istride;
		    ;
		    count--;
		    SET_VOXELS_LOADED;
		}
		break;
	    case ALL_NONZERO__TOP_NONZERO:
		/* first pixel: top-left, bottom-left and top-right voxels
		   contribute */
		if (!voxels_loaded) {
		    
    top_opc = opac_correct[ByteField(topRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata - voxel_istride, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
		    
    bot_opc = opac_correct[ByteField(botRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata - voxel_istride, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		}
		
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = 0.; trace_opcBL = 0.; trace_opcTR = 0.; trace_opcBR = 0.;
	trace_rsclrTL=0.; trace_rsclrBL=0.; trace_rsclrTR=0.; trace_rsclrBR=0.;
	trace_rclrTL= 0.; trace_rclrBL= 0.; trace_rclrTR= 0.; trace_rclrBR= 0.;
	trace_gclrTL= 0.; trace_gclrBL= 0.; trace_gclrTR= 0.; trace_gclrBR= 0.;
	trace_bclrTL= 0.; trace_bclrBL= 0.; trace_bclrTR= 0.; trace_bclrBR= 0.;
    }
#endif
;
		
       acc_opc = top_opc * wgtTL;
       
	    acc_rclr = (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_gclr = (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_bclr = (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = top_opc;
        trace_rclrTL = top_rclr;
                     trace_gclrTL = top_gclr;
                     trace_bclrTL = top_bclr;
	
    }
#endif
;
		
       acc_opc += bot_opc * wgtBL;
       
	    acc_rclr += (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_gclr += (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_bclr += (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBL = bot_opc;
        trace_rclrBL = bot_rclr;
                     trace_gclrBL = bot_gclr;
                     trace_bclrBL = bot_bclr;
	
    }
#endif
;
		
    top_opc = opac_correct[ByteField(topRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
		
       acc_opc += top_opc * wgtTR;
       
	    acc_rclr += (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_gclr += (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_bclr += (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTR = top_opc;
        trace_rclrTR = top_rclr;
                     trace_gclrTR = top_gclr;
                     trace_bclrTR = top_bclr;
	
    }
#endif
;
		
	COUNT_RESAMPLE;
	if (acc_opc > min_opacity) {
	    COUNT_COMPOSITE;
	    iopc = ipixel->opcflt;
#           ifndef SKIP_ERT
	        ASSERT(iopc < max_opacity);
#           endif
	    iopc_inv = (float)1. - iopc;
	    
	ipixel->rclrflt += acc_rclr * iopc_inv;
	ipixel->gclrflt += acc_gclr * iopc_inv;
	ipixel->bclrflt += acc_bclr * iopc_inv;
	    iopc += acc_opc * iopc_inv;
	    ipixel->opcflt = iopc;
	    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
#ifdef COMPUTE_SHADOW_BUFFER
	printf("{%3d}  %3d %3d", k, icount-i-count, j);
#else
	printf("[%3d]  %3d %3d", k, icount-i-count, j);
#endif
	printf("  %3.0f %3.0f %3.0f",trace_opcTL*255.,trace_rclrTL,wgtTL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBL*255.,trace_rclrBL,wgtBL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcTR*255.,trace_rclrTR,wgtTR*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBR*255.,trace_rclrBR,wgtBR*100.);
	printf("  %3.0f %3.0f\n", iopc*255., 
	       ipixel->rclrflt);
        
	printf("              ");
	printf("      %3.0f    ",trace_rsclrTL);
	printf("      %3.0f    ",trace_rsclrBL);
	printf("      %3.0f    ",trace_rsclrTR);
	printf("      %3.0f    ",trace_rsclrBR);
	printf("  %3.0f\n", shadow_pixel->opcflt * 255.);
        
	printf("              ");
	printf("      %3.0f    ",trace_gclrTL);
	printf("      %3.0f    ",trace_gclrBL);
	printf("      %3.0f    ",trace_gclrTR);
	printf("      %3.0f    ",trace_gclrBR);
	printf("      %3.0f\n", ipixel->gclrflt);
	printf("              ");
	printf("      %3.0f    ",trace_bclrTL);
	printf("      %3.0f    ",trace_bclrBL);
	printf("      %3.0f    ",trace_bclrTR);
	printf("      %3.0f    ",trace_bclrBR);
	printf("      %3.0f\n", ipixel->bclrflt);
    }
#endif /* DEBUG */
;
#           ifndef SKIP_ERT
	        if (iopc >= max_opacity) {
		    ASSERT(ipixel->lnk == 0);
		    ipixel->lnk = 1;
	        }
#           endif
	};
		ipixel += 1; shadow_pixel += 1;
		topRLEdata += 1 * voxel_istride;
		;
		count--;
		SET_VOXELS_LOADED;

		/* do the rest of the pixels in this run;
		   the top-left and top-right voxels contribute */
		while (count > 0) {
		    if (PIXEL_IS_OPAQUE(ipixel))
			break;
		    if (!voxels_loaded) {
			
    top_opc = opac_correct[ByteField(topRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata - voxel_istride, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
		    }
		    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = 0.; trace_opcBL = 0.; trace_opcTR = 0.; trace_opcBR = 0.;
	trace_rsclrTL=0.; trace_rsclrBL=0.; trace_rsclrTR=0.; trace_rsclrBR=0.;
	trace_rclrTL= 0.; trace_rclrBL= 0.; trace_rclrTR= 0.; trace_rclrBR= 0.;
	trace_gclrTL= 0.; trace_gclrBL= 0.; trace_gclrTR= 0.; trace_gclrBR= 0.;
	trace_bclrTL= 0.; trace_bclrBL= 0.; trace_bclrTR= 0.; trace_bclrBR= 0.;
    }
#endif
;
		    
       acc_opc = top_opc * wgtTL;
       
	    acc_rclr = (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_gclr = (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_bclr = (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = top_opc;
        trace_rclrTL = top_rclr;
                     trace_gclrTL = top_gclr;
                     trace_bclrTL = top_bclr;
	
    }
#endif
;
		    
    top_opc = opac_correct[ByteField(topRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
		    
       acc_opc += top_opc * wgtTR;
       
	    acc_rclr += (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_gclr += (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_bclr += (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTR = top_opc;
        trace_rclrTR = top_rclr;
                     trace_gclrTR = top_gclr;
                     trace_bclrTR = top_bclr;
	
    }
#endif
;
		    
	COUNT_RESAMPLE;
	if (acc_opc > min_opacity) {
	    COUNT_COMPOSITE;
	    iopc = ipixel->opcflt;
#           ifndef SKIP_ERT
	        ASSERT(iopc < max_opacity);
#           endif
	    iopc_inv = (float)1. - iopc;
	    
	ipixel->rclrflt += acc_rclr * iopc_inv;
	ipixel->gclrflt += acc_gclr * iopc_inv;
	ipixel->bclrflt += acc_bclr * iopc_inv;
	    iopc += acc_opc * iopc_inv;
	    ipixel->opcflt = iopc;
	    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
#ifdef COMPUTE_SHADOW_BUFFER
	printf("{%3d}  %3d %3d", k, icount-i-count, j);
#else
	printf("[%3d]  %3d %3d", k, icount-i-count, j);
#endif
	printf("  %3.0f %3.0f %3.0f",trace_opcTL*255.,trace_rclrTL,wgtTL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBL*255.,trace_rclrBL,wgtBL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcTR*255.,trace_rclrTR,wgtTR*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBR*255.,trace_rclrBR,wgtBR*100.);
	printf("  %3.0f %3.0f\n", iopc*255., 
	       ipixel->rclrflt);
        
	printf("              ");
	printf("      %3.0f    ",trace_rsclrTL);
	printf("      %3.0f    ",trace_rsclrBL);
	printf("      %3.0f    ",trace_rsclrTR);
	printf("      %3.0f    ",trace_rsclrBR);
	printf("  %3.0f\n", shadow_pixel->opcflt * 255.);
        
	printf("              ");
	printf("      %3.0f    ",trace_gclrTL);
	printf("      %3.0f    ",trace_gclrBL);
	printf("      %3.0f    ",trace_gclrTR);
	printf("      %3.0f    ",trace_gclrBR);
	printf("      %3.0f\n", ipixel->gclrflt);
	printf("              ");
	printf("      %3.0f    ",trace_bclrTL);
	printf("      %3.0f    ",trace_bclrBL);
	printf("      %3.0f    ",trace_bclrTR);
	printf("      %3.0f    ",trace_bclrBR);
	printf("      %3.0f\n", ipixel->bclrflt);
    }
#endif /* DEBUG */
;
#           ifndef SKIP_ERT
	        if (iopc >= max_opacity) {
		    ASSERT(ipixel->lnk == 0);
		    ipixel->lnk = 1;
	        }
#           endif
	};
		    ipixel += 1; shadow_pixel += 1;
		    topRLEdata += 1 * voxel_istride;
		    ;
		    count--;
		    SET_VOXELS_LOADED;
		}
		break;
	    case ALL_ZERO__BOT_NONZERO:
		/* first pixel: only the bottom-right voxel contributes */
		
    bot_opc = opac_correct[ByteField(botRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = 0.; trace_opcBL = 0.; trace_opcTR = 0.; trace_opcBR = 0.;
	trace_rsclrTL=0.; trace_rsclrBL=0.; trace_rsclrTR=0.; trace_rsclrBR=0.;
	trace_rclrTL= 0.; trace_rclrBL= 0.; trace_rclrTR= 0.; trace_rclrBR= 0.;
	trace_gclrTL= 0.; trace_gclrBL= 0.; trace_gclrTR= 0.; trace_gclrBR= 0.;
	trace_bclrTL= 0.; trace_bclrBL= 0.; trace_bclrTR= 0.; trace_bclrBR= 0.;
    }
#endif
;
		
       acc_opc = bot_opc * wgtBR;
       
	    acc_rclr = (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_gclr = (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_bclr = (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBR = bot_opc;
        trace_rclrBR = bot_rclr;
                     trace_gclrBR = bot_gclr;
                     trace_bclrBR = bot_bclr;
	
    }
#endif
;
		
	COUNT_RESAMPLE;
	if (acc_opc > min_opacity) {
	    COUNT_COMPOSITE;
	    iopc = ipixel->opcflt;
#           ifndef SKIP_ERT
	        ASSERT(iopc < max_opacity);
#           endif
	    iopc_inv = (float)1. - iopc;
	    
	ipixel->rclrflt += acc_rclr * iopc_inv;
	ipixel->gclrflt += acc_gclr * iopc_inv;
	ipixel->bclrflt += acc_bclr * iopc_inv;
	    iopc += acc_opc * iopc_inv;
	    ipixel->opcflt = iopc;
	    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
#ifdef COMPUTE_SHADOW_BUFFER
	printf("{%3d}  %3d %3d", k, icount-i-count, j);
#else
	printf("[%3d]  %3d %3d", k, icount-i-count, j);
#endif
	printf("  %3.0f %3.0f %3.0f",trace_opcTL*255.,trace_rclrTL,wgtTL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBL*255.,trace_rclrBL,wgtBL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcTR*255.,trace_rclrTR,wgtTR*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBR*255.,trace_rclrBR,wgtBR*100.);
	printf("  %3.0f %3.0f\n", iopc*255., 
	       ipixel->rclrflt);
        
	printf("              ");
	printf("      %3.0f    ",trace_rsclrTL);
	printf("      %3.0f    ",trace_rsclrBL);
	printf("      %3.0f    ",trace_rsclrTR);
	printf("      %3.0f    ",trace_rsclrBR);
	printf("  %3.0f\n", shadow_pixel->opcflt * 255.);
        
	printf("              ");
	printf("      %3.0f    ",trace_gclrTL);
	printf("      %3.0f    ",trace_gclrBL);
	printf("      %3.0f    ",trace_gclrTR);
	printf("      %3.0f    ",trace_gclrBR);
	printf("      %3.0f\n", ipixel->gclrflt);
	printf("              ");
	printf("      %3.0f    ",trace_bclrTL);
	printf("      %3.0f    ",trace_bclrBL);
	printf("      %3.0f    ",trace_bclrTR);
	printf("      %3.0f    ",trace_bclrBR);
	printf("      %3.0f\n", ipixel->bclrflt);
    }
#endif /* DEBUG */
;
#           ifndef SKIP_ERT
	        if (iopc >= max_opacity) {
		    ASSERT(ipixel->lnk == 0);
		    ipixel->lnk = 1;
	        }
#           endif
	};	
		ipixel += 1; shadow_pixel += 1;
		;
		botRLEdata += 1 * voxel_istride;
		count--;
		SET_VOXELS_LOADED;

		/* do the rest of the pixels in this run;
		   bottom-left and bottom-right voxels contribute */
		while (count > 0) {
		    if (PIXEL_IS_OPAQUE(ipixel))
			break;
		    if (!voxels_loaded) {
			
    bot_opc = opac_correct[ByteField(botRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata - voxel_istride, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		    }
		    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = 0.; trace_opcBL = 0.; trace_opcTR = 0.; trace_opcBR = 0.;
	trace_rsclrTL=0.; trace_rsclrBL=0.; trace_rsclrTR=0.; trace_rsclrBR=0.;
	trace_rclrTL= 0.; trace_rclrBL= 0.; trace_rclrTR= 0.; trace_rclrBR= 0.;
	trace_gclrTL= 0.; trace_gclrBL= 0.; trace_gclrTR= 0.; trace_gclrBR= 0.;
	trace_bclrTL= 0.; trace_bclrBL= 0.; trace_bclrTR= 0.; trace_bclrBR= 0.;
    }
#endif
;
		    
       acc_opc = bot_opc * wgtBL;
       
	    acc_rclr = (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_gclr = (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_bclr = (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBL = bot_opc;
        trace_rclrBL = bot_rclr;
                     trace_gclrBL = bot_gclr;
                     trace_bclrBL = bot_bclr;
	
    }
#endif
;
		    
    bot_opc = opac_correct[ByteField(botRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		    
       acc_opc += bot_opc * wgtBR;
       
	    acc_rclr += (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_gclr += (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_bclr += (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBR = bot_opc;
        trace_rclrBR = bot_rclr;
                     trace_gclrBR = bot_gclr;
                     trace_bclrBR = bot_bclr;
	
    }
#endif
;
		    
	COUNT_RESAMPLE;
	if (acc_opc > min_opacity) {
	    COUNT_COMPOSITE;
	    iopc = ipixel->opcflt;
#           ifndef SKIP_ERT
	        ASSERT(iopc < max_opacity);
#           endif
	    iopc_inv = (float)1. - iopc;
	    
	ipixel->rclrflt += acc_rclr * iopc_inv;
	ipixel->gclrflt += acc_gclr * iopc_inv;
	ipixel->bclrflt += acc_bclr * iopc_inv;
	    iopc += acc_opc * iopc_inv;
	    ipixel->opcflt = iopc;
	    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
#ifdef COMPUTE_SHADOW_BUFFER
	printf("{%3d}  %3d %3d", k, icount-i-count, j);
#else
	printf("[%3d]  %3d %3d", k, icount-i-count, j);
#endif
	printf("  %3.0f %3.0f %3.0f",trace_opcTL*255.,trace_rclrTL,wgtTL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBL*255.,trace_rclrBL,wgtBL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcTR*255.,trace_rclrTR,wgtTR*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBR*255.,trace_rclrBR,wgtBR*100.);
	printf("  %3.0f %3.0f\n", iopc*255., 
	       ipixel->rclrflt);
        
	printf("              ");
	printf("      %3.0f    ",trace_rsclrTL);
	printf("      %3.0f    ",trace_rsclrBL);
	printf("      %3.0f    ",trace_rsclrTR);
	printf("      %3.0f    ",trace_rsclrBR);
	printf("  %3.0f\n", shadow_pixel->opcflt * 255.);
        
	printf("              ");
	printf("      %3.0f    ",trace_gclrTL);
	printf("      %3.0f    ",trace_gclrBL);
	printf("      %3.0f    ",trace_gclrTR);
	printf("      %3.0f    ",trace_gclrBR);
	printf("      %3.0f\n", ipixel->gclrflt);
	printf("              ");
	printf("      %3.0f    ",trace_bclrTL);
	printf("      %3.0f    ",trace_bclrBL);
	printf("      %3.0f    ",trace_bclrTR);
	printf("      %3.0f    ",trace_bclrBR);
	printf("      %3.0f\n", ipixel->bclrflt);
    }
#endif /* DEBUG */
;
#           ifndef SKIP_ERT
	        if (iopc >= max_opacity) {
		    ASSERT(ipixel->lnk == 0);
		    ipixel->lnk = 1;
	        }
#           endif
	};
		    ipixel += 1; shadow_pixel += 1;
		    ;
		    botRLEdata += 1 * voxel_istride;
		    count--;
		    SET_VOXELS_LOADED;
		}
		break;
	    case TOP_NONZERO__BOT_NONZERO:
		/* first pixel: top-left and bottom-right voxels contribute */
		if (!voxels_loaded) {
		    
    top_opc = opac_correct[ByteField(topRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata - voxel_istride, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
		}
		
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = 0.; trace_opcBL = 0.; trace_opcTR = 0.; trace_opcBR = 0.;
	trace_rsclrTL=0.; trace_rsclrBL=0.; trace_rsclrTR=0.; trace_rsclrBR=0.;
	trace_rclrTL= 0.; trace_rclrBL= 0.; trace_rclrTR= 0.; trace_rclrBR= 0.;
	trace_gclrTL= 0.; trace_gclrBL= 0.; trace_gclrTR= 0.; trace_gclrBR= 0.;
	trace_bclrTL= 0.; trace_bclrBL= 0.; trace_bclrTR= 0.; trace_bclrBR= 0.;
    }
#endif
;
		
       acc_opc = top_opc * wgtTL;
       
	    acc_rclr = (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_gclr = (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_bclr = (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = top_opc;
        trace_rclrTL = top_rclr;
                     trace_gclrTL = top_gclr;
                     trace_bclrTL = top_bclr;
	
    }
#endif
;
		
    bot_opc = opac_correct[ByteField(botRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		
       acc_opc += bot_opc * wgtBR;
       
	    acc_rclr += (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_gclr += (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_bclr += (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBR = bot_opc;
        trace_rclrBR = bot_rclr;
                     trace_gclrBR = bot_gclr;
                     trace_bclrBR = bot_bclr;
	
    }
#endif
;
		
	COUNT_RESAMPLE;
	if (acc_opc > min_opacity) {
	    COUNT_COMPOSITE;
	    iopc = ipixel->opcflt;
#           ifndef SKIP_ERT
	        ASSERT(iopc < max_opacity);
#           endif
	    iopc_inv = (float)1. - iopc;
	    
	ipixel->rclrflt += acc_rclr * iopc_inv;
	ipixel->gclrflt += acc_gclr * iopc_inv;
	ipixel->bclrflt += acc_bclr * iopc_inv;
	    iopc += acc_opc * iopc_inv;
	    ipixel->opcflt = iopc;
	    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
#ifdef COMPUTE_SHADOW_BUFFER
	printf("{%3d}  %3d %3d", k, icount-i-count, j);
#else
	printf("[%3d]  %3d %3d", k, icount-i-count, j);
#endif
	printf("  %3.0f %3.0f %3.0f",trace_opcTL*255.,trace_rclrTL,wgtTL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBL*255.,trace_rclrBL,wgtBL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcTR*255.,trace_rclrTR,wgtTR*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBR*255.,trace_rclrBR,wgtBR*100.);
	printf("  %3.0f %3.0f\n", iopc*255., 
	       ipixel->rclrflt);
        
	printf("              ");
	printf("      %3.0f    ",trace_rsclrTL);
	printf("      %3.0f    ",trace_rsclrBL);
	printf("      %3.0f    ",trace_rsclrTR);
	printf("      %3.0f    ",trace_rsclrBR);
	printf("  %3.0f\n", shadow_pixel->opcflt * 255.);
        
	printf("              ");
	printf("      %3.0f    ",trace_gclrTL);
	printf("      %3.0f    ",trace_gclrBL);
	printf("      %3.0f    ",trace_gclrTR);
	printf("      %3.0f    ",trace_gclrBR);
	printf("      %3.0f\n", ipixel->gclrflt);
	printf("              ");
	printf("      %3.0f    ",trace_bclrTL);
	printf("      %3.0f    ",trace_bclrBL);
	printf("      %3.0f    ",trace_bclrTR);
	printf("      %3.0f    ",trace_bclrBR);
	printf("      %3.0f\n", ipixel->bclrflt);
    }
#endif /* DEBUG */
;
#           ifndef SKIP_ERT
	        if (iopc >= max_opacity) {
		    ASSERT(ipixel->lnk == 0);
		    ipixel->lnk = 1;
	        }
#           endif
	};
		ipixel += 1; shadow_pixel += 1;
		;
		botRLEdata += 1 * voxel_istride;
		count--;
		SET_VOXELS_LOADED;

		/* do the rest of the pixels in this run;
		   bottom-left and bottom-right voxels contribute */
		while (count > 0) {
		    if (PIXEL_IS_OPAQUE(ipixel))
			break;
		    if (!voxels_loaded) {
			
    bot_opc = opac_correct[ByteField(botRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata - voxel_istride, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		    }
		    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = 0.; trace_opcBL = 0.; trace_opcTR = 0.; trace_opcBR = 0.;
	trace_rsclrTL=0.; trace_rsclrBL=0.; trace_rsclrTR=0.; trace_rsclrBR=0.;
	trace_rclrTL= 0.; trace_rclrBL= 0.; trace_rclrTR= 0.; trace_rclrBR= 0.;
	trace_gclrTL= 0.; trace_gclrBL= 0.; trace_gclrTR= 0.; trace_gclrBR= 0.;
	trace_bclrTL= 0.; trace_bclrBL= 0.; trace_bclrTR= 0.; trace_bclrBR= 0.;
    }
#endif
;
		    
       acc_opc = bot_opc * wgtBL;
       
	    acc_rclr = (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_gclr = (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_bclr = (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBL = bot_opc;
        trace_rclrBL = bot_rclr;
                     trace_gclrBL = bot_gclr;
                     trace_bclrBL = bot_bclr;
	
    }
#endif
;
		    
    bot_opc = opac_correct[ByteField(botRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		    
       acc_opc += bot_opc * wgtBR;
       
	    acc_rclr += (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_gclr += (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_bclr += (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBR = bot_opc;
        trace_rclrBR = bot_rclr;
                     trace_gclrBR = bot_gclr;
                     trace_bclrBR = bot_bclr;
	
    }
#endif
;
		    
	COUNT_RESAMPLE;
	if (acc_opc > min_opacity) {
	    COUNT_COMPOSITE;
	    iopc = ipixel->opcflt;
#           ifndef SKIP_ERT
	        ASSERT(iopc < max_opacity);
#           endif
	    iopc_inv = (float)1. - iopc;
	    
	ipixel->rclrflt += acc_rclr * iopc_inv;
	ipixel->gclrflt += acc_gclr * iopc_inv;
	ipixel->bclrflt += acc_bclr * iopc_inv;
	    iopc += acc_opc * iopc_inv;
	    ipixel->opcflt = iopc;
	    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
#ifdef COMPUTE_SHADOW_BUFFER
	printf("{%3d}  %3d %3d", k, icount-i-count, j);
#else
	printf("[%3d]  %3d %3d", k, icount-i-count, j);
#endif
	printf("  %3.0f %3.0f %3.0f",trace_opcTL*255.,trace_rclrTL,wgtTL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBL*255.,trace_rclrBL,wgtBL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcTR*255.,trace_rclrTR,wgtTR*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBR*255.,trace_rclrBR,wgtBR*100.);
	printf("  %3.0f %3.0f\n", iopc*255., 
	       ipixel->rclrflt);
        
	printf("              ");
	printf("      %3.0f    ",trace_rsclrTL);
	printf("      %3.0f    ",trace_rsclrBL);
	printf("      %3.0f    ",trace_rsclrTR);
	printf("      %3.0f    ",trace_rsclrBR);
	printf("  %3.0f\n", shadow_pixel->opcflt * 255.);
        
	printf("              ");
	printf("      %3.0f    ",trace_gclrTL);
	printf("      %3.0f    ",trace_gclrBL);
	printf("      %3.0f    ",trace_gclrTR);
	printf("      %3.0f    ",trace_gclrBR);
	printf("      %3.0f\n", ipixel->gclrflt);
	printf("              ");
	printf("      %3.0f    ",trace_bclrTL);
	printf("      %3.0f    ",trace_bclrBL);
	printf("      %3.0f    ",trace_bclrTR);
	printf("      %3.0f    ",trace_bclrBR);
	printf("      %3.0f\n", ipixel->bclrflt);
    }
#endif /* DEBUG */
;
#           ifndef SKIP_ERT
	        if (iopc >= max_opacity) {
		    ASSERT(ipixel->lnk == 0);
		    ipixel->lnk = 1;
	        }
#           endif
	};
		    ipixel += 1; shadow_pixel += 1;
		    ;
		    botRLEdata += 1 * voxel_istride;
		    count--;
		    SET_VOXELS_LOADED;
		}
		break;
	    case BOT_NONZERO__BOT_NONZERO:
		/* do the pixels in this run; bottom-left and
		   bottom-right voxels contribute */
		while (count > 0) {
		    if (PIXEL_IS_OPAQUE(ipixel))
			break;
		    if (!voxels_loaded) {
			
    bot_opc = opac_correct[ByteField(botRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata - voxel_istride, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		    }
		    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = 0.; trace_opcBL = 0.; trace_opcTR = 0.; trace_opcBR = 0.;
	trace_rsclrTL=0.; trace_rsclrBL=0.; trace_rsclrTR=0.; trace_rsclrBR=0.;
	trace_rclrTL= 0.; trace_rclrBL= 0.; trace_rclrTR= 0.; trace_rclrBR= 0.;
	trace_gclrTL= 0.; trace_gclrBL= 0.; trace_gclrTR= 0.; trace_gclrBR= 0.;
	trace_bclrTL= 0.; trace_bclrBL= 0.; trace_bclrTR= 0.; trace_bclrBR= 0.;
    }
#endif
;
		    
       acc_opc = bot_opc * wgtBL;
       
	    acc_rclr = (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_gclr = (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_bclr = (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBL = bot_opc;
        trace_rclrBL = bot_rclr;
                     trace_gclrBL = bot_gclr;
                     trace_bclrBL = bot_bclr;
	
    }
#endif
;
		    
    bot_opc = opac_correct[ByteField(botRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		    
       acc_opc += bot_opc * wgtBR;
       
	    acc_rclr += (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_gclr += (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_bclr += (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBR = bot_opc;
        trace_rclrBR = bot_rclr;
                     trace_gclrBR = bot_gclr;
                     trace_bclrBR = bot_bclr;
	
    }
#endif
;
		    
	COUNT_RESAMPLE;
	if (acc_opc > min_opacity) {
	    COUNT_COMPOSITE;
	    iopc = ipixel->opcflt;
#           ifndef SKIP_ERT
	        ASSERT(iopc < max_opacity);
#           endif
	    iopc_inv = (float)1. - iopc;
	    
	ipixel->rclrflt += acc_rclr * iopc_inv;
	ipixel->gclrflt += acc_gclr * iopc_inv;
	ipixel->bclrflt += acc_bclr * iopc_inv;
	    iopc += acc_opc * iopc_inv;
	    ipixel->opcflt = iopc;
	    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
#ifdef COMPUTE_SHADOW_BUFFER
	printf("{%3d}  %3d %3d", k, icount-i-count, j);
#else
	printf("[%3d]  %3d %3d", k, icount-i-count, j);
#endif
	printf("  %3.0f %3.0f %3.0f",trace_opcTL*255.,trace_rclrTL,wgtTL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBL*255.,trace_rclrBL,wgtBL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcTR*255.,trace_rclrTR,wgtTR*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBR*255.,trace_rclrBR,wgtBR*100.);
	printf("  %3.0f %3.0f\n", iopc*255., 
	       ipixel->rclrflt);
        
	printf("              ");
	printf("      %3.0f    ",trace_rsclrTL);
	printf("      %3.0f    ",trace_rsclrBL);
	printf("      %3.0f    ",trace_rsclrTR);
	printf("      %3.0f    ",trace_rsclrBR);
	printf("  %3.0f\n", shadow_pixel->opcflt * 255.);
        
	printf("              ");
	printf("      %3.0f    ",trace_gclrTL);
	printf("      %3.0f    ",trace_gclrBL);
	printf("      %3.0f    ",trace_gclrTR);
	printf("      %3.0f    ",trace_gclrBR);
	printf("      %3.0f\n", ipixel->gclrflt);
	printf("              ");
	printf("      %3.0f    ",trace_bclrTL);
	printf("      %3.0f    ",trace_bclrBL);
	printf("      %3.0f    ",trace_bclrTR);
	printf("      %3.0f    ",trace_bclrBR);
	printf("      %3.0f\n", ipixel->bclrflt);
    }
#endif /* DEBUG */
;
#           ifndef SKIP_ERT
	        if (iopc >= max_opacity) {
		    ASSERT(ipixel->lnk == 0);
		    ipixel->lnk = 1;
	        }
#           endif
	};
		    ipixel += 1; shadow_pixel += 1;
		    ;
		    botRLEdata += 1 * voxel_istride;
		    count--;
		    SET_VOXELS_LOADED;
		}
		break;
	    case ALL_NONZERO__BOT_NONZERO:
		/* first pixel: top-left, bottom-left and bottom-right
		   voxels contribute */
		if (!voxels_loaded) {
		    
    top_opc = opac_correct[ByteField(topRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata - voxel_istride, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
		    
    bot_opc = opac_correct[ByteField(botRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata - voxel_istride, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		}
		
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = 0.; trace_opcBL = 0.; trace_opcTR = 0.; trace_opcBR = 0.;
	trace_rsclrTL=0.; trace_rsclrBL=0.; trace_rsclrTR=0.; trace_rsclrBR=0.;
	trace_rclrTL= 0.; trace_rclrBL= 0.; trace_rclrTR= 0.; trace_rclrBR= 0.;
	trace_gclrTL= 0.; trace_gclrBL= 0.; trace_gclrTR= 0.; trace_gclrBR= 0.;
	trace_bclrTL= 0.; trace_bclrBL= 0.; trace_bclrTR= 0.; trace_bclrBR= 0.;
    }
#endif
;
		
       acc_opc = top_opc * wgtTL;
       
	    acc_rclr = (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_gclr = (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_bclr = (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = top_opc;
        trace_rclrTL = top_rclr;
                     trace_gclrTL = top_gclr;
                     trace_bclrTL = top_bclr;
	
    }
#endif
;
		
       acc_opc += bot_opc * wgtBL;
       
	    acc_rclr += (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_gclr += (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_bclr += (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBL = bot_opc;
        trace_rclrBL = bot_rclr;
                     trace_gclrBL = bot_gclr;
                     trace_bclrBL = bot_bclr;
	
    }
#endif
;
		
    bot_opc = opac_correct[ByteField(botRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		
       acc_opc += bot_opc * wgtBR;
       
	    acc_rclr += (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_gclr += (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_bclr += (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBR = bot_opc;
        trace_rclrBR = bot_rclr;
                     trace_gclrBR = bot_gclr;
                     trace_bclrBR = bot_bclr;
	
    }
#endif
;
		
	COUNT_RESAMPLE;
	if (acc_opc > min_opacity) {
	    COUNT_COMPOSITE;
	    iopc = ipixel->opcflt;
#           ifndef SKIP_ERT
	        ASSERT(iopc < max_opacity);
#           endif
	    iopc_inv = (float)1. - iopc;
	    
	ipixel->rclrflt += acc_rclr * iopc_inv;
	ipixel->gclrflt += acc_gclr * iopc_inv;
	ipixel->bclrflt += acc_bclr * iopc_inv;
	    iopc += acc_opc * iopc_inv;
	    ipixel->opcflt = iopc;
	    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
#ifdef COMPUTE_SHADOW_BUFFER
	printf("{%3d}  %3d %3d", k, icount-i-count, j);
#else
	printf("[%3d]  %3d %3d", k, icount-i-count, j);
#endif
	printf("  %3.0f %3.0f %3.0f",trace_opcTL*255.,trace_rclrTL,wgtTL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBL*255.,trace_rclrBL,wgtBL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcTR*255.,trace_rclrTR,wgtTR*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBR*255.,trace_rclrBR,wgtBR*100.);
	printf("  %3.0f %3.0f\n", iopc*255., 
	       ipixel->rclrflt);
        
	printf("              ");
	printf("      %3.0f    ",trace_rsclrTL);
	printf("      %3.0f    ",trace_rsclrBL);
	printf("      %3.0f    ",trace_rsclrTR);
	printf("      %3.0f    ",trace_rsclrBR);
	printf("  %3.0f\n", shadow_pixel->opcflt * 255.);
        
	printf("              ");
	printf("      %3.0f    ",trace_gclrTL);
	printf("      %3.0f    ",trace_gclrBL);
	printf("      %3.0f    ",trace_gclrTR);
	printf("      %3.0f    ",trace_gclrBR);
	printf("      %3.0f\n", ipixel->gclrflt);
	printf("              ");
	printf("      %3.0f    ",trace_bclrTL);
	printf("      %3.0f    ",trace_bclrBL);
	printf("      %3.0f    ",trace_bclrTR);
	printf("      %3.0f    ",trace_bclrBR);
	printf("      %3.0f\n", ipixel->bclrflt);
    }
#endif /* DEBUG */
;
#           ifndef SKIP_ERT
	        if (iopc >= max_opacity) {
		    ASSERT(ipixel->lnk == 0);
		    ipixel->lnk = 1;
	        }
#           endif
	};
		ipixel += 1; shadow_pixel += 1;
		;
		botRLEdata += 1 * voxel_istride;
		count--;
		SET_VOXELS_LOADED;

		/* do the rest of the pixels in this run;
		   bottom-left and bottom-right voxels contribute */
		while (count > 0) {
		    if (PIXEL_IS_OPAQUE(ipixel))
			break;
		    if (!voxels_loaded) {
			
    bot_opc = opac_correct[ByteField(botRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata - voxel_istride, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		    }
		    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = 0.; trace_opcBL = 0.; trace_opcTR = 0.; trace_opcBR = 0.;
	trace_rsclrTL=0.; trace_rsclrBL=0.; trace_rsclrTR=0.; trace_rsclrBR=0.;
	trace_rclrTL= 0.; trace_rclrBL= 0.; trace_rclrTR= 0.; trace_rclrBR= 0.;
	trace_gclrTL= 0.; trace_gclrBL= 0.; trace_gclrTR= 0.; trace_gclrBR= 0.;
	trace_bclrTL= 0.; trace_bclrBL= 0.; trace_bclrTR= 0.; trace_bclrBR= 0.;
    }
#endif
;
		    
       acc_opc = bot_opc * wgtBL;
       
	    acc_rclr = (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_gclr = (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_bclr = (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBL = bot_opc;
        trace_rclrBL = bot_rclr;
                     trace_gclrBL = bot_gclr;
                     trace_bclrBL = bot_bclr;
	
    }
#endif
;
		    
    bot_opc = opac_correct[ByteField(botRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		    
       acc_opc += bot_opc * wgtBR;
       
	    acc_rclr += (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_gclr += (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_bclr += (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBR = bot_opc;
        trace_rclrBR = bot_rclr;
                     trace_gclrBR = bot_gclr;
                     trace_bclrBR = bot_bclr;
	
    }
#endif
;
		    
	COUNT_RESAMPLE;
	if (acc_opc > min_opacity) {
	    COUNT_COMPOSITE;
	    iopc = ipixel->opcflt;
#           ifndef SKIP_ERT
	        ASSERT(iopc < max_opacity);
#           endif
	    iopc_inv = (float)1. - iopc;
	    
	ipixel->rclrflt += acc_rclr * iopc_inv;
	ipixel->gclrflt += acc_gclr * iopc_inv;
	ipixel->bclrflt += acc_bclr * iopc_inv;
	    iopc += acc_opc * iopc_inv;
	    ipixel->opcflt = iopc;
	    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
#ifdef COMPUTE_SHADOW_BUFFER
	printf("{%3d}  %3d %3d", k, icount-i-count, j);
#else
	printf("[%3d]  %3d %3d", k, icount-i-count, j);
#endif
	printf("  %3.0f %3.0f %3.0f",trace_opcTL*255.,trace_rclrTL,wgtTL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBL*255.,trace_rclrBL,wgtBL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcTR*255.,trace_rclrTR,wgtTR*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBR*255.,trace_rclrBR,wgtBR*100.);
	printf("  %3.0f %3.0f\n", iopc*255., 
	       ipixel->rclrflt);
        
	printf("              ");
	printf("      %3.0f    ",trace_rsclrTL);
	printf("      %3.0f    ",trace_rsclrBL);
	printf("      %3.0f    ",trace_rsclrTR);
	printf("      %3.0f    ",trace_rsclrBR);
	printf("  %3.0f\n", shadow_pixel->opcflt * 255.);
        
	printf("              ");
	printf("      %3.0f    ",trace_gclrTL);
	printf("      %3.0f    ",trace_gclrBL);
	printf("      %3.0f    ",trace_gclrTR);
	printf("      %3.0f    ",trace_gclrBR);
	printf("      %3.0f\n", ipixel->gclrflt);
	printf("              ");
	printf("      %3.0f    ",trace_bclrTL);
	printf("      %3.0f    ",trace_bclrBL);
	printf("      %3.0f    ",trace_bclrTR);
	printf("      %3.0f    ",trace_bclrBR);
	printf("      %3.0f\n", ipixel->bclrflt);
    }
#endif /* DEBUG */
;
#           ifndef SKIP_ERT
	        if (iopc >= max_opacity) {
		    ASSERT(ipixel->lnk == 0);
		    ipixel->lnk = 1;
	        }
#           endif
	};
		    ipixel += 1; shadow_pixel += 1;
		    ;
		    botRLEdata += 1 * voxel_istride;
		    count--;
		    SET_VOXELS_LOADED;
		}
		break;
	    case ALL_ZERO__ALL_NONZERO:
		/* first pixel: top-right and bottom-right voxels contribute */
		
    top_opc = opac_correct[ByteField(topRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
		
    bot_opc = opac_correct[ByteField(botRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = 0.; trace_opcBL = 0.; trace_opcTR = 0.; trace_opcBR = 0.;
	trace_rsclrTL=0.; trace_rsclrBL=0.; trace_rsclrTR=0.; trace_rsclrBR=0.;
	trace_rclrTL= 0.; trace_rclrBL= 0.; trace_rclrTR= 0.; trace_rclrBR= 0.;
	trace_gclrTL= 0.; trace_gclrBL= 0.; trace_gclrTR= 0.; trace_gclrBR= 0.;
	trace_bclrTL= 0.; trace_bclrBL= 0.; trace_bclrTR= 0.; trace_bclrBR= 0.;
    }
#endif
;
		
       acc_opc = top_opc * wgtTR;
       
	    acc_rclr = (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_gclr = (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_bclr = (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTR = top_opc;
        trace_rclrTR = top_rclr;
                     trace_gclrTR = top_gclr;
                     trace_bclrTR = top_bclr;
	
    }
#endif
;
		
       acc_opc += bot_opc * wgtBR;
       
	    acc_rclr += (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_gclr += (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_bclr += (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBR = bot_opc;
        trace_rclrBR = bot_rclr;
                     trace_gclrBR = bot_gclr;
                     trace_bclrBR = bot_bclr;
	
    }
#endif
;
		
	COUNT_RESAMPLE;
	if (acc_opc > min_opacity) {
	    COUNT_COMPOSITE;
	    iopc = ipixel->opcflt;
#           ifndef SKIP_ERT
	        ASSERT(iopc < max_opacity);
#           endif
	    iopc_inv = (float)1. - iopc;
	    
	ipixel->rclrflt += acc_rclr * iopc_inv;
	ipixel->gclrflt += acc_gclr * iopc_inv;
	ipixel->bclrflt += acc_bclr * iopc_inv;
	    iopc += acc_opc * iopc_inv;
	    ipixel->opcflt = iopc;
	    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
#ifdef COMPUTE_SHADOW_BUFFER
	printf("{%3d}  %3d %3d", k, icount-i-count, j);
#else
	printf("[%3d]  %3d %3d", k, icount-i-count, j);
#endif
	printf("  %3.0f %3.0f %3.0f",trace_opcTL*255.,trace_rclrTL,wgtTL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBL*255.,trace_rclrBL,wgtBL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcTR*255.,trace_rclrTR,wgtTR*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBR*255.,trace_rclrBR,wgtBR*100.);
	printf("  %3.0f %3.0f\n", iopc*255., 
	       ipixel->rclrflt);
        
	printf("              ");
	printf("      %3.0f    ",trace_rsclrTL);
	printf("      %3.0f    ",trace_rsclrBL);
	printf("      %3.0f    ",trace_rsclrTR);
	printf("      %3.0f    ",trace_rsclrBR);
	printf("  %3.0f\n", shadow_pixel->opcflt * 255.);
        
	printf("              ");
	printf("      %3.0f    ",trace_gclrTL);
	printf("      %3.0f    ",trace_gclrBL);
	printf("      %3.0f    ",trace_gclrTR);
	printf("      %3.0f    ",trace_gclrBR);
	printf("      %3.0f\n", ipixel->gclrflt);
	printf("              ");
	printf("      %3.0f    ",trace_bclrTL);
	printf("      %3.0f    ",trace_bclrBL);
	printf("      %3.0f    ",trace_bclrTR);
	printf("      %3.0f    ",trace_bclrBR);
	printf("      %3.0f\n", ipixel->bclrflt);
    }
#endif /* DEBUG */
;
#           ifndef SKIP_ERT
	        if (iopc >= max_opacity) {
		    ASSERT(ipixel->lnk == 0);
		    ipixel->lnk = 1;
	        }
#           endif
	};
		ipixel += 1; shadow_pixel += 1;
		topRLEdata += 1 * voxel_istride;
		botRLEdata += 1 * voxel_istride;
		count--;
		SET_VOXELS_LOADED;

		/* do the rest of the pixels in this run;
		   all four voxels contribute */
		while (count > 0) {
		    if (PIXEL_IS_OPAQUE(ipixel))
			break;
		    if (!voxels_loaded) {
			
    top_opc = opac_correct[ByteField(topRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata - voxel_istride, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
			
    bot_opc = opac_correct[ByteField(botRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata - voxel_istride, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		    }
		    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = 0.; trace_opcBL = 0.; trace_opcTR = 0.; trace_opcBR = 0.;
	trace_rsclrTL=0.; trace_rsclrBL=0.; trace_rsclrTR=0.; trace_rsclrBR=0.;
	trace_rclrTL= 0.; trace_rclrBL= 0.; trace_rclrTR= 0.; trace_rclrBR= 0.;
	trace_gclrTL= 0.; trace_gclrBL= 0.; trace_gclrTR= 0.; trace_gclrBR= 0.;
	trace_bclrTL= 0.; trace_bclrBL= 0.; trace_bclrTR= 0.; trace_bclrBR= 0.;
    }
#endif
;
		    
       acc_opc = top_opc * wgtTL;
       
	    acc_rclr = (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_gclr = (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_bclr = (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = top_opc;
        trace_rclrTL = top_rclr;
                     trace_gclrTL = top_gclr;
                     trace_bclrTL = top_bclr;
	
    }
#endif
;
		    
       acc_opc += bot_opc * wgtBL;
       
	    acc_rclr += (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_gclr += (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_bclr += (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBL = bot_opc;
        trace_rclrBL = bot_rclr;
                     trace_gclrBL = bot_gclr;
                     trace_bclrBL = bot_bclr;
	
    }
#endif
;
		    
    top_opc = opac_correct[ByteField(topRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
		    
    bot_opc = opac_correct[ByteField(botRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		    
       acc_opc += top_opc * wgtTR;
       
	    acc_rclr += (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_gclr += (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_bclr += (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTR = top_opc;
        trace_rclrTR = top_rclr;
                     trace_gclrTR = top_gclr;
                     trace_bclrTR = top_bclr;
	
    }
#endif
;
		    
       acc_opc += bot_opc * wgtBR;
       
	    acc_rclr += (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_gclr += (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_bclr += (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBR = bot_opc;
        trace_rclrBR = bot_rclr;
                     trace_gclrBR = bot_gclr;
                     trace_bclrBR = bot_bclr;
	
    }
#endif
;
		    
	COUNT_RESAMPLE;
	if (acc_opc > min_opacity) {
	    COUNT_COMPOSITE;
	    iopc = ipixel->opcflt;
#           ifndef SKIP_ERT
	        ASSERT(iopc < max_opacity);
#           endif
	    iopc_inv = (float)1. - iopc;
	    
	ipixel->rclrflt += acc_rclr * iopc_inv;
	ipixel->gclrflt += acc_gclr * iopc_inv;
	ipixel->bclrflt += acc_bclr * iopc_inv;
	    iopc += acc_opc * iopc_inv;
	    ipixel->opcflt = iopc;
	    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
#ifdef COMPUTE_SHADOW_BUFFER
	printf("{%3d}  %3d %3d", k, icount-i-count, j);
#else
	printf("[%3d]  %3d %3d", k, icount-i-count, j);
#endif
	printf("  %3.0f %3.0f %3.0f",trace_opcTL*255.,trace_rclrTL,wgtTL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBL*255.,trace_rclrBL,wgtBL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcTR*255.,trace_rclrTR,wgtTR*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBR*255.,trace_rclrBR,wgtBR*100.);
	printf("  %3.0f %3.0f\n", iopc*255., 
	       ipixel->rclrflt);
        
	printf("              ");
	printf("      %3.0f    ",trace_rsclrTL);
	printf("      %3.0f    ",trace_rsclrBL);
	printf("      %3.0f    ",trace_rsclrTR);
	printf("      %3.0f    ",trace_rsclrBR);
	printf("  %3.0f\n", shadow_pixel->opcflt * 255.);
        
	printf("              ");
	printf("      %3.0f    ",trace_gclrTL);
	printf("      %3.0f    ",trace_gclrBL);
	printf("      %3.0f    ",trace_gclrTR);
	printf("      %3.0f    ",trace_gclrBR);
	printf("      %3.0f\n", ipixel->gclrflt);
	printf("              ");
	printf("      %3.0f    ",trace_bclrTL);
	printf("      %3.0f    ",trace_bclrBL);
	printf("      %3.0f    ",trace_bclrTR);
	printf("      %3.0f    ",trace_bclrBR);
	printf("      %3.0f\n", ipixel->bclrflt);
    }
#endif /* DEBUG */
;
#           ifndef SKIP_ERT
	        if (iopc >= max_opacity) {
		    ASSERT(ipixel->lnk == 0);
		    ipixel->lnk = 1;
	        }
#           endif
	};
		    ipixel += 1; shadow_pixel += 1;
		    topRLEdata += 1 * voxel_istride;
		    botRLEdata += 1 * voxel_istride;
		    count--;
		    SET_VOXELS_LOADED;
		}
		break;
	    case TOP_NONZERO__ALL_NONZERO:
		/* first pixel: top-left, top-right and bottom-right
		   voxels contribute */
		if (!voxels_loaded) {
		    
    top_opc = opac_correct[ByteField(topRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata - voxel_istride, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
		}
		
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = 0.; trace_opcBL = 0.; trace_opcTR = 0.; trace_opcBR = 0.;
	trace_rsclrTL=0.; trace_rsclrBL=0.; trace_rsclrTR=0.; trace_rsclrBR=0.;
	trace_rclrTL= 0.; trace_rclrBL= 0.; trace_rclrTR= 0.; trace_rclrBR= 0.;
	trace_gclrTL= 0.; trace_gclrBL= 0.; trace_gclrTR= 0.; trace_gclrBR= 0.;
	trace_bclrTL= 0.; trace_bclrBL= 0.; trace_bclrTR= 0.; trace_bclrBR= 0.;
    }
#endif
;
		
       acc_opc = top_opc * wgtTL;
       
	    acc_rclr = (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_gclr = (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_bclr = (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = top_opc;
        trace_rclrTL = top_rclr;
                     trace_gclrTL = top_gclr;
                     trace_bclrTL = top_bclr;
	
    }
#endif
;
		
    top_opc = opac_correct[ByteField(topRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
		
    bot_opc = opac_correct[ByteField(botRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		
       acc_opc += top_opc * wgtTR;
       
	    acc_rclr += (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_gclr += (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_bclr += (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTR = top_opc;
        trace_rclrTR = top_rclr;
                     trace_gclrTR = top_gclr;
                     trace_bclrTR = top_bclr;
	
    }
#endif
;
		
       acc_opc += bot_opc * wgtBR;
       
	    acc_rclr += (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_gclr += (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_bclr += (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBR = bot_opc;
        trace_rclrBR = bot_rclr;
                     trace_gclrBR = bot_gclr;
                     trace_bclrBR = bot_bclr;
	
    }
#endif
;
		
	COUNT_RESAMPLE;
	if (acc_opc > min_opacity) {
	    COUNT_COMPOSITE;
	    iopc = ipixel->opcflt;
#           ifndef SKIP_ERT
	        ASSERT(iopc < max_opacity);
#           endif
	    iopc_inv = (float)1. - iopc;
	    
	ipixel->rclrflt += acc_rclr * iopc_inv;
	ipixel->gclrflt += acc_gclr * iopc_inv;
	ipixel->bclrflt += acc_bclr * iopc_inv;
	    iopc += acc_opc * iopc_inv;
	    ipixel->opcflt = iopc;
	    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
#ifdef COMPUTE_SHADOW_BUFFER
	printf("{%3d}  %3d %3d", k, icount-i-count, j);
#else
	printf("[%3d]  %3d %3d", k, icount-i-count, j);
#endif
	printf("  %3.0f %3.0f %3.0f",trace_opcTL*255.,trace_rclrTL,wgtTL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBL*255.,trace_rclrBL,wgtBL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcTR*255.,trace_rclrTR,wgtTR*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBR*255.,trace_rclrBR,wgtBR*100.);
	printf("  %3.0f %3.0f\n", iopc*255., 
	       ipixel->rclrflt);
        
	printf("              ");
	printf("      %3.0f    ",trace_rsclrTL);
	printf("      %3.0f    ",trace_rsclrBL);
	printf("      %3.0f    ",trace_rsclrTR);
	printf("      %3.0f    ",trace_rsclrBR);
	printf("  %3.0f\n", shadow_pixel->opcflt * 255.);
        
	printf("              ");
	printf("      %3.0f    ",trace_gclrTL);
	printf("      %3.0f    ",trace_gclrBL);
	printf("      %3.0f    ",trace_gclrTR);
	printf("      %3.0f    ",trace_gclrBR);
	printf("      %3.0f\n", ipixel->gclrflt);
	printf("              ");
	printf("      %3.0f    ",trace_bclrTL);
	printf("      %3.0f    ",trace_bclrBL);
	printf("      %3.0f    ",trace_bclrTR);
	printf("      %3.0f    ",trace_bclrBR);
	printf("      %3.0f\n", ipixel->bclrflt);
    }
#endif /* DEBUG */
;
#           ifndef SKIP_ERT
	        if (iopc >= max_opacity) {
		    ASSERT(ipixel->lnk == 0);
		    ipixel->lnk = 1;
	        }
#           endif
	};
		ipixel += 1; shadow_pixel += 1;
		topRLEdata += 1 * voxel_istride;
		botRLEdata += 1 * voxel_istride;
		count--;
		SET_VOXELS_LOADED;
		    
		/* do the rest of the pixels in this run;
		   all four voxels contribute */
		while (count > 0) {
		    if (PIXEL_IS_OPAQUE(ipixel))
			break;
		    if (!voxels_loaded) {
			
    top_opc = opac_correct[ByteField(topRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata - voxel_istride, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
			
    bot_opc = opac_correct[ByteField(botRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata - voxel_istride, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		    }
		    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = 0.; trace_opcBL = 0.; trace_opcTR = 0.; trace_opcBR = 0.;
	trace_rsclrTL=0.; trace_rsclrBL=0.; trace_rsclrTR=0.; trace_rsclrBR=0.;
	trace_rclrTL= 0.; trace_rclrBL= 0.; trace_rclrTR= 0.; trace_rclrBR= 0.;
	trace_gclrTL= 0.; trace_gclrBL= 0.; trace_gclrTR= 0.; trace_gclrBR= 0.;
	trace_bclrTL= 0.; trace_bclrBL= 0.; trace_bclrTR= 0.; trace_bclrBR= 0.;
    }
#endif
;
		    
       acc_opc = top_opc * wgtTL;
       
	    acc_rclr = (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_gclr = (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_bclr = (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = top_opc;
        trace_rclrTL = top_rclr;
                     trace_gclrTL = top_gclr;
                     trace_bclrTL = top_bclr;
	
    }
#endif
;
		    
       acc_opc += bot_opc * wgtBL;
       
	    acc_rclr += (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_gclr += (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_bclr += (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBL = bot_opc;
        trace_rclrBL = bot_rclr;
                     trace_gclrBL = bot_gclr;
                     trace_bclrBL = bot_bclr;
	
    }
#endif
;
		    
    top_opc = opac_correct[ByteField(topRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
		    
    bot_opc = opac_correct[ByteField(botRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		    
       acc_opc += top_opc * wgtTR;
       
	    acc_rclr += (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_gclr += (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_bclr += (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTR = top_opc;
        trace_rclrTR = top_rclr;
                     trace_gclrTR = top_gclr;
                     trace_bclrTR = top_bclr;
	
    }
#endif
;
		    
       acc_opc += bot_opc * wgtBR;
       
	    acc_rclr += (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_gclr += (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_bclr += (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBR = bot_opc;
        trace_rclrBR = bot_rclr;
                     trace_gclrBR = bot_gclr;
                     trace_bclrBR = bot_bclr;
	
    }
#endif
;
		    
	COUNT_RESAMPLE;
	if (acc_opc > min_opacity) {
	    COUNT_COMPOSITE;
	    iopc = ipixel->opcflt;
#           ifndef SKIP_ERT
	        ASSERT(iopc < max_opacity);
#           endif
	    iopc_inv = (float)1. - iopc;
	    
	ipixel->rclrflt += acc_rclr * iopc_inv;
	ipixel->gclrflt += acc_gclr * iopc_inv;
	ipixel->bclrflt += acc_bclr * iopc_inv;
	    iopc += acc_opc * iopc_inv;
	    ipixel->opcflt = iopc;
	    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
#ifdef COMPUTE_SHADOW_BUFFER
	printf("{%3d}  %3d %3d", k, icount-i-count, j);
#else
	printf("[%3d]  %3d %3d", k, icount-i-count, j);
#endif
	printf("  %3.0f %3.0f %3.0f",trace_opcTL*255.,trace_rclrTL,wgtTL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBL*255.,trace_rclrBL,wgtBL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcTR*255.,trace_rclrTR,wgtTR*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBR*255.,trace_rclrBR,wgtBR*100.);
	printf("  %3.0f %3.0f\n", iopc*255., 
	       ipixel->rclrflt);
        
	printf("              ");
	printf("      %3.0f    ",trace_rsclrTL);
	printf("      %3.0f    ",trace_rsclrBL);
	printf("      %3.0f    ",trace_rsclrTR);
	printf("      %3.0f    ",trace_rsclrBR);
	printf("  %3.0f\n", shadow_pixel->opcflt * 255.);
        
	printf("              ");
	printf("      %3.0f    ",trace_gclrTL);
	printf("      %3.0f    ",trace_gclrBL);
	printf("      %3.0f    ",trace_gclrTR);
	printf("      %3.0f    ",trace_gclrBR);
	printf("      %3.0f\n", ipixel->gclrflt);
	printf("              ");
	printf("      %3.0f    ",trace_bclrTL);
	printf("      %3.0f    ",trace_bclrBL);
	printf("      %3.0f    ",trace_bclrTR);
	printf("      %3.0f    ",trace_bclrBR);
	printf("      %3.0f\n", ipixel->bclrflt);
    }
#endif /* DEBUG */
;
#           ifndef SKIP_ERT
	        if (iopc >= max_opacity) {
		    ASSERT(ipixel->lnk == 0);
		    ipixel->lnk = 1;
	        }
#           endif
	};
		    ipixel += 1; shadow_pixel += 1;
		    topRLEdata += 1 * voxel_istride;
		    botRLEdata += 1 * voxel_istride;
		    count--;
		    SET_VOXELS_LOADED;
		}
		break;
	    case BOT_NONZERO__ALL_NONZERO:
		/* first pixel: bottom-left, top-right and bottom-right
		   voxels contribute */
		if (!voxels_loaded) {
		    
    bot_opc = opac_correct[ByteField(botRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata - voxel_istride, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		}
		
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = 0.; trace_opcBL = 0.; trace_opcTR = 0.; trace_opcBR = 0.;
	trace_rsclrTL=0.; trace_rsclrBL=0.; trace_rsclrTR=0.; trace_rsclrBR=0.;
	trace_rclrTL= 0.; trace_rclrBL= 0.; trace_rclrTR= 0.; trace_rclrBR= 0.;
	trace_gclrTL= 0.; trace_gclrBL= 0.; trace_gclrTR= 0.; trace_gclrBR= 0.;
	trace_bclrTL= 0.; trace_bclrBL= 0.; trace_bclrTR= 0.; trace_bclrBR= 0.;
    }
#endif
;
		
       acc_opc = bot_opc * wgtBL;
       
	    acc_rclr = (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_gclr = (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_bclr = (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBL = bot_opc;
        trace_rclrBL = bot_rclr;
                     trace_gclrBL = bot_gclr;
                     trace_bclrBL = bot_bclr;
	
    }
#endif
;
		
    top_opc = opac_correct[ByteField(topRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
		
    bot_opc = opac_correct[ByteField(botRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		
       acc_opc += top_opc * wgtTR;
       
	    acc_rclr += (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_gclr += (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_bclr += (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTR = top_opc;
        trace_rclrTR = top_rclr;
                     trace_gclrTR = top_gclr;
                     trace_bclrTR = top_bclr;
	
    }
#endif
;
		
       acc_opc += bot_opc * wgtBR;
       
	    acc_rclr += (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_gclr += (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_bclr += (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBR = bot_opc;
        trace_rclrBR = bot_rclr;
                     trace_gclrBR = bot_gclr;
                     trace_bclrBR = bot_bclr;
	
    }
#endif
;
		
	COUNT_RESAMPLE;
	if (acc_opc > min_opacity) {
	    COUNT_COMPOSITE;
	    iopc = ipixel->opcflt;
#           ifndef SKIP_ERT
	        ASSERT(iopc < max_opacity);
#           endif
	    iopc_inv = (float)1. - iopc;
	    
	ipixel->rclrflt += acc_rclr * iopc_inv;
	ipixel->gclrflt += acc_gclr * iopc_inv;
	ipixel->bclrflt += acc_bclr * iopc_inv;
	    iopc += acc_opc * iopc_inv;
	    ipixel->opcflt = iopc;
	    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
#ifdef COMPUTE_SHADOW_BUFFER
	printf("{%3d}  %3d %3d", k, icount-i-count, j);
#else
	printf("[%3d]  %3d %3d", k, icount-i-count, j);
#endif
	printf("  %3.0f %3.0f %3.0f",trace_opcTL*255.,trace_rclrTL,wgtTL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBL*255.,trace_rclrBL,wgtBL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcTR*255.,trace_rclrTR,wgtTR*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBR*255.,trace_rclrBR,wgtBR*100.);
	printf("  %3.0f %3.0f\n", iopc*255., 
	       ipixel->rclrflt);
        
	printf("              ");
	printf("      %3.0f    ",trace_rsclrTL);
	printf("      %3.0f    ",trace_rsclrBL);
	printf("      %3.0f    ",trace_rsclrTR);
	printf("      %3.0f    ",trace_rsclrBR);
	printf("  %3.0f\n", shadow_pixel->opcflt * 255.);
        
	printf("              ");
	printf("      %3.0f    ",trace_gclrTL);
	printf("      %3.0f    ",trace_gclrBL);
	printf("      %3.0f    ",trace_gclrTR);
	printf("      %3.0f    ",trace_gclrBR);
	printf("      %3.0f\n", ipixel->gclrflt);
	printf("              ");
	printf("      %3.0f    ",trace_bclrTL);
	printf("      %3.0f    ",trace_bclrBL);
	printf("      %3.0f    ",trace_bclrTR);
	printf("      %3.0f    ",trace_bclrBR);
	printf("      %3.0f\n", ipixel->bclrflt);
    }
#endif /* DEBUG */
;
#           ifndef SKIP_ERT
	        if (iopc >= max_opacity) {
		    ASSERT(ipixel->lnk == 0);
		    ipixel->lnk = 1;
	        }
#           endif
	};
		ipixel += 1; shadow_pixel += 1;
		topRLEdata += 1 * voxel_istride;
		botRLEdata += 1 * voxel_istride;
		count--;
		SET_VOXELS_LOADED;
		    
		/* do the rest of the pixels in this run;
		   all four voxels contribute */
		while (count > 0) {
		    if (PIXEL_IS_OPAQUE(ipixel))
			break;
		    if (!voxels_loaded) {
			
    top_opc = opac_correct[ByteField(topRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata - voxel_istride, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
			
    bot_opc = opac_correct[ByteField(botRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata - voxel_istride, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		    }
		    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = 0.; trace_opcBL = 0.; trace_opcTR = 0.; trace_opcBR = 0.;
	trace_rsclrTL=0.; trace_rsclrBL=0.; trace_rsclrTR=0.; trace_rsclrBR=0.;
	trace_rclrTL= 0.; trace_rclrBL= 0.; trace_rclrTR= 0.; trace_rclrBR= 0.;
	trace_gclrTL= 0.; trace_gclrBL= 0.; trace_gclrTR= 0.; trace_gclrBR= 0.;
	trace_bclrTL= 0.; trace_bclrBL= 0.; trace_bclrTR= 0.; trace_bclrBR= 0.;
    }
#endif
;
		    
       acc_opc = top_opc * wgtTL;
       
	    acc_rclr = (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_gclr = (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_bclr = (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = top_opc;
        trace_rclrTL = top_rclr;
                     trace_gclrTL = top_gclr;
                     trace_bclrTL = top_bclr;
	
    }
#endif
;
		    
       acc_opc += bot_opc * wgtBL;
       
	    acc_rclr += (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_gclr += (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_bclr += (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBL = bot_opc;
        trace_rclrBL = bot_rclr;
                     trace_gclrBL = bot_gclr;
                     trace_bclrBL = bot_bclr;
	
    }
#endif
;
		    
    top_opc = opac_correct[ByteField(topRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
		    
    bot_opc = opac_correct[ByteField(botRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		    
       acc_opc += top_opc * wgtTR;
       
	    acc_rclr += (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_gclr += (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_bclr += (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTR = top_opc;
        trace_rclrTR = top_rclr;
                     trace_gclrTR = top_gclr;
                     trace_bclrTR = top_bclr;
	
    }
#endif
;
		    
       acc_opc += bot_opc * wgtBR;
       
	    acc_rclr += (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_gclr += (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_bclr += (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBR = bot_opc;
        trace_rclrBR = bot_rclr;
                     trace_gclrBR = bot_gclr;
                     trace_bclrBR = bot_bclr;
	
    }
#endif
;
		    
	COUNT_RESAMPLE;
	if (acc_opc > min_opacity) {
	    COUNT_COMPOSITE;
	    iopc = ipixel->opcflt;
#           ifndef SKIP_ERT
	        ASSERT(iopc < max_opacity);
#           endif
	    iopc_inv = (float)1. - iopc;
	    
	ipixel->rclrflt += acc_rclr * iopc_inv;
	ipixel->gclrflt += acc_gclr * iopc_inv;
	ipixel->bclrflt += acc_bclr * iopc_inv;
	    iopc += acc_opc * iopc_inv;
	    ipixel->opcflt = iopc;
	    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
#ifdef COMPUTE_SHADOW_BUFFER
	printf("{%3d}  %3d %3d", k, icount-i-count, j);
#else
	printf("[%3d]  %3d %3d", k, icount-i-count, j);
#endif
	printf("  %3.0f %3.0f %3.0f",trace_opcTL*255.,trace_rclrTL,wgtTL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBL*255.,trace_rclrBL,wgtBL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcTR*255.,trace_rclrTR,wgtTR*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBR*255.,trace_rclrBR,wgtBR*100.);
	printf("  %3.0f %3.0f\n", iopc*255., 
	       ipixel->rclrflt);
        
	printf("              ");
	printf("      %3.0f    ",trace_rsclrTL);
	printf("      %3.0f    ",trace_rsclrBL);
	printf("      %3.0f    ",trace_rsclrTR);
	printf("      %3.0f    ",trace_rsclrBR);
	printf("  %3.0f\n", shadow_pixel->opcflt * 255.);
        
	printf("              ");
	printf("      %3.0f    ",trace_gclrTL);
	printf("      %3.0f    ",trace_gclrBL);
	printf("      %3.0f    ",trace_gclrTR);
	printf("      %3.0f    ",trace_gclrBR);
	printf("      %3.0f\n", ipixel->gclrflt);
	printf("              ");
	printf("      %3.0f    ",trace_bclrTL);
	printf("      %3.0f    ",trace_bclrBL);
	printf("      %3.0f    ",trace_bclrTR);
	printf("      %3.0f    ",trace_bclrBR);
	printf("      %3.0f\n", ipixel->bclrflt);
    }
#endif /* DEBUG */
;
#           ifndef SKIP_ERT
	        if (iopc >= max_opacity) {
		    ASSERT(ipixel->lnk == 0);
		    ipixel->lnk = 1;
	        }
#           endif
	};
		    ipixel += 1; shadow_pixel += 1;
		    topRLEdata += 1 * voxel_istride;
		    botRLEdata += 1 * voxel_istride;
		    count--;
		    SET_VOXELS_LOADED;
		}
		break;
	    case ALL_NONZERO__ALL_NONZERO:
		/* do the pixels in this run; all four voxels contribute */
		while (count > 0) {
		    if (PIXEL_IS_OPAQUE(ipixel))
			break;
		    if (!voxels_loaded) {
			
    top_opc = opac_correct[ByteField(topRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata - voxel_istride, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
			
    bot_opc = opac_correct[ByteField(botRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata - voxel_istride, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		    }
		    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = 0.; trace_opcBL = 0.; trace_opcTR = 0.; trace_opcBR = 0.;
	trace_rsclrTL=0.; trace_rsclrBL=0.; trace_rsclrTR=0.; trace_rsclrBR=0.;
	trace_rclrTL= 0.; trace_rclrBL= 0.; trace_rclrTR= 0.; trace_rclrBR= 0.;
	trace_gclrTL= 0.; trace_gclrBL= 0.; trace_gclrTR= 0.; trace_gclrBR= 0.;
	trace_bclrTL= 0.; trace_bclrBL= 0.; trace_bclrTR= 0.; trace_bclrBR= 0.;
    }
#endif
;
		    
       acc_opc = top_opc * wgtTL;
       
	    acc_rclr = (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_gclr = (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_bclr = (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = top_opc;
        trace_rclrTL = top_rclr;
                     trace_gclrTL = top_gclr;
                     trace_bclrTL = top_bclr;
	
    }
#endif
;
		    
       acc_opc += bot_opc * wgtBL;
       
	    acc_rclr += (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_gclr += (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_bclr += (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBL = bot_opc;
        trace_rclrBL = bot_rclr;
                     trace_gclrBL = bot_gclr;
                     trace_bclrBL = bot_bclr;
	
    }
#endif
;
		    
    top_opc = opac_correct[ByteField(topRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
		    
    bot_opc = opac_correct[ByteField(botRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		    
       acc_opc += top_opc * wgtTR;
       
	    acc_rclr += (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_gclr += (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_bclr += (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTR = top_opc;
        trace_rclrTR = top_rclr;
                     trace_gclrTR = top_gclr;
                     trace_bclrTR = top_bclr;
	
    }
#endif
;
		    
       acc_opc += bot_opc * wgtBR;
       
	    acc_rclr += (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_gclr += (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_bclr += (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBR = bot_opc;
        trace_rclrBR = bot_rclr;
                     trace_gclrBR = bot_gclr;
                     trace_bclrBR = bot_bclr;
	
    }
#endif
;
		    
	COUNT_RESAMPLE;
	if (acc_opc > min_opacity) {
	    COUNT_COMPOSITE;
	    iopc = ipixel->opcflt;
#           ifndef SKIP_ERT
	        ASSERT(iopc < max_opacity);
#           endif
	    iopc_inv = (float)1. - iopc;
	    
	ipixel->rclrflt += acc_rclr * iopc_inv;
	ipixel->gclrflt += acc_gclr * iopc_inv;
	ipixel->bclrflt += acc_bclr * iopc_inv;
	    iopc += acc_opc * iopc_inv;
	    ipixel->opcflt = iopc;
	    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
#ifdef COMPUTE_SHADOW_BUFFER
	printf("{%3d}  %3d %3d", k, icount-i-count, j);
#else
	printf("[%3d]  %3d %3d", k, icount-i-count, j);
#endif
	printf("  %3.0f %3.0f %3.0f",trace_opcTL*255.,trace_rclrTL,wgtTL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBL*255.,trace_rclrBL,wgtBL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcTR*255.,trace_rclrTR,wgtTR*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBR*255.,trace_rclrBR,wgtBR*100.);
	printf("  %3.0f %3.0f\n", iopc*255., 
	       ipixel->rclrflt);
        
	printf("              ");
	printf("      %3.0f    ",trace_rsclrTL);
	printf("      %3.0f    ",trace_rsclrBL);
	printf("      %3.0f    ",trace_rsclrTR);
	printf("      %3.0f    ",trace_rsclrBR);
	printf("  %3.0f\n", shadow_pixel->opcflt * 255.);
        
	printf("              ");
	printf("      %3.0f    ",trace_gclrTL);
	printf("      %3.0f    ",trace_gclrBL);
	printf("      %3.0f    ",trace_gclrTR);
	printf("      %3.0f    ",trace_gclrBR);
	printf("      %3.0f\n", ipixel->gclrflt);
	printf("              ");
	printf("      %3.0f    ",trace_bclrTL);
	printf("      %3.0f    ",trace_bclrBL);
	printf("      %3.0f    ",trace_bclrTR);
	printf("      %3.0f    ",trace_bclrBR);
	printf("      %3.0f\n", ipixel->bclrflt);
    }
#endif /* DEBUG */
;
#           ifndef SKIP_ERT
	        if (iopc >= max_opacity) {
		    ASSERT(ipixel->lnk == 0);
		    ipixel->lnk = 1;
	        }
#           endif
	};
		    ipixel += 1; shadow_pixel += 1;
		    topRLEdata += 1 * voxel_istride;
		    botRLEdata += 1 * voxel_istride;
		    count--;
		    SET_VOXELS_LOADED;
		}
		break;
	    default:
		VPBug("illegal value for run states in compositing loop");
	    }
#else /* UNROLL_RUN_LOOP */
	    /* this run contains pixels, so process them */
	    while (count > 0) {
		if (last_run_state == ALL_ZERO && run_state == ALL_ZERO) {
		    ipixel += count; shadow_pixel += count;
		    if (i != -1) {
			;
			;
		    }
		    count = 0;
		    break;
		}
		if (ipixel->lnk != 0)
		    break;
		
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = 0.; trace_opcBL = 0.; trace_opcTR = 0.; trace_opcBR = 0.;
	trace_rsclrTL=0.; trace_rsclrBL=0.; trace_rsclrTR=0.; trace_rsclrBR=0.;
	trace_rclrTL= 0.; trace_rclrBL= 0.; trace_rclrTR= 0.; trace_rclrBR= 0.;
	trace_gclrTL= 0.; trace_gclrBL= 0.; trace_gclrTR= 0.; trace_gclrBR= 0.;
	trace_bclrTL= 0.; trace_bclrBL= 0.; trace_bclrTR= 0.; trace_bclrBR= 0.;
    }
#endif
;
		
       acc_opc = 0;
       acc_rclr = acc_gclr = acc_bclr = 0;
		if (last_run_state & TOP_NONZERO) {
		    if (!voxels_loaded) {
			
    top_opc = opac_correct[ByteField(topRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata - voxel_istride, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
		    }
		    
       acc_opc += top_opc * wgtTL;
       
	    acc_rclr += (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_gclr += (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_bclr += (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = top_opc;
        trace_rclrTL = top_rclr;
                     trace_gclrTL = top_gclr;
                     trace_bclrTL = top_bclr;
	
    }
#endif
;
		}
		if (last_run_state & BOT_NONZERO) {
		    if (!voxels_loaded) {
			
    bot_opc = opac_correct[ByteField(botRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata - voxel_istride, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		    }
		    
       acc_opc += bot_opc * wgtBL;
       
	    acc_rclr += (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_gclr += (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_bclr += (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBL = bot_opc;
        trace_rclrBL = bot_rclr;
                     trace_gclrBL = bot_gclr;
                     trace_bclrBL = bot_bclr;
	
    }
#endif
;
		}
		if (run_state & TOP_NONZERO) {
		    
    top_opc = opac_correct[ByteField(topRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
		    
       acc_opc += top_opc * wgtTR;
       
	    acc_rclr += (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_gclr += (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
	    acc_bclr += (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTR = top_opc;
        trace_rclrTR = top_rclr;
                     trace_gclrTR = top_gclr;
                     trace_bclrTR = top_bclr;
	
    }
#endif
;
		    topRLEdata += 1 * voxel_istride;
		} else {
		    if (i != -1) {
			;
		    }
		}
		if (run_state & BOT_NONZERO) {
		    
    bot_opc = opac_correct[ByteField(botRLEdata, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		    
       acc_opc += bot_opc * wgtBR;
       
	    acc_rclr += (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_gclr += (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
	    acc_bclr += (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBR;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBR = bot_opc;
        trace_rclrBR = bot_rclr;
                     trace_gclrBR = bot_gclr;
                     trace_bclrBR = bot_bclr;
	
    }
#endif
;
		    botRLEdata += 1 * voxel_istride;
		} else {
		    if (i != -1) {
			;
		    }
		}
		
	COUNT_RESAMPLE;
	if (acc_opc > min_opacity) {
	    COUNT_COMPOSITE;
	    iopc = ipixel->opcflt;
#           ifndef SKIP_ERT
	        ASSERT(iopc < max_opacity);
#           endif
	    iopc_inv = (float)1. - iopc;
	    
	ipixel->rclrflt += acc_rclr * iopc_inv;
	ipixel->gclrflt += acc_gclr * iopc_inv;
	ipixel->bclrflt += acc_bclr * iopc_inv;
	    iopc += acc_opc * iopc_inv;
	    ipixel->opcflt = iopc;
	    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
#ifdef COMPUTE_SHADOW_BUFFER
	printf("{%3d}  %3d %3d", k, icount-i-count, j);
#else
	printf("[%3d]  %3d %3d", k, icount-i-count, j);
#endif
	printf("  %3.0f %3.0f %3.0f",trace_opcTL*255.,trace_rclrTL,wgtTL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBL*255.,trace_rclrBL,wgtBL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcTR*255.,trace_rclrTR,wgtTR*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBR*255.,trace_rclrBR,wgtBR*100.);
	printf("  %3.0f %3.0f\n", iopc*255., 
	       ipixel->rclrflt);
        
	printf("              ");
	printf("      %3.0f    ",trace_rsclrTL);
	printf("      %3.0f    ",trace_rsclrBL);
	printf("      %3.0f    ",trace_rsclrTR);
	printf("      %3.0f    ",trace_rsclrBR);
	printf("  %3.0f\n", shadow_pixel->opcflt * 255.);
        
	printf("              ");
	printf("      %3.0f    ",trace_gclrTL);
	printf("      %3.0f    ",trace_gclrBL);
	printf("      %3.0f    ",trace_gclrTR);
	printf("      %3.0f    ",trace_gclrBR);
	printf("      %3.0f\n", ipixel->gclrflt);
	printf("              ");
	printf("      %3.0f    ",trace_bclrTL);
	printf("      %3.0f    ",trace_bclrBL);
	printf("      %3.0f    ",trace_bclrTR);
	printf("      %3.0f    ",trace_bclrBR);
	printf("      %3.0f\n", ipixel->bclrflt);
    }
#endif /* DEBUG */
;
#           ifndef SKIP_ERT
	        if (iopc >= max_opacity) {
		    ASSERT(ipixel->lnk == 0);
		    ipixel->lnk = 1;
	        }
#           endif
	};
		ipixel += 1; shadow_pixel += 1;
		count--;
		SET_VOXELS_LOADED;
		last_run_state = run_state;
	    }
#endif /* UNROLL_RUN_LOOP */

	    GET_HIRES_TIME(vpc, t1);
	    STORE_HIRES_TIME(vpc, VPTIMER_PROCESS_VOXELS, t0, t1);
	    COPY_HIRES_TIME(t0, t1);

	    if (count > 0) {
		Debug((vpc, VPDEBUG_COMPOSITE, "Backup(%d)\n", count));
		toprun_count += count;
		botrun_count += count;
		i += count;
	    }
#endif /* SKIP_COMPOSITE */

	    /***********************************************************
	     * Go on to next voxel run.
	     ***********************************************************/

	    last_run_state = run_state;
	} /* while (i > 0) */

	/***************************************************************
	 * Finish processing voxel scanline and go on to next one.
	 ***************************************************************/

#ifdef UNROLL_RUN_LOOP
	ASSERT(i == 0);
#else
	ASSERT(i == -1);
#endif

#ifndef SKIP_COMPOSITE
#ifdef UNROLL_RUN_LOOP
	/* do the last pixel (to the right of the last voxel) */
	if (last_run_state != ALL_ZERO && !PIXEL_IS_OPAQUE(ipixel)) {
	    /* last voxels are nonzero and the pixel is not opaque yet
	       so there is work to be done */
	    Debug((vpc, VPDEBUG_COMPOSITE, "Run(1)End\n"));
	    switch (last_run_state) {
	    case TOP_NONZERO:
		/* only the top-left voxel contributes */
		if (!voxels_loaded) {
		    
    top_opc = opac_correct[ByteField(topRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata - voxel_istride, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
		}
		
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = 0.; trace_opcBL = 0.; trace_opcTR = 0.; trace_opcBR = 0.;
	trace_rsclrTL=0.; trace_rsclrBL=0.; trace_rsclrTR=0.; trace_rsclrBR=0.;
	trace_rclrTL= 0.; trace_rclrBL= 0.; trace_rclrTR= 0.; trace_rclrBR= 0.;
	trace_gclrTL= 0.; trace_gclrBL= 0.; trace_gclrTR= 0.; trace_gclrBR= 0.;
	trace_bclrTL= 0.; trace_bclrBL= 0.; trace_bclrTR= 0.; trace_bclrBR= 0.;
    }
#endif
;
		
       acc_opc = top_opc * wgtTL;
       
	    acc_rclr = (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_gclr = (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_bclr = (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = top_opc;
        trace_rclrTL = top_rclr;
                     trace_gclrTL = top_gclr;
                     trace_bclrTL = top_bclr;
	
    }
#endif
;
		
	COUNT_RESAMPLE;
	if (acc_opc > min_opacity) {
	    COUNT_COMPOSITE;
	    iopc = ipixel->opcflt;
#           ifndef SKIP_ERT
	        ASSERT(iopc < max_opacity);
#           endif
	    iopc_inv = (float)1. - iopc;
	    
	ipixel->rclrflt += acc_rclr * iopc_inv;
	ipixel->gclrflt += acc_gclr * iopc_inv;
	ipixel->bclrflt += acc_bclr * iopc_inv;
	    iopc += acc_opc * iopc_inv;
	    ipixel->opcflt = iopc;
	    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
#ifdef COMPUTE_SHADOW_BUFFER
	printf("{%3d}  %3d %3d", k, icount-i-count, j);
#else
	printf("[%3d]  %3d %3d", k, icount-i-count, j);
#endif
	printf("  %3.0f %3.0f %3.0f",trace_opcTL*255.,trace_rclrTL,wgtTL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBL*255.,trace_rclrBL,wgtBL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcTR*255.,trace_rclrTR,wgtTR*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBR*255.,trace_rclrBR,wgtBR*100.);
	printf("  %3.0f %3.0f\n", iopc*255., 
	       ipixel->rclrflt);
        
	printf("              ");
	printf("      %3.0f    ",trace_rsclrTL);
	printf("      %3.0f    ",trace_rsclrBL);
	printf("      %3.0f    ",trace_rsclrTR);
	printf("      %3.0f    ",trace_rsclrBR);
	printf("  %3.0f\n", shadow_pixel->opcflt * 255.);
        
	printf("              ");
	printf("      %3.0f    ",trace_gclrTL);
	printf("      %3.0f    ",trace_gclrBL);
	printf("      %3.0f    ",trace_gclrTR);
	printf("      %3.0f    ",trace_gclrBR);
	printf("      %3.0f\n", ipixel->gclrflt);
	printf("              ");
	printf("      %3.0f    ",trace_bclrTL);
	printf("      %3.0f    ",trace_bclrBL);
	printf("      %3.0f    ",trace_bclrTR);
	printf("      %3.0f    ",trace_bclrBR);
	printf("      %3.0f\n", ipixel->bclrflt);
    }
#endif /* DEBUG */
;
#           ifndef SKIP_ERT
	        if (iopc >= max_opacity) {
		    ASSERT(ipixel->lnk == 0);
		    ipixel->lnk = 1;
	        }
#           endif
	};
		break;
	    case BOT_NONZERO:
		/* only the bottom left voxel contributes */
		if (!voxels_loaded) {
		    
    bot_opc = opac_correct[ByteField(botRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata - voxel_istride, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		}
		
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = 0.; trace_opcBL = 0.; trace_opcTR = 0.; trace_opcBR = 0.;
	trace_rsclrTL=0.; trace_rsclrBL=0.; trace_rsclrTR=0.; trace_rsclrBR=0.;
	trace_rclrTL= 0.; trace_rclrBL= 0.; trace_rclrTR= 0.; trace_rclrBR= 0.;
	trace_gclrTL= 0.; trace_gclrBL= 0.; trace_gclrTR= 0.; trace_gclrBR= 0.;
	trace_bclrTL= 0.; trace_bclrBL= 0.; trace_bclrTR= 0.; trace_bclrBR= 0.;
    }
#endif
;
		
       acc_opc = bot_opc * wgtBL;
       
	    acc_rclr = (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_gclr = (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_bclr = (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBL = bot_opc;
        trace_rclrBL = bot_rclr;
                     trace_gclrBL = bot_gclr;
                     trace_bclrBL = bot_bclr;
	
    }
#endif
;
		
	COUNT_RESAMPLE;
	if (acc_opc > min_opacity) {
	    COUNT_COMPOSITE;
	    iopc = ipixel->opcflt;
#           ifndef SKIP_ERT
	        ASSERT(iopc < max_opacity);
#           endif
	    iopc_inv = (float)1. - iopc;
	    
	ipixel->rclrflt += acc_rclr * iopc_inv;
	ipixel->gclrflt += acc_gclr * iopc_inv;
	ipixel->bclrflt += acc_bclr * iopc_inv;
	    iopc += acc_opc * iopc_inv;
	    ipixel->opcflt = iopc;
	    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
#ifdef COMPUTE_SHADOW_BUFFER
	printf("{%3d}  %3d %3d", k, icount-i-count, j);
#else
	printf("[%3d]  %3d %3d", k, icount-i-count, j);
#endif
	printf("  %3.0f %3.0f %3.0f",trace_opcTL*255.,trace_rclrTL,wgtTL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBL*255.,trace_rclrBL,wgtBL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcTR*255.,trace_rclrTR,wgtTR*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBR*255.,trace_rclrBR,wgtBR*100.);
	printf("  %3.0f %3.0f\n", iopc*255., 
	       ipixel->rclrflt);
        
	printf("              ");
	printf("      %3.0f    ",trace_rsclrTL);
	printf("      %3.0f    ",trace_rsclrBL);
	printf("      %3.0f    ",trace_rsclrTR);
	printf("      %3.0f    ",trace_rsclrBR);
	printf("  %3.0f\n", shadow_pixel->opcflt * 255.);
        
	printf("              ");
	printf("      %3.0f    ",trace_gclrTL);
	printf("      %3.0f    ",trace_gclrBL);
	printf("      %3.0f    ",trace_gclrTR);
	printf("      %3.0f    ",trace_gclrBR);
	printf("      %3.0f\n", ipixel->gclrflt);
	printf("              ");
	printf("      %3.0f    ",trace_bclrTL);
	printf("      %3.0f    ",trace_bclrBL);
	printf("      %3.0f    ",trace_bclrTR);
	printf("      %3.0f    ",trace_bclrBR);
	printf("      %3.0f\n", ipixel->bclrflt);
    }
#endif /* DEBUG */
;
#           ifndef SKIP_ERT
	        if (iopc >= max_opacity) {
		    ASSERT(ipixel->lnk == 0);
		    ipixel->lnk = 1;
	        }
#           endif
	};
		break;
	    case ALL_NONZERO:
		/* the top and bottom left voxels contribute */
		if (!voxels_loaded) {
		    
    top_opc = opac_correct[ByteField(topRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(topRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(topRLEdata - voxel_istride, wgt_offset);
    
        
	
    top_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    top_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        top_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    top_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    top_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    top_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        top_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        top_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        top_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = top_opc * slice_depth_cueing;
    
        top_rclr *= shade_factor;
	top_gclr *= shade_factor;
	top_bclr *= shade_factor;
    
            top_rsclr *= shade_factor;
	    top_gsclr *= shade_factor;
	    top_bsclr *= shade_factor;
		    
    bot_opc = opac_correct[ByteField(botRLEdata - voxel_istride, voxel_istride-1)];
    
    
    shade_index=num_materials*3*ShortField(botRLEdata - voxel_istride,norm_offset);
    weight_index = num_materials * ByteField(botRLEdata - voxel_istride, wgt_offset);
    
        
	
    bot_rclr = 
    shade_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_gclr = 
    shade_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
    bot_bclr = 
    shade_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_rsclr = 
    shadow_table[shade_index + 3*0 + 0] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_gsclr = 
    shadow_table[shade_index + 3*0 + 1] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
	
        bot_bsclr = 
    shadow_table[shade_index + 3*0 + 2] * 
	((num_materials > 1) ? weight_table[weight_index] : (float)1.0);
        for (m = 1; m < num_materials; m++) {
	     
	
    bot_rclr += 
    shade_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
    bot_gclr += 
    shade_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
    bot_bclr += 
    shade_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	
        bot_rsclr += 
    shadow_table[shade_index + 3*m + 0] * 
	weight_table[weight_index + m];
	
        bot_gsclr += 
    shadow_table[shade_index + 3*m + 1] * 
	weight_table[weight_index + m];
	
        bot_bsclr += 
    shadow_table[shade_index + 3*m + 2] * 
	weight_table[weight_index + m];
	};
    shade_factor = bot_opc * slice_depth_cueing;
    
        bot_rclr *= shade_factor;
	bot_gclr *= shade_factor;
	bot_bclr *= shade_factor;
    
            bot_rsclr *= shade_factor;
	    bot_gsclr *= shade_factor;
	    bot_bsclr *= shade_factor;
		}
		
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = 0.; trace_opcBL = 0.; trace_opcTR = 0.; trace_opcBR = 0.;
	trace_rsclrTL=0.; trace_rsclrBL=0.; trace_rsclrTR=0.; trace_rsclrBR=0.;
	trace_rclrTL= 0.; trace_rclrBL= 0.; trace_rclrTR= 0.; trace_rclrBR= 0.;
	trace_gclrTL= 0.; trace_gclrBL= 0.; trace_gclrTR= 0.; trace_gclrBR= 0.;
	trace_bclrTL= 0.; trace_bclrBL= 0.; trace_bclrTR= 0.; trace_bclrBR= 0.;
    }
#endif
;
		
       acc_opc = top_opc * wgtTL;
       
	    acc_rclr = (top_rclr + top_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_gclr = (top_gclr + top_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
	    acc_bclr = (top_bclr + top_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtTL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcTL = top_opc;
        trace_rclrTL = top_rclr;
                     trace_gclrTL = top_gclr;
                     trace_bclrTL = top_bclr;
	
    }
#endif
;
		
       acc_opc += bot_opc * wgtBL;
       
	    acc_rclr += (bot_rclr + bot_rsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_gclr += (bot_gclr + bot_gsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
	    acc_bclr += (bot_bclr + bot_bsclr *
			 ((float)1.0 - shadow_pixel->opcflt)) * wgtBL;
       
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
	trace_opcBL = bot_opc;
        trace_rclrBL = bot_rclr;
                     trace_gclrBL = bot_gclr;
                     trace_bclrBL = bot_bclr;
	
    }
#endif
;
		
	COUNT_RESAMPLE;
	if (acc_opc > min_opacity) {
	    COUNT_COMPOSITE;
	    iopc = ipixel->opcflt;
#           ifndef SKIP_ERT
	        ASSERT(iopc < max_opacity);
#           endif
	    iopc_inv = (float)1. - iopc;
	    
	ipixel->rclrflt += acc_rclr * iopc_inv;
	ipixel->gclrflt += acc_gclr * iopc_inv;
	ipixel->bclrflt += acc_bclr * iopc_inv;
	    iopc += acc_opc * iopc_inv;
	    ipixel->opcflt = iopc;
	    
#ifdef DEBUG
    if (ipixel == trace_pixel_ptr) {
#ifdef COMPUTE_SHADOW_BUFFER
	printf("{%3d}  %3d %3d", k, icount-i-count, j);
#else
	printf("[%3d]  %3d %3d", k, icount-i-count, j);
#endif
	printf("  %3.0f %3.0f %3.0f",trace_opcTL*255.,trace_rclrTL,wgtTL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBL*255.,trace_rclrBL,wgtBL*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcTR*255.,trace_rclrTR,wgtTR*100.);
	printf("  %3.0f %3.0f %3.0f",trace_opcBR*255.,trace_rclrBR,wgtBR*100.);
	printf("  %3.0f %3.0f\n", iopc*255., 
	       ipixel->rclrflt);
        
	printf("              ");
	printf("      %3.0f    ",trace_rsclrTL);
	printf("      %3.0f    ",trace_rsclrBL);
	printf("      %3.0f    ",trace_rsclrTR);
	printf("      %3.0f    ",trace_rsclrBR);
	printf("  %3.0f\n", shadow_pixel->opcflt * 255.);
        
	printf("              ");
	printf("      %3.0f    ",trace_gclrTL);
	printf("      %3.0f    ",trace_gclrBL);
	printf("      %3.0f    ",trace_gclrTR);
	printf("      %3.0f    ",trace_gclrBR);
	printf("      %3.0f\n", ipixel->gclrflt);
	printf("              ");
	printf("      %3.0f    ",trace_bclrTL);
	printf("      %3.0f    ",trace_bclrBL);
	printf("      %3.0f    ",trace_bclrTR);
	printf("      %3.0f    ",trace_bclrBR);
	printf("      %3.0f\n", ipixel->bclrflt);
    }
#endif /* DEBUG */
;
#           ifndef SKIP_ERT
	        if (iopc >= max_opacity) {
		    ASSERT(ipixel->lnk == 0);
		    ipixel->lnk = 1;
	        }
#           endif
	};
		break;
	    default:
		VPBug("illegal value for run state at end of scanline");
	    }
	} else if (last_run_state == ALL_ZERO) {
	    Debug((vpc, VPDEBUG_COMPOSITE, "ZeroSkip(1)End\n"));
	} else {
	    Debug((vpc, VPDEBUG_COMPOSITE, "ERTSkip(1)End\n"));
	}
#endif /* UNROLL_RUN_LOOP */
#endif /* SKIP_COMPOSITE */

#ifndef UNROLL_RUN_LOOP
	run_state = final_run_state;
#endif
	/* skip over any zero-length runs remaining in this scanline */
	if (j != 0 && ((run_state & 1) == 0)) {
	    toprun_count = *topRLElen++;
	    ASSERT(toprun_count == 0);
	}
	if (j != jcount && ((run_state & 2) == 0)) {
	    botrun_count = *botRLElen++;
	    ASSERT(botrun_count == 0);
	}

	/* go to next intermediate image scanline */
#ifdef UNROLL_RUN_LOOP
	ipixel += intermediate_width - icount;
#ifdef USE_SHADOW_BUFFER
	shadow_pixel += shadow_width - icount;
#endif
#else /* UNROLL_RUN_LOOP */
	ipixel += intermediate_width - (icount+1);
#ifdef USE_SHADOW_BUFFER
	shadow_pixel += shadow_width - (icount+1);
#endif
#endif /* UNROLL_RUN_LOOP */

	Debug((vpc, VPDEBUG_COMPOSITE, "ScanDone\n"));
    } /* for j */

    /***************************************************************
     * Finish processing the voxel slice.
     ***************************************************************/

    GET_HIRES_TIME(vpc, t1);
    STORE_HIRES_TIME(vpc, VPTIMER_TRAVERSE_RUNS, t0, t1);

    Debug((vpc, VPDEBUG_COMPOSITE, "SliceDone\n"));
}

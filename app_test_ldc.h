/*
 *
 * Copyright (c) 2018 Texas Instruments Incorporated
 *
 * All rights reserved not granted herein.
 *
 * Limited License.
 *
 * Texas Instruments Incorporated grants a world-wide, royalty-free, non-exclusive
 * license under copyrights and patents it now or hereafter owns or controls to make,
 * have made, use, import, offer to sell and sell ("Utilize") this software subject to the
 * terms herein.  With respect to the foregoing patent license, such license is granted
 * solely to the extent that any such patent is necessary to Utilize the software alone.
 * The patent license shall not apply to any combinations which include this software,
 * other than combinations with devices manufactured by or for TI ("TI Devices").
 * No hardware patent is licensed hereunder.
 *
 * Redistributions must preserve existing copyright notices and reproduce this license
 * (including the above copyright notice and the disclaimer and (if applicable) source
 * code license limitations below) in the documentation and/or other materials provided
 * with the distribution
 *
 * Redistribution and use in binary form, without modification, are permitted provided
 * that the following conditions are met:
 *
 * *       No reverse engineering, decompilation, or disassembly of this software is
 * permitted with respect to any software provided in binary form.
 *
 * *       any redistribution and use are licensed by TI for use only with TI Devices.
 *
 * *       Nothing shall obligate TI to provide you with source code for the software
 * licensed and provided to you in object code.
 *
 * If software source code is provided to you, modification and redistribution of the
 * source code are permitted provided that the following conditions are met:
 *
 * *       any redistribution and use of the source code, including any resulting derivative
 * works, are licensed by TI for use only with TI Devices.
 *
 * *       any redistribution and use of any object code compiled from the source code
 * and any resulting derivative works, are licensed by TI for use only with TI Devices.
 *
 * Neither the name of Texas Instruments Incorporated nor the names of its suppliers
 *
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * DISCLAIMER.
 *
 * THIS SOFTWARE IS PROVIDED BY TI AND TI'S LICENSORS "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TI AND TI'S LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


        // if (NULL != out1_img)
        // {
        //     if( ((vx_df_image)VX_DF_IMAGE_NV12 != out1_img_fmt) &&
        //         ((vx_df_image)VX_DF_IMAGE_UYVY != out1_img_fmt) &&
        //         ((vx_df_image)VX_DF_IMAGE_YUYV != out1_img_fmt) &&
        //         ((vx_df_image)VX_DF_IMAGE_U8 != out1_img_fmt))
        //     {
        //         status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
        //         VX_PRINT(VX_ZONE_ERROR, "'out1_img' should be an image of type:\n VX_DF_IMAGE_NV12 or VX_DF_IMAGE_UYVY or VX_DF_IMAGE_YUYV or VX_DF_IMAGE_U8 \n");
        //     }

        //     if ((((vx_df_image)VX_DF_IMAGE_UYVY == out1_img_fmt) ||
        //          ((vx_df_image)VX_DF_IMAGE_YUYV == out1_img_fmt)) &&
        //         ((vx_df_image)VX_DF_IMAGE_UYVY != in_img_fmt))
        //     {
        //         status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
        //         VX_PRINT(VX_ZONE_ERROR, "Output1 is YUV422I Input is not YUV422I \n");
        //     }

        //     if ((((vx_df_image)VX_DF_IMAGE_NV12 == out0_img_fmt) ||
        //          ((vx_df_image)TIVX_DF_IMAGE_NV12_P12 == out0_img_fmt) ||
        //          ((vx_df_image)VX_DF_IMAGE_UYVY == out1_img_fmt) ||
        //          ((vx_df_image)VX_DF_IMAGE_YUYV == out1_img_fmt)) &&
        //         ((vx_df_image)VX_DF_IMAGE_U8 != out1_img_fmt))
        //     {
        //         status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
        //         VX_PRINT(VX_ZONE_ERROR, "Output1 cannot be single plane if output0 is multi-plane \n");
        //     }

        //     if ((((vx_df_image)VX_DF_IMAGE_U8 == out0_img_fmt) ||
        //          ((vx_df_image)VX_DF_IMAGE_U16 == out0_img_fmt) ||
        //          ((vx_df_image)TIVX_DF_IMAGE_P12 == out0_img_fmt)) &&
        //          ((vx_df_image)VX_DF_IMAGE_NV12 == out1_img_fmt))
        //     {
        //         status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
        //         VX_PRINT(VX_ZONE_ERROR, "Output1 cannot be multi-plane if output0 is single-plane \n");
        //     }

        //     if ( (in_img_fmt == (vx_df_image)VX_DF_IMAGE_U8) &&
        //          !((out1_img_fmt == (vx_df_image)VX_DF_IMAGE_U8) || (out1_img_fmt == (vx_df_image)TIVX_DF_IMAGE_P12)))
        //     {
        //         status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
        //         VX_PRINT(VX_ZONE_ERROR, "'in_img' is VX_DF_IMAGE_U8 but 'out1_img' is not VX_DF_IMAGE_U8 or TIVX_DF_IMAGE_P12 \n");
        //     }

        //     if ( (in_img_fmt == (vx_df_image)VX_DF_IMAGE_U16) &&
        //          !((out1_img_fmt == (vx_df_image)VX_DF_IMAGE_U16) || (out1_img_fmt == (vx_df_image)VX_DF_IMAGE_U8)))
        //     {
        //         status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
        //         VX_PRINT(VX_ZONE_ERROR, "'in_img' is VX_DF_IMAGE_U16 but 'out1_img' is not VX_DF_IMAGE_U16 or VX_DF_IMAGE_U8 \n");
        //     }

        //     if ( (in_img_fmt == (vx_df_image)TIVX_DF_IMAGE_P12) &&
        //          !((out1_img_fmt == (vx_df_image)TIVX_DF_IMAGE_P12) || (out1_img_fmt == (vx_df_image)VX_DF_IMAGE_U8)))
        //     {
        //         status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
        //         VX_PRINT(VX_ZONE_ERROR, "'in_img' is TIVX_DF_IMAGE_P12 but 'out1_img' is not TIVX_DF_IMAGE_P12 or VX_DF_IMAGE_U8 \n");
        //     }

        //     if ( (in_img_fmt == (vx_df_image)VX_DF_IMAGE_NV12) &&
        //          !((out1_img_fmt == (vx_df_image)VX_DF_IMAGE_NV12) || (out1_img_fmt == (vx_df_image)TIVX_DF_IMAGE_NV12_P12)))
        //     {
        //         status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
        //         VX_PRINT(VX_ZONE_ERROR, "'in_img' is VX_DF_IMAGE_NV12 but 'out1_img' is not VX_DF_IMAGE_NV12 or TIVX_DF_IMAGE_NV12_P12 \n");
        //     }

        //     if ( (in_img_fmt == (vx_df_image)TIVX_DF_IMAGE_NV12_P12) &&
        //          !((out1_img_fmt == (vx_df_image)TIVX_DF_IMAGE_NV12_P12) || (out1_img_fmt == (vx_df_image)VX_DF_IMAGE_NV12)))
        //     {
        //         status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
        //         VX_PRINT(VX_ZONE_ERROR, "'in_img' is TIVX_DF_IMAGE_NV12_P12 but 'out1_img' is not TIVX_DF_IMAGE_NV12_P12 or VX_DF_IMAGE_NV12 \n");
        //     }

        //     if ( (in_img_fmt == (vx_df_image)VX_DF_IMAGE_UYVY) &&
        //          !((out1_img_fmt == (vx_df_image)VX_DF_IMAGE_UYVY) || (out1_img_fmt == (vx_df_image)VX_DF_IMAGE_YUYV) ||
        //            (out1_img_fmt == (vx_df_image)VX_DF_IMAGE_NV12) || (out1_img_fmt == (vx_df_image)TIVX_DF_IMAGE_NV12_P12) ))
        //     {
        //         status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
        //         VX_PRINT(VX_ZONE_ERROR, "'in_img' is VX_DF_IMAGE_UYVY but 'out1_img' is not VX_DF_IMAGE_UYVY or VX_DF_IMAGE_YUYV or VX_DF_IMAGE_NV12 or TIVX_DF_IMAGE_NV12_P12\n");
        //     }
        // }

#ifndef _APP_SINGLE_CAM_UTIL_H_
#define _APP_SINGLE_CAM_UTIL_H_

#include <TI/tivx.h>
#include <TI/tivx_task.h>
#include <TI/j7.h>
#include <TI/j7_imaging_aewb.h>
#include <tivx_utils_graph_perf.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#ifdef x86_64
#include <sys/stat.h>
#endif

#include <iss_sensors.h>
#include <iss_sensor_if.h>

#include <utils/perf_stats/include/app_perf_stats.h>

#define APP_MAX_FILE_PATH           (256u)
#define APP_ASSERT(x)               assert((x))
#define APP_ASSERT_VALID_REF(ref)   (APP_ASSERT(vxGetStatus((vx_reference)(ref))==VX_SUCCESS));

#define VISS_TEST_DATA_PATH_DEF  "./"
#define VISS_TEST_DATA_IMAGE  "input1.raw"
#define VISS_TEST_DATA_OUT  "output1.yuv"


#define MAX_NUM_BUF  8
#define NUM_BUFS 3u

typedef struct input_config
{
  int INPUT_IMAGE_TYPE;
  char INPUT_HOUZHUI[100] ;
  char INPUT_PATH[100];
  int OUT_0_IMAGE_TYPE;
  char OUT_0_HOUZHUI[100];
  int OUT_1_IMAGE_TYPE;
  char OUT_1_HOUZHUI[100];
  char RM_OUTPUT_PATH[100];
  char DS_OUTPUT_PATH[100];

  int LDC_TABLE_WIDTH;
  int LDC_TABLE_HEIGHT;
  int LDC_WIDTH;
  int LDC_HEIGHT;


  int BLOCK_WIDTH;
  int BLOCK_HEIGHT;
} input_config_t;

typedef struct
{
    vx_image gray_in_left_img[4];
    vx_image gray_out_left_img[4];
    //新增代码
    vx_image node_1_out;
    // vx_image gray_in_right_img[4];
    vx_image gray_out_right_img[4];
    //新增代码
    vx_image node_2_out;

    vx_image gray_graph_img;
    vx_image gray_out_graph_img;

    uint32_t width;
    uint32_t height;

    char input_path[APP_MAX_FILE_PATH];
    char rm_output_path[APP_MAX_FILE_PATH];
    char ds_output_path[APP_MAX_FILE_PATH];

    /* OpenVX references */
    vx_context context;
    vx_graph graph;

    vx_uint32 num_frames_to_run;

    vx_node node_left_ldc;
    vx_node node_right_ldc;
    uint32_t table_width, table_height;
    uint32_t ds_factor;

    vx_image mesh_left_img;
    vx_matrix matrix_right;

    tivx_vpac_ldc_params_t ldc_params;
    vx_user_data_object ldc_param_obj;
    tivx_vpac_ldc_mesh_params_t   mesh_params;
    vx_user_data_object mesh_params_obj;
    tivx_vpac_ldc_region_params_t region_params;
    tivx_vpac_ldc_multi_region_params_t multi_region_params;
    vx_user_data_object region_params_obj;

    input_config_t input_conf;

    app_perf_point_t total_perf;
    
} AppObj;





extern AppObj gAppObj;


#ifdef APP_DEBUG_SINGLE_CAM
#define APP_PRINTF(f_, ...) printf((f_), ##__VA_ARGS__)
#else
#define APP_PRINTF(f_, ...)
#endif


#endif //_APP_SINGLE_CAM_UTIL_H_


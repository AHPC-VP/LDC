
#include "app_test_ldc.h"
#include <utils/iss/include/app_iss.h>
#include <tivx_utils_file_rd_wr.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>

//#define DOUBLE_OUT
 #define SAVE_FILE
 //#define LOGOUT
 //#define MULTI_REGION
 #define TUOZHAN
 #define SAVE_MESH_2_IMG

#define LDC_DS_FACTOR               (1)
#define LDC_PIXEL_PAD               (1)

#define NODE_LDC_LEFT_ENENT   (1)
#define NODE_LDC_RIGHT_ENENT  (2)
#define NODE_LDC_LEFT_OUT_ENENT  (3)
#define NODE_LDC_RIGHT_OUT_ENENT  (4)

#define MAX_ATTRIBUTE_NAME (32u)
#define GRAPH_PARAM_ONE  (1)

AppObj gAppObj;

vx_status app_init(AppObj *obj);
vx_status app_deinit(AppObj *obj);
vx_status app_create_graph(AppObj *obj);
vx_status app_create_graph_use_own_mesh(AppObj *obj);
vx_status app_run_graph(AppObj *obj);
vx_status app_delete_graph(AppObj *obj);

uint64_t now_ms(void) {
    uint64_t time_in_usecs = 0;
    struct timeval tv;

    if (gettimeofday(&tv, NULL) < 0)
    {
        time_in_usecs = 0;
    }
    else
    {
        time_in_usecs = tv.tv_sec * 1000000ull + tv.tv_usec;
    }
    
    return time_in_usecs;
}

void show_image_attributes(vx_image image);

vx_status app_parse_cmd_line_args(AppObj *obj, int argc, char *argv[]);

vx_status app_load_vximage_mesh_from_text_file(char *filename, vx_image image);

int app_test_ldc_main(int argc, char* argv[])
{
    AppObj *obj = &gAppObj;
    
    if(app_parse_cmd_line_args(obj, argc, argv) == -1)
    {
        return;
    }
    app_init(obj);
    //app_create_graph(obj);
    app_create_graph_use_own_mesh(obj);
    app_run_graph(obj);
    app_delete_graph(obj);
    app_deinit(obj);

    return 0;
}


vx_status app_init(AppObj *obj)
{
    vx_status status;

    status = VX_FAILURE;

    obj->context = vxCreateContext();
    APP_ASSERT_VALID_REF(obj->context);

    tivxHwaLoadKernels(obj->context);
    tivxImagingLoadKernels(obj->context);
    APP_PRINTF("tivxImagingLoadKernels done\n");

    obj->table_width  = obj->input_conf.LDC_TABLE_WIDTH;
    obj->table_height = obj->input_conf.LDC_TABLE_HEIGHT;
    obj->ds_factor    = LDC_DS_FACTOR;

    status = VX_SUCCESS;

    return status;
}

vx_status app_deinit(AppObj *obj)
{
    vx_status status;

    status = VX_FAILURE;

    tivxHwaUnLoadKernels(obj->context);
    APP_PRINTF("tivxHwaUnLoadKernels done\n");

    tivxImagingUnLoadKernels(obj->context);
    APP_PRINTF("tivxImagingUnLoadKernels done\n");

    status = vxReleaseContext(&obj->context);
    if(VX_SUCCESS == status)
    {
        APP_PRINTF("vxReleaseContext done\n");
    }
    else
    {
        APP_PRINTF("Error: vxReleaseContext returned 0x%x \n", status);
    }

    return status;
}

#define INTER_BITS 5
#define INTER_TAB_SZIE (1 << INTER_BITS)

void float_to_s16q3(float f_in, short *s_ret)
{
    float adjust = 1.0 / 16.0;
    if (f_in < 0)
    {
        adjust = -adjust;
    }

    *s_ret = (f_in + adjust) * 8;
}


void convert_maps_resize(int w, int h, unsigned int *ret)
{
    int i = 0;
    int j = 0;

    short ret_x = 0;
    short ret_y = 0;

    int rx = 0;
    int ry = 0;


    for (i = 0; i < h; ++i)
    {
        for (j = 0; j < w; ++j)
        {
            float_to_s16q3(2 * j - j, &ret_x);
            float_to_s16q3(2 * i - i, &ret_y);
            //printf("tsx = %d, tsy = %d\n", miss_temp_sx, miss_temp_sy);

            rx = ret_x;
            ry = ret_y;
            ret[i * w + j] = (rx << 16) | (ry & 0x0ffff);
        }
    }
}

void convert_maps_mcorp(short *src_map1, unsigned short *src_map2, int w, int h, unsigned int *ret)
{
    int i = 0;
    int j = 0;

    int temp_sx = 0;
    int temp_sy = 0;

    //int miss_temp_sx = 0;
    // int miss_temp_sy = 0;

    short ret_x = 0;
    short ret_y = 0;

    double sx = 0.0;
    double sy = 0.0;

    int rx = 0;
    int ry = 0;

    FILE *fp = NULL;

    fp = fopen("/opt/vision_apps/mesh_src_map.txt", "w");

    unsigned short temp_dec = 0;

    for (i = 0; i < h; ++i)
    {
        for (j = 0; j < w; ++j)
        {
            temp_sx  = src_map1[i * w * 2 + j * 2];
            temp_sy  = src_map1[i * w * 2 + j * 2 + 1];
            //printf("temp_sx = %d, temp_sy = %d\n", temp_sx, temp_sy);
            temp_dec = src_map2[i * w + j];
            //printf("temp_sx = %d, temp_sy = %d\n", temp_sx, temp_sy);
            temp_sx  = temp_sx << INTER_BITS;
            temp_sx  = temp_sx + (temp_dec & (INTER_TAB_SZIE - 1));
            temp_sy  = temp_sy << INTER_BITS;
            temp_sy  = temp_sy + ((temp_dec >> INTER_BITS) & (INTER_TAB_SZIE - 1));

            sx = temp_sx;
            sy = temp_sy;
            sx = sx / INTER_TAB_SZIE;
            sy = sy / INTER_TAB_SZIE;

            fprintf(fp, "%d:%d ", (int)sx, (int)sy);
            if (0 == j % 20)
            {
                fprintf(fp, "\n");
            }

            float_to_s16q3(sx - j, &ret_x);
            float_to_s16q3(sy - i, &ret_y);
            //printf("tsx = %d, tsy = %d\n", miss_temp_sx, miss_temp_sy);
#if 0
            if (1080 == miss_temp_sy) //top half ok
            {

                float_to_s16q3(sx - j, &ret_x);
                float_to_s16q3(-i, &ret_y);
                //printf("tsy = %d, %d\n", miss_temp_sy, ret_x);
            }
#endif
            // if (ni < h)
            // {
            //     if (1080 == miss_temp_sy)
            //     {
            //         //float_to_s16q3(sx - j, &ret_x);
            //         //float_to_s16q3(1080 - i, &ret_y);
            //         //printf("tsy = %d, %d\n", miss_temp_sy, ret_x);
            //         float_to_s16q3(sx - j, &ret_x);
            //         float_to_s16q3(-i, &ret_y);
            //     }
            // }
            // else
            // {
            //     if (1080 == miss_temp_sy)
            //     {
            //         float_to_s16q3(sx - j, &ret_x);
            //         float_to_s16q3(1080 - i, &ret_y);
            //         //float_to_s16q3(sx - j, &ret_x);
            //         //float_to_s16q3(-i, &ret_y);
            //         //printf("tsy = %d, %d\n", miss_temp_sy, ret_x);
            //     }
            // }

            rx = ret_x;
            ry = ret_y;

            //printf("rx = %d, ry = %d\n", rx, ry);

            ret[i * w + j] = (rx << 16) | (ry & 0x0ffff);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
}

void convert_maps_mrize(short *src_map1, unsigned short *src_map2, int w, int h, unsigned int *ret)
{
    int i = 0;
    int j = 0;

    int temp_sx = 0;
    int temp_sy = 0;

    //int miss_temp_sx = 0;
    // int miss_temp_sy = 0;

    short ret_x = 0;
    short ret_y = 0;

    double sx = 0.0;
    double sy = 0.0;

    int rx = 0;
    int ry = 0;

    FILE *fp = NULL;

    fp = fopen("/opt/vision_apps/mesh_src_map.txt", "w");

    unsigned short temp_dec = 0;

    for (i = 0; i < h; ++i)
    {
        for (j = 0; j < w; ++j)
        {
            int nj = j * 2;
            int ni = i * 2;
            temp_sx  = src_map1[ni * w * 4 + nj * 2];
            temp_sy  = src_map1[ni * w * 4 + nj * 2 + 1];
            //printf("temp_sx = %d, temp_sy = %d\n", temp_sx, temp_sy);
            temp_dec = src_map2[ni * w * 2 + nj];
            temp_sx  = temp_sx << INTER_BITS;
            temp_sx  = temp_sx + (temp_dec & (INTER_TAB_SZIE - 1));
            temp_sy  = temp_sy << INTER_BITS;
            temp_sy  = temp_sy + ((temp_dec >> INTER_BITS) & (INTER_TAB_SZIE - 1));

            sx = temp_sx;
            sy = temp_sy;
            sx = sx / INTER_TAB_SZIE;
            sy = sy / INTER_TAB_SZIE;

            fprintf(fp, "%d:%d ", (int)sx, (int)sy);
            if (0 == j % 20)
            {
                fprintf(fp, "\n");
            }

            float_to_s16q3(sx - j, &ret_x);
            float_to_s16q3(sy - i, &ret_y);
            //printf("tsx = %d, tsy = %d\n", miss_temp_sx, miss_temp_sy);
#if 0
            if (1080 == miss_temp_sy) //top half ok
            {

                float_to_s16q3(sx - j, &ret_x);
                float_to_s16q3(-i, &ret_y);
                //printf("tsy = %d, %d\n", miss_temp_sy, ret_x);
            }
#endif
            // if (ni < h)
            // {
            //     if (1080 == miss_temp_sy)
            //     {
            //         //float_to_s16q3(sx - j, &ret_x);
            //         //float_to_s16q3(1080 - i, &ret_y);
            //         //printf("tsy = %d, %d\n", miss_temp_sy, ret_x);
            //         float_to_s16q3(sx - j, &ret_x);
            //         float_to_s16q3(-i, &ret_y);
            //     }
            // }
            // else
            // {
            //     if (1080 == miss_temp_sy)
            //     {
            //         float_to_s16q3(sx - j, &ret_x);
            //         float_to_s16q3(1080 - i, &ret_y);
            //         //float_to_s16q3(sx - j, &ret_x);
            //         //float_to_s16q3(-i, &ret_y);
            //         //printf("tsy = %d, %d\n", miss_temp_sy, ret_x);
            //     }
            // }

            rx = ret_x;
            ry = ret_y;

            //printf("rx = %d, ry = %d\n", rx, ry);

            ret[i * w + j] = (rx << 16) | (ry & 0x0ffff);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
}

void convert_maps(short *src_map1, unsigned short *src_map2, int w, int h, unsigned int *ret)
{
    int i = 0;
    int j = 0;

    int temp_sx = 0;
    int temp_sy = 0;

    //int miss_temp_sx = 0;
    int miss_temp_sy = 0;

    short ret_x = 0;
    short ret_y = 0;

    double sx = 0.0;
    double sy = 0.0;

    int rx = 0;
    int ry = 0;

    FILE *fp = NULL;

    fp = fopen("/opt/vision_apps/mesh_src_map.txt", "w");

    unsigned short temp_dec = 0;

    for (i = 0; i < h; ++i)
    {
        for (j = 0; j < w; ++j)
        {
            temp_sx  = src_map1[i * w * 2 + j * 2];
            miss_temp_sy = temp_sy  = src_map1[i * w * 2 + j * 2 + 1];
            //printf("temp_sx = %d, temp_sy = %d\n", temp_sx, temp_sy);
            temp_dec = src_map2[i * w + j];
            temp_sx  = temp_sx << INTER_BITS;
            temp_sx  = temp_sx + (temp_dec & (INTER_TAB_SZIE - 1));
            temp_sy  = temp_sy << INTER_BITS;
            temp_sy  = temp_sy + ((temp_dec >> INTER_BITS) & (INTER_TAB_SZIE - 1));

            sx = temp_sx;
            sy = temp_sy;
            sx = sx / INTER_TAB_SZIE;
            sy = sy / INTER_TAB_SZIE;

            fprintf(fp, "%d:%d ", (int)sx, (int)sy);
            if (0 == j % 20)
            {
                fprintf(fp, "\n");
            }

            float_to_s16q3(sx - j, &ret_x);
            float_to_s16q3(sy - i, &ret_y);
            //printf("tsx = %d, tsy = %d\n", miss_temp_sx, miss_temp_sy);
#if 0
            if (1080 == miss_temp_sy) //top half ok
            {

                float_to_s16q3(sx - j, &ret_x);
                float_to_s16q3(-i, &ret_y);
                //printf("tsy = %d, %d\n", miss_temp_sy, ret_x);
            }
#endif
            if (i < h / 2)
            {
                if (1080 == miss_temp_sy)
                {
                    //float_to_s16q3(sx - j, &ret_x);
                    //float_to_s16q3(1080 - i, &ret_y);
                    //printf("tsy = %d, %d\n", miss_temp_sy, ret_x);
                    float_to_s16q3(sx - j, &ret_x);
                    float_to_s16q3(-i, &ret_y);
                }
            }
            else
            {
                if (1080 == miss_temp_sy)
                {
                    float_to_s16q3(sx - j, &ret_x);
                    float_to_s16q3(1080 - i, &ret_y);
                    //float_to_s16q3(sx - j, &ret_x);
                    //float_to_s16q3(-i, &ret_y);
                    //printf("tsy = %d, %d\n", miss_temp_sy, ret_x);
                }
            }

            rx = ret_x;
            ry = ret_y;

            //printf("rx = %d, ry = %d\n", rx, ry);

            ret[i * w + j] = (rx << 16) | (ry & 0x0ffff);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
}

void convert_maps_down_sample(int w, int h, unsigned int *ret)
{
    int i = 0;
    int j = 0;

    int temp_sx = 0;
    int temp_sy = 0;

    //int miss_temp_sx = 0;
    // int miss_temp_sy = 0;

    short ret_x = 0;
    short ret_y = 0;

    double sx = 0.0;
    double sy = 0.0;

    int rx = 0;
    int ry = 0;

    unsigned short temp_dec = 0;
    FILE * fp = fopen("/opt/vision_apps/mesh_log1","wb");

    for (i = 0; i < h; ++i)
    {
        for (j = 0; j < w; ++j)
        {
            float_to_s16q3(2 * j - j, &ret_x);
            float_to_s16q3(2 * i - i, &ret_y);

            rx = ret_x;
            ry = ret_y;

            if(fp != NULL)
            {
                fprintf(fp,"i,j,sx,sy: %d,%d,%lf,%lf\n",i,j,sx,sy);
            }


            ret[i * w + j] = (rx << 16) | (ry & 0x0ffff);
        }
    }

    fclose(fp);
}

vx_status load_mesh_image_down_sample_1(vx_image image, unsigned int *ret,uint32_t w, uint32_t h)
{
    vx_uint32 width, height;
    vx_imagepatch_addressing_t image_addr;
    vx_rectangle_t rect;
    vx_map_id map_id;
    vx_df_image df;
    void *data_ptr;
    vx_status status;


    status = vxGetStatus((vx_reference)image);
    if(status==VX_SUCCESS)
    {
        vxQueryImage(image, VX_IMAGE_WIDTH, &width, sizeof(vx_uint32));
        vxQueryImage(image, VX_IMAGE_HEIGHT, &height, sizeof(vx_uint32));
        vxQueryImage(image, VX_IMAGE_FORMAT, &df, sizeof(vx_df_image));

        rect.start_x = 0;
        rect.start_y = 0;
        rect.end_x = width;
        rect.end_y = height;

        status = vxMapImagePatch(image,
            &rect,
            0,
            &map_id,
            &image_addr,
            &data_ptr,
            VX_WRITE_ONLY,
            VX_MEMORY_TYPE_HOST,
            VX_NOGAP_X
            );


        int k,l = 0;    
        int factor = 2;

        if(status==VX_SUCCESS)
        {
            int x, y;

            for(y=0; y < image_addr.dim_y; y++)
            {

                if (k >= h)
                {
                    k = h - 1;
                }
                l = 0;
                uint32_t *data = (uint32_t*)data_ptr + (image_addr.stride_y/4)*y;
                for(x=0; x < image_addr.dim_x; x++)
                {
                    if (l >= w)
                    {
                        l = w - 1;
                    }
                    data[x] = ret[k * w + l];
                    l += factor;
                }
                k += factor;
            }
            vxUnmapImagePatch(image, map_id);
        }
    }
    return status;
}

vx_status load_mesh_image_down_sample(vx_image image, unsigned int *ret , AppObj *obj)
{
    vx_uint32 width, height;
    vx_imagepatch_addressing_t image_addr;
    vx_rectangle_t rect;
    vx_map_id map_id;
    vx_df_image df;
    void *data_ptr;
    vx_status status;


    status = vxGetStatus((vx_reference)image);
    if(status==VX_SUCCESS)
    {
        vxQueryImage(image, VX_IMAGE_WIDTH, &width, sizeof(vx_uint32));
        vxQueryImage(image, VX_IMAGE_HEIGHT, &height, sizeof(vx_uint32));
        vxQueryImage(image, VX_IMAGE_FORMAT, &df, sizeof(vx_df_image));

        rect.start_x = 0;
        rect.start_y = 0;
        rect.end_x = width;
        rect.end_y = height;

        status = vxMapImagePatch(image,
            &rect,
            0,
            &map_id,
            &image_addr,
            &data_ptr,
            VX_WRITE_ONLY,
            VX_MEMORY_TYPE_HOST,
            VX_NOGAP_X
            );


        if(status==VX_SUCCESS)
        {
            uint32_t* data = data_ptr;
            int rows = 0;
            int cols = 0;
            for(int i = 0; i<image_addr.dim_x * image_addr.dim_y;i++)
            {
                cols = i%image_addr.dim_x;
                rows = i/image_addr.dim_x;

                #ifdef TUOZHAN

                if(cols >=  image_addr.dim_x-obj->input_conf.BLOCK_WIDTH)
                {
                   // data[i] = data[i-(cols - (image_addr.dim_x-obj->input_conf.BLOCK_WIDTH))-1];
                    data[i] = data[rows*image_addr.dim_x + (image_addr.dim_x - obj->input_conf.BLOCK_WIDTH - 1)];
                    continue;
                }


                if(rows >=  image_addr.dim_y-obj->input_conf.BLOCK_HEIGHT)
                {
                    data[i] = data[(image_addr.dim_y - obj->input_conf.BLOCK_HEIGHT -1)*image_addr.dim_x+cols];
                    continue;
                }


                // if(i >= image_addr.dim_x * (image_addr.dim_y- obj->input_conf.BLOCK_HEIGHT))
                // {
                //     int tmp = image_addr.dim_x * (image_addr.dim_y-obj->input_conf.BLOCK_HEIGHT-1) + (i%image_addr.dim_x); 
                //     data[i] = data[tmp];
                //     continue;  
                // }
                // if(i!=0 && i%image_addr.dim_x == 0)
                // {
                //     rows++;
                // }
                 data[i] = ret[cols*2 + rows*(image_addr.dim_x - obj->input_conf.BLOCK_WIDTH)*2];
                 #else
                 data[i] = ret[cols*2 + rows*image_addr.dim_x*2];
                 #endif
            }
            vxUnmapImagePatch(image, map_id);
        }
    }
    return status;
}

vx_status load_mesh_image_from_maphex_file(vx_image image, char *file_name)
{
    vx_uint32 width, height;
    vx_imagepatch_addressing_t image_addr;
    vx_rectangle_t rect;
    vx_map_id map_id;
    vx_df_image df;
    void *data_ptr;
    vx_status status;

    FILE *fp = fopen(file_name, "rb");

    status = vxGetStatus((vx_reference)image);
    if(status==VX_SUCCESS)
    {
        vxQueryImage(image, VX_IMAGE_WIDTH, &width, sizeof(vx_uint32));
        vxQueryImage(image, VX_IMAGE_HEIGHT, &height, sizeof(vx_uint32));
        vxQueryImage(image, VX_IMAGE_FORMAT, &df, sizeof(vx_df_image));

        rect.start_x = 0;
        rect.start_y = 0;
        rect.end_x = width;
        rect.end_y = height;

        status = vxMapImagePatch(image,
            &rect,
            0,
            &map_id,
            &image_addr,
            &data_ptr,
            VX_WRITE_ONLY,
            VX_MEMORY_TYPE_HOST,
            VX_NOGAP_X
            );

        if(status==VX_SUCCESS)
        {
            int x, y;

            for(y=0; y < image_addr.dim_y; y++)
            {
                uint32_t *data = (uint32_t*)data_ptr + (image_addr.stride_y/4)*y;
                for(x=0; x < image_addr.dim_x; x++)
                {
                    fscanf(fp, "%d ", &data[x]);
                    //printf("nnn data = %d\n", data[x]);
                }
                fscanf(fp, "\n");
            }
            vxUnmapImagePatch(image, map_id);
        }
    }
    fclose(fp);
    return status;
}

int mesh_index_file = 0;
vx_status load_mesh_image_from_buff(vx_image image, int w, int h, unsigned int *buff, int ds_factor)
{
    vx_uint32 width, height;
    vx_imagepatch_addressing_t image_addr;
    vx_rectangle_t rect;
    vx_map_id map_id;
    vx_df_image df;
    void *data_ptr;
    vx_status status;

    char f_path_name[APP_MAX_FILE_PATH];

    int k = 0;
    int l = 0;

    int factor = 1 << ds_factor;

    snprintf(f_path_name, APP_MAX_FILE_PATH, "/opt/vision_apps/rtos_mesh_%d.txt", mesh_index_file++);

    FILE *fp = fopen("/opt/vision_apps/rtos_mesh.txt", "wb");

    status = vxGetStatus((vx_reference)image);
    if(status==VX_SUCCESS)
    {
        vxQueryImage(image, VX_IMAGE_WIDTH, &width, sizeof(vx_uint32));
        vxQueryImage(image, VX_IMAGE_HEIGHT, &height, sizeof(vx_uint32));
        vxQueryImage(image, VX_IMAGE_FORMAT, &df, sizeof(vx_df_image));

        rect.start_x = 0;
        rect.start_y = 0;
        rect.end_x = width;
        rect.end_y = height;

        status = vxMapImagePatch(image,
            &rect,
            0,
            &map_id,
            &image_addr,
            &data_ptr,
            VX_WRITE_ONLY,
            VX_MEMORY_TYPE_HOST,
            VX_NOGAP_X
            );

        if(status==VX_SUCCESS)
        {
            int x, y;

            for(y=0; y < image_addr.dim_y; y++)
            {
                if (k >= h)
                {
                    k = h - 1;
                }
                l = 0;
                uint32_t *data = (uint32_t*)data_ptr + (image_addr.stride_y/4)*y;
                for(x=0; x < image_addr.dim_x; x++)
                {
                    if (l >= w)
                    {
                        l = w - 1;
                    }
                    data[x] = buff[k * w + l];
                    // data[x] = 4 * x;
                    l += factor;
                    fprintf(fp, "%d ", data[x]);
                    //printf("data = %d\n", data[x]);
                    //data[x] = 0;
                }
                k += factor;
                fprintf(fp, "\n");
            }
            vxUnmapImagePatch(image, map_id);
        }
    }
    fclose(fp);
    return status;
}


vx_status load_mesh_image_from_buff_corp(vx_image image, int w, int h, unsigned int *buff, int ds_factor)
{
    vx_uint32 width, height;
    vx_imagepatch_addressing_t image_addr;
    vx_rectangle_t rect;
    vx_map_id map_id;
    vx_df_image df;
    void *data_ptr;
    vx_status status;

    char f_path_name[APP_MAX_FILE_PATH];

    int k = 0;
    int l = 0;

    int factor = 1 << ds_factor;

    snprintf(f_path_name, APP_MAX_FILE_PATH, "/opt/vision_apps/rtos_mesh_%d.txt", mesh_index_file++);

    FILE *fp = fopen("/opt/vision_apps/rtos_mesh.txt", "wb");

    status = vxGetStatus((vx_reference)image);
    if(status==VX_SUCCESS)
    {
        vxQueryImage(image, VX_IMAGE_WIDTH, &width, sizeof(vx_uint32));
        vxQueryImage(image, VX_IMAGE_HEIGHT, &height, sizeof(vx_uint32));
        vxQueryImage(image, VX_IMAGE_FORMAT, &df, sizeof(vx_df_image));

        rect.start_x = 0;
        rect.start_y = 0;
        rect.end_x = width;
        rect.end_y = height;

        status = vxMapImagePatch(image,
            &rect,
            0,
            &map_id,
            &image_addr,
            &data_ptr,
            VX_WRITE_ONLY,
            VX_MEMORY_TYPE_HOST,
            VX_NOGAP_X
            );

        if(status==VX_SUCCESS)
        {
            int x, y;

            for(y=0; y < image_addr.dim_y; y++)
            {
                if (k >= h)
                {
                    k = h - 1;
                }
                l = 0;
                uint32_t *data = (uint32_t*)data_ptr + (image_addr.stride_y/4)*y;
                for(x=0; x < image_addr.dim_x; x++)
                {
                    if (l >= w)
                    {
                        l = w - 1;
                    }
                    data[x] = buff[k * w + l];
                    // data[x] = 4 * x;
                    l += factor;
                    fprintf(fp, "%d ", data[x]);
                    //printf("data = %d\n", data[x]);
                    data[x] = 0;
                }
                k += factor;
                fprintf(fp, "\n");
            }
            vxUnmapImagePatch(image, map_id);
        }
    }
    fclose(fp);
    return status;
}

void store_mesh_map(uint32_t w, uint32_t h, uint32_t ds_factor, unsigned int *buff)
{
    char f_path_name[APP_MAX_FILE_PATH];

    uint32_t k = 0;
    uint32_t l = 0;

    uint32_t table_width  = 0;
    uint32_t table_height = 0;
    uint32_t x = 0;
    uint32_t y = 0;

    table_width = (((w / (1 << ds_factor)) + 1u) + 15u) & (~15u);
    table_height = ((h / (1 << ds_factor)) + 1u);


    int factor = 1 << ds_factor;

    snprintf(f_path_name, APP_MAX_FILE_PATH, "/opt/vision_apps/rtos_mesh_%d.txt", mesh_index_file++);

    FILE *fp = fopen(f_path_name, "wb");


    for(y=0; y < table_height; y++)
    {
        if (k >= h)
        {
            k = h - 1;
        }
        l = 0;
        for(x=0; x < table_width; x++)
        {
            if (l >= w)
            {
                l = w - 1;
            }
            l += factor;
            fprintf(fp, "%d ", buff[k * w + l]);
        }
        k += factor;
        fprintf(fp, "\n");
    }

    fclose(fp);
}

unsigned int  temp_ret[1920 * 1080 * 10];
unsigned int  temp_ret_test[1920 * 1080 * 10];

void load_mesh_image(vx_image update, vx_image left, vx_image right)
{
    FILE *fp = NULL;

    FILE *sfp = NULL;

    short *map1;
    unsigned short *map2;

    unsigned int *ret = (unsigned int *)malloc(1920 * 1080 * sizeof(unsigned int));

    printf("load_mesh_image temp_addr = %p, size = %d\n", temp_ret, sizeof(temp_ret));
    printf("load_mesh_image temp_addr_n = %p, size = %d\n", temp_ret_test, sizeof(temp_ret_test));

    int len = 1920 * 1080 * 6;
    printf("load_mesh_image start tttttttttttt.\n");

    char *buff = (char *)malloc(1920 * 1080 * 6);

    fp = fopen("/opt/vision_apps/maphex.txt", "rb");
    printf("load_mesh_image start 1.\n");

    if (NULL == fp)
    {
        printf("load_mesh_image error.\n");
    }

    fread(buff, 1, len, fp);
    map1 = (short *)buff;
    map2 = (unsigned short *)(buff + 1920 * 1080 * 4);

    // convert_maps(map1, map2, 1920, 1080, temp_ret);
    convert_maps_resize(1920 / 2, 1080 / 2, temp_ret);
    sfp = fopen("/opt/vision_apps/rtos_left.txt", "wb");
    fwrite(ret, 1920 * 1080, sizeof(unsigned int), sfp);
    fclose(sfp);
    load_mesh_image_from_buff(left, 1920 / 2, 1080 / 2, temp_ret, 1);
    //store_mesh_map(1920, 1080, 2, temp_ret);
    //load_mesh_image_from_maphex_file(left, "/opt/vision_apps/rtos_mesh_0.txt");


    convert_maps(map1, map2, 1920, 1080, temp_ret);
    load_mesh_image_from_buff_corp(update, 1920, 1080, temp_ret, 2);

    fread(buff, 1, len, fp);
    map1 = (short *)buff;
    map2 = (unsigned short *)(buff + 1920 * 1080 * 4);
    /* rectify resize */
    // convert_maps_mrize(map1, map2, 1920 / 2, 1080 / 2, temp_ret);
    // sfp = fopen("/opt/vision_apps/rtos_right.txt", "wb");
    // fwrite(ret, 1920 * 1080, sizeof(unsigned int), sfp);
    // fclose(sfp);
    // load_mesh_image_from_buff(right, 1920 / 2, 1080 / 2, temp_ret, 1);
    // //store_mesh_map(1920, 1080, 2, temp_ret);
    // //load_mesh_image_from_maphex_file(right, "/opt/vision_apps/rtos_mesh_1.txt");
    

    /* rectify corp */
    convert_maps(map1, map2, 1920, 1080, temp_ret);
    sfp = fopen("/opt/vision_apps/rtos_right.txt", "wb");
    fwrite(ret, 1920 * 1080, sizeof(unsigned int), sfp);
    fclose(sfp);
    load_mesh_image_from_buff(right, 1920, 1080, temp_ret, 1);
    // store_mesh_map(1920, 1080, 2, temp_ret);
    // load_mesh_image_from_maphex_file(right, "/opt/vision_apps/rtos_mesh_1.txt");
}

void convert_maps_eyefish(short *src_map1, unsigned short *src_map2, int w, int h, unsigned int *ret)
{
    int i = 0;
    int j = 0;

    int temp_sx = 0;
    int temp_sy = 0;

    //int miss_temp_sx = 0;
    int miss_temp_sy = 0;

    short ret_x = 0;
    short ret_y = 0;

    double sx = 0.0;
    double sy = 0.0;

    int rx = 0;
    int ry = 0;

    unsigned short temp_dec = 0;

    FILE * pf;
    pf = fopen("/opt/vision_apps/mesh_log","wb");
    for (i = 0; i < h; ++i)
    {
        for (j = 0; j < w; ++j)
        {
            temp_sx  = src_map1[i * w * 2 + j * 2];
            miss_temp_sy = temp_sy  = src_map1[i * w * 2 + j * 2 + 1];
            temp_dec = src_map2[i * w + j];
            temp_sx  = temp_sx << INTER_BITS;
            temp_sx  = temp_sx + (temp_dec & (INTER_TAB_SZIE - 1));
            temp_sy  = temp_sy << INTER_BITS;
            temp_sy  = temp_sy + ((temp_dec >> INTER_BITS) & (INTER_TAB_SZIE - 1));

            sx = temp_sx;
            sy = temp_sy;
            sx = sx / INTER_TAB_SZIE;
            sy = sy / INTER_TAB_SZIE;

            if(sx == 1280.)
            {
                sx = sx-1;
            }

            if(sy == 960.)
            {
                sy = sy-1;
            }

            
            if(pf != NULL)
            {
                //fprintf(pf,"i,j,delta_x,delta_y: %d,%d,%lf,%lf\n",i,j,(sx - j*8),(sy - i*8));

                fprintf(pf,"out_x,out_y,in_x,in_y: %d,%d,%lf,%lf\n",j*8,i*8,sx,sy);
            }
            
            
            float_to_s16q3(sx - j*8, &ret_x);
            float_to_s16q3(sy - i*8, &ret_y);

            rx = ret_x;
            ry = ret_y;

            ret[i * w + j] = (rx << 16) | (ry & 0x0ffff);
        }
    }
    fclose(pf);

}


void convert_maps_eyefish_and_save(short *src_map1, unsigned short *src_map2, int w, int h, unsigned int *ret,vx_image img)
{
    int i = 0;
    int j = 0;

    int temp_sx = 0;
    int temp_sy = 0;

    //int miss_temp_sx = 0;
    int miss_temp_sy = 0;

    short ret_x = 0;
    short ret_y = 0;

    double sx = 0.0;
    double sy = 0.0;

    int rx = 0;
    int ry = 0;

    unsigned short temp_dec = 0;

    for (i = 0; i < h; ++i)
    {
        for (j = 0; j < w; ++j)
        {
            temp_sx  = src_map1[i * w * 2 + j * 2];
            miss_temp_sy = temp_sy  = src_map1[i * w * 2 + j * 2 + 1];
            temp_dec = src_map2[i * w + j];
            temp_sx  = temp_sx << INTER_BITS;
            temp_sx  = temp_sx + (temp_dec & (INTER_TAB_SZIE - 1));
            temp_sy  = temp_sy << INTER_BITS;
            temp_sy  = temp_sy + ((temp_dec >> INTER_BITS) & (INTER_TAB_SZIE - 1));

            sx = temp_sx;
            sy = temp_sy;
            sx = sx / INTER_TAB_SZIE;
            sy = sy / INTER_TAB_SZIE;
            
            

            
            float_to_s16q3(sx - j*8, &ret_x);
            float_to_s16q3(sy - i*8, &ret_y);

            rx = ret_x;
            ry = ret_y;

            ret[i * w + j] = (rx << 16) | (ry & 0x0ffff);
        }
    }
}

void load_mesh_eyefish(vx_image image, int w, int h, unsigned int *buff, int ds_factor)
{
    vx_uint32 width, height;
    vx_imagepatch_addressing_t image_addr;
    vx_rectangle_t rect;
    vx_map_id map_id;
    vx_df_image df;
    void *data_ptr;
    vx_status status;

    int k = 0;
    int l = 0;

    int factor = 1 << ds_factor;

    status = vxGetStatus((vx_reference)image);
    if(status==VX_SUCCESS)
    {
        vxQueryImage(image, VX_IMAGE_WIDTH, &width, sizeof(vx_uint32));
        vxQueryImage(image, VX_IMAGE_HEIGHT, &height, sizeof(vx_uint32));
        vxQueryImage(image, VX_IMAGE_FORMAT, &df, sizeof(vx_df_image));

        rect.start_x = 0;
        rect.start_y = 0;
        rect.end_x = width;
        rect.end_y = height;

        status = vxMapImagePatch(image,
            &rect,
            0,
            &map_id,
            &image_addr,
            &data_ptr,
            VX_WRITE_ONLY,
            VX_MEMORY_TYPE_HOST,
            VX_NOGAP_X
            );

        if(status==VX_SUCCESS)
        {
            int x, y;

            for(y=0; y < image_addr.dim_y; y++)
            {
                if (k >= h)
                {
                    k = h - 1;
                }
                l = 0;
                uint32_t *data = (uint32_t*)data_ptr + (image_addr.stride_y/4)*y;
                uint32_t *last_data = NULL;
                if(y > 0)
                {
                    last_data = (uint32_t*)data_ptr + (image_addr.stride_y/4)*(y-1);
                }
                for(x=0; x < image_addr.dim_x; x++)
                {
                    if (l >= w)
                    {
                        l = w - 1;
                    }
                    data[x] = buff[k * w + l];
                    l += factor;
                }
                k += factor;
            }
            vxUnmapImagePatch(image, map_id);
        }
    }
    return status;
}

void load_mesh_eyefish_1(vx_image image,unsigned int *buff,AppObj *obj,int tuozhan_w, int tuozhan_h)
{
    vx_uint32 width, height;
    vx_imagepatch_addressing_t image_addr;
    vx_rectangle_t rect;
    vx_map_id map_id;
    vx_df_image df;
    void *data_ptr;
    vx_status status;


    status = vxGetStatus((vx_reference)image);
    if(status==VX_SUCCESS)
    {
        vxQueryImage(image, VX_IMAGE_WIDTH, &width, sizeof(vx_uint32));
        vxQueryImage(image, VX_IMAGE_HEIGHT, &height, sizeof(vx_uint32));
        vxQueryImage(image, VX_IMAGE_FORMAT, &df, sizeof(vx_df_image));

        rect.start_x = 0;
        rect.start_y = 0;
        rect.end_x = width;
        rect.end_y = height;

        status = vxMapImagePatch(image,
            &rect,
            0,
            &map_id,
            &image_addr,
            &data_ptr,
            VX_WRITE_ONLY,
            VX_MEMORY_TYPE_HOST,
            VX_NOGAP_X
            );


        if(status==VX_SUCCESS)
        {
            uint32_t* data = data_ptr;
            int rows = 0;
            int cols = 0;
            for(int i = 0; i<image_addr.dim_x * image_addr.dim_y;i++)
            {
                cols = i%image_addr.dim_x;
                rows = i/image_addr.dim_x;

                #ifdef TUOZHAN

                if(cols >=  image_addr.dim_x-tuozhan_w)
                {
                    data[i] = data[rows*image_addr.dim_x + (image_addr.dim_x - tuozhan_w - 1)];
                    continue;
                }


                if(rows >=  image_addr.dim_y-tuozhan_h)
                {
                    data[i] = data[(image_addr.dim_y - tuozhan_h -1)*image_addr.dim_x+cols];
                    continue;
                }

                 data[i] = buff[cols + rows*(image_addr.dim_x - tuozhan_w)];
                 #else
                 data[i] = buff[cols + rows*image_addr.dim_x];
                 #endif
            }
            vxUnmapImagePatch(image, map_id);
        }
    }
    return status;
}

static void add_graph_parameter_by_node_index(vx_graph graph, vx_node node, vx_uint32 node_parameter_index)
{
    vx_parameter parameter = vxGetParameterByIndex(node, node_parameter_index);

    vxAddParameterToGraph(graph, parameter);
    vxReleaseParameter(&parameter);
}

vx_status vp_graph_ldc(vx_context           context,
                       vx_user_data_object  configuration,
                       vx_matrix            warp_matrix,
                       vx_user_data_object  region_prms,
                       vx_user_data_object  mesh_prms,
                       vx_image             mesh_img,
                       vx_user_data_object  dcc_db,
                       vx_image             in_img,
                       vx_image             out0_img,
                       vx_image             out1_img)
{
    vx_status status = (vx_status)VX_SUCCESS;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == (vx_status)VX_SUCCESS)
    {
        vx_node node = tivxVpacLdcNode(graph, configuration, warp_matrix, region_prms, mesh_prms, mesh_img,
                dcc_db, in_img, out0_img, out1_img);
        if (vxGetStatus((vx_reference)node)==(vx_status)VX_SUCCESS)
        {
            //status = setNodeTarget(node);
            status = vxSetNodeTarget(node, VX_TARGET_STRING, TIVX_TARGET_VPAC_LDC1);
            if (status == (vx_status)VX_SUCCESS)
            {
                status = vxVerifyGraph(graph);
            }
            if (status == (vx_status)VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

int loop_flag = 1;

void SignHandler(int iSignNo)
{
    printf("Capture sign no:%d ",iSignNo); 
    loop_flag = 0;
}
int16_t convert_double_to_16bit_fix(double fp,int32_t Q_num)
{
    int16_t fixed_point_decimal = 0;
    const double Q_max = (double)(1<<Q_num);
    const double fix_max = (double)((1<<15)-1);
    double float_point_decimal = fp*Q_max;
    if(float_point_decimal < (-fix_max))
    {
        float_point_decimal = -fix_max;
    }
    else if(float_point_decimal > fix_max)
    {
        float_point_decimal = fix_max;
    }
    fixed_point_decimal = (int16_t)float_point_decimal;
    return fixed_point_decimal;
}
double limit_precision_to_16bit(double high_precision_value,int32_t Q_num)
{
    double low_precision_value = 0.0;

    const double Q_max = (double)(1<<Q_num);
    const int64_t fix_max = ((1<<15)-1);
    int64_t low_precision_value_int = high_precision_value * Q_max;
    if(low_precision_value_int < (-fix_max))
    {
        low_precision_value_int = -fix_max;
    }
    else if(low_precision_value_int > fix_max)
    {
        low_precision_value_int = fix_max;
    }

    low_precision_value = low_precision_value_int / Q_max;

    return low_precision_value;
    
}
//#define Sf( y, x ) ((float*)(srcdata + y*srcstep))[x]
#define Sd( y, x ) ((double*)(srcdata + y*srcstep))[x]
//#define Df( y, x ) ((float*)(dstdata + y*dststep))[x]
#define Dd( y, x ) ((double*)(dstdata + y*dststep))[x]
void calc_hemograph_matrix(int16_t * mat,int32_t idx)
{
    double m[3][3];
    double low_precision_m[3][3];
    switch (idx)
    {
    case 0/* constant-expression */:
        m[0][0] = -2.50701085e-01;
        m[0][1] = -2.13542052e+00;
        m[0][2] = 1.20067303e+03;

        m[1][0] = -1.17893913e-17;
        m[1][1] = -2.79859471e+00;
        m[1][2] = 1.50794634e+03;

        m[2][0] = -1.11351878e-20;
        m[2][1] = -2.22439639e-03;
        m[2][2] = 1.00000000e+00;
        break;
    case 1/* constant-expression */:
        m[0][0] = -2.50626998e-01;
        m[0][1] = -2.13471629e+00;
        m[0][2] = 1.20060190e+03;

        m[1][0] = -1.75765899e-17;
        m[1][1] = -2.79779618e+00;
        m[1][2] = 1.50793957e+03;

        m[2][0] = -1.66013034e-20;
        m[2][1] = -2.22366282e-03;
        m[2][2] = 1.00000000e+00;
        break;
    case 2/* constant-expression */:
        m[0][0] = -2.50535348e-01;
        m[0][1] = -2.13384467e+00;
        m[0][2] = 1.20051392e+03;

        m[1][0] = 1.85712436e-17;
        m[1][1] = -2.79680803e+00;
        m[1][2] = 1.50793138e+03;

        m[2][0] = 1.75404856e-20;
        m[2][1] = -2.22275488e-03;
        m[2][2] = 1.00000000e+00;
        break;
    case 3/* constant-expression */:
        m[0][0] = -2.50436271e-01;
        m[0][1] = -2.13290261e+00;
        m[0][2] = 1.20041880e+03;

        m[1][0] = -1.33386393e-16;
        m[1][1] = -2.79574010e+00;
        m[1][2] = 1.50792263e+03;

        m[2][0] = -1.25986884e-19;
        m[2][1] = -2.22177356e-03;
        m[2][2] = 1.00000000e+00;
        break;
    
    default:
        // m[0][0] = -1.00000000e+00;
        // m[0][1] = 0;
        // m[0][2] = 0;

        // m[1][0] = 0;
        // m[1][1] = -1.00000000e+00;
        // m[1][2] = 0;

        // m[2][0] = 0;
        // m[2][1] = 0;
        // m[2][2] = 1.00000000e+00;

        m[0][0] = -1.00000000e+00;
        m[0][1] = 0;
        m[0][2] = 1280.00000000e+00;

        m[1][0] = 0;
        m[1][1] = -1.00000000e+00;
        m[1][2] = 960.00000000e+00;

        m[2][0] = 0;
        m[2][1] = 0;
        m[2][2] = 1.00000000e+00;
        break;
    }
    printf("idx = %d\n",idx);
    for(int i = 0; i < 3;i ++)
    {
        for(int j = 0; j < 3;j ++)
        {
            printf("m[%d][%d] = %lf\n",i,j,m[i][j]);
        }

    }
    low_precision_m[0][0] = limit_precision_to_16bit(m[0][0],12);
    low_precision_m[0][1] = limit_precision_to_16bit(m[0][1],12);
    low_precision_m[0][2] = limit_precision_to_16bit(m[0][2],3);
    low_precision_m[1][0] = limit_precision_to_16bit(m[1][0],12);
    low_precision_m[1][1] = limit_precision_to_16bit(m[1][1],12);
    low_precision_m[1][2] = limit_precision_to_16bit(m[1][2],3);
    low_precision_m[2][0] = limit_precision_to_16bit(m[2][0],23);
    low_precision_m[2][1] = limit_precision_to_16bit(m[2][1],23);
    low_precision_m[2][2] = 1.0;

    // for(int i = 0; i < 3;i ++)
    // {
    //     for(int j = 0; j < 3;j ++)
    //     {
    //         printf("low_precision_m[%d][%d] = %lf\n",i,j,low_precision_m[i][j]);
    //     }

    // }

    double det = low_precision_m[0][0]*(low_precision_m[1][1]*low_precision_m[2][2] - low_precision_m[1][2]*low_precision_m[2][1]);
    det = det - low_precision_m[0][1]*(low_precision_m[1][0]*low_precision_m[2][2] - low_precision_m[1][2]*low_precision_m[2][0]);
    det = det + low_precision_m[0][2]*(low_precision_m[1][0]*low_precision_m[2][1] - low_precision_m[1][1]*low_precision_m[2][0]);
    //printf("det = %lf\n",det);

    double low_precision_m_inv[3][3];
    double* srcdata = &low_precision_m[0][0];
    double* dstdata = &low_precision_m_inv[0][0];
    size_t srcstep = 3;
    size_t dststep = 3;

    double d = 1. /det;
    double t[9];

    t[0] = (Sd(1,1) * Sd(2,2) - Sd(1,2) * Sd(2,1)) * d;
    t[1] = (Sd(0,2) * Sd(2,1) - Sd(0,1) * Sd(2,2)) * d;
    t[2] = (Sd(0,1) * Sd(1,2) - Sd(0,2) * Sd(1,1)) * d;

    t[3] = (Sd(1,2) * Sd(2,0) - Sd(1,0) * Sd(2,2)) * d;
    t[4] = (Sd(0,0) * Sd(2,2) - Sd(0,2) * Sd(2,0)) * d;
    t[5] = (Sd(0,2) * Sd(1,0) - Sd(0,0) * Sd(1,2)) * d;

    t[6] = (Sd(1,0) * Sd(2,1) - Sd(1,1) * Sd(2,0)) * d;
    t[7] = (Sd(0,1) * Sd(2,0) - Sd(0,0) * Sd(2,1)) * d;
    t[8] = (Sd(0,0) * Sd(1,1) - Sd(0,1) * Sd(1,0)) * d;

    // for(int i = 0; i < 9;i ++)
    // {
    //     printf("t[%d] = %lf\n",i,t[i]);
    // }

    Dd(0,0) = t[0]; 
    Dd(0,1) = t[1]; 
    Dd(0,2) = t[2];
    Dd(1,0) = t[3]; 
    Dd(1,1) = t[4]; 
    Dd(1,2) = t[5];
    Dd(2,0) = t[6]; 
    Dd(2,1) = t[7]; 
    Dd(2,2) = t[8];

    double sort_m[9];
    double format_m[9];
    double divisor = low_precision_m_inv[2][2];
    for(int i = 0; i < 3;i ++)
    {
        for(int j = 0; j < 3;j ++)
        {
            format_m[i*3+j] = low_precision_m_inv[i][j] / divisor;
            //printf("low_precision_m_inv[%d][%d] = %lf\n",i,j,low_precision_m_inv[i][j]);
        }
    }
    
    for(int i = 0; i < 9;i ++)
    {
        int j = i/3;
        int k = i%3;
        sort_m[k*3+j] = format_m[i];
    }

    mat[0] = convert_double_to_16bit_fix(sort_m[0],12);
    mat[1] = convert_double_to_16bit_fix(sort_m[1],12);
    mat[2] = convert_double_to_16bit_fix(sort_m[2],23);
    mat[3] = convert_double_to_16bit_fix(sort_m[3],12);
    mat[4] = convert_double_to_16bit_fix(sort_m[4],12);
    mat[5] = convert_double_to_16bit_fix(sort_m[5],23);
    mat[6] = convert_double_to_16bit_fix(sort_m[6],3);
    mat[7] = convert_double_to_16bit_fix(sort_m[7],3);
    mat[8] = 1;

}


vx_status vp_save_image_to_binary(char *file_path, vx_image img,int file_type)
{
    vx_uint32 width,height;
    vx_imagepatch_addressing_t image_addr_bit;
    vx_rectangle_t rect;
    vx_map_id map_id_bit_1,map_id_bit_2;
    vx_df_image df_bit;

    FILE *fp = NULL;

    void *data_ptr_bit_1,*data_ptr_bit_2;

    vx_status status;

    fp = fopen(file_path,"wb");

    if(NULL == fp)
    {
        printf("open %s failed!\n",file_path);
        return -1;
    }
    printf("open %s success!\n",file_path);
    vxQueryImage(img, VX_IMAGE_WIDTH, &width, sizeof(vx_uint32));
    vxQueryImage(img, VX_IMAGE_HEIGHT, &height, sizeof(vx_uint32));
    vxQueryImage(img, VX_IMAGE_FORMAT, &df_bit, sizeof(vx_df_image));

    rect.start_x = 0;
    rect.start_y = 0;
    rect.end_x = width;
    rect.end_y = height;

    status = vxMapImagePatch(img,
            &rect,
            0,
            &map_id_bit_1,
            &image_addr_bit,
            &data_ptr_bit_1,
            VX_WRITE_ONLY,
            VX_MEMORY_TYPE_HOST,
            VX_NOGAP_X
            );

    if(file_type == VX_DF_IMAGE_NV12)
    {
        fwrite((uint8_t*)data_ptr_bit_1,sizeof(uint8_t),width*height,fp);
        fflush(fp);
        status = vxMapImagePatch(img,
            &rect,
            1,
            &map_id_bit_2,
            &image_addr_bit,
            &data_ptr_bit_2,
            VX_WRITE_ONLY,
            VX_MEMORY_TYPE_HOST,
            VX_NOGAP_X
            );
        fwrite((uint8_t*)data_ptr_bit_2,sizeof(uint8_t),width*(height/2),fp);
        fflush(fp);
        vxUnmapImagePatch(img,map_id_bit_1);
        vxUnmapImagePatch(img,map_id_bit_2);
        fclose(fp);
    }

    if(file_type == TIVX_DF_IMAGE_NV12_P12)
    {
        fwrite(data_ptr_bit_1,1,width*height*1.5,fp);
        fflush(fp);
        status = vxMapImagePatch(img,
            &rect,
            1,
            &map_id_bit_2,
            &image_addr_bit,
            &data_ptr_bit_2,
            VX_WRITE_ONLY,
            VX_MEMORY_TYPE_HOST,
            VX_NOGAP_X
            );
        fwrite(data_ptr_bit_2,1,width*(height/2)*1.5,fp);
        fflush(fp);
        vxUnmapImagePatch(img,map_id_bit_1);
        vxUnmapImagePatch(img,map_id_bit_2);
        fclose(fp);
    }


    if(file_type == VX_DF_IMAGE_UYVY)
    {
        fwrite((uint8_t*)data_ptr_bit_1,sizeof(uint8_t),width*2*height,fp);
        vxUnmapImagePatch(img,map_id_bit_1);
        fclose(fp);
    }


    if(file_type == VX_DF_IMAGE_YUYV)
    {
        fwrite((uint8_t*)data_ptr_bit_1,sizeof(uint8_t),width*2*height,fp);
        vxUnmapImagePatch(img,map_id_bit_1);
        fclose(fp);
    }
    
    return status;
}


vx_status vp_load_binary_to_image(vx_image img,char *file_path,int file_type)
{
    vx_uint32 width,height;
    vx_imagepatch_addressing_t image_addr_bit;
    vx_rectangle_t rect;
    vx_map_id map_id_bit,map_id_bit_1;
    vx_df_image df_bit;

    FILE *fp = NULL;

    void *data_ptr_bit,*data_ptr_bit_1;

    vx_status status;

    fp = fopen(file_path,"rb");

    if(NULL == fp)
    {
        printf("open %s failed!\n",file_path);
        return -1;
    }
    printf("open %s success!\n",file_path);
    vxQueryImage(img, VX_IMAGE_WIDTH, &width, sizeof(vx_uint32));
    vxQueryImage(img, VX_IMAGE_HEIGHT, &height, sizeof(vx_uint32));
    vxQueryImage(img, VX_IMAGE_FORMAT, &df_bit, sizeof(vx_df_image));

    rect.start_x = 0;
    rect.start_y = 0;
    rect.end_x = width;
    rect.end_y = height;

    status = vxMapImagePatch(img,
            &rect,
            0,
            &map_id_bit,
            &image_addr_bit,
            &data_ptr_bit,
            VX_WRITE_ONLY,
            VX_MEMORY_TYPE_HOST,
            VX_NOGAP_X
            );

    if(file_type == VX_DF_IMAGE_UYVY)
    {
        fread((uint8_t*)data_ptr_bit,sizeof(uint8_t),width*2*height,fp);
        vxUnmapImagePatch(img,map_id_bit);
        fclose(fp);
    }

    if(file_type == TIVX_DF_IMAGE_NV12_P12)
    {
        fread((uint8_t*)data_ptr_bit,sizeof(uint8_t),width*height*3/2,fp);
        status = vxMapImagePatch(img,
            &rect,
            1,
            &map_id_bit_1,
            &image_addr_bit,
            &data_ptr_bit_1,
            VX_WRITE_ONLY,
            VX_MEMORY_TYPE_HOST,
            VX_NOGAP_X
            );
        fread((uint8_t*)data_ptr_bit_1,sizeof(uint8_t),width*height*3/4,fp);
        vxUnmapImagePatch(img,map_id_bit);
        vxUnmapImagePatch(img,map_id_bit_1);
        fclose(fp);
    }

    if(file_type == VX_DF_IMAGE_NV12)
    {
        fread((uint8_t*)data_ptr_bit,sizeof(uint8_t),width*height,fp);
        status = vxMapImagePatch(img,
            &rect,
            1,
            &map_id_bit_1,
            &image_addr_bit,
            &data_ptr_bit_1,
            VX_WRITE_ONLY,
            VX_MEMORY_TYPE_HOST,
            VX_NOGAP_X
            );
        fread((uint8_t*)data_ptr_bit_1,sizeof(uint8_t),width*height*1/2,fp);
        vxUnmapImagePatch(img,map_id_bit);
        vxUnmapImagePatch(img,map_id_bit_1);
        fclose(fp);
    }


    
    return status;
}

vx_status app_create_graph_use_own_mesh(AppObj *obj)
{
    vx_status status = VX_FAILURE;

    char f_path_mesh[APP_MAX_FILE_PATH];

    vx_graph_parameter_queue_params_t gp_list[8];

    uint32_t table_width  = 0;
    uint32_t table_height = 0;
    int i = 0;

    obj->width  = obj->input_conf.LDC_WIDTH;
    obj->height = obj->input_conf.LDC_HEIGHT;

    obj->graph = vxCreateGraph(obj->context);
    APP_ASSERT_VALID_REF(obj->graph);

    for (i = 0; i < 4; ++i)
    {
        obj->gray_in_left_img[i]   = vxCreateImage(obj->context, obj->width, obj->height, obj->input_conf.INPUT_IMAGE_TYPE);
        obj->gray_out_left_img[i]  = vxCreateImage(obj->context, obj->table_width, obj->table_height, obj->input_conf.OUT_0_IMAGE_TYPE);
        obj->gray_out_right_img[i] = vxCreateImage(obj->context, obj->width, obj->height, obj->input_conf.OUT_0_IMAGE_TYPE);
    }

    //vx_image mesh_image = vxCreateImage(obj->context, obj->table_width/8, obj->table_height/8, VX_DF_IMAGE_U8);

    //新增代码
    #ifdef DOUBLE_OUT
    obj->node_1_out = vxCreateImage(obj->context, obj->table_width, obj->table_height, obj->input_conf.OUT_1_IMAGE_TYPE);
    obj->node_2_out = vxCreateImage(obj->context, obj->width, obj->height, obj->input_conf.OUT_1_IMAGE_TYPE);
    #endif
    //新增代码结束

    // signal(SIGINT, SignHandler); 

    printf("app_create_graph_use_own_mesh: w = %d, h = %d, tw = %d, th = %d\n",
           obj->width, obj->height, obj->table_width, obj->table_height);

    printf("block size : %d %d \n",obj->input_conf.BLOCK_WIDTH,obj->input_conf.BLOCK_HEIGHT);       

    /* Mesh Image */
    #ifdef TUOZHAN
    int tuozhan_w = 0;
    int tuozhan_h = 0;
    obj->mesh_left_img = vxCreateImage(obj->context,
                                        obj->table_width /8 +tuozhan_w, obj->table_height / 8 +tuozhan_h, VX_DF_IMAGE_U32);
                                       
    #else
    obj->mesh_left_img = vxCreateImage(obj->context,
                                       obj->table_width / 8 , obj->table_height / 8 , VX_DF_IMAGE_U32);
    #endif
    unsigned int * ret = malloc(obj->table_width/8  * obj->table_height/8 * sizeof(int));

    short *map1;
    unsigned short *map2;
    
    FILE *map_hex_fp = NULL; 
    map_hex_fp = fopen("/opt/vision_apps/LeftFishEyeRectify_maphex_data.bin", "rb");
    //map_hex_fp = fopen("/opt/vision_apps/maxhex.bin", "rb");
    if(map_hex_fp == NULL)
    {
        printf("open maphex file failed!\n");
    }
    char *map_hex_buff = (char *)malloc(obj->table_width/8 * obj->table_height/8 * 6);
    fread(map_hex_buff, 1, obj->table_width/8 * obj->table_height/8 * 6, map_hex_fp);
    map1 = (short *)map_hex_buff;
    map2 = (unsigned short *)(map_hex_buff+obj->table_width/8 * obj->table_height/8 * 4);

    // map1 = (short *)(map_hex_buff+obj->table_width/8 * obj->table_height/8 * 6);
    // map2 = (unsigned short *)(map_hex_buff+obj->table_width/8 * obj->table_height/8 * 10);
    convert_maps_eyefish(map1,map2,obj->table_width/8,obj->table_height/8,ret);
    load_mesh_eyefish_1(obj->mesh_left_img,ret,obj,tuozhan_w,tuozhan_h);


    //  printf("LOG1!!!!!!!!!!!!!!!!!!!!\n");
    //  convert_maps_down_sample(obj->table_width,obj->table_height,ret);
    //  printf("LOG2!!!!!!!!!!!!!!!!!!!!\n");
    //  load_mesh_image_down_sample(obj->mesh_left_img,ret,obj);
    //  printf("LOG3!!!!!!!!!!!!!!!!!!!!\n");

    // Create vx_float32 buffer object
    int rows = 3;
    int cols = 3;
    int16_t mat[rows*cols];

    calc_hemograph_matrix(&mat[0],5);

    // Create vx_matrix object
    obj->matrix_right = vxCreateMatrix(obj->context, VX_TYPE_INT16, cols, rows);

    // Set pointer reference between vx_float32 data object and
    vxCopyMatrix(obj->matrix_right, &mat[0], (vx_enum)VX_WRITE_ONLY, (vx_enum)VX_MEMORY_TYPE_HOST);

    /* Mesh Parameters */
    obj->mesh_params_obj = vxCreateUserDataObject(obj->context,
                                                  "tivx_vpac_ldc_mesh_params_t", sizeof(tivx_vpac_ldc_mesh_params_t), NULL);
    memset(&obj->mesh_params, 0, sizeof(tivx_vpac_ldc_mesh_params_t));

    tivx_vpac_ldc_mesh_params_init(&obj->mesh_params);
    obj->mesh_params.mesh_frame_width  = obj->table_width;
    obj->mesh_params.mesh_frame_height = obj->table_height;
    obj->mesh_params.subsample_factor  = 3;

    vxCopyUserDataObject(obj->mesh_params_obj, 0,
                         sizeof(tivx_vpac_ldc_mesh_params_t), &obj->mesh_params,
                         VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);

    /* Block Size parameters */
#ifdef MULTI_REGION
    obj->region_params_obj = vxCreateUserDataObject(obj->context,
                                                    "tivx_vpac_ldc_multi_region_params_t", sizeof(tivx_vpac_ldc_multi_region_params_t),
                                                    NULL);
#else
    obj->region_params_obj = vxCreateUserDataObject(obj->context,
                                                    "tivx_vpac_ldc_region_params_t", sizeof(tivx_vpac_ldc_region_params_t),
                                                    NULL);
#endif    


#ifdef MULTI_REGION
    // for(int ii = 0; ii <3; ii++)
    // {
    //     obj->multi_region_params.reg_height[ii] = obj->table_height/3;
    //     obj->multi_region_params.reg_width[ii] = obj->table_width/3;
    //     for(int jj = 0; jj < 3; jj++)
    //     {
    //         obj->multi_region_params.reg_params[ii][jj].out_block_height =  obj->input_conf.BLOCK_HEIGHT;
    //         obj->multi_region_params.reg_params[ii][jj].out_block_width =  obj->input_conf.BLOCK_WIDTH;
    //         obj->multi_region_params.reg_params[ii][jj].pixel_pad = LDC_PIXEL_PAD;
    //         obj->multi_region_params.reg_params[ii][jj].enable = 1;
    //     }
    // }

    obj->multi_region_params.reg_height[0] = obj->table_height/8;
    obj->multi_region_params.reg_width[0] = obj->table_width/8;
    obj->multi_region_params.reg_height[1] = obj->table_height/8 * 6;
    obj->multi_region_params.reg_width[1] = obj->table_width/8 * 6;
    obj->multi_region_params.reg_height[2] = obj->table_height/8;
    obj->multi_region_params.reg_width[2] = obj->table_width/8;

    for(int ii = 0; ii <3; ii++)
    {
        for(int jj = 0; jj < 3; jj++)
        {
            if(ii != 1 || jj != 1)
            {
                obj->multi_region_params.reg_params[ii][jj].out_block_height =  32;
                obj->multi_region_params.reg_params[ii][jj].out_block_width =  16;
                obj->multi_region_params.reg_params[ii][jj].pixel_pad = LDC_PIXEL_PAD;
                obj->multi_region_params.reg_params[ii][jj].enable = 1;
            }else{
                obj->multi_region_params.reg_params[ii][jj].out_block_height =  64;
                obj->multi_region_params.reg_params[ii][jj].out_block_width =  64;
                obj->multi_region_params.reg_params[ii][jj].pixel_pad = LDC_PIXEL_PAD;
                obj->multi_region_params.reg_params[ii][jj].enable = 1;
            }
        }
    }

    for(int i = 0; i < 3; i++)
    {
        for(int j = 0; j < 3; j++)
            {
                printf("W     H      PAD\n");
                printf("%d    %d      %d\n",obj->multi_region_params.reg_params[i][j].out_block_width,obj->multi_region_params.reg_params[i][j].out_block_height,obj->multi_region_params.reg_params[i][j].pixel_pad);
            }
    }

#else
    obj->region_params.out_block_width  = obj->input_conf.BLOCK_WIDTH;
    obj->region_params.out_block_height = obj->input_conf.BLOCK_HEIGHT;
    obj->region_params.pixel_pad        = 1;    
#endif

#ifdef MULTI_REGION

    vxCopyUserDataObject(obj->region_params_obj, 0,
                         sizeof(tivx_vpac_ldc_multi_region_params_t), &obj->multi_region_params,
                         VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);

#else   
    vxCopyUserDataObject(obj->region_params_obj, 0,
                         sizeof(tivx_vpac_ldc_region_params_t), &obj->region_params,
                         VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);

#endif

    /* LDC Configuration */
    tivx_vpac_ldc_params_init(&obj->ldc_params);
    obj->ldc_params.luma_interpolation_type = 1;

    obj->ldc_param_obj = vxCreateUserDataObject(obj->context,
                                                "tivx_vpac_ldc_params_t", sizeof(tivx_vpac_ldc_params_t), NULL);
    vxCopyUserDataObject(obj->ldc_param_obj, 0, sizeof(tivx_vpac_ldc_params_t),
                         &obj->ldc_params, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);

    // obj->gray_graph_img  = vxCreateImage(obj->context, obj->width, obj->height, VX_DF_IMAGE_U8);
    // obj->gray_out_graph_img = vxCreateImage(obj->context, obj->width, obj->height, VX_DF_IMAGE_U8);
    // char f_path_name[APP_MAX_FILE_PATH];
    // snprintf(f_path_name, APP_MAX_FILE_PATH, "%s/test0.bmp", obj->input_path);
    // status = tivx_utils_load_vximage_from_bmpfile(obj->gray_graph_img, f_path_name, vx_true_e);

    // snprintf(f_path_name, APP_MAX_FILE_PATH, "%s/grayttt_rrr.bmp", obj->output_path);
    // tivx_utils_save_vximage_to_bmpfile(f_path_name, obj->gray_out_graph_img);

    obj->node_left_ldc = tivxVpacLdcNode(obj->graph, obj->ldc_param_obj, NULL,
                                         obj->region_params_obj, obj->mesh_params_obj,
                                         obj->mesh_left_img, NULL, obj->gray_in_left_img[0],
                                         obj->gray_out_left_img[0], 
                                         #ifdef DOUBLE_OUT
                                         obj->node_1_out);
                                         #else
                                         NULL);
                                         #endif
                                         //新增代码
    //obj->mesh_left_img
    vxSetReferenceName((vx_reference)obj->node_left_ldc, "LDC_left_Processing");
    vxSetNodeTarget(obj->node_left_ldc, VX_TARGET_STRING, TIVX_TARGET_VPAC_LDC1);
    #ifdef DOUBLE_OUT
    tivxSetNodeParameterNumBufByIndex(obj->node_left_ldc,8,4);
    #endif

    obj->node_right_ldc = tivxVpacLdcNode(obj->graph, obj->ldc_param_obj, obj->matrix_right,
                                          obj->region_params_obj, NULL,
                                          NULL, NULL, obj->gray_in_left_img[0],
                                          obj->gray_out_right_img[0], 
                                          #ifdef DOUBLE_OUT
                                          obj->node_2_out);
                                          #else
                                          NULL);
                                          #endif
                                          //新增代码
    //obj->mesh_right_img
    vxSetReferenceName((vx_reference)obj->node_right_ldc, "LDC_right_Processing");
    vxSetNodeTarget(obj->node_right_ldc, VX_TARGET_STRING, TIVX_TARGET_VPAC_LDC1);
    #ifdef DOUBLE_OUT
    tivxSetNodeParameterNumBufByIndex(obj->node_right_ldc,8,4);
    #endif

    add_graph_parameter_by_node_index(obj->graph, obj->node_left_ldc, 6);
    add_graph_parameter_by_node_index(obj->graph, obj->node_left_ldc, 7);
    add_graph_parameter_by_node_index(obj->graph, obj->node_right_ldc, 7);
    printf("app_create_graph create ldc node successful\n");

    gp_list[0].graph_parameter_index = 0;
    gp_list[0].refs_list_size = 4;
    gp_list[0].refs_list = (vx_reference *)&obj->gray_in_left_img[0];

    // gp_list[1].graph_parameter_index = 1;
    // gp_list[1].refs_list_size = 4;
    // gp_list[1].refs_list = (vx_reference *)&obj->gray_in_right_img[0];

    gp_list[1].graph_parameter_index = 1;
    gp_list[1].refs_list_size = 4;
    gp_list[1].refs_list = (vx_reference *)&obj->gray_out_left_img[0];

#if GRAPH_PARAM_ONE
    gp_list[2].graph_parameter_index = 2;
    gp_list[2].refs_list_size = 4;
    gp_list[2].refs_list = (vx_reference *)&obj->gray_out_right_img[0];
#endif

    tivxSetGraphPipelineDepth(obj->graph, 4);

    vxSetGraphScheduleConfig(obj->graph,
            VX_GRAPH_SCHEDULE_MODE_QUEUE_AUTO,
#if GRAPH_PARAM_ONE
            3,//新增代码
#else
            3,
#endif
            gp_list
            );

    vxRegisterEvent((vx_reference)obj->graph, VX_EVENT_GRAPH_PARAMETER_CONSUMED, 0, NODE_LDC_LEFT_ENENT);

    vxRegisterEvent((vx_reference)obj->graph, VX_EVENT_GRAPH_PARAMETER_CONSUMED, 1, NODE_LDC_LEFT_OUT_ENENT);
#if GRAPH_PARAM_ONE
    vxRegisterEvent((vx_reference)obj->graph, VX_EVENT_GRAPH_PARAMETER_CONSUMED, 2, NODE_LDC_RIGHT_OUT_ENENT);
#endif

    status = vxVerifyGraph(obj->graph);
    APP_ASSERT(status==VX_SUCCESS);

    printf("app_create_graph over\n");

    return status;
}


int left_out_index = 0;
uint64_t left_average_time = 0;
int right_out_index = 0;
uint64_t right_average_time = 0;
vx_status app_run_graph(AppObj *obj)
{
    vx_status status = VX_FAILURE;

    int run_index = 0;
    uint32_t num_refs = 0;
    int i = 0;

    vx_image temp;

    vx_event_t event;

    char f_path_name[APP_MAX_FILE_PATH];
    char f_path_left[APP_MAX_FILE_PATH];
    char f_path_right[APP_MAX_FILE_PATH];

    for (i = 0; i < 4; ++i)
    {
        if(strncmp(obj->input_conf.INPUT_HOUZHUI,"bmp",4) == 0)
        {
            snprintf(f_path_name, APP_MAX_FILE_PATH, "%stest%d.bmp", obj->input_path,i);
            status = tivx_utils_load_vximage_from_bmpfile(obj->gray_in_left_img[i], f_path_name, vx_true_e);
        }
        else
        {
            snprintf(f_path_name, APP_MAX_FILE_PATH, "%stest%d.%s", obj->input_path,i,obj->input_conf.INPUT_HOUZHUI);
            status = vp_load_binary_to_image(obj->gray_in_left_img[i], f_path_name,obj->input_conf.INPUT_IMAGE_TYPE);
        }
        
        
    }

    for (run_index = 0; run_index < 4; ++run_index)
    {
        printf("enqueue!!!!!!!!!!!!\n");

        vxGraphParameterEnqueueReadyRef(obj->graph, 0, (vx_reference*)&obj->gray_in_left_img[run_index], 1);

        vxGraphParameterEnqueueReadyRef(obj->graph, 1, (vx_reference*)&obj->gray_out_left_img[run_index], 1);
#if GRAPH_PARAM_ONE
        vxGraphParameterEnqueueReadyRef(obj->graph, 2, (vx_reference*)&obj->gray_out_right_img[run_index], 1);
#endif

    }

    vx_reference refs[2] = {0};


    int loop_new = 0;

    int input_index  = 0;
    int output_index = 0;
    vx_enum state=0;

    printf("before graph!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    vxQueryGraph(obj->graph, (vx_enum)VX_GRAPH_STATE, &state, sizeof(vx_enum));
    loop_flag = 0;
    appPerfPointBegin(&obj->total_perf);
    while (loop_flag < 15 && VX_GRAPH_STATE_COMPLETED != state)
    {
        loop_flag++;
        event.app_value = 0;
        #ifdef LOGOUT
        printf("app_run_graph start.\n");
        #endif
        vxWaitEvent(obj->context, &event, vx_false_e);

        
        switch (event.app_value)
        {
         case NODE_LDC_LEFT_ENENT:
            vxGraphParameterDequeueDoneRef(obj->graph, 0, (vx_reference*)&temp, 1, &num_refs);
            #ifdef LOGOUT
            printf("left index = %d, time = %ld\n", input_index++, now_ms());
            #endif
            vxGraphParameterEnqueueReadyRef(obj->graph, 0, (vx_reference*)&temp, 1);
            break;
         case NODE_LDC_LEFT_OUT_ENENT:
            vxGraphParameterDequeueDoneRef(obj->graph, 1, (vx_reference*)&temp, 1, &num_refs);
            loop_new++;
            #ifdef LOGOUT
            printf("left out index = %d, time = %ld \n", left_out_index++, now_ms());
            #endif

            #ifdef SAVE_FILE
            if(strncmp(obj->input_conf.OUT_0_HOUZHUI,"bmp",4) == 0)
            {
                snprintf(f_path_left, APP_MAX_FILE_PATH, "%sout_1/img_A_%d.bmp", obj->ds_output_path, loop_new);
                tivx_utils_save_vximage_to_bmpfile(f_path_left, temp);
            }
            else
            {
                snprintf(f_path_left, APP_MAX_FILE_PATH, "%sout_1/img_A_%d.%s", obj->ds_output_path, loop_new,obj->input_conf.OUT_0_HOUZHUI);
                vp_save_image_to_binary(f_path_left, temp,obj->input_conf.OUT_0_IMAGE_TYPE);
            }
            #endif

            #ifdef DOUBLE_OUT
            #ifdef SAVE_FILE
            if(strncmp(obj->input_conf.OUT_1_HOUZHUI,"bmp",4) == 0)
            {
                snprintf(f_path_left, APP_MAX_FILE_PATH, "%sout_2/img_B_%d.bmp", obj->ds_output_path, loop_new);
                tivx_utils_save_vximage_to_bmpfile(f_path_left, obj->node_1_out);
            }
            else
            {
                snprintf(f_path_left, APP_MAX_FILE_PATH, "%sout_2/img_B_%d.%s", obj->ds_output_path, loop_new,obj->input_conf.OUT_1_HOUZHUI);
                vp_save_image_to_binary(f_path_left, obj->node_1_out,obj->input_conf.OUT_1_IMAGE_TYPE);
            }
            #endif
            #endif
            
            
            vxGraphParameterEnqueueReadyRef(obj->graph, 1, (vx_reference*)&temp, 1);
            break;
#if GRAPH_PARAM_ONE
         case NODE_LDC_RIGHT_OUT_ENENT:
            appPerfPointEnd(&obj->total_perf);
            appPerfPointBegin(&obj->total_perf);
            vxGraphParameterDequeueDoneRef(obj->graph, 2, (vx_reference*)&temp, 1, &num_refs);

            #ifdef LOGOUT
            printf("right out index = %d, time = %ld \n", right_out_index++, now_ms());
            #endif

            #ifdef SAVE_FILE
            if(strncmp(obj->input_conf.OUT_0_HOUZHUI,"bmp",4) == 0)
            {
                snprintf(f_path_left, APP_MAX_FILE_PATH, "%sout_1/img_A_%d.bmp", obj->rm_output_path, loop_new);
                tivx_utils_save_vximage_to_bmpfile(f_path_left, temp);
            }
            else
            {
                snprintf(f_path_left, APP_MAX_FILE_PATH, "%sout_1/img_A_%d.%s", obj->rm_output_path, loop_new,obj->input_conf.OUT_0_HOUZHUI);
                vp_save_image_to_binary(f_path_left, temp,obj->input_conf.OUT_0_IMAGE_TYPE);
            }
            #endif
            #ifdef DOUBLE_OUT
            #ifdef SAVE_FILE
            if(strncmp(obj->input_conf.OUT_1_HOUZHUI,"bmp",4) == 0)
            {
                snprintf(f_path_left, APP_MAX_FILE_PATH, "%sout_2/img_B_%d.bmp", obj->rm_output_path, loop_new);
                tivx_utils_save_vximage_to_bmpfile(f_path_left, obj->node_2_out);
            }
            else
            {
                snprintf(f_path_left, APP_MAX_FILE_PATH, "%sout_2/img_B_%d.%s", obj->rm_output_path, loop_new,obj->input_conf.OUT_1_HOUZHUI);
                vp_save_image_to_binary(f_path_left, obj->node_2_out,obj->input_conf.OUT_1_IMAGE_TYPE);
            }
            #endif
            #endif
            
            vxGraphParameterEnqueueReadyRef(obj->graph, 2, (vx_reference*)&temp, 1);
            if(loop_flag % 100 == 0)
            {
                appPerfPointPrint(&obj->total_perf);
                tivx_utils_graph_perf_print(obj->graph);
            }
            
            break;
#endif

        }       
    }

    

    vxWaitGraph(obj->graph);
    printf("after graph!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    while(vxWaitEvent(obj->context, &event, vx_true_e) == VX_SUCCESS);
    printf("after QQQQQ graph!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!ttttttttttt!!!!!!!\n");

    return status;
}

#if 0
vx_status app_run_graph(AppObj *obj)
{
    vx_status status = VX_FAILURE;

    int run_index = 0;

    char f_path_name[APP_MAX_FILE_PATH];
    char f_path_left[APP_MAX_FILE_PATH];
    char f_path_right[APP_MAX_FILE_PATH];

    for (run_index = 0; run_index < 1; ++run_index)
    {
        //snprintf(f_path_name, APP_MAX_FILE_PATH, "%s/test%d.bmp", obj->input_path, run_index);


        snprintf(f_path_name, APP_MAX_FILE_PATH, "%s/1920_l.bmp", obj->input_path);
        status = tivx_utils_load_vximage_from_bmpfile(obj->gray_in_left_img, f_path_name, vx_true_e);

        snprintf(f_path_name, APP_MAX_FILE_PATH, "%s/1920_r.bmp", obj->input_path);
        status = tivx_utils_load_vximage_from_bmpfile(obj->gray_in_right_img, f_path_name, vx_true_e);

        status = vxScheduleGraph(obj->graph);
        APP_ASSERT(VX_SUCCESS == status);

        status = vxWaitGraph(obj->graph);
        APP_ASSERT(VX_SUCCESS == status);

        snprintf(f_path_left, APP_MAX_FILE_PATH, "%s/gray_left.bmp", obj->output_path);
        snprintf(f_path_right, APP_MAX_FILE_PATH, "%s/gray_right.bmp", obj->output_path);

        tivx_utils_save_vximage_to_bmpfile(f_path_left, obj->gray_out_left_img);
        tivx_utils_save_vximage_to_bmpfile(f_path_right, obj->gray_out_right_img);
    }

    return status;
}
#endif
vx_status app_delete_graph(AppObj *obj)
{
    vx_status status = VX_FAILURE;

    if (NULL != obj->mesh_left_img)
    {
        APP_PRINTF("releasing mesh_left_img Image \n");
        status |= vxReleaseImage(&obj->mesh_left_img);
    }

    if (NULL != obj->matrix_right)
    {
        APP_PRINTF("releasing matrix_right Image \n");
        status |= vxReleaseMatrix(&obj->matrix_right);
    }

    #ifdef DOUBLE_OUT
    if (NULL != obj->node_1_out)
    {
        APP_PRINTF("releasing node_1_out Image \n");
        status |= vxReleaseImage(&obj->node_1_out);
    }

    if (NULL != obj->node_2_out)
    {
        APP_PRINTF("releasing node_2_out Image \n");
        status |= vxReleaseImage(&obj->node_2_out);
    }
    #endif

    for(int i = 0; i < 4; i++)
    {
    if (NULL != obj->gray_in_left_img[i])
    {
        APP_PRINTF("releasing gray_in_left_img Image \n");
        status |= vxReleaseImage(&obj->gray_in_left_img[i]);
    }

        if (NULL != obj->gray_out_left_img[i])
    {
        APP_PRINTF("releasing gray_out_left_img Image \n");
        status |= vxReleaseImage(&obj->gray_out_left_img[i]);
    }

    if (NULL != obj->gray_out_right_img[i])
    {
        APP_PRINTF("releasing gray_out_right_img Image \n");
        status |= vxReleaseImage(&obj->gray_out_right_img[i]);
    }
    }

    if (NULL != obj->mesh_params_obj)
    {
        APP_PRINTF("releasing LDC Mesh Parameters Object\n");
        status |= vxReleaseUserDataObject(&obj->mesh_params_obj);
    }

    if (NULL != obj->ldc_param_obj)
    {
        APP_PRINTF("releasing LDC Parameters Object\n");
        status |= vxReleaseUserDataObject(&obj->ldc_param_obj);
    }

    if (NULL != obj->region_params_obj)
    {
        APP_PRINTF("releasing LDC Region Parameters Object\n");
        status |= vxReleaseUserDataObject(&obj->region_params_obj);
    }

    if (NULL != obj->node_left_ldc)
    {
        APP_PRINTF("releasing node_left_ldc Node \n");
        status |= vxReleaseNode(&obj->node_left_ldc);
    }

    if(NULL != obj->node_right_ldc)
    {
        APP_PRINTF("releasing node_right_ldc Node \n");
        status |= vxReleaseNode(&obj->node_right_ldc);
    }

    APP_PRINTF("releasing graph\n");
    status |= vxReleaseGraph(&obj->graph);

    return status;
}

vx_status app_load_vximage_mesh_from_text_file(char *filename, vx_image image)
{
    vx_uint32 width, height;
    vx_imagepatch_addressing_t image_addr;
    vx_rectangle_t rect;
    vx_map_id map_id;
    vx_df_image df;
    void *data_ptr;
    vx_status status;

    status = vxGetStatus((vx_reference)image);
    if(status==VX_SUCCESS)
    {
        vxQueryImage(image, VX_IMAGE_WIDTH, &width, sizeof(vx_uint32));
        vxQueryImage(image, VX_IMAGE_HEIGHT, &height, sizeof(vx_uint32));
        vxQueryImage(image, VX_IMAGE_FORMAT, &df, sizeof(vx_df_image));

        rect.start_x = 0;
        rect.start_y = 0;
        rect.end_x = width;
        rect.end_y = height;

        status = vxMapImagePatch(image,
            &rect,
            0,
            &map_id,
            &image_addr,
            &data_ptr,
            VX_WRITE_ONLY,
            VX_MEMORY_TYPE_HOST,
            VX_NOGAP_X
            );

#if 0
        int x,y;
        for(y=0; y < image_addr.dim_y; y++)
        {
            uint32_t *data = (uint32_t*)data_ptr + (image_addr.stride_y/4)*y;
            for(x=0; x < image_addr.dim_x; x++)
            {
                data[x] =  0;
            }
        }
#else
        if(status==VX_SUCCESS)
        {
            FILE *fp = fopen(filename,"r");
            if(fp!=NULL)
            {
                size_t ret;
                int x,y,xoffset,yoffset;

                for(y=0; y < image_addr.dim_y; y++)
                {
                    uint32_t *data = (uint32_t*)data_ptr + (image_addr.stride_y/4)*y;
                    for(x=0; x < image_addr.dim_x; x++)
                    {
                        ret = fscanf(fp, "%d %d", &xoffset, &yoffset);

                        if(ret!=2)
                        {
                            printf("# ERROR: Unable to read data from file [%s]\n", filename);
                        }
                        data[x] = (xoffset << 16) | (yoffset & 0x0ffff);
                    }
                }
                fclose(fp);
            }
            else
            {
                printf("# ERROR: Unable to open file for reading [%s]\n", filename);
                status = VX_FAILURE;
            }
            vxUnmapImagePatch(image, map_id);
        }
#endif
    }
    return status;
}

void show_image_attributes(vx_image image)
{
    vx_uint32 width=0, height=0, ref_count=0;
    vx_df_image df=0;
    vx_size num_planes=0, size=0;
    vx_enum color_space=0, channel_range=0, memory_type=0;
    vx_char *ref_name=NULL;
    char df_name[MAX_ATTRIBUTE_NAME];
    char color_space_name[MAX_ATTRIBUTE_NAME];
    char channel_range_name[MAX_ATTRIBUTE_NAME];
    char memory_type_name[MAX_ATTRIBUTE_NAME];
    char ref_name_invalid[MAX_ATTRIBUTE_NAME];

    /** - Query image attributes.
     *
     *  Queries image for width, height, format, planes, size, space, range,
     *  range and memory type.
     *
     * \code
     */
    vxQueryImage(image, VX_IMAGE_WIDTH, &width, sizeof(vx_uint32));
    vxQueryImage(image, VX_IMAGE_HEIGHT, &height, sizeof(vx_uint32));
    vxQueryImage(image, VX_IMAGE_FORMAT, &df, sizeof(vx_df_image));
    vxQueryImage(image, VX_IMAGE_PLANES, &num_planes, sizeof(vx_size));
    vxQueryImage(image, VX_IMAGE_SIZE, &size, sizeof(vx_size));
    vxQueryImage(image, VX_IMAGE_SPACE, &color_space, sizeof(vx_enum));
    vxQueryImage(image, VX_IMAGE_RANGE, &channel_range, sizeof(vx_enum));
    vxQueryImage(image, VX_IMAGE_MEMORY_TYPE, &memory_type, sizeof(vx_enum));
    /** \endcode */

    vxQueryReference((vx_reference)image, VX_REFERENCE_NAME, &ref_name, sizeof(vx_char*));
    vxQueryReference((vx_reference)image, VX_REFERENCE_COUNT, &ref_count, sizeof(vx_uint32));

    switch(df)
    {
        case VX_DF_IMAGE_VIRT:
            strncpy(df_name, "VX_DF_IMAGE_VIRT", MAX_ATTRIBUTE_NAME);
            break;
        case VX_DF_IMAGE_RGB:
            strncpy(df_name, "VX_DF_IMAGE_RGB", MAX_ATTRIBUTE_NAME);
            break;
        case VX_DF_IMAGE_RGBX:
            strncpy(df_name, "VX_DF_IMAGE_RGBX", MAX_ATTRIBUTE_NAME);
            break;
        case VX_DF_IMAGE_NV12:
            strncpy(df_name, "VX_DF_IMAGE_NV12", MAX_ATTRIBUTE_NAME);
            break;
        case VX_DF_IMAGE_NV21:
            strncpy(df_name, "VX_DF_IMAGE_NV21", MAX_ATTRIBUTE_NAME);
            break;
        case VX_DF_IMAGE_UYVY:
            strncpy(df_name, "VX_DF_IMAGE_UYVY", MAX_ATTRIBUTE_NAME);
            break;
        case VX_DF_IMAGE_YUYV:
            strncpy(df_name, "VX_DF_IMAGE_YUYV", MAX_ATTRIBUTE_NAME);
            break;
        case VX_DF_IMAGE_IYUV:
            strncpy(df_name, "VX_DF_IMAGE_IYUV", MAX_ATTRIBUTE_NAME);
            break;
        case VX_DF_IMAGE_YUV4:
            strncpy(df_name, "VX_DF_IMAGE_YUV4", MAX_ATTRIBUTE_NAME);
            break;
        case VX_DF_IMAGE_U8:
            strncpy(df_name, "VX_DF_IMAGE_U8", MAX_ATTRIBUTE_NAME);
            break;
        case VX_DF_IMAGE_U16:
            strncpy(df_name, "VX_DF_IMAGE_U16", MAX_ATTRIBUTE_NAME);
            break;
        case VX_DF_IMAGE_S16:
            strncpy(df_name, "VX_DF_IMAGE_S16", MAX_ATTRIBUTE_NAME);
            break;
        case VX_DF_IMAGE_U32:
            strncpy(df_name, "VX_DF_IMAGE_U32", MAX_ATTRIBUTE_NAME);
            break;
        case VX_DF_IMAGE_S32:
            strncpy(df_name, "VX_DF_IMAGE_S32", MAX_ATTRIBUTE_NAME);
            break;
        default:
            strncpy(df_name, "VX_DF_IMAGE_UNKNOWN", MAX_ATTRIBUTE_NAME);
            break;
    }

    switch(color_space)
    {
        case VX_COLOR_SPACE_NONE:
            strncpy(color_space_name, "VX_COLOR_SPACE_NONE", MAX_ATTRIBUTE_NAME);
            break;
        case VX_COLOR_SPACE_BT601_525:
            strncpy(color_space_name, "VX_COLOR_SPACE_BT601_525", MAX_ATTRIBUTE_NAME);
            break;
        case VX_COLOR_SPACE_BT601_625:
            strncpy(color_space_name, "VX_COLOR_SPACE_BT601_625", MAX_ATTRIBUTE_NAME);
            break;
        case VX_COLOR_SPACE_BT709:
            strncpy(color_space_name, "VX_COLOR_SPACE_BT709", MAX_ATTRIBUTE_NAME);
            break;
        default:
            strncpy(color_space_name, "VX_COLOR_SPACE_UNKNOWN", MAX_ATTRIBUTE_NAME);
            break;
    }

    switch(channel_range)
    {
        case VX_CHANNEL_RANGE_FULL:
            strncpy(channel_range_name, "VX_CHANNEL_RANGE_FULL", MAX_ATTRIBUTE_NAME);
            break;
        case VX_CHANNEL_RANGE_RESTRICTED:
            strncpy(channel_range_name, "VX_CHANNEL_RANGE_RESTRICTED", MAX_ATTRIBUTE_NAME);
            break;
        default:
            strncpy(channel_range_name, "VX_CHANNEL_RANGE_UNKNOWN", MAX_ATTRIBUTE_NAME);
            break;
    }

    switch(memory_type)
    {
        case VX_MEMORY_TYPE_NONE:
            strncpy(memory_type_name, "VX_MEMORY_TYPE_NONE", MAX_ATTRIBUTE_NAME);
            break;
        case VX_MEMORY_TYPE_HOST:
            strncpy(memory_type_name, "VX_MEMORY_TYPE_HOST", MAX_ATTRIBUTE_NAME);
            break;
        default:
            strncpy(memory_type_name, "VX_MEMORY_TYPE_UNKNOWN", MAX_ATTRIBUTE_NAME);
            break;
    }

    if(ref_name==NULL)
    {
        strncpy(ref_name_invalid, "INVALID_REF_NAME", MAX_ATTRIBUTE_NAME);
        ref_name = &ref_name_invalid[0];
    }

    printf(" VX_TYPE_IMAGE: %s, %d x %d, %d plane(s), %d B, %s %s %s %s, %d refs\n",
        ref_name,
        width,
        height,
        (uint32_t)num_planes,
        (uint32_t)size,
        df_name,
        color_space_name,
        channel_range_name,
        memory_type_name,
        ref_count
        );
}

static void app_parse_cfg_file(AppObj *obj, char *cfg_file_name)
{
    FILE *fp = fopen(cfg_file_name, "r");
    char line_str[1024];
    char *token;

    if(fp==NULL)
    {
        printf("# ERROR: Unable to open config file [%s].\n", cfg_file_name);
        return;
    }

    while(fgets(line_str, sizeof(line_str), fp)!=NULL)
    {
        char s[]=" \t";

        if (strchr(line_str, '#'))
        {
            continue;
        }

        /* get the first token */
        token = strtok(line_str, s);

        if (NULL == token)
        {
            continue;
        }

        if(strcmp(token, "input_path")==0)
        {
            token = strtok(NULL, s);
            token[strlen(token) - 1] = 0;
            strcpy(obj->input_path, token);
            //printf("app_parse_cfg_file in file path = %s\n", obj->input_path);
        }
        else
        if(strcmp(token, "ds_output_path")==0)
        {
            token = strtok(NULL, s);
            token[strlen(token) - 1] = 0;
            strcpy(obj->ds_output_path, token);
            //printf("app_parse_cfg_file: out file path = %s\n", obj->output_path);
        }
        else
        if(strcmp(token, "rm_output_path")==0)
        {
            token = strtok(NULL, s);
            token[strlen(token) - 1] = 0;
            strcpy(obj->rm_output_path, token);
            //printf("app_parse_cfg_file: out file path = %s\n", obj->output_path);
        }
        else
        if(strcmp(token, "num_frames_to_run")==0)
        {
            token = strtok(NULL, s);
            obj->num_frames_to_run = atoi(token);
            //printf("app_parse_cfg_file: num_frames_to_run = [%d]\n", obj->num_frames_to_run);
        }
        else
        {
            APP_PRINTF("Invalid token [%s]\n", token);
        }
    }

    fclose(fp);

}

static void app_show_usage(int argc, char* argv[])
{
    printf("\n");
    printf("                        LDC Demo                         \n");
    printf(" ========================================================\n");
    printf("\n");
    printf(" Usage,\n");
    printf("  %s [--cfg <config file>]\n", argv[0]);
    printf("\n");
}

void app_set_cfg_default(AppObj *obj)
{
    snprintf(obj->input_path, APP_MAX_FILE_PATH, obj->input_conf.INPUT_PATH);
    snprintf(obj->rm_output_path, APP_MAX_FILE_PATH, obj->input_conf.RM_OUTPUT_PATH);
    snprintf(obj->ds_output_path, APP_MAX_FILE_PATH, obj->input_conf.DS_OUTPUT_PATH);
    obj->num_frames_to_run = 10;
}



int vx_DF_res(const char *vx_DF_img)
{
    printf("shit!!!!!!!!!!!!!!!!!!!! %s\n",vx_DF_img);
    if(strncmp(vx_DF_img,"VX_DF_IMAGE_U8",15) == 0)
    {
        return VX_DF_IMAGE('U','0','0','8');
    }
    if(strncmp(vx_DF_img,"VX_DF_IMAGE_U16",16) == 0)
    {
        return VX_DF_IMAGE('U','0','1','6');
    }
    if(strncmp(vx_DF_img,"VX_DF_IMAGE_UYVY",17) == 0)
    {
        return VX_DF_IMAGE('U','Y','V','Y');
    }
    if(strncmp(vx_DF_img,"VX_DF_IMAGE_YUYV",17) == 0)
    {
        return VX_DF_IMAGE('Y','U','Y','V');
    }
    if(strncmp(vx_DF_img,"VX_DF_IMAGE_NV12",17) == 0)
    {
        return VX_DF_IMAGE('N','V','1','2');
    }
    if(strncmp(vx_DF_img,"TIVX_DF_IMAGE_NV12_P12",23) == 0)
    {
        return VX_DF_IMAGE('N','1','2','P');
    }

    return -1;

}

/*

*/

vx_status app_read_config_file(AppObj *obj, const char* file_path)
{
    FILE *fp = NULL;
    fp = fopen(file_path,"r");
    if(fp == NULL)
    {
        printf("read file error %s!!!\n",file_path);
        return -1;
    }
    char INPUT_IMAGE_TYPE[30];
    char OUT_0_IMAGE_TYPE[30];
    char OUT_1_IMAGE_TYPE[30];
    
    fscanf(fp,"INPUT_IMAGE_TYPE: %s\n",INPUT_IMAGE_TYPE);
    
    if(vx_DF_res(INPUT_IMAGE_TYPE) == -1)
    {
        printf("ERORR vx_IMAGE_TYPE %s\n",INPUT_IMAGE_TYPE);
        return -1;
    }
    obj->input_conf.INPUT_IMAGE_TYPE = vx_DF_res(INPUT_IMAGE_TYPE);
    fscanf(fp,"INPUT_HOUZHUI: %s\n",obj->input_conf.INPUT_HOUZHUI);
    fscanf(fp,"INPUT_PATH: %s\n",obj->input_conf.INPUT_PATH);
    fscanf(fp,"OUT_0_IMAGE_TYPE: %s\n",OUT_0_IMAGE_TYPE);
    if(vx_DF_res(OUT_0_IMAGE_TYPE) == -1)
    {
        printf("ERORR vx_IMAGE_TYPE %s\n",OUT_0_IMAGE_TYPE);
        return -1;
    }
    obj->input_conf.OUT_0_IMAGE_TYPE = vx_DF_res(OUT_0_IMAGE_TYPE);
    fscanf(fp,"OUT_0_HOUZHUI: %s\n",obj->input_conf.OUT_0_HOUZHUI);
    fscanf(fp,"OUT_1_IMAGE_TYPE: %s\n",OUT_1_IMAGE_TYPE);
    if(vx_DF_res(OUT_1_IMAGE_TYPE) == -1)
    {
        printf("ERORR vx_IMAGE_TYPE %s\n",OUT_1_IMAGE_TYPE);
        return -1;
    }
    obj->input_conf.OUT_1_IMAGE_TYPE = vx_DF_res(OUT_1_IMAGE_TYPE);

    fscanf(fp,"OUT_1_HOUZHUI: %s\n",obj->input_conf.OUT_1_HOUZHUI);
    fscanf(fp,"RM_OUTPUT_PATH: %s\n",obj->input_conf.RM_OUTPUT_PATH);
    fscanf(fp,"DS_OUTPUT_PATH: %s\n",obj->input_conf.DS_OUTPUT_PATH);

    fscanf(fp,"LDC_TABLE_WIDTH: %d\n",&obj->input_conf.LDC_TABLE_WIDTH);
    fscanf(fp,"LDC_TABLE_HEIGHT: %d\n",&obj->input_conf.LDC_TABLE_HEIGHT);
    fscanf(fp,"LDC_WIDTH: %d\n",&obj->input_conf.LDC_WIDTH);
    fscanf(fp,"LDC_HEIGHT: %d\n",&obj->input_conf.LDC_HEIGHT);
    fscanf(fp,"BLOCK_WIDTH: %d\n",&obj->input_conf.BLOCK_WIDTH);
    fscanf(fp,"BLOCK_HEIGHT: %d\n",&obj->input_conf.BLOCK_HEIGHT);

}

void show_obj_info(AppObj *obj)
{
    printf("-------------------------CONFIG INFO----------------------------\n");
    printf("INPUT PATH: %s\n",obj->input_conf.INPUT_PATH);
    printf("INPUT HOUZHUI: %s\n",obj->input_conf.INPUT_HOUZHUI);
    printf("INPUT TYPE: %d\n",obj->input_conf.INPUT_IMAGE_TYPE);
    printf("HEIGHT: %d\n",obj->input_conf.LDC_HEIGHT);
    printf("WIDTH: %d\n",obj->input_conf.LDC_WIDTH);
    printf("OUT_0_HOUZHUI: %s\n",obj->input_conf.OUT_0_HOUZHUI);
    printf("OUT_0 TYPE: %d\n",obj->input_conf.OUT_0_IMAGE_TYPE);
    printf("OUT_1 HOUZHUI: %s\n",obj->input_conf.OUT_1_HOUZHUI);
    printf("OUT_1 TYPE: %d\n",obj->input_conf.OUT_1_IMAGE_TYPE);
}

vx_status app_parse_cmd_line_args(AppObj *obj, int argc, char *argv[])
{
    int i;
    if(argc==2)
    {
        if(app_read_config_file(obj,argv[1]) == -1)
        {
            return -1;
        }
        show_obj_info(obj);
        app_set_cfg_default(obj);
        app_show_usage(argc, argv);
        printf("Defaulting mode \n");
        return VX_SUCCESS;
    }

    // for(i=0; i<argc; i++)
    // {
    //     if(strcmp(argv[i], "--cfg")==0)
    //     {
    //         i++;
    //         if(i>=argc)
    //         {
    //             app_show_usage(argc, argv);
    //         }
    //         app_parse_cfg_file(obj, argv[i]);
    //         break;
    //     }
    //     else
    //     if(strcmp(argv[i], "--help")==0)
    //     {
    //         app_show_usage(argc, argv);
    //         return VX_FAILURE;
    //     }
    // }
    return VX_SUCCESS;
}

/*
Copyright (c) 2018, Digital Media Professionals. All rights reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include "Contours.h"

namespace
{

    typedef int8_t i8;
    typedef short i16;
    typedef int i32;
    typedef unsigned char u8;
    typedef unsigned short u16;
    typedef unsigned int u32;

#define CLAMP(val, a, b) ((val) < (a) ? (a) : ((val) > (b) ? (b) : (val)))

    // halffloat
    // exp11: 0.06256 - 0.12494 (16 - 32)
    // exp12: 0.12512 - 0.24988 (32 - 64)
    // threshold = 32
    // exp<12 ? 0 : 255
#define halfp2uint8(hp) ((((hp) & 0x7c00u) == 0 || ((hp) & 0x8000u) || (((hp) & 0x7c00u) < (12 << 10))) ? 0 : 255)
}

static bool operator==(const Point& t0, const Point& t1) {
    return t0.x == t1.x && t0.y == t1.y;
}


// エッジ検出
static void findContour(std::vector<Point> &contour, const u8 *image, int width, int height, Point &first)
{
    static const i8 mat_x[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };
    static const i8 mat_y[8] = { 0, -1, -1, -1, 0, 1, 1, 1 };
#define GET_PT(i, x, y) Point(x + mat_x[i], y + mat_y[i])

    contour.push_back(first);
    int x = first.x;
    int y = first.y;
    const u8 *dst = &image[x + width * y];

    bool ret = true;
    while (ret)
    {
        ret = false;

        //  c1  c2  c3
        //  c0 [pt] c4
        //  c7  c6  c5

        u8 c[8] = { 0 };
        if (x != 0)
        {
            c[1] = y != 0 ? *(dst - width - 1) : 0;
            c[0] = *(dst - 1);
            c[7] = y != height ? *(dst + width - 1) : 0;
        }
        if (x != width)
        {
            c[3] = y != 0 ? *(dst - width + 1) : 0;
            c[4] = *(dst + 1);
            c[5] = y != height ? *(dst + width + 1) : 0;
        }
        c[2] = y != 0 ? *(dst - width) : 0;
        c[6] = y != height ? *(dst + width) : 0;

        for (int i = 0; i < 8; i++)
        {
            if (c[i] && c[(i + 1) % 8] == 0)
            {
                Point pt = GET_PT(i, x, y);
                bool has_prev_same_pt = false;
                for (int j = (int)contour.size() - 2; j >= 0; j--)
                {
                    if (contour[j] == pt)
                    {
                        has_prev_same_pt = true;
                        break;
                    }
                }

                if (!has_prev_same_pt)
                {
                    contour.push_back(pt);
                    x = pt.x;
                    y = pt.y;
                    ret = true;
                    dst = &image[x + width * y];
                    break;
                }
            }
        }
    }
}

// 抽出したポイントのうち、まだソートしていないものを取得
static bool get_not_filled_point(std::vector<Point> &sorted_pts, std::vector<Point> &src_pts, Point &pt)
{
    for (u32 i = 0; i < src_pts.size(); i++)
    {
        bool found = false;

        for (u32 j = 0; j < sorted_pts.size(); j++)
        {
            if (src_pts[i] == sorted_pts[j])
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            int x0 = src_pts[i].x;
            int y0 = src_pts[i].y;
            int y_candidate[] = { y0 - 2, y0 - 1, y0 + 1, y0 + 2 };
            int x_candidate[] = { x0 - 2, x0 - 1, x0 + 1, x0 + 2 };

            for (u32 j = 0; !found && j < sorted_pts.size(); j++)
            {
                for (int y_idx = 0; !found && y_idx < 4; y_idx++)
                {
                    for (int x_idx = 0; x_idx < 4; x_idx++)
                    {
                        if (sorted_pts[j].x == x_candidate[x_idx] && sorted_pts[j].y == y_candidate[y_idx])
                        {
                            found = true;
                            break;
                        }
                    }
                }
            }
            if (!found)
            {
                pt = src_pts[i];
                return true;
            }
        }
    }

    return false;
}

// image: type: float16
bool contours(void *image, int width, int height, std::vector<Point> &sorted_pts)
{
    std::vector<Point> pts;
    i16 *p16 = (i16*)image;

    u8 *dst = (u8*)malloc(width * height * sizeof(u8));
    if (!dst)
        return false;

    u8 *p8 = dst;
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            i16 fp16 = p16[x + width * y];
            *p8++ = halfp2uint8(fp16);
            if (x > 0 && *(p8 - 2) != *(p8 - 1))
            {
                if (*(p8 - 2))
                {
                    Point pt(x - 1, y);
                    pts.push_back(pt);
                }
                else
                {
                    Point pt(x, y);
                    pts.push_back(pt);
                }
            }
        }
    }

#if 0
    {
      FILE* fp = fopen("dump.raw", "wb");
      fwrite(dst, width * height, 1, fp);
      fclose(fp);
    }
#endif

    if (pts.size() > 10)
    {
        size_t best_pts_len = 0;

        std::vector<Point> pts_all;
        for (int n = 0;; n++)
        {
            Point pt(0, 0);
            if (!get_not_filled_point(pts_all, pts, pt))
            {
                break;
            }
            std::vector<Point> contour;
            findContour(contour, dst, width, height, pt);
            if (best_pts_len < contour.size())
            {
                best_pts_len = contour.size();
                sorted_pts = contour;
            }
            for (u32 i = 0; i < contour.size(); i++)
                pts_all.push_back(contour[i]);
        }
    }

    free(dst);
    return true;
}

int get_edge_point(std::vector<Point> &simple, Point *left, Point *right)
{
    Point edge_pts[4];
    int indices[4] = { 0 };
    enum EDGE_PTS_STATE {
        EDGE_Y_MIN_L = 0,
        EDGE_Y_MIN_R,
        EDGE_X_MIN,
        EDGE_X_MAX,
    };
    edge_pts[EDGE_Y_MIN_L].y = 0x1000;
    edge_pts[EDGE_Y_MIN_R].y = 0x1000;
    edge_pts[EDGE_X_MIN].x = 0x1000;

    // close points loop
    simple.push_back( simple.front() );

    for (int i = 0; i < (int)simple.size(); i++)
    {
        if (simple[i].y < edge_pts[EDGE_Y_MIN_L].y)
        {
            edge_pts[EDGE_Y_MIN_L] = simple[i];
            indices[EDGE_Y_MIN_L] = i;
        }
        if (simple[i].y <= edge_pts[EDGE_Y_MIN_R].y)
        {
            edge_pts[EDGE_Y_MIN_R] = simple[i];
            indices[EDGE_Y_MIN_R] = i;
        }
        if (simple[i].x < edge_pts[EDGE_X_MIN].x)
        {
            edge_pts[EDGE_X_MIN] = simple[i];
            indices[EDGE_X_MIN] = i;
        }
        if (simple[i].x > edge_pts[EDGE_X_MAX].x)
        {
            edge_pts[EDGE_X_MAX] = simple[i];
            indices[EDGE_X_MAX] = i;
        }
    }
    bool is_valid_right = false;
    bool is_valid_left = false;

    // right
    if (indices[EDGE_Y_MIN_R] > indices[EDGE_X_MAX])
    {
        int i;
        int r = (indices[EDGE_Y_MIN_R] - indices[EDGE_X_MAX]) / (RESULT_POINT_LEN - 1);
        for (i = 0; i < RESULT_POINT_LEN - 1; i++)
            right[i] = simple[indices[EDGE_X_MAX] + r * i];
        right[i] = simple[indices[EDGE_Y_MIN_R] - 1];
        
        if (right[0].x || right[0].y || right[RESULT_POINT_LEN - 1].x || right[RESULT_POINT_LEN - 1].y)
            is_valid_right = true;
    }
    
    // left
    if (indices[EDGE_X_MIN] > indices[EDGE_Y_MIN_L])
    {
        int r = (indices[EDGE_X_MIN] - indices[EDGE_Y_MIN_L]) / (RESULT_POINT_LEN - 1);
        for (int i = 0; i < RESULT_POINT_LEN - 1; i++)
            left[RESULT_POINT_LEN - i - 1] = simple[indices[EDGE_Y_MIN_L] + r * i];
        left[0] = simple[indices[EDGE_X_MIN]];
        
        if (left[0].x || left[0].y || left[RESULT_POINT_LEN - 1].x || left[RESULT_POINT_LEN - 1].y)
            is_valid_left = true;
    }
    
    return (is_valid_right ? 2 : 0) | (is_valid_left ? 1 : 0);
}

void fp16x2_transpose(const void *image, void* buffer, int width, int height, int bpp)
{
    unsigned int *src = (unsigned int *)image;
    unsigned short *dst = (unsigned short *)buffer; //malloc(width * height * 2);
    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
	  //dst[y * width + (width - x - 1)] = (unsigned short)(src[y + x * height] & 0xffff);
	  dst[y * width + x] = (unsigned short)(src[y + x * height] & 0xffff);
        }
    }

    memcpy(src, dst, width * height * 2);
    //free(dst);
}

/*
 *  Copyright 2018 Digital Media Professionals Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "util_draw.h"
#include "LaneDetection_param.h"

using namespace std;
using namespace dmp;
using namespace util;

#define AVE_LEN 6

typedef int8_t i8;
typedef short i16;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

struct Point
{
  int x;
  int y;

  Point(int _x, int _y) : x(_x), y(_y) {}
  Point() : x(0), y(0) {}
  const Point& operator=(const Point& t1)
  {
    x = t1.x;
    y = t1.y;
    return t1;
  }
};
static bool operator==(const Point& t0, const Point& t1) {
  return t0.x == t1.x && t0.y == t1.y;
}

vector<Point> sorted_pts;
vector<Point> ave_top;
vector<Point> ave_left;
vector<Point> ave_right;
vector<Point> ave_center;
vector<Point> lpts, rpts;

static u8 binary_buffer[ PROC_W * PROC_H ];

static void findContour(std::vector<Point> &contour, Point &first)
{
  static const i8 mat_x[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };
  static const i8 mat_y[8] = { 0, -1, -1, -1, 0, 1, 1, 1 };
#define GET_PT(i, x, y) Point(x + mat_x[i], y + mat_y[i])

  contour.push_back(first);
  int x = first.x;
  int y = first.y;
  const u8 *pix = &binary_buffer[x + PROC_W * y];

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
	  c[1] = y != 0 ? *(pix - PROC_W - 1) : 0;
	  c[0] = *(pix - 1);
	  c[7] = y != PROC_H ? *(pix + PROC_W - 1) : 0;
        }
      if (x != PROC_W)
        {
	  c[3] = y != 0 ? *(pix - PROC_W + 1) : 0;
	  c[4] = *(pix + 1);
	  c[5] = y != PROC_H ? *(pix + PROC_W + 1) : 0;
        }
      c[2] = y != 0 ? *(pix - PROC_W) : 0;
      c[6] = y != PROC_H ? *(pix + PROC_W) : 0;

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
		  pix = &binary_buffer[x + PROC_W * y];
		  break;
                }
            }
        }
    }
}

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

// halffloat
// exp11: 0.06256 - 0.12494 (16 - 32)
// exp12: 0.12512 - 0.24988 (32 - 64)
// threshold = 32
// exp<12 ? 0 : 255
#define halfp2uint8(hp) ((((hp) & 0x7c00u) == 0 || ((hp) & 0x8000u) || (((hp) & 0x7c00u) < (12 << 10))) ? 0 : 255)

// buffer: type: float16
static bool contours(void *buffer, std::vector<Point> &sorted_pts)
{
  std::vector<Point> pts;
  i16 *p16 = (i16*)buffer;

  u8 *p8 = binary_buffer;
  for (int y = 0; y < PROC_H; y++)
    {
      for (int x = 0; x < PROC_W; x++)
        {
	  i16 fp16 = p16[x + PROC_W * y];
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
	  findContour(contour, pt);
	  if (best_pts_len < contour.size())
            {
	      best_pts_len = contour.size();
	      sorted_pts = contour;
            }
	  for (u32 i = 0; i < contour.size(); i++)
	    pts_all.push_back(contour[i]);
        }
    }

  return true;
}

inline int getPt( int n1 , int n2 , float perc )
{
  int diff = n2 - n1;
  return n1 + ( diff * perc );
}

static void curve(Point& p1, Point& p2, Point& p3, vector<Point>& pts)
{
  for( float i = 0 ; i < 1.0f ; i += 0.1f )
    {
      int xa = getPt( p1.x , p2.x , i );
      int ya = getPt( p1.y , p2.y , i );
      int xb = getPt( p2.x , p3.x , i );
      int yb = getPt( p2.y , p3.y , i );

      int x = getPt( xa , xb , i );
      int y = getPt( ya , yb , i );

      pts.push_back( Point(x,y) );
    }
}

static Point average(vector<Point>& points)
{
  Point point(0,0);
  for(auto& p : points )
    {
      point.x += p.x;
      point.y += p.y;
    }
  point.x /= points.size();
  point.y /= points.size();
  return point;
}

void fp16x2_transpose(const void *image, void* buffer)
{
  u32 *src = (u32 *)image;
  u16 *dst = (u16 *)buffer;
  for (int x = 0; x < PROC_W; x++)
    {
      for (int y = 0; y < PROC_H; y++)
        {
	  dst[y * PROC_W + x] = (u16)(src[y + x * PROC_H] & 0xffff);
        }
    }
}

void draw_lane_output(COverlayRGB &overlay, void* buffer)
{
  // draw detected lane image
  __fp16 *p16 = (__fp16*)buffer;
  for(int y=0; y<PROC_H; y++)
    {
      unsigned char* row = overlay.get_overlay_row_ptr_ref(y);
      for(int x=0; x<PROC_W; x++)
	{
	  float f = (float)p16[PROC_W * y + x];
	  f = f * f;
	  if(f>0.05f) row[x*3+2] = 255 * f+ row[x*3+0] * (1.0f - f);
	}
    }
}

void draw_lane(COverlayRGB& overlay, void* buffer, bool pause)
{
  pixfmt_type pixf(overlay.get_ren_buf_ref());
  base_ren_type ren(pixf);
  agg::scanline_u8 sl;
  agg::rasterizer_scanline_aa<> ras;
  agg::path_storage ps;
  agg::conv_stroke<agg::path_storage> pg(ps);
  pg.width(2.0f);
  
  sorted_pts.clear();
  contours(buffer, sorted_pts);

  if(sorted_pts.size()<4) return;

  Point top(0,0x1000), left(0x1000,0), right(0,0), center(0,0);

  if(!pause)
    {
      for(auto& p : sorted_pts)
	{
	  if(p.y < top.y) top = p;
	  if(p.x < left.x) left = p;
	  if(p.x > right.x) right = p;
	}
      top.x = 0;
      int ntop = 0, ncenter = 0;
      for(auto& p : sorted_pts)
	{
	  if(p.y == top.y)
	    {
	      top.x += p.x;
	      ntop++;
	    };
	  if(p.y < max(left.y,right.y))
	    {
	      center.x += p.x;
	      center.y += p.y;
	      ncenter++;
	    }
	}
      top.x /= ntop;
      center.x /= ncenter;
      center.y /= ncenter;

      if( ave_top.size() >= AVE_LEN ) ave_top.erase( ave_top.begin() );
      if( ave_left.size() >= AVE_LEN ) ave_left.erase( ave_left.begin() );
      if( ave_right.size() >= AVE_LEN ) ave_right.erase( ave_right.begin() );
      if( ave_center.size() >= AVE_LEN ) ave_center.erase( ave_center.begin() );

      ave_top.push_back( top );
      ave_left.push_back( left );
      ave_right.push_back( right );
      ave_center.push_back( center );
    }

  top = average(ave_top);
  left = average(ave_left);
  right = average(ave_right);
  center = average(ave_center);

  left.y = PROC_H-1;
  right.y = PROC_H-1;

  // draw lane curves
  Point lcp( (left.x + top.x)/2, (left.y + top.y)/2 );
  Point rcp( (right.x + top.x)/2, (right.y + top.y)/2 );
  int sx = ((center.x - lcp.x) - (rcp.x - center.x))/2;
  lcp.x += sx;
  rcp.x += sx;

  lpts.clear();
  rpts.clear();
  curve(left,lcp,top,lpts);
  curve(right,rcp,top,rpts);

  ps.move_to( lpts[0].x, lpts[0].y );
  for(auto& pt : lpts) ps.line_to( pt.x, pt.y );

  ps.move_to( rpts[0].x, rpts[0].y );
  for(auto& pt : rpts) ps.line_to( pt.x, pt.y );

  ras.add_path(pg);
  agg::render_scanlines_aa_solid(ras, sl, ren, agg::rgba8(255, 255, 0, 255));
}

void print_string(COverlayRGB &bg, string text, int x, int y, int font_size, int color)
{
  unsigned w = 0;
  unsigned h = 0;
  COverlayRGB::calculate_boundary_text(text, font_size, w, h);

  COverlayRGB overlay(get_screen_width(), get_screen_height());
  overlay.alloc_mem_overlay(w, h);
  overlay.copy_overlay(bg, x, y);
  overlay.set_text(0, 0, text, font_size, color);
  overlay.print_to_display(x, y);
}

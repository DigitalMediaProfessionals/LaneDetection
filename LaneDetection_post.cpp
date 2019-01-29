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
#include "Contours.h"

using namespace std;
using namespace dmp;
using namespace util;

//#define DRAW_LANE_1
#define DRAW_LANE_2
#define AVE_LEN 6

#if defined(DRAW_LANE_1) || defined(DRAW_LANE_2)
#define DRAW_LANE
vector<Point> sorted_pts;
#endif
#ifdef DRAW_LANE_2
vector<Point> ave_top;
vector<Point> ave_left;
vector<Point> ave_right;
vector<Point> ave_center;
vector<Point> lpts, rpts;
#endif

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
#ifdef DRAW_LANE
  pixfmt_type pixf(overlay.get_ren_buf_ref());
  base_ren_type ren(pixf);
  agg::scanline_u8 sl;
  agg::rasterizer_scanline_aa<> ras;
  agg::path_storage ps;
  agg::conv_stroke<agg::path_storage> pg(ps);
  pg.width(2.0f);
  
  sorted_pts.clear();
  contours(buffer, PROC_W, PROC_H, sorted_pts);

  if(sorted_pts.size()<4) return;
#endif

#ifdef DRAW_LANE_1
  for(unsigned i=0; i<sorted_pts.size(); i++)
    {
      if(i==0) ps.move_to(sorted_pts[i].x,sorted_pts[i].y);
      else ps.line_to(sorted_pts[i].x,sorted_pts[i].y);
    }
  ras.add_path(pg);
  agg::render_scanlines_aa_solid(ras, sl, ren, agg::rgba8(128, 128, 128, 255));
  ras.reset();
  ps.remove_all();

  Point edge_pts_left[RESULT_POINT_LEN];
  Point edge_pts_right[RESULT_POINT_LEN];
  int point_result = get_edge_point(sorted_pts, edge_pts_left, edge_pts_right);

  if(point_result & 1)
    {
      for(int i=0; i<RESULT_POINT_LEN; i++)
	{
	  if(i==0) ps.move_to(edge_pts_left[i].x, edge_pts_left[i].y);
	  else ps.line_to(edge_pts_left[i].x, edge_pts_left[i].y);
	}
    }
  if(point_result & 2)
    {
      for(int i=0; i<RESULT_POINT_LEN; i++)
	{
	  if(i==0) ps.move_to(edge_pts_right[i].x, edge_pts_right[i].y);
	  else ps.line_to(edge_pts_right[i].x, edge_pts_right[i].y);
	}
    }
#endif

#ifdef DRAW_LANE_2
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

#if 0
  ps.move_to(left.x, left.y);
  ps.line_to(top.x, top.y);
  ps.line_to(right.x, right.y);

  ps.move_to( (left.x+right.x)/2, (left.y+right.y)/2);
  ps.line_to(center.x, center.y);
  ps.line_to(top.x, top.y);
#else
  // curve
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
#endif
#endif // DRAW_LANE_2

#ifdef DRAW_LANE
  ras.add_path(pg);
  agg::render_scanlines_aa_solid(ras, sl, ren, agg::rgba8(255, 255, 0, 255));
#endif
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

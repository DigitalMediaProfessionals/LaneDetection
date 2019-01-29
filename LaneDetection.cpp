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

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "LaneDetection_gen.h"
#include "util_draw.h"
#include "util_input.h"
#include "demo_common.h"
#include "LaneDetection_param.h"
#include "Contours.h"

using namespace std;
using namespace dmp;
using namespace util;

#define SCREEN_W (get_screen_width())
#define SCREEN_H (get_screen_height())
#define IMAGE_PATH "./images_shibuya/"

// Define CNN network model object
CLaneDetection network;

// Buffer for decoded image data
uint32_t imgView[IMAGE_W * IMAGE_H];

// Buffer for pre-processed image data
__fp16 imgProc[PROC_W * PROC_H * 3];

int main(int argc, char **argv)
{
  // Initialize FB
  if (!init_fb())
    {
      cout << "init_fb() failed." << endl;
      return 1;
    }

  string image_path(IMAGE_PATH);
  if(argc>1) image_path = argv[1];
  cout << "image_path=" << image_path << endl;

  // Get input images filenames
  vector<string> image_names;
  get_jpeg_image_names(image_path, image_names);
  int num_images = image_names.size();
  if (num_images == 0)
    {
      cout << "No input images." << endl;
      return 1;
    }

  // Initialize network object
  network.Verbose(0);
  if (!network.Initialize())
    {
      return -1;
    }
  if (!network.LoadWeights(LANEDETECTION_WEIGHTS))
    {
      return -1;
    }
  if (!network.Commit())
    {
      return -1;
    }

  // Get HW module frequency
  string conv_freq;
  conv_freq = std::to_string(network.get_dv_info().conv_freq);

  // Create background and image overlay
  COverlayRGB bg_overlay(SCREEN_W, SCREEN_H);
  bg_overlay.alloc_mem_overlay(SCREEN_W, SCREEN_H);
  bg_overlay.load_ppm_img("fpgatitle");

  COverlayRGB cam_overlay(SCREEN_W, SCREEN_H);
  cam_overlay.alloc_mem_overlay(IMAGE_W, IMAGE_H);

  // Draw background two times for front and back buffer
  const char *titles[] = {
    "CNN - Lane Detection Demo",
    "Draw detected lane lines",
  };
  for (int i = 0; i < 2; ++i) {
    bg_overlay.print_to_display(0, 0);
    print_demo_title(bg_overlay, titles);
    swap_buffer();
  }

  int exit_code = -1;
  int image_nr = 0, image_step = 1;
  bool pause = false;
  //std::vector<float> tensor;

  // draw setup
  void* buffer = malloc(PROC_W * PROC_H * 2); // fp16x1
  pixfmt_type pixf( cam_overlay.get_ren_buf_ref() );
  base_ren_type ren(pixf);
  agg::scanline_u8 sl;
  agg::rasterizer_scanline_aa<> ras;
  agg::rgba8 color = agg::rgba8(255,255, 0, 255);
  agg::path_storage ps;
  agg::conv_stroke<agg::path_storage> pg(ps);
  pg.width(2.0);

  std::vector<Point> ave_top, ave_left, ave_right;
  
  // Enter main loop
  while (exit_code == -1)
    {
      // If not pause, decode next JPEG image and do pre-processing
      if (!pause)
	{
	  decode_jpg_file(image_names[image_nr], imgView, IMAGE_W, IMAGE_H);
	  cam_overlay.convert_to_overlay_pixel_format(imgView, IMAGE_W*IMAGE_H);

	  // Pre-process the image data
	  preproc_image(imgView, imgProc, IMAGE_W, IMAGE_H, PROC_W, PROC_H,
			0.0f, 0.0f, 0.0f, 1.0f / 255.0f, true, false);

	  image_nr += image_step;
	  image_nr %= num_images;
	}

      // Run network in HW
      memcpy(network.get_network_input_addr_cpu(), imgProc, PROC_W * PROC_H * 6);
      network.RunNetwork();

      // Handle output from HW
      const void* output = network.get_io_ptr() + network.get_output_layers()[0]->output_offs;
      fp16x2_transpose(output, buffer, PROC_W, PROC_H, 2);

      Point top(0,0x1000), left(0x1000,0), right(0,0);
      if(!pause)
	{
	  std::vector<Point> sorted_pts;
	  contours(buffer, PROC_W, PROC_H, sorted_pts);

	  Point top(0,0x1000), left(0x1000,0), right(0,0);
	  for(auto& p : sorted_pts)
	    {
	      if(p.y < top.y) top = p;
	      if(p.x < left.x) left = p;
	      if(p.x > right.x) right = p;
	    }
	  top.x = 0;
	  int ntop = 0;
	  for(auto& p : sorted_pts)
	    {
	      if(p.y == top.y) { top.x += p.x; ntop++; };
	    }
	  top.x /= ntop;

	  if( ave_top.size() > 15 ) ave_top.erase( ave_top.begin() );
	  if( ave_left.size() > 15 ) ave_left.erase( ave_left.begin() );
	  if( ave_right.size() > 15 ) ave_right.erase( ave_right.begin() );

	  ave_top.push_back( top );
	  ave_left.push_back( left );
	  ave_right.push_back( right );
	}
      top = Point(0,0);
      for(auto& p : ave_top )
	{
	  top.x += p.x;
	  top.y += p.y;
	}
      top.x /= ave_top.size();
      top.y /= ave_top.size();
      
      left = Point(0,0);
      for(auto& p : ave_left )
	{
	  left.x += p.x;
	  left.y += p.y;
	}
      left.x /= ave_left.size();
      //      left.y /= ave_left.size();
      left.y = PROC_H-1;
      
      right = Point(0,0);
      for(auto& p : ave_right )
	{
	  right.x += p.x;
	  right.y += p.y;
	}
      right.x /= ave_right.size();
      //      right.y /= ave_right.size();
      right.y = PROC_H-1;
      
      // draw lane
      ras.reset();
      ps.remove_all();

      ps.move_to(left.x, left.y);
      ps.line_to(top.x, top.y);
      ps.line_to(right.x, right.y);
      ras.add_path(pg);
      agg::render_scanlines_aa_solid(ras, sl, ren, color);
      
      // Draw detection result to screen
      cam_overlay.print_to_display(((SCREEN_W - IMAGE_W) / 2), 150);

      // Output HW processing times
      int conv_time_tot = network.get_conv_usec();
      print_conv_time(bg_overlay, (235 + IMAGE_H), conv_time_tot, conv_freq);

      swap_buffer();

      bool pause_prev = pause;
      int key = handle_keyboard_input(exit_code, pause);
      if(!pause_prev && pause)
	{
	  cout << "frame number = " << image_nr-1 << endl;
	}
      if(key>='1' && key<='9')
	{
	  image_step = key - '0';
	}
    }

  free(buffer);
  shutdown();

  return exit_code;
}

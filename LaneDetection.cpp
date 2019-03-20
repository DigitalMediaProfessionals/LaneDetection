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
#include <thread>

#include "util_draw.h"
#include "util_input.h"
#include "demo_common.h"

#include "LaneDetection_gen.h"
#include "LaneDetection_param.h"
#include "YOLOv3_gen.h"
#include "YOLOv3_param.h"

#define SCREEN_W (get_screen_width())
#define SCREEN_H (get_screen_height())
#define IMAGE_PATH "./images_lane/"

using namespace std;
using namespace dmp;
using namespace util;

// Define CNN network model object
CLaneDetection laneDetection;
CYOLOv3 yolo;

// Buffer for decoded image data
uint32_t imgView[IMAGE_W * IMAGE_H];
// Buffer for pre-processed image data
__fp16 imgProc[PROC_W * PROC_H * 3];

// Post-processing functions, defined in LaneDetection_post.cpp
void fp16x2_transpose(const void *image, void* buffer);
void draw_lane(COverlayRGB &overlay, void* buffer, bool pause);
void draw_lane_output(COverlayRGB &overlay, void* buffer);
void print_string(COverlayRGB &bg, string text, int x, int y, int font_size, int color);

// Post-processing functions, defined in YOLOv3_post.cpp
void get_bboxes(const vector<float> &tensor, vector<float> &boxes);
void draw_bboxes(const vector<float> &boxes, COverlayRGB &overlay);

float get_elapsed_time(struct timespec& ts0)
{
   struct timespec ts1;
   clock_gettime(CLOCK_REALTIME, &ts1);

   if((ts1.tv_nsec - ts0.tv_nsec) <= 0 ) ts1.tv_nsec += 1000000000;
   int pt = (ts1.tv_nsec - ts0.tv_nsec) / 1000;
   if(pt<0) pt = 0;
   return pt / 1000.0f;
}

void load_image(string image_name)
{
   decode_jpg_file(image_name, imgView, IMAGE_W, IMAGE_H);
}

void proc_image(COverlayRGB& overlay)
{
   overlay.convert_to_overlay_pixel_format(imgView, IMAGE_W*IMAGE_H);
   preproc_image(imgView, imgProc, IMAGE_W, IMAGE_H, PROC_W, PROC_H,
		 0.0f, 0.0f, 0.0f, 1.0f / 255.0f, true, false);
}

void run_network()
{
   memcpy(laneDetection.get_network_input_addr_cpu(), imgProc, PROC_W * PROC_H * 6);
   memcpy(yolo.get_network_input_addr_cpu(), imgProc, PROC_W * PROC_H * 6);
   laneDetection.RunNetwork();
   yolo.RunNetwork();
}

int main(int argc, char **argv)
{
   // Initialize FB
   if (!init_fb())
   {
      cout << "init_fb() failed." << endl;
      return 1;
   }

   string image_path(IMAGE_PATH);
   int frame_step = 1;
   if(argc>1) image_path = argv[1];
   if(argc>2) frame_step = atoi(argv[2]);
   if(image_path.back()!='/') image_path += '/';
   cout << "image_path=" << image_path << endl;
   cout << "frame_step=" << frame_step << endl;
  
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

   laneDetection.Verbose(0);
   if (!laneDetection.Initialize())
   {
      return -1;
   }
   if (!laneDetection.LoadWeights(LANEDETECTION_WEIGHTS))
   {
      return -1;
   }
   if (!laneDetection.Commit())
   {
      return -1;
   }

   yolo.Verbose(0);
   if (!yolo.Initialize())
   {
      return -1;
   }
   if (!yolo.LoadWeights(YOLOv3_WEIGHTS))
   {
      return -1;
   }
   if (!yolo.Commit())
   {
      return -1;
   }

   // Get HW module frequency
   string conv_freq;
   conv_freq = std::to_string(yolo.get_dv_info().conv_freq);

   // Create background and image overlay
   COverlayRGB bg_overlay(SCREEN_W, SCREEN_H);
   bg_overlay.alloc_mem_overlay(SCREEN_W, SCREEN_H);
   bg_overlay.load_ppm_img("fpgatitle");

   COverlayRGB cam_overlay[2] = { COverlayRGB(SCREEN_W, SCREEN_H),COverlayRGB(SCREEN_W, SCREEN_H) };
   cam_overlay[0].alloc_mem_overlay(IMAGE_W, IMAGE_H);
   cam_overlay[1].alloc_mem_overlay(IMAGE_W, IMAGE_H);
   unsigned cam_overlay_id = 0;

   // Draw background two times for front and back buffer
   const char *titles[] = {
      "Lane Detection and YOLOv3",
      "Draw detected lane and objects",
   };
   for (int i = 0; i < 2; ++i)
   {
      bg_overlay.print_to_display(0, 0);
      print_demo_title(bg_overlay, titles);

      int xpos = SCREEN_W-240;
      int ypos = SCREEN_H-20;

      print_string(bg_overlay,"'P': Draw perf info", xpos, ypos, 10, 0x00ffffff); ypos -= 20;
      print_string(bg_overlay,"'1-9': Frame steps", xpos, ypos, 10, 0x00ffffff); ypos -= 20;
      print_string(bg_overlay,"'S':   Save as capture%d.ppm", xpos, ypos, 10, 0x00ffffff); ypos -= 20;
      print_string(bg_overlay,"'O':   Draw lane output on/off", xpos, ypos, 10, 0x00ffffff); ypos -= 20;
      print_string(bg_overlay,"'L':   Draw lane line on/off", xpos, ypos, 10, 0x00ffffff); ypos -= 20;
      print_string(bg_overlay,"'B':   Draw bbox on/off", xpos, ypos, 10, 0x00ffffff); ypos -= 20;
      print_string(bg_overlay,"SPC:   Pause", xpos, ypos, 10, 0x00ffffff); ypos -= 20;
      print_string(bg_overlay,"ESC:   Quit", xpos, ypos, 10, 0x00ffffff); ypos -= 20;

      swap_buffer();
   }

   int exit_code = -1;
   int image_nr = 0, image_step = frame_step;
   bool pause = false;
   bool b_bbox = true;
   bool b_lane = true;
   bool b_lane_output = true;
   bool b_perf = false;
   int capture = 0;
   int perf_clear = 0;

   vector<float> tensor;
   vector<float> boxes;
   // lane detection result buffer
   void* buffer = malloc(PROC_W * PROC_H * 2); // fp16x1

   // preload
   load_image(image_names[image_nr]);
   proc_image(cam_overlay[cam_overlay_id]);

   // Run threads
   auto th_image = thread(load_image, image_names[image_nr]);
   auto th_network = thread(run_network);

   struct timespec ts0;
   clock_gettime(CLOCK_REALTIME, &ts0);

   // Enter main loop
   while (exit_code == -1)
   {
      struct timespec ts1;
      clock_gettime(CLOCK_REALTIME, &ts1);

      // Get image
      th_image.join();
      proc_image(cam_overlay[1-cam_overlay_id]);

      // Handle output from HW
      th_network.join();

      if(b_bbox)
      {
	 yolo.get_final_output(tensor);
      }
      if(b_lane_output || b_lane)
      {
	 const void* output = laneDetection.get_io_ptr() + laneDetection.get_output_layers()[0]->output_offs;
	 fp16x2_transpose(output, buffer);
      }
      int conv_time_tot = yolo.get_conv_usec() + laneDetection.get_conv_usec();

      // Run threads
      th_image = thread(load_image, image_names[image_nr]);
      th_network = thread(run_network);

      // Post draw
      if(b_bbox)
      {
	 get_bboxes(tensor, boxes);
	 draw_bboxes(boxes, cam_overlay[cam_overlay_id]);
      }
      if(b_lane_output)
      {
	 draw_lane_output(cam_overlay[cam_overlay_id], buffer);
      }
      if(b_lane)
      {
	 draw_lane(cam_overlay[cam_overlay_id], buffer, pause);
      }

      // Draw detection result to screen
      cam_overlay[cam_overlay_id].print_to_display(((SCREEN_W - IMAGE_W) / 2), 150);

      // Output HW processing times
      print_conv_time(bg_overlay, (235 + IMAGE_H), conv_time_tot, conv_freq);

      // Key controls
      bool pause_prev = pause;
      int key = handle_keyboard_input(exit_code, pause);
      if(!pause_prev && pause)
      {
	 cout << "frame number = " << image_nr << endl;
      }
      else if(key>='1' && key<='9')
      {
	 image_step = key - '0';
	 cout << "frame steps=" << image_step << endl;
      }
      else if(key=='b' || key=='B')
      {
	 b_bbox = !b_bbox;
	 cout << "draw bbox=" << (b_bbox? "on":"off") << endl;
      }
      else if(key=='l' || key=='L')
      {
	 b_lane = !b_lane;
	 cout << "draw lane=" << (b_lane? "on":"off") << endl;
      }
      else if(key=='o' || key=='O')
      {
	 b_lane_output = !b_lane_output;
	 cout << "draw lane output=" << (b_lane_output? "on":"off") << endl;
      }
      else if(key=='p' || key=='P')
      {
	 b_perf = !b_perf;
	 cout << "draw perf=" << (b_perf? "on":"off") << endl;
	 perf_clear = 2;
      }
      else if(key=='s' || key=='O')
      {
	 stringstream ss;
	 ss << "capture" << capture++;
	 cout << ss.str() << endl;
	 cam_overlay[cam_overlay_id].save_as_ppm_img( ss.str() );
      }
      if (!pause)
      {
	 image_nr += image_step;
	 image_nr %= num_images;
      }
      cam_overlay_id = 1 - cam_overlay_id;

      // draw performance info
      if(b_perf)
      {
	 float time0 = get_elapsed_time(ts0);
	 float time1 = get_elapsed_time(ts1);
	 float fps = 1000.0f / time0;
	
	 stringstream ss;
	 ss << " Frame:" << image_nr
	    << " Total:"<< fixed << setprecision(0) << setw(3)
	    << time0 << "ms CPU:" << time1 << "ms FPS:" << fps
	    << "   ";
	 print_string(bg_overlay, ss.str(), 0, 0, 10, 0x00ffffff);
      }
      else if(perf_clear)
      {
	 string clear(50,' ');
	 print_string(bg_overlay,clear, 0, 0, 10, 0x00000000);
	 perf_clear--;
      }

      clock_gettime(CLOCK_REALTIME, &ts0);

      swap_buffer();
   }

   th_image.join();
   th_network.join();

   free(buffer);
   shutdown();

   return exit_code;
}

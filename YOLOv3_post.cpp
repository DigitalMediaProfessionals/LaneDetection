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

#include <string>
#include <vector>
#include "util_draw.h"
#include "YOLOv3_param.h"

using namespace std;
using namespace dmp;
using namespace util;

const int NUM_TENSOR = NUM_CLASS + 5;

static inline float sigmoid(float x) { return 0.5f + 0.5f * std::tanh(0.5f * x); }
static inline float sigmoid_inverse(float x) { return std::log(x / (1.0f - x)); }

static inline float overlap(float x1, float w1, float x2, float w2) {
  float left = max(x1, x2);
  float right = min(x1 + w1, x2 + w2);
  return max(right - left, 0.0f);
}

static float box_iou(const float *a, const float *b) {
  float ow = overlap(a[0], a[2], b[0], b[2]);
  float oh = overlap(a[1], a[3], b[1], b[3]);
  float box_int = ow * oh;
  float box_uni = a[2] * a[3] + b[2] * b[3] - box_int;
  return box_int / box_uni;
}

static void decode_yolo_box(
    float *box, const unsigned *anchor, const unsigned *dim, int x, int y) {
  box[2] = exp(box[2]) * anchor[0] / PROC_W;
  box[3] = exp(box[3]) * anchor[1] / PROC_H;
  box[0] = (x + box[0]) / dim[0] - box[2] / 2.0f;
  box[1] = (y + box[1]) / dim[1] - box[3] / 2.0f;
}

void get_bboxes(const vector<float> &tensor, vector<float> &boxes) {
  int box_num = 0;
  const float *t = &tensor[0];
  float *box;

  static float INV_OBJ_THRESHOLD = 0;
#ifdef NON_TINY
  static unsigned DIM[] = {0, 0, 0, 0, 0, 0};
  static unsigned ANCHOR[] = { 116, 90, 156, 198, 373, 326,
                               30, 61, 62, 45, 59, 119,
                               10, 13, 16, 30, 33, 23 };
  if (!DIM[0]) {
    DIM[0] = PROC_W / 32;
    DIM[1] = PROC_H / 32;
    DIM[2] = PROC_W / 16;
    DIM[3] = PROC_H / 16;
    DIM[4] = PROC_W / 8;
    DIM[5] = PROC_H / 8;
  }
#else
  static unsigned DIM[4] = {0, 0, 0, 0};
  static unsigned ANCHOR[] = {81, 82, 135, 169, 344, 319, 23, 27, 37, 58, 81, 82};
  if (!DIM[0]) {
    DIM[0] = PROC_W / 32;
    DIM[1] = PROC_H / 32;
    DIM[2] = PROC_W / 16;
    DIM[3] = PROC_H / 16;
  }
#endif
  if (!INV_OBJ_THRESHOLD) {
    INV_OBJ_THRESHOLD = sigmoid_inverse(OBJ_THRESHOLD);
  }

  boxes.clear();
  for (int i = 0; i < 2; i++) {
    for (unsigned y = 0; y < DIM[i * 2 + 1]; y++) {
      for (unsigned x = 0; x < DIM[i * 2]; x++) {
        for (int n = 0; n < NUM_BOX_PER_TILE; n++) {
          if (t[4] < INV_OBJ_THRESHOLD) {
            t += NUM_TENSOR;
            continue;
          }

          boxes.resize(boxes.size() + NUM_TENSOR);
          box = &boxes[box_num * NUM_TENSOR];

          memcpy(box, t, sizeof(float) * NUM_TENSOR);
          ++box_num;
          t += NUM_TENSOR;

          box[0] = sigmoid(box[0]);
          box[1] = sigmoid(box[1]);
          box[4] = sigmoid(box[4]);
          for (int j = 5; j < NUM_TENSOR; j++) {
            box[j] = box[4] * sigmoid(box[j]);
            if (box[j] < CLS_THRESHOLD) box[j] = 0.0f;
          }

          decode_yolo_box(box, ANCHOR + (i * NUM_BOX_PER_TILE + n) * 2, DIM + i * 2, x, y);
        }
      }
    }
  }

  for (int i = 0; i < NUM_CLASS; i++) {
    float lastprob = 1.0f;
    do {
      float *mbox = nullptr;
      float mprob = 0.0f;
      for (int j = 0; j < box_num; j++) {
        box = &boxes[j * NUM_TENSOR];
        if (box[5 + i] > mprob && box[5 + i] < lastprob) {
          mbox = box;
          mprob = box[5 + i];
        }
      }
      lastprob = mprob;
      if (mbox) {
        for (int j = 0; j < box_num; j++) {
          box = &boxes[j * NUM_TENSOR];
          if (box[5 + i] == 0.0f or mbox == box) continue;
          if (box_iou(mbox, box) > NMS_THRESHOLD) box[5 + i] = 0.0f;
        }
      }
    } while (lastprob > 0.0f);
  }
}

const char *class_name[] = {
    "person",        "bicycle",       "car",           "motorbike",
    "aeroplane",     "bus",           "train",         "truck",
    "boat",          "traffic light", "fire hydrant",  "stop sign",
    "parking meter", "bench",         "bird",          "cat",
    "dog",           "horse",         "sheep",         "cow",
    "elephant",      "bear",          "zebra",         "giraffe",
    "backpack",      "umbrella",      "handbag",       "tie",
    "suitcase",      "frisbee",       "skis",          "snowboard",
    "sports ball",   "kite",          "baseball bat",  "baseball glove",
    "skateboard",    "surfboard",     "tennis racket", "bottle",
    "wine glass",    "cup",           "fork",          "knife",
    "spoon",         "bowl",          "banana",        "apple",
    "sandwich",      "orange",        "broccoli",      "carrot",
    "hot dog",       "pizza",         "donut",         "cake",
    "chair",         "sofa",          "pottedplant",   "bed",
    "diningtable",   "toilet",        "tvmonitor",     "laptop",
    "mouse",         "remote",        "keyboard",      "cell phone",
    "microwave",     "oven",          "toaster",       "sink",
    "refrigerator",  "book",          "clock",         "vase",
    "scissors",      "teddy bear",    "hair drier",    "toothbrush",
};

const uint32_t class_color[] = {
    0x00FF3F, 0x003FFF, 0xFF2600, 0xF200FF, 0xFF4C00, 0xA5FF00,
    0xFF7200, 0xFF8500, 0xFF9900, 0xFFAC00, 0xFFBF00, 0xFFD200,
    0xFFE500, 0xFFF800, 0xF2FF00, 0xDFFF00, 0xCBFF00, 0xB8FF00,
    0xFF5F00, 0x92FF00, 0x7FFF00, 0x6CFF00, 0x59FF00, 0x46FF00,
    0x33FF00, 0x1FFF00, 0x0CFF00, 0x00FF06, 0x00FF19, 0x00FF2C,
    0xFF0000, 0x00FF52, 0x00FF66, 0x00FF79, 0x00FF8C, 0x00FF9F,
    0x00FFB2, 0x00FFC5, 0x00FFD8, 0x00FFEB, 0x00FFFF, 0x00EBFF,
    0x00D8FF, 0x00C5FF, 0x00B2FF, 0x009FFF, 0x008CFF, 0x0079FF,
    0x0066FF, 0x0052FF, 0xFF1300, 0x002CFF, 0x0019FF, 0x0006FF,
    0x0C00FF, 0x1F00FF, 0x3200FF, 0x4600FF, 0x5900FF, 0x6C00FF,
    0x7F00FF, 0x9200FF, 0xA500FF, 0xB800FF, 0xCC00FF, 0xDF00FF,
    0xFF3900, 0xFF00F8, 0xFF00E5, 0xFF00D2, 0xFF00BF, 0xFF00AC,
    0xFF0098, 0xFF0085, 0xFF0072, 0xFF005F, 0xFF004C, 0xFF0039,
    0xFF0026, 0xFF0013,
};

void draw_bboxes(const vector<float> &boxes, COverlayRGB &overlay) {
  int num = boxes.size() / NUM_TENSOR;
  int image_w = overlay.get_overlay_width();
  int image_h = overlay.get_overlay_height();
  const float *box;

  for (int i = 0; i < num; i++) {
    box = &boxes[i * NUM_TENSOR];
    bool has_obj = false;
    int obj_type = -1;
    string s;
    for (int j = 0; j < NUM_CLASS; j++)
      if (box[5 + j] > 0.0f) {
        has_obj = true;
        obj_type = j;
        if (s.size() > 0) s += ",";
        s += class_name[obj_type];
      }
    if (!has_obj) continue;
#if 0
    for (int j = 0; j < NUM_TENSOR; j += 5)
    {
      for (int k = 0; k < 5; k++)
        printf("%.2f  ", box[j + k]);
      printf("\n");
    }
    printf("\n");
#endif
    int x = image_w * box[0];
    int y = image_h * box[1];
    int w = image_w * box[2];
    int h = image_h * box[3];
    uint32_t color = class_color[obj_type];

    int x0 = (x > image_w - 1) ? (image_w - 1) : ((x < 0) ? 0 : x);
    int y0 = (y > image_h - 1) ? (image_h - 1) : ((y < 0) ? 0 : y);
    int x1 = x + w;
    x1 = (x1 > image_w - 1) ? (image_w - 1) : ((x1 < 0) ? 0 : x1);
    int y1 = y + h;
    y1 = (y1 > image_h - 1) ? (image_h - 1) : ((y1 < 0) ? 0 : y1);
    x = x0;
    y = y0;
    w = x1 - x0;
    h = y1 - y0;
    bool no_text = (x + 8 * (int)s.length() >= image_w) || (y + 8 >= image_w) || (y - 9 < 0);

    overlay.set_box(x0, y0, x1, y1, color);
    if (!no_text) overlay.set_box_with_text(x0, y0, x1, y1, color, s);
  }
}


/*
 *  Copyright 2019 Digital Media Professionals Inc.
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

#pragma once

// define NON_TINY if the YOLOv3 is not tiny version.
// #define NON_TINY

#ifndef PROC_W
#define PROC_W 384
#define PROC_H 256
#endif

#define CLS_THRESHOLD 0.3f
#define NMS_THRESHOLD 0.45f
#define OBJ_THRESHOLD 0.2f

// For output of TinyYOLOv3
#define NUM_CLASS 80
#define NUM_BOX_PER_TILE   3

#define YOLOv3_WEIGHTS "YOLOv3_weights.bin"

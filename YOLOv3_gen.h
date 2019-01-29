/*
 *  Copyright 2018 Digital Media Professionals Inc.

 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at

 *      http://www.apache.org/licenses/LICENSE-2.0

 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.

 *  This source code was generated using DMP-DV700 tools.
 */
#pragma once

#include "dmp_network.h"

class CYOLOv3 : public CDMP_Network {
 private:
  /*!

  Layer description

  | ID | Layers | Type | Dim In | Dim Out | Param | Mem |
  | :- | :- | :-: | :-: | :-: | :-: | :-: |
  | 0 | FPGA-Layer | Convolution | (384, 256, 3) | (384, 256, 16) | - | - |
  | 0-0 | conv2d_1 | Convolution | (384, 256, 3) | (384, 256, 16) | - | 2336 |

  */
  void Layer_0();
  /*!

  Layer description

  | ID | Layers | Type | Dim In | Dim Out | Param | Mem |
  | :- | :- | :-: | :-: | :-: | :-: | :-: |
  | 1 | FPGA-Layer | Convolution | (384, 256, 16) | (192, 128, 16) | - | - |
  | 1-0 | max_pooling2d_1 | Pooling | (384, 256, 16) | (192, 128, 16) | - | - |

  */
  void Layer_1();
  /*!

  Layer description

  | ID | Layers | Type | Dim In | Dim Out | Param | Mem |
  | :- | :- | :-: | :-: | :-: | :-: | :-: |
  | 2 | FPGA-Layer | Convolution | (192, 128, 16) | (192, 128, 32) | - | - |
  | 2-0 | conv2d_2 | Convolution | (192, 128, 16) | (192, 128, 32) | - | 9280 |

  */
  void Layer_2();
  /*!

  Layer description

  | ID | Layers | Type | Dim In | Dim Out | Param | Mem |
  | :- | :- | :-: | :-: | :-: | :-: | :-: |
  | 3 | FPGA-Layer | Convolution | (192, 128, 32) | (96, 64, 32) | - | - |
  | 3-0 | max_pooling2d_2 | Pooling | (192, 128, 32) | (96, 64, 32) | - | - |

  */
  void Layer_3();
  /*!

  Layer description

  | ID | Layers | Type | Dim In | Dim Out | Param | Mem |
  | :- | :- | :-: | :-: | :-: | :-: | :-: |
  | 4 | FPGA-Layer | Convolution | (96, 64, 32) | (48, 32, 64) | - | - |
  | 4-0 | conv2d_3 | Convolution | (96, 64, 32) | (96, 64, 64) | - | 36992 |
  | 4-0 | max_pooling2d_3 | Pooling | (96, 64, 64) | (48, 32, 64) | - | - |

  */
  void Layer_4();
  /*!

  Layer description

  | ID | Layers | Type | Dim In | Dim Out | Param | Mem |
  | :- | :- | :-: | :-: | :-: | :-: | :-: |
  | 5 | FPGA-Layer | Convolution | (48, 32, 64) | (24, 16, 128) | - | - |
  | 5-0 | conv2d_4 | Convolution | (48, 32, 64) | (48, 32, 128) | - | 147712 |
  | 5-0 | max_pooling2d_4 | Pooling | (48, 32, 128) | (24, 16, 128) | - | - |

  */
  void Layer_5();
  /*!

  Layer description

  | ID | Layers | Type | Dim In | Dim Out | Param | Mem |
  | :- | :- | :-: | :-: | :-: | :-: | :-: |
  | 6 | FPGA-Layer | Convolution | (24, 16, 128) | (24, 16, 256) | - | - |
  | 6-0 | conv2d_5 | Convolution | (24, 16, 128) | (24, 16, 256) | - | 590336 |

  */
  void Layer_6();
  /*!

  Layer description

  | ID | Layers | Type | Dim In | Dim Out | Param | Mem |
  | :- | :- | :-: | :-: | :-: | :-: | :-: |
  | 7 | FPGA-Layer | Convolution | (24, 16, 256) | (12, 8, 256) | - | - |
  | 7-0 | max_pooling2d_5 | Pooling | (24, 16, 256) | (12, 8, 256) | - | - |

  */
  void Layer_7();
  /*!

  Layer description

  | ID | Layers | Type | Dim In | Dim Out | Param | Mem |
  | :- | :- | :-: | :-: | :-: | :-: | :-: |
  | 8 | FPGA-Layer | Convolution | (12, 8, 256) | (12, 8, 512) | - | - |
  | 8-0 | conv2d_6 | Convolution | (12, 8, 256) | (12, 8, 512) | - | 2360320 |
  | 8-0 | max_pooling2d_6 | Pooling | (12, 8, 512) | (12, 8, 512) | - | - |

  */
  void Layer_8();
  /*!

  Layer description

  | ID | Layers | Type | Dim In | Dim Out | Param | Mem |
  | :- | :- | :-: | :-: | :-: | :-: | :-: |
  | 9 | FPGA-Layer | Convolution | (12, 8, 512) | (12, 8, 1024) | - | - |
  | 9-0 | conv2d_7 | Convolution | (12, 8, 512) | (12, 8, 1024) | - | 9439232 |

  */
  void Layer_9();
  /*!

  Layer description

  | ID | Layers | Type | Dim In | Dim Out | Param | Mem |
  | :- | :- | :-: | :-: | :-: | :-: | :-: |
  | 10 | FPGA-Layer | Convolution | (12, 8, 1024) | (12, 8, 256) | - | - |
  | 10-0 | conv2d_8 | Convolution | (12, 8, 1024) | (12, 8, 256) | - | 590336 |

  */
  void Layer_10();
  /*!

  Layer description

  | ID | Layers | Type | Dim In | Dim Out | Param | Mem |
  | :- | :- | :-: | :-: | :-: | :-: | :-: |
  | 11 | FPGA-Layer | Convolution | (12, 8, 256) | (12, 8, 512) | - | - |
  | 11-0 | conv2d_9 | Convolution | (12, 8, 256) | (12, 8, 512) | - | 2360320 |

  */
  void Layer_11();
  /*!

  Layer description

  | ID | Layers | Type | Dim In | Dim Out | Param | Mem |
  | :- | :- | :-: | :-: | :-: | :-: | :-: |
  | 12 | FPGA-Layer | Convolution | (12, 8, 512) | (12, 8, 255) | - | - |
  | 12-0 | conv2d_10 | Convolution | (12, 8, 512) | (12, 8, 255) | - | 294272 |

  */
  void Layer_12();
  /*!

  Layer description

  | ID | Layers | Type | Dim In | Dim Out | Param | Mem |
  | :- | :- | :-: | :-: | :-: | :-: | :-: |
  | 13 | FPGA-Layer | Flatten | (12, 8, 255) | (24480,) | - | - |

  */
  void Layer_13();
  /*!

  Layer description

  | ID | Layers | Type | Dim In | Dim Out | Param | Mem |
  | :- | :- | :-: | :-: | :-: | :-: | :-: |
  | 14 | FPGA-Layer | Convolution | (12, 8, 256) | (12, 8, 128) | - | - |
  | 14-0 | conv2d_11 | Convolution | (12, 8, 256) | (12, 8, 128) | - | 73984 |

  */
  void Layer_14();
  /*!

  Layer description

  | ID | Layers | Type | Dim In | Dim Out | Param | Mem |
  | :- | :- | :-: | :-: | :-: | :-: | :-: |
  | 15 | FPGA-Layer | Convolution | (12, 8, 128) | (24, 16, 128) | - | - |
  | 15-0 | up_sampling2d_1 | UpSampling | (12, 8, 128) | (24, 16, 128) | - | - |

  */
  void Layer_15();
  /*!

  Layer description

  | ID | Layers | Type | Dim In | Dim Out | Param | Mem |
  | :- | :- | :-: | :-: | :-: | :-: | :-: |
  | 16 | FPGA-Layer | Concatenate | (24, 16, 384) | (24, 16, 384) | - | - |

  */
  void Layer_16();
  /*!

  Layer description

  | ID | Layers | Type | Dim In | Dim Out | Param | Mem |
  | :- | :- | :-: | :-: | :-: | :-: | :-: |
  | 17 | FPGA-Layer | Convolution | (24, 16, 384) | (24, 16, 256) | - | - |
  | 17-0 | conv2d_12 | Convolution | (24, 16, 384) | (24, 16, 256) | - | 1769984 |

  */
  void Layer_17();
  /*!

  Layer description

  | ID | Layers | Type | Dim In | Dim Out | Param | Mem |
  | :- | :- | :-: | :-: | :-: | :-: | :-: |
  | 18 | FPGA-Layer | Convolution | (24, 16, 256) | (24, 16, 255) | - | - |
  | 18-0 | conv2d_13 | Convolution | (24, 16, 256) | (24, 16, 255) | - | 147392 |

  */
  void Layer_18();
  /*!

  Layer description

  | ID | Layers | Type | Dim In | Dim Out | Param | Mem |
  | :- | :- | :-: | :-: | :-: | :-: | :-: |
  | 19 | FPGA-Layer | Flatten | (24, 16, 255) | (97920,) | - | - |

  */
  void Layer_19();
  /*!

  Layer description

  | ID | Layers | Type | Dim In | Dim Out | Param | Mem |
  | :- | :- | :-: | :-: | :-: | :-: | :-: |
  | 20 | FPGA-Layer | Concatenate | (122400,) | (122400,) | - | - |

  */
  void Layer_20();

 public:
  virtual bool Initialize();
  CYOLOv3();
  virtual ~CYOLOv3();
};

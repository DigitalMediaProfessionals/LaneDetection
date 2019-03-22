# LaneDetection : Lane Detection + YOLOv3

## Clone
```
$ cd dv-sdk/application
$ git clone {this repository url} -b release LaneDetection
```

## Syntax
```
$ sudo ./LaneDetection {image_path} {frame_step}
```
default image_path : ./images_lane
default frame_step : 1

## Keyboard controls

display key help on bottom right of screen

## YOLOv3 in LaneDetection

YOLOv3_gen.cpp/.h are building by model/yolov3.ini,
not using the created YOLOv3_weights.bin due to same original weights.

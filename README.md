# LaneDetection : Lane Detection + YOLOv3

![LaneDetection Result](https://raw.githubusercontent.com/DigitalMediaProfessionals/LaneDetection/master/LaneDetection.png)

## Syntax
```
$ sudo ./LaneDetection {image_path} {frame_step}
```

* default image_path : ./images_lane
* default frame_step : 1

## Keyboard controls

See help on bottom right of screen

## YOLOv3 in LaneDetection

YOLOv3_gen.cpp/.h are built from model/yolov3.ini.
The model is the same as one of `dv-sdk/application/YOLOv3`,
so that you do not need to copy the generated weight file
if one of YOLOv3 exists in `dv-sdk/application/bin` already.


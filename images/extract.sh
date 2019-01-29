ffmpeg -i shibuya.mp4 -r 20 -qmin 1 -qmax 1 -vf scale=384:256 ../../bin/images_yolo_lane/image%04d.jpeg

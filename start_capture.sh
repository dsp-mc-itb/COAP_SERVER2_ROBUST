
#!/bin/bash

rm -r tes/tes*
#For MJPEG
#rpicam-vid -t 100000 --codec mjpeg --segment 100 -o tes/test%05d.jpeg --width 1280 --height 720 -q 80
#For H264 small size
#rpicam-vid -t 100000 -g 15 --framerate 15 --segment 500 -o tes/test%05d --width 1280 --height 720 

#For H264 Large size
rpicam-vid -t 100000 -g 15 --framerate 30 --segment 500 -o tes/test%05d --width 1920 --height 1080 -b 80000000 --level 4.2 --denoise cdn_off 



#!/bin/bash

# This is a simple shell script to greet the user

# Prompt the user for their name
rm -r tes/tes*
rpicam-vid -t 100000 --codec mjpeg --segment 50 -o tes/test%05d.jpeg --width 1280 --height 720 -q 80

#rpicam-vid -t 100000 -g 15 --framerate 15 --segment 1000 -o tes/test%05d --width 1280 --height 720 

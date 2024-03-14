
#!/bin/bash

# This is a simple shell script to greet the user

# Prompt the user for their name
rm -r tes/tes*
rpicam-vid -t 60000 --codec mjpeg --segment 500 -o tes/test%05d.jpeg --width 1280 --height 720 -q 60


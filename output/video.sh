#!/bin/bash

# Set the output file path and name
output_file="video.mp4"

# Set the recording time in seconds
duration=5
fps=20
# Set the maximum bitrate (in Kbps)
max_bitrate=400
res="640:360"
start_time=$(date +%s)
# Run ffmpeg to record video from the Raspberry Pi camera
# ffmpeg -t $duration -f video4linux2 -input_format h264 -i /dev/video0 -vf "scale=${res}" -r $fps -crf 13 -g 2 -b:v ${max_bitrate}k $output_file -y
ffmpeg -f video4linux2 -i /dev/video0 -r 30 -preset ultrafast -vcodec libx264 -g 1 -tune zerolatency -vf "scale=640:480" -f segment -segment_time 0.1 -segment_format mpegts output%d.ts
# Record the end time
end_time=$(date +%s)

# Calculate the execution time
execution_time=$((end_time - start_time))
echo "--------------SUMMARY--------------------"
echo "Script execution time: ${execution_time} seconds"
echo "Video Duration: ${duration} seconds"
echo "FPS: ${fps}"
echo "Bit Rate: ${max_bitrate}"
echo "Resolution ${res}"
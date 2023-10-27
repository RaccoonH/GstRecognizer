# GST Recognizer
Recogniser is a GST plugin which can detect object on frames. It receives YUV420 raw frames and pushes also YUV420.  
It uses YOLOv5.  

## Build
It uses cmake to build, you can use build.sh.  
### Requirements:
1. opencv >= 4.5.4  
2. gstreamer  
3. cmake >= 3.16

You should use GCC >= 11.4.0 (at least it was tested only on GCC 11.4.0)  

## How to run
After build you need to make sure that config files (classes.txt, yolov5.onnx) in proper directory (git_repo/config)  
Example pipeline:  
```
gst-launch-1.0 --gst-plugin-path=$PWD/build filesrc location=test.mp4 ! decodebin ! recognizer display-detections=true ! autovideosink sync=false
```
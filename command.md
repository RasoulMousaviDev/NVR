//save stream to mp4
gst-launch-1.0 -e rtspsrc location=rtsp://admin:admin@192.168.1.239:554/stream ! rtph264depay ! h264parse ! mp4mux ! filesink location=/tmp/cameras/test.mp4


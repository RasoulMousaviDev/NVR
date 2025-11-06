#include <gst/gst.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <rtsp-url>\n", argv[0]);
        return 1;
    }

    const char *rtsp_url = argv[1];
    gst_init(&argc, &argv);

    // Pipeline: RTSP -> depay -> parse -> decode -> raw video -> stdout
    char pipeline[1024];
    snprintf(pipeline, sizeof(pipeline),
             "rtspsrc location=%s latency=0 ! decodebin ! videoconvert ! jpegenc ! multipartmux boundary=frame ! filesink fd=1",
             rtsp_url);

    GError *err = NULL;
    GstElement *pipe = gst_parse_launch(pipeline, &err);
    if (!pipe)
    {
        if (err)
        {
            fprintf(stderr, "Failed to create pipeline: %s\n", err->message);
            g_error_free(err);
        }
        return 1;
    }

    gst_element_set_state(pipe, GST_STATE_PLAYING);

    GstBus *bus = gst_element_get_bus(pipe);
    gboolean terminate = FALSE;

    while (!terminate)
    {
        GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                                     GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
        if (msg)
        {
            if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR)
            {
                GError *e = NULL;
                gst_message_parse_error(msg, &e, NULL);
                fprintf(stderr, "GStreamer error: %s\n", e ? e->message : "unknown");
                if (e)
                    g_error_free(e);
                terminate = TRUE;
            }
            else if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS)
            {
                terminate = TRUE;
            }
            gst_message_unref(msg);
        }
    }

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(pipe);
    gst_object_unref(bus);

    return 0;
}

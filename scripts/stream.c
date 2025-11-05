#include <gst/gst.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------- Error handler ---------------------------- */
static void on_error(GstBus *bus, GstMessage *msg, gpointer user_data)
{
    GError *err;
    gchar *debug_info;
    gst_message_parse_error(msg, &err, &debug_info);
    g_printerr("Error: %s\n", err->message);
    g_error_free(err);
    g_free(debug_info);
}

/* ------------------------- New sample handler ----------------------- */
static GstFlowReturn on_new_sample(GstElement *sink, gpointer user_data)
{
    GstSample *sample = NULL;
    GstBuffer *buffer;
    GstMapInfo map;

    /* Get sample from appsink */
    g_signal_emit_by_name(sink, "pull-sample", &sample);
    if (sample)
    {
        buffer = gst_sample_get_buffer(sample);
        gst_buffer_map(buffer, &map, GST_MAP_READ);

        /* MJPEG frame send to stdout */
        fwrite(map.data, 1, map.size, stdout);
        fflush(stdout);

        gst_buffer_unmap(buffer, &map);
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }
    return GST_FLOW_ERROR;
}

/* ------------------------- Main function ---------------------------- */
int main(int argc, char *argv[])
{
    GstElement *pipeline, *source, *depay, *decoder, *convert, *jpegenc, *sink;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;

    if (argc != 2)
    {
        g_printerr("Usage: %s <rtsp-url>\n", argv[0]);
        return -1;
    }

    gst_init(&argc, &argv);

    /* ------------------------- Create elements ---------------------------- */
    source = gst_element_factory_make("rtspsrc", "source");
    depay = gst_element_factory_make("rtph264depay", "depay");
    decoder = gst_element_factory_make("avdec_h264", "decoder");
    convert = gst_element_factory_make("videoconvert", "convert");
    jpegenc = gst_element_factory_make("jpegenc", "jpegenc");
    sink = gst_element_factory_make("appsink", "sink");

    pipeline = gst_pipeline_new("rtsp-pipeline");

    if (!pipeline || !source || !depay || !decoder || !convert || !jpegenc || !sink)
    {
        g_printerr("Failed to create elements.\n");
        return -1;
    }

    /* ------------------------- Configure elements ---------------------------- */
    g_object_set(G_OBJECT(source), "location", argv[1], NULL);
    g_object_set(G_OBJECT(sink), "emit-signals", TRUE, "sync", FALSE, NULL);
    g_signal_connect(sink, "new-sample", G_CALLBACK(on_new_sample), NULL);

    /* ------------------------- Build the pipeline ---------------------------- */
    gst_bin_add_many(GST_BIN(pipeline), source, depay, decoder, convert, jpegenc, sink, NULL);
    gst_element_link_many(depay, decoder, convert, jpegenc, sink, NULL);

    /* Connect dynamic pad from rtspsrc to depay */
    g_signal_connect(source, "pad-added", G_CALLBACK(+[](GstElement *src, GstPad *new_pad, gpointer data)
                                                     {
                                                         GstPad *sink_pad = gst_element_get_static_pad((GstElement *)data, "sink");
                                                         gst_pad_link(new_pad, sink_pad);
                                                         gst_object_unref(sink_pad);
                                                     }),
                     depay);

    /* ------------------------- Start pipeline ---------------------------- */
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        g_printerr("Unable to start pipeline\n");
        gst_object_unref(pipeline);
        return -1;
    }

    /* ------------------------- Wait for EOS or error ---------------------------- */
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                     (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    if (msg != NULL)
        gst_message_unref(msg);

    /* ------------------------- Clean up ---------------------------- */
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipeline);
    return 0;
}

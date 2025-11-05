#include <gst/gst.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

typedef struct
{
    char *rtsp_url;
    int duration_sec;
    char *camera_name;
    char *base_path;
} AppConfig;

GstElement *pipeline_global = NULL;

// ==================== GStreamer Pipeline ====================
void int_handler(int sig)
{
    if (pipeline_global)
    {
        gst_element_send_event(pipeline_global, gst_event_new_eos());
    }
}

// ==================== GStreamer Pipeline ====================
void mkdir_recursive(const char *path)
{
    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s", path);
    for (char *p = tmp + 1; *p; p++)
    {
        if (*p == '/')
        {
            *p = 0;
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

// ==================== GStreamer Pipeline ====================
static gchar *format_location_cb(GstElement *splitmux, guint fragment_id, gpointer user_data)
{
    AppConfig *config = (AppConfig *)user_data;
    char folder[512];
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    snprintf(folder, sizeof(folder), "%s/%s/%04d/%02d/%02d/%02d",
             config->base_path,
             config->camera_name,
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
             tm.tm_hour);

    mkdir_recursive(folder);

    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s/video-%02d-%02d.mp4",
             folder,
             tm.tm_min, tm.tm_sec);

    return g_strdup(full_path);
}

// ==================== GStreamer Pipeline ====================
GstElement *build_pipeline(AppConfig *config)
{
    GError *error = NULL;
    GstElement *pipeline;
    char pipeline_desc[1024];

    snprintf(pipeline_desc, sizeof(pipeline_desc),
             "rtspsrc location=%s ! rtph264depay ! h264parse ! queue ! "
             "splitmuxsink name=mux muxer-factory=mp4mux max-size-time=%llu000000000",
             config->rtsp_url,
             (unsigned long long)config->duration_sec);

    pipeline = gst_parse_launch(pipeline_desc, &error);
    if (!pipeline)
    {
        g_printerr("Failed to create pipeline: %s\n", error ? error->message : "unknown");
        if (error)
            g_error_free(error);
        return NULL;
    }

    GstElement *mux = gst_bin_get_by_name(GST_BIN(pipeline), "mux");
    if (!mux)
    {
        g_printerr("Failed to get splitmuxsink element 'mux'\n");
        gst_object_unref(pipeline);
        return NULL;
    }

    g_signal_connect(mux, "format-location", G_CALLBACK(format_location_cb), config);
    gst_object_unref(mux);

    return pipeline;
}

// ==================== GStreamer Pipeline ====================
int main(int argc, char *argv[])
{
    if (argc < 5)
    {
        printf("Usage: %s <rtsp_url> <duration_sec> <camera_name> <base_path>\n", argv[0]);
        return 1;
    }

    AppConfig config;
    config.rtsp_url = argv[1];
    config.duration_sec = atoi(argv[2]);
    config.camera_name = argv[3];
    config.base_path = argv[4];

    gst_init(&argc, &argv);

    GstElement *pipeline = build_pipeline(&config);
    if (!pipeline)
        return 1;

    pipeline_global = pipeline;
    signal(SIGINT, int_handler);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_print("Recording from %s ...\n", config.rtsp_url);

    // ==================== GStreamer Pipeline ====================
    GstBus *bus = gst_element_get_bus(pipeline);
    GstMessage *msg;
    gboolean terminate = FALSE;

    while (!terminate)
    {
        msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                         GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

        if (msg != NULL)
        {
            GError *err = NULL;
            gchar *debug_info = NULL;

            switch (GST_MESSAGE_TYPE(msg))
            {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(msg, &err, &debug_info);
                g_printerr("Error received from element %s: %s\n",
                           GST_OBJECT_NAME(msg->src), err->message);
                g_error_free(err);
                g_free(debug_info);
                terminate = TRUE;
                break;
            case GST_MESSAGE_EOS:
                g_print("End-Of-Stream reached.\n");
                terminate = TRUE;
                break;
            default:
                break;
            }
            gst_message_unref(msg);
        }
    }

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    gst_object_unref(bus);

    g_print("Recording stopped.\n");
    return 0;
}

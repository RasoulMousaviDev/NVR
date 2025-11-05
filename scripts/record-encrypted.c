#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <openssl/evp.h>

typedef struct
{
    char *rtsp_url;
    int duration_sec;
    char *camera_name;
    char *base_path;
    unsigned char key[32];
    unsigned char iv[16];
} AppConfig;

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

    char path[1024];
    snprintf(path, sizeof(path), "%s/video-%02d-%02d.mp4",
             folder, tm.tm_min, tm.tm_sec);

    return g_strdup(path);
}

// ==================== GStreamer Pipeline ====================
int encrypt_buffer(unsigned char *inbuf, int inlen, unsigned char *outbuf,
                   EVP_CIPHER_CTX *ctx)
{
    int outlen;
    EVP_EncryptUpdate(ctx, outbuf, &outlen, inbuf, inlen);
    return outlen;
}

// ==================== GStreamer Pipeline ====================
GstFlowReturn on_new_sample(GstAppSink *appsink, gpointer user_data)
{
    FILE *f = (FILE *)user_data;
    GstSample *sample = gst_app_sink_pull_sample(appsink);
    if (!sample)
        return GST_FLOW_OK;

    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_READ))
    {
        static EVP_CIPHER_CTX *ctx = NULL;
        static unsigned char outbuf[8192];

        if (!ctx)
        {
            ctx = EVP_CIPHER_CTX_new();
            AppConfig *config = (AppConfig *)g_object_get_data(G_OBJECT(appsink), "app-config");
            EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, config->key, config->iv);
        }

        int outlen = encrypt_buffer(map.data, map.size, outbuf, ctx);
        fwrite(outbuf, 1, outlen, f);

        gst_buffer_unmap(buffer, &map);
    }

    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

// ==================== GStreamer Pipeline ====================
GstElement *build_pipeline(AppConfig *config, FILE **outf)
{
    GError *error = NULL;
    GstElement *pipeline;

    char pipeline_desc[1024];
    snprintf(pipeline_desc, sizeof(pipeline_desc),
             "rtspsrc location=%s ! rtph264depay ! h264parse ! appsink name=sink",
             config->rtsp_url);

    pipeline = gst_parse_launch(pipeline_desc, &error);
    if (!pipeline)
    {
        g_printerr("Failed to create pipeline: %s\n", error ? error->message : "unknown");
        if (error)
            g_error_free(error);
        return NULL;
    }

    GstElement *appsink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    *outf = fopen(format_location_cb(NULL, 0, config), "wb");
    g_object_set(appsink, "emit-signals", TRUE, "sync", FALSE, NULL);
    g_object_set_data(G_OBJECT(appsink), "app-config", config);
    g_signal_connect(appsink, "new-sample", G_CALLBACK(on_new_sample), *outf);
    gst_object_unref(appsink);

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

    memset(config.key, 0x00, 32);
    memset(config.iv, 0x00, 16);

    gst_init(&argc, &argv);

    FILE *outfile;
    GstElement *pipeline = build_pipeline(&config, &outfile);
    if (!pipeline)
        return 1;

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_print("Recording and encrypting from %s ...\n", config.rtsp_url);

    GstBus *bus = gst_element_get_bus(pipeline);
    GstMessage *msg;
    gboolean terminate = FALSE;

    while (!terminate)
    {
        msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                         GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
        if (msg != NULL)
        {
            switch (GST_MESSAGE_TYPE(msg))
            {
            case GST_MESSAGE_ERROR:
            {
                GError *err = NULL;
                gchar *debug_info = NULL;
                gst_message_parse_error(msg, &err, &debug_info);
                g_printerr("Error from %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
                g_error_free(err);
                g_free(debug_info);
                terminate = TRUE;
                break;
            }
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
    fclose(outfile);

    g_print("Recording stopped.\n");
    return 0;
}

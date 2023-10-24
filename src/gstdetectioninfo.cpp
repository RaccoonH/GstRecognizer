#include "gstdetectioninfo.h"

GType gst_meta_detection_info_api_get_type(void)
{
    static GType type;
    static const gchar *tags[] = {NULL};

    if (g_once_init_enter(&type)) {
        GType _type = gst_meta_api_type_register("GstMetaDetectionInfoAPI", tags);
        g_once_init_leave(&type, _type);
    }
    return type;
}

gboolean gst_meta_detection_info_init(GstMeta *meta, gpointer params, GstBuffer *buffer)
{
    GstMetaDetectionInfo *detectMeta = (GstMetaDetectionInfo *)meta;
    detectMeta->size = 0;
    detectMeta->data = nullptr;

    return TRUE;
}

void gst_meta_detection_info_free(GstMeta *meta, GstBuffer *buffer)
{
    GstMetaDetectionInfo *detectMeta = (GstMetaDetectionInfo *)meta;
    delete[] detectMeta->data;
}

const GstMetaInfo *gst_meta_detection_info_get_info(void)
{
    static const GstMetaInfo *meta_info = NULL;

    if (g_once_init_enter(&meta_info)) {
        const GstMetaInfo *meta =
            gst_meta_register(gst_meta_detection_info_api_get_type(), "GstMetaDetectionInfo",
                              sizeof(GstMetaDetectionInfo),
                              (GstMetaInitFunction)gst_meta_detection_info_init,
                              (GstMetaFreeFunction)NULL,
                              (GstMetaTransformFunction)NULL);
        g_once_init_leave(&meta_info, meta);
    }
    return meta_info;
}
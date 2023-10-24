#ifndef __GST_DETECTION_INFO_H__
#define __GST_DETECTION_INFO_H__

#include <cstdint>
#include <gst/gst.h>

G_BEGIN_DECLS

typedef struct _GstMetaDetectionInfo GstMetaDetectionInfo;

struct _GstMetaDetectionInfo
{
    GstMeta meta;

    uint8_t *data;
    size_t size;
};

GType gst_meta_detection_info_api_get_type(void);
const GstMetaInfo *gst_meta_detection_info_get_info(void);
#define GST_META_DETECTION_INFO_GET(buf) ((GstMetaDetectionInfo *)gst_buffer_get_meta(buf, gst_meta_detection_info_api_get_type()))
#define GST_META_DETECTION_INFO_ADD(buf) ((GstMetaDetectionInfo *)gst_buffer_add_meta(buf, gst_meta_detection_info_get_info(), (gpointer)NULL))

G_END_DECLS

#endif
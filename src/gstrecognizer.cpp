#include "gstrecognizer.h"
#include "gstdetectioninfo.h"

#include <gst/base/base.h>
#include <gst/controller/controller.h>
#include <gst/gst.h>

GST_DEBUG_CATEGORY_STATIC(gst_recognizer_debug);
#define GST_CAT_DEFAULT gst_recognizer_debug

enum
{
    PROP_0,
    PROP_DISPLAY_DETECTIONS
};

static GstStaticPadTemplate sink_template =
    GST_STATIC_PAD_TEMPLATE("sink",
                            GST_PAD_SINK,
                            GST_PAD_ALWAYS,
                            GST_STATIC_CAPS(GST_VIDEO_CAPS_MAKE("I420")));

static GstStaticPadTemplate src_template =
    GST_STATIC_PAD_TEMPLATE("src",
                            GST_PAD_SRC,
                            GST_PAD_ALWAYS,
                            GST_STATIC_CAPS(GST_VIDEO_CAPS_MAKE("I420")));

G_DEFINE_TYPE(GstRecognizer, gst_recognizer, GST_TYPE_VIDEO_FILTER);

static void gst_recognizer_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_recognizer_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static GstFlowReturn gst_recognizer_transform_frame(GstVideoFilter *vfilter, GstVideoFrame *in_frame, GstVideoFrame *out_frame);
static gboolean gst_recognizer_set_info(GstVideoFilter *filter_, GstCaps *incaps, GstVideoInfo *in_info, GstCaps *outcaps, GstVideoInfo *out_info);
static void gst_recognizer_finalize(GObject *object);

static void gst_recognizer_class_init(GstRecognizerClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstElementClass *gstelement_class = GST_ELEMENT_CLASS(klass);
    GstVideoFilterClass *vfilter_class = GST_VIDEO_FILTER_CLASS(klass);

    gobject_class->set_property = gst_recognizer_set_property;
    gobject_class->get_property = gst_recognizer_get_property;
    gobject_class->finalize = gst_recognizer_finalize;

    g_object_class_install_property(gobject_class, PROP_DISPLAY_DETECTIONS,
                                    g_param_spec_boolean("display-detections", "Display objects detections", "Display objects detections",
                                                         TRUE, GParamFlags(G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS)));

    gst_element_class_add_pad_template(gstelement_class, gst_static_pad_template_get(&src_template));
    gst_element_class_add_pad_template(gstelement_class, gst_static_pad_template_get(&sink_template));

    gst_element_class_set_static_metadata(gstelement_class,
                                          "Recognizer", "Filter/Effect/Video", "Recognizer",
                                          "Andrey Tudunov");

    vfilter_class->transform_frame = GST_DEBUG_FUNCPTR(gst_recognizer_transform_frame);
    vfilter_class->set_info = GST_DEBUG_FUNCPTR(gst_recognizer_set_info);
}

static void
gst_recognizer_init(GstRecognizer *filter)
{
    RecognizerConfig defaultConfig;
    defaultConfig.model = "./yolov5s.onnx";
    defaultConfig.classLabelsList = "./classes.txt";
    defaultConfig.scoreThreshold = 0.2;
    defaultConfig.confidenceThreshold = 0.4;
    filter->recognizer = CreateRecognizer(defaultConfig);

    filter->displayDetection = false;
}

static void
gst_recognizer_finalize(GObject *object)
{
    delete GST_RECOGNIZER(object)->recognizer;
}

static void
gst_recognizer_set_property(GObject *object, guint prop_id,
                            const GValue *value, GParamSpec *pspec)
{
    GstRecognizer *filter = GST_RECOGNIZER(object);

    switch (prop_id) {
    case PROP_DISPLAY_DETECTIONS:
        filter->displayDetection = g_value_get_boolean(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gst_recognizer_get_property(GObject *object, guint prop_id,
                            GValue *value, GParamSpec *pspec)
{
    GstRecognizer *filter = GST_RECOGNIZER(object);

    switch (prop_id) {
    case PROP_DISPLAY_DETECTIONS:
        g_value_set_boolean(value, filter->displayDetection);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static gboolean
gst_recognizer_set_info(GstVideoFilter *filter_, GstCaps *incaps, GstVideoInfo *in_info, GstCaps *outcaps, GstVideoInfo *out_info)
{
    return TRUE;
}

static GstFlowReturn
gst_recognizer_transform_frame(GstVideoFilter *vfilter, GstVideoFrame *inFrame, GstVideoFrame *outFrame)
{
    GstRecognizer *filter = GST_RECOGNIZER(vfilter);

    Frame frame;
    frame.data.resize(outFrame->info.size);
    frame.height = GST_VIDEO_FRAME_HEIGHT(outFrame);
    frame.width = GST_VIDEO_FRAME_WIDTH(outFrame);

    int frameDataItr = 0;
    // Remove paddings and copy
    for (uint32_t n = 0; n < GST_VIDEO_FRAME_N_PLANES(inFrame); n++) {
        auto ss = GST_VIDEO_INFO_PLANE_STRIDE(&inFrame->info, n);
        auto sp = inFrame->data[n];

        gint comp[GST_VIDEO_MAX_COMPONENTS];
        gst_video_format_info_component(outFrame->info.finfo, n, comp);
        auto w = GST_VIDEO_FRAME_COMP_WIDTH(outFrame, comp[0]) * GST_VIDEO_FRAME_COMP_PSTRIDE(outFrame, comp[0]);
        auto h = GST_VIDEO_FRAME_COMP_HEIGHT(outFrame, comp[0]);

        for (int j = 0; j < h; j++) {
            if (frame.data.size() < frameDataItr + w) {
                frame.data.resize(frameDataItr + w);
            }
            memcpy(frame.data.data() + frameDataItr, sp, w);
            frameDataItr += w;
            sp += ss;
        }
    }

    Frame out;
    std::vector<uint8_t> detectInfo;
    if (filter->displayDetection) {
        filter->recognizer->Recognize(frame, detectInfo, &out);
        frameDataItr = 0;

        for (uint32_t n = 0; n < GST_VIDEO_FRAME_N_PLANES(inFrame); n++) {
            auto ds = GST_VIDEO_INFO_PLANE_STRIDE(&outFrame->info, n);
            auto dp = outFrame->data[n];

            gint comp[GST_VIDEO_MAX_COMPONENTS];
            gst_video_format_info_component(outFrame->info.finfo, n, comp);
            auto w = GST_VIDEO_FRAME_COMP_WIDTH(outFrame, comp[0]) * GST_VIDEO_FRAME_COMP_PSTRIDE(outFrame, comp[0]);
            auto h = GST_VIDEO_FRAME_COMP_HEIGHT(outFrame, comp[0]);

            for (int j = 0; j < h; j++) {
                memcpy(dp, out.data.data() + frameDataItr, w);
                dp += ds;
                frameDataItr += w;
            }
        }
    } else {
        filter->recognizer->Recognize(frame, detectInfo, nullptr);
        gst_video_frame_copy(outFrame, inFrame);
    }

    auto meta = GST_META_DETECTION_INFO_ADD(outFrame->buffer);
    meta->data = new uint8_t[detectInfo.size()];
    meta->size = detectInfo.size();
    memcpy(meta->data, detectInfo.data(), detectInfo.size());

    return GST_FLOW_OK;
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
#ifndef VERSION
#define VERSION "1.0.0"
#endif
#ifndef PACKAGE
#define PACKAGE "recognizer"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "librecognizer.so"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "origin"
#endif

static gboolean
plugin_init(GstPlugin *plugin)
{
    GST_DEBUG_CATEGORY_INIT(gst_recognizer_debug, "recognizer", 0, "Recognizer");
    return gst_element_register(plugin, "recognizer", GST_RANK_NONE, GST_TYPE_RECOGNIZER);
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  recognizer,
                  "Recognizer",
                  plugin_init,
                  "1.0.0",
                  "LGPL",
                  PACKAGE_NAME,
                  GST_PACKAGE_ORIGIN)

#ifndef __GST_RECOGNIZER_H__
#define __GST_RECOGNIZER_H__

#include <gst/gst.h>
#include <gst/video/gstvideofilter.h>

#include "irecognizer.h"

G_BEGIN_DECLS

#define GST_TYPE_RECOGNIZER (gst_recognizer_get_type())
#define GST_RECOGNIZER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_RECOGNIZER, GstRecognizer))
#define GST_RECOGNIZER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_RECOGNIZER, GstRecognizerClass))
#define GST_IS_RECOGNIZER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_RECOGNIZER))
#define GST_IS_RECOGNIZER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_RECOGNIZER))

typedef struct _GstRecognizer GstRecognizer;
typedef struct _GstRecognizerClass GstRecognizerClass;

struct _GstRecognizer
{
    GstVideoFilter element;

    IRecognizer *recognizer;
    bool displayDetection;
};

struct _GstRecognizerClass
{
    GstVideoFilterClass parent_class;
};

GType gst_recognizer_get_type(void);

G_END_DECLS

#endif

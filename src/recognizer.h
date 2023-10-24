#include "irecognizer.h"
#include <filesystem>
#include <opencv2/opencv.hpp>

constexpr float INPUT_WIDTH = 640.0;
constexpr float INPUT_HEIGHT = 640.0;
constexpr float NMS_THRESHOLD = 0.4;
constexpr float CONFIDENCE_THRESHOLD = 0.4;
constexpr int YOLOV5_OUTPUT_ARRAY_ROWS = 25200;

struct Detection
{
    int classID;
    float confidence;
    cv::Rect box;
};

namespace RecognizerTools
{
    cv::Mat FormatYolov5(const cv::Mat &source);
    void ToFrame(const cv::Mat &mat, Frame *frame);
}

class Recognizer : public IRecognizer
{
public:
    Recognizer(const RecognizerConfig &config);
    ~Recognizer() override;

    bool Recognize(Frame &in, std::vector<uint8_t> &detectInfo, Frame *out) override;

private:
    std::vector<Detection> ParseYoloOutputArray(float xFactor, float yFactor, float *data);

private:
    cv::dnn::Net m_net;
    std::vector<std::string> m_classLabels;

    RecognizerConfig m_config;
};

IRecognizer *CreateRecognizer(const RecognizerConfig &config)
{
    return new Recognizer(config);
}
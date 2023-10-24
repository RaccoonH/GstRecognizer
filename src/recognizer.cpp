#include "recognizer.h"
#include <fstream>
#include <opencv2/imgproc/types_c.h>

Recognizer::Recognizer(const RecognizerConfig &config)
{
    if (!std::filesystem::exists(config.model)) {
        throw std::runtime_error("Model " + config.model.string() + " doesn't exist");
    }
    if (!std::filesystem::exists(config.classLabelsList)) {
        throw std::runtime_error("Class labels list " + config.model.string() + " doesn't exist");
    }
    m_net = cv::dnn::readNet(config.model);
    m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);

    std::ifstream ifs(config.classLabelsList);
    std::string line;
    while (getline(ifs, line)) {
        m_classLabels.push_back(line);
    }
    if (m_classLabels.empty()) {
        throw std::runtime_error("Class labels list is empty");
    }

    m_config = config;
}
Recognizer::~Recognizer()
{
}
const std::vector<cv::Scalar> colors = {cv::Scalar(255, 255, 0), cv::Scalar(0, 255, 0), cv::Scalar(0, 255, 255), cv::Scalar(255, 0, 0)};

bool Recognizer::Recognize(Frame &in, std::vector<uint8_t> &detectInfo, Frame *out)
{
    cv::Mat frameYUV{in.height + in.height / 2, in.width, CV_8UC1, (void *)in.data.data()};
    cv::Mat frameBGR{in.height, in.width, CV_8UC3};
    cv::cvtColor(frameYUV, frameBGR, CV_YUV2BGR_I420, 3);

    auto frameYolo = RecognizerTools::FormatYolov5(frameBGR);
    cv::Mat frameBlob;
    cv::dnn::blobFromImage(frameYolo, frameBlob, 1. / 255., cv::Size(INPUT_WIDTH, INPUT_HEIGHT), cv::Scalar(), true, false);

    m_net.setInput(frameBlob);
    std::vector<cv::Mat> outputs;
    m_net.forward(outputs, m_net.getUnconnectedOutLayersNames());

    float *data = (float *)outputs[0].data;
    auto size = outputs[0].total() * outputs[0].elemSize();
    detectInfo.resize(size);
    std::memcpy(detectInfo.data(), outputs[0].data, size);

    if (out) {
        auto detections = ParseYoloOutputArray(frameYolo.cols / INPUT_WIDTH, frameYolo.rows / INPUT_HEIGHT, data);
        for (auto &detect : detections) {
            auto box = detect.box;
            const auto color = colors[detect.classID % colors.size()];
            cv::rectangle(frameBGR, box, color, 3);

            cv::rectangle(frameBGR, cv::Point(box.x, box.y - 20), cv::Point(box.x + box.width, box.y), color, cv::FILLED);
            cv::putText(frameBGR, m_classLabels[detect.classID].c_str(), cv::Point(box.x, box.y - 5), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0));
        }

        cv::Mat frameYUVOut{in.height + in.height / 2, in.width, CV_8UC1};
        cv::cvtColor(frameBGR, frameYUVOut, CV_BGR2YUV_I420, 3);

        RecognizerTools::ToFrame(frameYUVOut, out);
    }

    return true;
}

cv::Mat RecognizerTools::FormatYolov5(const cv::Mat &source)
{
    int col = source.cols;
    int row = source.rows;
    int max = std::max(col, row);
    cv::Mat result = cv::Mat::zeros(max, max, CV_8UC3);
    source.copyTo(result(cv::Rect(0, 0, col, row)));
    return result;
}

void RecognizerTools::ToFrame(const cv::Mat &mat, Frame *frame)
{
    auto size = mat.total() * mat.elemSize();
    frame->data.resize(size);
    std::memcpy(frame->data.data(), mat.data, size);

    frame->height = mat.rows;
    frame->width = mat.cols;
}

constexpr int X_BYTE_IDX = 0;
constexpr int Y_BYTE_IDX = 1;
constexpr int WIDTH_BYTE_IDX = 2;
constexpr int HEIGHT_BYTE_IDX = 3;
constexpr int CONFIDENCE_BYTE_IDX = 4;
constexpr int CLASSES_SCORES_FIRST_BYTE_IDX = 5;
std::vector<Detection> Recognizer::ParseYoloOutputArray(float xFactor, float yFactor, float *data)
{
    std::vector<Detection> detections;

    std::vector<int> class_ids;
    std::vector<float> confidences;
    std::vector<cv::Rect> boxes;
    for (int i = 0; i < YOLOV5_OUTPUT_ARRAY_ROWS; ++i) {
        float confidence = data[CONFIDENCE_BYTE_IDX];
        if (confidence >= m_config.confidenceThreshold) {

            float *classes_scores = data + CLASSES_SCORES_FIRST_BYTE_IDX;
            cv::Mat scores(1, m_classLabels.size(), CV_32FC1, classes_scores);
            cv::Point class_id;
            double max_class_score;
            minMaxLoc(scores, 0, &max_class_score, 0, &class_id);
            if (max_class_score > m_config.scoreThreshold) {

                confidences.push_back(confidence);

                class_ids.push_back(class_id.x);

                float x = data[X_BYTE_IDX];
                float y = data[Y_BYTE_IDX];
                float w = data[WIDTH_BYTE_IDX];
                float h = data[HEIGHT_BYTE_IDX];
                int left = int((x - 0.5 * w) * xFactor);
                int top = int((y - 0.5 * h) * yFactor);
                int width = int(w * xFactor);
                int height = int(h * yFactor);
                boxes.push_back(cv::Rect(left, top, width, height));
            }
        }

        data += m_classLabels.size() + CLASSES_SCORES_FIRST_BYTE_IDX;
    }

    std::vector<int> nms_result;
    cv::dnn::NMSBoxes(boxes, confidences, m_config.scoreThreshold, NMS_THRESHOLD, nms_result);
    for (int i = 0; i < nms_result.size(); i++) {
        int idx = nms_result[i];
        Detection result;
        result.classID = class_ids[idx];
        result.confidence = confidences[idx];
        result.box = boxes[idx];
        detections.push_back(result);
    }

    return detections;
}
#pragma once

#include <cstdint>
#include <filesystem>
#include <stdio.h>
#include <vector>

struct Frame
{
    std::vector<uint8_t> data;
    int width;
    int height;
};

struct RecognizerConfig
{
    std::filesystem::path model;
    std::filesystem::path classLabelsList;

    float confidenceThreshold;
    float scoreThreshold;
};

class IRecognizer
{
public:
    virtual bool Recognize(Frame &in, std::vector<uint8_t> &detectInfo, Frame *out) = 0;
    virtual ~IRecognizer() = default;
};

IRecognizer *CreateRecognizer(const RecognizerConfig &config);
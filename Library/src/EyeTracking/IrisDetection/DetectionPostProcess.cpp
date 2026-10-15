#include "EyeTracking/IrisDetection/DetectionPostProcess.hpp"

#include "opencv2/core/types.hpp"
#include "opencv2/core/saturate.hpp"

#include <algorithm>
#include <vector>

static auto generateAnchors(
    my::AnchorOptions options
) -> std::vector<cv::Rect2f>
{
    std::vector<cv::Rect2f> anchors;
    for (int i = 0; i < NUM_SIZES; ++i)
    {
        auto size = options.sizes[i];
        auto numLayer = options.numLayers[i];

        for (auto y = 0; y < size; ++y)
        {
            for (auto x = 0; x < size; ++x)
            {
                float const xCenter = (x + options.offsetX) * 1.f / size;
                float const yCenter = (y + options.offsetY) * 1.f / size;
                float const w = 1.f;
                float const h = 1.f;
                anchors.insert(
                    anchors.end(),
                    numLayer,
                    cv::Rect2f(xCenter - (w / 2.f), yCenter - (h / 2.f), w, h)
                );
            }
        }
    }
    return anchors;
}

my::DetectionPostProcess::DetectionPostProcess()
    : anchors_(generateAnchors(AnchorOptions()))
{
}

auto my::DetectionPostProcess::decodeBox(
    const std::vector<float>& rawBoxes, int index
) const -> cv::Rect2f
{
    auto anchor = anchors_.at(index);
    auto center = (anchor.tl() + anchor.br()) * 0.5;

    auto boxOffset = index * NUM_COORD;
    float cx = rawBoxes.at(boxOffset);
    float cy = rawBoxes.at(boxOffset + 1);
    float w = rawBoxes.at(boxOffset + 2);
    float h = rawBoxes.at(boxOffset + 3);

    cx = (cx / DETECTION_SIZE * anchor.width) + center.x;
    cy = (cy / DETECTION_SIZE * anchor.height) + center.y;
    w = w / DETECTION_SIZE * anchor.width;
    h = h / DETECTION_SIZE * anchor.height;

    return {cx - (w / 2), cy - (h / 2), w, h};
}

auto my::DetectionPostProcess::getHighestScoreDetection(
    const std::vector<float>& rawBoxes, const std::vector<float>& scores
) const -> my::Detection
{
    my::Detection detection;
    for (int i = 0; i < NUM_BOXES; i++)
    {
        if (scores.at(i) > std::max(MIN_THRESHOLD, detection.score))
        {
            auto data = decodeBox(rawBoxes, i);
            detection = my::Detection(scores.at(i), CLASS_ID, data);
        }
    }
    return detection;
}

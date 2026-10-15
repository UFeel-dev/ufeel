#ifndef HEADCALIBRATION_HPP_
#define HEADCALIBRATION_HPP_

#include <string>
#include <unordered_map>

class HeadCalibration
{
    public:

        using Probabilities = std::unordered_map<std::string, float>;
        using Decisions = std::unordered_map<std::string, bool>;

        HeadCalibration() = default;

        explicit HeadCalibration(Probabilities thresholds);
        auto apply(const Probabilities& probs) const -> Decisions;
        auto applySoft(const Probabilities& probs) const -> Probabilities;

    private:

        Probabilities thresholds_;
};

#endif /* !HEADCALIBRATION_HPP_ */

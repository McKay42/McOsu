/*
Copyright (c) 2025 ppy Pty Ltd <contact@ppy.sh>.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
// Performance calculator port pinned to ppy/osu 2026.1005.0-lazer.
#ifndef OSU_LAZER_PERFORMANCE_H
#define OSU_LAZER_PERFORMANCE_H

#include "OsuLazerTypes.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace OsuLazer
{
namespace PerformanceDetail
{
// osu!stable legacy mod flags (OsuReplay::Mods). Keeping the values local makes
// this header usable by the standalone parity harness without McEngine headers.
enum LegacyMod
{
    NoFail = 1,
    Easy = 2,
    TouchDevice = 4, // Applied while producing difficulty attributes; no direct pp multiplier.
    Hidden = 8,
    HardRock = 16,
    DoubleTime = 64,
    Relax = 128,
    HalfTime = 256,
    Nightcore = 512,
    Flashlight = 1024,
    SpunOut = 4096,
    Autopilot = 8192,
    ScoreV2 = 536870912
};

inline bool hasMod(const Score &score, int mod)
{
    return (score.Mods & mod) != 0;
}

inline double accuracy(const Score &score)
{
    const int total = score.Great + score.Ok + score.Meh + score.Miss;
    if (total <= 0)
        return 0.0;

    return clamp((score.Great * 6.0 + score.Ok * 2.0 + score.Meh) / (total * 6.0), 0.0, 1.0);
}

inline double difficultyRange(double difficulty, double valueAtZero, double valueAtFive, double valueAtTen)
{
    const double scale = (difficulty - 5.0) / 5.0;
    return difficulty > 5.0
        ? valueAtFive + (valueAtTen - valueAtFive) * scale
        : valueAtFive + (valueAtFive - valueAtZero) * scale;
}

inline double inverseDifficultyRange(double value, double valueAtZero, double valueAtFive, double valueAtTen)
{
    if (value < valueAtFive)
        return 5.0 + 5.0 * (valueAtFive - value) / (valueAtFive - valueAtTen);

    return 5.0 - 5.0 * (value - valueAtFive) / (valueAtZero - valueAtFive);
}

inline double rateAdjustedApproachRate(double approachRate, double clockRate)
{
    const double preempt = difficultyRange(approachRate, 1800.0, 1200.0, 450.0) / clockRate;
    return inverseDifficultyRange(preempt, 1800.0, 1200.0, 450.0);
}

inline double erfApprox(double x)
{
    if (x == 0.0)
        return 0.0;
    if (std::isinf(x))
        return x > 0.0 ? 1.0 : -1.0;
    if (std::isnan(x))
        return std::numeric_limits<double>::quiet_NaN();

    const double t = 1.0 / (1.0 + 0.3275911 * std::abs(x));
    const double tau = t * (0.254829592 + t * (-0.284496736 + t * (1.421413741 + t * (-1.453152027 + t * 1.061405429))));
    const double value = 1.0 - tau * std::exp(-x * x);
    return x >= 0.0 ? value : -value;
}

inline double erfInv(double x)
{
    if (x <= -1.0)
        return -std::numeric_limits<double>::infinity();
    if (x >= 1.0)
        return std::numeric_limits<double>::infinity();
    if (x == 0.0)
        return 0.0;

    const double a = 0.147;
    const double sign = x < 0.0 ? -1.0 : 1.0;
    x = std::abs(x);
    const double logarithm = std::log(1.0 - x * x);
    const double t1 = 2.0 / (Pi * a) + logarithm / 2.0;
    const double t2 = logarithm / a;
    const double baseApproximation = std::sqrt(t1 * t1 - t2) - t1;
    const double correction = x >= 0.85 ? std::pow((x - 0.85) / 0.293, 8.0) : 0.0;
    return sign * (std::sqrt(baseApproximation) + correction);
}

inline double skillDifficultyToPerformance(double difficulty)
{
    return 4.0 * std::pow(difficulty, 3.0);
}

inline double flashlightDifficultyToPerformance(double difficulty)
{
    return 25.0 * std::pow(difficulty, 2.0);
}

inline double cognitionSum(double reading, double flashlight)
{
    if (reading <= 0.0)
        return flashlight;
    if (flashlight <= 0.0)
        return reading;

    return norm(1.1, reading, flashlight * clamp(flashlight / reading, 0.25, 1.0));
}

class Calculator
{
public:
    Calculator(const Attributes &attributes, const Score &score)
        : attributes(attributes), score(score)
    {
        classic = score.Classic;
        scoreV2 = score.ScoreV2 || hasMod(score, LegacyMod::ScoreV2);
        totalHits = std::max(0, score.Great) + std::max(0, score.Ok) + std::max(0, score.Meh) + std::max(0, score.Miss);
        totalSuccessfulHits = std::max(0, score.Great) + std::max(0, score.Ok) + std::max(0, score.Meh);
        totalImperfectHits = std::max(0, score.Ok) + std::max(0, score.Meh) + std::max(0, score.Miss);
        scoreMaxCombo = std::max(0, std::min(score.MaxCombo, attributes.MaxCombo));
        scoreAccuracy = PerformanceDetail::accuracy(score);
        clockRate = score.ClockRate > 0.0 ? score.ClockRate : 1.0;

        greatHitWindow = (std::floor(difficultyRange(score.OD, 80.0, 50.0, 20.0)) - 0.5) / clockRate;
        okHitWindow = (std::floor(difficultyRange(score.OD, 140.0, 100.0, 60.0)) - 0.5) / clockRate;
        mehHitWindow = (std::floor(difficultyRange(score.OD, 200.0, 150.0, 100.0)) - 0.5) / clockRate;
        overallDifficulty = (79.5 - greatHitWindow) / 6.0;
        approachRate = rateAdjustedApproachRate(score.AR, clockRate);
    }

    double calculate()
    {
        double comboBasedMissCount = calculateComboBasedEstimatedMissCount();
        effectiveMissCount = comboBasedMissCount;

        if (classic && !scoreV2 && score.LegacyTotalScore > 0.0)
            effectiveMissCount = calculateScoreBasedMissCount();

        effectiveMissCount = clamp(effectiveMissCount, static_cast<double>(std::max(0, score.Miss)), static_cast<double>(totalHits));

        if (effectiveMissCount > 0.0)
        {
            aimEstimatedSliderBreaks = calculateEstimatedSliderBreaks(attributes.AimTopWeightedSliderFactor);
            speedEstimatedSliderBreaks = calculateEstimatedSliderBreaks(attributes.SpeedTopWeightedSliderFactor);
        }

        double multiplier = 1.12;
        if (hasMod(score, LegacyMod::NoFail))
            multiplier *= std::max(0.90, 1.0 - 0.02 * effectiveMissCount);
        if (hasMod(score, LegacyMod::SpunOut) && totalHits > 0)
            multiplier *= 1.0 - std::pow(static_cast<double>(attributes.SpinnerCount) / totalHits, 0.85);

        if (hasMod(score, LegacyMod::Relax))
        {
            const double okMultiplier = 0.75 * std::max(0.0, overallDifficulty > 0.0 ? 1.0 - overallDifficulty / 13.33 : 1.0);
            const double mehMultiplier = std::max(0.0, overallDifficulty > 0.0 ? 1.0 - std::pow(overallDifficulty / 13.33, 5.0) : 1.0);
            effectiveMissCount = std::min(effectiveMissCount + score.Ok * okMultiplier + score.Meh * mehMultiplier, static_cast<double>(totalHits));
        }

        speedDeviation = calculateSpeedDeviation();

        const double aim = computeAimValue();
        const double speed = computeSpeedValue();
        const double accuracyValue = computeAccuracyValue();
        const double reading = computeReadingValue();
        const double flashlight = computeFlashlightValue();
        const double cognition = cognitionSum(reading, flashlight);

        return std::pow(std::pow(aim, 1.1) + std::pow(speed, 1.1) + std::pow(accuracyValue, 1.1) + std::pow(cognition, 1.1), 1.0 / 1.1) * multiplier;
    }

private:
    double computeAimValue() const
    {
        if (hasMod(score, LegacyMod::Autopilot))
            return 0.0;

        double aimDifficulty = attributes.AimDifficulty;
        if (attributes.SliderCount > 0 && attributes.AimDifficultSliderCount > 0.0)
        {
            double improperlyFollowed;
            if (classic)
            {
                const int maximumPossibleDroppedSliders = totalImperfectHits;
                improperlyFollowed = clamp(static_cast<double>(std::min(maximumPossibleDroppedSliders, attributes.MaxCombo - scoreMaxCombo)), 0.0, attributes.AimDifficultSliderCount);
            }
            else
            {
                improperlyFollowed = clamp(static_cast<double>(score.SliderEndsDropped + score.SliderTickMiss), 0.0, attributes.AimDifficultSliderCount);
            }

            const double sliderNerf = (1.0 - attributes.SliderFactor) * std::pow(1.0 - improperlyFollowed / attributes.AimDifficultSliderCount, 3.0) + attributes.SliderFactor;
            aimDifficulty *= sliderNerf;
        }

        double value = skillDifficultyToPerformance(aimDifficulty);
        value *= 0.95 + 0.35 * std::min(1.0, totalHits / 2000.0) + (totalHits > 2000 ? std::log10(totalHits / 2000.0) * 0.5 : 0.0);

        if (effectiveMissCount > 0.0)
        {
            const double relevantMisses = std::min(effectiveMissCount + aimEstimatedSliderBreaks, static_cast<double>(totalImperfectHits + score.SliderTickMiss));
            value *= missPenalty(relevantMisses, attributes.AimDifficultStrainCount);
        }

        value *= scoreAccuracy;
        return value;
    }

    double computeSpeedValue() const
    {
        if (hasMod(score, LegacyMod::Relax) || std::isnan(speedDeviation))
            return 0.0;

        double value = skillDifficultyToPerformance(attributes.SpeedDifficulty);
        if (effectiveMissCount > 0.0)
        {
            const double relevantMisses = std::min(effectiveMissCount + speedEstimatedSliderBreaks, static_cast<double>(totalImperfectHits + score.SliderTickMiss));
            value *= missPenalty(relevantMisses, attributes.SpeedDifficultStrainCount);
        }

        value *= calculateSpeedHighDeviationNerf();
        if (attributes.SpeedDifficulty <= 0.0)
            return 0.0;

        const double effectiveHitWindow = 20.0 * std::pow(4.0 / attributes.SpeedDifficulty, 0.35);
        const double effectiveAccuracy = erfApprox(effectiveHitWindow / speedDeviation);
        value *= std::pow(effectiveAccuracy, 2.0);
        return value;
    }

    double computeAccuracyValue() const
    {
        if (hasMod(score, LegacyMod::Relax))
            return 0.0;

        int objectsWithAccuracy = attributes.HitCircleCount;
        if (!classic || scoreV2)
            objectsWithAccuracy += attributes.SliderCount;

        double betterAccuracy = 0.0;
        if (objectsWithAccuracy > 0)
            betterAccuracy = ((score.Great - std::max(totalHits - objectsWithAccuracy, 0)) * 6.0 + score.Ok * 2.0 + score.Meh) / (objectsWithAccuracy * 6.0);
        betterAccuracy = std::max(0.0, betterAccuracy);

        double value = std::pow(1.52163, overallDifficulty) * std::pow(betterAccuracy, 24.0) * 2.83;
        const double length = objectsWithAccuracy / 1000.0;
        value *= objectsWithAccuracy < 1000 ? std::pow(length, 0.3) : std::pow(length, 0.1);
        return value;
    }

    double computeFlashlightValue() const
    {
        if (!hasMod(score, LegacyMod::Flashlight))
            return 0.0;

        double value = flashlightDifficultyToPerformance(attributes.FlashlightDifficulty);
        if (effectiveMissCount > 0.0 && totalHits > 0)
            value *= 0.97 * std::pow(1.0 - std::pow(effectiveMissCount / totalHits, 0.775), std::pow(effectiveMissCount, 0.875));
        value *= comboScalingFactor();
        value *= 0.5 + scoreAccuracy / 2.0;
        return value;
    }

    double computeReadingValue() const
    {
        double value = skillDifficultyToPerformance(attributes.ReadingDifficulty);
        if (effectiveMissCount > 0.0)
            value *= missPenalty(effectiveMissCount + aimEstimatedSliderBreaks, attributes.ReadingDifficultNoteCount);
        value *= std::pow(scoreAccuracy, 3.0);
        return value;
    }

    double calculateComboBasedEstimatedMissCount() const
    {
        if (attributes.SliderCount <= 0)
            return score.Miss;

        double missCount = score.Miss;
        if (classic)
        {
            const double likelyMissedSliderEndPortion = 0.04 + 0.06 * std::pow(std::min(attributes.AimTopWeightedSliderFactor, 1.0), 2.0);
            const double fullComboThreshold = attributes.MaxCombo - std::min(4.0 + likelyMissedSliderEndPortion * attributes.SliderCount, static_cast<double>(attributes.SliderCount));
            if (scoreMaxCombo < fullComboThreshold)
                missCount = fullComboThreshold / std::max(1.0, static_cast<double>(scoreMaxCombo));

            missCount = std::min(missCount, static_cast<double>(totalImperfectHits));
            const int maximumSliderBreaks = std::min(attributes.SliderCount, (attributes.MaxCombo - scoreMaxCombo) / 2);
            if (missCount - score.Miss > maximumSliderBreaks)
                missCount = score.Miss + maximumSliderBreaks;
        }
        else
        {
            const double fullComboThreshold = attributes.MaxCombo - score.SliderEndsDropped;
            if (scoreMaxCombo < fullComboThreshold)
                missCount = fullComboThreshold / std::max(1.0, static_cast<double>(scoreMaxCombo));
            missCount = std::min(missCount, static_cast<double>(score.SliderTickMiss + score.Miss));
        }
        return missCount;
    }

    double calculateEstimatedSliderBreaks(double topWeightedSliderFactor) const
    {
        const int nonMissMistakes = score.Ok + score.Meh;
        if (!classic || nonMissMistakes == 0 || attributes.MaxCombo <= 0)
            return 0.0;

        const double missedComboPercent = 1.0 - static_cast<double>(scoreMaxCombo) / attributes.MaxCombo;
        double estimated = std::min(static_cast<double>(nonMissMistakes), effectiveMissCount * topWeightedSliderFactor);
        const double mistakeAdjustment = (nonMissMistakes - estimated + 4.5) / (nonMissMistakes + 4.0);
        estimated *= smoothstep(effectiveMissCount, 1.0, 2.0);
        return estimated * mistakeAdjustment * logistic(missedComboPercent, 0.33, 15.0);
    }

    double calculateSpeedDeviation() const
    {
        if (totalSuccessfulHits == 0)
            return std::numeric_limits<double>::quiet_NaN();

        double speedNotes = attributes.SpeedNoteCount + (totalHits - attributes.SpeedNoteCount) * 0.1;
        const double misses = std::min(static_cast<double>(score.Miss), speedNotes);
        const double mehs = std::min(static_cast<double>(score.Meh), speedNotes - misses);
        const double oks = std::min(static_cast<double>(score.Ok), speedNotes - misses - mehs);
        const double greats = std::max(0.0, speedNotes - misses - mehs - oks);
        return calculateDeviation(greats, oks, mehs);
    }

    double calculateDeviation(double greats, double oks, double mehs) const
    {
        if (greats + oks + mehs <= 0.0)
            return std::numeric_limits<double>::quiet_NaN();

        const double n = std::max(1.0, greats + oks);
        const double p = greats / n;
        const double z = 2.32634787404;
        const double pLowerBound = std::min(p, (n * p + z * z / 2.0) / (n + z * z) - z / (n + z * z) * std::sqrt(n * p * (1.0 - p) + z * z / 4.0));

        double deviation;
        if (pLowerBound > 0.01)
        {
            const double sqrtTwo = 1.4142135623730950;
            deviation = greatHitWindow / (sqrtTwo * erfInv(pLowerBound));
            const double tail = std::sqrt(2.0 / Pi) * okHitWindow * std::exp(-0.5 * std::pow(okHitWindow / deviation, 2.0))
                              / (deviation * erfApprox(okHitWindow / (sqrtTwo * deviation)));
            deviation *= std::sqrt(1.0 - tail);
        }
        else
            deviation = okHitWindow / std::sqrt(3.0);

        const double mehVariance = (mehHitWindow * mehHitWindow + okHitWindow * mehHitWindow + okHitWindow * okHitWindow) / 3.0;
        return std::sqrt(((greats + oks) * deviation * deviation + mehs * mehVariance) / (greats + oks + mehs));
    }

    double calculateSpeedHighDeviationNerf() const
    {
        const double speedValue = skillDifficultyToPerformance(attributes.SpeedDifficulty);
        const double cutoff = 100.0 + 220.0 * std::pow(22.0 / speedDeviation, 6.5);
        if (speedValue <= cutoff)
            return 1.0;

        const double scale = 50.0;
        double adjusted = scale * (std::log((speedValue - cutoff) / scale + 1.0) + cutoff / scale);
        adjusted = lerp(adjusted, speedValue, 1.0 - reverseLerp(speedDeviation, 22.0, 27.0));
        return adjusted / speedValue;
    }

    double calculateScoreBasedMissCount() const
    {
        if (attributes.MaxCombo == 0 || score.LegacyTotalScore == 0.0)
            return 0.0;

        const double scoreMultiplier = attributes.LegacyScoreBaseMultiplier * legacyScoreMultiplier();
        const double relevantComboPerObject = calculateRelevantComboPerObject();
        const double maximumMissCount = calculateMaximumComboBasedMissCount();
        const double scoreAtMaximumCombo = calculateScoreAtCombo(scoreMaxCombo, relevantComboPerObject, scoreMultiplier);
        const double remainingScore = score.LegacyTotalScore - scoreAtMaximumCombo;
        if (remainingScore <= 0.0)
            return maximumMissCount;

        const double expectedRemainingScore = calculateScoreAtCombo(attributes.MaxCombo - scoreMaxCombo, relevantComboPerObject, scoreMultiplier);
        return std::min(std::max(expectedRemainingScore / remainingScore, 1.0), maximumMissCount);
    }

    double calculateScoreAtCombo(double combo, double comboPerObject, double multiplier) const
    {
        const double estimatedObjects = combo / comboPerObject - 1.0;
        double comboScore = comboPerObject > 0.0
            ? (2.0 * (comboPerObject - 1.0) + (estimatedObjects - 1.0) * comboPerObject) * estimatedObjects / 2.0
            : 0.0;
        comboScore *= scoreAccuracy * 300.0 / 25.0 * multiplier;
        const double objectsHit = (totalHits - score.Miss) * combo / attributes.MaxCombo;
        return comboScore + (300.0 + attributes.NestedScorePerObject) * scoreAccuracy * objectsHit;
    }

    double calculateRelevantComboPerObject() const
    {
        double comboScore = attributes.MaximumLegacyComboScore;
        comboScore /= 300.0 / 25.0 * attributes.LegacyScoreBaseMultiplier;
        double result = (attributes.MaxCombo - 2.0) * attributes.MaxCombo;
        result /= std::max(attributes.MaxCombo + 2.0 * (comboScore - 1.0), 1.0);
        return result;
    }

    double calculateMaximumComboBasedMissCount() const
    {
        if (attributes.SliderCount <= 0)
            return score.Miss;

        const double likelyMissedSliderEndPortion = 0.04 + 0.06 * std::pow(std::min(attributes.AimTopWeightedSliderFactor, 1.0), 2.0);
        const double fullComboThreshold = attributes.MaxCombo - std::min(4.0 + likelyMissedSliderEndPortion * attributes.SliderCount, static_cast<double>(attributes.SliderCount));
        double missCount = 0.0;
        if (scoreMaxCombo < fullComboThreshold)
            missCount = std::pow(fullComboThreshold / std::max(1.0, static_cast<double>(scoreMaxCombo)), 2.5);

        missCount = std::min(missCount, static_cast<double>(totalImperfectHits));
        const int maximumSliderBreaks = std::min(attributes.SliderCount, (attributes.MaxCombo - scoreMaxCombo) / 2);
        if (missCount - score.Miss > maximumSliderBreaks)
            missCount = score.Miss + maximumSliderBreaks;
        return missCount;
    }

    double legacyScoreMultiplier() const
    {
        double multiplier = 1.0;
        if (hasMod(score, LegacyMod::NoFail)) multiplier *= scoreV2 ? 1.0 : 0.5;
        if (hasMod(score, LegacyMod::Easy)) multiplier *= 0.5;
        if (hasMod(score, LegacyMod::HalfTime)) multiplier *= 0.3;
        if (hasMod(score, LegacyMod::Hidden)) multiplier *= 1.06;
        if (hasMod(score, LegacyMod::HardRock)) multiplier *= scoreV2 ? 1.10 : 1.06;
        if (hasMod(score, LegacyMod::DoubleTime) || hasMod(score, LegacyMod::Nightcore)) multiplier *= scoreV2 ? 1.20 : 1.12;
        if (hasMod(score, LegacyMod::Flashlight)) multiplier *= 1.12;
        if (hasMod(score, LegacyMod::SpunOut)) multiplier *= 0.9;
        if (hasMod(score, LegacyMod::Relax) || hasMod(score, LegacyMod::Autopilot)) return 0.0;
        return multiplier;
    }

    double missPenalty(double misses, double difficultCount) const
    {
        return 0.93 / (misses / (4.0 * std::log(std::max(1.0, difficultCount))) + 1.0);
    }

    double comboScalingFactor() const
    {
        if (attributes.MaxCombo <= 0)
            return 1.0;
        return std::min(std::pow(static_cast<double>(scoreMaxCombo), 0.8) / std::pow(static_cast<double>(attributes.MaxCombo), 0.8), 1.0);
    }

    const Attributes &attributes;
    const Score &score;
    bool classic = true;
    bool scoreV2 = false;
    int totalHits = 0;
    int totalSuccessfulHits = 0;
    int totalImperfectHits = 0;
    int scoreMaxCombo = 0;
    double scoreAccuracy = 0.0;
    double clockRate = 1.0;
    double greatHitWindow = 0.0;
    double okHitWindow = 0.0;
    double mehHitWindow = 0.0;
    double overallDifficulty = 0.0;
    double approachRate = 0.0;
    double effectiveMissCount = 0.0;
    double aimEstimatedSliderBreaks = 0.0;
    double speedEstimatedSliderBreaks = 0.0;
    double speedDeviation = std::numeric_limits<double>::quiet_NaN();
};
}

inline double performance(const Attributes &attributes, const Score &score)
{
    return PerformanceDetail::Calculator(attributes, score).calculate();
}
}

#endif

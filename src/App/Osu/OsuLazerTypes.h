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
// osu!standard calculator port, pinned to ppy/osu 2026.1005.0-lazer.
#ifndef OSU_LAZER_TYPES_H
#define OSU_LAZER_TYPES_H

#include <algorithm>
#include <cmath>
#include <vector>
#include <limits>
#include <atomic>

namespace OsuLazer
{
constexpr int Version = 20260706;
constexpr double Pi = 3.14159265358979323846;
inline double clamp(double x, double lo, double hi) { return std::max(lo, std::min(hi, x)); }
inline double reverseLerp(double x, double lo, double hi) { return clamp((x-lo)/(hi-lo), 0, 1); }
inline double smoothstep(double x, double lo, double hi) { x=reverseLerp(x,lo,hi); return x*x*(3-2*x); }
inline double smootherstep(double x, double lo, double hi) { x=reverseLerp(x,lo,hi); return x*x*x*(x*(x*6-15)+10); }
inline double logistic(double x, double midpoint, double multiplier, double maximum=1) { return maximum/(1+std::exp(multiplier*(midpoint-x))); }
inline double lerp(double a, double b, double t) { return a+(b-a)*t; }
inline double norm(double p, double a, double b) { return std::pow(std::pow(a,p)+std::pow(b,p),1/p); }
inline double norm(double p, double a, double b, double c) { return std::pow(std::pow(a,p)+std::pow(b,p)+std::pow(c,p),1/p); }
struct Vec2
{
    float X=0, Y=0;
    Vec2() = default;
    Vec2(float x, float y): X(x), Y(y) {}
    Vec2 operator-(const Vec2 &b) const { return Vec2(X-b.X,Y-b.Y); }
    Vec2 operator+(const Vec2 &b) const { return Vec2(X+b.X,Y+b.Y); }
    Vec2 operator*(float s) const { return Vec2(X*s,Y*s); }
    double length() const { return std::sqrt(X*X+Y*Y); }
};
enum class ObjectType { Circle, Slider, Spinner };
// Contains difficulty objects only: Index=0 represents the second base hitobject.
// Time values are clock-rate adjusted except RawStartTime and RawPreempt.
struct Object
{
    int Index=0;
    ObjectType Type=ObjectType::Circle;
    Vec2 Position, EndPosition, BaseEndPosition, LastPosition;
    double StartTime=0, EndTime=0, RawStartTime=0, RawPreempt=1200;
    double DeltaTime=0, AdjustedDeltaTime=25, LastObjectEndDeltaTime=25;
    double Preempt=1200, HitWindowGreat=100, Radius=32;
    double JumpDistance=0, LazyJumpDistance=0, MinimumJumpDistance=0, MinimumJumpTime=25;
    double TravelDistance=0, TravelTime=0, LazyTravelDistance=0, LazyTravelTime=0;
    double Angle=std::numeric_limits<double>::quiet_NaN();
    double NormalisedVectorAngle=std::numeric_limits<double>::quiet_NaN();
    double SmallCircleBonus=1, OverallDifficulty=5;
    int RepeatCount=0;
    double OpacityAt(double rawTime, bool hidden) const
    {
        if (rawTime>RawStartTime) return 0;
        double fadeStart=RawStartTime-RawPreempt;
        double fadeDuration=400*std::min(1.0,RawPreempt/450.0);
        double opacity=clamp((rawTime-fadeStart)/std::max(1e-9,fadeDuration),0,1);
        if (hidden)
        {
            double hiddenFadeIn = Type == ObjectType::Slider ? fadeDuration : RawPreempt*0.4;
            opacity=std::min(opacity,1-clamp((rawTime-fadeStart-hiddenFadeIn)/std::max(1e-9,RawPreempt*0.3),0,1));
        }
        return opacity;
    }
    double DoubleTapFeasibility(const Object *next) const
    {
        if (!next) return 0;
        double dt=std::max(1.0,DeltaTime), ndt=std::max(1.0,next->DeltaTime);
        double ratio=dt/std::max(dt,std::abs(ndt-dt));
        double window=std::pow(std::min(1.0,dt/HitWindowGreat),5);
        double distance=std::pow(reverseLerp(LazyJumpDistance,100,50),2);
        return 1-std::pow(ratio,distance*(1-window));
    }
};
struct Attributes
{
    double StarRating=0, AimDifficulty=0, SpeedDifficulty=0, ReadingDifficulty=0, FlashlightDifficulty=0;
    double AimDifficultSliderCount=0, SpeedNoteCount=0, SliderFactor=1;
    double AimDifficultStrainCount=0, SpeedDifficultStrainCount=0, ReadingDifficultNoteCount=0;
    double AimTopWeightedSliderFactor=0, SpeedTopWeightedSliderFactor=0;
    double NestedScorePerObject=0, LegacyScoreBaseMultiplier=0, MaximumLegacyComboScore=0;
    int MaxCombo=0, HitCircleCount=0, SliderCount=0, SpinnerCount=0;
};
struct Mods
{
    bool Hidden=false, Flashlight=false, Relax=false, Autopilot=false, TouchDevice=false;
};
struct Result
{
    Attributes attributes;
    std::vector<double> aimStrains, speedStrains;
};
struct Score
{
    int Mods=0, Great=0, Ok=0, Meh=0, Miss=0, MaxCombo=0;
    double ClockRate=1, AR=5, OD=5, HP=5, LegacyTotalScore=0;
    bool Classic=true, ScoreV2=false;
    int SliderTickMiss=0, SliderEndsDropped=0;
};
}
#endif

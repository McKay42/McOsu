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
// Adapter from McOsu's loaded beatmap geometry to the pinned lazer calculator.
#ifndef OSU_LAZER_ADAPTER_H
#define OSU_LAZER_ADAPTER_H
#include "OsuLazerTypes.h"
#include "OsuSliderCurves.h"

namespace OsuLazer
{
inline Vec2 vector(const Vector2 &p) { return Vec2(p.x,p.y); }
inline double angleBetween(Vec2 current, Vec2 previous, Vec2 previousPrevious)
{
    Vec2 a=previousPrevious-previous, b=current-previous;
    float dot=a.X*b.X+a.Y*b.Y, det=a.X*b.Y-a.Y*b.X;
    return std::abs(std::atan2(det,dot));
}
inline std::vector<Object> preprocess(const OsuDifficultyCalculator::BeatmapDiffcalcData &b,
                                      const std::atomic<bool> &cancelled)
{
    std::vector<Object> objects;
    if (b.sortedHitObjects.size()<2 || !(b.speedMultiplier>0)) return objects;
    objects.reserve(b.sortedHitObjects.size());
    double radius=64.0*OsuGameRules::getRawHitCircleScale(::clamp<float>(b.CS,0,12.142f));
    float scale=50.0f/(float)radius;
    double preempt=std::max(1.0,(double)(int)OsuGameRules::getRawApproachTime(b.AR));
    std::vector<Vec2> secondLastNested(b.sortedHitObjects.size());
    // The first object is not a difficulty object, but its slider geometry and
    // position are needed when preprocessing the following two objects.
    for (size_t i=0;i<b.sortedHitObjects.size();++i)
    {
        if (cancelled.load()) return {};
        OsuDifficultyHitObject &base=b.sortedHitObjects[i];
        const OsuDifficultyHitObject &last=b.sortedHitObjects[i>0?i-1:0];
        Object o;
        o.Index=(int)i-1;
        o.Type=base.type==OsuDifficultyHitObject::TYPE::SLIDER?ObjectType::Slider:
               base.type==OsuDifficultyHitObject::TYPE::SPINNER?ObjectType::Spinner:ObjectType::Circle;
        o.Position=vector(base.pos); o.LastPosition=vector(last.pos); o.EndPosition=o.BaseEndPosition=o.Position;
        o.RawStartTime=base.baseTime; o.RawPreempt=preempt;
        o.StartTime=(double)base.baseTime/b.speedMultiplier;
        o.EndTime=(double)base.baseEndTime/b.speedMultiplier;
        o.DeltaTime=(double)(base.baseTime-last.baseTime)/b.speedMultiplier;
        o.AdjustedDeltaTime=std::max(25.0,o.DeltaTime);
        o.LastObjectEndDeltaTime=objects.empty()?o.AdjustedDeltaTime:std::max(25.0,o.StartTime-objects.back().EndTime);
        o.Radius=radius; o.Preempt=preempt/b.speedMultiplier;
        o.HitWindowGreat=2*(std::floor(80-6*(double)b.OD)-0.5)/b.speedMultiplier;
        o.HitWindowGreat=std::max(0.01,o.HitWindowGreat);
        o.OverallDifficulty=(79.5-o.HitWindowGreat/2)/6;
        o.SmallCircleBonus=std::max(1.0,1+(30-radius)/70);
        o.MinimumJumpTime=o.AdjustedDeltaTime;
        o.RepeatCount=std::max(0,base.repeats-1);
        if (o.Type==ObjectType::Slider)
        {
            if (!base.curve && base.scheduledCurveAlloc)
            {
                base.curve=OsuSliderCurve::createCurve(base.osuSliderCurveType,base.scheduledCurveAllocControlPoints,base.pixelLength,1.0f);
                base.updateCurveStackPosition(base.scheduledCurveAllocStackOffset);
            }
            const double span=base.spanDuration;
            const double duration=span*base.repeats;
            o.EndTime=o.StartTime+duration;
            // McOsu's nested times are already rate-adjusted. Its END is placed at the legacy leniency time.
            double tracking=std::max(duration-36.0/b.speedMultiplier,duration/2);
            for (const auto &t:base.scoringTimes)
                if(t.type==OsuDifficultyHitObject::SLIDER_SCORING_TIME::TYPE::TICK)
                    tracking=std::max(tracking,(double)t.time-o.StartTime);
            o.LazyTravelTime=tracking;
            if(base.curve && span>0)
            {
                o.BaseEndPosition=vector(base.curve->pointAt(base.repeats%2?1.0:0.0));
                auto at=[&](double t) { double p=t/span; p=std::fmod(p,2.0)>=1?1-std::fmod(p,1.0):std::fmod(p,1.0); return vector(base.curve->pointAt(clamp(p,0,1))); };
                Vec2 lazy=at(tracking), cursor=o.Position;
                secondLastNested[i]=o.Position;
                for(size_t j=0;j<base.scoringTimes.size();++j)
                {
                    const auto &t=base.scoringTimes[j];
                    Vec2 position=t.type==OsuDifficultyHitObject::SLIDER_SCORING_TIME::TYPE::END?
                        vector(base.curve->pointAt(base.repeats%2?1.0:0.0)):at((double)t.time-o.StartTime);
                    if(j+2==base.scoringTimes.size()) secondLastNested[i]=position;
                    Vec2 movement=position-cursor;
                    double length=50.0/radius*movement.length(), required=90;
                    if(j+1==base.scoringTimes.size())
                    {
                        Vec2 lazyMovement=lazy-cursor;
                        if(lazyMovement.length()<movement.length()) movement=lazyMovement;
                        length=50.0/radius*movement.length();
                    }
                    else if(t.type==OsuDifficultyHitObject::SLIDER_SCORING_TIME::TYPE::REPEAT) required=50;
                    if(length>required)
                    {
                        cursor=cursor+movement*(float)((length-required)/length);
                        o.LazyTravelDistance+=length-required;
                    }
                }
                o.EndPosition=cursor;
            }
            o.TravelDistance=o.LazyTravelDistance*std::max(1.0,std::pow((double)o.RepeatCount,0.3));
            o.TravelTime=std::max(25.0,o.LazyTravelTime);
        }
        if(i>0 && o.Type!=ObjectType::Spinner && last.type!=OsuDifficultyHitObject::TYPE::SPINNER)
        {
            const Object *prev=objects.empty()?nullptr:&objects.back();
            Vec2 lastCursor=prev?prev->EndPosition:o.LastPosition;
            o.JumpDistance=(o.LastPosition-o.Position).length()*scale;
            o.LazyJumpDistance=(o.Position-lastCursor).length()*scale;
            o.MinimumJumpDistance=o.LazyJumpDistance;
            if(prev && prev->Type==ObjectType::Slider)
            {
                o.MinimumJumpTime=std::max(25.0,o.AdjustedDeltaTime-std::max(25.0,prev->LazyTravelTime));
                Vec2 tail=last.curve?vector(last.curve->pointAt(last.repeats%2?1.0:0.0)):o.LastPosition;
                double tailDistance=(tail-o.Position).length()*scale;
                o.MinimumJumpDistance=std::max(0.0,std::min(o.LazyJumpDistance-30,tailDistance-120));
            }
            if(objects.size()>=2 && objects[objects.size()-2].Type!=ObjectType::Spinner)
            {
                Vec2 lastLast=objects[objects.size()-2].EndPosition;
                Vec2 angleCursor=prev->Type==ObjectType::Slider&&prev->TravelDistance>0?prev->Position:lastCursor;
                double angle=angleBetween(o.Position,angleCursor,lastLast);
                Vec2 sliderLast=prev->Type==ObjectType::Slider&&prev->TravelDistance>0?secondLastNested[i-1]:lastLast;
                o.Angle=std::min(angle,angleBetween(o.Position,lastCursor,sliderLast));
                Vec2 v=o.Position-angleCursor;
                o.NormalisedVectorAngle=std::atan2(std::abs(v.Y),std::abs(v.X));
            }
        }
        objects.push_back(o);
        if(i>0 && b.sortedHitObjects[i-1].scheduledCurveAlloc)
        {
            delete b.sortedHitObjects[i-1].curve;
            b.sortedHitObjects[i-1].curve=nullptr;
        }
    }
    objects.erase(objects.begin());
    return objects;
}
}
#endif

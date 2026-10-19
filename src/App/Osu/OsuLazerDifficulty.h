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

// Ported from ppy/osu 2026.1005.0-lazer.
// Copyright (c) ppy Pty Ltd <contact@ppy.sh>. Licensed under the MIT Licence.
// Upstream permission notice is included above.
#ifndef OSU_LAZER_DIFFICULTY_H
#define OSU_LAZER_DIFFICULTY_H

#include "OsuLazerTypes.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <functional>
#include <vector>

namespace OsuLazer
{
namespace Difficulty
{
static const double normalisedRadius = 50.0;
static const double normalisedDiameter = 100.0;
static const double performanceNormExponent = 1.1;

inline bool finiteAngle(double value) { return !std::isnan(value); }
inline double bpm(double ms, int delimiter = 4) { return 60000.0 / (ms * delimiter); }
inline double bell(double x)
{
    x = clamp((0.5 - std::abs(x - 0.5)) * 2.0, 0.0, 1.0);
    return x * x * (3.0 - 2.0 * x);
}
inline const Object *previous(const std::vector<Object> &objects, const Object &current, int skip = 0)
{
    const int index = current.Index - skip - 1;
    return index >= 0 && index < (int)objects.size() ? &objects[index] : NULL;
}
inline const Object *next(const std::vector<Object> &objects, const Object &current, int skip = 0)
{
    const int index = current.Index + skip + 1;
    return index >= 0 && index < (int)objects.size() ? &objects[index] : NULL;
}
inline double distance(const Vec2 &a, const Vec2 &b) { return (a - b).length(); }

inline double acute(double angle) { return smoothstep(angle, 140.0 * Pi / 180.0, 40.0 * Pi / 180.0); }
inline double wide(double angle) { return smoothstep(angle, 40.0 * Pi / 180.0, 140.0 * Pi / 180.0); }

inline double vectorRepetition(const std::vector<Object> &objects, const Object &current, const Object &last)
{
    if (!finiteAngle(current.Angle) || !finiteAngle(last.Angle)) return 1.0;
    double count = 0;
    for (int i = 0; i < 6; ++i)
    {
        const Object *p = previous(objects, current, i);
        if (!p) break;
        if (std::max(current.AdjustedDeltaTime, p->AdjustedDeltaTime) > 1.1 * std::min(current.AdjustedDeltaTime, p->AdjustedDeltaTime)) break;
        if (finiteAngle(p->NormalisedVectorAngle) && finiteAngle(current.NormalisedVectorAngle))
            count += std::cos(8 * std::min(11.25 * Pi / 180.0, std::abs(current.NormalisedVectorAngle - p->NormalisedVectorAngle)));
    }
    const double repetition = std::pow(std::min(0.5 / count, 1.0), 2);
    const double stack = smootherstep(current.LazyJumpDistance, 0, normalisedDiameter);
    const double adjusted = std::cos(2 * std::min(45.0 * Pi / 180.0, std::abs(current.Angle - last.Angle) * stack));
    const double base = 1 - 0.15 * acute(last.Angle) * adjusted;
    return std::pow(base + (1 - base) * repetition * 0.5 * stack, 2);
}

inline double snap(const std::vector<Object> &objects, const Object &current, bool sliders)
{
    const Object *last = previous(objects, current);
    if (!last || current.Type == ObjectType::Spinner || current.Index <= 1 || last->Type == ObjectType::Spinner) return 0;
    const Object *last2 = previous(objects, current, 2);
    double currDistance = sliders ? current.LazyJumpDistance : current.JumpDistance;
    double currVelocity = currDistance / current.AdjustedDeltaTime;
    if (last->Type == ObjectType::Slider && sliders)
        currVelocity = std::max(currVelocity, (last->LazyTravelDistance + current.LazyJumpDistance) / current.AdjustedDeltaTime);
    const double prevDistance = sliders ? last->LazyJumpDistance : last->JumpDistance;
    const double prevVelocity = prevDistance / last->AdjustedDeltaTime;
    double value = currVelocity * vectorRepetition(objects, current, *last);
    if (finiteAngle(current.Angle) && finiteAngle(last->Angle))
    {
        const double velocity = std::min(currVelocity, prevVelocity);
        double acuteBonus = 0;
        if (std::max(current.AdjustedDeltaTime, last->AdjustedDeltaTime) < 1.25 * std::min(current.AdjustedDeltaTime, last->AdjustedDeltaTime))
        {
            acuteBonus = acute(current.Angle);
            acuteBonus *= 0.08 + 0.92 * (1 - std::min(acuteBonus, std::pow(acute(last->Angle), 3)));
            acuteBonus *= velocity * smootherstep(bpm(current.AdjustedDeltaTime, 2), 300, 400) * smootherstep(currDistance, 0, normalisedDiameter * 2);
        }
        double wideBonus = wide(current.Angle);
        wideBonus *= 0.25 + 0.75 * (1 - std::min(wideBonus, std::pow(wide(last->Angle), 3)));
        double wideCurrVelocity = currDistance / std::pow(current.AdjustedDeltaTime, 1.45);
        double widePrevVelocity = prevDistance / std::pow(last->AdjustedDeltaTime, 1.45);
        if (last->Type == ObjectType::Slider && sliders)
            wideCurrVelocity = std::max(wideCurrVelocity, (last->LazyTravelDistance + current.LazyJumpDistance) / std::pow(current.AdjustedDeltaTime, 1.45));
        wideBonus *= std::min(wideCurrVelocity, widePrevVelocity);
        if (last2)
        {
            const double d = distance(last2->Position, last->Position);
            if (d < 1) wideBonus *= 1 - 0.55 * (1 - d);
        }
        value += std::max(acuteBonus * 2.41, wideBonus * 9.67);
        const double wiggle = velocity * smootherstep(currDistance, normalisedRadius, normalisedDiameter)
            * std::pow(reverseLerp(currDistance, normalisedDiameter * 3, normalisedDiameter), 1.8)
            * smootherstep(current.Angle, 110.0 * Pi / 180.0, 60.0 * Pi / 180.0)
            * smootherstep(prevDistance, normalisedRadius, normalisedDiameter)
            * std::pow(reverseLerp(prevDistance, normalisedDiameter * 3, normalisedDiameter), 1.8)
            * smootherstep(last->Angle, 110.0 * Pi / 180.0, 60.0 * Pi / 180.0);
        value += wiggle * 1.02;
    }
    if (std::max(prevVelocity, currVelocity) != 0)
    {
        if (sliders) currVelocity = currDistance / current.AdjustedDeltaTime;
        const double ratio = smoothstep(std::abs(prevVelocity - currVelocity) / std::max(prevVelocity, currVelocity), 0, 1);
        const double overlap = std::min(normalisedDiameter * 1.25 / std::min(current.AdjustedDeltaTime, last->AdjustedDeltaTime), std::abs(prevVelocity - currVelocity));
        value += overlap * ratio * std::pow(std::min(current.AdjustedDeltaTime, last->AdjustedDeltaTime) / std::max(current.AdjustedDeltaTime, last->AdjustedDeltaTime), 2) * 0.9;
    }
    if (current.Type == ObjectType::Slider && sliders)
    {
        const double bonus = current.TravelDistance / current.TravelTime;
        value += (bonus < 1 ? bonus : std::pow(bonus, 0.75)) * 1.5;
    }
    value *= current.SmallCircleBonus;
    return value / (1 - std::pow(0.03, std::pow(current.AdjustedDeltaTime / 1000.0, 0.65)));
}

inline double agility(const Object &current, const Object *last)
{
    if (current.Type == ObjectType::Spinner) return 0;
    const double cap = normalisedDiameter * 1.2;
    const double d = (last ? last->LazyTravelDistance : 0) + current.LazyJumpDistance;
    double value = std::min(d, cap) / cap * 1000 / current.AdjustedDeltaTime;
    value *= std::pow(current.SmallCircleBonus, 1.5);
    return value / (1 - std::pow(0.2, current.AdjustedDeltaTime / 1000.0));
}

inline double overlap(const Object &a, const Object &b)
{
    return clamp(1 - std::pow(std::max(distance(a.Position, b.Position) - a.Radius, 0.0) / a.Radius, 2), 0, 1);
}
inline double flow(const std::vector<Object> &objects, const Object &current, bool sliders)
{
    const Object *last = previous(objects, current);
    if (!last || current.Type == ObjectType::Spinner || current.Index <= 1 || last->Type == ObjectType::Spinner) return 0;
    const Object *lastLast = previous(objects, current, 1);
    double currDistance = sliders ? current.LazyJumpDistance : current.JumpDistance;
    const double prevDistance = sliders ? last->LazyJumpDistance : last->JumpDistance;
    double currVelocity = currDistance / current.AdjustedDeltaTime;
    if (last->Type == ObjectType::Slider && sliders)
        currVelocity = std::max(currVelocity, (last->LazyTravelDistance + current.LazyJumpDistance) / current.AdjustedDeltaTime);
    const double prevVelocity = prevDistance / last->AdjustedDeltaTime;
    double value = currVelocity * std::sqrt(current.SmallCircleBonus);
    value *= 1 + std::min(0.25, std::pow((std::max(current.AdjustedDeltaTime, last->AdjustedDeltaTime) - std::min(current.AdjustedDeltaTime, last->AdjustedDeltaTime)) / 50, 4));
    if (finiteAngle(current.Angle) && finiteAngle(last->Angle))
    {
        const double angularVelocity = std::sin(std::abs(current.Angle - last->Angle) / 2) * 180.0 / (current.AdjustedDeltaTime * 0.1);
        value *= 0.8 + std::sqrt(angularVelocity / 270.0);
    }
    double overlapWeight = 1;
    if (current.Index > 2 && lastLast)
        overlapWeight = 1 - overlap(current, *last) * overlap(current, *lastLast) * overlap(*last, *lastLast);
    if (finiteAngle(current.Angle)) value += currVelocity * acute(current.Angle) * overlapWeight;
    if (std::max(prevVelocity, currVelocity) != 0)
    {
        if (sliders) currVelocity = currDistance / current.AdjustedDeltaTime;
        const double ratio = smoothstep(std::abs(prevVelocity - currVelocity) / std::max(prevVelocity, currVelocity), 0, 1);
        const double buff = std::min(normalisedDiameter * 1.25 / std::min(current.AdjustedDeltaTime, last->AdjustedDeltaTime), std::abs(prevVelocity - currVelocity));
        value += buff * ratio * overlapWeight * 0.52;
    }
    if (current.Type == ObjectType::Slider && sliders) value += current.TravelDistance / current.TravelTime;
    return std::pow(value, 1.45) * smootherstep(currDistance, 0, normalisedRadius);
}

inline double doubleTap(const std::vector<Object> &objects, const Object &current)
{
    return 1.0 - current.DoubleTapFeasibility(next(objects, current));
}
inline double speedValue(const std::vector<Object> &objects, const Object &current)
{
    if (current.Type == ObjectType::Spinner) return 0;
    double strainTime = current.AdjustedDeltaTime;
    strainTime /= clamp((strainTime / current.HitWindowGreat) / 0.93, 0.92, 1);
    double bonus = 0;
    if (bpm(strainTime) > 200) bonus = 0.75 * std::pow((75.0 - strainTime) / 40.0, 2);
    return (1 + bonus) * 1000 / strainTime / (1 - std::pow(0.3, current.AdjustedDeltaTime / 1000.0)) * doubleTap(objects, current);
}

struct Island
{
    int Delta, Count, Occurrences;
    explicit Island(int delta) : Delta(std::max(delta, 25)), Count(1), Occurrences(1) {}
    void add(int delta) { if (Delta == std::numeric_limits<int>::max()) Delta = std::max(delta, 25); ++Count; }
    bool polarity(const Island &o, double e) const { return Count > 1 && o.Count > 1 && std::abs(Delta-o.Delta)<e && Count%2==o.Count%2; }
    bool equals(const Island &o, double e) const { return std::abs(Delta-o.Delta)<e && Count==o.Count; }
};
inline double rhythm(const std::vector<Object> &objects, const Object &current)
{
    if (current.Type == ObjectType::Spinner) return 0;
    const int historical = std::min(current.Index, 32);
    if (historical < 2) return 1;
    const double epsilon = current.HitWindowGreat * 0.3;
    Island island(std::numeric_limits<int>::max()), priorIsland(std::numeric_limits<int>::max());
    std::vector<Island> islands;
    double complexity=0, startDifficulty=0; bool firstSwitch=false; int rhythmStart=0;
    while (rhythmStart < historical-2 && current.StartTime-previous(objects,current,rhythmStart)->StartTime < 5000) ++rhythmStart;
    const Object *prev=previous(objects,current,rhythmStart), *prevPrev=previous(objects,current,rhythmStart+1);
    if (!prev || !prevPrev) return 1;
    for (int i=rhythmStart; i>0; --i)
    {
        const Object *cur=previous(objects,current,i-1); if (!cur || cur->Type==ObjectType::Spinner) continue;
        const double timeDecay=(5000-(current.StartTime-cur->StartTime))/5000, noteDecay=(double)(historical-i)/historical;
        const double decay=std::min(noteDecay,timeDecay), cd=std::max(cur->DeltaTime,1e-7), pd=std::max(prev->DeltaTime,1e-7);
        const double difference=std::abs(pd-cd), ratio=std::max(pd,cd)/std::min(pd,cd);
        if (island.Delta==std::numeric_limits<int>::max()) island=Island((int)cd);
        double effective=(1+26*std::min(0.5,bell(ratio-std::trunc(ratio)))) * clamp(2-ratio/8,0,1) * clamp((difference-epsilon)/epsilon,0,1);
        if (prev->Type==ObjectType::Slider)
        {
            const double lazy=std::max(cur->MinimumJumpTime,1e-7), real=std::max(cur->LastObjectEndDeltaTime,1e-7);
            const double lr=std::max(lazy,cd)/std::min(lazy,cd), rr=std::max(real,cd)/std::min(real,cd);
            effective=std::min(effective,std::min(1+26*std::min(0.5,bell(lr-std::trunc(lr))),1+26*std::min(0.5,bell(rr-std::trunc(rr)))));
        }
        if (difference<epsilon) island.add((int)cd);
        if (firstSwitch && difference>epsilon)
        {
            if (cur->Type==ObjectType::Slider) effective*=0.5;
            if (island.polarity(priorIsland,epsilon)) effective*=0.5;
            if (std::max(prevPrev->DeltaTime,1e-7)>pd+epsilon && pd>cd+epsilon) effective*=0.125;
            if (priorIsland.Count==island.Count) effective*=0.5;
            if (pd>cd+epsilon) effective*=0.65;
            bool found=false;
            for (size_t j=0;j<islands.size();++j) if (islands[j].equals(island,epsilon))
            {
                if (priorIsland.equals(island,epsilon)) ++islands[j].Occurrences;
                const double power=logistic(island.Delta,58.33,0.24,2.75);
                effective*=std::min(3.0/islands[j].Occurrences,std::pow(1.0/islands[j].Occurrences,power)); found=true; break;
            }
            if (!found && island.Count>0) islands.push_back(island);
            effective*=1-prev->DoubleTapFeasibility(cur)*0.75;
            complexity+=(island.Count>1?std::sqrt(effective*startDifficulty):0.7)*decay;
            startDifficulty=effective; if (pd+epsilon<cd) firstSwitch=false; priorIsland=island; island=Island((int)cd);
        }
        else if (!firstSwitch && pd>cd+epsilon)
        {
            firstSwitch=true; if (cur->Type==ObjectType::Slider) effective*=0.6; if (prev->Type==ObjectType::Slider) effective*=0.6;
            startDifficulty=effective; island=Island((int)cd);
        }
        prevPrev=prev; prev=cur;
    }
    complexity*=reverseLerp(island.Count,22,3);
    return std::sqrt(4+complexity*0.95)/2;
}

inline double timeNerf(double dt) { return clamp(2-dt/1500.0,0,1); }
inline double constantAngle(const std::vector<Object> &objects, const Object &current)
{
    double count=0, gap=0; int index=0; const Object *p0=&current,*p1=NULL,*p2=NULL;
    while (gap<2000)
    {
        const Object *loop=previous(objects,current,index); if (!loop) break;
        const double interval=1-reverseLerp(loop->AdjustedDeltaTime,200,2000);
        if (finiteAngle(loop->Angle)&&finiteAngle(current.Angle))
        {
            double diff=std::abs(current.Angle-loop->Angle), alternating=Pi;
            if (finiteAngle(p0->Angle)&&p1&&p2&&finiteAngle(p1->Angle)&&finiteAngle(p2->Angle))
            {
                alternating=std::abs(p1->Angle-loop->Angle)+std::abs(p2->Angle-p0->Angle);
                double weight=reverseLerp(std::min(loop->Angle,p0->Angle)*180/Pi,20,5)*reverseLerp(std::max(loop->Angle,p0->Angle)*180/Pi,60,120);
                alternating=lerp(Pi,0.1*alternating,weight);
            }
            const double stack=smootherstep(loop->LazyJumpDistance,0,normalisedRadius);
            count+=std::cos(3*std::min(30.0*Pi/180.0,std::min(diff,alternating)*stack))*interval;
        }
        gap=current.StartTime-loop->StartTime; ++index; p2=p1; p1=p0; p0=loop;
    }
    return clamp(2/count,0.2,1);
}
inline double reading(const std::vector<Object> &objects, const Object &current, bool hidden)
{
    if (current.Type==ObjectType::Spinner || current.Index==0) return 0;
    const double velocity=std::max(1.0,current.LazyJumpDistance/current.AdjustedDeltaTime), angle=constantAngle(objects,current);
    double visible=0;
    for (const Object *p=next(objects,current); p; p=next(objects,*p))
    {
        if (p->StartTime-current.StartTime>3000 || current.StartTime<p->StartTime-p->Preempt) break;
        visible+=p->OpacityAt(current.RawStartTime,false)*timeNerf(p->StartTime-current.StartTime);
    }
    double past=0;
    for (int i=0;i<current.Index;++i)
    {
        const Object *p=previous(objects,current,i); if (!p || current.StartTime-p->StartTime>3000 || p->StartTime<current.StartTime-current.Preempt) break;
        past+=current.OpacityAt(p->RawStartTime,false)*smootherstep(p->LazyJumpDistance,15,150)*timeNerf(current.StartTime-p->StartTime);
    }
    double future=std::sqrt(visible); const Object *n=next(objects,current); if(n) future*=smootherstep(n->LazyJumpDistance,15,150);
    double density=std::max(0.0,std::pow(past+future,1.7)*0.4*angle*velocity-2.5); density=std::pow(density,0.45)*2.4;
    const double preempt=std::pow((500-current.Preempt+std::abs(current.Preempt-500))/2,2.5)/140000*angle*velocity;
    double hiddenValue=0;
    if(hidden)
    {
        hiddenValue=std::pow((std::pow(current.Preempt,2.2)*0.01+std::pow(visible+past,3.3)*3)*angle*velocity*0.01,0.4)*0.28;
        const Object *p=previous(objects,current);
        if(p && current.LazyJumpDistance==0 && current.OpacityAt(p->RawStartTime,true)==0 && p->StartTime>current.StartTime-current.Preempt)
            hiddenValue+=0.28*2500/std::pow(current.AdjustedDeltaTime,1.5);
    }
    return norm(1.5,preempt,hiddenValue,density)/(1-std::pow(0.8,current.AdjustedDeltaTime/1000.0));
}

inline double flashlight(const std::vector<Object> &objects, const Object &current, bool hidden)
{
    if(current.Type==ObjectType::Spinner) return 0;
    const double scaling=52.0/current.Radius; double small=1,cumulative=0,value=0,repeats=0; const Object *last=&current;
    for(int i=0;i<std::min(current.Index,10);++i)
    {
            const Object *p=previous(objects,current,i); if(!p) break; cumulative+=last->AdjustedDeltaTime;
        if(p->Type!=ObjectType::Spinner)
        {
            const double jump=distance(current.Position,p->BaseEndPosition); if(i==0) small=std::min(1.0,jump/75.0);
            const double stack=std::min(1.0,(p->LazyJumpDistance/scaling)/25.0);
            const double opacity=1+0.4*(1-current.OpacityAt(p->RawStartTime,hidden));
            value+=stack*opacity*scaling*jump/cumulative;
            if(finiteAngle(p->Angle)&&finiteAngle(current.Angle)&&std::abs(p->Angle-current.Angle)<0.02) repeats+=std::max(1.0-0.1*i,0.0);
        }
        last=p;
    }
    value=std::pow(small*value,2); if(hidden)value*=1.2; value*=0.2+0.8/(repeats+1);
    if(current.Type==ObjectType::Slider)
    {
        const double pixel=current.LazyTravelDistance/scaling;
        double bonus=std::sqrt(std::max(0.0,pixel/current.TravelTime-0.5))*pixel;
        if(current.RepeatCount>0) bonus/=(current.RepeatCount+1);
        value+=bonus*1.3;
    }
    return value;
}

inline double sumSorted(std::vector<double> values, double scale, double exponent, double &weightSum)
{
    std::sort(values.begin(),values.end(),std::greater<double>()); double sum=0; int index=0; weightSum=0;
    for(size_t i=0;i<values.size();++i) if(values[i]>0)
    {
        const double weight=(1+scale/(1+index))/(std::pow((double)index,exponent)+1+scale/(1+index));
        weightSum+=weight; sum+=values[i]*weight; ++index;
    }
    return sum;
}
inline double weightedCount(const std::vector<double>& values,double consistent,double midpoint=0.88,double multiplier=10)
{
    if(consistent==0) return 0;
    double result=0;
    for(size_t i=0;i<values.size();++i) result+=logistic(values[i]/consistent,midpoint,multiplier,1.1);
    return result;
}

struct Peak { double value,length; Peak(double v,double l):value(v),length(std::round(l)){} };
class AimSkill
{
public:
    AimSkill(const std::vector<Object>& o,const Mods&m,bool sliders):objects(o),mods(m),includeSliders(sliders),current(0),begin(0),end(0),peak(0),storedLength(0){}
    void process(const Object&o)
    {
        double strain;
        if(o.Index==0){begin=o.StartTime;end=begin+400;strain=strainAt(o);peak=strain;values.push_back(strain);return;}
        while(o.StartTime>end)
        {
            save(end-begin);
            begin=end;
            const Object*p=previous(objects,o);
            const double initial=current*std::pow(0.2,(begin-(p?p->StartTime:o.StartTime-o.DeltaTime))/1000.0);
            if(!queue.empty())
            {
                const std::pair<double,double> queued=queue.front();
                queue.erase(queue.begin());
                end=queued.second+400;
                peak=std::max(initial,queued.first);
            }
            else
            {
                end=begin+400;
                peak=initial;
            }
        }
        strain=strainAt(o);
        if(strain>peak){queue.clear();save(o.StartTime-begin);begin=o.StartTime;end=begin+400;peak=strain;}
        else {while(!queue.empty()&&queue.back().first<strain)queue.pop_back();queue.push_back(std::make_pair(strain,o.StartTime));}
        values.push_back(strain);
    }
    double difficulty() const
    {
        std::vector<Peak> p=peaks;p.push_back(Peak(peak,end-begin));
        std::sort(p.begin(),p.end(),[](const Peak&a,const Peak&b){return a.value>b.value;});
        std::vector<Peak> reduced; double time=0; size_t skip=0;
        while(skip<p.size()&&time<4000){Peak s=p[skip++];for(double added=0;added<s.length;added+=20){double scale=std::log10(lerp(1,10,clamp((time+added)/4000,0,1)));reduced.push_back(Peak(s.value*lerp(0.727,1,scale),std::min(20.0,s.length-added)));}time+=s.length;}
        for(;skip<p.size();++skip) reduced.push_back(p[skip]);
        std::sort(reduced.begin(),reduced.end(),[](const Peak&a,const Peak&b){return a.value>b.value;});
        double result=0,t=0;for(size_t i=0;i<reduced.size();++i){double e=t+reduced[i].length/400;result+=reduced[i].value*(std::pow(0.9,t)-std::pow(0.9,e));t=e;}return result/0.1;
    }
    double count(double difficulty)const{return values.empty()?0:(difficulty*0.1==0?values.size():weightedCount(values,difficulty*0.1));}
    double sliderCount(double difficulty)const{return sliderValues.empty()?0:weightedCount(sliderValues,difficulty*0.1);}
    double difficultSliders()const{if(sliderValues.empty())return 0;double mx=*std::max_element(sliderValues.begin(),sliderValues.end());return weightedCount(sliderValues,mx,0.5,12);}
    const std::vector<double>& strains()const{return values;}
private:
    const std::vector<Object>&objects;const Mods&mods;bool includeSliders;double current,begin,end,peak,storedLength;std::vector<double>values,sliderValues;std::vector<Peak>peaks;std::vector<std::pair<double,double> >queue;
    void save(double length)
    {
        if(length<=0) return;
        Peak saved(peak,length);
        std::vector<Peak>::iterator position=std::lower_bound(peaks.begin(),peaks.end(),saved,[](const Peak&a,const Peak&b){return a.value>b.value;});
        peaks.insert(position,saved);
        storedLength+=saved.length;
        const double maxStoredLength=11.0/(1.0-0.9)*400.0;
        while(storedLength>maxStoredLength&&!peaks.empty())
        {
            storedLength-=peaks.back().length;
            peaks.pop_back();
        }
    }
    double strainAt(const Object&o)
    {
        const double decay=std::pow(0.2,o.AdjustedDeltaTime/1000);current*=decay;
        if(!mods.Autopilot)
        {
            const Object*l=previous(objects,o);double s=snap(objects,o,includeSliders)*70.9,a=agility(o,l)*2.35,f=flow(objects,o,includeSliders)*242;
            double combined=norm(1.2,s,a),ratio=f/combined,pSnap=ratio==0?0:(std::isnan(ratio)?1:1/(1+std::exp(-7.27*std::log(ratio))));
            if(mods.TouchDevice){s=std::pow(s,0.89);combined=norm(1.2,s,a);}if(mods.Relax){combined*=0.75;f*=0.6;}
            double adjusted=(combined*pSnap+f*(1-pSnap))*1.12*(0.985+std::pow(std::max(0.0,o.OverallDifficulty),2)/4000);
            current+=adjusted*(1-decay);
        }
        if(o.Type==ObjectType::Slider) sliderValues.push_back(current);
        return current;
    }
};

inline Attributes calculateAttributes(const AimSkill& aim,const AimSkill& aimNoSliders,const std::vector<double>& speedValues,const std::vector<double>& speedSliders,const std::vector<double>& readingValues,double readingReducedCount,const std::vector<double>& flashPeaks,double flashPeak,const Mods& mods,size_t processedObjects,int hitCircles,int sliders,int spinners)
{
    const double aimValue=aim.difficulty(),aimNoValue=aimNoSliders.difficulty();double speedWeight=0,readingWeight=0;
    const double speedValueTotal=sumSorted(speedValues,20,0.9,speedWeight);
    std::vector<double>readingTransformed=readingValues;for(int i=0;i<std::min((int)readingTransformed.size(),(int)readingReducedCount);++i){double scale=std::log10(lerp(1,10,clamp(i/readingReducedCount,0,1)));readingTransformed[i]*=scale;}
    const double readingValueTotal=sumSorted(readingTransformed,1,0.9,readingWeight);
    double flashValue=0;if(mods.Flashlight){for(size_t i=0;i<flashPeaks.size();++i)flashValue+=flashPeaks[i];flashValue+=flashPeak;double total=processedObjects+1;flashValue*=0.7+0.1*std::min(1.0,total/200)+(total>200?0.2*std::min(1.0,(total-200)/200):0);}
    Attributes a;const double aimRating=std::pow(aimValue,0.63)*0.02275,aimNoRating=std::pow(aimNoValue,0.63)*0.02275;
    a.AimDifficulty=aimRating;a.SpeedDifficulty=std::sqrt(speedValueTotal)*0.0675;a.ReadingDifficulty=std::sqrt(readingValueTotal)*0.0675;a.FlashlightDifficulty=std::sqrt(flashValue)*0.0675;
    a.SliderFactor=aimValue>0?aimNoRating/aimRating:1;a.AimDifficultSliderCount=aim.difficultSliders();a.AimDifficultStrainCount=aim.count(aimValue);a.SpeedDifficultStrainCount=weightedCount(speedValues,speedWeight?speedValueTotal/speedWeight:0);a.ReadingDifficultNoteCount=weightedCount(readingValues,readingWeight?readingValueTotal/readingWeight:0,1.15,5);
    double maxSpeed=speedValues.empty()?0:*std::max_element(speedValues.begin(),speedValues.end());a.SpeedNoteCount=weightedCount(speedValues,maxSpeed,0.5,12);a.AimTopWeightedSliderFactor=aimNoSliders.sliderCount(aimNoValue)/std::max(1.0,aimNoSliders.count(aimNoValue)-aimNoSliders.sliderCount(aimNoValue));a.SpeedTopWeightedSliderFactor=weightedCount(speedSliders,speedWeight?speedValueTotal/speedWeight:0)/std::max(1.0,a.SpeedDifficultStrainCount-weightedCount(speedSliders,speedWeight?speedValueTotal/speedWeight:0));
    double aimPerformance=4*std::pow(aimRating,3),speedPerformance=4*std::pow(a.SpeedDifficulty,3),readingPerformance=4*std::pow(a.ReadingDifficulty,3),flashPerformance=25*std::pow(a.FlashlightDifficulty,2);double cognition=readingPerformance<=0?flashPerformance:(flashPerformance<=0?readingPerformance:norm(performanceNormExponent,readingPerformance,flashPerformance*clamp(flashPerformance/readingPerformance,0.25,1)));a.StarRating=std::cbrt(norm(performanceNormExponent,aimPerformance,speedPerformance,cognition)*1.12);
    a.HitCircleCount=hitCircles;a.SliderCount=sliders;a.SpinnerCount=spinners;a.MaxCombo=processedObjects+1;
    return a;
}

inline Result calculateInternal(const std::vector<Object>& objects,const Mods& mods,const std::atomic<bool>& cancelled,std::vector<Attributes>* liveAttributes)
{
    Result result;if(objects.empty())return result;
    AimSkill aim(objects,mods,true),aimNoSliders(objects,mods,false);
    std::vector<double> speedValues, speedSliders, readingValues, flashlightValues;double speedStrain=0,readingStrain=0,flashStrain=0;
    double readingReducedCount=0,readingEnd=0;double flashPeak=0,flashSectionEnd=0;std::vector<double>flashPeaks;
    int hitCircles=1,sliders=0,spinners=0;
    if(liveAttributes) liveAttributes->reserve(objects.size());
    for(size_t i=0;i<objects.size();++i)
    {
        if(cancelled.load()){if(liveAttributes)liveAttributes->clear();return Result();}
        const Object&o=objects[i];aim.process(o);aimNoSliders.process(o);
        double sd=0;if(!mods.Relax){double decay=std::pow(0.3,o.AdjustedDeltaTime/1000);speedStrain*=decay;double d=speedValue(objects,o);if(mods.Autopilot)d*=0.5;speedStrain+=d*(1-decay)*1.16;sd=speedStrain*rhythm(objects,o);}speedValues.push_back(sd);if(o.Type==ObjectType::Slider)speedSliders.push_back(sd);
        double rdDecay=std::pow(0.8,o.DeltaTime/1000);readingStrain*=rdDecay;double rd=reading(objects,o,mods.Hidden);if(mods.TouchDevice)rd=std::pow(rd,0.89);if(mods.Relax)rd*=0.4;if(mods.Autopilot)rd*=0.1;rd*=0.825+std::pow(std::max(0.0,o.OverallDifficulty),2.2)/1125;readingStrain+=rd*(1-rdDecay)*2.5;readingValues.push_back(readingStrain);
        if(i==0) readingEnd=o.StartTime+60000;
        if(o.StartTime<=readingEnd) ++readingReducedCount;
        if(mods.Flashlight){if(o.Index==0)flashSectionEnd=std::ceil(o.StartTime/400)*400;while(o.StartTime>flashSectionEnd){flashPeaks.push_back(flashPeak);const Object*p=previous(objects,o);flashPeak=flashStrain*std::pow(0.15,(flashSectionEnd-(p?p->StartTime:o.StartTime-o.DeltaTime))/1000);flashSectionEnd+=400;}flashStrain*=std::pow(0.15,o.DeltaTime/1000);double fd=flashlight(objects,o,mods.Hidden);if(mods.TouchDevice)fd=std::pow(fd,0.9);if(mods.Relax)fd*=0.7;if(mods.Autopilot)fd*=0.4;fd*=0.985+std::pow(std::max(0.0,o.OverallDifficulty),2)/4000;flashStrain+=fd*0.058;flashPeak=std::max(flashPeak,flashStrain);flashlightValues.push_back(flashStrain);}
        if(o.Type==ObjectType::Circle)++hitCircles;else if(o.Type==ObjectType::Slider)++sliders;else ++spinners;
        if(liveAttributes) liveAttributes->push_back(calculateAttributes(aim,aimNoSliders,speedValues,speedSliders,readingValues,readingReducedCount,flashPeaks,flashPeak,mods,i+1,hitCircles,sliders,spinners));
    }
    result.attributes=liveAttributes?liveAttributes->back():calculateAttributes(aim,aimNoSliders,speedValues,speedSliders,readingValues,readingReducedCount,flashPeaks,flashPeak,mods,objects.size(),hitCircles,sliders,spinners);
    result.aimStrains=aim.strains();result.speedStrains=speedValues;return result;
}

inline Result calculate(const std::vector<Object>& objects,const Mods& mods,const std::atomic<bool>& cancelled)
{
    return calculateInternal(objects,mods,cancelled,NULL);
}

inline std::vector<Attributes> calculateLive(const std::vector<Object>& objects,const Mods& mods,const std::atomic<bool>& cancelled)
{
    std::vector<Attributes> result;
    calculateInternal(objects,mods,cancelled,&result);
    return result;
}

}
}
#endif

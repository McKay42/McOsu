//================ Copyright (c) 2015, PG & Jeffrey Han (opsu!), All rights reserved. =================//
//
// Purpose:		slider. curve classes have been taken from opsu!, albeit heavily modified
//
// $NoKeywords: $slider
//=====================================================================================================//

#ifndef OSUSLIDER_H
#define OSUSLIDER_H

#include "OsuHitObject.h"

class OsuSliderCurve;
class OsuSliderCurveEqualDistanceMulti;

class Shader;
class VertexArrayObject;

class OsuSlider : public OsuHitObject
{
public:
	OsuSlider(char type, int repeat, float pixelLength, std::vector<Vector2> points, std::vector<float> ticks, float sliderTime, float sliderTimeWithoutRepeats, long time, int sampleType, int comboNumber, int colorCounter, OsuBeatmap *beatmap);
	virtual ~OsuSlider();

	virtual void draw(Graphics *g);
	virtual void draw2(Graphics *g);
	virtual void update(long curPos);

	void updateStackPosition(float stackOffset);

	Vector2 getRawPosAt(long pos);
	Vector2 getOriginalRawPosAt(long pos);
	inline Vector2 getAutoCursorPos(long curPos) {return m_vCurPoint;}

	virtual void onClickEvent(Vector2 cursorPos, std::vector<OsuBeatmap::CLICK> &clicks);
	virtual void onReset(long curPos);

	inline int getRepeat() const {return m_iRepeat;}
	inline std::vector<Vector2> getRawPoints() const {return m_points;}
	inline float getPixelLength() const {return m_fPixelLength;}

private:
	static ConVar *m_osu_playfield_mirror_horizontal_ref;
	static ConVar *m_osu_playfield_mirror_vertical_ref;
	static ConVar *m_osu_playfield_rotation_ref;

	void drawStartCircle(Graphics *g, float alpha);
	void drawEndCircle(Graphics *g, float alpha, float sliderSnake = 1.0f);

	void updateAnimations(long curPos);

	void onHit(OsuBeatmap::HIT result, long delta, bool startOrEnd, float targetDelta = 0.0f, float targetAngle = 0.0f);
	void onRepeatHit(bool successful, bool sliderend);
	void onTickHit(bool successful, int tickIndex);
	void onSliderBreak();

	float getT(long pos, bool raw);

	enum SLIDERTYPE
	{
		SLIDER_CATMULL = 'C',
		SLIDER_BEZIER = 'B',
		SLIDER_LINEAR = 'L',
		SLIDER_PASSTHROUGH = 'P'
	};

	OsuSliderCurve *m_curve;

	char m_cType;
	int m_iRepeat;
	float m_fPixelLength;
	std::vector<Vector2> m_points;
	float m_fSliderTime;
	float m_fSliderTimeWithoutRepeats;

	struct SLIDERTICK
	{
		float percent;
		bool finished;
	};
	std::vector<SLIDERTICK> m_ticks; // ticks (drawing)

	struct SLIDERCLICK
	{
		long time;
		bool finished;
		bool successful;
		bool sliderend;
		int type;
		int tickIndex;
	};
	std::vector<SLIDERCLICK> m_clicks; // repeats (type 0) + ticks (type 1)

	float m_fSlidePercent; // 0.0f - 1.0f - 0.0f - 1.0f - etc.
	float m_fActualSlidePercent; // 0.0f - 1.0f
	float m_fHiddenAlpha;
	float m_fHiddenSlowFadeAlpha;

	Vector2 m_vCurPoint;
	Vector2 m_vCurPointRaw;

	OsuBeatmap::HIT m_startResult;
	OsuBeatmap::HIT m_endResult;
	bool m_bStartFinished;
	float m_fStartHitAnimation;
	bool m_bEndFinished;
	float m_fEndHitAnimation;
	float m_fEndSliderBodyFadeAnimation;
	long m_iLastClickHeld;
	bool m_bCursorLeft;
	bool m_bCursorInside;
	bool m_bHeldTillEnd;
	float m_fFollowCircleTickAnimationScale;
	float m_fFollowCircleAnimationScale;
	float m_fFollowCircleAnimationAlpha;

	int m_iReverseArrowPos;
	int m_iCurRepeat;
	bool m_bInReverse;
	bool m_bHideNumberAfterFirstRepeatHit;

	//TEMP:
	float m_fSliderBreakRapeTime;
};



//**********************//
//	 Curve Base Class	//
//**********************//

class OsuSliderCurve
{
public:
	static Shader *BLEND_SHADER;

public:
	OsuSliderCurve(OsuSlider *parent, OsuBeatmap *beatmap);
	virtual ~OsuSliderCurve();

	void draw(Graphics *g, Color color, float alpha);
	void draw(Graphics *g, Color color, float alpha, float t, float t2 = 0.0f);

	virtual void updateStackPosition(float stackMulStackOffset);

	virtual Vector2 pointAt(float t) = 0;
	virtual Vector2 originalPointAt(float t) = 0;

	inline float getStartAngle() const {return m_fStartAngle;}
	inline float getEndAngle() const {return m_fEndAngle;}

protected:
	static float CURVE_POINTS_SEPERATION;
	static int UNIT_CONE_DIVIDES;
	static std::vector<float> UNIT_CONE;
	static VertexArrayObject *MASTER_CIRCLE_VAO;
	static float MASTER_CIRCLE_VAO_RADIUS;

	void drawFillCircle(Graphics *g, Vector2 center);
	void drawFillSliderBody(Graphics *g, int drawUpTo);
	void drawFillSliderBody2(Graphics *g, int drawUpTo, int drawFrom = 0);

	OsuBeatmap *m_beatmap;
	OsuSlider *m_slider;
	std::vector<Vector2> m_points;

	// these must be explicitely set in one of the subclasses
	std::vector<Vector2> m_curvePoints;
	std::vector<Vector2> m_originalCurvePoints;
	float m_fStartAngle;
	float m_fEndAngle;

private:
	// rendering optimization
	float m_fBoundingBoxMinX;
	float m_fBoundingBoxMaxX;
	float m_fBoundingBoxMinY;
	float m_fBoundingBoxMaxY;
};



//******************************************//
//	 Curve Type Base Class & Curve Types	//
//******************************************//

class OsuSliderCurveType
{
public:
	OsuSliderCurveType();
	virtual ~OsuSliderCurveType() {;}

	void init(float approxlength);

	virtual Vector2 pointAt(float t) = 0;

	inline std::vector<Vector2> getCurvePoints() const {return m_points;}
	inline std::vector<float> getCurveDistances() const {return m_curveDistances;}
	inline float getTotalDistance() const {return m_fTotalDistance;}
	inline int getCurvesCount() const {return m_iNCurve;}

private:
	int m_iNCurve;
	float m_fTotalDistance;
	std::vector<Vector2> m_points;
	std::vector<float> m_curveDistances;
};


class OsuSliderCurveTypeBezier2 : public OsuSliderCurveType
{
public:
	OsuSliderCurveTypeBezier2(std::vector<Vector2> points);

	Vector2 pointAt(float t);

private:
	static long binomialCoefficient(int n, int k);
	static double bernstein(int i, int n, float t);

	std::vector<Vector2> m_points;
};


class OsuSliderCurveTypeCentripetalCatmullRom : public OsuSliderCurveType
{
public:
	OsuSliderCurveTypeCentripetalCatmullRom(std::vector<Vector2> points);

	Vector2 pointAt(float t);

private:
	float m_time[4];

	std::vector<Vector2> m_points;
};



//******************//
//	 Curve Classes	//
//******************//

class OsuSliderCurveEqualDistanceMulti : public OsuSliderCurve
{
public:
	OsuSliderCurveEqualDistanceMulti(OsuSlider *parent, OsuBeatmap *beatmap);
	virtual ~OsuSliderCurveEqualDistanceMulti() {;}

	void init(std::vector<OsuSliderCurveType*> curvesList);

	Vector2 pointAt(float t);
	Vector2 originalPointAt(float t);

private:
	int m_iNCurve;
};


class OsuSliderCurveLinearBezier : public OsuSliderCurveEqualDistanceMulti
{
public:
	OsuSliderCurveLinearBezier(OsuSlider *parent, bool line, OsuBeatmap *beatmap);
};

class OsuSliderCurveCatmull : public OsuSliderCurveEqualDistanceMulti
{
public:
	OsuSliderCurveCatmull(OsuSlider *parent, OsuBeatmap *beatmap);
};

class OsuSliderCurveCircumscribedCircle : public OsuSliderCurve
{
public:
	OsuSliderCurveCircumscribedCircle(OsuSlider *parent, OsuBeatmap *beatmap);

	Vector2 pointAt(float t);
	Vector2 originalPointAt(float t);

	void updateStackPosition(float stackMulStackOffset); // must also override this, due to the custom pointAt() function!

private:
	Vector2 intersect(Vector2 a, Vector2 ta, Vector2 b, Vector2 tb);
	bool isIn(float a, float b, float c);

	Vector2 m_vCircleCenter;
	Vector2 m_vOriginalCircleCenter;
	float m_fRadius;
	float m_fCalculationStartAngle;
	float m_fCalculationEndAngle;
};

#endif

//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		score handler (calculation, storage, hud)
//
// $NoKeywords: $osu
//===============================================================================//

#ifndef OSUSCORE_H
#define OSUSCORE_H

#include "cbase.h"

class Osu;
class OsuBeatmap;

class OsuScore
{
public:
	enum HIT
	{
		HIT_NULL,
		HIT_MISS,
		HIT_50,
		HIT_100,
		HIT_300,
		/*
		HIT_100K,
		HIT_300K,
		HIT_300G,
		*/
		HIT_SLIDER10,
		HIT_SLIDER30
	};

	enum GRADE
	{
		GRADE_XH,
		GRADE_SH,
		GRADE_X,
		GRADE_S,
		GRADE_A,
		GRADE_B,
		GRADE_C,
		GRADE_D,
		GRADE_F,
		GRADE_N
	};

public:
	OsuScore(Osu *osu);

	void reset(); // only OsuBeatmap may call this function!

	void addHitResult(OsuBeatmap *beatmap, HIT hit, long delta, bool ignoreOnHitErrorBar, bool hitErrorBarOnly, bool ignoreCombo, bool ignoreScore); // only OsuBeatmap may call this function!
	void addSliderBreak(); // only OsuBeatmap may call this function!
	void addPoints(int points);

	inline GRADE getGrade() {return m_grade;}
	inline int getScore() {return m_iScore;}
	inline int getCombo() {return m_iCombo;}
	inline int getComboMax() {return m_iComboMax;}
	inline float getAccuracy() {return m_fAccuracy;}
	inline float getUnstableRate() {return m_fUnstableRate;}
	inline int getNumMisses() {return m_iNumMisses;}
	inline int getNumSliderBreaks() {return m_iNumSliderBreaks;}
	inline int getNum50s() {return m_iNum50s;}
	inline int getNum100s() {return m_iNum100s;}
	inline int getNum100ks() {return m_iNum100ks;}
	inline int getNum300s() {return m_iNum300s;}
	inline int getNum300gs() {return m_iNum300gs;}

private:
	Osu *m_osu;

	std::vector<HIT> m_hitresults;
	std::vector<int> m_hitdeltas;
	GRADE m_grade;
	int m_iScore;
	int m_iCombo;
	int m_iComboMax;
	float m_fAccuracy;
	float m_fUnstableRate;

	int m_iNumMisses;
	int m_iNumSliderBreaks;
	int m_iNum50s;
	int m_iNum100s;
	int m_iNum100ks;
	int m_iNum300s;
	int m_iNum300gs;
};

#endif

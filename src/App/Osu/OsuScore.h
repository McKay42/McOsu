//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		score handler (calculation, storage, hud)
//
// $NoKeywords: $osu
//===============================================================================//

#ifndef OSUSCORE_H
#define OSUSCORE_H

#include "cbase.h"

class ConVar;

class Osu;
class OsuBeatmap;

class OsuScore
{
public:
	enum class HIT
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

	enum class GRADE
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
	void addPoints(int points, bool isSpinner);
	void setDead(bool dead);

	void setStarsTomTotal(float starsTomTotal) {m_fStarsTomTotal = starsTomTotal;}
	void setStarsTomAim(float starsTomAim) {m_fStarsTomAim = starsTomAim;}
	void setStarsTomSpeed(float starsTomSpeed) {m_fStarsTomSpeed = starsTomSpeed;}
	void setPPv2(float ppv2) {m_fPPv2 = ppv2;}

	inline float getStarsTomTotal() const {return m_fStarsTomTotal;}
	inline float getStarsTomAim() const {return m_fStarsTomAim;}
	inline float getStarsTomSpeed() const {return m_fStarsTomSpeed;}
	inline float getPPv2() const {return m_fPPv2;}

	unsigned long long getScore();
	inline GRADE getGrade() const {return m_grade;}
	inline int getCombo() const {return m_iCombo;}
	inline int getComboMax() const {return m_iComboMax;}
	inline float getAccuracy() const {return m_fAccuracy;}
	inline float getUnstableRate() const {return m_fUnstableRate;}
	inline float getHitErrorAvgMin() const {return m_fHitErrorAvgMin;}
	inline float getHitErrorAvgMax() const {return m_fHitErrorAvgMax;}
	inline int getNumMisses() const {return m_iNumMisses;}
	inline int getNumSliderBreaks() const {return m_iNumSliderBreaks;}
	inline int getNum50s() const {return m_iNum50s;}
	inline int getNum100s() const {return m_iNum100s;}
	inline int getNum100ks() const {return m_iNum100ks;}
	inline int getNum300s() const {return m_iNum300s;}
	inline int getNum300gs() const {return m_iNum300gs;}

	inline bool isDead() const {return m_bDead;}
	inline bool hasDied() const {return m_bDied;}

private:
	static ConVar *m_osu_draw_statistics_pp;

	void onScoreChange();

	Osu *m_osu;

	std::vector<HIT> m_hitresults;
	std::vector<int> m_hitdeltas;

	GRADE m_grade;

	float m_fStarsTomTotal;
	float m_fStarsTomAim;
	float m_fStarsTomSpeed;
	float m_fPPv2;

	unsigned long long m_iScoreV1;
	unsigned long long m_iScoreV2;
	unsigned long long m_iScoreV2ComboPortion;
	unsigned long long m_iBonusPoints;
	int m_iCombo;
	int m_iComboMax;
	int m_iComboFull;
	float m_fAccuracy;
	float m_fHitErrorAvgMin;
	float m_fHitErrorAvgMax;
	float m_fUnstableRate;

	int m_iNumMisses;
	int m_iNumSliderBreaks;
	int m_iNum50s;
	int m_iNum100s;
	int m_iNum100ks;
	int m_iNum300s;
	int m_iNum300gs;

	bool m_bDead;
	bool m_bDied;
};

#endif

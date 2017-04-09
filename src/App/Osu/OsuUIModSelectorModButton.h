//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		mod image buttons (EZ, HD, HR, HT, DT, etc.)
//
// $NoKeywords: $osumsmb
//===============================================================================//

#ifndef OSUUIMODSELECTORMODBUTTON_H
#define OSUUIMODSELECTORMODBUTTON_H

#include "CBaseUIImageButton.h"

class Osu;
class OsuSkinImage;
class OsuModSelector;

class OsuUIModSelectorModButton : public CBaseUIButton
{
public:
	OsuUIModSelectorModButton(Osu *osu, OsuModSelector* osuModSelector, float xPos, float yPos, float xSize, float ySize, UString name);

	virtual void draw(Graphics *g);
	virtual void update();

	void click() {onMouseDownInside();}

	void resetState();

	void setState(unsigned int state, UString modName, UString tooltipText, OsuSkinImage *img);
	void setBaseScale(float xScale, float yScale);
	void setAvailable(bool available) {m_bAvailable = available;}

	UString getActiveModName();
	inline int getState() const {return m_iState;}
	inline bool isOn() const {return m_bOn;}

private:
	virtual void onMouseDownInside();

	void setOn(bool on);
	void setState(int state);

	Osu *m_osu;
	OsuModSelector *m_osuModSelector;

	bool m_bAvailable;
	bool m_bOn;
	int m_iState;
	float m_fEnabledScaleMultiplier;
	float m_fEnabledRotationDeg;
	Vector2 m_vBaseScale;

	struct STATE
	{
		UString modName;
		std::vector<UString> tooltipTextLines;
		OsuSkinImage *img;
	};
	std::vector<STATE> m_states;

	Vector2 m_vScale;
	float m_fRot;
	OsuSkinImage *m_activeImage;
};

#endif

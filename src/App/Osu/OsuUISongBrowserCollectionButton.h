//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		collection button (also used for grouping)
//
// $NoKeywords: $osusbcb
//===============================================================================//

#ifndef OSUUISONGBROWSERCOLLECTIONBUTTON_H
#define OSUUISONGBROWSERCOLLECTIONBUTTON_H

#include "OsuUISongBrowserButton.h"

class OsuUISongBrowserCollectionButton : public OsuUISongBrowserButton
{
public:
	OsuUISongBrowserCollectionButton(Osu *osu, OsuSongBrowser2 *songBrowser, CBaseUIScrollView *view, float xPos, float yPos, float xSize, float ySize, UString name, UString collectionName, std::vector<OsuUISongBrowserButton*> children);
	virtual ~OsuUISongBrowserCollectionButton() {;}

	virtual void draw(Graphics *g);

	virtual void setPreviousButton(OsuUISongBrowserCollectionButton *previousButton);
	virtual OsuUISongBrowserCollectionButton *getPreviousButton();

	virtual OsuBeatmap *getBeatmap() const {return NULL;}
	virtual std::vector<OsuUISongBrowserButton*> getChildren();

	inline UString getCollectionName() {return m_sCollectionName;}

private:
	static OsuUISongBrowserCollectionButton *s_previousButton;

	virtual void onSelected(bool wasSelected);
	virtual void onDeselected();

	UString buildTitleString();

	UString m_sCollectionName;

	float m_fTitleScale;
};

#endif

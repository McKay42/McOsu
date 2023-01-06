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
	OsuUISongBrowserCollectionButton(Osu *osu, OsuSongBrowser2 *songBrowser, CBaseUIScrollView *view, OsuUIContextMenu *contextMenu, float xPos, float yPos, float xSize, float ySize, UString name, UString collectionName, std::vector<OsuUISongBrowserButton*> children);

	virtual void draw(Graphics *g);

	void triggerContextMenu(Vector2 pos);

	const UString &getCollectionName() const {return m_sCollectionName;}

private:
	virtual void onSelected(bool wasSelected, bool wasClicked);
	virtual void onRightMouseUpInside();

	void onContextMenu(UString text, int id = -1);
	void onRenameCollectionConfirmed(UString text, int id = -1);
	void onDeleteCollectionConfirmed(UString text, int id = -1);

	UString buildTitleString();

	UString m_sCollectionName;

	float m_fTitleScale;
};

#endif

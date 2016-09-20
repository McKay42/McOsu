//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		text bar overlay which can eat inputs (also used for key bindings)
//
// $NoKeywords: $osunot
//===============================================================================//

#ifndef OSUNOTIFICATIONOVERLAY_H
#define OSUNOTIFICATIONOVERLAY_H

#include "OsuScreen.h"
#include "KeyboardEvent.h"

class Osu;

class OsuNotificationOverlayKeyListener
{
public:
	virtual ~OsuNotificationOverlayKeyListener() {;}
	virtual void onKey(KeyboardEvent &e) = 0;
};

class OsuNotificationOverlay : public OsuScreen
{
public:
	OsuNotificationOverlay(Osu *osu);
	virtual ~OsuNotificationOverlay(){;}

	void draw(Graphics *g);

	void onKeyDown(KeyboardEvent &e);
	void onKeyUp(KeyboardEvent &e);
	void onChar(KeyboardEvent &e);

	void addNotification(UString text, Color textColor = 0xffffffff, bool waitForKey = false);

	void stopWaitingForKey();

	void addKeyListener(OsuNotificationOverlayKeyListener *keyListener) {m_keyListener = keyListener;}

	bool isVisible();

private:
	struct NOTIFICATION
	{
		UString text;
		Color textColor;

		float time;
		float alpha;
		float backgroundAnim;
		float fallAnim;
	};

	void drawNotificationText(Graphics *g, NOTIFICATION &n);
	void drawNotificationBackground(Graphics *g, NOTIFICATION &n);

	Osu *m_osu;

	NOTIFICATION m_notification1;
	NOTIFICATION m_notification2;

	bool m_bWaitForKey;
	OsuNotificationOverlayKeyListener *m_keyListener;
};

#endif

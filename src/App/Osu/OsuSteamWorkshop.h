//================ Copyright (c) 2019, PG, All rights reserved. =================//
//
// Purpose:		workshop support (subscribed items, upload/download/status)
//
// $NoKeywords: $osusteamws
//===============================================================================//

#ifndef OSUSTEAMWORKSHOP_H
#define OSUSTEAMWORKSHOP_H

#include "cbase.h"

class Osu;

class OsuSteamWorkshopLoader;
class OsuSteamWorkshopUploader;

class OsuSteamWorkshop
{
public:
	enum class SUBSCRIBED_ITEM_TYPE
	{
		SKIN
	};

	enum class SUBSCRIBED_ITEM_STATUS
	{
		DOWNLOADING,
		INSTALLED
	};

	struct SUBSCRIBED_ITEM
	{
		SUBSCRIBED_ITEM_TYPE type;
		SUBSCRIBED_ITEM_STATUS status;
		uint64_t id;
		UString title;
		UString installInfo;
	};

public:
	OsuSteamWorkshop(Osu *osu);
	~OsuSteamWorkshop();

	void refresh(bool async, bool alsoLoadDetailsWhichTakeVeryLongToLoad = true);

	bool isReady() const;
	bool areDetailsLoaded() const;
	bool isUploading() const;
	const std::vector<SUBSCRIBED_ITEM> &getSubscribedItems() const {return m_subscribedItems;}

private:
	friend class OsuSteamWorkshopLoader;
	friend class OsuSteamWorkshopUploader;

	void onUpload();
	void handleUploadError(UString errorMessage);

	Osu *m_osu;
	OsuSteamWorkshopLoader *m_loader;
	OsuSteamWorkshopUploader *m_uploader;

	std::vector<SUBSCRIBED_ITEM> m_subscribedItems;

	ConVar *m_osu_skin_ref;
	ConVar *m_osu_skin_is_from_workshop_ref;
};

#endif

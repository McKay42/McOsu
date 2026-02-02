//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		checks if an update is available from github
//
// $NoKeywords: $osuupdchk
//===============================================================================//

#include "OsuUpdateHandler.h"

#include "Engine.h"
#include "ResourceManager.h"
//#include "NetworkHandler.h"
#include "ConVar.h"
#include "File.h"

//#include "JSON.h"
//#include "miniz.h"

#include "Osu.h"

const char *OsuUpdateHandler::GITHUB_API_RELEASE_URL = "https://api.github.com/repos/McKay42/McOsu/releases";
const char *OsuUpdateHandler::GITHUB_RELEASE_DOWNLOAD_URL = "https://github.com/McKay42/McOsu/releases";
const char *OsuUpdateHandler::TEMP_UPDATE_DOWNLOAD_FILEPATH = "update.zip";

ConVar *OsuUpdateHandler::m_osu_release_stream_ref = NULL;

#ifdef MCENGINE_FEATURE_PTHREADS

void *OsuUpdateHandler::run(void *data)
{
	// not using semaphores/mutexes here because it's not critical

	if (data == NULL) return NULL;

	OsuUpdateHandler *handler = (OsuUpdateHandler*)data;

	if (handler->_m_bKYS) return NULL; // cancellation point

	// check for updates
	handler->_requestUpdate();

	if (handler->_m_bKYS) return NULL; // cancellation point

	// continue if we have one. reset the thread in both cases after we're done
	if (handler->isUpdateAvailable())
	{
		// get the newest release to install
		UString downloadUrl = "";
		float latestVersion = Osu::version->getFloat();
		for (int i=0; i<handler->m_releases.size(); i++)
		{
			if (handler->m_releases[i].os == env->getOS() && handler->m_releases[i].stream == handler->getReleaseStream() && handler->m_releases[i].version > latestVersion)
			{
				latestVersion = handler->m_releases[i].version;
				downloadUrl = handler->m_releases[i].downloadURL;
			}
		}

		// try to download and install the update
		if (handler->_downloadUpdate(downloadUrl))
		{
			if (handler->_m_bKYS) return NULL; // cancellation point

			handler->_installUpdate(TEMP_UPDATE_DOWNLOAD_FILEPATH);
		}

		handler->m_updateThread = 0; // reset

		if (handler->_m_bKYS) return NULL; // cancellation point

		// retry up to 3 times if something went wrong
		if (handler->getStatus() != STATUS::STATUS_SUCCESS_INSTALLATION)
		{
			handler->m_iNumRetries++;
			if (handler->m_iNumRetries < 4)
				handler->checkForUpdates();
		}
	}
	else
		handler->m_updateThread = 0; // reset

	return NULL;
}

#endif

OsuUpdateHandler::OsuUpdateHandler()
{
#ifdef MCENGINE_FEATURE_PTHREADS

	m_updateThread = 0;

#endif

	m_status = Osu::autoUpdater ? STATUS::STATUS_CHECKING_FOR_UPDATE : STATUS::STATUS_UP_TO_DATE;
	m_iNumRetries = 0;
	_m_bKYS = false;

	// convar refs
	if (m_osu_release_stream_ref == NULL)
		m_osu_release_stream_ref = convar->getConVarByName("osu_release_stream");
}

OsuUpdateHandler::~OsuUpdateHandler()
{
#ifdef MCENGINE_FEATURE_PTHREADS

	if (m_updateThread != 0)
		engine->showMessageErrorFatal("Fatal Error", "OsuUpdateHandler was destroyed while the update thread is still running!!!");

#endif
}

void OsuUpdateHandler::stop()
{
#ifdef MCENGINE_FEATURE_PTHREADS

	if (m_updateThread != 0)
	{
		_m_bKYS = true;
		m_updateThread = 0;
	}

#endif
}

void OsuUpdateHandler::wait()
{
#ifdef MCENGINE_FEATURE_PTHREADS

	if (m_updateThread != 0)
	{
		pthread_join(m_updateThread, NULL);
		m_updateThread = 0;
	}

#endif
}

void OsuUpdateHandler::checkForUpdates()
{
#ifdef MCENGINE_FEATURE_PTHREADS

	if (!Osu::autoUpdater || Osu::debug->getBool() || m_updateThread != 0) return;

	int ret = pthread_create(&m_updateThread, NULL, OsuUpdateHandler::run, (void*)this);
	if (ret)
	{
		m_updateThread = 0;
		debugLog("OsuUpdateHandler: Error, pthread_create() returned %i!\n", ret);
		return;
	}

	if (m_iNumRetries > 0)
		debugLog("OsuUpdateHandler::checkForUpdates() retry %i ...\n", m_iNumRetries);

#endif
}

bool OsuUpdateHandler::isUpdateAvailable()
{
	for (int i=0; i<m_releases.size(); i++)
	{
		if (m_releases[i].os == env->getOS() && m_releases[i].stream == getReleaseStream() && m_releases[i].version > Osu::version->getFloat())
			return true;
	}
	return false;
}

void OsuUpdateHandler::_requestUpdate()
{
	debugLog("OsuUpdateHandler::requestUpdate()\n");
	m_status = STATUS::STATUS_CHECKING_FOR_UPDATE;

	if (m_releases.size() > 0 && isUpdateAvailable()) return; // don't need to get twice

	m_releases.clear();
	std::vector<GITHUB_RELEASE_BUILD> asyncReleases;

	UString gitReleases/* = engine->getNetworkHandler()->httpGet(GITHUB_API_RELEASE_URL)*/;

	//	[array
	//		{object
	//			"tag_name" : "22",
	//			"target_commitish": "master"
	//			"assets" : [array
	//							{object
	//								"browser_download_url": "https://github.com/McKay42/McOsu/releases/download/22/McOsu.Alpha.22.zip"
	//							}
	//			]
	//		},
	//		{
	//		},
	//		{
	//		}
	//	]

	/*
	JSONValue *value = JSON::Parse(gitReleases.toUtf8());
	if (value != NULL)
	{
		JSONArray root;
		if (value->IsArray() == false)
			printf("OsuUpdateChecker: Invalid JSON array.\n");
		else
		{
			root = value->AsArray();
			for (int i=0; i<root.size(); i++)
			{
				if (root[i]->IsObject())
				{
					JSONObject release = root[i]->AsObject();

					if (release.find(L"tag_name") != release.end() && release[L"tag_name"]->IsString() && release.find(L"target_commitish") != release.end() && release[L"target_commitish"]->IsString())
					{
						// get release version, branch, stream and OS
						UString versionString = UString(release[L"tag_name"]->AsString().c_str());

						Environment::OS os = stringToOS(versionString);
						if (os == Environment::OS::OS_NULL)
						{
							printf("OsuUpdateChecker: Invalid OS in version \"%s\".\n", versionString.toUtf8());
							continue;
						}

						STREAM stream = stringToStream(versionString);
						if (stream == STREAM::STREAM_NULL)
						{
							printf("OsuUpdateChecker: Invalid stream in version \"%s\".\n", versionString.toUtf8());
							continue;
						}

						float version = versionString.toFloat();
						UString branch = UString(release[L"target_commitish"]->AsString().c_str());

						// we are only interested in the master branch
						if (branch == "master")
						{
							if (release.find(L"assets") != release.end() && release[L"assets"]->IsArray())
							{
								JSONArray assets = release[L"assets"]->AsArray();
								for (int a=0; a<assets.size(); a++)
								{
									if (assets[a]->IsObject())
									{
										JSONObject asset = assets[a]->AsObject();

										if (asset.find(L"browser_download_url") != asset.end() && asset[L"browser_download_url"]->IsString())
										{
											// get download URL
											UString downloadURL = UString(asset[L"browser_download_url"]->AsString().c_str());

											// we now have everything
											GITHUB_RELEASE_BUILD b;
											b.os = os;
											b.stream = stream;
											b.version = version;
											b.downloadURL = downloadURL;
											asyncReleases.push_back(b);
										}
									}
									else
										printf("OsuUpdateChecker: Invalid JSON asset object.\n");
								}
							}
						}
					}
				}
				else
					printf("OsuUpdateChecker: Invalid JSON object.\n");
			}
		}
		delete value;
	}
	*/

	m_releases = asyncReleases;

	debugLog("OsuUpdateChecker: Found %i releases.\n", m_releases.size());
	for (int i=0; i<m_releases.size(); i++)
	{
		debugLog("OsuUpdateChecker: Release #%i: version = %g, downloadURL = %s\n", i, m_releases[i].version, m_releases[i].downloadURL.toUtf8());
	}

	if (!isUpdateAvailable())
		m_status = STATUS::STATUS_UP_TO_DATE;
}

bool OsuUpdateHandler::_downloadUpdate(UString url)
{
	debugLog("OsuUpdateHandler::downloadUpdate( %s )\n", url.toUtf8());
	m_status = STATUS::STATUS_DOWNLOADING_UPDATE;

	// setting the status in every error check return is retarded

	// download
	std::string data/* = engine->getNetworkHandler()->httpDownload(url)*/;
	if (data.length() < 2)
	{
		debugLog("OsuUpdateHandler::downloadUpdate() error, downloaded file is too small (%i)!\n", data.length());
		m_status = STATUS::STATUS_ERROR;
		return false;
	}

	// write to disk
	debugLog("OsuUpdateHandler: Downloaded file has %i length, writing ...\n", data.length());
	std::ofstream file(TEMP_UPDATE_DOWNLOAD_FILEPATH, std::ios::out | std::ios::binary);
	if (file.good())
	{
		file.write(data.data(), data.length());
		file.close();
	}
	else
	{
		debugLog("OsuUpdateHandler::downloadUpdate() error, can't write file!\n");
		m_status = STATUS::STATUS_ERROR;
		return false;
	}

	debugLog("OsuUpdateHandler::downloadUpdate() finished successfully.\n");
	return true;
}

void OsuUpdateHandler::_installUpdate(UString zipFilePath)
{
	debugLog("OsuUpdateHandler::installUpdate( %s )\n", zipFilePath.toUtf8());
	m_status = STATUS::STATUS_INSTALLING_UPDATE;

	// setting the status in every error check return is retarded

	if (!env->fileExists(zipFilePath))
	{
		debugLog("OsuUpdateHandler::installUpdate() error, \"%s\" does not exist!\n", zipFilePath.toUtf8());
		m_status = STATUS::STATUS_ERROR;
		return;
	}

	// load entire file
	File f(zipFilePath);
	if (!f.canRead())
	{
		debugLog("OsuUpdateHandler::installUpdate() error, can't read file!\n");
		m_status = STATUS::STATUS_ERROR;
		return;
	}
	const char *content = f.readFile();

	/*
	// initialize zip
	mz_zip_archive zip_archive;
	memset(&zip_archive, 0, sizeof(zip_archive));
	if (!mz_zip_reader_init_mem(&zip_archive, content, f.getFileSize(), 0))
	{
		debugLog("OsuUpdateHandler::installUpdate() error, couldn't mz_zip_reader_init_mem()!\n");
		m_status = STATUS::STATUS_ERROR;
		return;
	}

	mz_uint numFiles = mz_zip_reader_get_num_files(&zip_archive);
	if (numFiles <= 0)
	{
		debugLog("OsuUpdateHandler::installUpdate() error, %u files!\n", numFiles);
		m_status = STATUS::STATUS_ERROR;
		return;
	}
	if (!mz_zip_reader_is_file_a_directory(&zip_archive, 0))
	{
		debugLog("OsuUpdateHandler::installUpdate() error, first index is not the main directory!\n");
		m_status = STATUS::STATUS_ERROR;
		return;
	}

	// get main dir name (assuming that the first file is the main subdirectory)
	mz_zip_archive_file_stat file_stat;
	mz_zip_reader_file_stat(&zip_archive, 0, &file_stat);
	UString mainDirectory = UString(file_stat.m_filename);

	// split raw dirs and files
	std::vector<UString> files;
	std::vector<UString> dirs;
	for (int i=1; i<mz_zip_reader_get_num_files(&zip_archive); i++)
	{
		mz_zip_archive_file_stat file_stat;
		if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat))
		{
			debugLog("OsuUpdateHandler::installUpdate() warning, couldn't mz_zip_reader_file_stat() index %i!\n", i);
			continue;
		}

		if (mz_zip_reader_is_file_a_directory(&zip_archive, i))
			dirs.push_back(UString(file_stat.m_filename));
		else
			files.push_back(UString(file_stat.m_filename));

		debugLog("OsuUpdateHandler: Filename: \"%s\", isDir: %i, uncompressed size: %u, compressed size: %u\n", file_stat.m_filename, (int)mz_zip_reader_is_file_a_directory(&zip_archive, i), (unsigned int)file_stat.m_uncomp_size, (unsigned int)file_stat.m_comp_size);
	}

	// repair/create missing/new dirs
	UString cfgDir = "cfg/";
	bool cfgDirExists = env->directoryExists(cfgDir);
	for (int i=0; i<dirs.size(); i++)
	{
		int mainDirectoryOffset = dirs[i].find(mainDirectory);
		if (mainDirectoryOffset == 0 && dirs[i].length() - mainDirectoryOffset > 0 && mainDirectoryOffset + mainDirectory.length() < dirs[i].length())
		{
			UString newDir = dirs[i].substr(mainDirectoryOffset + mainDirectory.length());

			if (!env->directoryExists(newDir))
			{
				debugLog("OsuUpdateHandler: Creating directory %s\n", newDir.toUtf8());
				env->createDirectory(newDir);
			}
		}
	}

	// on windows it's impossible to overwrite a running executable, but we can simply rename/move it
	// this is not necessary on other operating systems, but it doesn't break anything so
	UString executablePath = env->getExecutablePath();
	if (executablePath.length() > 0)
	{
		UString oldExecutablePath = executablePath;
		oldExecutablePath.append(".old");
		env->deleteFile(oldExecutablePath); // must delete potentially existing file from previous update, otherwise renaming will not work
		env->renameFile(executablePath, oldExecutablePath);
	}

	// extract and overwrite almost everything
	for (int i=0; i<files.size(); i++)
	{
		// ignore cfg directory (don't want to overwrite user settings), except if it doesn't exist
		if (files[i].find(cfgDir) != -1 && cfgDirExists)
		{
			debugLog("OsuUpdateHandler: Ignoring file \"%s\"\n", files[i].toUtf8());
			continue;
		}

		int mainDirectoryOffset = files[i].find(mainDirectory);

		if (mainDirectoryOffset == 0 && files[i].length() - mainDirectoryOffset > 0 && mainDirectoryOffset + mainDirectory.length() < files[i].length())
		{
			UString outFilePath = files[i].substr(mainDirectoryOffset + mainDirectory.length());
			debugLog("OsuUpdateHandler: Writing %s\n", outFilePath.toUtf8());
			mz_zip_reader_extract_file_to_file(&zip_archive, files[i].toUtf8(), outFilePath.toUtf8(), 0);
		}
		else if (mainDirectoryOffset != 0)
			debugLog("OsuUpdateHandler::installUpdate() warning, ignoring file \"%s\" because it's not in the main dir!\n", files[i].toUtf8());
	}

	mz_zip_reader_end(&zip_archive);
	*/

	m_status = STATUS::STATUS_SUCCESS_INSTALLATION;
}

OsuUpdateHandler::STREAM OsuUpdateHandler::stringToStream(UString streamString)
{
	STREAM stream = STREAM::STREAM_NULL;
	if (streamString.find("desktop") != -1)
		stream = STREAM::STREAM_DESKTOP;
	else if (streamString.find("vr") != -1)
		stream = STREAM::STREAM_VR;

	return stream;
}

Environment::OS OsuUpdateHandler::stringToOS(UString osString)
{
	Environment::OS os = Environment::OS::OS_NULL;
	if (osString.find("windows") != -1)
		os = Environment::OS::OS_WINDOWS;
	else if (osString.find("linux") != -1)
		os = Environment::OS::OS_LINUX;
	/*
	else if (osString.find("macos") != -1)
		os = Environment::OS::OS_MACOS;
	*/

	return os;
}

OsuUpdateHandler::STREAM OsuUpdateHandler::getReleaseStream()
{
	return stringToStream(m_osu_release_stream_ref->getString());
}

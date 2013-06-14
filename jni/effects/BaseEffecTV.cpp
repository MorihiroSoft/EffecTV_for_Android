/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * BaseEffecTV.cpp :
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "BaseEffecTV.h"

#define  LOG_TAG "BaseEffecTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void BaseEffecTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Clear data
	mUtils         = NULL;
	video_width    = 0;
	video_height   = 0;
	video_area     = 0;

	// Set default parameters (no clear)
	if (reset) {
	}
}

void BaseEffecTV::setConfigPath(const char* dir, const char* file)
{
	LOGI("%s(L=%d): dir=%s, file=%s", __func__, __LINE__, dir, file);
	sprintf(mConfigPath, "%s/%s.cfg", dir, file);
	intialize(true);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
BaseEffecTV::BaseEffecTV(void)
: mUtils(NULL)
, video_width(0)
, video_height(0)
, video_area(0)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	mConfigPath[0] = '\0';
}

// Destructor
BaseEffecTV::~BaseEffecTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Initialize
int BaseEffecTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	mUtils       = utils;
	video_width  = width;
	video_height = height;
	video_area   = width * height;
	return 0;
}

// Finalize
int BaseEffecTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	intialize(false);
	return 0;
}

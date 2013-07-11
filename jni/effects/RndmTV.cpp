/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * RndmTV.cpp :
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

/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * rndmTV Random noise based on video signal
 * (c)2002 Ed Tannenbaum <et@et-arts.com>
 *
 */

#include "RndmTV.h"

#define  LOG_TAG "RndmTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "RndmTV";
static const char* EFFECT_TITLE = "RndmTV";
static const char* FUNC_LIST[]  = {
		"B/W",
		"Color",
		NULL
};

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void RndmTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			mode = 1;
		}
	} else {
		writeConfig();
	}
}

int RndmTV::readConfig()
{
	LOGI("%s(L=%d): path=%s", __func__, __LINE__, mConfigPath);
	FILE* fp = fopen(mConfigPath, "rb");
	if (fp == NULL) {
		return CONFIG_E_FOPEN;
	}
	int ver;
	bool rcode =
			FREAD_1(fp, ver) &&
			FREAD_1(fp, mode);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int RndmTV::writeConfig()
{
	LOGI("%s(L=%d): path=%s", __func__, __LINE__, mConfigPath);
	FILE* fp = fopen(mConfigPath, "wb");
	if (fp == NULL) {
		LOGE("%s(L=%d): fp=NULL", __func__, __LINE__);
		return CONFIG_E_FOPEN;
	}
	bool rcode =
			FWRITE_1(fp, CONFIG_VER) &&
			FWRITE_1(fp, mode);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
RndmTV::RndmTV(void)
: mode(0)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
RndmTV::~RndmTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* RndmTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* RndmTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** RndmTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int RndmTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	return 0;
}

// Finalize
int RndmTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return super::stop();
}

// Convert
int RndmTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	if (mode == 0) {
		for (int i=video_area; i>0; i--) {
			if ((mUtils->fastrand()>>24) < *src_yuv) {
				*dst_rgb = 0x00FFFFFF; // White
			} else {
				*dst_rgb = 0x00000000; // Black
			}
			src_yuv++;
			dst_rgb++;
		}
	} else {
		RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

		for (int i=video_area; i>0; i--) {
			int tmp = 0x00000000;
			int rtmp = mUtils->fastrand()>>24;
			if (rtmp < (((*src_rgb) & 0x00FF0000)>>16)) {
				tmp |= 0x00FF0000; // Red
			}
			if (rtmp < (((*src_rgb) & 0x0000FF00)>>8)) {
				tmp |= 0x0000FF00; // Green
			}
			if (rtmp < ((*src_rgb) & 0x000000FF)) {
				tmp |= 0x000000FF; // Blue
			}
			*dst_rgb = tmp;
			src_rgb++;
			dst_rgb++;
		}
	}

	return 0;
}

// Key functions
const char* RndmTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // B/W
		mode = 0;
		break;
	case 1: // Color
		mode = 1;
		break;
	}
	return NULL;
}

// Touch action
const char* RndmTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	return NULL;
}

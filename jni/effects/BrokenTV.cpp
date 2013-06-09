/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * BrokenTV.cpp :
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
 * Copyright (C) 2001-2006 FUKUCHI Kentarou
 *
 * BrokenTV - simulate broken VTR.
 * Copyright (C) 2002 Jun IIO
 *
 */

#include "BrokenTV.h"

#define  LOG_TAG "BrokenTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "BrokenTV";
static const char* EFFECT_TITLE = "Broken\nTV";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Speed\n+",
		"Speed\n-",
		NULL
};

#define MAX_SCROLL_SPEED 100
#define MIN_SCROLL_SPEED 1
#define DEF_SCROLL_SPEED 10

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void BrokenTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	offset = 0;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info = 1;
			speed = DEF_SCROLL_SPEED;
		}
	} else {
		writeConfig();
	}
}

int BrokenTV::readConfig()
{
	LOGI("%s(L=%d): path=%s", __func__, __LINE__, mConfigPath);
	FILE* fp = fopen(mConfigPath, "rb");
	if (fp == NULL) {
		return CONFIG_E_FOPEN;
	}
	int ver;
	bool rcode =
			FREAD_1(fp, ver) &&
			FREAD_1(fp, show_info) &&
			FREAD_1(fp, speed);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int BrokenTV::writeConfig()
{
	LOGI("%s(L=%d): path=%s", __func__, __LINE__, mConfigPath);
	FILE* fp = fopen(mConfigPath, "wb");
	if (fp == NULL) {
		LOGE("%s(L=%d): fp=NULL", __func__, __LINE__);
		return CONFIG_E_FOPEN;
	}
	bool rcode =
			FWRITE_1(fp, CONFIG_VER) &&
			FWRITE_1(fp, show_info) &&
			FWRITE_1(fp, speed);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
BrokenTV::BrokenTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
BrokenTV::~BrokenTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* BrokenTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* BrokenTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** BrokenTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int BrokenTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	return 0;
}

// Finalize
int BrokenTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return super::stop();
}

// Convert
int BrokenTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	memcpy(dst_rgb, src_rgb+(video_height - offset) * video_width,
			offset * video_width * PIXEL_SIZE);
	memcpy(dst_rgb+offset*video_width, src_rgb,
			(video_height - offset) * video_width * PIXEL_SIZE);
	add_noise(dst_rgb);

	offset = (offset + speed) % video_height;

	if (show_info) {
		sprintf(dst_msg, "Speed: %1d",
				speed);
	}

	return 0;
}

// Key functions
int BrokenTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Speed +
		speed += 1;
		if (speed > MAX_SCROLL_SPEED) {
			speed = MAX_SCROLL_SPEED;
		}
		break;
	case 2: // Speed -
		speed -= 1;
		if (speed < MIN_SCROLL_SPEED) {
			speed = MIN_SCROLL_SPEED;
		}
		break;
	}
	return 0;
}

// Touch action
int BrokenTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	return 0;
}

//---------------------------------------------------------------------
// LOCAL METHODs
//---------------------------------------------------------------------
//
void BrokenTV::add_noise(RGB32* dst)
{
	for (int y=offset,dy=0; dy<3 && y<video_height; y++,dy++) {
		int i = y * video_width;
		for (int x=0; x<video_width; x++,i++) {
			if (dy == 2 && mUtils->fastrand()>>31) {
				continue;
			}
			dst[i] = (mUtils->fastrand()>>31 ? 0xffffff : 0);
		}
	}
}

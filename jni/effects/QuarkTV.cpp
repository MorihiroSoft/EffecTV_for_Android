/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * QuarkTV.cpp :
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
 * QuarkTV - motion disolver.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include "QuarkTV.h"

#define  LOG_TAG "QuarkTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "QuarkTV";
static const char* EFFECT_TITLE = "Quark\nTV";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Delay\n+",
		"Delay\n-",
		NULL
};

#define PLANES 16

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void QuarkTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	plane      = -1;
	buffer     = NULL;
	planetable = NULL;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info = 1;
			delay = PLANES;
		}
	} else {
		writeConfig();
	}
}

int QuarkTV::readConfig()
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
			FREAD_1(fp, delay);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int QuarkTV::writeConfig()
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
			FWRITE_1(fp, delay);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
QuarkTV::QuarkTV(void)
: show_info(0)
, delay(0)
, plane(0)
, buffer(NULL)
, planetable(NULL)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
QuarkTV::~QuarkTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* QuarkTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* QuarkTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** QuarkTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int QuarkTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	// Allocate memory & setup
	buffer     = new RGB32[video_area * PLANES];
	planetable = new RGB32*[PLANES];
	if (buffer == NULL) {
		return -1;
	}
	memset(buffer, 0, video_area * PLANES);
	for (int i=0; i<PLANES; i++) {
		planetable[i] = &buffer[video_area * i];
	}

	return 0;
}

// Finalize
int QuarkTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Free memory
	if (buffer != NULL) {
		delete[] buffer;
	}
	if (planetable != NULL) {
		delete[] planetable;
	}

	//
	return super::stop();
}

// Convert
int QuarkTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	if (plane < 0) {
		for (int i=0; i<delay; i++) {
			memcpy(planetable[i], src_rgb, video_area * PIXEL_SIZE);
		}
		plane = 0;
	} else {
		memcpy(planetable[plane], src_rgb, video_area * PIXEL_SIZE);
	}

	for (int i=0; i<video_area; i++) {
		int cf = (mUtils->fastrand() >> 24) % delay;
		dst_rgb[i] = (planetable[cf])[i];
	}

	plane = (plane + 1) % delay;

	if (show_info) {
		sprintf(dst_msg, "Delay: %1d",
				delay);
	}

	return 0;
}

// Key functions
int QuarkTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Delay +
		delay++;
		if (delay > PLANES) {
			delay = PLANES;
		}
		plane = -1;
		break;
	case 2: // Delay -
		delay--;
		if (delay < 1) {
			delay = 1;
		}
		plane = -1;
		break;
	}
	return 0;
}

// Touch action
int QuarkTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	return 0;
}

/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * NervousTV.cpp :
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
 * nervousTV - The name says it all...
 * Copyright (C) 2002 TANNENBAUM Edo
 *
 * 2002/2/9
 *   Original code copied same frame twice, and did not use memcpy().
 *   I modifed those point.
 *   -Kentaro Fukuchi
 */

#include "NervousTV.h"

#define  LOG_TAG "NervousTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "NervousTV";
static const char* EFFECT_TITLE = "Nervous\nTV";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Random\nmode",
		"Scratch\nmode",
		NULL
};
static const char* MODE_LIST[] = {
		"Random mode",
		"Scratch mode",
};

#define PLANES 32

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void NervousTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	plane      = -1;
	timer      = 0;
	stride     = 0;
	readplane  = 0;
	buffer     = NULL;
	planetable = NULL;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info = 1;
			mode      = 0;
		}
	} else {
		writeConfig();
	}
}

int NervousTV::readConfig()
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
			FREAD_1(fp, mode);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int NervousTV::writeConfig()
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
			FWRITE_1(fp, mode);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
NervousTV::NervousTV(void)
: show_info(0)
, mode(0)
, plane(0)
, timer(0)
, stride(0)
, readplane(0)
, buffer(NULL)
, planetable(NULL)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
NervousTV::~NervousTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* NervousTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* NervousTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** NervousTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int NervousTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	// Allocate memory & setup
	buffer     = new RGB32[video_area * PLANES];
	planetable = new RGB32*[PLANES];
	if (buffer == NULL || planetable == NULL) {
		return -1;
	}
	memset(buffer, 0, video_area * PLANES * PIXEL_SIZE);
	for (int i=0; i<PLANES; i++) {
		planetable[i] = &buffer[video_area * i];
	}

	return 0;
}

// Finalize
int NervousTV::stop(void)
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
int NervousTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	if (plane < 0) {
		for (int i=0; i<PLANES; i++) {
			memcpy(planetable[i], src_rgb, video_area * PIXEL_SIZE);
		}
		plane = 0;
	} else {
		memcpy(planetable[plane], src_rgb, video_area * PIXEL_SIZE);
	}

	switch(mode) {
	case 0: // Random mode
		readplane = mUtils->fastrand() % PLANES;
		break;
	case 1: // Scratch mode
		if (timer) {
			readplane = readplane + stride;
			while(readplane <  0     ) readplane += PLANES;
			while(readplane >= PLANES) readplane -= PLANES;
			timer--;
		} else {
			readplane = mUtils->fastrand() % PLANES;
			stride    = mUtils->fastrand() % 5 - 2;
			if (stride >= 0) stride++;
			timer     = mUtils->fastrand() % 6 + 2;
		}
		break;
	}
	memcpy(dst_rgb, planetable[readplane], video_area * PIXEL_SIZE);

	plane = (plane + 1) % PLANES;

	if (show_info) {
		sprintf(dst_msg, "Mode: %s",
				MODE_LIST[mode]);
	}

	return 0;
}

// Key functions
const char* NervousTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Random mode
	case 2: // Scratch mode
		mode = key_code - 1;
		break;
	}
	return NULL;
}

// Touch action
const char* NervousTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	return NULL;
}

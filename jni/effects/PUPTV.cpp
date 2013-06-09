/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * PUPTV.cpp :
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
 * PUPTV - comes from "Partial UPdate", certain part of image is updated at a
 *         frame. This techniques is also used by 1DTV, but the result is
 *         (slightly) different.
 * Copyright (C) 2003 FUKUCHI Kentaro
 *
 */

#include "PUPTV.h"

#define  LOG_TAG "PUPTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "PUPTV";
static const char* EFFECT_TITLE = "PUPTV";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"PUP\nmode",
		"Param\n+",
		"Param\n-",
		NULL
};
static const char* MODE_LIST[] = {
		"Horizontal",
		"Vertical",
		"Diagonal",
		"Dissolution",
		"Random",
		"Raster",
};
static const int pup_mode_num = sizeof(MODE_LIST) / sizeof(MODE_LIST[0]);

#define MIN_PARAM 0
#define MAX_PARAM 100
#define DEF_STEP  10

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void PUPTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	bgIsSet = 0;
	phase   = 0;
	buffer  = NULL;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info = 1;
			mode  = 0;
			param = DEF_STEP;
		}
	} else {
		writeConfig();
	}
}

int PUPTV::readConfig()
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
			FREAD_1(fp, mode) &&
			FREAD_1(fp, param);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int PUPTV::writeConfig()
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
			FWRITE_1(fp, mode) &&
			FWRITE_1(fp, param);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
PUPTV::PUPTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
PUPTV::~PUPTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* PUPTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* PUPTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** PUPTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int PUPTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	// Allocate memory & setup
	buffer = new RGB32[video_area];
	if (buffer == NULL) {
		return -1;
	}

	return 0;
}

// Finalize
int PUPTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Free memory
	if (buffer != NULL) {
		delete[] buffer;
	}

	//
	return super::stop();
}

// Convert
int PUPTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	if (!bgIsSet) {
		resetBuffer(src_rgb);
	}

	switch(mode) {
	case 0:
		horizontalPup(src_rgb);
		break;
	case 1:
		verticalPup(src_rgb);
		break;
	case 2:
		diagonalPup(src_rgb);
		break;
	case 3:
		dissolutionPup(src_rgb);
		break;
	case 4:
		randomPup(src_rgb);
		break;
	case 5:
		rasterPup(src_rgb);
		break;
	default:
		break;
	}

	memcpy(dst_rgb, buffer, video_area * PIXEL_SIZE);

	if (show_info) {
		sprintf(dst_msg, "Mode: %s, Param: %1d (Touch screen to clear)",
				MODE_LIST[mode],
				param);
	}

	return 0;
}

// Key functions
int PUPTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // PUP mode
		mode = (mode + 1) % pup_mode_num;
		phase = 0;
		break;
	case 2: // Param +
		param++;
		if (param > MAX_PARAM) {
			param = MAX_PARAM;
		}
		break;
	case 3: // Param -
		param--;
		if (param < MIN_PARAM) {
			param = MIN_PARAM;
		}
		break;
	}
	return 0;
}

// Touch action
int PUPTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	switch(action) {
	case 0: // Down -> Space
		bgIsSet = 0;
		phase = 0;
		break;
	case 1: // Move
		break;
	case 2: // Up
		break;
	}
	return 0;
}

//---------------------------------------------------------------------
// LOCAL METHODs
//---------------------------------------------------------------------
//
int PUPTV::resetBuffer(RGB32* src)
{
	memcpy(buffer, src, video_area * PIXEL_SIZE);
	bgIsSet = 1;
	return 0;
}

//
void PUPTV::horizontalPup(RGB32* src)
{
	const int min  = 2;
	const int max  = (video_height>100 ? 100 : video_height);
	const int step = (param - MIN_PARAM) * (max - min) / (MAX_PARAM - MIN_PARAM) + min;

	phase %= step;

	RGB32* p = src    + video_width * phase;
	RGB32* q = buffer + video_width * phase;
	for (int y=phase; y<video_height; y+=step) {
		memcpy(q, p, video_width * PIXEL_SIZE);
		p += video_width * step;
		q += video_width * step;
	}

	phase = (phase + 1) % step;
}

//
void PUPTV::verticalPup(RGB32* src)
{
	const int min  = 2;
	const int max  = (video_width>100 ? 100 : video_width);
	const int step = (param - MIN_PARAM) * (max - min) / (MAX_PARAM - MIN_PARAM) + min;

	phase %= step;

	RGB32* p = src;
	RGB32* q = buffer;
	for (int y=0; y<video_height; y++) {
		for (int x=phase; x<video_width; x+=step) {
			q[x] = p[x];
		}
		p += video_width;
		q += video_width;
	}

	phase = (phase + 1) % step;
}

//
void PUPTV::diagonalPup(RGB32* src)
{
	const int min  = 0;
	const int max  = 100;
	const int step = (param - MIN_PARAM) * (max - min) / (MAX_PARAM - MIN_PARAM) + min;

	if (step == 0) {
		memcpy(buffer, src, video_area * PIXEL_SIZE);
		return;
	}

	phase %= step;

	RGB32* p = src;
	RGB32* q = buffer;
	for (int y=0; y<video_height; y++) {
		int x = (phase + y) % step;
		for(; x<video_width; x+=step) {
			q[x] = p[x];
		}
		p += video_width;
		q += video_width;
	}

	phase = (phase + 1) % step;
}

//
void PUPTV::dissolutionPup(RGB32* src)
{
	const int min  = 1;
	const int max  = 100;
	const int step = (param - MIN_PARAM) * (max - min) / (MAX_PARAM - MIN_PARAM) + min;

	phase %= step;

	RGB32* p = src    + phase;
	RGB32* q = buffer + phase;
	for (int i=phase; i<video_area; i+=step) {
		*q = *p;
		p += step;
		q += step;
	}

	phase = (phase + 1) % step;
}

//
void PUPTV::randomPup(RGB32* src)
{
	const int full  = 100;
	const int min   = 10;
	const int max   = 90;
	const int limit = (param - MIN_PARAM) * (max - min) / (MAX_PARAM - MIN_PARAM) + min;

	RGB32* p = src;
	RGB32* q = buffer;
	for (int i=video_area; i>0; i--) {
		if ((int)(mUtils->fastrand() % full) < limit) {
			*q = *p;
		}
		p++;
		q++;
	}
}

//
void PUPTV::rasterPup(RGB32* src)
{
	const int min  = 2;
	const int max  = (video_height>100 ? 100 : video_height);
	const int step = (param - MIN_PARAM) * (max - min) / (MAX_PARAM - MIN_PARAM) + min;

	phase %= step;

	RGB32* p = src;
	RGB32* q = buffer;
	for (int y=0; y<video_height; y++) {
		if (y & 1) {
			for (int x=phase; x<video_width; x+=step) {
				q[x] = p[x];
			}
		} else {
			for (int x=video_width-1-phase; x>=0; x-=step) {
				q[x] = p[x];
			}
		}
		p += video_width;
		q += video_width;
	}

	phase = (phase + 1) % step;
}

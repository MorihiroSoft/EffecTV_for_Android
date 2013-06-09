/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * ChameleonTV.cpp :
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
 * ChameleonTV - Vanishing into the wall!!
 * Copyright (C) 2003 FUKUCHI Kentaro
 *
 */

#include "ChameleonTV.h"

#define  LOG_TAG "ChameleonTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "ChameleonTV";
static const char* EFFECT_TITLE = "Chame\nleonTV";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Dis\nappearing",
		"Appearing",
		NULL
};
static const char* MODE_LIST[] = {
		"Disappearing (Touch screen to update BG)",
		"Appearing (Touch screen to update BG)",
};

#define PLANES_DEPTH 6
#define PLANES (1<<PLANES_DEPTH)

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void ChameleonTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	bgIsSet    = 0;
	plane      = 0;
	bgimage    = NULL;
	sum        = NULL;
	timebuffer = NULL;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info = 1;
			mode = 0;
		}
	} else {
		writeConfig();
	}
}

int ChameleonTV::readConfig()
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

int ChameleonTV::writeConfig()
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
ChameleonTV::ChameleonTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
ChameleonTV::~ChameleonTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* ChameleonTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* ChameleonTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "functionfunction label list(NULL TERMINATED)"
const char** ChameleonTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int ChameleonTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	// Allocate memory & setup
	bgimage    = new RGB32[video_area];
	sum        = new unsigned int[video_area];
	timebuffer = new unsigned char[video_area * PLANES];
	if (bgimage == NULL || sum == NULL || timebuffer == NULL) {
		return -1;
	}
	memset(sum, 0, video_area * sizeof(unsigned int));
	memset(timebuffer, 0, video_area * PLANES);

	return 0;
}

// Finalize
int ChameleonTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Free memory
	if (bgimage != NULL) {
		delete[] bgimage;
	}
	if (sum != NULL) {
		delete[] sum;
	}
	if (timebuffer != NULL) {
		delete[] timebuffer;
	}

	//
	return super::stop();
}

// Convert
int ChameleonTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	if (!bgIsSet) {
		setBackground(src_rgb);
	}

	if (mode == 0) {
		drawDisappearing(src_rgb, dst_rgb);
	} else {
		drawAppearing(src_rgb, dst_rgb);
	}

	if (show_info) {
		sprintf(dst_msg, "Mode: %s",
				MODE_LIST[mode]);
	}

	return 0;
}

// Key functions
int ChameleonTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Disappearing
	case 2: // Appearing
		mode = key_code - 1;
		break;
	}
	return 0;
}

// Touch action
int ChameleonTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	switch(action) {
	case 0: // Down -> Space
		bgIsSet = 0;
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
void ChameleonTV::drawDisappearing(RGB32* src, RGB32* dst)
{
	unsigned char* p = timebuffer + plane * video_area;
	RGB32*         q = bgimage;
	unsigned int*  s = sum;
	for (int i=video_area; i>0; i--) {
		RGB32 Y = *src++;

		int r = (Y>>16) & 0xff;
		int g = (Y>>8) & 0xff;
		int b = Y & 0xff;

		int R = (*q>>16) & 0xff;
		int G = (*q>>8) & 0xff;
		int B = *q & 0xff;

		Y = (r + g * 2 + b) >> 2;
		*s -= *p;
		*s += Y;
		*p = Y;
		Y = (abs(((int)Y<<PLANES_DEPTH) - (int)(*s)) * 8)>>PLANES_DEPTH;
		if (Y>255) Y = 255;

		R += ((r - R) * Y) >> 8;
		G += ((g - G) * Y) >> 8;
		B += ((b - B) * Y) >> 8;
		*dst++ = (R<<16)|(G<<8)|B;

		p++;
		q++;
		s++;
	}
	plane++;
	plane = plane & (PLANES-1);
}

//
void ChameleonTV::drawAppearing(RGB32* src, RGB32* dst)
{
	unsigned char* p = timebuffer + plane * video_area;
	RGB32*         q = bgimage;
	unsigned int*  s = sum;
	for (int i=video_area; i>0; i--) {
		RGB32 Y = *src++;

		int r = (Y>>16) & 0xff;
		int g = (Y>>8) & 0xff;
		int b = Y & 0xff;

		int R = (*q>>16) & 0xff;
		int G = (*q>>8) & 0xff;
		int B = *q & 0xff;

		Y = (r + g * 2 + b) >> 2;
		*s -= *p;
		*s += Y;
		*p = Y;
		Y = (abs(((int)Y<<PLANES_DEPTH) - (int)(*s)) * 8)>>PLANES_DEPTH;
		if (Y>255) Y = 255;

		r += ((R - r) * Y) >> 8;
		g += ((G - g) * Y) >> 8;
		b += ((B - b) * Y) >> 8;
		*dst++ = (r<<16)|(g<<8)|b;

		p++;
		q++;
		s++;
	}
	plane++;
	plane = plane & (PLANES-1);
}

//
void ChameleonTV::setBackground(RGB32* src)
{
	memcpy(bgimage, src, video_area * PIXEL_SIZE);
	bgIsSet = 1;
}

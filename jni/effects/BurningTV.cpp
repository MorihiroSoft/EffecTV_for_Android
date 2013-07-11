/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * BurningTV.cpp :
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
 * BurningTV - burns incoming objects.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 * Fire routine is taken from Frank Jan Sorensen's demo program.
 */

#include "BurningTV.h"

#define  LOG_TAG "BurningTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "BurningTV";
static const char* EFFECT_TITLE = "Burning\nTV";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Fore\nground",
		"Every\nthing",
		"Threshold\n+",
		"Threshold\n-",
		NULL
};
static const char* MODE_LIST[] = {
		"Foreground (Touch screen to update BG)",
		"Everything",
};

#define MaxColor 120
#define Decay 15

#define MAX_THRESHOLD 235
#define MIN_THRESHOLD 15 //16
#define DEF_THRESHOLD 50
#define DLT_THRESHOLD 5

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void BurningTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	bgIsSet = 0;
	palette = NULL;
	buffer  = NULL;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info = 1;
			mode = 1;
			threshold = DEF_THRESHOLD;
		}
	} else {
		writeConfig();
	}
}

int BurningTV::readConfig()
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
			FREAD_1(fp, threshold);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int BurningTV::writeConfig()
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
			FWRITE_1(fp, threshold);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
BurningTV::BurningTV(void)
: show_info(0)
, mode(0)
, threshold(0)
, bgIsSet(0)
, palette(NULL)
, buffer(NULL)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
BurningTV::~BurningTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* BurningTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* BurningTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** BurningTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int BurningTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	// Allocate memory & setup
	int ret = makePalette();
	if (ret != 0) {
		return -1;
	}

	buffer = new unsigned char[video_area];
	if (buffer == NULL) {
		return -1;
	}
	memset(buffer, 0, video_area);

	mUtils->image_set_threshold_yuv_y(threshold);

	return 0;
}

// Finalize
int BurningTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Free memory
	if (palette != NULL) {
		delete[] palette;
	}
	if (buffer != NULL) {
		delete[] buffer;
	}

	//
	return super::stop();
}

// Convert
int BurningTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	if (!bgIsSet) {
		setBackground(src_yuv);
	}

	unsigned char* diff;
	if (mode == 0) {
		diff = mUtils->image_bgsubtract_yuv_y(src_yuv);
	} else {
		diff = mUtils->image_yuv_y_over(src_yuv);
	}

	for (int x=1; x<video_width-1; x++) {
		unsigned char v = 0;
		for (int y=0; y<video_height-1; y++) {
			unsigned char w = diff[y*video_width+x];
			buffer[y*video_width+x] |= v ^ w;
			v = w;
		}
	}
	for (int x=1; x<video_width-1; x++) {
		int x2 = video_width + x;
		for (int y=1; y<video_height; y++) {
			unsigned char v = buffer[x2];
			if (v<Decay)
				buffer[x2-video_width] = 0;
			else
				buffer[x2-video_width+mUtils->fastrand()%3-1] = v - (mUtils->fastrand()&Decay);
			x2 += video_width;
		}
	}

	for (int y=0,i=0; y<video_height; y++) {
		dst_rgb[i++] = 0;
		for (int x=1; x<video_width-1; x++,i++) {
			RGB32 a = (src_rgb[i] & 0xfefeff) + palette[buffer[i]];
			RGB32 b = a & 0x1010100;
			dst_rgb[i] = a | (b - (b >> 8));
		}
		dst_rgb[i++] = 0;
	}

	if (show_info) {
		sprintf(dst_msg, "Mode: %s, Threshold: %1d",
				MODE_LIST[mode],
				threshold);
	}

	return 0;
}

// Key functions
const char* BurningTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Foreground
	case 2: // Everything
		mode = key_code - 1;
		break;
	case 3: // Threshold +
		threshold += DLT_THRESHOLD;
		if (threshold > MAX_THRESHOLD) {
			threshold = MAX_THRESHOLD;
		}
		mUtils->image_set_threshold_yuv_y(threshold);
		break;
	case 4: // Threshold -
		threshold -= DLT_THRESHOLD;
		if (threshold < MIN_THRESHOLD) {
			threshold = MIN_THRESHOLD;
		}
		mUtils->image_set_threshold_yuv_y(threshold);
		break;
	}
	return NULL;
}

// Touch action
const char* BurningTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	switch(action) {
	case 0: // Down -> Space
		if (mode == 0) {
			bgIsSet = 0;
		}
		break;
	case 1: // Move
		break;
	case 2: // Up
		break;
	}
	return NULL;
}

//---------------------------------------------------------------------
// LOCAL METHODs
//---------------------------------------------------------------------
//
int BurningTV::makePalette(void)
{
	palette = new RGB32[256];
	if (palette == NULL) {
		return -1;
	}

	int r, g, b;
	for (int i=0; i<MaxColor; i++) {
		mUtils->HSItoRGB(4.6-1.5*i/MaxColor, (double)i/MaxColor, (double)i/MaxColor, &r, &g, &b);
		palette[i] = ((r<<16)|(g<<8)|b) & 0xfefeff;
	}
	for (int i=MaxColor; i<256; i++) {
		if(r<255)r++; if(r<255)r++; if(r<255)r++;
		if(g<255)g++;
		if(g<255)g++;
		if(b<255)b++;
		if(b<255)b++;
		palette[i] = ((r<<16)|(g<<8)|b) & 0xfefeff;
	}

	return 0;
}

//
int BurningTV::setBackground(YUV* src)
{
	mUtils->image_bgset_yuv_y(src);
	bgIsSet = 1;
	return 0;
}

/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * FireTV.cpp :
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
 * FireTV - clips incoming objects and burn them.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 * Fire routine is taken from Frank Jan Sorensen's demo program.
 */

#include "FireTV.h"

#define  LOG_TAG "FireTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "FireTV";
static const char* EFFECT_TITLE = "FireTV";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Fore\nground",
		"Light\npart",
		"Dark\npart",
		"Threshold\n+",
		"Threshold\n-",
		NULL
};
static const char* MODE_LIST[] = {
		"Foreground (Touch screen to update BG)",
		"Light part",
		"Dark part",
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
void FireTV::intialize(bool reset)
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
			mode = 0;
			threshold = DEF_THRESHOLD;
		}
	} else {
		writeConfig();
	}
}

int FireTV::readConfig()
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

int FireTV::writeConfig()
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
FireTV::FireTV(void)
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
FireTV::~FireTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* FireTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* FireTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** FireTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int FireTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	// Allocate memory & setup
	if (makePalette() != 0) {
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
int FireTV::stop(void)
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
int FireTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);

	if (!bgIsSet) {
		setBackground(src_yuv);
	}

	unsigned char* diff;
	unsigned char yuv_y;
	switch(mode) {
	case 0:
	default:
		diff = mUtils->image_bgsubtract_yuv_y(src_yuv);
		for (int i=0; i<video_area-video_width; i++) {
			buffer[i] |= diff[i];
		}
		break;
	case 1:
		for (int i=0; i<video_area-video_width; i++) {
			yuv_y = src_yuv[i];
			if(yuv_y > 150) {
				buffer[i] |= yuv_y;
			}
		}
		break;
	case 2:
		for (int i=0; i<video_area-video_width; i++) {
			yuv_y = src_yuv[i];
			if(yuv_y < 60) {
				buffer[i] |= 0xff - yuv_y;
			}
		}
		break;
	}

	for (int x=1; x<video_width-1; x++) {
		int i = video_width + x;
		for (int y=1; y<video_height; y++) {
			unsigned char v = buffer[i];
			if (v < Decay) {
				buffer[i-video_width] = 0;
			} else {
				buffer[i-video_width+mUtils->fastrand()%3-1] = v - (mUtils->fastrand()&Decay);
			}
			i += video_width;
		}
	}

	for (int y=0; y<video_height; y++) {
		for (int x=1; x<video_width-1; x++) {
			dst_rgb[y*video_width+x] = palette[buffer[y*video_width+x]];
		}
	}

	if (show_info) {
		if (mode == 0) {
			sprintf(dst_msg, "Mode: %s, Threshold: %1d",
					MODE_LIST[mode],
					threshold);
		} else {
			sprintf(dst_msg, "Mode: %s",
					MODE_LIST[mode]);
		}
	}

	return 0;
}

// Key functions
int FireTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Foreground
	case 2: // Light part
	case 3: // Dark part
		mode = key_code - 1;
		break;
	case 4: // Threshold +
		if (mode == 0) {
			threshold += DLT_THRESHOLD;
			if (threshold > MAX_THRESHOLD) {
				threshold = MAX_THRESHOLD;
			}
			mUtils->image_set_threshold_yuv_y(threshold);
		}
		break;
	case 5: // Threshold -
		if (mode == 0) {
			threshold -= DLT_THRESHOLD;
			if (threshold < MIN_THRESHOLD) {
				threshold = MIN_THRESHOLD;
			}
			mUtils->image_set_threshold_yuv_y(threshold);
		}
		break;
	}
	return 0;
}

// Touch action
int FireTV::touch(int action, int x, int y)
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
	return 0;
}

//---------------------------------------------------------------------
// LOCAL METHODs
//---------------------------------------------------------------------
//
int FireTV::makePalette(void)
{
	palette = new RGB32[256];
	if (palette == NULL) {
		return -1;
	}

	int r, g, b;
	for (int i=0; i<MaxColor; i++) {
		mUtils->HSItoRGB(4.6-1.5*i/MaxColor, (double)i/MaxColor, (double)i/MaxColor, &r, &g, &b);
		palette[i] = ((r<<16)|(g<<8)|b);
	}
	for (int i=MaxColor; i<256; i++) {
		if(r<255)r++; if(r<255)r++; if(r<255)r++;
		if(g<255)g++;
		if(g<255)g++;
		if(b<255)b++;
		if(b<255)b++;
		palette[i] = ((r<<16)|(g<<8)|b);
	}

	return 0;
}

//
int FireTV::setBackground(YUV* src)
{
	mUtils->image_bgset_yuv_y(src);
	bgIsSet = 1;

	return 0;
}

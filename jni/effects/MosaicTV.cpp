/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * MosaicTV.cpp :
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
 * MosaicTV - censors incoming objects
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include "MosaicTV.h"

#define  LOG_TAG "MosaicTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "MosaicTV";
static const char* EFFECT_TITLE = "Mosaic\nTV";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Half\nsize",
		"Double\nsize",
		"Threshold\n+",
		"Threshold\n-",
		NULL
};

#define CENSOR_LEVEL 20

#define MAX_THRESHOLD 235
#define MIN_THRESHOLD 15 //16
#define DEF_THRESHOLD 40
#define DLT_THRESHOLD 5

#define DEF_MOSAIC_SIZE 16
#define MAX_MOSAIC_SIZE 32
#define MIN_MOSAIC_SIZE  8

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void MosaicTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	bgIsSet = 0;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info = 1;
			threshold = DEF_THRESHOLD;
			mosaic_size = DEF_MOSAIC_SIZE;
		}
		mosaic_half = mosaic_size / 2;
	} else {
		writeConfig();
	}
}

int MosaicTV::readConfig()
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
			FREAD_1(fp, threshold) &&
			FREAD_1(fp, mosaic_size);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int MosaicTV::writeConfig()
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
			FWRITE_1(fp, threshold) &&
			FWRITE_1(fp, mosaic_size);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
MosaicTV::MosaicTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
MosaicTV::~MosaicTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* MosaicTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* MosaicTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** MosaicTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int MosaicTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	mUtils->image_set_threshold_yuv_y(threshold);

	return 0;
}

// Finalize
int MosaicTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return super::stop();
}

// Convert
int MosaicTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	if (!bgIsSet) {
		setBackground(src_yuv);
	}

	unsigned char* diff = mUtils->image_bgsubtract_yuv_y(src_yuv);

	for (int y=0; y<video_height-(mosaic_size-1); y+=mosaic_size) {
		for (int x=0; x<video_width-(mosaic_size-1); x+=mosaic_size) {
			int count = 0;
			RGB32*         p = &src_rgb[y*video_width+x];
			RGB32*         q = &dst_rgb[y*video_width+x];
			unsigned char* r = &diff[y*video_width+x];
			for (int yy=0; yy<mosaic_size; yy++) {
				for (int xx=0; xx<mosaic_size; xx++) {
					count += r[yy*video_width+xx];
				}
			}
			if (count > CENSOR_LEVEL*255) {
				int v = p[mosaic_half*video_width+mosaic_half];
				for (int yy=0; yy<mosaic_size; yy++) {
					for (int xx=0; xx<mosaic_size; xx++){
						q[yy*video_width+xx] = v;
					}
				}
			} else {
				for (int yy=0; yy<mosaic_size; yy++) {
					for (int xx=0; xx<mosaic_size; xx++){
						q[yy*video_width+xx] = p[yy*video_width+xx];
					}
				}
			}
		}
	}

	if (show_info) {
		sprintf(dst_msg, "Size: %1dx%1d, Threshold: %1d (Touch screen to update BG)",
				mosaic_size,
				mosaic_size,
				threshold);
	}

	return 0;
}

// Key functions
int MosaicTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Half size
		mosaic_size >>= 1;
		if (mosaic_size < MIN_MOSAIC_SIZE) {
			mosaic_size = MIN_MOSAIC_SIZE;
		}
		mosaic_half = mosaic_size / 2;
		break;
	case 2: // Double size
		mosaic_size <<= 1;
		if (mosaic_size > MAX_MOSAIC_SIZE) {
			mosaic_size = MAX_MOSAIC_SIZE;
		}
		mosaic_half = mosaic_size / 2;
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
	return 0;
}

// Touch action
int MosaicTV::touch(int action, int x, int y)
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
int MosaicTV::setBackground(YUV* src)
{
	mUtils->image_bgset_yuv_y(src);
	bgIsSet = 1;
	return 0;
}

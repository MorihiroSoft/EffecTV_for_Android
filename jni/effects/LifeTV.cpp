/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * LifeTV.cpp :
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
 * LifeTV - Play John Horton Conway's `Life' game with video input.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 * This idea is stolen from Nobuyuki Matsushita's installation program of
 * ``HoloWall''. (See http://www.csl.sony.co.jp/person/matsu/index.html)
 */

#include "LifeTV.h"

#define  LOG_TAG "LifeTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "LifeTV";
static const char* EFFECT_TITLE = "LifeTV";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Threshold\n+",
		"Threshold\n-",
		NULL
};

#define MAX_THRESHOLD 235
#define MIN_THRESHOLD 15 //16
#define DEF_THRESHOLD 40
#define DLT_THRESHOLD 5

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void LifeTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	field  = NULL;
	field1 = NULL;
	field2 = NULL;

	// set default parameters. (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info = 1;
			threshold = DEF_THRESHOLD;
		}
	} else {
		writeConfig();
	}
}

int LifeTV::readConfig()
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
			FREAD_1(fp, threshold);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int LifeTV::writeConfig()
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
			FWRITE_1(fp, threshold);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
LifeTV::LifeTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
LifeTV::~LifeTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* LifeTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* LifeTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** LifeTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int LifeTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	// Allocate memory & setup
	field = new unsigned char[video_area*2];
	if (field == NULL) {
		return -1;
	}

	field1 = field;
	field2 = field + video_area;
	clear_field();

	mUtils->image_set_threshold_yuv_y(threshold);

	return 0;
}

// Finalize
int LifeTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Free memory
	if (field != NULL) {
		delete[] field;
	}

	//
	return super::stop();
}

// Convert
int LifeTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	const int vw = video_width;

	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	unsigned char* w = mUtils->image_diff_filter(mUtils->image_bgsubtract_update_yuv_y(src_yuv));
	for (int x=0; x<video_area; x++) {
		field1[x] |= w[x];
	}

	memset(dst_rgb, 0, video_area * PIXEL_SIZE);

	RGB32*         p  = src_rgb + vw + 1;
	RGB32*         q  = dst_rgb + vw + 1;
	unsigned char* r0 = field1 + 1;
	unsigned char* r1 = field2 + vw + 1;

	/* each value of cell is 0 or 0xff. 0xff can be treated as -1, so
	 * following equations treat each value as negative number. */
	for (int y=1; y<video_height-1; y++) {
		unsigned char sum1 = 0;
		unsigned char sum2 = r0[0] + r0[vw] + r0[vw*2];
		for (int x=1; x<vw-1; x++) {
			unsigned char sum3 = r0[1] + r0[vw+1] + r0[vw*2+1];
			unsigned char sum  = sum1 + sum2 + sum3;
			unsigned char v    = 0 - ((sum==0xfd)|((r0[vw]!=0)&(sum==0xfc)));
			*r1 = v;
			RGB32 pix = (signed char)v;
			//pix = pix >> 8;
			*q = pix | *p;
			sum1 = sum2;
			sum2 = sum3;

			p++;
			q++;
			r0++;
			r1++;
		}
		p  += 2;
		q  += 2;
		r0 += 2;
		r1 += 2;
	}
	w = field1;
	field1 = field2;
	field2 = w;

	if (show_info) {
		sprintf(dst_msg, "Threshold: %1d (Touch screen to clear field)",
				threshold);
	}

	return 0;
}

// Key functions
int LifeTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Threshold +
		threshold += DLT_THRESHOLD;
		if (threshold > MAX_THRESHOLD) {
			threshold = MAX_THRESHOLD;
		}
		mUtils->image_set_threshold_yuv_y(threshold);
		break;
	case 2: // Threshold -
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
int LifeTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	switch(action) {
	case 0: // Down -> Space
		clear_field();
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
void LifeTV::clear_field(void)
{
	memset(field1, 0, video_area);
}

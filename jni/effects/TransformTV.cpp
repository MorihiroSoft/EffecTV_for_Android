/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * TransformTV.cpp :
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
 * Plugin for EffecTV by Fukuchi Kentarou
 * Written by clifford smith <nullset@dookie.net>
 *
 * TransForm.c: Performs positinal translations on images
 *
 * Space selects b/w the different transforms
 *
 * basicaly TransformList contains an array of
 * values indicating where pixels go.
 * This value could be stored here or generated. The ones i use
 * here are generated.
 * Special value: -1 = black, -2 = get values from mapFromT(x,y,t)
 * ToDo: support multiple functions ( -3 to -10 or something?)
 * Note: the functions CAN return -1 to mean black....
 *
 */

#include "TransformTV.h"

#define  LOG_TAG "TransformTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "TransformTV";
static const char* EFFECT_TITLE = "Transform\nTV";
static const char* FUNC_LIST[] = {
		"Show\ninfo",
		"Mode",
		NULL
};
static const char* MODE_LIST[] = {
		"NORMAL",
		"H-FLIP",
		"V-FLIP",
		"ROTATE",
		"NOISE",
		"SQUARE",
};

#define TABLE_MAX 6 /* # of transforms */

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void TransformTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	TableList = NULL;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info = 1;
			mode      = 5;
		}
	} else {
		writeConfig();
	}
}

int TransformTV::readConfig()
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

int TransformTV::writeConfig()
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
TransformTV::TransformTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
TransformTV::~TransformTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* TransformTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* TransformTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** TransformTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int TransformTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	// Allocate memory & setup
	TableList = new int*[TABLE_MAX];
	if (TableList == NULL) {
		return -1;
	}
	for (int i=0; i<TABLE_MAX; i++) {
		TableList[i] = new int[video_area];
		if (TableList[i] == NULL) {
			return -1;
		}
	}

	for (int y=0;y < video_height;y++) {
		for (int x=0;x < video_width;x++) {
			int i = x + y * video_width;
			TableList[0][x                +y*video_width] = i;
			TableList[1][(video_width-1-x)+y*video_width] = i;
			TableList[2][x                +(video_height-1-y)*video_width] = i;
			TableList[3][(video_width-1-x)+(video_height-1-y)*video_width] = i;
			TableList[4][x                +y*video_width] = -2; /* Function */
		}
	}
	SquareTableInit();

	return 0;
}

// Finalize
int TransformTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);

	// Free memory
	if (TableList != NULL) {
		for (int i=0; i<TABLE_MAX; i++) {
			if (TableList[i] != NULL) {
				delete[] TableList[i];
			}
		}
		delete[] TableList;
	}

	//
	return super::stop();
}

// Convert
int TransformTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	static int t=0;
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	t++;

	for (int y=0; y<video_height; y++) {
		for (int x=0; x<video_width; x++) {
			int dst, value;
			dst = TableList[mode][x + y*video_width];
			if (dst == -2) {
				dst = mapFromT(x, y, t);
			}
			if (dst == -1) {
				value = 0;
			} else {
				value = src_rgb[dst];
			}
			dst_rgb[x+y*video_width] = value;
		}
	}

	if (show_info) {
		sprintf(dst_msg, "Mode: %s",
				MODE_LIST[mode]);
	}

	return 0;
}

// Key functions
int TransformTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Table
		mode = (mode + 1) % TABLE_MAX;
		break;
	}
	return 0;
}

// Touch action
int TransformTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	return 0;
}

//---------------------------------------------------------------------
// LOCAL METHODs
//---------------------------------------------------------------------
//
int TransformTV::mapFromT(int x,int y, int t)
{
	int yd = y + (mUtils->fastrand() >> 30) - 2;
	int xd = x + (mUtils->fastrand() >> 30) - 2;
	if (xd > video_width) {
		xd-=1;
	}
	return xd + yd * video_width;
}

//
void TransformTV::SquareTableInit(void)
{
	const int size = 16;

	for (int y=0; y<video_height; y++) {
		int ty = y % size - size / 2;
		if ((y/size)%2) {
			ty = y - ty;
		} else {
			ty = y + ty;
		}
		if (ty < 0) ty = 0;
		if (ty >= video_height) ty = video_height - 1;
		for (int x=0; x<video_width; x++) {
			int tx = x % size - size / 2;
			if ((x/size)%2) {
				tx = x - tx;
			} else {
				tx = x + tx;
			}
			if (tx < 0) tx = 0;
			if (tx >= video_width) tx = video_width - 1;
			TableList[5][x+y*video_width] = tx + ty * video_width;
		}
	}
}

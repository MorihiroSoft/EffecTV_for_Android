/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * RandomDotStereoTV.cpp :
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
 * RandomDotStereoTV - makes random dot stereogram.
 * Copyright (C) 2002 FUKUCHI Kentarou
 *
 */

#include "RandomDotStereoTV.h"

#define  LOG_TAG "RandomDotStereoTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "RandomDotStereoTV";
static const char* EFFECT_TITLE = "Random\nDot\nStereoTV";
static const char* FUNC_LIST[]  = {
		"Wall\neyed",
		"Cross\neyed",
		"Stride\n+",
		"Stride\n-",
		NULL
};

#define MAX_STRIDE (200)
#define MIN_STRIDE (40)

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void RandomDotStereoTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			mode   = 0;
			stride = 40;
		}
	} else {
		writeConfig();
	}
}

int RandomDotStereoTV::readConfig()
{
	LOGI("%s(L=%d): path=%s", __func__, __LINE__, mConfigPath);
	FILE* fp = fopen(mConfigPath, "rb");
	if (fp == NULL) {
		return CONFIG_E_FOPEN;
	}
	int ver;
	bool rcode =
			FREAD_1(fp, ver) &&
			FREAD_1(fp, mode) &&
			FREAD_1(fp, stride);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int RandomDotStereoTV::writeConfig()
{
	LOGI("%s(L=%d): path=%s", __func__, __LINE__, mConfigPath);
	FILE* fp = fopen(mConfigPath, "wb");
	if (fp == NULL) {
		LOGE("%s(L=%d): fp=NULL", __func__, __LINE__);
		return CONFIG_E_FOPEN;
	}
	bool rcode =
			FWRITE_1(fp, CONFIG_VER) &&
			FWRITE_1(fp, mode) &&
			FWRITE_1(fp, stride);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
RandomDotStereoTV::RandomDotStereoTV(void)
: mode(0)
, stride(0)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
RandomDotStereoTV::~RandomDotStereoTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* RandomDotStereoTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* RandomDotStereoTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** RandomDotStereoTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int RandomDotStereoTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	return 0;
}

// Finalize
int RandomDotStereoTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return super::stop();
}

// Convert
int RandomDotStereoTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);

	memset(dst_rgb, 0, video_area * PIXEL_SIZE);

	YUV*   p = src_yuv;
	RGB32* q = dst_rgb;

	if (mode) {
		for (int y=0; y<video_height; y++) {
			for (int i=0; i<stride; i++) {
				if (mUtils->fastrand() & 0xc0000000) {
					continue;
				}

				int x = video_width / 2 + i;
				q[x] = 0x00FFFFFF;

				while(x + stride / 2 < video_width) {
					x += stride + (p[x + stride / 2] >> 5);
					if(x >= video_width) break;
					q[x] = 0x00FFFFFF;
				}

				x = video_width / 2 + i;
				while(x - stride / 2 >= 0) {
					x -= stride + (p[x - stride / 2] >> 5);
					if(x < 0) break;
					q[x] = 0x00FFFFFF;
				}
			}
			p += video_width;
			q += video_width;
		}
	} else {
		for(int y=0; y<video_height; y++) {
			for(int i=0; i<stride; i++) {
				if(mUtils->fastrand() & 0xc0000000) {
					continue;
				}

				int x = video_width / 2 + i;
				q[x] = 0x00FFFFFF;

				while(x + stride/2 < video_width) {
					x += stride - (p[x + stride / 2] >> 5);
					if (x >= video_width) break;
					q[x] = 0x00FFFFFF;
				}

				x = video_width / 2 + i;
				while(x - stride/2 >= 0) {
					x -= stride - (p[x - stride / 2] >> 5);
					if (x < 0) break;
					q[x] = 0x00FFFFFF;
				}
			}
			p += video_width;
			q += video_width;
		}
	}

	// Red marker
	q = dst_rgb + video_width + (video_width - stride) / 2;
	for (int y=0; y<4; y++) {
		for (int x=0; x<4; x++) {
			q[x       ] = 0x00FF0000;
			q[x+stride] = 0x00FF0000;
		}
		q += video_width;
	}

	return 0;
}

// Key functions
const char* RandomDotStereoTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Wall-eyed
	case 1: // Cross-eyed
		mode = key_code;
		break;
	case 2: // Stride +
		stride += 10;
		if (stride > MAX_STRIDE) stride = MAX_STRIDE;
		break;
	case 3: // Stride -
		stride -= 10;
		if (stride < MIN_STRIDE) stride = MIN_STRIDE;
		break;
	}
	return NULL;
}

// Touch action
const char* RandomDotStereoTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	return NULL;
}

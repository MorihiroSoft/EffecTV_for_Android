/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * OpTV.cpp :
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
 * OpTV - Optical art meets real-time video effect.
 * Copyright (C) 2004-2005 FUKUCHI Kentaro
 *
 */

#include "OpTV.h"

#define  LOG_TAG "OpTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "OpTV";
static const char* EFFECT_TITLE = "OpTV";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Op\nmode",
		"Speed\n+",
		"Speed\n-",
		"Reverse",
		"Threshold\n+",
		"Threshold\n-",
		NULL
};
static const char* MODE_LIST[] = {
		"Maelstrom",
		"Radiation",
		"Perspective horizontal stripes",
		"Vertical stripes",
};

#define OPMAP_MAX 4
#define OP_SPIRAL1  0
#define OP_SPIRAL2  1
#define OP_PARABOLA 2
#define OP_HSTRIPE  3

#define MAX_THRESHOLD 235
#define MIN_THRESHOLD 15 //16
#define DEF_THRESHOLD 120
#define DLT_THRESHOLD 5

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void OpTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	phase   = 0;
	opmap   = NULL;
	palette = NULL;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info = 1;
			mode = 0;
			speed = 16;
			threshold = DEF_THRESHOLD;
		}
	} else {
		writeConfig();
	}
}

int OpTV::readConfig()
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
			FREAD_1(fp, speed) &&
			FREAD_1(fp, threshold);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int OpTV::writeConfig()
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
			FWRITE_1(fp, speed) &&
			FWRITE_1(fp, threshold);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
OpTV::OpTV(void)
: show_info(0)
, mode(0)
, speed(0)
, threshold(0)
, phase(0)
, opmap(NULL)
, palette(NULL)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
OpTV::~OpTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* OpTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* OpTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** OpTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int OpTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	// Allocate memory & setup
	opmap   = new signed char*[OPMAP_MAX];
	palette = new RGB32[256];
	if (opmap == NULL || palette == NULL) {
		return -1;
	}
	for (int i=0; i<OPMAP_MAX; i++) {
		opmap[i] = new signed char[video_area];
		if (opmap[i] == NULL) {
			return -1;
		}
	}

	initPalette();
	setOpmap();

	mUtils->image_set_threshold_yuv_y(threshold);

	return 0;
}

// Finalize
int OpTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Free memory
	if (opmap != NULL) {
		for (int i=0; i<OPMAP_MAX; i++) {
			if (opmap[i] == NULL) {
				delete[] opmap[i];
			}
		}
		delete[] opmap;
	}
	if (palette != NULL) {
		delete[] palette;
	}

	//
	return super::stop();
}

// Convert
int OpTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);

	unsigned char* diff = mUtils->image_yuv_y_over(src_yuv);

	signed char* p = opmap[mode];
	RGB32*       q = dst_rgb;
	for (int i=video_area; i>0; i--) {
		*q++ = palette[(((signed char)(*p++ + phase)) ^ *diff++) & 0xFF];
	}

	phase -= speed;

	if (show_info) {
		sprintf(dst_msg, "Mode: %s, Speed: %1d, Threshold: %1d",
				MODE_LIST[mode],
				speed,
				threshold);
	}

	return 0;
}

// Key functions
const char* OpTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // PUP mode
		mode = (mode + 1) % OPMAP_MAX;
		break;
	case 2: // Speed +
		speed++;
		break;
	case 3: // Speed -
		speed--;
		break;
	case 4: // Reverse
		speed = -speed;
		break;
	case 5: // Threshold +
		threshold += DLT_THRESHOLD;
		if (threshold > MAX_THRESHOLD) {
			threshold = MAX_THRESHOLD;
		}
		mUtils->image_set_threshold_yuv_y(threshold);
		break;
	case 6: // Threshold -
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
const char* OpTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	return NULL;
}

//---------------------------------------------------------------------
// LOCAL METHODs
//---------------------------------------------------------------------
//
void OpTV::initPalette(void)
{
	for (int i=0; i<112; i++) {
		palette[i] = 0;
		palette[i+128] = 0xffffff;
	}
	for (int i=0; i<16; i++) {
		unsigned char v = 16 * (i + 1) - 1;
		palette[i+112] = (v<<16) | (v<<8) | v;
		v = 255 - v;
		palette[i+240] = (v<<16) | (v<<8) | v;
	}
}

//
void OpTV::setOpmap(void)
{
	const int sci = 640 / video_width;
	for (int y=0,i=0; y<video_height; y++) {
		float yy = (float)(y - video_height / 2) / video_width;
		float y2 = yy * yy;
		for (int x=0; x<video_width; x++,i++) {
			float xx = (float)x / video_width - 0.5f;
			float r  = sqrtf(xx * xx + y2);
			float at = (float)atan2(xx, yy);

			opmap[OP_SPIRAL1][i] = ((int)((at / (float)M_PI * 256) + (r * 4000))) & 0xFF;

			int   j  = (int)(r * 300 / 32);
			float rr = r * 300 - j * 32;
			j *= 64;
			j += (rr > 28 ? (rr - 28) * 16 : 0);
			opmap[OP_SPIRAL2][i] = ((int)((at / (float)M_PI * 4096) + (r * 1600) - j)) & 0xFF;

			opmap[OP_PARABOLA][i] = ((int)(yy / (xx * xx * 0.3f + 0.1f) * 400)) & 0xFF;

			opmap[OP_HSTRIPE][i] = x * 8 * sci;
		}
	}
}

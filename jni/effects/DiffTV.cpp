/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * DiffTV.cpp :
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
 * diff.c: color independant differencing.  Just a little test.
 *  copyright (c) 2001 Sam Mertens.  This code is subject to the provisions of
 *  the GNU Public License.
 *
 * Controls:
 *      c   -   lower tolerance (threshhold)
 *      v   -   increase tolerance (threshhold)
 *
 */

#include "DiffTV.h"

#define  LOG_TAG "DiffTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "DiffTV";
static const char* EFFECT_TITLE = "DiffTV";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Threshold\n+",
		"Threshold\n-",
		NULL
};

#define TOLERANCE_STEP  4

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void DiffTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	prevbuf = NULL;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info = 1;
			g_tolerance[0] = 16;
			g_tolerance[1] = 16;
			g_tolerance[2] = 16;
		}
	} else {
		writeConfig();
	}
}

int DiffTV::readConfig()
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
			FREAD_N(fp, g_tolerance, 3);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int DiffTV::writeConfig()
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
			FWRITE_N(fp, g_tolerance, 3);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
DiffTV::DiffTV(void)
: show_info(0)
, g_tolerance()
, prevbuf(NULL)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
DiffTV::~DiffTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* DiffTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* DiffTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** DiffTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int DiffTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	// Allocate memory & setup
	prevbuf = new RGB32[video_area];
	if (prevbuf == NULL) {
		return -1;
	}
	memset(prevbuf, 0, video_area * PIXEL_SIZE);

	return 0;
}

// Finalize
int DiffTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Free memory
	if (prevbuf != NULL) {
		delete[] prevbuf;
	}

	//
	return super::stop();
}

// Convert
int DiffTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	for (int y=0,i=0; y<video_height; y++) {
		for (int x=0; x<video_width; x++,i++) {
			unsigned int red_val, red_diff;
			unsigned int grn_val, grn_diff;
			unsigned int blu_val, blu_diff;

			// MMX would just eat this algorithm up
			unsigned int src_red = (src_rgb[i] & 0x00FF0000) >> 16;
			unsigned int src_grn = (src_rgb[i] & 0x0000FF00) >> 8;
			unsigned int src_blu = (src_rgb[i] & 0x000000FF);

			unsigned int old_red = (prevbuf[i] & 0x00FF0000) >> 16;
			unsigned int old_grn = (prevbuf[i] & 0x0000FF00) >> 8;
			unsigned int old_blu = (prevbuf[i] & 0x000000FF);

			// RED
			if (src_red > old_red) {
				red_val = 0xFF;
				red_diff = src_red - old_red;
			} else {
				red_val = 0x7F;
				red_diff = old_red - src_red;
			}
			red_val = (red_diff>=g_tolerance[0] ? red_val : 0x00);

			// GREEN
			if (src_grn > old_grn) {
				grn_val = 0xFF;
				grn_diff = src_grn - old_grn;
			} else {
				grn_val = 0x7F;
				grn_diff = old_grn - src_grn;
			}
			grn_val = (grn_diff>=g_tolerance[1] ? grn_val : 0x00);

			// BLUE
			if (src_blu > old_blu) {
				blu_val = 0xFF;
				blu_diff = src_blu - old_blu;
			} else {
				blu_val = 0x7F;
				blu_diff = old_blu - src_blu;
			}
			blu_val = (blu_diff>=g_tolerance[2] ? blu_val : 0x00);

			prevbuf[i] = (
					(((src_red + old_red) >> 1) << 16) +
					(((src_grn + old_grn) >> 1) <<  8) +
					(((src_blu + old_blu) >> 1)));

			dst_rgb[i] = (red_val << 16) + (grn_val << 8) + blu_val;
		}
	}

	if (show_info) {
		sprintf(dst_msg, "Tolerance: %1d",
				g_tolerance[0]);
	}

	return 0;
}

// Key functions
int DiffTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Threshold +
		for (int i=0; i<3; i++) {
			if (g_tolerance[i] > 0xFF-TOLERANCE_STEP) {
				g_tolerance[i] = 0xFF;
			} else {
				g_tolerance[i] += TOLERANCE_STEP;
			}
		}
		break;
	case 2: // Threshold -
		for (int i=0; i<3; i++) {
			if (g_tolerance[i] < TOLERANCE_STEP) {
				g_tolerance[i] = 0x00;
			} else {
				g_tolerance[i] -= TOLERANCE_STEP;
			}
		}
		break;
	}
	return 0;
}

// Touch action
int DiffTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	return 0;
}

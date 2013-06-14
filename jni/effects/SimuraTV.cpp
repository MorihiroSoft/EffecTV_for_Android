/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * SimuraTV.cpp :
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
 * SimuraTV - color distortion and mirrored image effector
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include "SimuraTV.h"

#define  LOG_TAG "SimuraTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "SimuraTV";
static const char* EFFECT_TITLE = "Simura\nTV";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Mirror",
		"No\nColor",
		"Blue\n1-3",
		"Green\n1-3",
		"Cyan\n1-3",
		"Red\n1-3",
		"Magenta\n1-3",
		"Yellow\n1-3",
		"White\n1-3",
		"Other\n1-5",
		NULL
};
static const char* mirror_names[] = {
		"Normal",
		"Left",
		"Right",
		"Bottom",
		"Bottom-Left",
		"Bottom-Right",
		"Top",
		"Top-Left",
		"Top-Right",
};

static const struct ColorsTable {
	int num;
	RGB32 colors[5];
} colors_table[] = {
		{1, {0x000000}},
		{3, {0x000080, 0x0000e0, 0x0000ff}},
		{3, {0x008000, 0x00e000, 0x00ff00}},
		{3, {0x008080, 0x00e0e0, 0x00ffff}},
		{3, {0x800000, 0xe00000, 0xff0000}},
		{3, {0x800080, 0xe000e0, 0xff00ff}},
		{3, {0x808000, 0xe0e000, 0xffff00}},
		{3, {0x808080, 0xe0e0e0, 0xffffff}},
		{5, {0x76ca0a, 0x3cafaa, 0x60a848, 0x504858, 0x89ba43}},
};

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void SimuraTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	color = 0;
	w1    = 0;
	h1    = 0;
	w2    = 0;
	h2    = 0;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info   = 1;
			mirror      = 1;
			color_mode  = 1;
			color_level = 0;
		}
	} else {
		writeConfig();
	}
}

int SimuraTV::readConfig()
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
			FREAD_1(fp, mirror) &&
			FREAD_1(fp, color_mode) &&
			FREAD_1(fp, color_level);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int SimuraTV::writeConfig()
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
			FWRITE_1(fp, mirror) &&
			FWRITE_1(fp, color_mode) &&
			FWRITE_1(fp, color_level);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
SimuraTV::SimuraTV(void)
: show_info(0)
, mirror(0)
, color_mode(0)
, color_level(0)
, color(0)
, w1(0)
, h1(0)
, w2(0)
, h2(0)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
SimuraTV::~SimuraTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* SimuraTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* SimuraTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** SimuraTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int SimuraTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	w1 = video_width;
	h1 = video_height;
	w2 = w1 / 2;
	h2 = h1 / 2;

	return 0;
}

// Finalize
int SimuraTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return super::stop();
}

// Convert
int SimuraTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	color = colors_table[color_mode].colors[color_level];

	switch(mirror) {
	case 0:
	default:
		mirror_no(src_rgb, dst_rgb);
		break;
	case 1:
		mirror_l(src_rgb, dst_rgb);
		break;
	case 2:
		mirror_r(src_rgb, dst_rgb);
		break;
	case 3:
		mirror_d(src_rgb, dst_rgb);
		break;
	case 4:
		mirror_dl(src_rgb, dst_rgb);
		break;
	case 5:
		mirror_dr(src_rgb, dst_rgb);
		break;
	case 6:
		mirror_u(src_rgb, dst_rgb);
		break;
	case 7:
		mirror_ul(src_rgb, dst_rgb);
		break;
	case 8:
		mirror_ur(src_rgb, dst_rgb);
		break;
	}

	if (show_info) {
		sprintf(dst_msg, "Mode: %s (0x%08X)",
				mirror_names[mirror],
				color);
	}

	return 0;
}

// Key functions
int SimuraTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Mirror
		mirror = (mirror + 1) % (sizeof(mirror_names)/sizeof(mirror_names[0]));
		break;
	case  2: // Color - No
	case  3: // Color - Blue
	case  4: // Color - Green
	case  5: // Color - Cyan
	case  6: // Color - Red
	case  7: // Color - Magenta
	case  8: // Color - Yellow
	case  9: // Color - White
	case 10: // Color - Other
		int m = key_code - 2;
		if (m != color_mode) {
			color_mode  = m;
			color_level = 0;
		} else {
			color_level = (color_level + 1) % colors_table[color_mode].num;
		}
		break;
	}
	return 0;
}

// Touch action
int SimuraTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	switch(action) {
	case 0: // Down -> Space
		mirror = 0;
		color_mode  = 0;
		color_level = 0;
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
void SimuraTV::mirror_no(RGB32* src, RGB32* dst)
{
	for (int i=0; i<video_area; i++) {
		dst[i] = src[i] ^ color;
	}
}

//
void SimuraTV::mirror_l(RGB32* src, RGB32* dst)
{
	for (int y=0; y<h1; y++) {
		for (int x=0; x<w2; x++) {
			RGB32 c = src[y*w1+x] ^ color;
			dst[y*w1+x       ] = c;
			dst[y*w1+(w1-x-1)] = c;
		}
	}
}

//
void SimuraTV::mirror_r(RGB32* src, RGB32* dst)
{
	for (int y=0; y<h1; y++) {
		for (int x=w2; x<w1; x++) {
			RGB32 c = src[y*w1+x] ^ color;
			dst[y*w1+x       ] = c;
			dst[y*w1+(w1-x-1)] = c;
		}
	}
}

//
void SimuraTV::mirror_u(RGB32* src, RGB32* dst)
{
	for (int y=0; y<h2; y++) {
		for (int x=0; x<w1; x++) {
			RGB32 c = src[y*w1+x] ^ color;
			dst[y*w1+x       ] = c;
			dst[(h1-y-1)*w1+x] = c;
		}
	}
}

//
void SimuraTV::mirror_ul(RGB32* src, RGB32* dst)
{
	for (int y=0; y<h2; y++) {
		for (int x=0; x<w2; x++) {
			RGB32 c = src[y*w1+x] ^ color;
			dst[y*w1+x              ] = c;
			dst[y*w1+(w1-x-1)       ] = c;
			dst[(h1-y-1)*w1+x       ] = c;
			dst[(h1-y-1)*w1+(w1-x-1)] = c;
		}
	}
}

//
void SimuraTV::mirror_ur(RGB32* src, RGB32* dst)
{
	for (int y=0; y<h2; y++) {
		for (int x=w2; x<w1; x++) {
			RGB32 c = src[y*w1+x] ^ color;
			dst[y*w1+x              ] = c;
			dst[y*w1+(w1-x-1)       ] = c;
			dst[(h1-y-1)*w1+x       ] = c;
			dst[(h1-y-1)*w1+(w1-x-1)] = c;
		}
	}
}

//
void SimuraTV::mirror_d(RGB32* src, RGB32* dst)
{
	for (int y=h2; y<h1; y++) {
		for (int x=0; x<w1; x++) {
			RGB32 c = src[y*w1+x] ^ color;
			dst[y*w1+x       ] = c;
			dst[(h1-y-1)*w1+x] = c;
		}
	}
}

//
void SimuraTV::mirror_dl(RGB32* src, RGB32* dst)
{
	for (int y=h2; y<h1; y++) {
		for (int x=0; x<w2; x++) {
			RGB32 c = src[y*w1+x] ^ color;
			dst[y*w1+x              ] = c;
			dst[y*w1+(w1-x-1)       ] = c;
			dst[(h1-y-1)*w1+x       ] = c;
			dst[(h1-y-1)*w1+(w1-x-1)] = c;
		}
	}
}

//
void SimuraTV::mirror_dr(RGB32* src, RGB32* dst)
{
	for (int y=h2; y<h1; y++) {
		for (int x=w2; x<w1; x++) {
			RGB32 c = src[y*w1+x] ^ color;
			dst[y*w1+x              ] = c;
			dst[y*w1+(w1-x-1)       ] = c;
			dst[(h1-y-1)*w1+x       ] = c;
			dst[(h1-y-1)*w1+(w1-x-1)] = c;
		}
	}
}

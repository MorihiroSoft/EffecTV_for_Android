/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * ShagadelicTV.cpp :
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
 * ShagadelicTV - makes you shagadelic! Yeah baby yeah!
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * Inspired by Adrian Likin's script for the GIMP.
 */

#include "ShagadelicTV.h"

#define  LOG_TAG "ShagadelicTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "ShagadelicTV";
static const char* EFFECT_TITLE = "Shagadel\nicTV";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Red\nmask",
		"Green\nmask",
		"Blue\nmask",
		NULL
};

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void ShagadelicTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	phase  = 0;
	rx     = 0;
	ry     = 0;
	bx     = 0;
	by     = 0;
	rvx    = -2;
	rvy    = -2;
	bvx    = 2;
	bvy    = 2;
	mask   = 0xffffff;
	mask0  = 0xffffff;
	ripple = NULL;
	spiral = NULL;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info = 1;
		}
	} else {
		writeConfig();
	}
}

int ShagadelicTV::readConfig()
{
	LOGI("%s(L=%d): path=%s", __func__, __LINE__, mConfigPath);
	FILE* fp = fopen(mConfigPath, "rb");
	if (fp == NULL) {
		return CONFIG_E_FOPEN;
	}
	int ver;
	bool rcode =
			FREAD_1(fp, ver) &&
			FREAD_1(fp, show_info);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int ShagadelicTV::writeConfig()
{
	LOGI("%s(L=%d): path=%s", __func__, __LINE__, mConfigPath);
	FILE* fp = fopen(mConfigPath, "wb");
	if (fp == NULL) {
		LOGE("%s(L=%d): fp=NULL", __func__, __LINE__);
		return CONFIG_E_FOPEN;
	}
	bool rcode =
			FWRITE_1(fp, CONFIG_VER) &&
			FWRITE_1(fp, show_info);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
ShagadelicTV::ShagadelicTV(void)
: show_info(0)
, phase(0)
, rx(0)
, ry(0)
, bx(0)
, by(0)
, rvx(0)
, rvy(0)
, bvx(0)
, bvy(0)
, mask(0)
, mask0(0)
, ripple(NULL)
, spiral(NULL)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
ShagadelicTV::~ShagadelicTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* ShagadelicTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* ShagadelicTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** ShagadelicTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int ShagadelicTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	// Allocate memory & setup
	ripple = new signed char[video_area*4];
	spiral = new signed char[video_area];
	if (ripple == NULL || spiral == NULL) {
		return -1;
	}

	for (int y=0,i=0; y<video_height*2; y++) {
		float yy = (float)y / video_width - 1.0f;
		yy *= yy;
		for (int x=0; x<video_width*2; x++,i++) {
			float xx = (float)x / video_width - 1.0f;
			xx *= xx;
			ripple[i] = ((unsigned int)(sqrtf(xx+yy)*3000)) & 0xFF;
		}
	}

	for (int y=0,i=0; y<video_height; y++) {
		float yy = (float)(y - video_height / 2) / video_width;
		for (int x=0; x<video_width; x++,i++) {
			float xx = (float)x / video_width - 0.5f;
			spiral[i] = ((unsigned int)
					(((atan2(xx, yy)/M_PI+1.0f)*256*9) + (sqrtf(xx*xx+yy*yy)*1800))) & 0xFF;
			/* Here is another Swinger!
			 * ((atan2(xx, yy)/M_PI*256) + (sqrt(xx*xx+yy*yy)*3000))&255;
			 */
		}
	}

	rx = mUtils->fastrand() % video_width;
	ry = mUtils->fastrand() % video_height;
	bx = mUtils->fastrand() % video_width;
	by = mUtils->fastrand() % video_height;

	return 0;
}

// Finalize
int ShagadelicTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Free memory
	if (ripple != NULL) {
		delete[] ripple;
	}
	if (spiral != NULL) {
		delete[] spiral;
	}

	//
	return super::stop();
}

// Convert
int ShagadelicTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	signed char* pr = &ripple[ry*video_width*2 + rx];
	signed char* pg = spiral;
	signed char* pb = &ripple[by*video_width*2 + bx];

	for (int y=0; y<video_height; y++) {
		for (int x=0; x<video_width; x++) {
			RGB32 v = *src_rgb++ | 0x1010100;
			v = (v - 0x707060) & 0x1010100;
			v -= v>>8;
			/* Try another Babe!
			 * v = *src_rgb++;
			 * *dst_rgb++ = v & ((r<<16)|(g<<8)|b);
			 */
			unsigned char r = (signed char)(*pr+phase*2)>>7;
			unsigned char g = (signed char)(*pg+phase*3)>>7;
			unsigned char b = (signed char)(*pb-phase  )>>7;
			*dst_rgb++ = v & ((r<<16)|(g<<8)|b) & mask;
			pr++;
			pg++;
			pb++;
		}
		pr += video_width;
		pb += video_width;
	}

	phase -= 8;
	if ((rx+rvx)<0 || (rx+rvx)>=video_width ) rvx =-rvx;
	if ((ry+rvy)<0 || (ry+rvy)>=video_height) rvy =-rvy;
	if ((bx+bvx)<0 || (bx+bvx)>=video_width ) bvx =-bvx;
	if ((by+bvy)<0 || (by+bvy)>=video_height) bvy =-bvy;
	rx += rvx;
	ry += rvy;
	bx += bvx;
	by += bvy;

	if (show_info) {
		sprintf(dst_msg, "Touch screen to mask color (Mask: 0x%08X)",
				mask0);
	}

	return 0;
}

// Key functions
int ShagadelicTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Red mask
		mask0 = 0x00FF0000;
		break;
	case 2: // Green mask
		mask0 = 0x0000FF00;
		break;
	case 3: // Blue mask
		mask0 = 0x000000FF;
		break;
	}
	return 0;
}

// Touch action
int ShagadelicTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	switch(action) {
	case 0: // Down -> Space
		mask &= (mask0 ^ 0x00FFFFFF);
		break;
	case 1: // Move
		break;
	case 2: // Up
		mask |= mask0;
		break;
	}
	return 0;
}

/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * BaltanTV.cpp :
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
 * BaltanTV - like StreakTV, but following for a long time
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include "BaltanTV.h"

#define  LOG_TAG "BaltanTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

//static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "BaltanTV";
static const char* EFFECT_TITLE = "Baltan\nTV";
static const char* FUNC_LIST[]  = {
		NULL
};

#define PLANES 32
#define STRIDE 8
#define STRIDE_MASK 0xfcfcfcfc
#define STRIDE_SHIFT 2

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void BaltanTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	plane      = -1;
	buffer     = NULL;
	planetable = NULL;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
		}
	} else {
		writeConfig();
	}
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
BaltanTV::BaltanTV(void)
: plane(0)
, buffer(NULL)
, planetable(NULL)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
BaltanTV::~BaltanTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* BaltanTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* BaltanTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** BaltanTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int BaltanTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	// Allocate memory & setup
	buffer     = new RGB32[video_area * PLANES];
	planetable = new RGB32*[PLANES];
	if (buffer == NULL || planetable == NULL) {
		return -1;
	}
	memset(buffer, 0, video_area * PLANES);
	for (int i=0; i<PLANES; i++) {
		planetable[i] = &buffer[video_area * i];
	}

	return 0;
}

// Finalize
int BaltanTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Free memory
	if (buffer != NULL) {
		delete[] buffer;
	}
	if (planetable != NULL) {
		delete[] planetable;
	}

	//
	return super::stop();
}

// Convert
int BaltanTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	// Store buffer
	if (plane < 0) {
		for (int i=0; i<video_area; i++) {
			planetable[0][i] = (src_rgb[i] & STRIDE_MASK) >> STRIDE_SHIFT;
		}
		for (int i=1; i<PLANES; i++) {
			memcpy(planetable[i], planetable[0], video_area * PIXEL_SIZE);
		}
		plane = 0;
	} else {
		for (int i=0; i<video_area; i++) {
			planetable[plane][i] = (src_rgb[i] & STRIDE_MASK) >> STRIDE_SHIFT;
		}
	}

	// Store buffer
	int cf = plane & (STRIDE-1);
	for (int i=0; i<video_area; i++) {
		dst_rgb[i] =
				planetable[cf][i] +
				planetable[cf+STRIDE][i] +
				planetable[cf+STRIDE*2][i] +
				planetable[cf+STRIDE*3][i];
		planetable[plane][i] = (dst_rgb[i] & STRIDE_MASK) >> STRIDE_SHIFT;
	}

	plane++;
	plane = plane & (PLANES-1);

	return 0;
}

// Key functions
int BaltanTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	return 0;
}

// Touch action
int BaltanTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	return 0;
}

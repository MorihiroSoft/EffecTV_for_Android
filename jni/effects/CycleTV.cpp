/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * CycleTV.cpp :
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
 * cycleTV - no effect.
 * Written by clifford smith <nullset@dookie.net>
 *
 */

#include "CycleTV.h"

#define  LOG_TAG "CycleTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

//static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "CycleTV";
static const char* EFFECT_TITLE = "Cycle\nTV";
static const char* FUNC_LIST[]  = {
		NULL
};

#define NEWCOLOR(c,o) ((c+o)%230)

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void CycleTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	roff = 0;
	goff = 0;
	boff = 0;

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
CycleTV::CycleTV(void)
: roff(0)
, goff(0)
, boff(0)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
CycleTV::~CycleTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* CycleTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* CycleTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** CycleTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int CycleTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	return 0;
}

// Finalize
int CycleTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return super::stop();
}

// Convert
int CycleTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	roff += 1;
	goff += 3;
	boff += 7;

	for (int i=video_area; i>0; i--) {
		RGB32 t = *src_rgb++;
		*dst_rgb++ = RGB(
				NEWCOLOR(RED(t),   roff),
				NEWCOLOR(GREEN(t), goff),
				NEWCOLOR(BLUE(t),  boff));
	}

	return 0;
}

// Key functions
int CycleTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	return 0;
}

// Touch action
int CycleTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	return 0;
}

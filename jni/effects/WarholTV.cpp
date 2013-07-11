/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * WarholTV.cpp :
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
 * WarholTV - Hommage aux Andy Warhol
 * Copyright (C) 2002 Jun IIO
 *
 */

#include "WarholTV.h"

#define  LOG_TAG "WarholTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

//static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "WarholTV";
static const char* EFFECT_TITLE = "Warhol\nTV";
static const char* FUNC_LIST[]  = {
		NULL
};

#define DIVIDER 3

static const RGB32 colortable[] = {
		0x000080, 0x008000, 0x800000,
		0x00e000, 0x808000, 0x800080,
		0x808080, 0x008080, 0xe0e000,
};

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void WarholTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data

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
WarholTV::WarholTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
WarholTV::~WarholTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* WarholTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* WarholTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** WarholTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int WarholTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	return 0;
}

// Finalize
int WarholTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return super::stop();
}

// Convert
int WarholTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	for (int y=0; y<video_height; y++) {
		for (int x=0; x<video_width; x++) {
			int p = (x * DIVIDER) % video_width;
			int q = (y * DIVIDER) % video_height;
			int i = ((y * DIVIDER) / video_height) * DIVIDER +
					((x * DIVIDER) / video_width);
			*dst_rgb++ = src_rgb[q * video_width + p] ^ colortable[i];
		}
	}

	return 0;
}

// Key functions
const char* WarholTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	return NULL;
}

// Touch action
const char* WarholTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	return NULL;
}

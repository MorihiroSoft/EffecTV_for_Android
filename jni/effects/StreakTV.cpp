/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * StreakTV.cpp :
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
 * StreakTV - afterimage effector.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include "StreakTV.h"

#define  LOG_TAG "StreakTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "StreakTV";
static const char* EFFECT_TITLE = "Streak\nTV";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Blend\n+",
		"Blend\n-",
		"Delay\n+",
		"Delay\n-",
		NULL
};

#define PLANES 32
#define MIN_BLEND 1 // 1<<1 = 2
#define MAX_BLEND 3 // 1<<3 = 8

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void StreakTV::intialize(bool reset)
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
			show_info = 1;
			blend = 2; // (1<<2) = 4
			delay = 8; // PLANES/(1<<blend) = max
		}
	} else {
		writeConfig();
	}
}

int StreakTV::readConfig()
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
			FREAD_1(fp, blend) &&
			FREAD_1(fp, delay);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int StreakTV::writeConfig()
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
			FWRITE_1(fp, blend) &&
			FWRITE_1(fp, delay);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
StreakTV::StreakTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
StreakTV::~StreakTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* StreakTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* StreakTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** StreakTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int StreakTV::start(Utils* utils, int width, int height)
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
int StreakTV::stop(void)
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
int StreakTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	const unsigned char mask1    = ((0xFF >> blend) << blend);
	const RGB32         mask     = ((mask1 << 16) | (mask1 << 8) | mask1);
	const int           blendnum = (1 << blend);

	// Store buffer
	if (plane < 0) {
		RGB32* p = src_rgb;
		RGB32* q = planetable[0];
		for (int i=video_area; i>0; i--) {
			*q++ = (*p++ & mask) >> blend;
		}
		for (int i=1; i<PLANES; i++) {
			memcpy(planetable[i], planetable[0], video_area * PIXEL_SIZE);
		}
		plane = 0;
	} else {
		RGB32* p = src_rgb;
		RGB32* q = planetable[plane];
		for (int i=video_area; i>0; i--) {
			*q++ = (*p++ & mask) >> blend;
		}
	}

	// Calculate draw color
	{
		RGB32* p = planetable[plane];
		RGB32* q = dst_rgb;
		for (int i=video_area; i>0; i--) {
			*q++ = *p++;
		}
	}
	for (int j=1; j<blendnum; j++) {
		RGB32* p = planetable[(plane - delay * j + PLANES) % PLANES];
		RGB32* q = dst_rgb;
		for (int i=video_area; i>0; i--) {
			*q++ += *p++;
		}
	}

	plane = ((plane + 1) % PLANES);

	if (show_info) {
		sprintf(dst_msg, "Blend: %1d(N=%1d), Delay: %1d/%1d",
				blend,
				(1 << blend),
				delay,
				(PLANES / (1 << blend)));
	}

	return 0;
}

// Key functions
int StreakTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Blend +
		blend++;
		if (blend > MAX_BLEND) {
			blend = MAX_BLEND;
		}
		if (delay > PLANES / (1 << blend)) {
			delay = PLANES / (1 << blend);
		}
		plane = -1;
		break;
	case 2: // Blend -
		blend--;
		if (blend < MIN_BLEND) {
			blend = MIN_BLEND;
		}
		plane = -1;
		break;
	case 3: // Delay +
		delay++;
		if (delay > PLANES / (1 << blend)) {
			delay = PLANES / (1 << blend);
		}
		break;
	case 4: // Delay -
		delay--;
		if (delay < 0) {
			delay = 0;
		}
		break;
	}
	return 0;
}

// Touch action
int StreakTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	return 0;
}

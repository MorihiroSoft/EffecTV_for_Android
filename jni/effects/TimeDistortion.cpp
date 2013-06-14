/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * TimeDistortion.cpp :
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
 * TimeDistortionTV - scratch the surface and playback old images.
 * Copyright (C) 2005 Ryo-ta
 *
 * Ported and arranged by Kentaro Fukuchi
 */

#include "TimeDistortion.h"

#define  LOG_TAG "TimeDistortion"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "TimeDistortion";
static const char* EFFECT_TITLE = "TimeDisto\nrtion";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Threshold\n+",
		"Threshold\n-",
		NULL
};

#define PLANES 32

#define MAX_THRESHOLD 235
#define MIN_THRESHOLD 15 //16
#define DEF_THRESHOLD 40
#define DLT_THRESHOLD 5

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void TimeDistortion::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	bgIsSet       = 0;
	plane         = -1;
	warptimeFrame = 0;
	buffer        = NULL;
	planetable    = NULL;
	warptime[0]   = NULL;
	warptime[1]   = NULL;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info = 1;
			threshold = DEF_THRESHOLD;
		}
	} else {
		writeConfig();
	}
}

int TimeDistortion::readConfig()
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
			FREAD_1(fp, threshold);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int TimeDistortion::writeConfig()
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
			FWRITE_1(fp, threshold);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
TimeDistortion::TimeDistortion(void)
: show_info(0)
, threshold(0)
, bgIsSet(0)
, plane(0)
, warptimeFrame(0)
, buffer(NULL)
, planetable(NULL)
, warptime()
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
TimeDistortion::~TimeDistortion(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* TimeDistortion::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* TimeDistortion::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** TimeDistortion::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int TimeDistortion::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	// Allocate memory & setup
	buffer      = new RGB32[video_area * PLANES];
	planetable  = new RGB32*[PLANES];
	warptime[0] = new int[video_area];
	warptime[1] = new int[video_area];
	if (buffer == NULL || planetable == NULL || warptime[0] == NULL || warptime[1] == NULL) {
		return -1;
	}
	memset(buffer, 0, video_area * PLANES * PIXEL_SIZE);
	for (int i=0; i<PLANES; i++) {
		planetable[i] = &buffer[video_area * i];
	}
	memset(warptime[0], 0, video_area * sizeof(int));
	memset(warptime[1], 0, video_area * sizeof(int));

	mUtils->image_set_threshold_yuv_y(threshold);

	return 0;
}

// Finalize
int TimeDistortion::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Free memory
	if (buffer != NULL) {
		delete[] buffer;
	}
	if (planetable != NULL) {
		delete[] planetable;
	}
	if (warptime[0] != NULL) {
		delete[] warptime[0];
	}
	if (warptime[1] != NULL) {
		delete[] warptime[1];
	}

	//
	return super::stop();
}

// Convert
int TimeDistortion::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	if (plane < 0) {
		for (int i=0; i<PLANES; i++) {
			memcpy(planetable[i], src_rgb, video_area * PIXEL_SIZE);
		}
		plane = 0;
	} else {
		memcpy(planetable[plane], src_rgb, video_area * PIXEL_SIZE);
	}

	if (!bgIsSet) {
		setBackground(src_yuv);
	}
	unsigned char* diff = mUtils->image_bgsubtract_update_yuv_y(src_yuv);

	int* p = warptime[warptimeFrame    ] + video_width + 1;
	int* q = warptime[warptimeFrame ^ 1];
	for (int y=video_height-2; y>0; y--) {
		for (int x=video_width-2; x>0; x--) {
			int i = *(p - video_width) + *(p - 1) + *(p + 1) + *(p + video_width);
			if (i > 3) i -= 3;
			p++;
			*q++ = i >> 2;
		}
		p += 2;
		q += 2;
	}

	q = warptime[warptimeFrame ^ 1] + video_width + 1;
	for (int i=0; i<video_area; i++) {
		if (*diff++) {
			*q = PLANES - 1;
		}
		*dst_rgb++ = planetable[(plane - *q + PLANES) & (PLANES - 1)][i];
		q++;
	}

	plane++;
	plane = plane & (PLANES-1);

	warptimeFrame ^= 1;

	if (show_info) {
		sprintf(dst_msg, "Threshold: %1d",
				threshold);
	}

	return 0;
}

// Key functions
int TimeDistortion::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Threshold +
		threshold += DLT_THRESHOLD;
		if (threshold > MAX_THRESHOLD) {
			threshold = MAX_THRESHOLD;
		}
		mUtils->image_set_threshold_yuv_y(threshold);
		break;
	case 2: // Threshold -
		threshold -= DLT_THRESHOLD;
		if (threshold < MIN_THRESHOLD) {
			threshold = MIN_THRESHOLD;
		}
		mUtils->image_set_threshold_yuv_y(threshold);
		break;
	}
	return 0;
}

// Touch action
int TimeDistortion::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	return 0;
}

//---------------------------------------------------------------------
// LOCAL METHODs
//---------------------------------------------------------------------
//
int TimeDistortion::setBackground(YUV* src)
{
	mUtils->image_bgset_yuv_y(src);
	bgIsSet = 1;

	return 0;
}

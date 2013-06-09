/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * SloFastTV.cpp :
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
 * slofastTV - nonlinear time TV
 * Copyright (C) 2005 SCHUBERT Erich
 * based on slofastTV Copyright (C) 2002 TANNENBAUM Edo
 *
 */

#include "SloFastTV.h"

#define  LOG_TAG "SloFastTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "SloFastTV";
static const char* EFFECT_TITLE = "SloFast\nTV";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Plane\n+",
		"Plane\n-",
		NULL
};
static const char* MODE_LIST[] = {
		"FILL",
		"FLUSH",
};

#define PLANES 32
#define STATE_FILL  0
#define STATE_FLUSH 1

#define MIN_PLANES 8
#define MAX_PLANES PLANES
#define DEF_PLANES 8

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void SloFastTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	mode       = STATE_FILL;
	head       = 0;
	tail       = 0;
	count      = 0;
	buffer     = NULL;
	planetable = NULL;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info = 1;
			planes = DEF_PLANES;
		}
	} else {
		writeConfig();
	}
}

int SloFastTV::readConfig()
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
			FREAD_1(fp, planes);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int SloFastTV::writeConfig()
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
			FWRITE_1(fp, planes);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
SloFastTV::SloFastTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
SloFastTV::~SloFastTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* SloFastTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* SloFastTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** SloFastTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int SloFastTV::start(Utils* utils, int width, int height)
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
	memset(buffer, 0, video_area * PLANES * PIXEL_SIZE);
	for (int i=0; i<PLANES; i++) {
		planetable[i] = &buffer[video_area*i];
	}

	return 0;
}

// Finalize
int SloFastTV::stop(void)
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
int SloFastTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	/* store new frame */
	if (mode == STATE_FILL || (count & 0x1) == 1) {
		memcpy(planetable[head], src_rgb, video_area * PIXEL_SIZE);
		head = (head + 1) % planes;

		/* switch mode when head catches tail */
		if (head == tail) mode = STATE_FLUSH;
	}

	/* copy current tail image */
	memcpy(dst_rgb, planetable[tail], video_area * PIXEL_SIZE);
	if (mode == STATE_FLUSH || (count & 0x1) == 1) {
		tail = (tail + 1) % planes;

		/* switch mode when tail reaches head */
		if (head == tail) mode = STATE_FILL;
	}

	count++;

	if (show_info) {
		sprintf(dst_msg, "Mode: %s, Plane: %1d",
				MODE_LIST[mode],
				planes);
	}

	return 0;
}

// Key functions
int SloFastTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Plane +
		planes++;
		if (planes > MAX_PLANES) {
			planes = MAX_PLANES;
		}
		mode  = STATE_FILL;
		head  = 0;
		tail  = 0;
		count = 0;
		break;
	case 2: // Plane -
		planes--;
		if (planes < MIN_PLANES) {
			planes = MIN_PLANES;
		}
		mode  = STATE_FILL;
		head  = 0;
		tail  = 0;
		count = 0;
		break;
	}
	return 0;
}

// Touch action
int SloFastTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	return 0;
}

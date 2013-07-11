/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * OneDTV.cpp :
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
 * 1DTV - scans line by line and generates amazing still image.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include "OneDTV.h"

#define  LOG_TAG "OneDTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "1DTV";
static const char* EFFECT_TITLE = "1DTV";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Speed\n+",
		"Speed\n-",
		NULL
};

#define MIN_SPEED 1
#define MAX_SPEED 10
#define DEF_SPEED 3

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void OneDTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	line      = 0;
	prev_line = 0;
	prev_cnt  = 0;
	linebuf   = NULL;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info = 1;
			speed = DEF_SPEED;
		}
	} else {
		writeConfig();
	}
}

int OneDTV::readConfig()
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
			FREAD_1(fp, speed);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int OneDTV::writeConfig()
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
			FWRITE_1(fp, speed);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
OneDTV::OneDTV(void)
: show_info(0)
, speed(0)
, line(0)
, prev_line(0)
, prev_cnt(0)
, linebuf(NULL)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
OneDTV::~OneDTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* OneDTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* OneDTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** OneDTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int OneDTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	// Allocate memory & setup
	linebuf = new RGB32[video_width * MAX_SPEED];
	if (linebuf == NULL) {
		return -1;
	}

	return 0;
}

// Finalize
int OneDTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Free memory
	if (linebuf != NULL) {
		delete[] linebuf;
	}

	//
	return super::stop();
}

// Convert
int OneDTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	int cnt = (line+speed>video_height ? video_height-line : speed);

	blitline_buf(src_rgb, dst_rgb, cnt);

	line = (line + cnt) % video_height;

	dst_rgb += video_width * line;
	for (int i=0; i<video_width; i++) {
		dst_rgb[i] = 0x00ff00;
	}

	if (show_info) {
		sprintf(dst_msg, "Speed: %1d",
				speed);
	}

	return 0;
}

// Key functions
const char* OneDTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Speed +
		speed++;
		if (speed > MAX_SPEED) {
			speed = MAX_SPEED;
		}
		break;
	case 2: // Speed -
		speed--;
		if (speed < MIN_SPEED) {
			speed = MIN_SPEED;
		}
		break;
	}
	return NULL;
}

// Touch action
const char* OneDTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	return NULL;
}

//---------------------------------------------------------------------
// LOCAL METHODs
//---------------------------------------------------------------------
//
void OneDTV::blitline_buf(RGB32* src, RGB32* dst, int cnt)
{
	memcpy(dst + video_width * prev_line, linebuf, video_width * prev_cnt * PIXEL_SIZE);

	src += video_width * line;
	dst += video_width * line;
	memcpy(dst,     src, video_width * cnt * PIXEL_SIZE);
	memcpy(linebuf, src, video_width * cnt * PIXEL_SIZE);

	prev_line = line;
	prev_cnt  = cnt;
}

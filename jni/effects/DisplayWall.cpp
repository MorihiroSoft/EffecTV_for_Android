/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * DisplayWall.cpp :
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
 * DisplayWall
 * Copyright (C) 2005-2006 FUKUCHI Kentaro
 *
 */

#include "DisplayWall.h"

#define  LOG_TAG "DisplayWall"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "DisplayWall";
static const char* EFFECT_TITLE = "Display\nWall";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Scroll\nLeft",
		"Scroll\nRight",
		"Scroll\nUp",
		"Scroll\nDown",
		"Scale\n+",
		"Scale\n-",
		"Speed\n+",
		"Speed\n-",
		NULL
};

#define MAX_SCALE 9
#define MIN_SCALE 1
#define DEF_SCALE 3

#define MAX_SPEED 100
#define MIN_SPEED 0
#define DEF_SPEED 10

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void DisplayWall::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	dx   = 1;
	dy   = 0;
	bx   = 0;
	by   = 0;
	cx   = 0;
	cy   = 0;
	mx   = 0;
	my   = 0;
	vecx = NULL;
	vecy = NULL;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info = 1;
			scale = DEF_SCALE;
			speed = DEF_SPEED;
		}
	} else {
		writeConfig();
	}
}

int DisplayWall::readConfig()
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
			FREAD_1(fp, scale) &&
			FREAD_1(fp, speed);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int DisplayWall::writeConfig()
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
			FWRITE_1(fp, scale) &&
			FWRITE_1(fp, speed);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
DisplayWall::DisplayWall(void)
: show_info(0)
, scale(0)
, speed(0)
, dx(0)
, dy(0)
, bx(0)
, by(0)
, cx(0)
, cy(0)
, mx(0)
, my(0)
, vecx(NULL)
, vecy(NULL)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
DisplayWall::~DisplayWall(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* DisplayWall::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* DisplayWall::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** DisplayWall::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int DisplayWall::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	// Allocate memory & setup
	vecx = new int[video_area];
	vecy = new int[video_area];
	if (vecx == NULL || vecy == NULL) {
		return -1;
	}

	cx = video_width  / 2;
	cy = video_height / 2;

	initVec();

	return 0;
}

// Finalize
int DisplayWall::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Free memory
	if (vecx != NULL) {
		delete[] vecx;
	}
	if (vecy != NULL) {
		delete[] vecy;
	}

	//
	return super::stop();
}

// Convert
int DisplayWall::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	bx += dx * speed;
	by += dy * speed;
	while(bx < 0) bx += video_width;
	while(bx >= video_width) bx -= video_width;
	while(by < 0) by += video_height;
	while(by >= video_height) by -= video_height;

	if(scale == 1) {
		bx = cx;
		by = cy;
	}

	for (int y=0,i=0; y<video_height; y++) {
		for (int x=0; x<video_width; x++,i++) {
			int px = bx + vecx[i] * scale;
			int py = by + vecy[i] * scale;
			while(px < 0) px += video_width;
			while(px >= video_width) px -= video_width;
			while(py < 0) py += video_height;
			while(py >= video_height) py -= video_height;

			dst_rgb[i] = src_rgb[py * video_width + px];
		}
	}

	if (show_info) {
		sprintf(dst_msg, "Scale: %1d, Speed: %1d",
				scale,
				speed);
	}

	return 0;
}

// Key functions
int DisplayWall::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Scroll Left
		dx = 1;
		dy = 0;
		break;
	case 2: // Scroll Right
		dx = -1;
		dy = 0;
		break;
	case 3: // Scroll Up
		dx = 0;
		dy = 1;
		break;
	case 4: // Scroll Down
		dx = 0;
		dy = -1;
		break;
	case 5: // Scale +
		scale++;
		if (scale > MAX_SCALE) scale = MAX_SCALE;
		break;
	case 6: // Scale -
		scale--;
		if (scale < MIN_SCALE) scale = MIN_SCALE;
		break;
	case 7: // Speed +
		speed++;
		if (speed > MAX_SPEED) speed = MAX_SPEED;
		break;
	case 8: // Speed -
		speed--;
		if (speed < MIN_SPEED) speed = MIN_SPEED;
		break;
	}
	return 0;
}

// Touch action
int DisplayWall::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	switch(action) {
	case 0: // Down
		speed = 0;
		mx = x;
		my = y;
		break;
	case 1: // Move
		bx += mx - x;
		by += my - y;
		mx = x;
		my = y;
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
void DisplayWall::initVec(void)
{
	for (int y=0,i=0; y<video_height; y++) {
		for (int x=0; x<video_width; x++,i++) {
			double vx = (double)(x - cx) / video_width;
			double vy = (double)(y - cy) / video_width;

			vx *= 1.0 - vx * vx * 0.4;
			vy *= 1.0 - vx * vx * 0.8;
			vx *= 1.0 - (double)y / video_height * 0.15;
			vecx[i] = vx * video_width;
			vecy[i] = vy * video_width;
		}
	}
}

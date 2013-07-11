/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * VertigoTV.cpp :
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
 * VertigoTV - Alpha blending with zoomed and rotated images.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include "VertigoTV.h"

#define  LOG_TAG "VertigoTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "VertigoTV";
static const char* EFFECT_TITLE = "Vertigo\nTV";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Speed\n+",
		"Speed\n-",
		"Zoom\n+",
		"Zoom\n-",
		NULL
};

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void VertigoTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	dx             = 0;
	dy             = 0;
	sx             = 0;
	sy             = 0;
	buffer         = NULL;
	current_buffer = NULL;
	alt_buffer     = NULL;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info = 1;
			phase           = 0.0;
			phase_increment = 0.02;
			zoomrate        = 1.01;
		}
	} else {
		writeConfig();
	}
}

int VertigoTV::readConfig()
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
			FREAD_1(fp, phase) &&
			FREAD_1(fp, phase_increment) &&
			FREAD_1(fp, zoomrate);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int VertigoTV::writeConfig()
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
			FWRITE_1(fp, phase) &&
			FWRITE_1(fp, phase_increment) &&
			FWRITE_1(fp, zoomrate);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
VertigoTV::VertigoTV(void)
: show_info(0)
, dx(0)
, dy(0)
, sx(0)
, sy(0)
, phase(0)
, phase_increment(0)
, zoomrate(0)
, buffer(NULL)
, current_buffer(NULL)
, alt_buffer(NULL)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
VertigoTV::~VertigoTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* VertigoTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* VertigoTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** VertigoTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int VertigoTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	// Allocate memory & setup
	buffer = new RGB32[video_area * 2];
	if (buffer == NULL) {
		return -1;
	}
	memset(buffer, 0, video_area * 2 * PIXEL_SIZE);

	current_buffer = buffer;
	alt_buffer     = buffer + video_area;

	return 0;
}

// Finalize
int VertigoTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Free memory
	if (buffer != NULL) {
		delete[] buffer;
	}

	//
	return super::stop();
}

// Convert
int VertigoTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	setParams();

	RGB32* p = alt_buffer;
	for (int y=video_height; y>0; y--) {
		int ox = sx;
		int oy = sy;
		for (int x=video_width; x>0; x--) {
			int i = (oy>>16) * video_width + (ox>>16);
			if (i <  0         ) i = 0;
			if (i >= video_area) i = video_area;
			RGB32 v = current_buffer[i] & 0xfcfcff;
			v = (v * 3) + ((*src_rgb++) & 0xfcfcff);
			*p++ = (v>>2);
			ox += dx;
			oy += dy;
		}
		sx -= dy;
		sy += dx;
	}

	memcpy(dst_rgb, alt_buffer, video_area * PIXEL_SIZE);

	p = current_buffer;
	current_buffer = alt_buffer;
	alt_buffer = p;

	if (show_info) {
		sprintf(dst_msg, "Speed: %.2f, Zoom: %.2f (Touch screen to clear)",
				phase_increment,
				zoomrate);
	}

	return 0;
}

// Key functions
const char* VertigoTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Speed +
		phase_increment += 0.01;
		break;
	case 2: // Speed -
		phase_increment -= 0.01;
		if (phase_increment < 0.01) {
			phase_increment = 0.01;
		}
		break;
	case 3: // Zoom +
		zoomrate += 0.01;
		if (zoomrate > 1.1) {
			zoomrate = 1.1;
		}
		break;
	case 4: // Zoom -
		zoomrate -= 0.01;
		if (zoomrate < 1.01) {
			zoomrate = 1.01;
		}
		break;
	}
	return NULL;
}

// Touch action
const char* VertigoTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	switch(action) {
	case 0: // Down -> Space
		phase = 0.0;
		phase_increment = 0.02;
		zoomrate = 1.01;
		break;
	case 1: // Move
		break;
	case 2: // Up
		break;
	}
	return NULL;
}

//---------------------------------------------------------------------
// LOCAL METHODs
//---------------------------------------------------------------------
//
void VertigoTV::setParams(void)
{
	const double x = video_width  / 2;
	const double y = video_height / 2;
	const double t = (x * x + y * y) * zoomrate;

	double dizz = sin(phase) * 10 + sin(phase * 1.9 + 5) * 5;
	double vx, vy;
	if (video_width > video_height) {
		if (dizz >= 0) {
			if (dizz > x) dizz = x;
			vx = (x * (x - dizz) + y * y) / t;
		} else {
			if(dizz < -x) dizz = -x;
			vx = (x * (x + dizz) + y * y) / t;
		}
		vy = (dizz * y) / t;
	} else {
		if (dizz >= 0) {
			if (dizz > y) dizz = y;
			vx = (x * x + y * (y - dizz)) / t;
		} else {
			if (dizz < -y) dizz = -y;
			vx = (x * x + y * (y + dizz)) / t;
		}
		vy = (dizz*x) / t;
	}
	dx = vx * 65536;
	dy = vy * 65536;
	sx = (-vx * x + vy * y + x + cos(phase * 5) * 2) * 65536;
	sy = (-vx * y - vy * x + y + sin(phase * 6) * 2) * 65536;

	phase += phase_increment;
	if (phase > 5700000) phase = 0;
}

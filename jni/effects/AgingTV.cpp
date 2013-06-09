/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * AgingTV.cpp :
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
 * AgingTV - film-aging effect.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include "AgingTV.h"

#define  LOG_TAG "AgingTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "AgingTV";
static const char* EFFECT_TITLE = "Aging\nTV";
static const char* FUNC_LIST[] = {
		NULL
};

#define SCRATCH_MAX 20

static const int dx[8] = { 1,  1,  0, -1, -1, -1, 0, 1 };
static const int dy[8] = { 0, -1, -1, -1,  0,  1, 1, 1 };

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void AgingTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	aging_mode    = 0;
	scratch_lines = 0;
	scratches     = NULL;
	area_scale    = 0;
	pits_interval = 0;
	dust_interval = 0;

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
AgingTV::AgingTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
AgingTV::~AgingTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* AgingTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* AgingTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** AgingTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int AgingTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	// Allocate memory & setup
	scratches = new scratch[SCRATCH_MAX];
	if (scratches == NULL) {
		return -1;
	}

	aging_mode_switch();

	return 0;
}

// Finalize
int AgingTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Free memory
	if (scratches != NULL) {
		delete[] scratches;
	}

	//
	return super::stop();
}

// Convert
int AgingTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	coloraging(src_rgb, dst_rgb);
	scratching(dst_rgb);
	pits(dst_rgb);
	if (area_scale > 1) {
		dusts(dst_rgb);
	}

	return 0;
}

// Key functions
int AgingTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	return 0;
}

// Touch action
int AgingTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	return 0;
}

//---------------------------------------------------------------------
// LOCAL METHODs
//---------------------------------------------------------------------
//
void AgingTV::aging_mode_switch(void)
{
	switch(aging_mode) {
	default:
	case 0:
		scratch_lines = 7;
		/* Most of the parameters are tuned for 640x480 mode */
		/* area_scale is set to 10 when screen size is 640x480. */
		area_scale = video_width * video_height / 64 / 480;
	}
	if (area_scale <= 0) {
		area_scale = 1;
	}
}

//
void AgingTV::coloraging(RGB32* src, RGB32* dst)
{
	static int c = 0x18;

	c -= (int)(mUtils->fastrand())>>28;
	if (c < 0) c = 0;
	if (c > 0x18) c = 0x18;
	for (int i=video_area; i>0; i--) {
		RGB32 a = *src++;
		RGB32 b = (a & 0xfcfcfc)>>2;
		*dst++ = a - b + (c|(c<<8)|(c<<16)) + ((mUtils->fastrand()>>8)&0x101010);
	}
}

//
void AgingTV::scratching(RGB32* dst)
{
	const int vw = video_width;
	const int vh = video_height;
	int y1, y2;
	RGB32* p;

	for (int i=0; i<scratch_lines; i++) {
		if (scratches[i].life) {
			scratches[i].x = scratches[i].x + scratches[i].dx;
			if (scratches[i].x < 0 || scratches[i].x > vw*256) {
				scratches[i].life = 0;
				break;
			}
			p = dst + (scratches[i].x>>8);
			if (scratches[i].init) {
				y1 = scratches[i].init;
				scratches[i].init = 0;
			} else {
				y1 = 0;
			}
			scratches[i].life--;
			if (scratches[i].life) {
				y2 = vh;
			} else {
				y2 = mUtils->fastrand() % vh;
			}
			for (int y=y1; y<y2; y++) {
				RGB32 a = (*p & 0xfefeff) + 0x202020;
				RGB32 b = a & 0x1010100;
				*p = a | (b - (b>>8));
				p += vw;
			}
		} else {
			if ((mUtils->fastrand()&0xf0000000) == 0) {
				scratches[i].life = 2 + (mUtils->fastrand()>>27);
				scratches[i].x = mUtils->fastrand() % (vw * 256);
				scratches[i].dx = ((int)mUtils->fastrand())>>23;
				scratches[i].init = (mUtils->fastrand() % (vh-1))+1;
			}
		}
	}
}

//
void AgingTV::pits(RGB32* dst)
{
	const int vw = video_width;
	const int vh = video_height;
	int pnum, pnumscale;

	pnumscale = area_scale * 2;
	if (pits_interval) {
		pnum = pnumscale + (mUtils->fastrand()%pnumscale);
		pits_interval--;
	} else {
		pnum = mUtils->fastrand()%pnumscale;
		if ((mUtils->fastrand()&0xf8000000) == 0) {
			pits_interval = (mUtils->fastrand()>>28) + 20;
		}
	}
	for (int i=0; i<pnum; i++) {
		int x = mUtils->fastrand()%(vw-1);
		int y = mUtils->fastrand()%(vh-1);
		int s = mUtils->fastrand()>>28;
		for (int j=0; j<s; j++) {
			x = x + mUtils->fastrand()%3-1;
			y = y + mUtils->fastrand()%3-1;
			if (x<0 || x>=vw) break;
			if (y<0 || y>=vh) break;
			dst[y*vw + x] = 0xc0c0c0;
		}
	}
}

//
void AgingTV::dusts(RGB32* dst)
{
	const int vw = video_width;
	const int vh = video_height;

	if (dust_interval == 0) {
		if ((mUtils->fastrand()&0xf0000000) == 0) {
			dust_interval = mUtils->fastrand()>>29;
		}
		return;
	}

	int n = area_scale*4 + (mUtils->fastrand()>>27);
	for (int i=0; i<n; i++) {
		int x = mUtils->fastrand()%vw;
		int y = mUtils->fastrand()%vh;
		int d = mUtils->fastrand()>>29;
		int l = mUtils->fastrand()%area_scale + 5;
		for (int j=0; j<l; j++) {
			dst[y*vw + x] = 0x101010;
			y += dy[d];
			x += dx[d];
			if (x<0 || x>=vw) break;
			if (y<0 || y>=vh) break;
			d = (d + mUtils->fastrand()%3 - 1) & 7;
		}
	}
	dust_interval--;
}

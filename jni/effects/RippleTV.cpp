/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * RippleTV.cpp :
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
 * RippleTV - Water ripple effect.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include "RippleTV.h"

#define  LOG_TAG "RippleTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "RippleTV";
static const char* EFFECT_TITLE = "Ripple\nTV";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Motion\ndetection",
		"Rain\ndrop",
		"Threshold\n+",
		"Threshold\n-",
		NULL
};
static const char* MODE_LIST[] = {
		"Motion detection",
		"Rain drop",
};

#define MAX_THRESHOLD 235
#define MIN_THRESHOLD 15 //16
#define DEF_THRESHOLD 70
#define DLT_THRESHOLD 5

static const int point   = 16;
static const int impact  = 2;
static const int decay   = 8;
static const int loopnum = 2;

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void RippleTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	bgIsSet  = 0;
	map_w    = 0;
	map_h    = 0;
	map      = NULL;
	map1     = NULL;
	map2     = NULL;
	map3     = NULL;
	vtable   = NULL;
	sqrtable = NULL;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info = 1;
			mode = 1;
			threshold = DEF_THRESHOLD;
		}
	} else {
		writeConfig();
	}
}

int RippleTV::readConfig()
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
			FREAD_1(fp, mode) &&
			FREAD_1(fp, threshold);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int RippleTV::writeConfig()
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
			FWRITE_1(fp, mode) &&
			FWRITE_1(fp, threshold);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
RippleTV::RippleTV(void)
: show_info(0)
, mode(0)
, threshold(0)
, bgIsSet(0)
, map_w(0)
, map_h(0)
, map(NULL)
, map1(NULL)
, map2(NULL)
, map3(NULL)
, vtable(NULL)
, sqrtable(NULL)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
RippleTV::~RippleTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* RippleTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* RippleTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** RippleTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int RippleTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	map_h = video_height / 2 + 1;
	map_w = video_width  / 2 + 1;

	// Allocate memory & setup
	map      = new int[map_w * map_h * 3];
	vtable   = new signed char[map_w * map_h * 2];
	sqrtable = new int[256];
	if (vtable == NULL || map == NULL || sqrtable == NULL) {
		return -1;
	}
	memset(map,    0, map_w * map_h * 3 * sizeof(int));
	memset(vtable, 0, map_w * map_h * 2 * sizeof(signed char));

	map1 = map;
	map2 = map + map_w * map_h;
	map3 = map + map_w * map_h * 2;

	setTable();

	mUtils->image_set_threshold_yuv_y(threshold);

	return 0;
}

// Finalize
int RippleTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Free memory
	if (vtable != NULL) {
		delete[] vtable;
	}
	if (map != NULL) {
		delete[] map;
	}
	if (sqrtable != NULL) {
		delete[] sqrtable;
	}

	//
	return super::stop();
}

// Convert
int RippleTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	const int mw = map_w;
	const int mh = map_h;
	const int vw = video_width;
	const int vh = video_height;

	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);
	int dx, dy;
	int* p;
	int* q;
	int* r;
	signed char* vp;

	/* impact from the motion or rain drop */
	if (mode) {
		raindrop();
	} else {
		motiondetect(src_yuv);
	}


	/* This function is called only 30 times per second. To increase a speed
	 * of wave, iterates this loop several times. */
	for (int i=loopnum; i>0; i--) {
		/* wave simulation */
		p = map1 + mw + 1;
		q = map2 + mw + 1;
		r = map3 + mw + 1;
		for (int y=mh-2; y>0; y--) {
			for (int x=mw-2; x>0; x--) {
				int h = *(p-mw-1) + *(p-mw+1) + *(p+mw-1) + *(p+mw+1) +
						*(p-mw) + *(p-1) + *(p+1) + *(p+mw) - (*p)*9;
				h = h >> 3;
				int v = *p - *q;
				v += h - (v >> decay);
				*r = v + *p;
				p++;
				q++;
				r++;
			}
			p += 2;
			q += 2;
			r += 2;
		}

		/* low pass filter */
		p = map3 + mw + 1;
		q = map2 + mw + 1;
		for (int y=mh-2; y>0; y--) {
			for (int x=mw-2; x>0; x--) {
				int h = *(p-mw) + *(p+mw) + *(p-1) + *(p+1) + (*p)*60;
				*q = h >> 6;
				p++;
				q++;
			}
			p+=2;
			q+=2;
		}

		p = map1;
		map1 = map2;
		map2 = p;
	}

	vp = vtable;
	p = map1;
	for (int y=mh-1; y>0; y--) {
		for (int x=mw-1; x>0; x--) {
			/* difference of the height between two voxel. They are twiced to
			 * emphasise the wave. */
			vp[0] = sqrtable[((p[0] - p[1] )>>(point-1))&0xff];
			vp[1] = sqrtable[((p[0] - p[mw])>>(point-1))&0xff];
			p++;
			vp+=2;
		}
		p++;
		vp+=2;
	}

	vp = vtable;

	/* draw refracted image. The vector table is stretched. */
	for (int y=0; y<vh; y+=2) {
		for (int x=0; x<vw; x+=2) {
			int h = (int)vp[0];
			int v = (int)vp[1];
			dx = x + h;
			dy = y + v;
			if (dx <  0     ) dx = 0;
			if (dy <  0     ) dy = 0;
			if (dx >= vw ) dx = vw - 1;
			if (dy >= vh) dy = vh - 1;
			dst_rgb[0] = src_rgb[dy * vw + dx];

			int i = dx;

			dx = x + 1 + (h + (int)vp[2]) / 2;
			if (dx <  0    ) dx = 0;
			if (dx >= vw) dx = vw - 1;
			dst_rgb[1] = src_rgb[dy * vw + dx];

			dy = y + 1 + (v + (int)vp[mw * 2 + 1]) / 2;
			if (dy <  0     ) dy = 0;
			if (dy >= vh) dy = vh - 1;
			dst_rgb[vw    ] = src_rgb[dy * vw + i];
			dst_rgb[vw + 1] = src_rgb[dy * vw + dx];
			dst_rgb += 2;
			vp += 2;
		}
		dst_rgb += vw;
		vp += 2;
	}

	if (show_info) {
		sprintf(dst_msg, "Mode: %s, Threshold: %1d (Touch screen to clear)",
				MODE_LIST[mode],
				threshold);
	}

	return 0;
}

// Key functions
const char* RippleTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Motion detection
	case 2: // Rain drop
		mode = key_code - 1;
		break;
	case 3: // Threshold +
		threshold += DLT_THRESHOLD;
		if (threshold > MAX_THRESHOLD) {
			threshold = MAX_THRESHOLD;
		}
		mUtils->image_set_threshold_yuv_y(threshold);
		break;
	case 4: // Threshold -
		threshold -= DLT_THRESHOLD;
		if (threshold < MIN_THRESHOLD) {
			threshold = MIN_THRESHOLD;
		}
		mUtils->image_set_threshold_yuv_y(threshold);
		break;
	}
	return NULL;
}

// Touch action
const char* RippleTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	switch(action) {
	case 0: // Down -> Space
		memset(map, 0, map_h * map_w * 2 * sizeof(int));
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
void RippleTV::setTable(void)
{
	for (int i=0; i<128; i++) {
		sqrtable[i] = i * i;
	}
	for (int i=1; i<=128; i++) {
		sqrtable[256-i] = - i * i;
	}
}

//
int RippleTV::setBackground(YUV* src)
{
	mUtils->image_bgset_yuv_y(src);
	bgIsSet = 1;
	return 0;
}

//
void RippleTV::motiondetect(YUV* src)
{
	const int vw = video_width;
	const int mw = map_w;
	const int mh = map_h;

	if (!bgIsSet) {
		setBackground(src);
	}

	unsigned char* diff = mUtils->image_bgsubtract_update_yuv_y(src);
	int* p = map1 + mw + 1;
	int* q = map2 + mw + 1;
	diff += vw + 2;

	for (int y=mh-2; y>0; y--) {
		for (int x=mw-2; x>0; x--) {
			int h = (int)*diff + (int)*(diff+1) + (int)*(diff+vw) + (int)*(diff+vw+1);
			if (h > 0) {
				*p = h << (point + impact - 8);
				*q = *p;
			}
			p++;
			q++;
			diff += 2;
		}
		diff += vw + 2;
		p+=2;
		q+=2;
	}
}

//
void RippleTV::drop(int power)
{
	int  x = mUtils->fastrand()%(map_w-4)+2;
	int  y = mUtils->fastrand()%(map_h-4)+2;
	int* p = map1 + y*map_w + x;
	int* q = map2 + y*map_w + x;
	*p = power;
	*q = power;
	*(p-map_w) = *(p-1) = *(p+1) = *(p+map_w) = power/2;
	*(p-map_w-1) = *(p-map_w+1) = *(p+map_w-1) = *(p+map_w+1) = power/4;
	*(q-map_w) = *(q-1) = *(q+1) = *(q+map_w) = power/2;
	*(q-map_w-1) = *(q-map_w+1) = *(q+map_w-1) = *(p+map_w+1) = power/4;
}

//
void RippleTV::raindrop(void)
{
	static int period = 0;
	static int rain_stat = 0;
	static unsigned int drop_prob = 0;
	static int drop_prob_increment = 0;
	static int drops_per_frame_max = 0;
	static int drops_per_frame = 0;
	static int drop_power = 0;

	if (period == 0) {
		switch(rain_stat) {
		case 0:
			period = (mUtils->fastrand()>>23) + 100;
			drop_prob = 0;
			drop_prob_increment = 0x00ffffff / period;
			drop_power = (-(mUtils->fastrand()>>28)-2)<<point;
			drops_per_frame_max = 2<<(mUtils->fastrand()>>30); // 2,4,8 or 16
			rain_stat = 1;
			break;
		case 1:
			drop_prob = 0x00ffffff;
			drops_per_frame = 1;
			drop_prob_increment = 1;
			period = (drops_per_frame_max - 1) * 16;
			rain_stat = 2;
			break;
		case 2:
			period = (mUtils->fastrand()>>22) + 1000;
			drop_prob_increment = 0;
			rain_stat = 3;
			break;
		case 3:
			period = (drops_per_frame_max - 1) * 16;
			drop_prob_increment = -1;
			rain_stat = 4;
			break;
		case 4:
			period = (mUtils->fastrand()>>24) + 60;
			drop_prob_increment = -(drop_prob / period);
			rain_stat = 5;
			break;
		case 5:
		default:
			period = (mUtils->fastrand()>>23) + 500;
			drop_prob = 0;
			rain_stat = 0;
			break;
		}
	}
	switch(rain_stat) {
	default:
	case 0:
		break;
	case 1:
	case 5:
		if ((mUtils->fastrand()>>8) < drop_prob) {
			drop(drop_power);
		}
		drop_prob += drop_prob_increment;
		break;
	case 2:
	case 3:
	case 4:
		for (int i=drops_per_frame/16; i>0; i--) {
			drop(drop_power);
		}
		drops_per_frame += drop_prob_increment;
		break;
	}
	period--;
}

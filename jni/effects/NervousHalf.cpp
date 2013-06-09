/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * NervousHalf.cpp :
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
 * NervousHalf - Or your bitter half.
 * Copyright (C) 2002 TANNENBAUM Edo
 * Copyright (C) 2004 Kentaro Fukuchi
 *
 * 2004/11/27
 *  The most of this code has been taken from Edo's NervousTV.
 */

#include "NervousHalf.h"

#define  LOG_TAG "NervousHalf"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "NervousHalf";
static const char* EFFECT_TITLE = "Nervous\nHalf";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Mode",
		"Dir",
		"Mirror",
		"Delay\n+",
		"Delay\n-",
		NULL
};
static const char* nh_mode[] = {
		"Delay",
		"Scratch",
		"Nervous",
};
static const char* nh_dir[] = {
		"Right half",
		"Left half",
		"Bottom half",
		"Upper half",
};
static const char* nh_mirror[] = {
		"Normal",
		"Mirror",
		"Copy",
};

#define PLANES 32

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void NervousHalf::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	plane          = -1;
	scratchTimer   = 0;
	scratchStride  = 0;
	scratchCurrent = 0;
	buffer         = NULL;
	planetable     = NULL;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info = 1;
			mode      = 0;
			dir       = 0;
			mirror    = 1;
			delay     = 10;
		}
	} else {
		writeConfig();
	}
}

int NervousHalf::readConfig()
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
			FREAD_1(fp, dir) &&
			FREAD_1(fp, mirror) &&
			FREAD_1(fp, delay);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int NervousHalf::writeConfig()
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
			FWRITE_1(fp, dir) &&
			FWRITE_1(fp, mirror) &&
			FWRITE_1(fp, delay);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
NervousHalf::NervousHalf(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
NervousHalf::~NervousHalf(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* NervousHalf::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* NervousHalf::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** NervousHalf::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int NervousHalf::start(Utils* utils, int width, int height)
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
int NervousHalf::stop(void)
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
int NervousHalf::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
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

	int readplane;
	switch(mode) {
	default:
	case 0:
		readplane = nextDelay();
		break;
	case 1:
		readplane = nextScratch();
		break;
	case 2:
		readplane = nextNervous();
		break;
	}
	RGB32* buf = planetable[readplane];

	switch(dir) {
	default:
	case 0:
		left(src_rgb, buf, dst_rgb, mirror);
		break;
	case 1:
		right(src_rgb, buf, dst_rgb, mirror);
		break;
	case 2:
		upper(src_rgb, buf, dst_rgb, mirror);
		break;
	case 3:
		bottom(src_rgb, buf, dst_rgb, mirror);
		break;
	}

	plane = (plane + 1) % PLANES;

	if (show_info) {
		if (mode == 0) {
			sprintf(dst_msg, "Mode: %s, Dir: %s, Mirror: %s, Delay: %1d",
					nh_mode[mode],
					nh_dir[dir],
					nh_mirror[mirror],
					delay);
		} else {
			sprintf(dst_msg, "Mode: %s, Dir: %s, Mirror: %s",
					nh_mode[mode],
					nh_dir[dir],
					nh_mirror[mirror]);
		}
	}

	return 0;
}

// Key functions
int NervousHalf::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Mode
		mode = (mode + 1) % 3;
		break;
	case 2: // Dir
		dir = (dir + 1) % 4;
		break;
	case 3: // Mirror
		mirror = (mirror + 1) % 3;
		break;
	case 4: // Delay +
		if (mode == 0) {
			delay++;
			if (delay > PLANES - 1) {
				delay = PLANES - 1;
			}
		}
		break;
	case 5: // Delay -
		if (mode == 0) {
			delay--;
			if (delay < 0) {
				delay = 0;
			}
		}
		break;
	}
	return 0;
}

// Touch action
int NervousHalf::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	return 0;
}

//---------------------------------------------------------------------
// LOCAL METHODs
//---------------------------------------------------------------------
//
int NervousHalf::nextDelay(void)
{
	return (plane - delay + PLANES) % PLANES;
}

//
int NervousHalf::nextScratch(void)
{
	if (scratchTimer) {
		scratchCurrent = scratchCurrent + scratchStride;
		while(scratchCurrent <  0     ) scratchCurrent += PLANES;
		while(scratchCurrent >= PLANES) scratchCurrent -= PLANES;
		scratchTimer--;
	} else {
		scratchCurrent = mUtils->fastrand() % PLANES;
		scratchStride  = mUtils->fastrand() % 5 - 2;
		if (scratchStride >= 0) scratchStride++;
		scratchTimer   = mUtils->fastrand() % 6 + 2;
	}

	return scratchCurrent;
}

//
int NervousHalf::nextNervous(void)
{
	return mUtils->fastrand() % PLANES;
}

//
void NervousHalf::upper(RGB32* src, RGB32* buf, RGB32* dst, int mirror)
{
	int len = video_height / 2 * video_width;
	memcpy(dst, src, len * PIXEL_SIZE);

	switch(mirror) {
	case 1: {
		RGB32* p = buf + len - video_width;
		dst += len;
		len = PIXEL_SIZE * video_width;
		for (int y=video_height/2; y>0; y--) {
			memcpy(dst, p, len);
			p -= video_width;
			dst += video_width;
		}
	} break;
	case 2:
		memcpy(dst + len, buf, len * PIXEL_SIZE);
		break;
	case 0:
	default:
		memcpy(dst + len, buf + len, len * PIXEL_SIZE);
		break;
	}
}

//
void NervousHalf::bottom(RGB32* src, RGB32* buf, RGB32* dst, int mirror)
{
	int len = video_height / 2 * video_width;
	memcpy(dst + len, src + len, len * PIXEL_SIZE);

	RGB32* s2 = buf;
	RGB32* d  = dst;

	switch(mirror) {
	case 1:
		s2 += video_area - video_width;
		len = PIXEL_SIZE * video_width;
		for (int y=video_height/2; y>0; y--) {
			memcpy(d, s2, len);
			d  += video_width;
			s2 -= video_width;
		}
		break;
	case 2:
		memcpy(d, s2 + len, len * PIXEL_SIZE);
		break;
	case 0:
	default:
		memcpy(d, s2, len * PIXEL_SIZE);
		break;
	}
}

//
void NervousHalf::left(RGB32* src, RGB32* buf, RGB32* dst, int mirror)
{
	int len = video_width / 2;
	int st = len * PIXEL_SIZE;

	RGB32* s1 = src;
	RGB32* s2 = buf;
	RGB32* d  = dst;

	switch(mirror) {
	case 1:
		s2 += len;
		for (int y=0; y<video_height; y++) {
			memcpy(d, s1, st);
			RGB32* d1 = d + len;
			for (int x=0; x<len; x++) {
				*d1++ = *s2--;
			}
			d  += video_width;
			s1 += video_width;
			s2 += video_width + len;
		}
		break;
	case 2:
		for (int y=0; y<video_height; y++) {
			memcpy(d, s1, st);
			memcpy(d + len, s2, st);
			d  += video_width;
			s1 += video_width;
			s2 += video_width;
		}
		break;
	case 0:
	default:
		s2 += len;
		for (int y=0; y<video_height; y++) {
			memcpy(d, s1, st);
			memcpy(d + len, s2, st);
			d  += video_width;
			s1 += video_width;
			s2 += video_width;
		}
		break;
	}
}

//
void NervousHalf::right(RGB32* src, RGB32* buf, RGB32* dst, int mirror)
{
	int len = video_width / 2;
	int st = len * PIXEL_SIZE;

	RGB32* s1 = src + len;
	RGB32* s2 = buf;
	RGB32* d  = dst;

	switch(mirror) {
	case 1:
		s2 += video_width - 1;
		for (int y=0; y<video_height; y++) {
			memcpy(d + len, s1, st);
			RGB32* d1 = d;
			for (int x=0; x<len; x++) {
				*d1++ = *s2--;
			}
			d  += video_width;
			s1 += video_width;
			s2 += video_width + len;
		}
		break;
	case 2:
		s2 += len;
		for (int y=0; y<video_height; y++) {
			memcpy(d + len, s1, st);
			memcpy(d, s2, st);
			d  += video_width;
			s1 += video_width;
			s2 += video_width;
		}
		break;
	case 0:
	default:
		for (int y=0; y<video_height; y++) {
			memcpy(d + len, s1, st);
			memcpy(d, s2, st);
			d  += video_width;
			s1 += video_width;
			s2 += video_width;
		}
		break;
	}
}

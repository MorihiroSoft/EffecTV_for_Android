/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * SparkTV.cpp :
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
 * SparkTV - spark effect.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include "SparkTV.h"

#define  LOG_TAG "SparkTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "SparkTV";
static const char* EFFECT_TITLE = "Spark\nTV";
static const char* FUNC_LIST[] = {
		"Show\ninfo",
		"Fore\nground",
		"Light\npart",
		"Dark\npart",
		"Threshold\n+",
		"Threshold\n-",
		NULL
};
static const char* MODE_LIST[] = {
		"Foreground (Touch screen to update BG)",
		"Light part",
		"Dark part",
};

#define SPARK_MAX 10
#define POINT_MAX 100
#define SPARK_BLUE 0x80
#define SPARK_CYAN 0x6080
#define SPARK_WHITE 0x808080
#define MARGINE 20

#define MAX_THRESHOLD 235
#define MIN_THRESHOLD 15 //16
#define DEF_THRESHOLD 40
#define DLT_THRESHOLD 5

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void SparkTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	bgIsSet     = 0;
	sparks_head = 0;
	sparks      = NULL;
	sparks_life = NULL;
	px          = NULL;
	py          = NULL;
	pp          = NULL;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info = 1;
			mode = 0;
			threshold = DEF_THRESHOLD;
		}
	} else {
		writeConfig();
	}
}

int SparkTV::readConfig()
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

int SparkTV::writeConfig()
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
SparkTV::SparkTV(void)
: show_info(0)
, mode(0)
, threshold(0)
, bgIsSet(0)
, sparks_head(0)
, sparks(NULL)
, sparks_life(NULL)
, px(NULL)
, py(NULL)
, pp(NULL)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
SparkTV::~SparkTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* SparkTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* SparkTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** SparkTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int SparkTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	// Allocate memory & setup
	sparks      = new shortvec[SPARK_MAX];
	sparks_life = new int[SPARK_MAX];
	px          = new int[POINT_MAX];
	py          = new int[POINT_MAX];
	pp          = new int[POINT_MAX];
	if (sparks == NULL || sparks_life == NULL || px == NULL || py == NULL || pp == NULL) {
		return -1;
	}

	for (int i=0; i<SPARK_MAX; i++) {
		sparks_life[i] = 0;
	}
	for (int i=0; i<POINT_MAX; i++) {
		pp[i] = 0;
	}

	mUtils->image_set_threshold_yuv_y(threshold);

	return 0;
}

// Finalize
int SparkTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Free memory
	if (sparks != NULL) {
		delete[] sparks;
	}
	if (sparks_life != NULL) {
		delete[] sparks_life;
	}
	if (px != NULL) {
		delete[] px;
	}
	if (py != NULL) {
		delete[] py;
	}
	if (pp != NULL) {
		delete[] pp;
	}

	//
	return super::stop();
}

// Convert
int SparkTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	if (!bgIsSet) {
		setBackground(src_yuv);
	}

	unsigned char* diff;
	switch(mode) {
	default:
	case 0:
		diff = mUtils->image_diff_filter(mUtils->image_bgsubtract_yuv_y(src_yuv));
		break;
	case 1:
		diff = mUtils->image_diff_filter(mUtils->image_yuv_y_over(src_yuv));
		break;
	case 2:
		diff = mUtils->image_diff_filter(mUtils->image_yuv_y_under(src_yuv));
		break;
	}

	memcpy(dst_rgb, src_rgb, video_area * PIXEL_SIZE);

	shortvec sv = detectEdgePoints(diff);
	if ((mUtils->fastrand()&0x10000000) == 0) {
		if (shortvec_length2(sv) > 400) {
			sparks[sparks_head] = sv;
			sparks_life[sparks_head] = (mUtils->fastrand()>>29) + 2;
			sparks_head = (sparks_head+1) % SPARK_MAX;
		}
	}
	for (int i=0; i<SPARK_MAX; i++) {
		if (sparks_life[i]) {
			draw_spark(sparks[i], dst_rgb, video_width, video_height);
			sparks_life[i]--;
		}
	}

	if (show_info) {
		sprintf(dst_msg, "Mode: %s, Threshold: %1d",
				MODE_LIST[mode],
				threshold);
	}

	return 0;
}

// Key functions
const char* SparkTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Foreground
	case 2: // Light part
	case 3: // Dark part
		mode = key_code - 1;
		break;
	case 4: // Threshold +
		threshold += DLT_THRESHOLD;
		if (threshold > MAX_THRESHOLD) {
			threshold = MAX_THRESHOLD;
		}
		mUtils->image_set_threshold_yuv_y(threshold);
		break;
	case 5: // Threshold -
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
const char* SparkTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	switch(action) {
	case 0: // Down -> Space
		bgIsSet = 0;
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
int SparkTV::setBackground(YUV* src)
{
	mUtils->image_bgset_yuv_y(src);
	bgIsSet = 1;
	return 0;
}

//
int SparkTV::shortvec_length2(shortvec sv)
{
	int dx = sv.x2 - sv.x1;
	int dy = sv.y2 - sv.y1;
	return (dx*dx+dy*dy);
}

//
void SparkTV::draw_sparkline_dx(int x, int y, int dx, int dy, RGB32* dst, int width, int height)
{
	RGB32* p   = &dst[y*width+x];
	int    t   = dx;
	int    ady = abs(dy);
	for (int i=0; i<dx; i++) {
		if (y>2 && y<height-2) {
			RGB32 a = (p[-width*2] & 0xfffeff) + SPARK_BLUE;
			RGB32 b = a & 0x100;
			p[-width*2] = a | (b - (b >> 8));

			a = (p[-width] & 0xfefeff) + SPARK_CYAN;
			b = a & 0x10100;
			p[-width] = a | (b - (b >> 8));

			a = (p[0] & 0xfefeff) + SPARK_WHITE;
			b = a & 0x1010100;
			p[0] = a | (b - (b >> 8));

			a = (p[width] & 0xfefeff) + SPARK_CYAN;
			b = a & 0x10100;
			p[width] = a | (b - (b >> 8));

			a = (p[width*2] & 0xfffeff) + SPARK_BLUE;
			b = a & 0x100;
			p[width*2] = a | (b - (b >> 8));
		}
		p++;
		t -= ady;
		if (t < 0) {
			t += dx;
			if (dy < 0) {
				y--;
				p -= width;
			} else {
				y++;
				p += width;
			}
		}
	}
}

//
void SparkTV::draw_sparkline_dy(int x, int y, int dx, int dy, RGB32* dst, int width, int height)
{
	RGB32* p   = &dst[y*width+x];
	int    t   = dy;
	int    adx = abs(dx);
	for (int i=0; i<dy; i++) {
		if (x>2 && x<width-2) {
			RGB32 a = (p[-2] & 0xfffeff) + SPARK_BLUE;
			RGB32 b = a & 0x100;
			p[-2] = a | (b - (b >> 8));

			a = (p[-1] & 0xfefeff) + SPARK_CYAN;
			b = a & 0x10100;
			p[-1] = a | (b - (b >> 8));

			a = (p[0] & 0xfefeff) + SPARK_WHITE;
			b = a & 0x1010100;
			p[0] = a | (b - (b >> 8));

			a = (p[1] & 0xfefeff) + SPARK_CYAN;
			b = a & 0x10100;
			p[1] = a | (b - (b >> 8));

			a = (p[2] & 0xfffeff) + SPARK_BLUE;
			b = a & 0x100;
			p[2] = a | (b - (b >> 8));
		}
		p += width;
		t -= adx;
		if (t < 0) {
			t += dy;
			if (dx < 0) {
				x--;
				p--;
			} else {
				x++;
				p++;
			}
		}
	}
}

//
void SparkTV::draw_sparkline(int x1, int y1, int x2, int y2, RGB32* dst, int width, int height)
{
	int dx = x2 - x1;
	int dy = y2 - y1;

	if (abs(dx) > abs(dy)) {
		if (dx < 0) {
			draw_sparkline_dx(x2, y2, -dx, -dy, dst, width, height);
		} else {
			draw_sparkline_dx(x1, y1, dx, dy, dst, width, height);
		}
	} else {
		if (dy < 0) {
			draw_sparkline_dy(x2, y2, -dx, -dy, dst, width, height);
		} else {
			draw_sparkline_dy(x1, y1, dx, dy, dst, width, height);
		}
	}
}

//
void SparkTV::break_line(int a, int b, int width, int height)
{
	int dx = px[b] - px[a];
	int dy = py[b] - py[a];
	if ((dx*dx+dy*dy)<100 || (b-a)<3) {
		pp[a] = b;
		return;
	}

	int len = (abs(dx)+abs(dy))/4;
	int x = px[a] + dx/2 - len/2 + len*(int)((mUtils->fastrand())&255)/256;
	int y = py[a] + dy/2 - len/2 + len*(int)((mUtils->fastrand())&255)/256;
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x >= width) x = width - 1;
	if (y >= height) y = height - 1;
	int c = (a+b)/2;
	px[c] = x;
	py[c] = y;
	break_line(a, c, width, height);
	break_line(c, b, width, height);
}

//
void SparkTV::draw_spark(shortvec sv, RGB32* dst, int width, int height)
{
	px[0] = sv.x1;
	py[0] = sv.y1;
	px[POINT_MAX-1] = sv.x2;
	py[POINT_MAX-1] = sv.y2;
	break_line(0, POINT_MAX-1, width, height);
	for (int i=0; pp[i]>0; i=pp[i]) {
		draw_sparkline(px[i], py[i], px[pp[i]], py[pp[i]], dst, width, height);
	}
}

//
SparkTV::shortvec SparkTV::scanline_dx(int dir, int y1, int y2, unsigned char* diff)
{
	const int width = video_width;
	const int dy = 256 * (y2 - y1) / width;
	int x = (dir==1 ? 0 : width - 1);
	int y = y1 * 256;

	int start = 0;
	shortvec sv;
	for (int i=0; i<width; i++) {
		if (start == 0) {
			if (diff[(y>>8)*width+x]) {
				sv.x1 = x;
				sv.y1 = y>>8;
				start = 1;
			}
		} else {
			if (diff[(y>>8)*width+x] == 0) {
				sv.x2 = x;
				sv.y2 = y>>8;
				start = 2;
				break;
			}
		}
		y += dy;
		x += dir;
	}
	if (start == 0) {
		sv.x1 = sv.x2 = sv.y1 = sv.y2 = 0;
	}
	if (start == 1) {
		sv.x2 = x - dir;
		sv.y2 = (y - dy)>>8;
	}
	return sv;
}

//
SparkTV::shortvec SparkTV::scanline_dy(int dir, int x1, int x2, unsigned char* diff)
{
	const int width  = video_width;
	const int height = video_height;
	const int dx = 256 * (x2 - x1) / height;
	int x = x1 * 256;
	int y = (dir==1 ? 0 : height - 1);

	int start = 0;
	struct shortvec sv;
	for (int i=0; i<height; i++) {
		if (start == 0) {
			if (diff[y*width+(x>>8)]) {
				sv.x1 = x>>8;
				sv.y1 = y;
				start = 1;
			}
		} else {
			if (diff[y*width+(x>>8)] == 0) {
				sv.x2 = x>>8;
				sv.y2 = y;
				start = 2;
				break;
			}
		}
		x += dx;
		y += dir;
	}
	if (start == 0) {
		sv.x1 = sv.x2 = sv.y1 = sv.y2 = 0;
	}
	if (start == 1) {
		sv.x2 = (x - dx)>>8;
		sv.y2 = y - dir;
	}
	return sv;
}

//
SparkTV::shortvec SparkTV::detectEdgePoints(unsigned char* diff)
{
	int p1, p2;
	switch(mUtils->fastrand()>>30) {
	case 0:
		p1 = mUtils->fastrand()%(video_width - MARGINE*2);
		p2 = mUtils->fastrand()%(video_width - MARGINE*2);
		return scanline_dy(1, p1, p2, diff);
	case 1:
		p1 = mUtils->fastrand()%(video_width - MARGINE*2);
		p2 = mUtils->fastrand()%(video_width - MARGINE*2);
		return scanline_dy(-1, p1, p2, diff);
	case 2:
		p1 = mUtils->fastrand()%(video_height - MARGINE*2);
		p2 = mUtils->fastrand()%(video_height - MARGINE*2);
		return scanline_dx(1, p1, p2, diff);
	default:
	case 3:
		p1 = mUtils->fastrand()%(video_height - MARGINE*2);
		p2 = mUtils->fastrand()%(video_height - MARGINE*2);
		return scanline_dx(-1, p1, p2, diff);
	}
}

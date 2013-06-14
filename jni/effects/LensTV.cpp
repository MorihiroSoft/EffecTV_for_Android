/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * LensTV.cpp :
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
 * lensTV - old skool Demo lens Effect
 * Code taken from "The Demo Effects Colletion" 0.0.4
 * http://www.paassen.tmfweb.nl/retrodemo.html
 *
 *
 * Ported to EffecTV BSB by Buddy Smith
 * Modified from BSB for EffecTV 0.3.x by Ed Tannenbaaum
 * ET added interactive control via mouse as follows....
 * Spacebar toggles interactive mode (off by default)
 * In interactive mode:
 *   Mouse with no buttons pressed moves magnifier
 *   Left button and y movement controls size of magnifier
 *   Right Button and y movement controls magnification.
 *
 * This works best in Fullscreen mode due to mouse trapping
 *
 * You can now read the fine print in the TV advertisements!
 */

#include "LensTV.h"

#define  LOG_TAG "LensTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "LensTV";
static const char* EFFECT_TITLE = "LensTV";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Auto\nmove",
		"Size\n+",
		"Size\n-",
		"Curvature\nR +",
		"Curvature\nR -",
		"Init.",
		NULL
};

#define MAX_LENS_SIZE (video_height)
#define MIN_LENS_SIZE 5
#define DEF_LENS_SIZE 150

#define MAX_LENS_CRVR 200
#define MIN_LENS_CRVR 5
#define DEF_LENS_CRVR 30

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void LensTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	lens_x = 0;
	lens_y = 0;
	lens   = NULL;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info = 1;
			mode      = 1;
			lens_xd   = 5;
			lens_yd   = 5;
			lens_size = DEF_LENS_SIZE;
			lens_crvr = DEF_LENS_CRVR;
		}
	} else {
		writeConfig();
	}
}

int LensTV::readConfig()
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
			FREAD_1(fp, lens_xd) &&
			FREAD_1(fp, lens_yd) &&
			FREAD_1(fp, lens_size) &&
			FREAD_1(fp, lens_crvr);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int LensTV::writeConfig()
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
			FWRITE_1(fp, lens_xd) &&
			FWRITE_1(fp, lens_yd) &&
			FWRITE_1(fp, lens_size) &&
			FWRITE_1(fp, lens_crvr);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
LensTV::LensTV(void)
: show_info(0)
, mode(0)
, lens_x(0)
, lens_y(0)
, lens_xd(0)
, lens_yd(0)
, lens_size(0)
, lens_crvr(0)
, lens(NULL)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
LensTV::~LensTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* LensTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* LensTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** LensTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int LensTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	// Allocate memory & setup
	if (init() != 0) {
		return -1;
	}

	lens_x = video_width  / 2;
	lens_y = video_height / 2;

	return 0;
}

// Finalize
int LensTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Free memory
	if (lens != NULL) {
		delete[] lens;
	}

	//
	return super::stop();
}

// Convert
int LensTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	memcpy(dst_rgb, src_rgb, video_area * PIXEL_SIZE);

	apply_lens(lens_x, lens_y, src_rgb, dst_rgb);

	// Auto move
	if (mode == 1) {
		lens_x += lens_xd;
		lens_y += lens_yd;

		if (lens_x + lens_xd < - lens_size / 2 + 1 ||
				lens_x + lens_xd >= video_width - lens_size / 2 - 1) {
			lens_xd = -lens_xd;
		}
		if (lens_y + lens_yd < - lens_size / 2 + 1 ||
				lens_y + lens_yd >= video_height - lens_size / 2 - 1) {
			lens_yd = -lens_yd;
		}
	}

	if (show_info) {
		sprintf(dst_msg, "Mode: %1d, Size: %1d, Curv: %1d",
				mode,
				lens_size,
				lens_crvr);
	}

	return 0;
}

// Key functions
int LensTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Auto move
		mode = 1 - mode;
		break;
	case 2: // Size +
		lens_size += 5;
		if (lens_size > MAX_LENS_SIZE) lens_size = MAX_LENS_SIZE;
		init();
		clipmag();
		break;
	case 3: // Size -
		lens_size -= 5;
		if (lens_size < MIN_LENS_SIZE) lens_size = MIN_LENS_SIZE;
		init();
		clipmag();
		break;
	case 4: // Curvature R(radius) +
		lens_crvr += 5;
		if (lens_crvr > MAX_LENS_CRVR) lens_crvr = MAX_LENS_CRVR;
		init();
		break;
	case 5: // Curvature R(radius) -
		lens_crvr -= 5;
		if (lens_crvr < MIN_LENS_CRVR) lens_crvr = MIN_LENS_CRVR;
		init();
		break;
	case 6: // Init.
		mode      = 1;
		lens_xd   = 5;
		lens_yd   = 5;
		lens_size = DEF_LENS_SIZE;
		lens_crvr = DEF_LENS_CRVR;
		init();
		clipmag();
		break;
	}
	return 0;
}

// Touch action
int LensTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	switch(action) {
	case 0: // Down -> Space
	case 1: // Move
		mode = 0;
		lens_x = x - lens_size / 2;
		lens_y = y - lens_size / 2;
		clipmag();
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
int LensTV::init(void)
{
	// Realloc memory & clear
	if (lens != NULL) {
		delete[] lens;
	}
	lens = new int[lens_size * lens_size];
	if (lens == NULL) {
		return -1;
	}
	memset(lens, 0, lens_size * lens_size * sizeof(int));

	/* generate the lens distortion */
	int r = lens_size / 2;
	int d = lens_crvr;

	/* the shift in the following expression is a function of the
	 * distance of the current point from the center of the sphere.
	 * If you imagine:
	 *
	 *       eye
	 *
	 *   .-~~~~~~~-.    sphere surface
	 * .`           '.
	 * ---------------  viewing plane
	 *        .         center of sphere
	 *
	 * For each point across the viewing plane, draw a line from the
	 * point on the sphere directly above that point to the center of
	 * the sphere.  It will intersect the viewing plane somewhere
	 * closer to the center of the sphere than the original point.
	 * The shift function below is the end result of the above math,
	 * given that the height of the point on the sphere can be derived
	 * from:
	 *
	 * x^2 + y^2 + z^2 = radius^2
	 *
	 * x and y are known, z is based on the height of the viewing
	 * plane.
	 *
	 * The radius of the sphere is the distance from the center of the
	 * sphere to the edge of the viewing plane, which is a neat little
	 * triangle.  If d = the distance from the center of the sphere to
	 * the center of the plane (aka, lens_zoom) and r = half the width
	 * of the plane (aka, lens_width/2) then radius^2 = d^2 + r^2.
	 *
	 * Things become simpler if we take z=0 to be at the plane's
	 * height rather than the center of the sphere, turning the z^2 in
	 * the expression above to (z+d)^2, since the center is now at
	 * (0, 0, -d).
	 *
	 * So, the resulting function looks like:
	 *
	 * x^2 + y^2 + (z+d)^2 = d^2 + r^2
	 *
	 * Expand the (z-d)^2:
	 *
	 * x^2 + y^2 + z^2 + 2dz + d^2 = d^2 + r^2
	 *
	 * Rearrange things to be a quadratic in terms of z:
	 *
	 * z^2 + 2dz + x^2 + y^2 - r^2 = 0
	 *
	 * Note that x, y, and r are constants, so apply the quadratic
	 * formula:
	 *
	 * For ax^2 + bx + c = 0,
	 *
	 * x = (-b +- sqrt(b^2 - 4ac)) / 2a
	 *
	 * We can ignore the negative result, because we want the point at
	 * the top of the sphere, not at the bottom.
	 *
	 * x = (-2d + sqrt(4d^2 - 4 * (x^2 + y^2 - r^2))) / 2
	 *
	 * Note that you can take the -4 out of both expressions in the
	 * square root to put -2 outside, which then cancels out the
	 * division:
	 *
	 * z = -d + sqrt(d^2 - (x^2 + y^2 - r^2))
	 *
	 * This now gives us the height of the point on the sphere
	 * directly above the equivalent point on the plane.  Next we need
	 * to find where the line between this point and the center of the
	 * sphere at (0, 0, -d) intersects the viewing plane at (?, ?, 0).
	 * This is a matter of the ratio of line below the plane vs the
	 * total line length, multiplied by the (x,y) coordinates.  This
	 * ratio can be worked out by the height of the line fragment
	 * below the plane, which is d, and the total height of the line,
	 * which is d + z, or the height above the plane of the sphere
	 * surface plus the height of the plane above the center of the
	 * sphere.
	 *
	 * ratio = d/(d + z)
	 *
	 * Subsitute in the formula for z:
	 *
	 * ratio = d/(d + -d + sqrt(d^2 - (x^2 + y^2 - r^2))
	 *
	 * Simplify to:
	 *
	 * ratio = d/sqrt(d^2 - (x^2 + y^2 - r^2))
	 *
	 * Since d and r are constant, we now have a formula we can apply
	 * for each (x,y) point within the sphere to give the (x',y')
	 * coordinates of the point we should draw to project the image on
	 * the plane to the surface of the sphere.  I subtract the
	 * original (x,y) coordinates to give an offset rather than an
	 * absolute coordinate, then convert that offset to the image
	 * dimensions, and store the offset in a matrix the size of the
	 * intersecting circle.  Drawing the lens is then a matter of:
	 *
	 * screen[coordinate] = image[coordinate + lens[y][x]]
	 *
	 */

	/* it is sufficient to generate 1/4 of the lens and reflect this
	 * around; a sphere is mirrored on both the x and y axes */
	for (int y=0; y<r; y++) {
		for (int x=0; x<r; x++) {
			int ix, iy, offset, dist;
			dist = x * x + y * y - r * r;
			if (dist < 0) {
				double shift = d / sqrt(d * d - dist);
				ix = x * shift - x;
				iy = y * shift - y;
			} else {
				ix = 0;
				iy = 0;
			}
			offset = (iy * video_width + ix);
			lens[(r - y) * lens_size + r - x] = -offset;
			lens[(r + y) * lens_size + r + x] = offset;
			offset = (-iy * video_width + ix);
			lens[(r + y) * lens_size + r - x] = -offset;
			lens[(r - y) * lens_size + r + x] = offset;
		}
	}

	return 0;
}

//
void LensTV::clipmag(void)
{
	if (lens_x < - lens_size / 2 + 1) {
		lens_x = - lens_size / 2 + 1;
	}
	if (lens_x >= video_width - lens_size / 2 - 1) {
		lens_x = video_width - lens_size / 2 - 1;
	}

	if (lens_y < - lens_size / 2 + 1) {
		lens_y = - lens_size / 2 + 1;
	}
	if (lens_y >= video_height - lens_size / 2 - 1) {
		lens_y = video_height - lens_size / 2 - 1;
	}
}

//
void LensTV::apply_lens(int ox, int oy, RGB32* src, RGB32* dst)
{
	int x, y, noy, pos, nox;
	int* p;

	p = lens;
	for (y=0; y<lens_size; y++) {
		for (x=0; x<lens_size; x++) {
			nox = (x + ox);
			noy = (y + oy);
			if (nox >= 0 && noy >= 0 && nox < video_width && noy < video_height) {
				pos = (noy * video_width) + nox;
				dst[pos] = src[pos + *p];
			}
			p++;
		}
	}
}

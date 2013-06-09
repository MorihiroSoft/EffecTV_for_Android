/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * Settings.java : Application settings
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

package jp.effectv.android;

import java.io.File;
import java.util.Set;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.hardware.Camera.Size;
import android.os.Environment;
import android.util.Log;

public class Settings {
	private static final boolean DEBUG = false;
	private static final String TAG = "Settings";

	//---------------------------------------------------------------------
	// CONSTANTS
	//---------------------------------------------------------------------
	public static final String PREFS_NAME = "effectv";

	public final String WORD_LOW;
	public final String WORD_HIGH;
	public final String WORD_STANDARD;
	public final String WORD_CUSTOM;

	public final String KEY_EFFECT_HIDE_LIST;
	public final String KEY_CAMERA_ID;
	public final String KEY_CAMERA_SIZE; // [High/Low]
	public final String KEY_CAMERA_FPS;  // [High/Low]
	public final String KEY_PHOTO_FMT;   // [jpg/png]
	public final String KEY_VIDEO_MIME;  // ["video/avc"]
	public final String KEY_VIDEO_BPS;   // [bps]
	public final String KEY_VIDEO_IFI;   // [sec.]
	public final String KEY_SAVE_PATH;   // [Standard/Custom]
	public final String KEY_SAVE_PATH_CUSTOM;

	private final int    DEF_CAMERA_ID;
	private final String DEF_CAMERA_SIZE;
	private final String DEF_CAMERA_FPS;
	private final String DEF_PHOTO_FMT;
	private final String DEF_VIDEO_MIME;
	private final String DEF_VIDEO_BPS;
	private final String DEF_VIDEO_IFI;
	private final String DEF_SAVE_PATH;

	private static final int DEF_CAMERA_SIZE_L_W = 320; // [pix.]
	private static final int DEF_CAMERA_SIZE_L_H = 240; // [pix.]
	private static final int DEF_CAMERA_SIZE_H_W = 640; // [pix.]
	private static final int DEF_CAMERA_SIZE_H_H = 480; // [pix.]
	private static final int DEF_CAMERA_FPS_L    = 15;  // [fps]
	private static final int DEF_CAMERA_FPS_H    = 30;  // [fps]

	// Camera
	private static class CameraParam {
		public boolean mFacingFront;
		public Size    mSizeLow;
		public Size    mSizeHigh;
		public int[]   mFpsRangeLow;
		public int[]   mFpsRangeHigh;
	};
	private static CameraParam[] mCameraParams = null;

	//---------------------------------------------------------------------
	// MEMBERS
	//---------------------------------------------------------------------
	private SharedPreferences mPrefs = null;

	private String[] mEffectNames   = null;
	private String[] mEffectTitles  = null;
	private int[]    mEffectIndexs  = null;
	private int      mEffectShowCnt = 0;

	//---------------------------------------------------------------------
	// PUBLIC METHODS
	//---------------------------------------------------------------------
	public Settings(Context c) {
		if (DEBUG) Log.d(TAG, "Settings()");
		mPrefs = c.getSharedPreferences(PREFS_NAME, 0);

		Resources res = c.getResources();

		WORD_LOW             = res.getString(R.string.settings_word_low);
		WORD_HIGH            = res.getString(R.string.settings_word_high);
		WORD_STANDARD        = res.getString(R.string.settings_word_standard);
		WORD_CUSTOM          = res.getString(R.string.settings_word_custom);

		KEY_EFFECT_HIDE_LIST = res.getString(R.string.settings_effect_hide_list_key);
		KEY_CAMERA_ID        = res.getString(R.string.settings_camera_id_key);
		KEY_CAMERA_SIZE      = res.getString(R.string.settings_camera_size_key);
		KEY_CAMERA_FPS       = res.getString(R.string.settings_camera_fps_key);
		KEY_PHOTO_FMT        = res.getString(R.string.settings_photo_fmt_key);
		KEY_VIDEO_MIME       = res.getString(R.string.settings_video_mime_key);
		KEY_VIDEO_BPS        = res.getString(R.string.settings_video_bps_key);
		KEY_VIDEO_IFI        = res.getString(R.string.settings_video_ifi_key);
		KEY_SAVE_PATH        = res.getString(R.string.settings_save_path_key);
		KEY_SAVE_PATH_CUSTOM = res.getString(R.string.settings_save_path_custom_key);

		DEF_CAMERA_ID        = 0;
		DEF_CAMERA_SIZE      = res.getStringArray(R.array.settings_camera_size_values)[1];
		DEF_CAMERA_FPS       = res.getStringArray(R.array.settings_camera_fps_values)[0];
		DEF_PHOTO_FMT        = res.getStringArray(R.array.settings_photo_fmt_values)[0];
		DEF_VIDEO_MIME       = res.getStringArray(R.array.settings_video_mime_values)[0];
		DEF_VIDEO_BPS        = res.getStringArray(R.array.settings_video_bps_values)[3];
		DEF_VIDEO_IFI        = res.getStringArray(R.array.settings_video_ifi_values)[0];
		DEF_SAVE_PATH        = res.getStringArray(R.array.settings_save_path_values)[0];

		initCameraParams();

		// Effects
		mEffectNames  = MainApplication.native_names();
		mEffectTitles = MainApplication.native_titles();
		mEffectIndexs = new int[mEffectNames.length];

		//
		updateSettings();
	}

	public void registerOnSharedPreferenceChangeListener(
			SharedPreferences.OnSharedPreferenceChangeListener listener) {
		if (DEBUG) Log.d(TAG, "registerOnSharedPreferenceChangeListener()");
		if (listener != null) {
			mPrefs.registerOnSharedPreferenceChangeListener(listener);
		}
	}

	public void unregisterOnSharedPreferenceChangeListener(
			SharedPreferences.OnSharedPreferenceChangeListener listener) {
		if (DEBUG) Log.d(TAG, "unregisterOnSharedPreferenceChangeListener()");
		if (listener != null) {
			mPrefs.unregisterOnSharedPreferenceChangeListener(listener);
		}
	}

	public void updateSettings() {
		final int effect_num = mEffectNames.length;
		final Set<String> effect_hide_list = getEffectHideList();
		if (effect_hide_list == null) {
			mEffectShowCnt = effect_num;
			for (int i=0; i<effect_num; i++) {
				mEffectIndexs[i] = i;
			}
		} else {
			mEffectShowCnt = 0;
			for (int i=0; i<effect_num; i++) {
				boolean show = true;
				for (final String s : effect_hide_list) {
					if (s.equals(mEffectNames[i])) {
						show = false;
						break;
					}
				}
				if (show) {
					mEffectIndexs[mEffectShowCnt++] = i;
				}
			}
		}
	}

	//---------------------------------------------------------------------
	// GETTER
	//---------------------------------------------------------------------
	/** getter: Effect hide list. */
	public final Set<String> getEffectHideList() {
		return mPrefs.getStringSet(KEY_EFFECT_HIDE_LIST, null);
	}

	/** getter: Camera Id. [front/back] */
	public int getCameraId() {
		return mPrefs.getInt(KEY_CAMERA_ID, DEF_CAMERA_ID);
	}

	/** getter: Camera preview(Video encoder) size. [Low/High] */
	public final String getCameraSize() {
		return mPrefs.getString(KEY_CAMERA_SIZE, DEF_CAMERA_SIZE);
	}

	/** getter: Preview(Video encoder) frame rate. [Low/High] */
	public final String getCameraFps() {
		return mPrefs.getString(KEY_CAMERA_FPS, DEF_CAMERA_FPS);
	}

	/** getter: Photo save format. [jpg/png] */
	public final String getPhotoFmt() {
		return mPrefs.getString(KEY_PHOTO_FMT, DEF_PHOTO_FMT);
	}

	/** getter: Video encoder MIME type. */
	public final String getVideoMime() {
		return mPrefs.getString(KEY_VIDEO_MIME, DEF_VIDEO_MIME);
	}

	/** getter: Video encoder bit rate. */
	public final String getVideoBps() {
		return mPrefs.getString(KEY_VIDEO_BPS, DEF_VIDEO_BPS);
	}

	/** getter: Video encoder I-frame interval. */
	public final String getVideoIfi() {
		return mPrefs.getString(KEY_VIDEO_IFI, DEF_VIDEO_IFI);
	}

	/** getter: Save path. [Standard/Custom] */
	public final String getSavePath() {
		return mPrefs.getString(KEY_SAVE_PATH, DEF_SAVE_PATH);
	}

	/** getter: Save path. - Custom */
	public final String getSavePathCustom() {
		return mPrefs.getString(KEY_SAVE_PATH_CUSTOM, getSavePathStandard());
	}

	//---------------------------------------------------------------------
	// SETTER
	//---------------------------------------------------------------------
	/** setter: Effect hide list. */
	public void setEffectHideList(Set<String> val) {
		mPrefs.edit().putStringSet(KEY_EFFECT_HIDE_LIST, val).commit();
	}

	/** setter: Camera Id. [front/back] */
	public void setCameraId(int val) {
		mPrefs.edit().putInt(KEY_CAMERA_ID, val).commit();
	}

	/** setter: Camera preview(Video encoder) size. [Low/High] */
	public void setCameraSize(final String val) {
		mPrefs.edit().putString(KEY_CAMERA_SIZE, val).commit();
	}

	/** setter: Preview(Video encoder) frame rate. [Low/High] */
	public void setCameraFps(final String val) {
		mPrefs.edit().putString(KEY_CAMERA_FPS, val).commit();
	}

	/** setter: Photo save format. [jpg/png] */
	public void setPhotoFmt(final String val) {
		mPrefs.edit().putString(KEY_PHOTO_FMT, val).commit();
	}

	/** setter: Video encoder MIME type. */
	public void getVideoMime(final String val) {
		mPrefs.edit().putString(KEY_VIDEO_MIME, val).commit();
	}

	/** setter: Video encoder bit rate. */
	public void setVideoBps(final String val) {
		mPrefs.edit().putString(KEY_VIDEO_BPS, val).commit();
	}

	/** setter: Video encoder I-frame interval. */
	public void setVideoIfi(final String val) {
		mPrefs.edit().putString(KEY_VIDEO_IFI, val).commit();
	}

	/** setter: Save path. [Standard/Custom] */
	public void setSavePath(final String val) {
		mPrefs.edit().putString(KEY_SAVE_PATH, val).commit();
	}

	/** setter: Save path. - Custom */
	public void setSavePathCustom(final String val) {
		mPrefs.edit().putString(KEY_SAVE_PATH_CUSTOM, val).commit();
	}

	//---------------------------------------------------------------------
	// UTIL.
	//---------------------------------------------------------------------
	/** toggle: Camera Id. [front/back] */
	public boolean toggleCameraId() {
		if (mCameraParams.length < 2) {
			return false;
		}
		setCameraId(getCameraId()==0 ? 1 : 0);
		return true;
	}

	/** getter: Camera facing. [true=front, false=back] */
	public boolean getCameraFacingFront() {
		return mCameraParams[getCameraId()].mFacingFront;
	}

	/** getter: Camera preview(Video encoder) width. */
	public int getCameraW() {
		final String val = getCameraSize();
		if (WORD_LOW.equals(val)) {
			return mCameraParams[getCameraId()].mSizeLow.width;
		} else {
			return mCameraParams[getCameraId()].mSizeHigh.width;
		}
	}

	/** getter: Camera preview(Video encoder) height. */
	public int getCameraH() {
		final String val = getCameraSize();
		if (WORD_LOW.equals(val)) {
			return mCameraParams[getCameraId()].mSizeLow.height;
		} else {
			return mCameraParams[getCameraId()].mSizeHigh.height;
		}
	}

	/** getter: Camera preview frame rate range. [x1000] */
	public int[] getCameraFpsByRange() {
		final String val = getCameraFps();
		if (WORD_LOW.equals(val)) {
			return mCameraParams[getCameraId()].mFpsRangeLow;
		} else {
			return mCameraParams[getCameraId()].mFpsRangeHigh;
		}
	}

	/** getter(int): Video encoder frame rate range. [x1] */
	public int getVideoFpsAsInt() {
		final String val = getCameraFps();
		if (WORD_LOW.equals(val)) {
			return DEF_CAMERA_FPS_L;
		} else {
			return DEF_CAMERA_FPS_H;
		}
	}

	/** getter(type): Photo save format. */
	public Bitmap.CompressFormat getPhotoFmtAsType() {
		final String val = getCameraFps();
		if ("jpg".equals(val)) {
			return Bitmap.CompressFormat.JPEG;
		} else {
			return Bitmap.CompressFormat.PNG;
		}
	}

	/** getter(int): Video encoder bit rate. */
	public int getVideoBpsAsInt() {
		return Integer.valueOf(getVideoBps());
	}

	/** getter(int): Video encoder bit rate. */
	public int getVideoIfiAsInt() {
		return Integer.valueOf(getVideoIfi());
	}

	/** getter: Save path. - Standard */
	public final String getSavePathStandard() {
		return Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM).getPath()+"/Camera";
	}

	/** getter(File): Save path. */
	public File getSavePathAsFile() {
		final String val = getSavePath();
		File f;
		if (WORD_STANDARD.equals(val)) {
			f = new File(getSavePathCustom());
			if (f.exists() || f.mkdirs()) {
				return f;
			}
			Log.e(TAG, "Cannot access path: "+f.getAbsolutePath());
			Log.e(TAG, "Use standard path.");
			setSavePath(WORD_CUSTOM);
		}
		f = new File(getSavePathStandard());
		if (f.exists() || f.mkdirs()) {
			return f;
		}
		throw new RuntimeException(f.getAbsolutePath());
	}

	/** getter: Effect name list. */
	public final String[] getEffectNames() {
		return mEffectNames;
	}

	/** getter: Effect "SHOW" index list. */
	public final int[] getEffectIndexs() {
		return mEffectIndexs;
	}

	/** getter: Effect "SHOW" title list. */
	public final String[] getEffectShowTitles() {
		if (mEffectShowCnt < 1) {
			return null;
		} else {
			final String[] list = new String[mEffectShowCnt];
			for (int i=0; i<mEffectShowCnt; i++) {
				list[i] = mEffectTitles[mEffectIndexs[i]];
			}
			return list;
		}
	}

	//---------------------------------------------------------------------
	// PRIVATE METHODS
	//---------------------------------------------------------------------
	private static void initCameraParams() {
		if (DEBUG) Log.d(TAG, "initCameraParams()");
		int num = Camera.getNumberOfCameras();
		if (num < 1) {
			throw new UnsupportedOperationException("No camera");
		}

		mCameraParams = new CameraParam[num];

		for (int i=0; i<num; i++) {
			mCameraParams[i] = new CameraParam();

			Camera.Parameters cp = null;
			try {
				Camera c = Camera.open(i);
				cp = c.getParameters();
				c.release();
			} catch (Exception e) {
				e.printStackTrace();
				throw new RuntimeException();
			}

			final Camera.CameraInfo ci = new Camera.CameraInfo();
			Camera.getCameraInfo(i, ci);

			// Facing
			mCameraParams[i].mFacingFront =
					(ci.facing == Camera.CameraInfo.CAMERA_FACING_FRONT);
			Log.v(TAG, "FrontFacing: "+mCameraParams[i].mFacingFront);

			// Size
			Camera.Size sl = null;
			Camera.Size sh = null;
			for (Camera.Size s : cp.getSupportedPreviewSizes()) {
				Log.v(TAG, "SupooirtedPreviewSize: "+s.width+", "+s.height);
				if (s.width <= DEF_CAMERA_SIZE_L_W &&
						s.height <= DEF_CAMERA_SIZE_L_H) {
					if (sl == null || sl.width < s.width) {
						sl = s;
					}
				}
				if (s.width <= DEF_CAMERA_SIZE_H_W &&
						s.height <= DEF_CAMERA_SIZE_H_H) {
					if (sh == null || sh.width < s.width) {
						sh = s;
					}
				}
			}
			mCameraParams[i].mSizeLow  = sl;
			mCameraParams[i].mSizeHigh = sh;
			Log.v(TAG, "SizeLow : "+sl.width+", "+sl.height);
			Log.v(TAG, "SizeHigh: "+sh.width+", "+sh.height);

			// Frame rate
			final int min = Camera.Parameters.PREVIEW_FPS_MIN_INDEX;
			final int max = Camera.Parameters.PREVIEW_FPS_MAX_INDEX;
			int[] fr_l = null;
			int[] fr_h = null;
			for (int[] s : cp.getSupportedPreviewFpsRange()) {
				Log.v(TAG, "SupportedPreviewFpsRange: "+s[min]+", "+s[max]);
				if (fr_l == null) {
					fr_l = s;
					fr_h = s;
				} else {
					if (s[max] == fr_l[max] && s[min] < fr_l[min]) {
						fr_l = s;
					} else if (s[max] > DEF_CAMERA_FPS_L*1000) {
						if (s[max] < fr_l[max]) {
							fr_l = s;
						}
					} else {
						if (fr_l[max] >= DEF_CAMERA_FPS_L*1000 || s[max] > fr_l[max]) {
							fr_l = s;
						}
					}
					if (s[max] == fr_h[max] && s[min] < fr_h[min]) {
						fr_h = s;
					} else if (s[max] > DEF_CAMERA_FPS_H*1000) {
						if (s[max] < fr_h[max]) {
							fr_h = s;
						}
					} else {
						if (fr_h[max] >= DEF_CAMERA_FPS_H*1000 || s[max] > fr_h[max]) {
							fr_h = s;
						}
					}
				}
			}
			mCameraParams[i].mFpsRangeLow  = fr_l;
			mCameraParams[i].mFpsRangeHigh = fr_h;
			Log.v(TAG, "FpsRangeLow : "+fr_l[min]+", "+fr_l[max]);
			Log.v(TAG, "FpsRangeHigh: "+fr_h[min]+", "+fr_h[max]);

			// Format (check only)
			final int fmt = cp.getPreviewFormat();
			switch (fmt) {
			case ImageFormat.NV21:
				Log.v(TAG, "ImageFormat: NV21");
				break;
			case ImageFormat.YUY2:
				throw new UnsupportedOperationException(
						"Not supported: ImageFormat=YUY2");
			case ImageFormat.YV12:
				throw new UnsupportedOperationException(
						"Not supported: ImageFormat=YV12");
			default:
				throw new UnsupportedOperationException(
						"Not supported: ImageFormat=???("+fmt+")");
			}

			// Auto/Lock support (check only)
			final boolean ae_lock = cp.isAutoExposureLockSupported();
			final boolean wb_lock = cp.isAutoWhiteBalanceLockSupported();
			Log.v(TAG, "AutoExposureLock: "+ae_lock);
			Log.v(TAG, "AutoWhiteBalanceLock: "+wb_lock);
		}
	}
}

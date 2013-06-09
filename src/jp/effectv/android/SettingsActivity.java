/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * SettingsActivity.java :
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

import java.io.IOException;

import android.app.Activity;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.MultiSelectListPreference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceManager;
import android.util.Log;

public class SettingsActivity extends Activity {
	private static final boolean DEBUG = false;
	private static final String TAG = "SettingsActivity";

	@Override
	public void onCreate(Bundle savedInstanceState) {
		if (DEBUG) Log.d(TAG, "onCreate()");
		super.onCreate(savedInstanceState);
		getFragmentManager().beginTransaction()
		.replace(android.R.id.content, new SettingsFragment()).commit();
	}

	//---------------------------------------------------------------------
	// FRAGMENT
	//---------------------------------------------------------------------
	public static class SettingsFragment extends PreferenceFragment implements
	SharedPreferences.OnSharedPreferenceChangeListener,
	DirSelectionDialog.OnDirSelectListener
	{
		private static final boolean DEBUG = SettingsActivity.DEBUG;
		private static final String TAG = "SettingsFragment";

		//-----------------------------------------------------------------
		// MEMBERS
		//-----------------------------------------------------------------
		private Settings mSettings = null;

		private MultiSelectListPreference mPrefEffectHideList = null;

		private ListPreference mPrefCameraSize = null;
		private ListPreference mPrefCameraFps  = null;
		private ListPreference mPrefPhotoFmt   = null;
		private ListPreference mPrefVideoMime  = null;
		private ListPreference mPrefVideoBps   = null;
		private ListPreference mPrefVideoIfi   = null;
		private ListPreference mPrefSavePath   = null;

		//-----------------------------------------------------------------
		// PUBLIC METHODS
		//-----------------------------------------------------------------
		@Override
		public void onCreate(Bundle savedInstanceState){
			if (DEBUG) Log.d(TAG, "onCreate()");
			super.onCreate(savedInstanceState);
			getPreferenceManager().setSharedPreferencesName(Settings.PREFS_NAME);
			addPreferencesFromResource(R.layout.settings_activity);

			mSettings = new Settings(getActivity());

			PreferenceManager pm = getPreferenceManager();
			mPrefEffectHideList = (MultiSelectListPreference)pm.findPreference(mSettings.KEY_EFFECT_HIDE_LIST);
			mPrefCameraSize = (ListPreference)pm.findPreference(mSettings.KEY_CAMERA_SIZE);
			mPrefCameraFps  = (ListPreference)pm.findPreference(mSettings.KEY_CAMERA_FPS);
			mPrefPhotoFmt   = (ListPreference)pm.findPreference(mSettings.KEY_PHOTO_FMT);
			mPrefVideoMime  = (ListPreference)pm.findPreference(mSettings.KEY_VIDEO_MIME);
			mPrefVideoBps   = (ListPreference)pm.findPreference(mSettings.KEY_VIDEO_BPS);
			mPrefVideoIfi   = (ListPreference)pm.findPreference(mSettings.KEY_VIDEO_IFI);
			mPrefSavePath   = (ListPreference)pm.findPreference(mSettings.KEY_SAVE_PATH);

			final String[] effect_names  = mSettings.getEffectNames();
			mPrefEffectHideList.setEntries(effect_names);
			mPrefEffectHideList.setEntryValues(effect_names);

			mPrefCameraSize.setValue(mSettings.getCameraSize());
			mPrefCameraFps.setValue(mSettings.getCameraFps());
			mPrefPhotoFmt.setValue(mSettings.getPhotoFmt());
			mPrefVideoMime.setValue(mSettings.getVideoMime());
			mPrefVideoBps.setValue(mSettings.getVideoBps());
			mPrefVideoIfi.setValue(mSettings.getVideoIfi());
			mPrefSavePath.setValue(mSettings.getSavePath());
		}

		@Override
		public void onResume() {
			if (DEBUG) Log.d(TAG, "onResume()");
			super.onResume();
			updateSummary();
			getPreferenceScreen().getSharedPreferences()
			.registerOnSharedPreferenceChangeListener(this);
		}

		@Override
		public void onPause() {
			if (DEBUG) Log.d(TAG, "onPause()");
			super.onPause();
			getPreferenceScreen().getSharedPreferences()
			.unregisterOnSharedPreferenceChangeListener(this);
		}

		@Override
		public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
			if (DEBUG) Log.d(TAG, "onSharedPreferenceChanged(): key="+key);
			mSettings.updateSettings();

			if (mSettings.KEY_SAVE_PATH.equals(key)) {
				// Custom ?
				if ("Custom".equals(mSettings.getSavePath())) {
					DirSelectionDialog d = new DirSelectionDialog(getActivity(), this);
					d.show(mSettings.getSavePathCustom());
				}
			}

			updateSummary();
		}

		@Override
		public void onDirSelected(final String dir) {
			if (DEBUG) Log.d(TAG, "onSharedPreferenceChanged(): dir="+dir);
			mSettings.setSavePathCustom(dir);
		}

		//-----------------------------------------------------------------
		// PRIVATE METHODS
		//-----------------------------------------------------------------
		private void updateSummary() {
			if (DEBUG) Log.d(TAG, "updateSummary()");

			mPrefCameraSize.setSummary(
					String.format("%s (%1dx%1d)",
							mSettings.getCameraSize(),
							mSettings.getCameraW(),
							mSettings.getCameraH()));

			final int[] camera_fps_range = mSettings.getCameraFpsByRange();
			mPrefCameraFps.setSummary(
					String.format("%s (%1d~%1d/%1d)",
							mSettings.getCameraFps(),
							camera_fps_range[0]/1000,
							camera_fps_range[1]/1000,
							mSettings.getVideoFpsAsInt()));

			mPrefPhotoFmt.setSummary(mSettings.getPhotoFmt());

			mPrefVideoMime.setSummary(mSettings.getVideoMime());

			int video_bps = mSettings.getVideoBpsAsInt();
			if (video_bps >= 1024 * 1024) {
				mPrefVideoBps.setSummary(
						String.format("%1d Mbps", video_bps / 1024 / 1024));
			} else {
				mPrefVideoBps.setSummary(
						String.format("%1d kbps", video_bps / 1024));
			}

			mPrefVideoIfi.setSummary(mSettings.getVideoIfi() + " sec.");

			try {
				mPrefSavePath.setSummary(
						String.format("%s: %s",
								mSettings.getSavePath(),
								mSettings.getSavePathAsFile().getCanonicalPath()));
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
	}
}

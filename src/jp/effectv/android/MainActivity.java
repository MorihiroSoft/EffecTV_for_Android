/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * MainActivity.java :
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

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Calendar;
import java.util.Locale;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.media.MediaScannerConnection;
import android.os.Bundle;
import android.text.format.DateFormat;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.CompoundButton;
import android.widget.ImageButton;
import android.widget.ListView;
import android.widget.Toast;
import android.widget.ToggleButton;

public class MainActivity extends Activity implements
SharedPreferences.OnSharedPreferenceChangeListener,
View.OnClickListener,
View.OnTouchListener,
AdapterView.OnItemClickListener,
CameraView.CameraPreviewBuffer,
ToggleButton.OnCheckedChangeListener
{
	private static final boolean DEBUG = false;
	private static final String TAG = "MainActivity";

	//---------------------------------------------------------------------
	// MEMBERS
	//---------------------------------------------------------------------
	// View parts
	private ImageButton  mBtnFace    = null;
	private ImageButton  mBtnMenu    = null;
	private ImageButton  mBtnPhoto   = null;
	private ImageButton  mBtnVideo   = null;
	private ToggleButton mBtnLock    = null;
	private ListView     mLstEffects = null;
	private ListView     mLstFuncs   = null;
	private CameraView   mCameraView = null;
	private EffectView   mEffectView = null;

	// Settings
	private Settings mSettings = null;

	// Effects
	private int      mEffectType  = -1;
	private String[] mEffectFuncs = null;

	// Buffers
	private boolean  mPlaying = false;
	private byte[]   mSrcYuv  = null;
	private int[][]  mDstRgb  = {null,null};
	private byte[][] mDstYuv  = {null,null};
	private char[][] mDstMsg  = new char[2][MainApplication.DST_MSG_LEN];
	private int[]    mDstIdx  = {0};

	// Encoder
	private Encoder mEncoder = null;
	private int     mEncoderColorFormat = 0;

	//---------------------------------------------------------------------
	// PUBLIC / PROTECTED METHODS
	//---------------------------------------------------------------------
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		if (DEBUG) Log.d(TAG, "onCreate()");
		super.onCreate(savedInstanceState);

		// Theme.NoTitleBar.Fullscreen
		getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
		requestWindowFeature(Window.FEATURE_NO_TITLE);

		setContentView(R.layout.main_activity);

		// for debug...
		boolean debug_dump = true;
		if (debug_dump) {
			DumpMediaCodec.dump(true);
		}

		// Native methods
		File f = getFilesDir();
		if (f == null || !f.exists()) {
			throw new RuntimeException(f.getAbsolutePath());
		}
		MainApplication.native_init(f.getPath());

		// View parts
		mBtnFace    = (ImageButton)findViewById(R.id.btn_face);
		mBtnMenu    = (ImageButton)findViewById(R.id.btn_menu);
		mBtnPhoto   = (ImageButton)findViewById(R.id.btn_photo);
		mBtnVideo   = (ImageButton)findViewById(R.id.btn_video);
		mBtnLock    = (ToggleButton)findViewById(R.id.btn_lock);
		mLstEffects = (ListView)findViewById(R.id.lst_effects);
		mLstFuncs   = (ListView)findViewById(R.id.lst_funcs);
		mCameraView = (CameraView)findViewById(R.id.cameraview);
		mEffectView = (EffectView)findViewById(R.id.effectview);

		mBtnPhoto.setOnClickListener(this);
		mBtnVideo.setOnClickListener(this);
		mBtnLock.setOnCheckedChangeListener(null); // -> resume()
		mBtnFace.setOnClickListener(this);
		mBtnMenu.setOnClickListener(this);
		mLstEffects.setOnItemClickListener(this);
		mLstFuncs.setOnItemClickListener(this);
		mEffectView.setOnTouchListener(this);

		// Settings
		mSettings = new Settings(this);
	}

	@Override
	public void onSharedPreferenceChanged(SharedPreferences pref, String key) {
		if (DEBUG) Log.d(TAG, "onSharedPreferenceChanged(): key="+key);
		pause();
		resume();
	}

	@Override
	protected void onResume() {
		if (DEBUG) Log.d(TAG, "onResume()");
		super.onResume();
		// Settings
		mSettings.updateSettings();

		// Effects
		mLstEffects.setAdapter(new ArrayAdapter<String>(this,
				R.layout.effects_list_item,
				mSettings.getEffectShowTitles()));
		setEffectType(0);

		//
		resume();
		mSettings.registerOnSharedPreferenceChangeListener(this);
	}

	@Override
	protected void onPause() {
		if (DEBUG) Log.d(TAG, "onPause()");
		mSettings.unregisterOnSharedPreferenceChangeListener(this);
		pause();
		super.onPause();
	}

	@Override
	public void onClick(View v) {
		if (DEBUG) Log.d(TAG, "onClick(): v="+v);
		switch(v.getId()) {
		case R.id.btn_photo:
			takePhoto();
			break;
		case R.id.btn_video:
			if (mEncoder != null) {
				stopVideo();
			} else {
				startVideo();
			}
			break;
		case R.id.btn_face:
			switchCamera();
			break;
		case R.id.btn_menu:
			showMenu();
			break;
		}
	}

	@Override
	public void onCheckedChanged(CompoundButton v, boolean isChecked) {
		if (DEBUG) Log.d(TAG, "onCheckedChanged(): v="+v+", isChecked="+isChecked);
		switch(v.getId()) {
		case R.id.btn_lock:
			mCameraView.setAutoLock(isChecked);
			break;
		}
	}

	@Override
	public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
		if (DEBUG) Log.d(TAG, "onItemClick(): position="+position+", id="+id);
		switch (parent.getId()) {
		case R.id.lst_effects:
			setEffectType(position);
			break;
		case R.id.lst_funcs:
			final String msg = MainApplication.native_event(position);
			if (msg != null) {
				Toast.makeText(this, msg, Toast.LENGTH_LONG).show();
			}
			break;
		}
	}

	@Override
	public boolean onTouch(View v, MotionEvent event) {
		if (DEBUG) Log.d(TAG, "onTouch() - EffectView");
		switch (v.getId()) {
		case R.id.effectview:
			final int cw = mSettings.getCameraW();
			final int ch = mSettings.getCameraH();
			final int ew = mEffectView.getWidth();
			final int eh = mEffectView.getHeight();

			switch (event.getAction()) {
			case MotionEvent.ACTION_DOWN: {
				int x = (int)event.getX() * cw / ew;
				int y = (int)event.getY() * ch / eh;
				final String msg = MainApplication.native_touch(0, x, y);
				if (msg != null) {
					Toast.makeText(this, msg, Toast.LENGTH_LONG).show();
				}
				return true;
			}
			case MotionEvent.ACTION_MOVE: {
				int x = (int)event.getX() * cw / ew;
				int y = (int)event.getY() * ch / eh;
				MainApplication.native_touch(1, x, y);
				return true;
			}
			case MotionEvent.ACTION_UP: {
				final String msg = MainApplication.native_touch(2, 0, 0);
				if (msg != null) {
					Toast.makeText(this, msg, Toast.LENGTH_LONG).show();
				}
				return true;
			}
			}
			break;
		}
		return false;
	}

	@Override
	public byte[] getBuffer() {
		if (DEBUG) Log.d(TAG, "getBuffer()");
		return mSrcYuv;
	}

	@Override
	public void onPreviewFrame(byte[] src_yuv) {
		if (DEBUG) Log.d(TAG, "onPreviewFrame()");

		final int idx = 1 - mDstIdx[0];

		if (mEncoder == null) {
			MainApplication.native_draw(src_yuv, mDstRgb[idx], mDstMsg[idx], 0, null);
		} else {
			MainApplication.native_draw(src_yuv, mDstRgb[idx], mDstMsg[idx], mEncoderColorFormat, mDstYuv[idx]);
			mEncoder.offerEncoder(mDstYuv[idx]);
		}

		synchronized (mDstIdx) {
			mDstIdx[0] = idx;
			mDstIdx.notify();
		}
	}

	//---------------------------------------------------------------------
	// PRIVATE METHODS
	//---------------------------------------------------------------------
	private void resume() {
		// Buffers
		final int cw = mSettings.getCameraW();
		final int ch = mSettings.getCameraH();
		final int rgb_size = cw * ch;
		final int yuv_size = cw * ch * 3 / 2;
		mSrcYuv    = new byte[yuv_size];
		mDstRgb[0] = new int[rgb_size];
		mDstRgb[1] = new int[rgb_size];
		mDstYuv[0] = new byte[yuv_size];
		mDstYuv[1] = new byte[yuv_size];

		// CameraView
		mCameraView.setup(
				mSettings.getCameraId(),
				mSettings.getCameraW(),
				mSettings.getCameraH(),
				mSettings.getCameraFpsByRange(),
				this);
		mCameraView.resume();

		mBtnLock.setOnCheckedChangeListener(null);
		mBtnLock.setChecked(mCameraView.getAutoLock());
		mBtnLock.setOnCheckedChangeListener(this);

		// JNI
		if (!mPlaying) {
			if (MainApplication.native_start(
					mSettings.getCameraFacingFront(),
					mSettings.getCameraW(),
					mSettings.getCameraH(),
					mSettings.getVideoFpsAsInt(),
					mEffectType) !=0) {
				MainApplication.native_stop();
			} else {
				mPlaying = true;
			}
		}

		// EffectView
		mEffectView.setup(cw, ch, mDstRgb, mDstMsg, mDstIdx);
		mEffectView.resume();
		mEffectView.setZOrderMediaOverlay(true);
		mEffectView.setZOrderOnTop(true);
		mEffectView.getParent().bringChildToFront(mEffectView);

		// Video
		// -> not auto restart
	}

	private void pause() {
		// Video
		stopVideo();

		// EffectView
		mEffectView.pause();

		// JNI
		if (mPlaying) {
			MainApplication.native_stop();
			mPlaying = false;
		}

		// CameraView, Auto lock
		mCameraView.pause();
		mBtnLock.setTextColor(0xFF000000);

		// Buffers
		mSrcYuv    = null;
		mDstRgb[0] = null;
		mDstRgb[1] = null;
		mDstYuv[0] = null;
		mDstYuv[1] = null;
	}

	/*
	private int getDisplayRotation() {
		if (DEBUG) Log.d(TAG, "getRotation()");
		switch(getWindowManager().getDefaultDisplay().getRotation()) {
		case Surface.ROTATION_0:
			return 0;
		case Surface.ROTATION_90:
			return 90;
		case Surface.ROTATION_180:
			return 180;
		case Surface.ROTATION_270:
			return 270;
		default:
			throw new RuntimeException();
		}
	}
	 */

	private void switchCamera() {
		if (DEBUG) Log.d(TAG, "switchCamera()");
		mSettings.toggleCameraId();
	}

	private void setEffectType(int pos) {
		if (DEBUG) Log.d(TAG, "setEffectType(): pos="+pos);
		final int[] indexs = mSettings.getEffectIndexs();
		mEffectType = (indexs==null ? 0 : indexs[pos]);
		mLstEffects.setItemChecked(pos, true);
		mEffectFuncs = MainApplication.native_funcs(mEffectType);
		mLstFuncs.setAdapter(new ArrayAdapter<String>(
				this, R.layout.funcs_list_item, mEffectFuncs));

		if (mPlaying) {
			MainApplication.native_stop();
			if (MainApplication.native_start(
					mSettings.getCameraFacingFront(),
					mSettings.getCameraW(),
					mSettings.getCameraH(),
					mSettings.getVideoFpsAsInt(),
					mEffectType) !=0 ) {
				mPlaying = false;
			}
		}
	}

	private void showMenu() {
		if (DEBUG) Log.d(TAG, "showMenu()");
		final Intent i = new Intent(this, SettingsActivity.class);
		startActivity(i);
	}

	private void takePhoto() {
		if (DEBUG) Log.d(TAG, "takePhoto()");
		final int cw = mSettings.getCameraW();
		final int ch = mSettings.getCameraH();

		// Create bitmap
		Bitmap bmp = null;
		synchronized (mDstIdx) {
			bmp = Bitmap.createBitmap(cw, ch, Bitmap.Config.ARGB_8888);
			Canvas c = new Canvas(bmp);
			Paint p = new Paint();
			c.drawBitmap(mDstRgb[mDstIdx[0]], 0, cw, 0, 0, cw, ch, false, p);
		}

		// Find unique file name
		final File dir = mSettings.getSavePathAsFile();
		final String now = (String)DateFormat.format("yyyyMMdd_kkmmss", Calendar.getInstance());
		File path = null;
		for (int i=1; ; i++) {
			final String name = String.format(Locale.US,
					"ETV_%s_%1d.%s", now, i, mSettings.getPhotoFmt());
			path = new File(dir, name);
			if (!path.exists()) {
				break;
			}
		}

		// Save file
		BufferedOutputStream os = null;
		try {
			os = new BufferedOutputStream(new FileOutputStream(path));
			bmp.compress(
					mSettings.getPhotoFmtAsType(),
					90, os);
		} catch (Exception e){
			e.printStackTrace();
		} finally {
			if (os != null) {
				try {
					os.close();
				} catch (IOException e) {
					e.printStackTrace();
				}
			}
		}

		// Registry MediaScanner
		MediaScannerConnection.scanFile(this,
				new String[]{ path.getAbsolutePath() },
				new String[]{ "image/jpeg" },
				null);

		// Release bitmap
		if (bmp != null) {
			bmp.recycle();
			bmp = null;
		}
	}

	private void startVideo() {
		if (DEBUG) Log.d(TAG, "startVideo()");
		mEncoder = new Encoder(this);
		if (mEncoder.start(
				mSettings.getCameraW(),
				mSettings.getCameraH(),
				mSettings.getVideoFpsAsInt(),
				mSettings.getVideoMime(),
				mSettings.getVideoBpsAsInt(),
				mSettings.getVideoIfiAsInt()) != 0) {
			stopVideo();
			return;
		}
		mEncoderColorFormat = mEncoder.getColorFormat();
		mBtnVideo.setImageResource(R.drawable.ic_switch_video_active);
	}

	private void stopVideo() {
		if (DEBUG) Log.d(TAG, "stopVideo()");
		if (mEncoder != null) {
			mEncoder.stop(mSettings.getSavePathAsFile());
			mEncoder = null;
		}
		mBtnVideo.setImageResource(R.drawable.ic_switch_video);
	}
}

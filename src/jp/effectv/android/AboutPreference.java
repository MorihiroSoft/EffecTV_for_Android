/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * AboutPreference.java :
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

import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.Color;
import android.preference.DialogPreference;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.webkit.WebView;
import android.widget.LinearLayout;

public class AboutPreference extends DialogPreference
{
	private static final boolean DEBUG = false;
	private static final String TAG = "AboutPreference";

	//---------------------------------------------------------------------
	// MEMBERS
	//---------------------------------------------------------------------
	private static final String androidns="http://schemas.android.com/apk/res/android";
	private Context mContext;
	private String mMessage = "???";

	//---------------------------------------------------------------------
	// PUBLIC/PROTECTED METHODS
	//---------------------------------------------------------------------
	public AboutPreference(Context context, AttributeSet attrs) {
		super(context, attrs);
		if (DEBUG) Log.d(TAG, "AboutPreference()");
		mContext = context;

		setNegativeButtonText(null);

		// Title
		try {
			PackageInfo packageInfo = context.getPackageManager().getPackageInfo(
					context.getPackageName(),
					PackageManager.GET_META_DATA);
			int t = attrs.getAttributeResourceValue(androidns, "dialogTitle", 0);
			if (t > 0) {
				setDialogTitle(context.getResources().getString(t) +
						" Ver." + packageInfo.versionName);
			}
		} catch (NameNotFoundException e) {
			e.printStackTrace();
		}

		// Message
		int m = attrs.getAttributeResourceValue(androidns, "dialogMessage", 0);
		if (m > 0) {
			mMessage = context.getResources().getString(m);
		}
	}

	@Override
	protected View onCreateDialogView() {
		if (DEBUG) Log.d(TAG, "onCreateDialogView()");
		LinearLayout layout = new LinearLayout(mContext);
		layout.setOrientation(LinearLayout.VERTICAL);
		layout.setPadding(6,6,6,6);

		WebView web = new WebView(mContext);
		web.setBackgroundColor(Color.TRANSPARENT);
		if (mMessage.startsWith("file:///")) {
			web.loadUrl(mMessage);
		} else {
			web.loadData(mMessage, "text/html", "utf-8");
		}
		layout.addView(web);

		return layout;
	}

	@Override
	protected void onBindDialogView(View v) {
		if (DEBUG) Log.d(TAG, "onBindDialogView()");
		super.onBindDialogView(v);
	}
}

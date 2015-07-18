/*
 * Copyright (C) 2014 Frank Morgner
 *
 * This file is part of RemoteSmartCardReader.
 *
 * RemoteSmartCardReader is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * RemoteSmartCardReader is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * RemoteSmartCardReader.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.vsmartcard.remotesmartcardreader.app;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.preference.PreferenceManager;

class Settings {

    private static final String KEY_VPCD_HOST = "hostname";
    private static final String KEY_VPCD_PORT = "port";

    private static final String DEFAULT_VPCD_HOST = "10.0.2.2";
    private static final String DEFAULT_VPCD_PORT = "35963";

    private final SharedPreferences settings;

    public Settings (Context activity) {
        settings = PreferenceManager.getDefaultSharedPreferences(activity);
    }

    private void setString(String key, String value) {
        Editor editor = settings.edit();
        editor.putString(key, value);
        editor.apply();
    }

//    private void setBoolean(String key, boolean value) {
//        Editor editor = settings.edit();
//        editor.putBoolean(key, value);
//        editor.apply();
//    }

    public void setVPCDSettings(String hostname, String port) {
        Editor editor = settings.edit();
        editor.putString(KEY_VPCD_HOST, hostname);
        editor.putString(KEY_VPCD_PORT, port);
        editor.apply();
    }

    public String getVPCDHost() {
        return settings.getString(KEY_VPCD_HOST, DEFAULT_VPCD_HOST);
    }

    public String getVPCDPort() {
        return settings.getString(KEY_VPCD_PORT, DEFAULT_VPCD_PORT);
    }
}

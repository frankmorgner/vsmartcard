/*
 * Copyright (C) 2015 Frank Morgner
 *
 * This file is part of ACardEmulator.
 *
 * ACardEmulator is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * ACardEmulator is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ACardEmulator.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.vsmartcard.acardemulator.emulators;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;

import com.vsmartcard.acardemulator.R;
import com.vsmartcard.acardemulator.Util;

public class EmulatorSingleton {
    public static final String TAG = "com.vsmartcard.acardemulator.EmulatorService";
    public static final String EXTRA_CAPDU = "MSG_CAPDU";
    public static final String EXTRA_RAPDU = "MSG_RAPDU";
    public static final String EXTRA_ERROR = "MSG_ERROR";
    public static final String EXTRA_DESELECT = "MSG_DESELECT";
    public static final String EXTRA_INSTALL = "MSG_INSTALL";

    private static Emulator emulator = null;
    private static boolean do_destroy = false;

    private static void destroyEmulatorIfRequested(Context context) {
        if (do_destroy) {
            emulator.destroy();
            emulator = null;
            do_destroy = false;
            Intent i = new Intent(TAG);
            i.putExtra(EXTRA_DESELECT, "Uninstalled all applets.");
            LocalBroadcastManager.getInstance(context).sendBroadcast(i);
        }
    }

    public static void destroyEmulator() {
        do_destroy = true;
    }

    public static void createEmulator(Context context) {
        // force reloading applets if requested
        destroyEmulatorIfRequested(context);

        if (emulator == null) {
            Log.d("", "Begin transaction");
            SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(context);
            String str_emulator = SP.getString("emulator", "");
            if (str_emulator.equals(context.getString(R.string.vicc))) {
                emulator = new VICCEmulator(
                        SP.getString("hostname", VICCEmulator.DEFAULT_HOSTNAME),
                        Integer.parseInt(SP.getString("port", Integer.toString(VICCEmulator.DEFAULT_PORT))));
            } else {
                emulator = new JCEmulator(context,
                        SP.getBoolean("activate_helloworld", false),
                        SP.getBoolean("activate_openpgp", false),
                        SP.getBoolean("activate_oath", false),
                        SP.getBoolean("activate_isoapplet", false),
                        SP.getBoolean("activate_gidsapplet", false));
            }
        }
    }

    public static byte[] process(Context context, byte[] capdu) {
        byte[] rapdu = emulator.process(capdu);

        Intent i = new Intent(TAG);
        i.putExtra(EXTRA_CAPDU, Util.byteArrayToHexString(capdu));
        if (rapdu != null)
            i.putExtra(EXTRA_RAPDU, Util.byteArrayToHexString(rapdu));
        LocalBroadcastManager.getInstance(context).sendBroadcast(i);

        return rapdu;
    }

    public static void deactivate() {
        emulator.deactivate();
    }
}
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

package com.vsmartcard.acardemulator;

import android.content.Intent;
import android.nfc.cardemulation.HostApduService;
import android.os.Bundle;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;

import com.vsmartcard.acardemulator.emulators.EmulatorSingleton;

public class EmulatorHostApduService extends HostApduService {

    @Override
    public void onCreate() {
        super.onCreate();
        EmulatorSingleton.createEmulator(this);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    @Override
    public byte[] processCommandApdu(byte[] capdu, Bundle extras) {
        return EmulatorSingleton.process(this, capdu);
    }

    @Override
    public void onDeactivated(int reason) {
        Intent i = new Intent(EmulatorSingleton.TAG);

        Log.d("", "End transaction");
        switch (reason) {
            case DEACTIVATION_LINK_LOSS:
                i.putExtra(EmulatorSingleton.EXTRA_DESELECT, "link lost");
                EmulatorSingleton.deactivate();
                break;
            case DEACTIVATION_DESELECTED:
                EmulatorSingleton.deactivate();
                break;
        }

        LocalBroadcastManager.getInstance(this).sendBroadcast(i);
    }
}

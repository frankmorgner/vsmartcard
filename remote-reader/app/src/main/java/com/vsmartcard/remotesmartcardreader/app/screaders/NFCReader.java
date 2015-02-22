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

package com.vsmartcard.remotesmartcardreader.app.screaders;

import android.content.Intent;
import android.nfc.NfcAdapter;
import android.nfc.Tag;
import android.nfc.tech.IsoDep;
import android.os.Build;
import android.util.Log;

import com.vsmartcard.remotesmartcardreader.app.Hex;

import java.io.IOException;

public class NFCReader implements SCReader {

    private IsoDep card;

    public NFCReader(IsoDep sc) throws IOException {
        this.card = sc;
        sc.connect();
        card.setTimeout(TIMEOUT);
    }

    @Override
    public void eject() throws IOException {
        card.close();
    }

    public static int TIMEOUT = 2500;
    @Override
    public void powerOn() {
        /* should already be connected... */
    }

    private static byte[] SELECT_MF = {(byte) 0x00, (byte) 0xa4, (byte) 0x00, (byte) 0x0C};
    private void selectMF() throws IOException {
        byte[] response = card.transceive(SELECT_MF);
        if (response.length == 2 && response[0] == 0x90 && response[1] == 0x00) {
            Log.d(this.getClass().getName(), "Resetting the card by selecting the MF results in " + Hex.getHexString(response));
        }
    }

    @Override
    public void powerOff() throws IOException {
        selectMF();
    }

    @Override
    public void reset() throws IOException {
        selectMF();
    }

    /* calculation based on https://code.google.com/p/ifdnfc/source/browse/src/atr.c */
    @Override
    public byte[] getATR() {
        byte[] historicalBytes = card.getHistoricalBytes();
        if (historicalBytes == null) {
            historicalBytes = new byte[0];
        }

        /* copy historical bytes if available */
        byte[] atr = new byte[4+historicalBytes.length+1];
        atr[0] = (byte) 0x3b;
        atr[1] = (byte) (0x80+historicalBytes.length);
        atr[2] = (byte) 0x80;
        atr[3] = (byte) 0x01;
        System.arraycopy(historicalBytes, 0, atr, 4, historicalBytes.length);

        /* calculate TCK */
        byte tck = atr[1];
        for (int idx = 2; idx < atr.length; idx++) {
            tck ^= atr[idx];
        }
        atr[atr.length-1] = tck;

        return atr;
    }

    @Override
    public byte[] transmit(byte[] apdu) throws IOException {
        return card.transceive(apdu);
    }

    public static NFCReader get(Intent intent) {
        NFCReader nfcReader = null;

        if (intent.getExtras() != null) {
            Tag tag = intent.getParcelableExtra(NfcAdapter.EXTRA_TAG);
            if (tag != null) {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
                    /* Disconnect from the tag here. The reader mode will capture the tag again */
                    try {
                        IsoDep.get(tag).close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                } else {
                    nfcReader = NFCReader.get(tag);
                }
                intent.removeExtra(NfcAdapter.EXTRA_TAG);
            }
        }

        return nfcReader;
    }

    public static NFCReader get(Tag tag) {
        NFCReader nfcReader = null;
        try {
            nfcReader = new NFCReader(IsoDep.get(tag));
        } catch (IOException e) {
            e.printStackTrace();
        }
        return nfcReader;
    }
}

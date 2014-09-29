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

public class DummyReader implements SCReader {

    @Override
    public void eject() {
    }

    @Override
    public void powerOn() {
    }

    @Override
    public void powerOff() {
    }

    @Override
    public void reset() {
    }

    private static final byte[] dummyATR = {0x3B, 0x68, 0x00, (byte) 0xFF, 56, 43, 0x41, 0x52, 0x44, 0x6E, 0x73, 0x73};
    @Override
    public byte[] getATR() {
        return dummyATR;
    }

    private static final byte[] dummyResponse = {0x6F, 0x00};
    @Override
    public byte[] transmit(byte[] apdu) {
        return dummyResponse;
    }
}

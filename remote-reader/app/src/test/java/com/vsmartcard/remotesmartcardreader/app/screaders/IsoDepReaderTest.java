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

import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;

public class IsoDepReaderTest {

    @Test
    public void testTranslateToMbli() {
        // Test frame size translation
        byte[] protocolInfo = new byte[]{0x00, (byte) 0x80, 0x00}; // maxFrameSizeCode = 8 -> 256
        Byte mbli = IsoDepReader.translateToMbli(protocolInfo, 251);
        assertNotNull(mbli);
        assertEquals((byte) 1, mbli.byteValue());

        // Test invalid/null protocolInfo
        assertNull(IsoDepReader.translateToMbli(null, 100));
        assertNull(IsoDepReader.translateToMbli(new byte[]{0x00}, 100));

        // Test delegation from NFCReader
        assertEquals(mbli, NFCReader.translateToMbli(protocolInfo, 251));
    }
}

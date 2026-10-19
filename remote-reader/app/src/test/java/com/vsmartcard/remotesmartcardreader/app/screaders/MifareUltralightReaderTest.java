/*
 * Copyright (C) 2026 Frank Morgner
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

import android.nfc.tech.MifareUltralight;

import org.junit.Test;

import java.io.IOException;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

public class MifareUltralightReaderTest {

    private static class MockMifareUltralightTransceiver implements MifareUltralightReader.Transceiver {
        boolean connected = false;
        boolean closed = false;
        int timeout = 0;
        int type = MifareUltralight.TYPE_ULTRALIGHT;
        byte[] uid = new byte[]{0x04, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
        final Map<Integer, byte[]> pages = new HashMap<>();
        byte[] lastTransceived = null;
        byte[] nextTransceiveResponse = new byte[]{0x00, 0x04};
        boolean throwIOException = false;

        @Override
        public void connect() throws IOException {
            connected = true;
        }

        @Override
        public void close() throws IOException {
            closed = true;
        }

        @Override
        public void setTimeout(int timeout) {
            this.timeout = timeout;
        }

        @Override
        public int getTimeout() {
            return timeout;
        }

        @Override
        public int getType() {
            return type;
        }

        @Override
        public byte[] getUid() {
            return uid;
        }

        @Override
        public byte[] readPages(int pageOffset) throws IOException {
            byte[] out = new byte[16];
            for (int i = 0; i < 4; i++) {
                byte[] p = pages.get((pageOffset + i) & 0x0F);
                if (p != null) {
                    System.arraycopy(p, 0, out, i * 4, 4);
                }
            }
            return out;
        }

        @Override
        public void writePage(int pageOffset, byte[] data) throws IOException {
            pages.put(pageOffset, Arrays.copyOf(data, 4));
        }

        @Override
        public byte[] transceive(byte[] data) throws IOException {
            if (throwIOException) {
                throw new IOException("Simulated RF error");
            }
            lastTransceived = data != null ? data.clone() : null;
            return nextTransceiveResponse != null ? nextTransceiveResponse.clone() : null;
        }
    }

    @Test
    public void testAtrUltralightAndUltralightC() throws IOException {
        MockMifareUltralightTransceiver mock = new MockMifareUltralightTransceiver();

        // 1. MIFARE Ultralight: NN = 00 03, TCK = 68 (ACS ACR122U V2.04 Section 3.1.1 Table 2, Pages 8-9)
        mock.type = MifareUltralight.TYPE_ULTRALIGHT;
        MifareUltralightReader readerUl = new MifareUltralightReader(mock, null);
        assertArrayEquals(
                new byte[]{
                        (byte) 0x3B, (byte) 0x8F, (byte) 0x80, (byte) 0x01,
                        (byte) 0x80, (byte) 0x4F, (byte) 0x0C, (byte) 0xA0,
                        (byte) 0x00, (byte) 0x00, (byte) 0x03, (byte) 0x06,
                        (byte) 0x03, (byte) 0x00, (byte) 0x03,
                        (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
                        (byte) 0x68
                },
                readerUl.getATR()
        );

        // 2. MIFARE Ultralight C: NN = 00 3A, TCK = 51 (PC/SC Part 3 Supplemental Section 2.1.2.2 Table 3, Page 4)
        mock.type = MifareUltralight.TYPE_ULTRALIGHT_C;
        MifareUltralightReader readerUlC = new MifareUltralightReader(mock, null);
        byte[] atrUlC = readerUlC.getATR();
        assertEquals((byte) 0x00, atrUlC[13]);
        assertEquals((byte) 0x3A, atrUlC[14]);
        assertEquals((byte) 0x51, atrUlC[19]);
    }

    @Test
    public void testGetDataUid() throws IOException {
        MockMifareUltralightTransceiver mock = new MockMifareUltralightTransceiver();
        MifareUltralightReader reader = new MifareUltralightReader(mock, null);

        // 5-byte APDU: FF CA 00 00 00
        byte[] resp = reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xCA, 0x00, 0x00, 0x00});
        assertArrayEquals(
                new byte[]{0x04, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, (byte) 0x90, 0x00},
                resp
        );

        // 4-byte APDU: FF CA 00 00 (without Le)
        byte[] resp4 = reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xCA, 0x00, 0x00});
        assertArrayEquals(
                new byte[]{0x04, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, (byte) 0x90, 0x00},
                resp4
        );
    }

    @Test
    public void testIsoProbing() throws IOException {
        MockMifareUltralightTransceiver mock = new MockMifareUltralightTransceiver();
        MifareUltralightReader reader = new MifareUltralightReader(mock, null);

        // ISO 7816-4 SELECT MF (00 A4 00 0C) probing should succeed with 90 00 without RF transceive
        byte[] resp = reader.transmit(new byte[]{0x00, (byte) 0xA4, 0x00, 0x0C});
        assertArrayEquals(new byte[]{(byte) 0x90, 0x00}, resp);
    }

    @Test
    public void testReadAndUpdateBinarySingleAndMultiplePages() throws IOException {
        MockMifareUltralightTransceiver mock = new MockMifareUltralightTransceiver();
        MifareUltralightReader reader = new MifareUltralightReader(mock, null);

        // Update 1 page (Page 4): FF D6 00 04 04 DE AD BE EF
        byte[] updateSingle = reader.transmit(new byte[]{
                (byte) 0xFF, (byte) 0xD6, 0x00, 0x04, 0x04,
                (byte) 0xDE, (byte) 0xAD, (byte) 0xBE, (byte) 0xEF
        });
        assertArrayEquals(new byte[]{(byte) 0x90, 0x00}, updateSingle);

        // Read 1 page (Page 4, Le=04): FF B0 00 04 04 -> DE AD BE EF 90 00
        byte[] readSingle = reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xB0, 0x00, 0x04, 0x04});
        assertArrayEquals(
                new byte[]{(byte) 0xDE, (byte) 0xAD, (byte) 0xBE, (byte) 0xEF, (byte) 0x90, 0x00},
                readSingle
        );

        // Read 4 pages (Page 4..7, Le=10h): FF B0 00 04 10 -> 16 bytes + 90 00
        byte[] readFour = reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xB0, 0x00, 0x04, 0x10});
        assertEquals(18, readFour.length);
        assertEquals((byte) 0xDE, readFour[0]);
        assertEquals((byte) 0x90, readFour[16]);
        assertEquals((byte) 0x00, readFour[17]);

        // Read without Le (4-byte APDU): FF B0 00 04 -> defaults to 16 bytes + 90 00
        byte[] readDefault = reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xB0, 0x00, 0x04});
        assertEquals(18, readDefault.length);
        assertEquals((byte) 0xDE, readDefault[0]);

        // Invalid length for Update Binary (not multiple of 4 bytes) -> 67 00
        assertArrayEquals(
                new byte[]{(byte) 0x67, 0x00},
                reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xD6, 0x00, 0x04, 0x03, 1, 2, 3})
        );
    }

    @Test
    public void testDirectTransmitAndLibfreefareNativeFrames() throws IOException {
        MockMifareUltralightTransceiver mock = new MockMifareUltralightTransceiver();
        MifareUltralightReader reader = new MifareUltralightReader(mock, null);

        // 1. Native WRITE (A2 [Page=05] [4B]) -> returns 90 00 (PC/SC 2-byte minimum)
        byte[] writeResp = reader.transmit(new byte[]{(byte) 0xA2, 0x05, 0x11, 0x22, 0x33, 0x44});
        assertArrayEquals(new byte[]{(byte) 0x90, 0x00}, writeResp);

        // 2. Native COMPATIBILITY WRITE (A0 [Page=06] [16B], only first 4 bytes written per NXP MF0ICU1 §7.6.8 Table 13)
        byte[] compatWrite = new byte[18];
        compatWrite[0] = (byte) 0xA0;
        compatWrite[1] = 0x06;
        compatWrite[2] = (byte) 0xAA;
        compatWrite[3] = (byte) 0xBB;
        compatWrite[4] = (byte) 0xCC;
        compatWrite[5] = (byte) 0xDD;
        compatWrite[6] = (byte) 0xFF; // ignored
        assertArrayEquals(new byte[]{(byte) 0x90, 0x00}, reader.transmit(compatWrite));

        // 3. Native READ (30 [Page=05]) -> 16 bytes (Pages 5, 6, 7, 8)
        byte[] readResp = reader.transmit(new byte[]{0x30, 0x05});
        assertEquals(16, readResp.length);
        assertArrayEquals(new byte[]{0x11, 0x22, 0x33, 0x44}, Arrays.copyOfRange(readResp, 0, 4));
        assertArrayEquals(
                new byte[]{(byte) 0xAA, (byte) 0xBB, (byte) 0xCC, (byte) 0xDD},
                Arrays.copyOfRange(readResp, 4, 8)
        );

        // 4. Direct Transmit (FF 00 00 00 01 60) -> GET_VERSION on Ultralight EV1
        byte[] directResp = reader.transmit(new byte[]{(byte) 0xFF, 0x00, 0x00, 0x00, 0x01, 0x60});
        assertArrayEquals(new byte[]{0x60}, mock.lastTransceived);
        assertArrayEquals(new byte[]{0x00, 0x04, (byte) 0x90, 0x00}, directResp);

        reader.eject();
        assertTrue(mock.closed);
    }

    @Test
    public void testPageBoundaryValidations() throws IOException {
        MockMifareUltralightTransceiver mock = new MockMifareUltralightTransceiver();
        MifareUltralightReader reader = new MifareUltralightReader(mock, null);

        // Read starting at page 254 with Le=16 (4 pages -> up to page 258 > 256) -> 6A 86
        byte[] oobRead = reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xB0, 0x00, (byte) 0xFE, 0x10});
        assertArrayEquals(new byte[]{(byte) 0x6A, (byte) 0x86}, oobRead);

        // Update starting at page 255 with Lc=8 (2 pages -> up to page 257 > 256) -> 6A 86
        byte[] oobUpdate = reader.transmit(new byte[]{
                (byte) 0xFF, (byte) 0xD6, 0x00, (byte) 0xFF, 0x08,
                1, 2, 3, 4, 5, 6, 7, 8
        });
        assertArrayEquals(new byte[]{(byte) 0x6A, (byte) 0x86}, oobUpdate);
    }

    @Test
    public void testDirectTransmitInvalidP1P2AndAtsOnStorageCard() throws IOException {
        MockMifareUltralightTransceiver mock = new MockMifareUltralightTransceiver();
        MifareUltralightReader reader = new MifareUltralightReader(mock, null);

        // Direct Transmit: P1 != 0 -> 6A 86
        assertArrayEquals(
                new byte[]{(byte) 0x6A, (byte) 0x86},
                reader.transmit(new byte[]{(byte) 0xFF, 0x00, 0x01, 0x00, 0x01, 0x60})
        );

        // Direct Transmit: P2 != 0 -> 6A 86
        assertArrayEquals(
                new byte[]{(byte) 0x6A, (byte) 0x86},
                reader.transmit(new byte[]{(byte) 0xFF, 0x00, 0x00, 0x01, 0x01, 0x60})
        );

        // Get Data: ATS historical bytes (P1=0x01) on storage card -> 6A 81 (Function not supported)
        assertArrayEquals(
                new byte[]{(byte) 0x6A, (byte) 0x81},
                reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xCA, 0x01, 0x00, 0x00})
        );
    }

    @Test
    public void testNativeTransceiveFailureReturns6300() throws IOException {
        MockMifareUltralightTransceiver mock = new MockMifareUltralightTransceiver();
        MifareUltralightReader reader = new MifareUltralightReader(mock, null);

        mock.throwIOException = true;
        // Native custom frame (e.g. 1A 00 Auth) failure returns 63 00 rather than crashing VPCD
        byte[] errResp = reader.transmit(new byte[]{0x1A, 0x00});
        assertArrayEquals(new byte[]{(byte) 0x63, 0x00}, errResp);
    }
}

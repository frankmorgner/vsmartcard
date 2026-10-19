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

import java.io.IOException;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

public class IsoDepReaderTest {

    private static class MockTransceiver implements IsoDepReader.Transceiver {
        boolean closed = false;
        int timeout = 0;
        byte[] uid = new byte[]{0x04, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
        byte[] historicalBytes = new byte[]{(byte) 0x80, 0x31, (byte) 0x80, 0x65};
        byte[] typeBHistoricalBytes = null;
        byte[] lastSent = null;
        byte[] nextResponse = new byte[]{(byte) 0x90, 0x00};
        boolean throwIOException = false;

        @Override
        public void close() throws IOException {
            closed = true;
        }

        @Override
        public void setTimeout(int timeout) {
            this.timeout = timeout;
        }

        @Override
        public byte[] transceive(byte[] data) throws IOException {
            if (throwIOException) {
                throw new IOException("Simulated RF connection failure");
            }
            lastSent = data != null ? data.clone() : null;
            return nextResponse != null ? nextResponse.clone() : null;
        }

        @Override
        public byte[] getHistoricalBytes() {
            return historicalBytes;
        }

        @Override
        public byte[] getTypeBHistoricalBytes() {
            return typeBHistoricalBytes;
        }

        @Override
        public byte[] getUid() {
            return uid;
        }
    }

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

    @Test
    public void testGetDataUidForDesfireAndIsoDep() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        IsoDepReader reader = new IsoDepReader(mock, null);
        assertEquals(500, mock.timeout);

        // FF CA 00 00 00: PC/SC Part 3 Section 3.2.2.1.3 Get Data (UID)
        byte[] resp = reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xCA, 0x00, 0x00, 0x00});
        assertArrayEquals(
                new byte[]{0x04, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, (byte) 0x90, 0x00},
                resp
        );
        assertNull(mock.lastSent);

        // Le < UID length -> 6C 07 (PC/SC Part 3 Table 3-9)
        byte[] shortLe = reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xCA, 0x00, 0x00, 0x04});
        assertArrayEquals(new byte[]{(byte) 0x6C, 0x07}, shortLe);

        // Le > UID length -> zero-padded to Le + 62 82 (PC/SC Part 3 Table 3-9)
        byte[] longLe = reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xCA, 0x00, 0x00, 0x09});
        assertArrayEquals(
                new byte[]{0x04, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x00, 0x00, (byte) 0x62, (byte) 0x82},
                longLe
        );
    }

    @Test
    public void testGetDataAtsHistoricalBytes() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        IsoDepReader reader = new IsoDepReader(mock, null);

        // FF CA 01 00 00: PC/SC Part 3 Section 3.2.2.1.3 Get Data (ATS Historical Bytes)
        byte[] resp = reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xCA, 0x01, 0x00, 0x00});
        assertArrayEquals(
                new byte[]{(byte) 0x80, 0x31, (byte) 0x80, 0x65, (byte) 0x90, 0x00},
                resp
        );
    }

    @Test
    public void testGetDataInvalidParameters() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        IsoDepReader reader = new IsoDepReader(mock, null);

        // P2 != 0x00 -> 6A 86
        assertArrayEquals(
                new byte[]{(byte) 0x6A, (byte) 0x86},
                reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xCA, 0x00, 0x01, 0x00})
        );
        // 4-byte APDU without Le: FF CA 00 00 -> returns full UID + 90 00
        byte[] resp4 = reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xCA, 0x00, 0x00});
        assertArrayEquals(
                new byte[]{0x04, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, (byte) 0x90, 0x00},
                resp4
        );

        // Wrong length (e.g. 3 bytes or 6 bytes) -> 67 00
        assertArrayEquals(
                new byte[]{(byte) 0x67, 0x00},
                reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xCA, 0x00})
        );
        assertArrayEquals(
                new byte[]{(byte) 0x67, 0x00},
                reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xCA, 0x00, 0x00, 0x00, 0x00})
        );
    }

    @Test
    public void testDirectTransmit() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        IsoDepReader reader = new IsoDepReader(mock, null);
        mock.nextResponse = new byte[]{0x00, 0x11, 0x22};

        // FF 00 00 00 02 60 00 (ACS ACR122U Section 6.1 Direct Transmit)
        byte[] resp = reader.transmit(new byte[]{(byte) 0xFF, 0x00, 0x00, 0x00, 0x02, 0x60, 0x00});
        assertArrayEquals(new byte[]{0x60, 0x00}, mock.lastSent);
        assertArrayEquals(new byte[]{0x00, 0x11, 0x22, (byte) 0x90, 0x00}, resp);
    }

    @Test
    public void testDesfireNativeSingleByteResponseAppends9000() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        IsoDepReader reader = new IsoDepReader(mock, null);

        // MIFARE DESFire native frame (e.g., 60 = GetVersion or 0A 00 = Authenticate)
        // returning 1-byte status 0xAF -> ACS ACR122U Section 7.2 appends 90 00
        mock.nextResponse = new byte[]{(byte) 0xAF};
        byte[] resp = reader.transmit(new byte[]{0x60});
        assertArrayEquals(new byte[]{0x60}, mock.lastSent);
        assertArrayEquals(new byte[]{(byte) 0xAF, (byte) 0x90, 0x00}, resp);
    }

    @Test
    public void testStandardIso7816ApduPassThrough() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        IsoDepReader reader = new IsoDepReader(mock, null);

        mock.nextResponse = new byte[]{0x6F, 0x00, (byte) 0x90, 0x00};
        byte[] cmd = new byte[]{0x00, (byte) 0xA4, 0x04, 0x00, 0x00};
        byte[] resp = reader.transmit(cmd);
        assertArrayEquals(cmd, mock.lastSent);
        assertArrayEquals(new byte[]{0x6F, 0x00, (byte) 0x90, 0x00}, resp);
    }

    @Test
    public void testDirectTransmitInvalidP1P2() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        IsoDepReader reader = new IsoDepReader(mock, null);

        // P1 != 0x00 -> 6A 86
        assertArrayEquals(
                new byte[]{(byte) 0x6A, (byte) 0x86},
                reader.transmit(new byte[]{(byte) 0xFF, 0x00, 0x01, 0x00, 0x02, 0x60, 0x00})
        );
        // P2 != 0x00 -> 6A 86
        assertArrayEquals(
                new byte[]{(byte) 0x6A, (byte) 0x86},
                reader.transmit(new byte[]{(byte) 0xFF, 0x00, 0x00, 0x01, 0x02, 0x60, 0x00})
        );
    }

    @Test
    public void testTransceiveNullOrExceptionReturns6F00() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        IsoDepReader reader = new IsoDepReader(mock, null);

        // Null response from transceive -> returns 6F 00 instead of null
        mock.nextResponse = null;
        byte[] nullResp = reader.transmit(new byte[]{0x00, (byte) 0xA4, 0x04, 0x00, 0x00});
        assertArrayEquals(new byte[]{(byte) 0x6F, 0x00}, nullResp);

        // IOException from transceive -> returns 6F 00 instead of crashing VPCD session
        mock.throwIOException = true;
        byte[] errResp = reader.transmit(new byte[]{0x00, (byte) 0xA4, 0x04, 0x00, 0x00});
        assertArrayEquals(new byte[]{(byte) 0x6F, 0x00}, errResp);
    }

    @Test
    public void testDirectTransmitNullResponseReturns6300() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        IsoDepReader reader = new IsoDepReader(mock, null);
        mock.nextResponse = null;

        byte[] resp = reader.transmit(new byte[]{(byte) 0xFF, 0x00, 0x00, 0x00, 0x02, 0x60, 0x00});
        assertArrayEquals(new byte[]{(byte) 0x63, 0x00}, resp);
    }
}

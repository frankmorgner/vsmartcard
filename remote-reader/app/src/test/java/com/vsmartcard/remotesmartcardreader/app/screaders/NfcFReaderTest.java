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
import java.util.Arrays;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

public class NfcFReaderTest {

    private static class MockTransceiver implements NfcFReader.Transceiver {
        byte[] lastSent;
        byte[] nextResponse;
        boolean closed = false;
        int timeout = 0;

        @Override
        public byte[] transceive(byte[] data) throws IOException {
            this.lastSent = data != null ? data.clone() : null;
            return nextResponse != null ? nextResponse.clone() : new byte[0];
        }

        @Override
        public void close() throws IOException {
            this.closed = true;
        }

        @Override
        public void setTimeout(int timeout) {
            this.timeout = timeout;
        }
    }

    private static final byte[] TEST_IDM = new byte[]{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    private static final byte[] TEST_PMM = new byte[]{0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18};
    private static final byte[] TEST_SYS = new byte[]{0x00, 0x03};

    @Test
    public void testFelicaAtr() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        NfcFReader reader = new NfcFReader(mock, TEST_IDM, TEST_PMM, TEST_SYS, null);

        byte[] atr = reader.getATR();
        assertNotNull(atr);
        assertEquals(20, atr.length);

        // Header check: 3B 8F 80 01
        assertEquals((byte) 0x3B, atr[0]);
        assertEquals((byte) 0x8F, atr[1]);
        assertEquals((byte) 0x80, atr[2]);
        assertEquals((byte) 0x01, atr[3]);

        // Historical bytes category indicator: 80 4F 0C A0 00 00 03 06
        assertEquals((byte) 0x80, atr[4]);
        assertEquals((byte) 0x4F, atr[5]);
        assertEquals((byte) 0x0C, atr[6]);
        assertEquals((byte) 0xA0, atr[7]);
        assertEquals((byte) 0x00, atr[8]);
        assertEquals((byte) 0x00, atr[9]);
        assertEquals((byte) 0x03, atr[10]);
        assertEquals((byte) 0x06, atr[11]);

        // FeliCa identification: 11 00
        assertEquals((byte) 0x11, atr[12]);
        assertEquals((byte) 0x00, atr[13]);

        // TCK checksum verification
        byte tck = atr[1];
        for (int i = 2; i < atr.length - 1; i++) {
            tck ^= atr[i];
        }
        assertEquals(atr[atr.length - 1], tck);
        assertEquals((byte) 0x42, tck);
    }

    @Test
    public void testGetDataIdm() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        NfcFReader reader = new NfcFReader(mock, TEST_IDM, TEST_PMM, TEST_SYS, null);

        // FF CA 00 00 00: GET DATA - UID (IDm)
        byte[] apdu = new byte[]{(byte) 0xFF, (byte) 0xCA, 0x00, 0x00, 0x00};
        byte[] resp = reader.transmit(apdu);

        byte[] expected = new byte[10];
        System.arraycopy(TEST_IDM, 0, expected, 0, 8);
        expected[8] = (byte) 0x90;
        expected[9] = (byte) 0x00;
        assertArrayEquals(expected, resp);
    }

    @Test
    public void testGetDataPmm() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        NfcFReader reader = new NfcFReader(mock, TEST_IDM, TEST_PMM, TEST_SYS, null);

        // FF CA 01 00 00: GET DATA - PMm
        byte[] apdu = new byte[]{(byte) 0xFF, (byte) 0xCA, 0x01, 0x00, 0x00};
        byte[] resp = reader.transmit(apdu);

        byte[] expected = new byte[10];
        System.arraycopy(TEST_PMM, 0, expected, 0, 8);
        expected[8] = (byte) 0x90;
        expected[9] = (byte) 0x00;
        assertArrayEquals(expected, resp);
    }

    @Test
    public void testGetDataSystemCode() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        NfcFReader reader = new NfcFReader(mock, TEST_IDM, TEST_PMM, TEST_SYS, null);

        // FF CA 02 00 00: GET DATA - System Code
        byte[] apdu = new byte[]{(byte) 0xFF, (byte) 0xCA, 0x02, 0x00, 0x00};
        byte[] resp = reader.transmit(apdu);

        byte[] expected = new byte[4];
        System.arraycopy(TEST_SYS, 0, expected, 0, 2);
        expected[2] = (byte) 0x90;
        expected[3] = (byte) 0x00;
        assertArrayEquals(expected, resp);
    }

    @Test
    public void testSelectServiceCode() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        NfcFReader reader = new NfcFReader(mock, TEST_IDM, TEST_PMM, TEST_SYS, null);

        // Default should be 0x090F
        assertEquals(NfcFReader.DEFAULT_SERVICE_CODE, reader.getSelectedServiceCode());

        // Select Service 0x1A8B (Little endian: 8B 1A)
        byte[] apdu = new byte[]{(byte) 0xFF, (byte) 0xA4, 0x00, 0x01, 0x02, (byte) 0x8B, 0x1A};
        byte[] resp = reader.transmit(apdu);

        assertArrayEquals(new byte[]{(byte) 0x90, 0x00}, resp);
        assertEquals(0x1A8B, reader.getSelectedServiceCode());
    }

    @Test
    public void testReadBinarySuccess() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        NfcFReader reader = new NfcFReader(mock, TEST_IDM, TEST_PMM, TEST_SYS, null);

        // Mock FeliCa Read Without Encryption Response (0x07):
        // Length 29 = 1 + 1 + 8 + 1 + 1 + 1 + 16
        byte[] mockBlockData = new byte[]{
                0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x04, 0x00,
                0x05, 0x00, 0x06, 0x00, 0x07, 0x00, 0x08, 0x00
        };
        byte[] felicaResp = new byte[29];
        felicaResp[0] = 29;
        felicaResp[1] = 0x07; // Read response
        System.arraycopy(TEST_IDM, 0, felicaResp, 2, 8);
        felicaResp[10] = 0x00; // SF1
        felicaResp[11] = 0x00; // SF2
        felicaResp[12] = 0x01; // 1 block
        System.arraycopy(mockBlockData, 0, felicaResp, 13, 16);
        mock.nextResponse = felicaResp;

        // FF B0 00 00 10: READ BINARY block 0, 16 bytes
        byte[] apdu = new byte[]{(byte) 0xFF, (byte) 0xB0, 0x00, 0x00, 0x10};
        byte[] resp = reader.transmit(apdu);

        // Verify request sent to mock
        assertNotNull(mock.lastSent);
        assertEquals(16, mock.lastSent.length);
        assertEquals(0x10, mock.lastSent[0]); // Length 16
        assertEquals(0x06, mock.lastSent[1]); // Read Without Encryption command
        assertArrayEquals(TEST_IDM, Arrays.copyOfRange(mock.lastSent, 2, 10));
        assertEquals(0x01, mock.lastSent[10]); // 1 service
        assertEquals(0x0F, mock.lastSent[11]); // Service Code low (0x090F -> 0x0F)
        assertEquals(0x09, mock.lastSent[12]); // Service Code high (0x090F -> 0x09)
        assertEquals(0x01, mock.lastSent[13]); // 1 block
        assertEquals((byte) 0x80, mock.lastSent[14]); // 2-byte block list element
        assertEquals(0x00, mock.lastSent[15]); // Block 0

        // Verify response returned to PC/SC: 16 bytes block data + 90 00
        byte[] expected = new byte[18];
        System.arraycopy(mockBlockData, 0, expected, 0, 16);
        expected[16] = (byte) 0x90;
        expected[17] = (byte) 0x00;
        assertArrayEquals(expected, resp);
    }

    @Test
    public void testReadBinaryErrorReturnsGracefulZeroBlock() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        NfcFReader reader = new NfcFReader(mock, TEST_IDM, TEST_PMM, TEST_SYS, null);

        // Mock FeliCa error response: SF1 = 0xA1 (out of range or unwritten block)
        byte[] felicaResp = new byte[13];
        felicaResp[0] = 13;
        felicaResp[1] = 0x07;
        System.arraycopy(TEST_IDM, 0, felicaResp, 2, 8);
        felicaResp[10] = (byte) 0xA1;
        felicaResp[11] = 0x00;
        mock.nextResponse = felicaResp;

        byte[] apdu = new byte[]{(byte) 0xFF, (byte) 0xB0, 0x00, 0x00, 0x10};
        byte[] resp = reader.transmit(apdu);

        // When card returns SF error, NfcFReader returns 16 zero bytes + 90 00
        // so TransiticViewer's loop can continue through all blocks without aborting
        byte[] expected = new byte[18];
        expected[16] = (byte) 0x90;
        expected[17] = (byte) 0x00;
        assertArrayEquals(expected, resp);
    }

    @Test
    public void testWriteBinarySuccess() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        NfcFReader reader = new NfcFReader(mock, TEST_IDM, TEST_PMM, TEST_SYS, null);

        // Mock FeliCa Write Without Encryption Response (0x09):
        byte[] felicaResp = new byte[12];
        felicaResp[0] = 12;
        felicaResp[1] = 0x09;
        System.arraycopy(TEST_IDM, 0, felicaResp, 2, 8);
        felicaResp[10] = 0x00; // SF1
        felicaResp[11] = 0x00; // SF2
        mock.nextResponse = felicaResp;

        byte[] dataToWrite = new byte[16];
        dataToWrite[0] = (byte) 0xAA;
        byte[] apdu = new byte[5 + 16];
        apdu[0] = (byte) 0xFF;
        apdu[1] = (byte) 0xD6;
        apdu[2] = 0x00;
        apdu[3] = 0x03; // block 3
        apdu[4] = 0x10; // 16 bytes
        System.arraycopy(dataToWrite, 0, apdu, 5, 16);

        byte[] resp = reader.transmit(apdu);
        assertArrayEquals(new byte[]{(byte) 0x90, 0x00}, resp);

        // Verify sent packet
        assertNotNull(mock.lastSent);
        assertEquals(32, mock.lastSent.length);
        assertEquals(0x20, mock.lastSent[0]);
        assertEquals(0x08, mock.lastSent[1]); // Write Without Encryption command
        assertEquals(0x03, mock.lastSent[15]); // block 3
        assertEquals((byte) 0xAA, mock.lastSent[16]);
    }

    @Test
    public void testDirectTransparentTransmit() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        NfcFReader reader = new NfcFReader(mock, TEST_IDM, TEST_PMM, TEST_SYS, null);

        mock.nextResponse = new byte[]{0x05, 0x01, 0x02, 0x03, 0x04};

        // FF 00 00 00 04 [01 02 03 04]
        byte[] apdu = new byte[]{(byte) 0xFF, 0x00, 0x00, 0x00, 0x04, 0x01, 0x02, 0x03, 0x04};
        byte[] resp = reader.transmit(apdu);

        assertArrayEquals(new byte[]{0x01, 0x02, 0x03, 0x04}, mock.lastSent);
        assertArrayEquals(new byte[]{0x05, 0x01, 0x02, 0x03, 0x04, (byte) 0x90, 0x00}, resp);
    }

    @Test
    public void testRawPacket() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        NfcFReader reader = new NfcFReader(mock, TEST_IDM, TEST_PMM, TEST_SYS, null);

        mock.nextResponse = new byte[]{0x06, 0x01, 0x02, 0x03, 0x04, 0x05};

        // Raw packet where byte 0 == length (6)
        byte[] raw = new byte[]{0x06, 0x00, (byte) 0xFF, (byte) 0xFF, 0x00, 0x00};
        byte[] resp = reader.transmit(raw);

        assertArrayEquals(raw, mock.lastSent);
        assertArrayEquals(mock.nextResponse, resp);
    }

    @Test
    public void testIsoProbeApdu() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        NfcFReader reader = new NfcFReader(mock, TEST_IDM, TEST_PMM, TEST_SYS, null);

        // Select MF (00 A4 00 0C)
        byte[] resp = reader.transmit(new byte[]{0x00, (byte) 0xA4, 0x00, 0x0C});
        assertArrayEquals(new byte[]{(byte) 0x90, 0x00}, resp);
    }

    @Test
    public void testEjectClosesTransceiver() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        NfcFReader reader = new NfcFReader(mock, TEST_IDM, TEST_PMM, TEST_SYS, null);

        reader.eject();
        assertTrue(mock.closed);
    }

    @Test
    public void testReadWithoutEncryptionSafeOnIOException() throws IOException {
        // Transceiver throws IOException during transceive
        NfcFReader.Transceiver failingTransceiver = new NfcFReader.Transceiver() {
            @Override
            public byte[] transceive(byte[] data) throws IOException {
                throw new IOException("Simulated tag communication error");
            }

            @Override
            public void close() throws IOException {
            }

            @Override
            public void setTimeout(int timeout) {
            }
        };

        NfcFReader reader = new NfcFReader(failingTransceiver, TEST_IDM, TEST_PMM, TEST_SYS, null);

        // FF B0 00 00 10: READ BINARY
        byte[] apdu = new byte[]{(byte) 0xFF, (byte) 0xB0, 0x00, 0x00, 0x10};
        byte[] resp = reader.transmit(apdu);

        // Returns graceful zero block + 90 00 and does NOT throw IOException or break VPCD
        byte[] expected = new byte[18];
        expected[16] = (byte) 0x90;
        expected[17] = (byte) 0x00;
        assertArrayEquals(expected, resp);
    }

    @Test
    public void testGetDataInvalidP2Returns6A86() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        NfcFReader reader = new NfcFReader(mock, TEST_IDM, TEST_PMM, TEST_SYS, null);

        // FF CA 00 01 00: P2 is not 00
        byte[] apdu = new byte[]{(byte) 0xFF, (byte) 0xCA, 0x00, 0x01, 0x00};
        byte[] resp = reader.transmit(apdu);
        assertArrayEquals(new byte[]{(byte) 0x6A, (byte) 0x86}, resp);
    }

    @Test
    public void testGetDataUnsupportedP1Returns6A81() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        NfcFReader reader = new NfcFReader(mock, TEST_IDM, TEST_PMM, TEST_SYS, null);

        // FF CA 05 00 00: P1 is unsupported
        byte[] apdu = new byte[]{(byte) 0xFF, (byte) 0xCA, 0x05, 0x00, 0x00};
        byte[] resp = reader.transmit(apdu);
        assertArrayEquals(new byte[]{(byte) 0x6A, (byte) 0x81}, resp);
    }

    @Test
    public void testSelectServiceCodeInvalidLengthReturns6700() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        NfcFReader reader = new NfcFReader(mock, TEST_IDM, TEST_PMM, TEST_SYS, null);

        // FF A4 00 01 01 0F: Lc is only 1 byte (requires 2 bytes)
        byte[] apdu = new byte[]{(byte) 0xFF, (byte) 0xA4, 0x00, 0x01, 0x01, 0x0F};
        byte[] resp = reader.transmit(apdu);
        assertArrayEquals(new byte[]{(byte) 0x67, 0x00}, resp);
    }

    @Test
    public void testWriteBinaryInvalidLengthReturns6700() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        NfcFReader reader = new NfcFReader(mock, TEST_IDM, TEST_PMM, TEST_SYS, null);

        // FF D6 00 00 08 [8 bytes only]: requires at least 16 bytes
        byte[] apdu = new byte[]{(byte) 0xFF, (byte) 0xD6, 0x00, 0x00, 0x08, 1, 2, 3, 4, 5, 6, 7, 8};
        byte[] resp = reader.transmit(apdu);
        assertArrayEquals(new byte[]{(byte) 0x67, 0x00}, resp);
    }

    @Test
    public void testTransparentTransmitInvalidLengthReturns6700() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        NfcFReader reader = new NfcFReader(mock, TEST_IDM, TEST_PMM, TEST_SYS, null);

        // FF 00 00 00 05 [only 2 bytes provided]: Length mismatch
        byte[] apdu = new byte[]{(byte) 0xFF, 0x00, 0x00, 0x00, 0x05, 1, 2};
        byte[] resp = reader.transmit(apdu);
        assertArrayEquals(new byte[]{(byte) 0x67, 0x00}, resp);
    }

    @Test
    public void testNullOrEmptyApduReturns6700() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        NfcFReader reader = new NfcFReader(mock, TEST_IDM, TEST_PMM, TEST_SYS, null);

        assertArrayEquals(new byte[]{(byte) 0x67, 0x00}, reader.transmit(null));
        assertArrayEquals(new byte[]{(byte) 0x67, 0x00}, reader.transmit(new byte[0]));
    }

    @Test
    public void testReadBinaryOutOfBoundsBlockReturns6A82() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        NfcFReader reader = new NfcFReader(mock, TEST_IDM, TEST_PMM, TEST_SYS, null);

        // Block 256 (P1=0x01, P2=0x00): exceeds 1-byte block list address space (0..255)
        byte[] apdu1 = new byte[]{(byte) 0xFF, (byte) 0xB0, 0x01, 0x00, 0x10};
        assertArrayEquals(new byte[]{(byte) 0x6A, (byte) 0x82}, reader.transmit(apdu1));

        // Start block 250, Le=160 (10 blocks -> end block 259 > 255)
        byte[] apdu2 = new byte[]{(byte) 0xFF, (byte) 0xB0, 0x00, (byte) 0xFA, (byte) 0xA0};
        assertArrayEquals(new byte[]{(byte) 0x6A, (byte) 0x82}, reader.transmit(apdu2));
    }

    @Test
    public void testWriteBinaryOutOfBoundsBlockReturns6A82() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        NfcFReader reader = new NfcFReader(mock, TEST_IDM, TEST_PMM, TEST_SYS, null);

        // Block 256 (P1=0x01, P2=0x00) with 16 bytes data
        byte[] apdu = new byte[5 + 16];
        apdu[0] = (byte) 0xFF;
        apdu[1] = (byte) 0xD6;
        apdu[2] = 0x01;
        apdu[3] = 0x00;
        apdu[4] = 0x10;
        assertArrayEquals(new byte[]{(byte) 0x6A, (byte) 0x82}, reader.transmit(apdu));
    }

    @Test
    public void testReadMultipleBlocks() throws IOException {
        MockTransceiver mock = new MockTransceiver();
        NfcFReader reader = new NfcFReader(mock, TEST_IDM, TEST_PMM, TEST_SYS, null);

        // 2 blocks = 32 bytes data
        byte[] mockBlockData = new byte[32];
        mockBlockData[0] = 0x11;
        mockBlockData[16] = 0x22;

        byte[] felicaResp = new byte[13 + 32];
        felicaResp[0] = (byte) (13 + 32);
        felicaResp[1] = 0x07;
        System.arraycopy(TEST_IDM, 0, felicaResp, 2, 8);
        felicaResp[10] = 0x00;
        felicaResp[11] = 0x00;
        felicaResp[12] = 0x02; // 2 blocks
        System.arraycopy(mockBlockData, 0, felicaResp, 13, 32);
        mock.nextResponse = felicaResp;

        // FF B0 00 05 20: read 2 blocks starting at block 5
        byte[] apdu = new byte[]{(byte) 0xFF, (byte) 0xB0, 0x00, 0x05, 0x20};
        byte[] resp = reader.transmit(apdu);

        // Check request sent to mock
        assertNotNull(mock.lastSent);
        assertEquals(14 + 2 * 2, mock.lastSent.length); // 18 bytes
        assertEquals(0x02, mock.lastSent[13]); // numBlocks = 2
        assertEquals(0x05, mock.lastSent[15]); // block 5
        assertEquals(0x06, mock.lastSent[17]); // block 6

        // Check response returned to PC/SC: 32 bytes data + 90 00
        byte[] expected = new byte[34];
        System.arraycopy(mockBlockData, 0, expected, 0, 32);
        expected[32] = (byte) 0x90;
        expected[33] = (byte) 0x00;
        assertArrayEquals(expected, resp);
    }
}

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

import java.io.IOException;

public interface SCReader {

    void eject() throws IOException;

    void powerOn() throws IOException;
    void powerOff() throws IOException;
    void reset() throws IOException;

    /**
     * Receive ATR from the smart card.
     *
     * @return ATR on success or null in case of an error
     */
    byte[] getATR();

    /**
     * Send an APDU to the smart card
     *
     * @param apdu Data to be sent
     * @return response data or null in case of an error
     */
    byte[] transmit(byte[] apdu) throws IOException;
}
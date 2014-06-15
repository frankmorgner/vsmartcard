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

import android.os.Handler;
import android.os.Message;

public class MessageSender {
    public static final int MESSAGE_CONNECTED = 0;
    public static final int MESSAGE_DISCONNECTED = 1;
    public static final int MESSAGE_ERROR = 2;
    public static final int MESSAGE_ATR = 3;
    public static final int MESSAGE_ON = 4;
    public static final int MESSAGE_OFF = 5;
    public static final int MESSAGE_RESET = 6;
    public static final int MESSAGE_CAPDU = 7;
    public static final int MESSAGE_RAPDU = 8;
    private Handler handler;

    public MessageSender(Handler handler) {
        this.handler = handler;
    }

    private void sendMessage(int messageCode, Object object) {
        Message message = handler.obtainMessage(messageCode, object);
        handler.sendMessage(message);
    }

    public void sendConnected(String details) {
        sendMessage(MESSAGE_CONNECTED, details);
    }

    public void sendATR(String details) {
        sendMessage(MESSAGE_ATR, details);
    }

    public void sendCAPDU(String details) {
        sendMessage(MESSAGE_CAPDU, details);
    }

    public void sendDisconnected(String details) {
        sendMessage(MESSAGE_DISCONNECTED, details);
    }

    public void sendError(String details) {
        sendMessage(MESSAGE_ERROR, details);
    }

    public void sendOff() {
        sendMessage(MESSAGE_OFF, "");
    }

    public void sendOn() {
        sendMessage(MESSAGE_ON, "");
    }

    public void sendRAPDU(String details) {
        sendMessage(MESSAGE_RAPDU, details);
    }

    public void sendReset() {
        sendMessage(MESSAGE_RESET, "");
    }
}

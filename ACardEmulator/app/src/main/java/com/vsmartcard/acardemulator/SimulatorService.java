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
import android.os.StrictMode;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;

import com.licel.jcardsim.base.Simulator;
import com.licel.jcardsim.base.SimulatorRuntime;
import com.licel.jcardsim.samples.HelloWorldApplet;
import com.licel.jcardsim.utils.AIDUtil;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.Socket;

import openpgpcard.OpenPGPApplet;
import pkgYkneoOath.YkneoOath;
import net.pwendland.javacard.pki.isoapplet.IsoApplet;
import com.mysmartlogon.gidsApplet.GidsApplet;

public class SimulatorService extends HostApduService {

    private static final boolean useVPCD = false;
    public static final int DEFAULT_PORT = 35963;
    private static final String hostname = "192.168.42.158";
    private int port = DEFAULT_PORT;

    private static Socket socket;
    private static InputStream inputStream;
    private static OutputStream outputStream;

    public static final String TAG = "com.vsmartcard.acardemulator.SimulatorService";
    public static final String EXTRA_CAPDU = "MSG_CAPDU";
    public static final String EXTRA_RAPDU = "MSG_RAPDU";
    public static final String EXTRA_ERROR = "MSG_ERROR";
    public static final String EXTRA_DESELECT = "MSG_DESELECT";
    public static final String EXTRA_INSTALL = "MSG_INSTALL";

    private static Simulator simulator = null;

    private void createSimulator() {
        String aid, name, extra_install = "", extra_error = "";
        simulator = new Simulator(new SimulatorRuntime());

        name = getResources().getString(R.string.applet_helloworld);
        aid = getResources().getString(R.string.aid_helloworld);
        try {
            simulator.installApplet(AIDUtil.create(aid), HelloWorldApplet.class);
            extra_install += "\n" + name + " (AID: " + aid + ")";
        } catch (Exception e) {
            e.printStackTrace();
            extra_error += "\n" + "Could not install " + name + " (AID: " + aid + ")";
        }

        name = getResources().getString(R.string.applet_openpgp);
        aid = getResources().getString(R.string.aid_openpgp);
        try {
            byte[] aid_bytes = Util.hexStringToByteArray(aid);
            byte[] inst_params = new byte[aid.length()+1];
            inst_params[0] = (byte) aid_bytes.length;
            System.arraycopy(aid_bytes, 0, inst_params, 1, aid_bytes.length);
            simulator.installApplet(AIDUtil.create(aid), OpenPGPApplet.class, inst_params, (short) 0, (byte) inst_params.length);
            extra_install += "\n" + name + " (AID: " + aid + ")";
        } catch (Exception e) {
            e.printStackTrace();
            extra_error += "\n" + "Could not install " + name + " (AID: " + aid + ")";
        }

        name = getResources().getString(R.string.applet_oath);
        aid = getResources().getString(R.string.aid_oath);
        try {
            byte[] aid_bytes = Util.hexStringToByteArray(aid);
            byte[] inst_params = new byte[aid.length()+1];
            inst_params[0] = (byte) aid_bytes.length;
            System.arraycopy(aid_bytes, 0, inst_params, 1, aid_bytes.length);
            simulator.installApplet(AIDUtil.create(aid), YkneoOath.class, inst_params, (short) 0, (byte) inst_params.length);
            extra_install += "\n" + name + " (AID: " + aid + ")";
        } catch (Exception e) {
            e.printStackTrace();
            extra_error += "\n" + "Could not install " + name + " (AID: " + aid + ")";
        }

        name = getResources().getString(R.string.applet_isoapplet);
        aid = getResources().getString(R.string.aid_isoapplet);
        try {
            simulator.installApplet(AIDUtil.create(aid), IsoApplet.class);
            extra_install += "\n" + name + " (AID: " + aid + ")";
        } catch (Exception e) {
            e.printStackTrace();
            extra_error += "\n" + "Could not install " + name + " (AID: " + aid + ")";
        }

        name = getResources().getString(R.string.applet_gidsapplet);
        aid = getResources().getString(R.string.aid_gidsapplet);
        try {
            simulator.installApplet(AIDUtil.create(aid), GidsApplet.class);
            extra_install += "\n" + name + " (AID: " + aid + ")";
        } catch (Exception e) {
            e.printStackTrace();
            extra_error += "\n" + "Could not install " + name + " (AID: " + aid + ")";
        }

        Intent i = new Intent(TAG);
        if (!extra_error.isEmpty())
            i.putExtra(EXTRA_ERROR, extra_error);
        if (!extra_install.isEmpty())
            i.putExtra(EXTRA_INSTALL, extra_install);
        LocalBroadcastManager.getInstance(this).sendBroadcast(i);
    }

    @Override
    public void onCreate () {
        super.onCreate();

        Log.d("", "Begin transaction");
        if (useVPCD) {
            if  (socket == null) {
                try {
                    if (android.os.Build.VERSION.SDK_INT > 9) {
                        StrictMode.ThreadPolicy policy = new StrictMode.ThreadPolicy.Builder().permitAll().build();
                        StrictMode.setThreadPolicy(policy);
                    }
                    vpcdConnect();
                    sendPowerOn();
                } catch (IOException e) {
                    e.printStackTrace();
                    vpcdDisconnect();
                }
            }
        } else {
            if (simulator == null)
                createSimulator();
        }
    }

    @Override
    public byte[] processCommandApdu(byte[] capdu, Bundle extras) {
        Intent i = new Intent(TAG);
        String extra_error = "";
        byte[] rapdu = null;

        i.putExtra(EXTRA_CAPDU, Util.byteArrayToHexString(capdu));
        try {
            if (useVPCD) {
                rapdu = transmit(capdu);
            } else {
                rapdu = simulator.transmitCommand(capdu);
            }
            if (rapdu != null)
                i.putExtra(EXTRA_RAPDU, Util.byteArrayToHexString(rapdu));
        } catch (Exception e) {
            e.printStackTrace();
            vpcdDisconnect();
            extra_error += "Internal error";
        }

        if (!extra_error.isEmpty())
            i.putExtra(EXTRA_ERROR, extra_error);

        LocalBroadcastManager.getInstance(this).sendBroadcast(i);

        return rapdu;
    }

    @Override
    public void onDeactivated(int reason) {
        Intent i = new Intent(TAG);

        Log.d("", "End transaction");
        switch (reason) {
            case DEACTIVATION_LINK_LOSS:
                i.putExtra(EXTRA_DESELECT, "link lost");
                if (useVPCD)
                    try {
                        sendPowerOff();
                    } catch (IOException e) {
                        e.printStackTrace();
                        vpcdDisconnect();
                    }
                break;
            case DEACTIVATION_DESELECTED:
                i.putExtra(EXTRA_DESELECT, "deactivated");
                if (useVPCD)
                    try {
                        sendReset();
                    } catch (IOException e) {
                        e.printStackTrace();
                        vpcdDisconnect();
                    }
                break;
        }
        if (useVPCD) {
            //vpcdDisconnect();
        }

        LocalBroadcastManager.getInstance(this).sendBroadcast(i);
    }

    private byte[] receiveFromVPCD() throws IOException {
        /* convert length from network byte order.
        Note that Java always uses network byte order internally. */
        int length1 = inputStream.read();
        int length2 = inputStream.read();
        if (length1 == -1 || length2 == -1) {
            // EOF
            return null;
        }
        int length = (length1 << 8) + length2;

        byte[] data = new byte[length];

        int offset = 0;
        while (length > 0) {
            int read = inputStream.read(data, offset, length);
            if (read == -1) {
                // EOF
                return null;
            }
            offset += read;
            length -= read;
        }

        return data;
    }

    private byte[] transmit(byte[] capdu) throws IOException {
        sendToVPCD(capdu);
        return receiveFromVPCD();
    }

    private void sendPowerOff() throws IOException {
        Log.d("", "Power Off");
        sendToVPCD(new byte[] {0x00});
    }
    private void sendPowerOn() throws IOException {
        Log.d("", "Power On");
        sendToVPCD(new byte[] {0x01});
    }
    private void sendReset() throws IOException {
        Log.d("", "Reset");
        sendToVPCD(new byte[] {0x02});
    }

    private void sendToVPCD(byte[] data) throws IOException {
        /* convert length to network byte order.
        Note that Java always uses network byte order internally. */
        byte[] length = new byte[2];
        length[0] = (byte) (data.length >> 8);
        length[1] = (byte) (data.length & 0xff);
        outputStream.write(length);

        outputStream.write(data, 0, data.length);

        outputStream.flush();
    }

    private void vpcdConnect() throws IOException {
        Log.d("", "Connecting to " + hostname + ":" + Integer.toString(port));
        socket = new Socket(InetAddress.getByName(hostname), port);
        outputStream = socket.getOutputStream();
        inputStream = socket.getInputStream();
    }

    private void vpcdDisconnect() {
        Log.d("", "Disconnecting");
        if  (socket != null) {
            try {
                socket.close();
            } catch (IOException e) {
                // ignore
            } finally {
                socket = null;
            }
        }
    }
}

package com.vsmartcard.acardemulator.emulators;

import android.content.Context;
import android.content.Intent;
import android.support.v4.content.LocalBroadcastManager;

import com.licel.jcardsim.base.Simulator;
import com.licel.jcardsim.base.SimulatorRuntime;
import com.licel.jcardsim.samples.HelloWorldApplet;
import com.licel.jcardsim.utils.AIDUtil;
import com.mysmartlogon.gidsApplet.GidsApplet;
import com.vsmartcard.acardemulator.R;
import com.vsmartcard.acardemulator.Util;

import net.pwendland.javacard.pki.isoapplet.IsoApplet;

import openpgpcard.OpenPGPApplet;
import pkgYkneoOath.YkneoOath;

public class JCEmulator implements Emulator {

    private static Simulator simulator = null;

    public JCEmulator(
            Context context,
            boolean activate_helloworld,
            boolean activate_openpgp,
            boolean activate_oath,
            boolean activate_isoapplet,
            boolean activate_gidsapplet) {
        String aid, name, extra_install = "", extra_error = "";
        simulator = new Simulator(new SimulatorRuntime());

        if (activate_helloworld) {
            name = context.getResources().getString(R.string.applet_helloworld);
            aid = context.getResources().getString(R.string.aid_helloworld);
            try {
                simulator.installApplet(AIDUtil.create(aid), HelloWorldApplet.class);
                extra_install += "\n" + name + " (AID: " + aid + ")";
            } catch (Exception e) {
                e.printStackTrace();
                extra_error += "\n" + "Could not install " + name + " (AID: " + aid + ")";
            }
        }

        if (activate_openpgp) {
            name = context.getResources().getString(R.string.applet_openpgp);
            aid = context.getResources().getString(R.string.aid_openpgp);
            try {
                byte[] aid_bytes = Util.hexStringToByteArray(aid);
                byte[] inst_params = new byte[aid.length() + 1];
                inst_params[0] = (byte) aid_bytes.length;
                System.arraycopy(aid_bytes, 0, inst_params, 1, aid_bytes.length);
                simulator.installApplet(AIDUtil.create(aid), OpenPGPApplet.class, inst_params, (short) 0, (byte) inst_params.length);
                extra_install += "\n" + name + " (AID: " + aid + ")";
            } catch (Exception e) {
                e.printStackTrace();
                extra_error += "\n" + "Could not install " + name + " (AID: " + aid + ")";
            }
        }

        if (activate_oath) {
            name = context.getResources().getString(R.string.applet_oath);
            aid = context.getResources().getString(R.string.aid_oath);
            try {
                byte[] aid_bytes = Util.hexStringToByteArray(aid);
                byte[] inst_params = new byte[aid.length() + 1];
                inst_params[0] = (byte) aid_bytes.length;
                System.arraycopy(aid_bytes, 0, inst_params, 1, aid_bytes.length);
                simulator.installApplet(AIDUtil.create(aid), YkneoOath.class, inst_params, (short) 0, (byte) inst_params.length);
                extra_install += "\n" + name + " (AID: " + aid + ")";
            } catch (Exception e) {
                e.printStackTrace();
                extra_error += "\n" + "Could not install " + name + " (AID: " + aid + ")";
            }
        }

        if (activate_isoapplet) {
            name = context.getResources().getString(R.string.applet_isoapplet);
            aid = context.getResources().getString(R.string.aid_isoapplet);
            try {
                simulator.installApplet(AIDUtil.create(aid), IsoApplet.class);
                extra_install += "\n" + name + " (AID: " + aid + ")";
            } catch (Exception e) {
                e.printStackTrace();
                extra_error += "\n" + "Could not install " + name + " (AID: " + aid + ")";
            }
        }

        if (activate_gidsapplet) {
            name = context.getResources().getString(R.string.applet_gidsapplet);
            aid = context.getResources().getString(R.string.aid_gidsapplet);
            try {
                simulator.installApplet(AIDUtil.create(aid), GidsApplet.class);
                extra_install += "\n" + name + " (AID: " + aid + ")";
            } catch (Exception e) {
                e.printStackTrace();
                extra_error += "\n" + "Could not install " + name + " (AID: " + aid + ")";
            }
        }

        Intent i = new Intent(EmulatorSingleton.TAG);
        if (!extra_error.isEmpty())
            i.putExtra(EmulatorSingleton.EXTRA_ERROR, extra_error);
        if (!extra_install.isEmpty())
            i.putExtra(EmulatorSingleton.EXTRA_INSTALL, extra_install);
        LocalBroadcastManager.getInstance(context).sendBroadcast(i);
    }

    public void destroy() {
        if (simulator != null) {
            simulator.reset();
            simulator = null;
        }
    }

    public byte[] process(byte[] commandAPDU) {
        return simulator.transmitCommand(commandAPDU);
    }

    public void deactivate() {
        simulator.reset();
    }
}
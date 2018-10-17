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

import android.annotation.TargetApi;
import android.app.PendingIntent;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.nfc.NfcAdapter;
import android.nfc.Tag;
import android.os.Build;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;

import com.example.android.common.logger.Log;
import com.example.android.common.logger.LogFragment;
import com.example.android.common.logger.LogWrapper;
import com.example.android.common.logger.MessageOnlyLogFilter;

import com.vsmartcard.remotesmartcardreader.app.screaders.*;

@TargetApi(Build.VERSION_CODES.KITKAT)
public class MainActivity extends AppCompatActivity implements NfcAdapter.ReaderCallback {

    private VPCDWorker vpcdTest;
    private AlertDialog dialog;
    private int oldOrientation;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        FloatingActionButton fab = (FloatingActionButton) findViewById(R.id.fab);
        assert fab != null;
        fab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (vpcdTest != null && !vpcdTest.isCancelled()) {
                    Snackbar.make(view, "Disconnecting from VPCD...", Snackbar.LENGTH_LONG)
                            .setAction("Action", null).show();
                    vpcdDisconnect();
                } else {
                    Snackbar.make(view, "Testing with dummy card...", Snackbar.LENGTH_LONG)
                        .setAction("Action", null).show();
                    vpcdConnect(new DummyReader());
                }
            }
        });
        PendingIntent.getActivity(this, 0, new Intent(this, this.getClass()).addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP), 0);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            startActivity(new Intent(this, SettingsActivity.class));
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    @Override
    protected  void onStart() {
        super.onStart();
        initializeLogging();
    }

    /** Create a chain of targets that will receive log data */
    private void initializeLogging() {
        // Wraps Android's native log framework.
        LogWrapper logWrapper = new LogWrapper();
        // Using Log, front-end to the logging chain, emulates android.util.log method signatures.
        Log.setLogNode(logWrapper);

        // Filter strips out everything except the message text.
        MessageOnlyLogFilter msgFilter = new MessageOnlyLogFilter();
        logWrapper.setNext(msgFilter);

        // On screen logging via a fragment with a TextView.
        LogFragment logFragment = (LogFragment) getSupportFragmentManager()
                .findFragmentById(R.id.log_fragment);
        msgFilter.setNext(logFragment.getLogView());
    }

    @Override
    public void onPause() {
        super.onPause();
        vpcdDisconnect();
        disableReaderMode();
    }

    private void enableReaderMode() {
        NfcAdapter adapter = NfcAdapter.getDefaultAdapter(this);
        if (!adapter.isEnabled()) {
            if (dialog == null) {
                dialog = new AlertDialog.Builder(this)
                        .setMessage("NFC is required to communicate with a contactless smart card. Do you want to enable NFC now?")
                        .setTitle("Enable NFC")
                        .setPositiveButton(android.R.string.yes, new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int id) {
                                startActivity(new Intent(Settings.ACTION_NFC_SETTINGS));
                            }
                        })
                        .setNegativeButton(android.R.string.no, new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int id) {
                            }
                        }).create();
            }
            dialog.show();
        }

        // avoid re-starting the App and loosing the tag by rotating screen
        oldOrientation = getRequestedOrientation();
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_NOSENSOR);

        SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(this);
        int timeout = Integer.parseInt(SP.getString("delay", "500"));
        Bundle bundle = new Bundle();
        bundle.putInt(NfcAdapter.EXTRA_READER_PRESENCE_CHECK_DELAY, timeout * 10);
        adapter.enableReaderMode(this, this,
                NfcAdapter.FLAG_READER_NFC_A | NfcAdapter.FLAG_READER_NFC_B | NfcAdapter.FLAG_READER_SKIP_NDEF_CHECK,
                bundle);
    }

    private void disableReaderMode() {
        if (dialog != null) {
            dialog.dismiss();
        }

        setRequestedOrientation(oldOrientation);

        NfcAdapter nfc = NfcAdapter.getDefaultAdapter(this);
        if (nfc != null) {
            nfc.disableReaderMode(this);
        }
    }

    @Override
    public void onTagDiscovered(Tag tag) {
        vpcdDisconnect();
        String[] techList = tag.getTechList();
        for (String aTechList : techList) {
            if (aTechList.equals("android.nfc.tech.NfcA")) {
                Log.i(getClass().getName(), "Discovered ISO/IEC 14443-A tag");
            } else if (aTechList.equals("android.nfc.tech.NfcB")) {
                Log.i(getClass().getName(), "Discovered ISO/IEC 14443-B tag");
            }
        }
        NFCReader nfcReader = NFCReader.get(tag, this);
        if (nfcReader != null) {
            vpcdConnect(nfcReader);
        }
    }

    private void vpcdConnect(SCReader scReader) {
        SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(this);
        int port = Integer.parseInt(SP.getString("port", Integer.toString(VPCDWorker.DEFAULT_PORT)));
        String hostname = SP.getString("hostname", VPCDWorker.DEFAULT_HOSTNAME);
        vpcdTest = new VPCDWorker();
        vpcdTest.execute(new VPCDWorker.VPCDWorkerParams(hostname, port, scReader));
    }

    private void vpcdDisconnect() {
        if (vpcdTest != null) {
            vpcdTest.cancel(true);
            vpcdTest = null;
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        Intent intent = getIntent();
        // Check to see that the Activity started due to a discovered tag
        if (NfcAdapter.ACTION_TECH_DISCOVERED.equals(intent.getAction())) {
            vpcdDisconnect();
            NFCReader nfcReader = NFCReader.get(intent, this);
            if (nfcReader != null) {
                vpcdConnect(nfcReader);
            } else {
                super.onNewIntent(intent);
            }
        }
        enableReaderMode();
    }

    @Override
    public void onNewIntent(Intent intent) {
        // onResume gets called after this to handle the intent
        setIntent(intent);
    }

}

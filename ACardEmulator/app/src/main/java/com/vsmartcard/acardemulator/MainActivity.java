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

import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v4.content.LocalBroadcastManager;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import com.vsmartcard.acardemulator.emulators.EmulatorSingleton;


public class MainActivity extends AppCompatActivity {

    private TextView textViewVPCDStatus;
    private BroadcastReceiver bReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals(EmulatorSingleton.TAG)) {
                String capdu = intent.getStringExtra(EmulatorSingleton.EXTRA_CAPDU);
                String rapdu = intent.getStringExtra(EmulatorSingleton.EXTRA_RAPDU);
                String error = intent.getStringExtra(EmulatorSingleton.EXTRA_ERROR);
                String deselect = intent.getStringExtra(EmulatorSingleton.EXTRA_DESELECT);
                String install = intent.getStringExtra(EmulatorSingleton.EXTRA_INSTALL);
                if (install != null)
                    textViewVPCDStatus.append(getResources().getString(R.string.status_install) + ":" + install + "\n");
                if (capdu != null)
                    textViewVPCDStatus.append(getResources().getString(R.string.status_capdu) + ": " + capdu + "\n");
                if (error != null)
                    textViewVPCDStatus.append(getResources().getString(R.string.status_error) + ": " + error + "\n");
                if (rapdu != null) {
                    textViewVPCDStatus.append(getResources().getString(R.string.status_rapdu) + ": " + rapdu + "\n");
                }
                if (deselect != null)
                    textViewVPCDStatus.append(getResources().getString(R.string.status_disconnected) + ": " + deselect + "\n");
            }
        }
    };
    private AlertDialog dialog;
    final String PREFS = "ACardEmulatorPrefs";
    final String PREF_LASTVERSION = "last version";
    private static String SAVED_STATUS = "textViewVPCDStatus";

    private void showStartupMessage() {
        if (dialog == null)
            dialog = new AlertDialog.Builder(this)
                    .setMessage(R.string.startup_message)
                    .setTitle(R.string.startup_title)
                    .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int id) {
                        }
                    }).create();
        dialog.show();
    }

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
                SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
                String emulator = SP.getString("emulator", "");
                if (emulator != getString(R.string.vicc)) {
                    Snackbar.make(view, "Scheduled re-installation of all applets...", Snackbar.LENGTH_LONG)
                            .setAction("Action", null).show();
                    EmulatorSingleton.destroyEmulator();
                }
            }
        });

        textViewVPCDStatus = (TextView) findViewById(R.id.textViewLog);
        SharedPreferences settings = getSharedPreferences(PREFS, 0);
        if (settings.getInt(PREF_LASTVERSION, 0) != BuildConfig.VERSION_CODE) {
            showStartupMessage();
            settings.edit().putInt(PREF_LASTVERSION, BuildConfig.VERSION_CODE).commit();
        }

        LocalBroadcastManager bManager = LocalBroadcastManager.getInstance(this);
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(EmulatorSingleton.TAG);
        bManager.registerReceiver(bReceiver, intentFilter);

        Intent serviceIntent = new Intent(this.getApplicationContext(), SmartcardProviderService.class);
        startService(serviceIntent);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle presses on the action bar items
        switch (item.getItemId()) {
            case R.id.action_copy:
                // Code to Copy the content of Text View to the Clip board.
                ClipboardManager clipboard = (ClipboardManager) getSystemService(CLIPBOARD_SERVICE);
                ClipData clip = ClipData.newPlainText("simple text", textViewVPCDStatus.getText());
                clipboard.setPrimaryClip(clip);
                Toast.makeText(getApplicationContext(), "Log copied to clipboard.",
                        Toast.LENGTH_LONG).show();
                return true;
            case R.id.action_delete:
                textViewVPCDStatus.setText("");
                return true;
            case R.id.action_help:
                showStartupMessage();
                return true;
            case R.id.action_settings:
                startActivity(new Intent(this, SettingsActivity.class));
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    @Override
    public void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        textViewVPCDStatus.setText(savedInstanceState.getCharSequence(SAVED_STATUS));
    }
    @Override
    public void onSaveInstanceState(Bundle savedInstanceState) {
        savedInstanceState.putCharSequence(SAVED_STATUS, textViewVPCDStatus.getText());
        super.onSaveInstanceState(savedInstanceState);
    }

    @Override
    protected void onPause() {
        if (dialog != null)
            dialog.dismiss();
        super.onPause();
    }
}
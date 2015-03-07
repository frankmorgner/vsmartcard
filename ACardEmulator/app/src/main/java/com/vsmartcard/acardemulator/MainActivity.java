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

import android.content.BroadcastReceiver;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.support.v4.content.LocalBroadcastManager;
import android.support.v7.app.ActionBarActivity;
import android.os.Bundle;
import android.text.Layout;
import android.text.method.ScrollingMovementMethod;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.TextView;
import android.widget.Toast;


public class MainActivity extends ActionBarActivity {

    private TextView textViewVPCDStatus;
    private BroadcastReceiver bReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals(SimulatorService.TAG)) {
                String capdu = intent.getStringExtra(SimulatorService.EXTRA_CAPDU);
                String rapdu = intent.getStringExtra(SimulatorService.EXTRA_RAPDU);
                String error = intent.getStringExtra(SimulatorService.EXTRA_ERROR);
                String deselect = intent.getStringExtra(SimulatorService.EXTRA_DESELECT);
                String install = intent.getStringExtra(SimulatorService.EXTRA_INSTALL);
                if (install != null)
                    textViewVPCDStatus.append(getResources().getString(R.string.status_install) + ":" + install + "\n");
                if (capdu != null)
                    textViewVPCDStatus.append(getResources().getString(R.string.status_capdu) + ": " + capdu + "\n");
                if (error != null)
                    textViewVPCDStatus.append(getResources().getString(R.string.status_error) + ": " + error + "\n");
                if (rapdu != null) {
                    textViewVPCDStatus.append(getResources().getString(R.string.status_rapdu) + ": " + rapdu + "\n");
                    Layout layout = textViewVPCDStatus.getLayout();
                    if (layout != null) {
                        int scrollAmount = layout.getLineTop(textViewVPCDStatus.getLineCount()) - textViewVPCDStatus.getHeight();
                        if (scrollAmount > 0)
                            textViewVPCDStatus.scrollTo(0, scrollAmount);
                        else
                            textViewVPCDStatus.scrollTo(0, 0);
                    }
                }
                if (deselect != null)
                    textViewVPCDStatus.append(getResources().getString(R.string.status_disconnected) + ": " + deselect + "\n");
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        textViewVPCDStatus = (TextView) findViewById(R.id.textViewLog);
        textViewVPCDStatus.setMovementMethod(new ScrollingMovementMethod());

        LocalBroadcastManager bManager = LocalBroadcastManager.getInstance(this);
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(SimulatorService.TAG);
        bManager.registerReceiver(bReceiver, intentFilter);
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
            default:
                return super.onOptionsItemSelected(item);
        }
    }



    private static String saved_status_key = "textViewVPCDStatus";
    @Override
    public void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        textViewVPCDStatus.setText(savedInstanceState.getCharSequence(saved_status_key));
    }
    @Override
    public void onSaveInstanceState(Bundle savedInstanceState) {
        savedInstanceState.putCharSequence(saved_status_key, textViewVPCDStatus.getText());
        super.onSaveInstanceState(savedInstanceState);
    }
}

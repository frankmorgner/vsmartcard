#!/usr/bin/env python

import sys, os
import subprocess
from threading import Thread, Event
try:
    import pygtk
    pygtk.require("2.0")
except:
    pass
try:
    import gtk, gobject, glib
    import pango
except:
    sys.exit(1)

#glade_dir is set by the build system
from pinpad_globals import *

class PinpadGTK:
    """This a simple GTK based GUI to enter a PIN"""

    def __init__(self):
        btn_names = ["btnOne", "btnTwo", "btnThree", "btnFour", "btnFive",
                     "btnSix", "btnSeven", "btnEight", "btnNine", "btnDel",
                     "btnZero", "btnOk"]

        self.pin = ""

        #Set the Glade file
        self.gladefile = glade_dir + "/pinpad.glade"
        self.builder = gtk.Builder()
        try:
            self.builder.add_from_file(self.gladefile)
        except glib.GError, e:
            popup = gtk.MessageDialog(None, gtk.DIALOG_MODAL,
                gtk.MESSAGE_WARNING, gtk.BUTTONS_OK,
                e.message)
            popup.run()
            popup.destroy()
            raise e

        #Get the Main Window, and connect the "destroy" event
        #Create our dictionay and connect it
        dic = { "on_btnOne_clicked" : self.digit_clicked,
                "on_btnTwo_clicked" : self.digit_clicked,
                "on_btnThree_clicked" : self.digit_clicked,
                "on_btnFour_clicked" : self.digit_clicked,
                "on_btnFive_clicked" : self.digit_clicked,
                "on_btnSix_clicked" : self.digit_clicked,
                "on_btnSeven_clicked" : self.digit_clicked,
                "on_btnEight_clicked" : self.digit_clicked,
                "on_btnNine_clicked" : self.digit_clicked,
                "on_btnZero_clicked" : self.digit_clicked,
                "on_btnOk_clicked" : self.btnOk_clicked,
                "on_btnDel_clicked" : self.btnDel_clicked,
                "on_MainWindow_destroy_event" : gtk.main_quit,
                "on_MainWindow_destroy" : gtk.main_quit }

        self.builder.connect_signals(dic)

        #Change the font for the text field
        txtOutput = self.builder.get_object("txtOutput")
        font = pango.FontDescription("Monospace")
        txtOutput.modify_font(pango.FontDescription("sans 24"))

        #Look for card and set the label accordingly
        lbl_cardStatus = self.builder.get_object("lbl_cardStatus")
        self.cardChecker = cardChecker(lbl_cardStatus, ePA_ATR)
        gobject.idle_add(self.cardChecker.start)

        #Change the font for the buttons
        #For this you have to retrieve the label of each button and change
        #its font
        for name in btn_names:
            btn = self.builder.get_object(name)
            if btn.get_use_stock():
                lbl = btn.child.get_children()[1]
            elif isinstance(btn.child, gtk.Label):
                lbl = btn.child
            else:
                raise ValueError("Button does not have a label")

            if (name == "btnOk" or name == "btnDel"):
                lbl.modify_font(pango.FontDescription("sans 16"))
            else:
                lbl.modify_font(pango.FontDescription("sans 20"))

        #Prepare the Progress Bar window for later use
        self.progressWindow = Popup(self.builder)

        #Display Main Window
        self.window = self.builder.get_object("MainWindow")
        self.window.show()

        if (self.window):
            self.window.connect("destroy", self.shutdown)

    def shutdown(self, widget):
        self.cardChecker.stop()
        self.cardChecker.join()
        gtk.main_quit()

    def digit_clicked(self, widget):
        c = widget.get_label()
        lbl = self.builder.get_object("txtOutput")
        txt = lbl.get_text()
        if len(txt) < 6:
            lbl.set_text(txt + '*')
            self.pin += widget.get_label()

    def btnOk_clicked(self, widget):

        #Check which radio button is checked, so we know what kind of secret to
        #use for PACE
        env_arg = os.environ
        env_arg["PIN"] = self.pin #We alway use the eID-PIN

        #We have to provide the type of secret to use via a command line parameter
        param = "--pin"

        try:
            self.cardChecker.pause() #Stop polling the card while PACE is running
            p = subprocess.Popen(["pace-tool", param, "-v"],
                    stdout=subprocess.PIPE, env=env_arg, close_fds=True)
        except OSError, e:
            popup = gtk.MessageDialog(self.window, gtk.DIALOG_MODAL |
                    gtk.DIALOG_DESTROY_WITH_PARENT, gtk.MESSAGE_ERROR,
                    gtk.BUTTONS_OK, "pace-tool wurde nicht gefunden.")
            popup.run()
            popup.destroy()
            self.cardChecker.resume() #Restart cardChecker
            return

        line = p.stdout.readline()
        self.progressWindow.show()
        while gtk.events_pending():
            gtk.main_iteration()
        while line:
            self.progressWindow.inc_fraction()
            while gtk.events_pending():
                gtk.main_iteration()
            line = p.stdout.readline()

        ret = p.poll()
        self.progressWindow.hide()
        self.cardChecker.resume() #Restart cardChecker

        if (ret == 0):
            popup = gtk.MessageDialog(self.window, gtk.DIALOG_MODAL |
                gtk.DIALOG_DESTROY_WITH_PARENT, gtk.MESSAGE_INFO,
                gtk.BUTTONS_OK, "PIN wurde korrekt eingegeben")
            res = popup.run()
            gtk.main_quit()
        else:
            lbl = self.builder.get_object("txtOutput")
            popup = gtk.MessageDialog(self.window, gtk.DIALOG_MODAL |
                gtk.DIALOG_DESTROY_WITH_PARENT, gtk.MESSAGE_WARNING,
                gtk.BUTTONS_OK,
                "PIN wurde falsch eingegeben. Bitte erneut versuchen")
            res = popup.run()
            popup.destroy()
            lbl.set_text("")
            self.pin = ""

    def btnDel_clicked(self, widget):
        self.pin = self.pin[:-1]
        lbl = self.builder.get_object("txtOutput")
        txt = lbl.get_text()
        if len(txt) < 6:
            lbl.set_text(len(self.pin) * '*')


class Popup(object):

    def __init__(self, builder):
        self.builder = builder
        self.window = self.builder.get_object("progWindow")
        self.progbar = self.builder.get_object("progBar")

    def show(self):
        self.window.show()

    def hide(self):
        self.window.hide()

    def inc_fraction(self):
        fraction = self.progbar.get_fraction()
        if fraction < 1.0:
            fraction += 0.1
        else:
           fraction = 0.0
        self.progbar.set_fraction(fraction)

class cardChecker(Thread):
    """ This class searches for a card with a given ATR and displays
        wether or not it was found on the label of a widget """

    def __init__(self, widget, atr, intervall=1):
        Thread.__init__(self)
        self._finished = Event()
        self._paused = Event()
        self.widget = widget
        self.targetATR = atr
        self.intervall = intervall

    def __cardCheck(self):
        """
        Actually this method should make use of pyscard, but I couldn't make
        it run on the OpenMoko yet. Therefor we call opensc-tool instead, which
        leads to higher delays """

        try:
            p = subprocess.Popen(["opensc-tool", "--atr"], stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE, close_fds=True)

            line = p.stdout.readline().rstrip()
            if line == self.targetATR:
                gobject.idle_add(self.widget.set_label, STR_CARD_FOUND)
            else:
                gobject.idle_add(self.widget.set_label, STR_NO_CARD)

        except OSError, e:
            pass #FIXME

    def run(self):
        while (True):
            if self._finished.isSet(): return
            if self._paused.isSet(): continue 
            self.__cardCheck()
            self._finished.wait(self.intervall)

    def stop(self):
        self._finished.set()

    def pause(self):
        self._paused.set()

    def resume(self):
        self._paused.clear()

if __name__ == "__main__":
    gobject.threads_init()
    pinpad = PinpadGTK()
    gtk.main()

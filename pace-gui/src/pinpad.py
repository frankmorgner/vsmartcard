#!/usr/bin/env python

import sys, os
import subprocess
import time
try:
    import pygtk
    pygtk.require("2.0")
except:
    pass
try:
    import gtk, gobject
    import pango
except:
    sys.exit(1)

#GUITARGET is set by the build system
PATH = GUITARGET

class PinpadGTK:
    """This a simple GTK based GUI to enter a PIN"""

    btn_names = ["btnOne", "btnTwo", "btnThree", "btnFour", "btnFive", "btnSix",
                 "btnSeven", "btnEight", "btnNine", "btnDel", "btnZero", "btnOk"                ]

    def __init__(self):
        btn_names = ["btnOne", "btnTwo", "btnThree", "btnFour", "btnFive", "btnSix",
                     "btnSeven", "btnEight", "btnNine", "btnDel", "btnZero", "btnOk"]

        self.pin = ""

        #Set the Glade file
        self.gladefile = PATH+"pinpad.glade"
        self.builder = gtk.Builder()
        self.builder.add_from_file(self.gladefile)

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
            self.window.connect("destroy", gtk.main_quit)

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
        rdio = self.builder.get_object("rdioCAN")
        rdioGrp = rdio.get_group()
        rdioActive = [r for r in rdioGrp if r.get_active()]
        env_arg = os.environ
        env_arg[rdioActive[0].get_label()] = self.pin

        #We have to provide the type of secret to use via a command line parameter
        param = "--" + rdioActive[0].get_label().lower()

        p = subprocess.Popen(["pace-tool", param, "-v"], stdout=subprocess.PIPE,
                env=env_arg, close_fds=True)

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

        if (ret == 0):
            popup = gtk.MessageDialog(self.window, gtk.DIALOG_MODAL |
                gtk.DIALOG_DESTROY_WITH_PARENT, gtk.MESSAGE_INFO, gtk.BUTTONS_OK,
                "PIN wurde korrekt eingegeben")
            res = popup.run()
            gtk.main_quit()
        else:
            lbl = self.builder.get_object("txtOutput")
            popup = gtk.MessageDialog(self.window, gtk.DIALOG_MODAL |
                gtk.DIALOG_DESTROY_WITH_PARENT, gtk.MESSAGE_WARNING, gtk.BUTTONS_OK,
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

if __name__ == "__main__":
    pinpad = PinpadGTK()
    gtk.main()

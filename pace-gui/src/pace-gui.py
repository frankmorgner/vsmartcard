#!/usr/bin/env python
# coding: utf-8

import sys, os, subprocess
from threading import Thread, Event
try:
    import pygtk
    pygtk.require("2.0")
    import gtk, gobject, glib, pango
except:
    sys.exit(1)

try:
    import pace
except:
    print >> sys.stderr, "Could not import pace module, please install pyPACE"
    sys.exit(1)

from pinpad_globals import *

at_chat_strings = [
        "Altersverifikation", #"Age Verification",
        "Gemeindeschlüssel Verifikation", #Community ID Verification
        "Pseudonymfunktion", #"Restrictied Identification",
        "Priviligiertes Terminal", #"Privileged Terminal",
        "Verwendung der CAN", #"CAN allowed",
        "PIN Managment",
        "Zertifikat einspielen", #"Install Certificate",
        "Qualifiziertes Zertifikat einspielen", #"Install Qualified Certificate",
        "Dokumenttyp lesen", #"Read DG 1",
        "Staat lesen", #"Read DG 2",
        "Ablaufdatum lesen", #"Read DG 3",
        "Vorname lesen", #"Read DG 4",
        "Nachname lesen", #"Read DG 5",
        u"Künstlername lesen", #"Read DG 6",
        "Doktorgrad lesen", #"Read DG 7",
        "Geburtsdatum lesen", #"Read DG 8",
        "Geburtsort lesen", #"Read DG 9",
        u"Ungültig", #"Read DG 10",
        u"Ungültig", #"Read DG 11",
        u"Ungültig", #"Read DG 12",
        u"Ungültig", #"Read DG 13",
        u"Ungültig", #"Read DG 14",
        u"Ungültig", #"Read DG 15",
        u"Ungültig", #"Read DG 16",
        "Adresse lesen", #"Read DG 17",
        u"Gemeindeschlüssel lesen", #"Read DG 18",
        u"Ungültig", #"Read DG 19",
        u"Ungültig", #"Read DG 20",
        u"Ungültig", #"Read DG 21",
        u"Ungültig", #"RFU",
        u"Ungültig", #"RFU",
        u"Ungültig", #"RFU",
        u"Ungültig", #"RFU",
        u"Ungültig", #"Write DG 21",
        u"Ungültig", #"Write DG 20",
        u"Ungültig", #"Write DG 19",
        u"Gemeindeschlüssel schreiben", #"Write DG 18",
        u"Adresse schreiben", #"Write DG 17"
]

class MokoWindow(gtk.Window):
    """
    Base class for all our dialogues. It's a simple gtk.Window, with a title at
    the top and to buttons at the bottom. In the middle there is a vBox where
    we can put custom stuff for each dialoge.
    All the dialoges are stored as a double-linked list, so the navigation from
    one window to another is easy
    """

    def __init__(self, title, predecessor):
        super(MokoWindow, self).__init__()

        self.title_str = title
        self.predecessor = predecessor

        assert(isinstance(self.title_str, basestring))

        self.connect("destroy", gtk.main_quit)
        self.set_default_size(480, 640) #Display resolution of the OpenMoko
        #self.set_resizable(False)

        #Main VBox, which consists of the title, the body, and the buttons
        #The size of the elements is inhomogenous and the spacing between elements is 5 pixel
        self.__vb = gtk.VBox(False, 5)
        self.add(self.__vb)

        #Title label at the top of the window
        lblTitle = gtk.Label(self.title_str)
        lblTitle.modify_font(pango.FontDescription("sans 14"))
        lblTitle.set_alignment(0.5, 0.0)
        self.__vb.pack_start(lblTitle, False, False)

        #Body VBox for custom widgets
        self.body = gtk.VBox(False)
        #Add a scrollbar in case we must display lots of information
        #Only use a vertical spacebar, not a horizontal one
        scrolled_window = gtk.ScrolledWindow()
        scrolled_window.set_policy(gtk.POLICY_NEVER, gtk.POLICY_AUTOMATIC)
        scrolled_window.set_shadow_type(gtk.SHADOW_ETCHED_OUT)
        self.__vb.pack_start(scrolled_window)
        scrolled_window.add_with_viewport(self.body)

        #Add two buttons at the bottom of the window
        hbox = gtk.HBox(True, 5)
        btnBack = gtk.Button(u"Zurück")
        btnBack.set_size_request(150, 75)
        btnBack.connect("clicked", self.btnBack_clicked, None)
        if isinstance(btnBack.child, gtk.Label):
            lbl = btnBack.child
        else:
            raise ValueError("Button does not have a label")
        lbl.modify_font(pango.FontDescription("sans 12"))
        hbox.pack_start(btnBack, True, True)
        
        btnForward = gtk.Button("Weiter")
        btnForward.set_size_request(150, 75)
        btnForward.connect("clicked", self.btnForward_clicked, None)
        if isinstance(btnForward.child, gtk.Label):
            lbl = btnForward.child
        else:
            raise ValueError("Button does not have a label")
        hbox.pack_start(btnForward, True, True)
        lbl.modify_font(pango.FontDescription("sans 12"))
        self.__vb.pack_start(hbox, False, False)

    def btnBack_clicked(self, widget, data=None):
        if self.predecessor == None:
            gtk.main_quit()
        else:
            self.hide()
            self.predecessor.show()
#        raise NotImplementedError("Please implement this method in a subclass")

    def btnForward_clicked(self, widget, data=None):
        if self.successor == None:
            pass
        else:
            self.hide()
            self.successor.show_all()
#        raise NotImplementedError("Please implement this method in a subclass")

class CertificateDescriptionWindow(MokoWindow):

    def __init__(self, binDescription, binCert):
        super(CertificateDescriptionWindow, self).__init__("Dienstanbieter", None)

        #binDesciption contains the Certificate Description as a octet string
        desc = pace.d2i_CVC_CERTIFICATE_DESCRIPTION(binDescription)

        self.successor = CVCWindow(binCert, self)

        #Display the validity period:
        cvc = pace.d2i_CV_CERT(binCert) #FIXME
        effective_date = self.formatDate(pace.get_effective_date(cvc))
        expiration_date = self.formatDate(pace.get_expiration_date(cvc))
        self.addRow(u"Gültig ab:", effective_date)
        self.addRow(u"Gültig bis:", expiration_date)

        #Display issuer Name and possibly URL and subject name and possibly URL
        issuerName = pace.get_issuer_name(desc)
        issuerURL = pace.get_issuer_url(desc)
        self.addRow("Zertifikatsaussteller:", issuerName)
        if issuerURL != None:
            self.addRow("", issuerURL, True)
        subjectName = pace.get_subject_name(desc)
        subjectURL = pace.get_subject_url(desc)
        self.addRow("Zertifikatsinhaber:", subjectName)
        if subjectURL != None:
            self.addRow("", subjectURL, True)
        self.set_focus(None)

        #Extract, format and display the terms of usage
        terms = pace.get_termsOfUsage(desc)
        formated_terms = self.formatTOU(terms)
        self.addRow("Beschreibung:", formated_terms)

        self.show_all()

    def formatDate(self, date):
        """Take a date string in the form that is included in CV Certificates
           (YYMMDD) and return it in a better readable format (DD.MM.YYYY)"""

        assert(isinstance(date, basestring))
        assert(len(date) == 6)

        yy = date[:2]
        mm = date[2:4]
        dd = date[4:6] #Cut off trailing \0

        #Implicit assumption: 2000 <= year < 3000
        return dd + "." + mm + "." + "20" + yy

    def formatTOU(self, terms):

#        for s in self.terms_array:
#            s = s.replace(", ", "\n").strip()
#            lbl = gtk.Label(s)
#            lbl.set_alignment(0.0, 0.0)
#            lbl.set_width_chars(60)
#            lbl.set_line_wrap(True) #FIXME: Doesn't work on the Moko
#            lbl.modify_font(pango.FontDescription("bold"))
#            self.body.pack_start(gtk.HSeparator())
#            self.body.pack_start(lbl, False, False, 2)

        return terms.replace(", ", "\r\n")

    def addRow(self, title, text, url=False):
        if not url: #URL has no title
            lblTitle = gtk.Label(title)
            lblTitle.set_width_chars(20)
            lblTitle.set_alignment(0.0, 0.5)
            lblTitle.modify_font(pango.FontDescription("bold"))
            self.body.pack_start(lblTitle)
        hbox = gtk.HBox()
        lbl = gtk.Label("") #Empty label for spacing
        lbl.set_width_chars(3)
        hbox.pack_start(lbl)
        if url:
            lblText = gtk.LinkButton(text, text)
            if isinstance(lblText.child, gtk.Label):
                lbl = lblText.child
            else:
                raise ValueError("Button does not have a label")
            lbl.set_width_chars(40)

        else:
            lblText = gtk.Label(text)
            lblText.set_width_chars(40)

        lblText.set_alignment(0.0, 0.0)
        hbox.pack_start(lblText)
        self.body.pack_start(hbox)

class CVCWindow(MokoWindow):

    def __init__(self, binCert, predecessor):
        super(CVCWindow, self).__init__("Zugriffsrechte", None)

        self.predecessor = predecessor
#        self.successor = PinpadWindow(self)

        #Convert the binary certificate to the internal representation and
        #extract the relative authorization from the chat
        cvc = pace.d2i_CV_CERT(binCert)
        chat = pace.cvc_get_chat(cvc)
        self.chat = pace.get_binary_chat(chat)
        self.rel_auth = []
        for c in self.chat:
            self.rel_auth.append(ord(c))
        self.rel_auth_len = len(self.rel_auth)

        #Extract the access rights from the CHAT and display them in the window
        self.access_rights = []
        j = 0
        for i in range((self.rel_auth_len - 1) * 8 - 2):
            if (i % 8 == 0):
                j += 1
            if self.rel_auth[self.rel_auth_len - j] & (1 << (i % 8)):
                    chk = customCheckButton(at_chat_strings[i], i, self.body)
                    self.access_rights.append(chk)

#        self.show_all()

    def btnForward_clicked(self, widget, data=None):
        """ Check wether any access right have been deselected and modify the
            CHAT accordingly """

        for right in self.access_rights:
            if not right.is_active():
                idx = right.idx
                self.chat_array[len(self.chat) - 1 - idx / 8] ^= (1 << (idx % 8))

        #super(CVCWindow, self).btnForward_clicked(widget, data)
        self.hide()
        PinpadGTK()

class customCheckButton(object):
    """This class provides a custom version of gtk.CheckButton.
       The main difference isthat the checkbox can be placed at the right
       side of the label and that we can store an index with the button"""

    def __init__(self, label, index, vbox):
        #Setup a label with the name of the access right
        self.lbl = gtk.Label(label)
        self.lbl.set_alignment(0.0, 0.5)
        self.lbl.set_padding(20, 0)
        self.lbl.modify_font(pango.FontDescription("bold"))

        #...and a checkbox on the right side of the label
        self.idx = index
        self.chk = gtk.CheckButton("")
        self.chk.set_active(True)
        self.chk.set_alignment(1.0, 0.5)

        #Insert the label and the checkbox in the vbox and add a seperator
        hbox = gtk.HBox()
        hbox.pack_start(self.lbl, True, True)
        hbox.pack_start(self.chk, False, False)
        vbox.pack_start(gtk.HSeparator())
        vbox.pack_start(hbox)

    def is_active(self):
        return self.chk.get_active()

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
            #If we encounter an exception at startup, we display a popup with
            #the error message
            popup = gtk.MessageDialog(None, gtk.DIALOG_MODAL,
                gtk.MESSAGE_WARNING, gtk.BUTTONS_OK, e.message)
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
        txtOutput.modify_font(pango.FontDescription("Monospace 16"))

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
                lbl.modify_font(pango.FontDescription("sans 14"))
            else:
                lbl.modify_font(pango.FontDescription("sans 18"))

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
    CertificateDescriptionWindow(TEST_DESCRIPTION, TEST_CVC)
    gtk.main()

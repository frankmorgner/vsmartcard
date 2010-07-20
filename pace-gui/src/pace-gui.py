#!/usr/bin/env python
# coding: utf-8

import sys, os, subprocess
from threading import Thread, Event
try:
    import pygtk
    pygtk.require("2.0")
    import gtk, gobject, glib, pango
except ImportError:
    sys.exit(1)

try:
    import pace
except ImportError:
    print >> sys.stderr, "Could not import pace module, please install pyPACE"
    sys.exit(1)

from pace_gui_globals import AT_CHAT_STRINGS, STR_CARD_FOUND, STR_NO_CARD, IMAGES #FIXME:Fuckup
import pace_gui_globals as gui_globals

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
        #Display resolution of OpenMoko minus height of the SHR toolbar
        self.set_size_request(480, 586)
        #self.set_resizable(False)

        #Main VBox, which consists of the title, the body, and the buttons
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
        """Go back to the previous window. If there is no previous window,
           do nothing"""
        if self.predecessor is None:
            gtk.main_quit()
        else:
            self.predecessor.show_all()
            self.hide()

    def btnForward_clicked(self, widget, data=None):
        """Go to the next window. If there is no next window, do nothing"""
        if self.successor is None:
            pass
        else:
            self.successor.show_all()
            self.hide()

class MsgBox(gtk.Dialog):
    """The gtk.MessageDialog looks like crap on the Moko, so we write our own"""

    def __init__(self, parent, msg, img_type, flags):
        #img is a string which is used as a key for the IMAGES dictionary
        if img_type is not None:
            assert IMAGES.has_key(img_type)

        super(MsgBox, self).__init__(title="Foo", parent=parent, flags=flags)

        #Dialog will be resized to screen width no matter what. Therefore we
        #use the whole width from the beginning to avoid being resized
        self.set_size_request(480, 160)

        hbox_top = gtk.HBox()
        lbl = gtk.Label(msg)
        lbl.set_width_chars(20)
        lbl.set_line_wrap(True)
        btn = gtk.Button("Ok")
        btn.connect("clicked", self.__destroy)
        img = gtk.Image()
        img.set_from_file(IMAGES[img_type])
        hbox_top.pack_start(img, False, False)
        hbox_top.pack_start(lbl, True, True)
        self.vbox.pack_start(hbox_top)
        #self.vbox.pack_start(gtk.HSeparator())
        hbox_bottom = gtk.HBox()
        spacer = gtk.Label("")
        spacer.set_size_request(380, 25)
        hbox_bottom.pack_start(spacer, False, False)
        hbox_bottom.pack_start(btn, True, True)
        self.vbox.pack_start(hbox_bottom)
        #hbox.pack_start(vbox)
        #self.add(hbox)

        self.set_decorated(False)
        self.set_modal(True)

        self.show_all()

    def __destroy(self, widget=None, data=None):
        self.destroy()


class CertificateDescriptionWindow(MokoWindow):
    """This window is used to display information about the service provider,
       extracted from the terminal certificate and the appendant certificate
       description"""

    def __init__(self, binDescription, binCert):
        super(CertificateDescriptionWindow, self).__init__("Dienstanbieter", None)

        #binDesciption contains the Certificate Description as a octet string
        desc = pace.d2i_CVC_CERTIFICATE_DESCRIPTION(binDescription)

        self.successor = CVCWindow(binCert, self)

        #Display the validity period:
        cvc = pace.d2i_CV_CERT(binCert)
        effective_date = self.formatDate(pace.get_effective_date(cvc))
        expiration_date = self.formatDate(pace.get_expiration_date(cvc))
        validity_period = effective_date + " - " + expiration_date
        #self.addRow(u"Gültig ab:", effective_date)
        #self.addRow(u"Gültig bis:", expiration_date)
        self.addRow(u"Gültigkeitszeitraum:", validity_period)

        #Display issuer Name and possibly URL and subject name and possibly URL
        issuerName = pace.get_issuer_name(desc)
        issuerURL = pace.get_issuer_url(desc)
        self.addRow("Zertifikatsaussteller:", issuerName)
        if issuerURL is not None:
            self.addRow("", issuerURL, True)
        subjectName = pace.get_subject_name(desc)
        subjectURL = pace.get_subject_url(desc)
        self.addRow("Zertifikatsinhaber:", subjectName)
        if subjectURL is not None:
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

        year = date[:2]
        month = date[2:4]
        day = date[4:6] #Cut off trailing \0

        #Implicit assumption: 2000 <= year < 3000
        return day + "." + month + "." + "20" + year

    def formatTOU(self, terms):

#        for s in self.terms_array:
#            s = s.replace(", ", "\n").strip()
#            lbl = gtk.Label(s)
#            lbl.set_alignment(0.0, 0.0)
#            lbl.set_width_chars(60)
#            lbl.set_line_wrap(True) #XXX: Doesn't work on the Moko
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
        self.successor = PinpadGTK("pin", None)

        #Convert the binary certificate to the internal representation and
        #extract the relative authorization from the chat
        cvc = pace.d2i_CV_CERT(binCert)
        chat = pace.cvc_get_chat(cvc)
        self.chat = pace.get_binary_chat(chat)
        self.rel_auth = []
        for char in self.chat:
            self.rel_auth.append(ord(char))
        self.rel_auth_len = len(self.rel_auth)

        #Extract the access rights from the CHAT and display them in the window
        self.access_rights = []
        j = 0
        for i in range((self.rel_auth_len - 1) * 8 - 2):
            if (i % 8 == 0):
                j += 1
            if self.rel_auth[self.rel_auth_len - j] & (1 << (i % 8)):
                chk = customCheckButton(i, self.body)
                self.access_rights.append(chk)

    def btnForward_clicked(self, widget, data=None):
        """ Check wether any access right have been deselected and modify the
            CHAT accordingly """

        for right in self.access_rights:
            if not right.is_active():
                idx = right.idx
                self.rel_auth[len(self.chat) - 1 - idx / 8] ^= (1 << (idx % 8))

        #super(CVCWindow, self).btnForward_clicked(widget, data)
        newCHAT = self.__formatHexString(self.rel_auth)
        self.successor.set_chat(newCHAT)
        self.successor.show()
        self.hide()

    def __formatHexString(self, int_list):
        """Take a list of integers and convert it to a hex string, where the
           individual bytes are seperated by a colon (e.g. "0f:00:00")"""
        hex_str = ""
        for i in int_list:
            byte = hex(i)[2:]
            if len(byte) == 1: byte = "0" + byte
            hex_str += byte + ":"

        return hex_str[:-1]

class customCheckButton(object):
    """This class provides a custom version of gtk.CheckButton.
       The main difference isthat the checkbox can be placed at the right
       side of the label and that we can store an index with the button"""

    def __init__(self, index, vbox):
        self.idx = index
        #Fetch name and (possibly empty) helptext
        strings = AT_CHAT_STRINGS[index]
        self.label = strings[0]
        self.helptext = strings[1]

        #Setup a label with the name of the access right
        self.lbl = gtk.Label(self.label)
        self.lbl.set_alignment(0.0, 0.5)
        self.lbl.set_padding(20, 0)
        self.lbl.modify_font(pango.FontDescription("bold"))

        #If we have a helptext, put an image at the left side of the label
        self.info_box = None
        if self.helptext is not None:
            self.info_box = gtk.EventBox()
            img = gtk.Image()
            img.set_from_file(IMAGES["info"])
            self.info_box.add(img)
            self.info_box.connect("button_release_event", self._show_info)

        #...and a checkbox on the right side of the label
        self.chk = gtk.CheckButton("")
        self.chk.set_active(True)
        self.chk.set_alignment(1.0, 0.5)

        #Insert the label and the checkbox in the vbox and add a seperator
        hbox = gtk.HBox()
        if self.info_box is not None:
            hbox.pack_start(self.info_box, False, False)
        else: #pack empty label for spacing
            spacer = gtk.Label("")
            spacer.set_size_request(32, 32)
            hbox.pack_start(spacer, False, False)
        hbox.pack_start(self.lbl, True, True)
        hbox.pack_start(self.chk, False, False)
        vbox.pack_start(gtk.HSeparator())
        vbox.pack_start(hbox)

    def is_active(self):
        return self.chk.get_active()

    def _show_info(self, widget=None, data=None):
        """Show a messagebox containing info about the access right in
           question"""
        print self.helptext

class PinpadGTK:
    """This a simple GTK based GUI to enter a PIN/PUK/CAN"""

    def __init__(self, secret="pin", chat=None, cert=None):
        btn_names = ["btnOne", "btnTwo", "btnThree", "btnFour", "btnFive",
                     "btnSix", "btnSeven", "btnEight", "btnNine", "btnDel",
                     "btnZero", "btnOk"]

        self.pin = ""
        self.secret = secret
        if self.secret == "pin" or self.secret == "can":
            self.secret_len = 6
        elif self.secret == "transport":
            self.secret_len = 5
        elif self.secret == "puk":
            self.secret_len = 10
        else:
            raise ValueError("Unknwon secret type: %s" % self.secret)
            #gtk.main_quit()
        self.chat = chat
        self.cert = cert

        #Set the Glade file
        self.gladefile = gui_globals.GLADE_FILE
        self.builder = gtk.Builder()
        try:
            self.builder.add_from_file(self.gladefile)
        except glib.GError, err:
            #If we encounter an exception at startup, we display a popup with
            #the error message
            popup = MsgBox(None, err.message, "error", None)
            popup.run()
            popup.destroy()
            raise err

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
                "on_MainWindow_destroy_event" : self.shutdown,
                "on_MainWindow_destroy" : self.shutdown }

        self.builder.connect_signals(dic)

        #Change the font for the text field and set it up up properly for the
        #type of secret we are using
        self.output = self.builder.get_object("txtOutput")
        self.output.modify_font(pango.FontDescription("Monospace 16"))
        self.output.set_text(self.secret_len * "_")

        #Look for card and set the label accordingly
        self.lbl_cardStatus = self.builder.get_object("lbl_cardStatus")
        #Fetch the status image
        self.img_cardStatus = self.builder.get_object("img_cardStatus")
        self.img_cardStatus.set_from_file(IMAGES["error"])

        #We only start the thread for polling the card when the window
        #is shown
        self.cardChecker = None

        #Change the font for the buttons
        #For this you have to retrieve the label of each button and change
        #its font
        for btn_name in btn_names:
            btn = self.builder.get_object(btn_name)
            if btn.get_use_stock():
                lbl = btn.child.get_children()[1]
            elif isinstance(btn.child, gtk.Label):
                lbl = btn.child
            else:
                raise ValueError("Button does not have a label")

            if (btn_name == "btnOk" or btn_name == "btnDel"):
                lbl.modify_font(pango.FontDescription("sans 14"))
            else:
                lbl.modify_font(pango.FontDescription("sans 18"))

        #Display Main Window
        self.window = self.builder.get_object("MainWindow")
        self.window.connect("destroy", self.shutdown)
        self.window.hide()

    def show(self):
        """Start the polling thread, then show the window"""
        if self.cardChecker is None:
            self.cardChecker = cardChecker(self.lbl_cardStatus, self.img_cardStatus,
                    gui_globals.ePA_ATR)
            gobject.idle_add(self.cardChecker.start)
        self.window.show_all()

    def set_chat(self, chat):
        self.chat = chat

    def shutdown(self, widget):
        """Stop the cardChecker thread before exiting the application"""
        if self.cardChecker is not None:
            self.cardChecker.stop()
            self.cardChecker.join()
        gtk.main_quit()

    def digit_clicked(self, widget):
        """We indicate the ammount of digits with underscores
           For every digit that is enterd we replace one underscore with a star
           We only accept secret_len digits and ignore every additional digit"""
        digit = widget.get_label()
        assert digit.isdigit()
        txt = self.output.get_text()
        if len(self.pin) < self.secret_len:
#            self.output.set_text(txt.replace("_", digit, 1))
            idx = txt.find('_')
            output = txt[:idx] + digit + txt[idx+1:]
            self.output.set_text(output)
            gobject.timeout_add(750, self.hide_digits, idx)
            self.pin += digit

    def hide_digits(self, idx):
        """Change character number idx of self.output text to a '*' """
        output = ""
        txt = self.output.get_text()
        if txt[idx].isdigit():
            output = txt[:idx] + '*' + txt[idx+1:]
        self.output.set_text(output)
        return False

    def btnOk_clicked(self, widget):
        """Pass the entered secret to pace-tool. If pace-tool is run
           sucessfully exit, otherwise restart the PIN entry"""

        env_args = os.environ

        #cmd contains the command and all the parameters for our subproccess
        cmd = ["pace-tool"]

        #We have to select the type of secret to use via a command line
        #parameter and provide the actual secret via an environment variable
        if self.secret == "pin" or self.secret == "transport":
            cmd.append("--pin")
            env_args["PIN"] = self.pin
        elif self.secret == "can":
            cmd.append("--can")
            env_args["CAN"] = self.pin
        elif self.secret == "puk":
            cmd.append("--puk")
            env_args["PUK"] = self.pin
        else:
            raise ValueError("Unknown secret type: %s" % self.secret)

        #If we have a CHAT, we pass it to pace-tool
        if (self.chat):
#            cmd.append("--chat=" + self.chat)
            print "--chat=" + self.chat

        cmd.append("-v")

        #Try to call pace-tool. This is a blocking call. An animation is being
        #shown while the subprocess is running
        try:
            #Stop polling the card while PACE is running
            self.cardChecker.pause()
            proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, env=env_args,
                    close_fds=True)
        except OSError:
            popup = MsgBox(self.window, "pace-tool wurde nicht gefunden",
                           "error", gtk.DIALOG_DESTROY_WITH_PARENT)
            popup.run()
            popup.destroy()
            self.cardChecker.resume() #Restart cardChecker
            return

        #Show the animation to indicate that the program is not dead
        waiting = gtk.Window(gtk.WINDOW_POPUP)
        animation = gtk.gdk.PixbufAnimation(IMAGES["wait"])
        img = gtk.Image()
        img.set_from_animation(animation)
        waiting.add(img)
        waiting.set_position(gtk.WIN_POS_CENTER_ALWAYS)
        waiting.set_modal(True)
        waiting.set_transient_for(self.window)
        waiting.set_decorated(False)
        waiting.show_all()

        #Try to keep the GUI responsive by taking care of the event queue
        line = proc.stdout.readline()
        while line:
            while gtk.events_pending():
                gtk.main_iteration()
            line = proc.stdout.readline()

        #Get the return value of the pace-tool process
        ret = proc.poll()
        waiting.destroy()
        self.cardChecker.resume()

        if (ret == 0):
            popup = MsgBox(self.window, "Pin wurde korrekt eingegeben", "apply",
                           gtk.DIALOG_DESTROY_WITH_PARENT)
            popup.run()
            popup.destroy()
            #XXX: Actually we should return to the application that started
            #the PIN entry
            self.shutdown(None)
        else:          
            popup = MsgBox(self.window, "PIN wurde falsch eingegeben", "error",
                            gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT) 
            popup.run()
            popup.destroy()
            self.output.set_text(self.secret_len * "_")
            self.pin = ""

    def btnDel_clicked(self, widget):
        """Remove one digit from the pin and replace the last * in the output
           field with an underscore"""
        self.pin = self.pin[:-1]
        num_digits = len(self.pin)
        output = num_digits * '*' + (self.secret_len - num_digits) * "_" 
        self.output.set_text(output)

class cardChecker(Thread):
    """ This class searches for a card with a given ATR and displays
        wether or not using a label and an image """

    def __init__(self, lbl, img, atr, intervall=1):
        Thread.__init__(self)
        self._finished = Event()
        self._paused = Event()
        self.lbl = lbl
        self.img = img
        self.target_atr = atr
        self.intervall = intervall

    def _check_card(self):
        """
        Actually this method should make use of pyscard, but I couldn't make
        it run on the OpenMoko yet. Therefor we call opensc-tool instead, which
        leads to higher delays """

        proc = subprocess.Popen(["opensc-tool", "--atr"],
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, close_fds=True)

        line = proc.stdout.readline().rstrip()
        if line == self.target_atr:
            gobject.idle_add(self.lbl.set_label, STR_CARD_FOUND)
            gobject.idle_add(self.img.set_from_file, IMAGES["apply"])
        else:
            gobject.idle_add(self.lbl.set_label, STR_NO_CARD)
            gobject.idle_add(self.img.set_from_file, IMAGES["error"])

    def run(self):
        """Main loop: Poll the card if the thread is not paused or finished"""
        while (True):
            if self._finished.isSet():
                return
            if self._paused.isSet():
                continue
            self._check_card()
            self._finished.wait(self.intervall)

    def stop(self):
        self._finished.set()

    def pause(self):
        self._paused.set()

    def resume(self):
        self._paused.clear()

if __name__ == "__main__":
    gobject.threads_init()
    CertificateDescriptionWindow(gui_globals.TEST_DESCRIPTION,
            gui_globals.TEST_CVC)
    gtk.main()

#coding: utf-8

import sys, subprocess, os
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

from eid_gui_globals import IMAGES, GLADE_FILE, EPA_ATR
from widgets import customCheckButton
from logic import cardChecker, break_text

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
        lbl_title = gtk.Label(self.title_str)
        lbl_title.modify_font(pango.FontDescription("sans 14"))
        lbl_title.set_alignment(0.5, 0.0)
        self.__vb.pack_start(lbl_title, False, False)

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
        if self.predecessor is None:
            btnBack = gtk.Button("Abbruch")
        else:
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

        #Place window in the center of the screen
        self.set_position(gtk.WIN_POS_CENTER_ALWAYS)

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

    def __init__(self, parent, msg, img_type):
        #img is a string which is used as a key for the IMAGES dictionary
        if img_type is not None:
            assert IMAGES.has_key(img_type)

        super(MsgBox, self).__init__(title="", parent=parent,
                    flags= gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT)

        #Dialog will be resized to screen width no matter what. Therefore we
        #use the whole width from the beginning to avoid being resized
        self.set_size_request(480, 160)

        hbox_top = gtk.HBox()
        lbl = gtk.Label(break_text(msg, 35))
        #lbl.set_width_chars(20)
        #lbl.set_line_wrap(True)
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

        #self.set_decorated(False)
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
        issuer_name = pace.get_issuer_name(desc)
        issuer_url = pace.get_issuer_url(desc)
        self.addRow("Zertifikatsaussteller:", issuer_name)
        if issuer_url is not None:
            self.addRow("", issuer_url, True)
        subject_name = pace.get_subject_name(desc)
        subject_url = pace.get_subject_url(desc)
        self.addRow("Zertifikatsinhaber:", subject_name)
        if subject_url is not None:
            self.addRow("", subject_url, True)
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
            lbl_title = gtk.Label(title)
            lbl_title.set_width_chars(20)
            lbl_title.set_alignment(0.0, 0.5)
            lbl_title.modify_font(pango.FontDescription("bold"))
            self.body.pack_start(lbl_title)
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
    """This window is used to display and modify the access encoded in the CHAT
       of a CV Certificate"""

    def __init__(self, binCert, predecessor):
        super(CVCWindow, self).__init__("Zugriffsrechte", predecessor)

        self.successor = PinpadGTK("pin", None)

        #Convert the binary certificate to the internal representation and
        #extract the relative authorization from the chat
        cvc = pace.d2i_CV_CERT(binCert)
        chat = pace.cvc_get_chat(cvc)
        self.chat_oid = pace.get_chat_oid(chat)
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
        new_chat_list = []
        for byte in self.chat_oid:
            new_chat_list.append(ord(byte))
        new_chat_list.append(len(self.rel_auth))
        new_chat_list.extend(self.rel_auth)
        new_chat = self.__formatHexString(new_chat_list)
        self.successor.set_chat(new_chat)
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

class PinpadGTK(object):
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
        self.gladefile = GLADE_FILE
        self.builder = gtk.Builder()
        try:
            self.builder.add_from_file(self.gladefile)
        except glib.GError, err:
            #If we encounter an exception at startup, we display a popup with
            #the error message
            popup = MsgBox(None, err.message, "error")
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
                    EPA_ATR)
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

    def btnDel_clicked(self, widget):
        """Remove one digit from the pin and replace the last * in the output
           field with an underscore"""
        #Disable the "Okay" button
        btn_okay = self.builder.get_object("btnOk")
        btn_okay.set_sensitive(False)
        #Delete the last digit
        self.pin = self.pin[:-1]
        num_digits = len(self.pin)
        #Update display
        output = num_digits * '*' + (self.secret_len - num_digits) * "_"
        self.output.set_text(output)

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
            gobject.timeout_add(750, self.__hide_digit, idx)
            self.pin += digit
            #If we have enough digits we enable the "Okay" button
            if len(self.pin) == self.secret_len:
                btn_okay = self.builder.get_object("btnOk")
                btn_okay.set_sensitive(True)

    def __hide_digit(self, idx):
        """Change character number idx of self.output text to a '*' """
        output = ""
        txt = self.output.get_text()
        if txt[idx].isdigit():
            output = txt[:idx] + '*' + txt[idx+1:]
        self.output.set_text(output)
        return False

    def btnOk_clicked(self, widget):
        """Pass the entered secret to npa-tool. If npa-tool is run
           sucessfully exit, otherwise restart the PIN entry"""

        env_args = os.environ

        #cmd contains the command and all the parameters for our subproccess
        cmd = ["npa-tool"]

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

        #If we have a CHAT, we pass it to npa-tool
        #if (self.chat):
        #    cmd.append("--chat=" + self.chat)

        cmd.append("-v")

        #Try to call npa-tool. This is a blocking call. An animation is being
        #shown while the subprocess is running
        try:
            #Stop polling the card while PACE is running
            self.cardChecker.pause()
            proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, env=env_args,
                    close_fds=True)
        except OSError:
            popup = MsgBox(self.window, "npa-tool wurde nicht gefunden",
                           "error")
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

        #Get the return value of the npa-tool process
        ret = proc.poll()
        waiting.destroy()
        self.cardChecker.resume()

        if (ret == 0):
            popup = MsgBox(self.window, "Pin wurde korrekt eingegeben", "apply")
            popup.run()
            popup.destroy()
            #XXX: Actually we should return to the application that started
            #the PIN entry
            self.shutdown(None)
        else:
            popup = MsgBox(self.window, "PIN wurde falsch eingegeben", "error")
            popup.run()
            popup.destroy()
            self.output.set_text(self.secret_len * "_")
            self.pin = ""

class PINChanger(PinpadGTK):
        """Window that allows the user to change the PIN of his or her card
           This is basically a simple state machine similiar to this chain:
           start->old_pin->new_pin1->new_pin2->end"""

        def __init__(self):
            super(PINChanger, self).__init__(secret="pin")

            self.states = ("old_pin", "first_new_pin", "second_new_pin")
            self.state = 0
            self.__old_pin = ""
            self.__new_pin1 = ""
            self.__update_gui()

        def btnOk_clicked(self, widget, data=None):
            """PIN was entered . Check it and according to this check proceed
               to the next state"""

            if self.__check_entry(self.pin):
                self.__next_state()
            else:
                self.__previous_state()
            self.__update_gui()

        def __check_entry(self, pin):
            """Process the PIN entered
               @return: boolean"""

            if self.states[self.state] == "old_pin":
                #Try to perform PACE with the card to see if the PIN is correct
                if self.__check_old_pin():
                    self.__old_pin = pin
                    return True
                else:                    
                    return False
            elif self.states[self.state] == "first_new_pin":
                self.__new_pin1 = pin
                return True
            elif self.states[self.state] == "second_new_pin":
                if pin == self.__new_pin1:
                    return self.__change_pin()
                else:
                    self.__new_pin1 = ""
                    return False

        def __previous_state(self):
            """Go to the previous state"""

            if self.states[self.state] == "old_pin":
                #Keep on trying
                pass
            elif self.states[self.state] == "first_new_pin":
                #Can't happen because we always return True in self.__check_entry
                pass
            elif self.states[self.state] == "second_new_pin":
                self.state = 1 #Enter first PIN again

        def __next_state(self):
            """Go to the next state"""
            if self.state <= 1:
                self.state += 1
            else:
                self.__success()

            #Clear input for next state
            self.pin = ""

        def __update_gui(self):
            """Update  the GUI elements according to the current state"""

            lbl_title = self.builder.get_object("lblInstruction")
            txt_out = self.builder.get_object("txtOutput")
            btn_ok = self.builder.get_object("btnOk")

            txt_out.set_text('_' * self.secret_len)

            if self.states[self.state] == "old_pin":
                lbl_title.set_text("Alte PIN eingeben")
            elif self.states[self.state] == "first_new_pin":
                lbl_title.set_text("Neue PIN eingeben")
            elif self.states[self.state] == "second_new_pin":
                lbl_title.set_text("PIN wiederholen")
            lbl_title.modify_font(pango.FontDescription("bold 11"))

            btn_ok.set_sensitive(False)

        def __check_old_pin(self):
            """Run PACE with the old pin to see if it is correct"""
            #cmd contains the command and all the parameters for our subproccess
            cmd = ["npa-tool", "--pin=" + self.pin]

            #Try to call npa-tool. This is a blocking call. An animation is being
            #shown while the subprocess is running
            try:
            #Stop polling the card while PACE is running
               self.cardChecker.pause()
               proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, close_fds=True)
            except OSError:
                popup = MsgBox(self.window, "npa-tool wurde nicht gefunden",
                               "error")
                popup.run()
                popup.destroy()
                self.cardChecker.resume() #Restart cardChecker
                return False

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

            #Get the return value of the npa-tool process
            ret = proc.poll()
            waiting.destroy()
            self.cardChecker.resume()

            if (ret == 0):
                return True
            else:
                popup = MsgBox(self.window, "PIN wurde falsch eingegeben", "error")
                popup.run()
                popup.destroy()
                return False


        def __change_pin(self):
            """Change the pin using npa-tool"""

            #cmd contains the command and all the parameters for our subproccess
            cmd = ["npa-tool", "--pin=" + self.__old_pin,
                        "--new-pin=" + self.__new_pin1]

            #Try to call npa-tool. This is a blocking call. An animation is being
            #shown while the subprocess is running
            try:
            #Stop polling the card while PACE is running
               self.cardChecker.pause()
               proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, close_fds=True)
            except OSError:
                popup = MsgBox(self.window, "npa-tool wurde nicht gefunden",
                               "error")
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

            #Get the return value of the npa-tool process
            ret = proc.poll()
            waiting.destroy()
            self.cardChecker.resume()

            if (ret == 0):
                return True
            else:
                popup = MsgBox(self.window, "Ein Fehler ist aufgetreten", "error")
                popup.run()
                popup.destroy()
                return False

        def __success(self):
            suc = MsgBox(self.window, u"PIN wurde erfolgreich geändert", "apply")
            suc.run()
            suc.destroy()
            self.cardChecker.stop()
            gtk.main_quit() #Actually we should return to the caller of the window

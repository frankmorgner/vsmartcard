import sys
try:
    import pygtk
    pygtk.require("2.0")
    import gtk, pango
except ImportError:
    sys.exit(1)

from eid_gui_globals import AT_CHAT_STRINGS, IMAGES
import windows

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
        self.parent = vbox.get_parent_window()

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
        help = windows.MsgBox(self.parent, self.helptext, "info")


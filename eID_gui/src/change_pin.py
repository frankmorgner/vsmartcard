#!/usr/bin/env python
# coding: utf-8

import sys, os, subprocess
try:
    import pygtk
    pygtk.require("2.0")
    import gtk, gobject
except ImportError:
    sys.exit(1)

from eid.eid_gui_globals import TEST_DESCRIPTION, TEST_CVC
from eid.windows import PINChanger

if __name__ == "__main__":
    gobject.threads_init()
    window = PINChanger()
    window.show()
    gtk.main()

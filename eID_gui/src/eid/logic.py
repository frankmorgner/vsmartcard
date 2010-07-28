import gobject

import subprocess
from threading import Thread, Event
from eid_gui_globals import STR_CARD_FOUND, STR_NO_CARD, IMAGES

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


class RelayMiddleman(object):
    """
    The RelayMiddleman class serves as a base from which a user might derive
    their own relay middle man class.  This base class implements the simplest
    Man-in-the-Middle:  the NoOp.
    """

    def handleInPDU(self, inPDU: bytes):
        """
        This method is called on each PDU that is fed into the realy (vdpu -> vicc).
        It may be overwritten to modify the packages send from the terminal to the 
        real smart card.
        """
        return inPDU

    def handleOutPDU(self, outPDU: bytes):
        """
        This method is called on each PDU that is produced by the relay (vicc -> vdpu).
        It may be overwritten to modify the packages send from the real smart card to the
        terminal.
        """
        return outPDU

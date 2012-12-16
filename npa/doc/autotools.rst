.. highlight:: sh

============
Installation
============

The nPA Smart Card Library uses the GNU Build System to compile and install. If you are
unfamiliar with it, please have a look at :file:`INSTALL`. If you have a look
around and can not find it, you are probably working bleeding edge in the
repository.  Run the following command in :file:`npa` to
get the missing standard auxiliary files::
    
    autoreconf --verbose --install

To configure (:command:`configure --help` lists possible options), build and
install the nPA Smart Card Library now do the following::
    
    ./configure
    make
    make install

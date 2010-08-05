SUBDIRS = eid
bin_SCRIPTS = eid_gui.py \
	change_pin.py

paceguidir = $(prefix)/bin

uidir = $(datadir)/eid_gui
ui_DATA = pinpad.glade

appdir = $(datadir)/applications/
app_DATA = pace-gui.desktop \
	change-pin.desktop

EXTRA_DIST =  			\
	$(bin_SCRIPTS) \
	$(app_DATA) \
	$(ui_DATA)

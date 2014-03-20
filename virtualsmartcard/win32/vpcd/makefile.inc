.SUFFIXES: .inx

STAMP=stampinf

.inx{$(OBJ_PATH)\$(O)}.inf:
    copy $(@B).inx $@
    $(STAMP) -f $@ -a $(_BUILDARCH) -k $(KMDF_VERSION_MAJOR).$(KMDF_VERSION_MINOR) -u $(UMDF_VERSION_MAJOR).$(UMDF_VERSION_MINOR).0


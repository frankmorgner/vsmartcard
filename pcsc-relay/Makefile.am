SUBDIRS=src doc win32

win:
	make -C win32 $@

dist-hook:
	test -d .svn && \
		svn2cl --group-by-day --reparagraph --separate-daylogs --include-actions --include-rev || true

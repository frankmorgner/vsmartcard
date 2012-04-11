ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src m4 patches doc

dist-hook:
	test -d .svn && \
	   	svn2cl --group-by-day --reparagraph --separate-daylogs --include-actions --include-rev || true

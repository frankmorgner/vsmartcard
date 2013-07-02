DISTCHECK_CONFIGURE_FLAGS = --enable-pace

ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src m4 patches doc

distcheck-hook:
	test -d .svn && \
	   	svn2cl --group-by-day --reparagraph --separate-daylogs --include-actions --include-rev \
		|| true
	test -d ../.git && \
		git2cl > ChangeLog \
		|| true

ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src doc

distcheck-hook:
	test -d .svn && \
		svn2cl --group-by-day --reparagraph --separate-daylogs --include-actions --include-rev || true

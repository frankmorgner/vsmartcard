ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src doc

EXTRA_DIST = test_vicc_with_npa-tool.sh

distcheck-hook:
	test -d .svn && \
		svn2cl --group-by-day --reparagraph --separate-daylogs --include-actions --include-rev || true

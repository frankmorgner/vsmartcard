ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src doc npa-example-data

EXTRA_DIST = test_vicc_with_npa-tool.sh test_vicc_with_handler_test.sh

distcheck-hook:
	test -d .svn && \
		svn2cl --group-by-day --reparagraph --separate-daylogs --include-actions --include-rev \
		|| true
	test -d ../.git && \
		git log --pretty --numstat --summary . | git2cl > ChangeLog \
		|| true

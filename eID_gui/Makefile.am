SUBDIRS = src images

distcheck-hook:
	test -d .svn && \
		svn2cl --group-by-day --reparagraph --separate-daylogs --include-actions --include-rev \
		|| true
	test -d ../.git && \
		git log --pretty --numstat --summary . | git2cl > ChangeLog \
		|| true

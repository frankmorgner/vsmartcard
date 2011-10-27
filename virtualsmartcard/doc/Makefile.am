EXTRA_DIST = README.rst generate_modules.py api

.PHONY: doc
doc:
	./generate_modules.py $(top_srcdir)/src/vpicc --dest-dir=api --suffix=rst --no-toc

clean-local:
	rm -rf api/*

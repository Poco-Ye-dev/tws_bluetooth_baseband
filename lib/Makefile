# SPDX-License-Identifier: MIT
#
# Build every library that has its own Makefile.  Each library is responsible
# for its own object list and for naming its archive.

.DEFAULT_GOAL := archive

.PHONY: archive clean

archive:
	@for d in */; do \
		if [ -s "$$d/Makefile" ]; then \
			$(MAKE) -C "$$d" archive || exit 1; \
		fi \
	done

clean:
	@for d in */; do \
		if [ -s "$$d/Makefile" ]; then \
			$(MAKE) -C "$$d" clean || exit 1; \
		fi \
	done
	@rm -f $(AR_DIR)/*.a

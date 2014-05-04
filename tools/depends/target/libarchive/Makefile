include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

LIBNAME=libarchive
VERSION=3.1.2
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(LIBNAME)-$(VERSION).tar.gz
BASE_URL=http://www.libarchive.org/downloads

LIBDYLIB=$(PLATFORM)/xbmc_build/$(LIBNAME)/libarchive.a
all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); rm -rf xbmc_build; mkdir -p xbmc_build
	cd $(PLATFORM)/xbmc_build; $(CMAKE) -DCMAKE_LIBRARY_PATH=$(TOOLCHAIN)/lib/$(HOST) -DENABLE_ACL=OFF -DENABLE_CPIO=OFF -DENABLE_TAR=OFF -DENABLE_TEST=OFF -DENABLE_XATTR=OFF -DENABLE_OPENSSL=OFF -DENABLE_ICONV=OFF ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/xbmc_build
	touch $@
.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/xbmc_build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/xbmc_build clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

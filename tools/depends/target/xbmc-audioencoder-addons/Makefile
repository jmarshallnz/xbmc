-include ../../Makefile.include

LIBNAME = xbmc-audioencoder-addons
ifeq ($(CMAKE),)
  CMAKE = cmake
  ifneq ($(PREFIX),)
    INSTALL_PREFIX = -DCMAKE_INSTALL_PREFIX=$(PREFIX)
  endif
endif

ifeq (darwin, $(findstring darwin, $(HOST)))
  CMAKE_EXTRA = -DPACKAGE_ZIP=1 -DOUTPUT_DIR=../../../../../addons/
else
  CMAKE_EXTRA = -DOUTPUT_DIR=$(PREFIX)
endif

all: .installed-$(PLATFORM)


clean:
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

.installed-$(PLATFORM):
	mkdir -p $(PLATFORM); \
         cd $(PLATFORM); \
         $(CMAKE) $(INSTALL_PREFIX) -DPREFIX=$(PREFIX) $(CMAKE_EXTRA) \
         ../../../../../project/cmake/addons ;\
         $(MAKE);
	touch $@

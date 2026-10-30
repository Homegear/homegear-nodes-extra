AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -Wall -std=c++20 -DFORTIFY_SOURCE=2 -DGCRYPT_NO_DEPRECATED
AM_LDFLAGS = -Wl,-rpath=/lib/homegear -Wl,-rpath=/usr/lib/homegear -Wl,-rpath=/usr/local/lib/homegear
LIBS += -Wl,-Bdynamic -lhomegear-node

libdir = $(localstatedir)/lib/homegear/node-blue/nodes/hvac

lib_LTLIBRARIES = heating-controller.la heating-cooling-controller.la heating-mode.la

heating_controller_la_SOURCES = heating-controller/Factory.cpp heating-controller/HeatingController.cpp heating-controller/MyNode.cpp
heating_controller_la_LDFLAGS =-module -avoid-version -shared

heating_cooling_controller_la_SOURCES = heating-cooling-controller/Factory.cpp heating-cooling-controller/HeatingCoolingController.cpp heating-cooling-controller/MyNode.cpp
heating_cooling_controller_la_LDFLAGS =-module -avoid-version -shared

heating_mode_la_SOURCES = heating-mode/Factory.cpp heating-mode/MyNode.cpp
heating_mode_la_LDFLAGS =-module -avoid-version -shared

hvac_ladir = $(libdir)
hvac_la_DATA = package.json heating-controller/heating-controller.hni heating-cooling-controller/heating-cooling-controller.hni heating-mode/heating-mode.hni

locale_endir = $(libdir)/locales/en
locale_en_DATA = locales/en-US/hvac.json

locale_en_usdir = $(libdir)/locales/en-US
locale_en_us_DATA = locales/en-US/hvac.json

install-exec-hook:
	rm -f $(DESTDIR)$(libdir)/heating-controller.la
	rm -f $(DESTDIR)$(libdir)/heating-cooling-controller.la
	rm -f $(DESTDIR)$(libdir)/heating-mode.la

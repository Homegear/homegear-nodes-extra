AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -Wall -std=c++20 -DFORTIFY_SOURCE=2 -DGCRYPT_NO_DEPRECATED
AM_LDFLAGS = -Wl,-rpath=/lib/homegear -Wl,-rpath=/usr/lib/homegear -Wl,-rpath=/usr/local/lib/homegear
LIBS += -Wl,-Bdynamic -lhomegear-node

libdir = $(localstatedir)/lib/homegear/node-blue/nodes/shading

lib_LTLIBRARIES = shading.la

shading_la_SOURCES = shading/Factory.cpp shading/MyNode.cpp shading/InputEvent.cpp shading/ShadingController.cpp shading/StateMachine.cpp
shading_la_LDFLAGS =-module -avoid-version -shared

shading_ladir = $(libdir)
shading_la_DATA = package.json shading/shading.hni

locale_endir = $(libdir)/locales/en
locale_en_DATA = locales/en-US/shading.json

locale_en_usdir = $(libdir)/locales/en-US
locale_en_us_DATA = locales/en-US/shading.json

install-exec-hook:
	rm -f $(DESTDIR)$(libdir)/shading.la

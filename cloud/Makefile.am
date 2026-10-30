AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -Wall -std=c++20 -DFORTIFY_SOURCE=2 -DGCRYPT_NO_DEPRECATED
AM_LDFLAGS = -Wl,-rpath=/lib/homegear -Wl,-rpath=/usr/lib/homegear -Wl,-rpath=/usr/local/lib/homegear
LIBS += -Wl,-Bdynamic -lhomegear-node -lhomegear-base

libdir = $(localstatedir)/lib/homegear/node-blue/nodes/cloud

lib_LTLIBRARIES = cloud-device.la

cloud_device_la_SOURCES = cloud-device/Factory.cpp cloud-device/CloudDevice.cpp
cloud_device_la_LDFLAGS =-module -avoid-version -shared

cloud_ladir = $(libdir)
cloud_la_DATA = package.json cloud-device/cloud-device.html

locale_endir = $(libdir)/locales/en
locale_en_DATA = locales/en-US/cloud.json locales/en-US/cloud-device.help.html

locale_en_usdir = $(libdir)/locales/en-US
locale_en_us_DATA = locales/en-US/cloud.json locales/en-US/cloud-device.help.html

install-exec-hook:
	rm -f $(DESTDIR)$(libdir)/cloud-device.la

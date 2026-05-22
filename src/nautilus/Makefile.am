common_cppflags = -DLOCALEDIR="\"$(localedir)\"" $(GTK_CFLAGS)

common_ldflags = \
	-avoid-version \
	-module \
	-shared

common_libadd = \
	../hash/hash.la \
	../util/util.la \
	$(GTK_LIBS)

BUILT_SOURCES = properties-resources.c \
	libgtkhash-properties-caja.ver \
	libgtkhash-properties-nautilus.ver \
	libgtkhash-properties-nemo.ver \
	libgtkhash-properties-thunar.ver

CLEANFILES = properties-resources.c *.gcda *.gcno *.ver

EXTRA_DIST = \
	meson.build \
	extension.map.in

if GTK4
properties_resource_xml = ../../data/nautilus/gtkhash-properties-gtk4.gresource.xml
properties_ui = ../../data/nautilus/gtkhash-properties-gtk4.ui
else
properties_resource_xml = ../../data/nautilus/gtkhash-properties.gresource.xml
properties_ui = ../../data/nautilus/gtkhash-properties.ui
endif

if ENABLE_CAJA

cajadir = $(CAJA_EXTENSION_DIR)
caja_LTLIBRARIES = libgtkhash-properties-caja.la

nodist_libgtkhash_properties_caja_la_SOURCES = \
	properties-resources.c

libgtkhash_properties_caja_la_SOURCES = \
	properties.c properties.h \
	properties-hash.c properties-hash.h \
	properties-list.c properties-list.h \
	properties-prefs.c properties-prefs.h

libgtkhash_properties_caja_la_CPPFLAGS = \
	$(common_cppflags) \
	$(CAJA_CFLAGS) \
	-DIN_CAJA_EXTENSION

libgtkhash_properties_caja_la_CFLAGS = $(GTKHASH_CFLAGS)

libgtkhash_properties_caja_la_LDFLAGS = \
	$(common_ldflags) \
	-Wl,--version-script=libgtkhash-properties-caja.ver

libgtkhash_properties_caja_la_LIBADD = \
	$(common_libadd) \
	$(CAJA_LIBS)

endif

if ENABLE_NAUTILUS

nautilusdir = $(NAUTILUS_EXTENSION_DIR)
nautilus_LTLIBRARIES = libgtkhash-properties-nautilus.la

nodist_libgtkhash_properties_nautilus_la_SOURCES = \
	properties-resources.c

libgtkhash_properties_nautilus_la_SOURCES = \
	properties.c properties.h \
	properties-hash.c properties-hash.h \
	properties-list.c properties-list.h \
	properties-prefs.c properties-prefs.h

libgtkhash_properties_nautilus_la_CPPFLAGS = \
	$(common_cppflags) \
	$(NAUTILUS_CFLAGS) \
	-DIN_NAUTILUS_EXTENSION

libgtkhash_properties_nautilus_la_CFLAGS = $(GTKHASH_CFLAGS)

libgtkhash_properties_nautilus_la_LDFLAGS = \
	$(common_ldflags) \
	-Wl,--version-script=libgtkhash-properties-nautilus.ver

libgtkhash_properties_nautilus_la_LIBADD = \
	$(common_libadd) \
	$(NAUTILUS_LIBS)

endif

if ENABLE_NEMO

nemodir = $(NEMO_EXTENSION_DIR)
nemo_LTLIBRARIES = libgtkhash-properties-nemo.la

nodist_libgtkhash_properties_nemo_la_SOURCES = \
	properties-resources.c

libgtkhash_properties_nemo_la_SOURCES = \
	properties.c properties.h \
	properties-hash.c properties-hash.h \
	properties-list.c properties-list.h \
	properties-prefs.c properties-prefs.h

libgtkhash_properties_nemo_la_CPPFLAGS = \
	$(common_cppflags) \
	$(NEMO_CFLAGS) \
	-DIN_NEMO_EXTENSION

libgtkhash_properties_nemo_la_CFLAGS = $(GTKHASH_CFLAGS)

libgtkhash_properties_nemo_la_LDFLAGS = \
	$(common_ldflags) \
	-Wl,--version-script=libgtkhash-properties-nemo.ver

libgtkhash_properties_nemo_la_LIBADD = \
	$(common_libadd) \
	$(NEMO_LIBS)

endif

if ENABLE_THUNAR

thunardir = $(THUNAR_EXTENSION_DIR)
thunar_LTLIBRARIES = libgtkhash-properties-thunar.la

nodist_libgtkhash_properties_thunar_la_SOURCES = \
	properties-resources.c

libgtkhash_properties_thunar_la_SOURCES = \
	properties.c properties.h \
	properties-hash.c properties-hash.h \
	properties-list.c properties-list.h \
	properties-prefs.c properties-prefs.h

libgtkhash_properties_thunar_la_CPPFLAGS = \
	$(common_cppflags) \
	$(THUNAR_CFLAGS) \
	-DIN_THUNAR_EXTENSION

libgtkhash_properties_thunar_la_CFLAGS = $(GTKHASH_CFLAGS)

libgtkhash_properties_thunar_la_LDFLAGS = \
	$(common_ldflags) \
	-Wl,--version-script=libgtkhash-properties-thunar.ver

libgtkhash_properties_thunar_la_LIBADD = \
	$(common_libadd) \
	$(THUNAR_LIBS)

endif

libgtkhash-properties-caja.ver:
	$(AM_V_GEN)printf "{\nglobal:\n\tcaja_module_*;\nlocal:\n\t*;\n};\n" > $@

libgtkhash-properties-nautilus.ver:
	$(AM_V_GEN)printf "{\nglobal:\n\tnautilus_module_*;\nlocal:\n\t*;\n};\n" > $@

libgtkhash-properties-nemo.ver:
	$(AM_V_GEN)printf "{\nglobal:\n\tnemo_module_*;\nlocal:\n\t*;\n};\n" > $@

libgtkhash-properties-thunar.ver:
	$(AM_V_GEN)printf "{\nglobal:\n\tthunar_extension_*;\nlocal:\n\t*;\n};\n" > $@

properties-resources.c: $(properties_resource_xml) $(properties_ui)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) \
		--c-name gtkhash_properties_resources \
		--internal \
		--generate-source \
		--sourcedir="$(srcdir)"/../../data/nautilus/ \
		--target=$@ \
		$<

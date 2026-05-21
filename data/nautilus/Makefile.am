gsettings_SCHEMAS = org.gtkhash.plugin.gschema.xml

if GTK4
properties_resource_xml = gtkhash-properties-gtk4.gresource.xml
properties_ui = gtkhash-properties-gtk4.ui
else
properties_resource_xml = gtkhash-properties.gresource.xml
properties_ui = gtkhash-properties.ui
endif

EXTRA_DIST = \
	$(gsettings_SCHEMAS) \
	$(properties_ui) \
	$(properties_resource_xml) \
	gtkhash-properties.gresource.xml \
	gtkhash-properties.ui \
	gtkhash-properties-gtk4.gresource.xml \
	gtkhash-properties-gtk4.ui \
	meson.build

@GSETTINGS_RULES@
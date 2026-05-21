install_data(
    'org.gtkhash.plugin.gschema.xml',
    install_dir: get_option('datadir') / 'glib-2.0' / 'schemas',
)

if gtk_version == '4'
    gtkhash_properties_gresource = files('gtkhash-properties-gtk4.gresource.xml')
else
    gtkhash_properties_gresource = files('gtkhash-properties.gresource.xml')
endif
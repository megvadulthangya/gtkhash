subdir('hash')
subdir('util')

if enable_nautilus_data
    subdir('nautilus')
endif

if build_gtkhash

    gtkhash_common_sources = [
        'callbacks.c',
        'check.c',
        'gui.c',
        'hash.c',
        'list.c',
        'opts.c',
        'uri-digest.c',
    ]
    gtkhash_common_sources += gnome.compile_resources('resources',
        gtkhash_gtk_gresource,
        c_name: 'resources',
        source_dir: meson.current_source_dir() / '..' / 'data',
        extra_args: [
            '--internal',
            '--manual-register',
        ],
    )

    gtkhash = executable('gtkhash',
        gtkhash_common_sources,
        'main.c',
        'prefs.c',

        include_directories: root_include,
        dependencies: [
            glib,
            gtk,
        ],
        link_with: [
            libhash,
            libutil,
        ],
        install: true,
    )

    # Test executable.

    # Additional sources for test executable
    test_extra_sources = []
    if gtk_version == '4'
        test_extra_sources += gnome.compile_resources('test-ui-resource',
            files('../data/gtkhash-gtk4-test.gresource.xml'),
            c_name: 'test_ui_resource',
            source_dir: meson.current_source_dir() / '..' / 'data',
            extra_args: [
                '--internal',
                '--manual-register',
            ],
        )
    endif

    gtkhash_test = executable('gtkhash-test',
        gtkhash_common_sources,
        test_extra_sources,
        'test.c',

        include_directories: root_include,
        dependencies: [
            glib,
            gtk,
        ],
        link_with: [
            libhash,
            libutil,
        ],
    )

    test_files = []

    configure_file(
        input : 'test.md5sum.in',
        output: 'test.md5sum',
        configuration: { 'builddir': meson.current_build_dir(), },
    )

    bytes_tests_values = [ '0', '1', '10M', '5G', ]
    foreach bytes_test : bytes_tests_values
        filename = '@0@.bytes'.format(bytes_test)
        test_files += custom_target(filename,
            output: filename,
            command: [ 'truncate', '-s', '@0@'.format(bytes_test), '@OUTPUT@', ],
        )
    endforeach

    # Environment for tests: force software rendering to avoid GL crashes.
    test_env = ['GSK_RENDERER=cairo', 'GDK_GL=disable']

    foreach lib, functions : hash_libs
        lib_enabled = config.get('ENABLE_' + lib.to_upper()) == 1

        test('gtkhash-test-@0@'.format(lib),
            gtkhash_test,
            depends: test_files,
            env: test_env + ['GTKHASH_TEST_LIB=@0@'.format(lib)],
            protocol: 'tap',
        )
    endforeach

    test('gtkhash-test',
        gtkhash_test,
        depends: test_files,
        env: test_env,
        protocol: 'tap',
    )

endif
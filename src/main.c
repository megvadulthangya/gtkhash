/*
 *   Copyright (C) 2007-2020 Tristan Heaven <tristan@tristanheaven.net>
 *
 *   This file is part of GtkHash.
 *
 *   GtkHash is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   GtkHash is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with GtkHash. If not, see <https://gnu.org/licenses/gpl-2.0.txt>.
 */

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <gtk/gtk.h>

#include "main.h"
#include "opts.h"
#include "hash.h"
#include "gui.h"
#include "prefs.h"
#include "check.h"
#include "callbacks.h"
#include "uri-digest.h"

#if ENABLE_NLS
static void nls_init(void)
{
#ifdef G_OS_WIN32
    char *pkgdir = g_win32_get_package_installation_directory_of_module(NULL);
    char *localedir = g_build_filename(pkgdir, "share", "locale", NULL);
    bindtextdomain(GETTEXT_PACKAGE, localedir);
    g_free(localedir);
    g_free(pkgdir);
#else
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
#endif

    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);
}
#endif

/* Recursively collect regular files from a GFile (including directories) */
static GSList *
uri_digest_list_from_files_recursive(GFile *file)
{
    GSList *uris = NULL;

    if (!file)
        return NULL;

    GFileType type = g_file_query_file_type(file, G_FILE_QUERY_INFO_NONE, NULL);
    if (type == G_FILE_TYPE_DIRECTORY) {
        GFileEnumerator *enumerator = g_file_enumerate_children(file,
            G_FILE_ATTRIBUTE_STANDARD_NAME "," G_FILE_ATTRIBUTE_STANDARD_TYPE,
            G_FILE_QUERY_INFO_NONE, NULL, NULL);
        if (enumerator) {
            GFileInfo *info;
            while ((info = g_file_enumerator_next_file(enumerator, NULL, NULL))) {
                GFile *child = g_file_get_child(file,
                    g_file_info_get_name(info));
                GSList *child_uris = uri_digest_list_from_files_recursive(child);
                uris = g_slist_concat(uris, child_uris);
                g_object_unref(child);
                g_object_unref(info);
            }
            g_file_enumerator_close(enumerator, NULL, NULL);
            g_object_unref(enumerator);
        }
    } else if (type == G_FILE_TYPE_REGULAR) {
        char *uri = g_file_get_uri(file);
        uris = g_slist_prepend(uris, uri);
    }

    return uris;
}

#if GTK_CHECK_VERSION(4, 0, 0)
static void on_startup(GtkApplication *app, gpointer user_data)
{
    gui_init();
    prefs_init();
    check_init();

    opts_postinit();

    if (!GUI_VIEW_IS_VALID(gui.view)) {
        gui_set_view(GUI_VIEW_FILE);
        gui_update();
    }

    gui_set_application(app);
    callbacks_init(app);
}

static void on_activate(GtkApplication *app, gpointer user_data)
{
    gtk_window_present(GTK_WINDOW(gui.window));
}

static void on_open(GtkApplication *app, GFile **files, gint n_files,
    const gchar *hint, gpointer user_data)
{
    GSList *ud_list = NULL;

    for (gint i = 0; i < n_files; i++) {
        GSList *file_uris = uri_digest_list_from_files_recursive(files[i]);
        GSList *l;
        for (l = file_uris; l; l = l->next) {
            char *uri = l->data;
            ud_list = g_slist_prepend(ud_list, uri_digest_new(uri, NULL));
        }
        g_slist_free(file_uris);
    }

    ud_list = g_slist_reverse(ud_list);

    guint added = gui_add_ud_list(ud_list, GUI_VIEW_INVALID);
    uri_digest_list_free_full(ud_list);

    gui_update();

    if (added && gui_get_state() == GUI_STATE_IDLE)
        gui_start_hash();

    gtk_window_present(GTK_WINDOW(gui.window));
}
#endif

int main(int argc, char **argv)
{
#if ENABLE_NLS
    nls_init();
#endif

    hash_init();

    opts_preinit(&argc, &argv);

#if GTK_CHECK_VERSION(4, 0, 0)
    GtkApplication *app = gtk_application_new("org.gtkhash.gtkhash",
        G_APPLICATION_HANDLES_OPEN);
    g_signal_connect(app, "startup", G_CALLBACK(on_startup), NULL);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    g_signal_connect(app, "open", G_CALLBACK(on_open), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    check_deinit();
    prefs_deinit();
    gui_deinit();
    hash_deinit();

    return status;
#else
    gtk_init(NULL, NULL);

    gui_init();
    prefs_init();
    check_init();

    opts_postinit();

    gui_run();

    check_deinit();
    prefs_deinit();
    gui_deinit();
    hash_deinit();

    return EXIT_SUCCESS;
#endif
}
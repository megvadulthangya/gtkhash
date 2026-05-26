/*
 * Copyright (C) 2007-2020 Tristan Heaven <tristan@tristanheaven.net>
 *
 * This file is part of GtkHash.
 *
 * GtkHash is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * GtkHash is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GtkHash. If not, see <https://gnu.org/licenses/gpl-2.0.txt>.
 */

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "callbacks.h"
#include "main.h"
#include "gui.h"
#include "hash.h"
#include "prefs.h"
#include "list.h"
#include "check.h"
#include "uri-digest.h"
#include "hash/hash-string.h"

static bool on_window_delete_event(void)
{
#if GTK_CHECK_VERSION(4,0,0)
    gtk_widget_set_visible(GTK_WIDGET(gui.window), false);
    g_application_quit(g_application_get_default());
#else
    gtk_widget_hide(GTK_WIDGET(gui.window));
    gtk_main_quit();
#endif

    return true;
}

#if GTK_CHECK_VERSION(4,0,0)
static void on_open_digest_dialog_response(GObject *source, GAsyncResult *res, gpointer user_data)
{
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
    GError *error = NULL;
    GListModel *files = gtk_file_dialog_open_multiple_finish(dialog, res, &error);
    if (error) {
        g_warning("Open digest dialog error: %s", error->message);
        g_error_free(error);
        g_object_unref(dialog);
        return;
    }

    GSList *ud_list = NULL;
    guint n = g_list_model_get_n_items(files);
    for (guint i = 0; i < n; i++) {
        GFile *file = G_FILE(g_list_model_get_item(files, i));
        if (file) {
            ud_list = check_file_load(ud_list, file);
            g_object_unref(file);
        }
    }

    if (ud_list) {
        if (gui.view == GUI_VIEW_FILE_LIST)
            gui_add_ud_list(ud_list, GUI_VIEW_FILE_LIST);
        else
            gui_add_ud_list(ud_list, GUI_VIEW_INVALID);

        gui_update();
        uri_digest_list_free_full(ud_list);
    }
    g_object_unref(files);
    g_object_unref(dialog);
}
#endif

static void on_menuitem_open_activate(void)
{
#if GTK_CHECK_VERSION(4,0,0)
    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, _("Open Digest File"));
    gtk_file_dialog_set_accept_label(dialog, _("_Open"));

    GListStore *filter_store = g_list_store_new(GTK_TYPE_FILE_FILTER);

#ifndef G_OS_WIN32
    GtkFileFilter *filter = NULL;

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter,
        _("Digest/Checksum Files (*.sha1, *.md5, *.sfv, …)"));
    check_file_add_filters(filter);
    g_list_store_append(filter_store, filter);
    g_object_unref(filter);

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, _("All Files"));
    gtk_file_filter_add_pattern(filter, "*");
    g_list_store_append(filter_store, filter);
    g_object_unref(filter);
#endif

    gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filter_store));
    g_object_unref(filter_store);

    gtk_file_dialog_open_multiple(dialog, GTK_WINDOW(gui.window), NULL,
        on_open_digest_dialog_response, NULL);
#elif (GTK_CHECK_VERSION(3,20,0) && ENABLE_NATIVE_FILE_CHOOSER)
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(gtk_file_chooser_native_new(
        _("Open Digest File"), gui.window, GTK_FILE_CHOOSER_ACTION_OPEN,
        _("_Open"), _("_Cancel")));
#else
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(
        gtk_file_chooser_dialog_new(_("Open Digest File"), gui.window,
            GTK_FILE_CHOOSER_ACTION_OPEN,
            _("_Cancel"), GTK_RESPONSE_CANCEL,
            _("_Open"), GTK_RESPONSE_ACCEPT,
            NULL));
#endif

#if GTK_CHECK_VERSION(4,0,0)
    // Handled above
#else
    gtk_file_chooser_set_select_multiple(chooser, true);
    gtk_file_chooser_set_local_only(chooser, false);
#endif

#if !GTK_CHECK_VERSION(4,0,0) && !defined(G_OS_WIN32)
    GtkFileFilter *filter = NULL;

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter,
        _("Digest/Checksum Files (*.sha1, *.md5, *.sfv, …)"));
    check_file_add_filters(filter);
    gtk_file_chooser_add_filter(chooser, filter);

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, _("All Files"));
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(chooser, filter);
#endif

#if GTK_CHECK_VERSION(4,0,0)
    // Nothing here, async handled above
#elif (GTK_CHECK_VERSION(3,20,0) && ENABLE_NATIVE_FILE_CHOOSER)
    if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        GSList *files = gtk_file_chooser_get_files(chooser);
        GSList *ud_list = NULL;

        for (GSList *p = files; p; p = p->next)
            if (p->data)
                ud_list = check_file_load(ud_list, p->data);

        if (ud_list) {
            if (gui.view == GUI_VIEW_FILE_LIST)
                gui_add_ud_list(ud_list, GUI_VIEW_FILE_LIST);
            else
                gui_add_ud_list(ud_list, GUI_VIEW_INVALID);

            gui_update();

            uri_digest_list_free_full(ud_list);
        }

        g_slist_free_full(files, g_object_unref);
    }
    g_object_unref(chooser);
#else
    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        GSList *files = gtk_file_chooser_get_files(chooser);
        GSList *ud_list = NULL;

        for (GSList *p = files; p; p = p->next)
            if (p->data)
                ud_list = check_file_load(ud_list, p->data);

        if (ud_list) {
            if (gui.view == GUI_VIEW_FILE_LIST)
                gui_add_ud_list(ud_list, GUI_VIEW_FILE_LIST);
            else
                gui_add_ud_list(ud_list, GUI_VIEW_INVALID);

            gui_update();

            uri_digest_list_free_full(ud_list);
        }

        g_slist_free_full(files, g_object_unref);
    }
    gtk_widget_destroy(GTK_WIDGET(chooser));
#endif
}

#if GTK_CHECK_VERSION(4,0,0)
static void on_save_as_dialog_response(GObject *source, GAsyncResult *res, gpointer user_data)
{
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
    GError *error = NULL;
    GFile *file = gtk_file_dialog_save_finish(dialog, res, &error);
    if (error) {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            g_warning("Save dialog error: %s", error->message);
        g_error_free(error);
        g_object_unref(dialog);
        return;
    }

    if (file) {
        char *filename = g_file_get_path(file);
        if (filename) {
            check_file_save(filename);
            g_free(filename);
        }
        g_object_unref(file);
    }
    g_object_unref(dialog);
}
#endif

static void on_menuitem_save_as_activate(void)
{
#if GTK_CHECK_VERSION(4,0,0)
    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, _("Save Digest File"));
    gtk_file_dialog_set_accept_label(dialog, _("_Save"));
    gtk_file_dialog_save(dialog, GTK_WINDOW(gui.window), NULL,
        on_save_as_dialog_response, NULL);
#elif (GTK_CHECK_VERSION(3,20,0) && ENABLE_NATIVE_FILE_CHOOSER)
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(gtk_file_chooser_native_new(
        _("Save Digest File"), gui.window, GTK_FILE_CHOOSER_ACTION_SAVE,
        _("_Save"), _("_Cancel")));
#else
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(
        gtk_file_chooser_dialog_new(_("Save Digest File"), gui.window,
            GTK_FILE_CHOOSER_ACTION_SAVE,
            _("_Cancel"), GTK_RESPONSE_CANCEL,
            _("_Save"), GTK_RESPONSE_ACCEPT,
            NULL));
#endif

#if !GTK_CHECK_VERSION(4,0,0)
    gtk_file_chooser_set_select_multiple(chooser, false);
    gtk_file_chooser_set_local_only(chooser, true);
    gtk_file_chooser_set_do_overwrite_confirmation(chooser, true);
#endif

#if GTK_CHECK_VERSION(4,0,0)
    // Async handled above
#elif (GTK_CHECK_VERSION(3,20,0) && ENABLE_NATIVE_FILE_CHOOSER)
    if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(chooser);
        check_file_save(filename);
        g_free(filename);
    }
    g_object_unref(chooser);
#else
    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(chooser);
        check_file_save(filename);
        g_free(filename);
    }
    gtk_widget_destroy(GTK_WIDGET(chooser));
#endif
}

static void on_menuitem_quit_activate(void)
{
#if GTK_CHECK_VERSION(4,0,0)
    gtk_widget_set_visible(GTK_WIDGET(gui.window), false);
    g_application_quit(g_application_get_default());
#else
    gtk_widget_hide(GTK_WIDGET(gui.window));
    gtk_main_quit();
#endif
}

static void on_menuitem_edit_activate(void)
{
    GtkWidget *widget = gtk_window_get_focus(gui.window);
    bool selectable = false;
    bool editable = false;
    bool selection_ready = false;
    bool clipboard_ready = false;

    if (GTK_IS_ENTRY(widget)) {
        selectable = gtk_entry_get_text_length(GTK_ENTRY(widget));
        editable = gtk_editable_get_editable(GTK_EDITABLE(widget));
        selection_ready = gtk_editable_get_selection_bounds(
            GTK_EDITABLE(widget), NULL, NULL);
#if GTK_CHECK_VERSION(4,0,0)
        GdkClipboard *clipboard = gtk_widget_get_clipboard(GTK_WIDGET(widget));
        GdkContentFormats *formats = gdk_clipboard_get_formats(clipboard);
        clipboard_ready = gdk_content_formats_contain_gtype(formats, G_TYPE_STRING);
#else
        clipboard_ready = gtk_clipboard_wait_is_text_available(
            gtk_clipboard_get(GDK_NONE));
#endif
    }

    gtk_widget_set_sensitive(GTK_WIDGET(gui.menuitem_cut),
        selection_ready && editable);
    gtk_widget_set_sensitive(GTK_WIDGET(gui.menuitem_copy),
        selection_ready);
    gtk_widget_set_sensitive(GTK_WIDGET(gui.menuitem_paste),
        editable && clipboard_ready);
    gtk_widget_set_sensitive(GTK_WIDGET(gui.menuitem_delete),
        selection_ready && editable);
    gtk_widget_set_sensitive(GTK_WIDGET(gui.menuitem_select_all),
        selectable);
}

#if GTK_CHECK_VERSION(4,0,0)
static void clipboard_store_selection(GtkEditable *editable, GdkClipboard *clipboard, bool cut)
{
    GtkEntryBuffer *buffer = gtk_entry_get_buffer(GTK_ENTRY(editable));
    int start, end;
    if (gtk_editable_get_selection_bounds(editable, &start, &end)) {
        const char *text = gtk_entry_buffer_get_text(buffer);
        char *selected = g_strndup(text + start, end - start);
        gdk_clipboard_set_text(clipboard, selected);
        if (cut) {
            gtk_editable_delete_selection(editable);
        }
        g_free(selected);
    }
}

static void on_clipboard_read_text_ready(GObject *source, GAsyncResult *res, gpointer user_data)
{
    GdkClipboard *cb = GDK_CLIPBOARD(source);
    GError *error = NULL;
    char *text = gdk_clipboard_read_text_finish(cb, res, &error);
    if (text && !error) {
        GtkEditable *editable = GTK_EDITABLE(user_data);
        int pos = -1;
        gtk_editable_insert_text(editable, text, -1, &pos);
        g_free(text);
    }
    g_clear_error(&error);
    g_object_unref(user_data);
}

static void clipboard_paste(GtkEditable *editable, GdkClipboard *clipboard)
{
    gdk_clipboard_read_text_async(clipboard, NULL, on_clipboard_read_text_ready, g_object_ref(editable));
}
#endif

static void on_menuitem_cut_activate(void)
{
    GtkEditable *widget = GTK_EDITABLE(gtk_window_get_focus(gui.window));
#if GTK_CHECK_VERSION(4,0,0)
    GdkClipboard *clipboard = gtk_widget_get_clipboard(GTK_WIDGET(widget));
    clipboard_store_selection(widget, clipboard, true);
#else
    gtk_editable_cut_clipboard(widget);
#endif
}

static void on_menuitem_copy_activate(void)
{
    GtkEditable *widget = GTK_EDITABLE(gtk_window_get_focus(gui.window));
#if GTK_CHECK_VERSION(4,0,0)
    GdkClipboard *clipboard = gtk_widget_get_clipboard(GTK_WIDGET(widget));
    clipboard_store_selection(widget, clipboard, false);
#else
    gtk_editable_copy_clipboard(widget);
#endif
}

static void on_menuitem_paste_activate(void)
{
    GtkEditable *widget = GTK_EDITABLE(gtk_window_get_focus(gui.window));
#if GTK_CHECK_VERSION(4,0,0)
    GdkClipboard *clipboard = gtk_widget_get_clipboard(GTK_WIDGET(widget));
    clipboard_paste(widget, clipboard);
#else
    gtk_editable_paste_clipboard(widget);
#endif
}

static void on_menuitem_delete_activate(void)
{
    GtkEditable *widget = GTK_EDITABLE(gtk_window_get_focus(gui.window));

    gtk_editable_delete_selection(widget);
}

static void on_menuitem_select_all_activate(void)
{
    GtkEditable *widget = GTK_EDITABLE(gtk_window_get_focus(gui.window));

    gtk_editable_set_position(widget, -1);
    gtk_editable_select_region(widget, 0, -1);
}

static void on_menuitem_prefs_activate(void)
{
#if GTK_CHECK_VERSION(4,0,0)
    gtk_widget_set_visible(GTK_WIDGET(gui.dialog), true);
#else
    gtk_widget_show(GTK_WIDGET(gui.dialog));
#endif
}

static void on_radiomenuitem_toggled(void)
{
    enum gui_view_e view = GUI_VIEW_INVALID;

#if GTK_CHECK_VERSION(4,0,0)
    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(gui.radiomenuitem_file))) {
        view = GUI_VIEW_FILE;
    } else if (gtk_check_button_get_active(GTK_CHECK_BUTTON(gui.radiomenuitem_text))) {
        view = GUI_VIEW_TEXT;
    } else if (gtk_check_button_get_active(GTK_CHECK_BUTTON(gui.radiomenuitem_file_list))) {
        view = GUI_VIEW_FILE_LIST;
    }
#else
    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gui.radiomenuitem_file))) {
        view = GUI_VIEW_FILE;
    } else if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gui.radiomenuitem_text))) {
        view = GUI_VIEW_TEXT;
    } else if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gui.radiomenuitem_file_list))) {
        view = GUI_VIEW_FILE_LIST;
    }
#endif

    gui_set_view(view);
    gui_update();
}

static void on_menuitem_about_activate(void)
{
    static const char * const artists[] = {
        "Icon derived from GTK Logo "
        "https://wiki.gnome.org/Projects/GTK/Logo",
        NULL
    };

    static const char * const authors[] = {
        "Tristan Heaven <tristan@tristanheaven.net>",
        NULL
    };

    const char *snap_version = NULL;
    if (g_strcmp0(g_getenv("SNAP_NAME"), PACKAGE) == 0)
        snap_version = g_getenv("SNAP_VERSION");

    char *version = g_markup_printf_escaped("%s",
        snap_version ? snap_version : VERSION);

    gtk_show_about_dialog(
            gui.window,
            "artists", artists,
            "authors", authors,
            "comments", _("A desktop utility for computing message digests or checksums"),
            "license-type", GTK_LICENSE_GPL_2_0,
            "logo-icon-name", "org.gtkhash.gtkhash",
            "program-name", PACKAGE_NAME,
#if ENABLE_NLS
            "translator-credits", _("translator-credits"),
#endif
            "version", version,
            "website", "https://github.com/gtkhash/gtkhash",
            NULL);

    g_free(version);
}

#if !GTK_CHECK_VERSION(4,0,0)
static void on_filechooserbutton_selection_changed(void)
{
    bool enabled = hash_funcs_count_enabled();
    char *uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(gui.filechooserbutton));

    if (enabled && uri) {
        g_free(uri);
        gtk_widget_set_sensitive(GTK_WIDGET(gui.button_hash), true);
    } else
        gtk_widget_set_sensitive(GTK_WIDGET(gui.button_hash), false);

    gui_clear_digests();
}
#endif

#if GTK_CHECK_VERSION(4,0,0)
static void on_add_files_dialog_response(GObject *source, GAsyncResult *res, gpointer user_data)
{
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
    GError *error = NULL;
    GListModel *files = gtk_file_dialog_open_multiple_finish(dialog, res, &error);
    if (error) {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            g_warning("Add files dialog error: %s", error->message);
        g_error_free(error);
        g_object_unref(dialog);
        return;
    }

    guint n = g_list_model_get_n_items(files);
    GSList *uris = NULL;
    for (guint i = 0; i < n; i++) {
        GFile *file = G_FILE(g_list_model_get_item(files, i));
        char *uri = g_file_get_uri(file);
        uris = g_slist_append(uris, uri);
        g_object_unref(file);
    }

    GSList *ud_list = uri_digest_list_from_uri_list(uris);
    gui_add_ud_list(ud_list, GUI_VIEW_FILE_LIST);
    uri_digest_list_free_full(ud_list);
    g_slist_free_full(uris, g_free);
    g_object_unref(files);
    g_object_unref(dialog);
}
#endif

static void on_toolbutton_add_clicked(void)
{
#if GTK_CHECK_VERSION(4,0,0)
    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, _("Select Files"));
    gtk_file_dialog_set_accept_label(dialog, _("_Open"));
    gtk_file_dialog_open_multiple(dialog, GTK_WINDOW(gui.window), NULL,
        on_add_files_dialog_response, NULL);
#elif (GTK_CHECK_VERSION(3,20,0) && ENABLE_NATIVE_FILE_CHOOSER)
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(gtk_file_chooser_native_new(
        _("Select Files"), gui.window, GTK_FILE_CHOOSER_ACTION_OPEN,
        _("_Open"), _("_Cancel")));
    gtk_file_chooser_set_select_multiple(chooser, true);
    gtk_file_chooser_set_local_only(chooser, false);

    if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        GSList *uris = gtk_file_chooser_get_uris(chooser);
        GSList *ud_list = uri_digest_list_from_uri_list(uris);
        gui_add_ud_list(ud_list, GUI_VIEW_FILE_LIST);
        uri_digest_list_free_full(ud_list);
        g_slist_free(uris);
    }
    g_object_unref(chooser);
#else
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(
        gtk_file_chooser_dialog_new(_("Select Files"), gui.window,
            GTK_FILE_CHOOSER_ACTION_OPEN,
            _("_Cancel"), GTK_RESPONSE_CANCEL,
            _("_Open"), GTK_RESPONSE_ACCEPT,
            NULL));
    gtk_file_chooser_set_select_multiple(chooser, true);
    gtk_file_chooser_set_local_only(chooser, false);

    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        GSList *uris = gtk_file_chooser_get_uris(chooser);
        GSList *ud_list = uri_digest_list_from_uri_list(uris);
        gui_add_ud_list(ud_list, GUI_VIEW_FILE_LIST);
        uri_digest_list_free_full(ud_list);
        g_slist_free(uris);
    }
    gtk_widget_destroy(GTK_WIDGET(chooser));
#endif
}

static void on_treeselection_changed(void)
{
    const int rows = gtk_tree_selection_count_selected_rows(gui.treeselection);

    gtk_widget_set_sensitive(GTK_WIDGET(gui.toolbutton_remove), (rows > 0));
}

#if GTK_CHECK_VERSION(4,0,0)
static void show_menu_treeview(double x, double y)
#else
static void show_menu_treeview(GdkEventButton *event)
#endif
{
#if !GTK_CHECK_VERSION(4,0,0)
    const int rows = gtk_tree_selection_count_selected_rows(gui.treeselection);

    gtk_widget_set_sensitive(GTK_WIDGET(gui.menuitem_treeview_remove),
        (rows > 0));

    bool can_copy = false;

    if (rows == 1) {
        for (int i = 0; i < HASH_FUNCS_N; i++) {
            if (!hash.funcs[i].enabled)
                continue;
            char *digest = list_get_selected_digest(i);
            if (digest && *digest) {
                can_copy = true;
                gtk_widget_set_sensitive(GTK_WIDGET(
                    gui.hash_widgets[i].menuitem_treeview_copy), true);
            } else {
                gtk_widget_set_sensitive(GTK_WIDGET(
                    gui.hash_widgets[i].menuitem_treeview_copy), false);
            }
            g_free(digest);
        }
    }

    gtk_widget_set_sensitive(GTK_WIDGET(gui.menuitem_treeview_copy),
        can_copy);
#endif

#if GTK_CHECK_VERSION(4,0,0)
    GdkRectangle rect = { (int)x, (int)y, 1, 1 };
    gtk_popover_set_pointing_to(GTK_POPOVER(gui.menu_treeview), &rect);
    gtk_popover_popup(GTK_POPOVER(gui.menu_treeview));
#elif GTK_CHECK_VERSION(3,22,0)
    gtk_menu_popup_at_pointer(GTK_MENU(gui.menu_treeview), (GdkEvent *)event);
#else
    if (event) {
        gtk_menu_popup(GTK_MENU(gui.menu_treeview), NULL, NULL, NULL, NULL,
            event->button, event->time);
    } else {
        gtk_menu_popup(GTK_MENU(gui.menu_treeview), NULL, NULL, NULL, NULL,
            0, gtk_get_current_event_time());
    }
#endif
}

#if GTK_CHECK_VERSION(4,0,0)
static gboolean on_treeview_key_pressed(GtkEventControllerKey *controller,
                                        guint keyval, guint keycode,
                                        GdkModifierType state,
                                        gpointer user_data)
{
    if ((keyval == GDK_KEY_Menu) ||
        (keyval == GDK_KEY_F10 && (state & GDK_SHIFT_MASK)))
    {
        show_menu_treeview(0, 0);
        return GDK_EVENT_STOP;
    }

    return GDK_EVENT_PROPAGATE;
}
#endif

#if !GTK_CHECK_VERSION(4,0,0)
static void on_treeview_popup_menu(void)
{
    /* Note: Shift+F10 can trigger this, so it's possible for the pointer
       to be outside the window */

    show_menu_treeview(NULL);
}
#endif

#if !GTK_CHECK_VERSION(4,0,0)
static bool on_treeview_button_press_event(G_GNUC_UNUSED GtkWidget *widget,
    GdkEventButton *event)
{
    if (gdk_event_triggers_context_menu((GdkEvent *)event)) {
        show_menu_treeview(event);
        // Stop processing the event now so the selection won't be changed
        return true;
    }

    return false;
}

static void on_treeview_drag_data_received(G_GNUC_UNUSED GtkWidget *widget,
    GdkDragContext *context, G_GNUC_UNUSED gint x, G_GNUC_UNUSED gint y,
    GtkSelectionData *selection, G_GNUC_UNUSED guint info, guint t,
    G_GNUC_UNUSED gpointer data)
{
    char **uris = gtk_selection_data_get_uris(selection);
    if (!uris) {
        gtk_drag_finish(context, false, true, t);
        return;
    }

    GSList *ud_list = uri_digest_list_from_uri_strv(uris);

    gui_add_ud_list(ud_list, GUI_VIEW_FILE_LIST);

    uri_digest_list_free(ud_list);
    g_strfreev(uris);

    gtk_drag_finish(context, true, true, t);
}
#endif

static void on_menuitem_treeview_copy_activate(G_GNUC_UNUSED void *menuitem,
    struct hash_func_s *func)
{
    char *digest = list_get_selected_digest(func->id);
    g_assert(digest);

#if GTK_CHECK_VERSION(4,0,0)
    GdkClipboard *clipboard = gtk_widget_get_clipboard(GTK_WIDGET(gui.window));
    gdk_clipboard_set_text(clipboard, digest);
#else
    gtk_clipboard_set_text(gtk_clipboard_get(GDK_NONE), digest, -1);
#endif

    g_free(digest);
}

static void on_menuitem_treeview_show_toolbar_toggled(void)
{
#if GTK_CHECK_VERSION(4,0,0)
    const bool show_toolbar = gtk_check_button_get_active(
        GTK_CHECK_BUTTON(gui.menuitem_treeview_show_toolbar));
#else
    const bool show_toolbar = gtk_check_menu_item_get_active(
        GTK_CHECK_MENU_ITEM(gui.menuitem_treeview_show_toolbar));
#endif

    gtk_widget_set_visible(GTK_WIDGET(gui.toolbar), show_toolbar);
}

static void on_button_hash_clicked(G_GNUC_UNUSED GtkButton *button,
    struct hash_func_s *func)
{
    if (gui.view == GUI_VIEW_FILE) {
#if GTK_CHECK_VERSION(4,0,0)
        if (!gtk_widget_get_sensitive(GTK_WIDGET(gui.button_hash)))
            return;
#else
        // Workaround for when user clicks Cancel in FileChooserDialog and
        // uri is changed without emitting the "selection-changed" signal
        on_filechooserbutton_selection_changed();
        if (!gtk_widget_get_sensitive(GTK_WIDGET(gui.button_hash)))
            return;
#endif

        // Single-function hash
        if (func) {
            for (int i = 0; i < HASH_FUNCS_N; i++) {
                if (!hash.funcs[i].supported || i == func->id)
                    continue;
                hash.funcs[i].enabled = false;
            }
        }
    }

    gui_start_hash();
}

#if GTK_CHECK_VERSION(4,0,0)
static void on_entry_check_icon_press(GtkEntry *entry,
    GtkEntryIconPosition pos, G_GNUC_UNUSED gpointer data)
{
    if (pos != GTK_ENTRY_ICON_PRIMARY)
        return;

    gtk_editable_set_text(GTK_EDITABLE(entry), "");
    GdkClipboard *clipboard = gtk_widget_get_clipboard(GTK_WIDGET(entry));
    clipboard_paste(GTK_EDITABLE(entry), clipboard);
}
#else
static void on_entry_check_icon_press(GtkEntry *entry,
    GtkEntryIconPosition pos, GdkEventButton *event)
{
    if (pos != GTK_ENTRY_ICON_PRIMARY)
        return;
    if (event->type != GDK_BUTTON_PRESS)
        return;
    if (event->button != 1)
        return;

    gtk_entry_set_text(entry, "");
    gtk_editable_paste_clipboard(GTK_EDITABLE(entry));
}
#endif

static void on_togglebutton_hmac_file_toggled(void)
{
    g_assert(gui.view == GUI_VIEW_FILE);

    bool active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gui.togglebutton_hmac_file));
    gtk_widget_set_sensitive(GTK_WIDGET(gui.entry_hmac_file), active);

    gui_clear_digests();

    gui_update_hash_func_labels(active);
}

static void on_togglebutton_hmac_text_toggled(void)
{
    g_assert(gui.view == GUI_VIEW_TEXT);

    bool active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gui.togglebutton_hmac_text));
    gtk_widget_set_sensitive(GTK_WIDGET(gui.entry_hmac_text), active);

    hash_string();

    gui_update_hash_func_labels(active);
}

#if !GTK_CHECK_VERSION(4,0,0)
static void on_menuitem_show_hmac_key_toggled(GtkCheckMenuItem *item,
    GtkEntry *entry)
{
    const bool active = gtk_check_menu_item_get_active(item);

    gtk_entry_set_visibility(entry, active);
}
#endif

#if GTK_CHECK_VERSION(4,0,0)
static void on_entry_hmac_click_gesture_pressed(GtkGestureClick *gesture,
    int n_press, double x, double y, gpointer user_data)
{
    GtkEntry *entry = GTK_ENTRY(user_data);
    guint button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));

    if (button == GDK_BUTTON_SECONDARY) {
        GMenu *menu = g_menu_new();
        GMenuItem *item = g_menu_item_new(_("_Show HMAC Key"), "win.show-hmac-key");
        g_menu_append_item(menu, item);
        g_object_unref(item);

        GtkWidget *popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
        g_object_unref(menu);

        gtk_widget_set_parent(popover, GTK_WIDGET(entry));
        gtk_popover_popup(GTK_POPOVER(popover));
    }
}
#else
static void on_entry_hmac_populate_popup(GtkEntry *entry, GtkMenu *menu)
{
    GtkWidget *item;

    // Add separator
    item = gtk_separator_menu_item_new();
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_check_menu_item_new_with_mnemonic(_("_Show HMAC Key"));
    // Add checkbutton
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item),
        gtk_entry_get_visibility(entry));
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    g_signal_connect(item, "toggled",
        G_CALLBACK(on_menuitem_show_hmac_key_toggled), entry);
}
#endif

static bool on_dialog_delete_event(void)
{
#if GTK_CHECK_VERSION(4,0,0)
    gtk_widget_set_visible(GTK_WIDGET(gui.dialog), false);
#else
    gtk_widget_hide(GTK_WIDGET(gui.dialog));
#endif
    return true;
}

static void on_dialog_combobox_changed(void)
{
    for (int i = 0; i < HASH_FUNCS_N; i++) {
        if (!hash.funcs[i].supported)
            continue;

#if GTK_CHECK_VERSION(4,0,0)
        gtk_editable_set_text(GTK_EDITABLE(gui.hash_widgets[i].entry_file), "");
        gtk_editable_set_text(GTK_EDITABLE(gui.hash_widgets[i].entry_text), "");
#else
        gtk_entry_set_text(GTK_ENTRY(gui.hash_widgets[i].entry_file), "");
        gtk_entry_set_text(GTK_ENTRY(gui.hash_widgets[i].entry_text), "");
#endif
    }

    list_clear_digests();

    if (gui.view == GUI_VIEW_TEXT)
        hash_string();
    else
        gui_check_digests();
}

#if GTK_CHECK_VERSION(4,0,0)
static void on_treeview_click_gesture_pressed(GtkGestureClick *gesture,
    int n_press, double x, double y, gpointer user_data)
{
    guint button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
    GdkModifierType state = gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(gesture));

    if (button == GDK_BUTTON_SECONDARY || (button == GDK_BUTTON_PRIMARY && (state & GDK_CONTROL_MASK))) {
        show_menu_treeview(x, y);
    }
}

static GMenuModel * create_treeview_popover_model(void)
{
    GMenu *menu = g_menu_new();

    g_menu_append(menu, _("_Add"), "win.treeview_add");
    g_menu_append(menu, _("_Remove"), "win.treeview_remove");
    g_menu_append(menu, _("_Clear"), "win.treeview_clear");

    GMenu *copy_submenu = g_menu_new();
    for (int i = 0; i < HASH_FUNCS_N; i++) {
        if (!hash.funcs[i].supported)
            continue;
        gchar *action = g_strdup_printf("win.treeview_copy_hash('%s')", hash.funcs[i].name);
        g_menu_append(copy_submenu, hash.funcs[i].name, action);
        g_free(action);
    }
    g_menu_append_submenu(menu, _("Copy _Digest"), G_MENU_MODEL(copy_submenu));
    g_object_unref(copy_submenu);

    g_menu_append(menu, _("Show _Toolbar"), "win.treeview_show_toolbar");

    return G_MENU_MODEL(menu);
}

// Action callbacks for GTK4
static void on_app_quit_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    on_menuitem_quit_activate();
}
static void on_app_prefs_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    on_menuitem_prefs_activate();
}
static void on_app_about_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    on_menuitem_about_activate();
}

static void on_win_open_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    on_menuitem_open_activate();
}
static void on_win_save_as_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    on_menuitem_save_as_activate();
}
static void on_win_cut_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    on_menuitem_cut_activate();
}
static void on_win_copy_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    on_menuitem_copy_activate();
}
static void on_win_paste_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    on_menuitem_paste_activate();
}
static void on_win_delete_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    on_menuitem_delete_activate();
}
static void on_win_select_all_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    on_menuitem_select_all_activate();
}
static void on_win_view_mode_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    const gchar *mode = g_variant_get_string(parameter, NULL);
    g_simple_action_set_state(action, parameter);
    enum gui_view_e view = GUI_VIEW_INVALID;
    if (g_strcmp0(mode, "file") == 0)
        view = GUI_VIEW_FILE;
    else if (g_strcmp0(mode, "text") == 0)
        view = GUI_VIEW_TEXT;
    else if (g_strcmp0(mode, "file_list") == 0)
        view = GUI_VIEW_FILE_LIST;
    if (view != GUI_VIEW_INVALID) {
        gui_set_view(view);
        gui_update();
    }
}
static void on_win_treeview_show_toolbar_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    GVariant *state = g_action_get_state(G_ACTION(action));
    gboolean current = g_variant_get_boolean(state);
    g_variant_unref(state);
    gboolean new_state = !current;
    g_simple_action_set_state(action, g_variant_new_boolean(new_state));
    if (gui.toolbar)
        gtk_widget_set_visible(GTK_WIDGET(gui.toolbar), new_state);
}
static void on_win_treeview_copy_hash_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    const gchar *func_name = g_variant_get_string(parameter, NULL);
    for (int i = 0; i < HASH_FUNCS_N; i++) {
        if (hash.funcs[i].supported && g_strcmp0(hash.funcs[i].name, func_name) == 0) {
            on_menuitem_treeview_copy_activate(NULL, &hash.funcs[i]);
            return;
        }
    }
    g_warning("Unknown hash function: %s", func_name);
}

/* Proper GAction wrappers for existing helper functions */
static void on_win_treeview_add_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    on_toolbutton_add_clicked();
}

static void on_win_treeview_remove_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    list_remove_selection();
}

static void on_win_treeview_clear_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    list_clear();
}

/* GTK4 file chooser button dialog - now integrated with gui.c globals */
static void on_filechooserbutton_dialog_response(GObject *source, GAsyncResult *res, gpointer user_data)
{
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
    GError *error = NULL;
    GFile *file = gtk_file_dialog_open_finish(dialog, res, &error);
    if (file) {
        char *uri = g_file_get_uri(file);
        gui_filechooserbutton_set_uri(uri);
        g_free(uri);
        g_object_unref(file);
        gui_update();
    } else if (error) {
        g_warning("File chooser error: %s", error->message);
        g_error_free(error);
    }
    g_object_unref(dialog);
}

static void on_filechooserbutton_clicked(GtkButton *button, gpointer user_data)
{
    GtkWindow *window = GTK_WINDOW(gui.window);
    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, _("Open File"));
    gtk_file_dialog_open(dialog, window, NULL,
                         (GAsyncReadyCallback) on_filechooserbutton_dialog_response,
                         NULL);
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
void callbacks_init(GtkApplication *app)
#else
void callbacks_init(void)
#endif
{
#define CON(OBJ, SIG, CB) \
    g_signal_connect(G_OBJECT(OBJ), SIG, G_CALLBACK(CB), NULL)

#if !GTK_CHECK_VERSION(4,0,0)
    CON(gui.window,                         "delete-event",        on_window_delete_event);
#else
    g_signal_connect(gui.window, "close-request", G_CALLBACK(on_window_delete_event), NULL);
#endif

#if !GTK_CHECK_VERSION(4,0,0)
    CON(gui.menuitem_open,                  "activate",            on_menuitem_open_activate);
    CON(gui.menuitem_save_as,               "activate",            on_menuitem_save_as_activate);
    CON(gui.menuitem_quit,                  "activate",            on_menuitem_quit_activate);
    CON(gui.menuitem_edit,                  "activate",            on_menuitem_edit_activate);
    CON(gui.menuitem_cut,                   "activate",            on_menuitem_cut_activate);
    CON(gui.menuitem_copy,                  "activate",            on_menuitem_copy_activate);
    CON(gui.menuitem_paste,                 "activate",            on_menuitem_paste_activate);
    CON(gui.menuitem_delete,                "activate",            on_menuitem_delete_activate);
    CON(gui.menuitem_select_all,            "activate",            on_menuitem_select_all_activate);
    CON(gui.menuitem_prefs,                 "activate",            on_menuitem_prefs_activate);
    CON(gui.radiomenuitem_file,             "toggled",             on_radiomenuitem_toggled);
    CON(gui.radiomenuitem_text,             "toggled",             on_radiomenuitem_toggled);
    CON(gui.radiomenuitem_file_list,        "toggled",             on_radiomenuitem_toggled);
    CON(gui.menuitem_about,                 "activate",            on_menuitem_about_activate);
#endif

#if !GTK_CHECK_VERSION(4,0,0)
    CON(gui.filechooserbutton,              "selection-changed",   on_filechooserbutton_selection_changed);
#else
    /* Single connection for GTK4 file chooser button */
    g_signal_connect(gui.filechooserbutton, "clicked",
                     G_CALLBACK(on_filechooserbutton_clicked), NULL);
#endif
    CON(gui.entry_text,                     "changed",             hash_string);
    CON(gui.togglebutton_hmac_file,         "toggled",             on_togglebutton_hmac_file_toggled);
    CON(gui.togglebutton_hmac_text,         "toggled",             on_togglebutton_hmac_text_toggled);
#if GTK_CHECK_VERSION(4,0,0)
    GtkGesture *gesture_file = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture_file), 0);
    g_signal_connect(gesture_file, "pressed", G_CALLBACK(on_entry_hmac_click_gesture_pressed), gui.entry_hmac_file);
    gtk_widget_add_controller(GTK_WIDGET(gui.entry_hmac_file), GTK_EVENT_CONTROLLER(gesture_file));

    GtkGesture *gesture_text = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture_text), 0);
    g_signal_connect(gesture_text, "pressed", G_CALLBACK(on_entry_hmac_click_gesture_pressed), gui.entry_hmac_text);
    gtk_widget_add_controller(GTK_WIDGET(gui.entry_hmac_text), GTK_EVENT_CONTROLLER(gesture_text));
#else
    CON(gui.entry_hmac_file,                "populate-popup",      on_entry_hmac_populate_popup);
    CON(gui.entry_hmac_text,                "populate-popup",      on_entry_hmac_populate_popup);
#endif
    CON(gui.entry_hmac_file,                "changed",             gui_clear_digests);
    CON(gui.entry_hmac_text,                "changed",             hash_string);
    CON(gui.entry_check_file,               "changed",             gui_check_digests);
    CON(gui.entry_check_file,               "icon-press",          on_entry_check_icon_press);
    CON(gui.entry_check_text,               "changed",             gui_check_digests);
    CON(gui.entry_check_text,               "icon-press",          on_entry_check_icon_press);
#if !GTK_CHECK_VERSION(4,0,0)
    CON(gui.toolbutton_add,                 "clicked",             on_toolbutton_add_clicked);
    CON(gui.toolbutton_remove,              "clicked",             list_remove_selection);
    CON(gui.toolbutton_clear,               "clicked",             list_clear);
#endif
    CON(gui.treeselection,                  "changed",             on_treeselection_changed);
#if !GTK_CHECK_VERSION(4,0,0)
    CON(gui.treeview,                       "popup-menu",          on_treeview_popup_menu);
#else
    {
        GtkEventController *key_controller = gtk_event_controller_key_new();
        g_signal_connect(key_controller, "key-pressed",
                         G_CALLBACK(on_treeview_key_pressed), NULL);
        gtk_widget_add_controller(GTK_WIDGET(gui.treeview), key_controller);
    }
#endif
#if GTK_CHECK_VERSION(4,0,0)
    GtkGesture *treeview_gesture = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(treeview_gesture), 0);
    g_signal_connect(treeview_gesture, "pressed", G_CALLBACK(on_treeview_click_gesture_pressed), NULL);
    gtk_widget_add_controller(GTK_WIDGET(gui.treeview), GTK_EVENT_CONTROLLER(treeview_gesture));
#else
    CON(gui.treeview,                       "button-press-event",  on_treeview_button_press_event);
    CON(gui.treeview,                       "drag-data-received",  on_treeview_drag_data_received);
#endif

#if !GTK_CHECK_VERSION(4,0,0)
    CON(gui.menuitem_treeview_add,          "activate",            on_toolbutton_add_clicked);
    CON(gui.menuitem_treeview_remove,       "activate",            list_remove_selection);
    CON(gui.menuitem_treeview_clear,        "activate",            list_clear);
    CON(gui.menuitem_treeview_show_toolbar, "toggled",             on_menuitem_treeview_show_toolbar_toggled);
#endif

    CON(gui.button_hash,                    "clicked",             on_button_hash_clicked);
    CON(gui.button_stop,                    "clicked",             gui_stop_hash);
#if !GTK_CHECK_VERSION(4,0,0)
    CON(gui.dialog,                         "delete-event",        G_CALLBACK(on_dialog_delete_event));
#else
    g_signal_connect(gui.dialog, "close-request", G_CALLBACK(on_dialog_delete_event), NULL);
#endif
    CON(gui.dialog_button_close,            "clicked",             G_CALLBACK(on_dialog_delete_event));
    CON(gui.dialog_togglebutton_show_hmac,  "toggled",             gui_update);
    CON(gui.dialog_combobox,                "changed",             on_dialog_combobox_changed);

    for (int i = 0; i < HASH_FUNCS_N; i++) {
        if (!hash.funcs[i].supported)
            continue;

        CON(gui.hash_widgets[i].button, "toggled", gui_update);
        g_signal_connect(gui.hash_widgets[i].label_file, "clicked",
            G_CALLBACK(on_button_hash_clicked), &hash.funcs[i]);
#if !GTK_CHECK_VERSION(4,0,0)
        g_signal_connect(gui.hash_widgets[i].menuitem_treeview_copy,
            "activate", G_CALLBACK(on_menuitem_treeview_copy_activate),
            &hash.funcs[i]);
#endif
    }

#if GTK_CHECK_VERSION(4,0,0)
    // Register GTK4 actions
    {
        const GActionEntry app_entries[] = {
            { "quit",        on_app_quit_activate,   NULL, NULL, NULL },
            { "preferences", on_app_prefs_activate,  NULL, NULL, NULL },
            { "about",       on_app_about_activate,   NULL, NULL, NULL },
        };
        g_action_map_add_action_entries(G_ACTION_MAP(app), app_entries, G_N_ELEMENTS(app_entries), NULL);

        const GActionEntry win_entries[] = {
            { "open",        on_win_open_activate,        NULL, NULL, NULL },
            { "save_as",     on_win_save_as_activate,     NULL, NULL, NULL },
            { "cut",         on_win_cut_activate,         NULL, NULL, NULL },
            { "copy",        on_win_copy_activate,        NULL, NULL, NULL },
            { "paste",       on_win_paste_activate,       NULL, NULL, NULL },
            { "delete",      on_win_delete_activate,      NULL, NULL, NULL },
            { "select_all",  on_win_select_all_activate,  NULL, NULL, NULL },
            { "treeview_add",    on_win_treeview_add_activate,   NULL, NULL, NULL },
            { "treeview_remove", on_win_treeview_remove_activate, NULL, NULL, NULL },
            { "treeview_clear",  on_win_treeview_clear_activate,  NULL, NULL, NULL },
            { "treeview_copy_hash", on_win_treeview_copy_hash_activate, "s", NULL, NULL },
        };
        g_action_map_add_action_entries(G_ACTION_MAP(gui.window), win_entries, G_N_ELEMENTS(win_entries), NULL);

        // Stateful view mode action
        GSimpleAction *view_action = g_simple_action_new_stateful("view_mode",
            G_VARIANT_TYPE_STRING, g_variant_new_string("file"));
        g_signal_connect(view_action, "activate", G_CALLBACK(on_win_view_mode_activate), NULL);
        g_action_map_add_action(G_ACTION_MAP(gui.window), G_ACTION(view_action));

        // Stateful treeview_show_toolbar action
        GSimpleAction *toolbar_action = g_simple_action_new_stateful("treeview_show_toolbar",
            NULL, g_variant_new_boolean(FALSE));
        g_signal_connect(toolbar_action, "activate", G_CALLBACK(on_win_treeview_show_toolbar_activate), NULL);
        g_action_map_add_action(G_ACTION_MAP(gui.window), G_ACTION(toolbar_action));

        // Replace treeview popover model with dynamic one
        GMenuModel *treeview_menu = create_treeview_popover_model();
        gtk_popover_menu_set_menu_model(GTK_POPOVER_MENU(gui.menu_treeview), treeview_menu);
        g_object_unref(treeview_menu);
    }
#endif

#undef CON
}
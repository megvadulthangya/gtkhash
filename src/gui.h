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

#ifndef GUI_H
#define GUI_H

#include <stdbool.h>
#include <stdint.h>
#include <gtk/gtk.h>

#include "hash/hash-func.h"

enum gui_view_e {
    GUI_VIEW_INVALID = -1,
    GUI_VIEW_FILE,
    GUI_VIEW_TEXT,
    GUI_VIEW_FILE_LIST
};
#define GUI_VIEW_IS_VALID(view) ((view) >= GUI_VIEW_FILE)

enum gui_state_e {
    GUI_STATE_INVALID = -1,
    GUI_STATE_IDLE,
    GUI_STATE_BUSY
};
#define GUI_STATE_IS_VALID(state) ((state) >= GUI_STATE_IDLE)

struct hash_widget_s {
    GtkWidget *button;
    GtkWidget *label_file;
    GtkWidget *label_text;
    GtkWidget *entry_file;
    GtkWidget *entry_text;
#if !GTK_CHECK_VERSION(4, 0, 0)
    GtkWidget *menuitem_treeview_copy;
#endif
};

struct gui_s {
    GtkWindow *window;

#if !GTK_CHECK_VERSION(4, 0, 0)
    GtkWidget *menuitem_open;
    GtkWidget *menuitem_save_as;
    GtkWidget *menuitem_quit;
    GtkWidget *menuitem_edit;
    GtkWidget *menuitem_cut;
    GtkWidget *menuitem_copy;
    GtkWidget *menuitem_paste;
    GtkWidget *menuitem_delete;
    GtkWidget *menuitem_select_all;
    GtkWidget *menuitem_prefs;
    GtkWidget *radiomenuitem_file;
    GtkWidget *radiomenuitem_text;
    GtkWidget *radiomenuitem_file_list;
    GtkWidget *menuitem_about;
#endif

    // Toolbar
    GtkWidget *toolbar;
    GtkWidget *toolbutton_add;
    GtkWidget *toolbutton_remove;
    GtkWidget *toolbutton_clear;

    // Containers
    GtkBox *vbox_single;
    GtkBox *vbox_list;
    GtkBox *hbox_input;
    GtkBox *hbox_output;
    GtkBox *vbox_outputlabels;
    GtkBox *vbox_digests_file;
    GtkBox *vbox_digests_text;

    // Inputs
    GtkWidget *filechooserbutton;
    GtkEntry *entry_text;
    GtkEntry *entry_check_file;
    GtkEntry *entry_check_text;
    GtkToggleButton *togglebutton_hmac_file;
    GtkToggleButton *togglebutton_hmac_text;
    GtkEntry *entry_hmac_file;
    GtkEntry *entry_hmac_text;

    // Labels
    GtkLabel *label_file;
    GtkLabel *label_text;

    // Tree View
    GtkTreeView *treeview;
    GtkTreeSelection *treeselection;
#if !GTK_CHECK_VERSION(4, 0, 0)
    GtkMenu *menu_treeview;
    GtkWidget *menuitem_treeview_add;
    GtkWidget *menuitem_treeview_remove;
    GtkWidget *menuitem_treeview_clear;
    GtkMenu *menu_treeview_copy;
    GtkWidget *menuitem_treeview_copy;
    GtkWidget *menuitem_treeview_show_toolbar;
#else
    GObject *menu_treeview; /* GMenu or GtkPopoverMenu */
#endif

    // Buttons
    GtkSeparator *hseparator_buttons;
    GtkButton *button_hash;
    GtkButton *button_stop;

    // Progress Bar
    GtkProgressBar *progressbar;

    // Dialog
    GtkDialog *dialog;
    GtkGrid *dialog_grid;
    GtkWidget *dialog_togglebutton_show_hmac;
    GtkComboBox *dialog_combobox;
    GtkButton *dialog_button_close;

    // Hash
    struct hash_widget_s hash_widgets[HASH_FUNCS_N];
    enum gui_view_e view;

    // GTK3-specific
    GtkListStore *liststore;
    GtkTreeModel *treemodel;
};

extern struct gui_s gui;

void gui_init(void);
void gui_set_view(const enum gui_view_e view);
void gui_enable_hash_func(const enum hash_func_e id);
void gui_update_hash_funcs(void);
void gui_update_hash_func_labels(const bool hmac_enabled);
void gui_update(void);
void gui_set_digest_format(const enum digest_format_e format);
enum digest_format_e gui_get_digest_format(void);
const uint8_t *gui_get_hmac_key(size_t *key_size);
void gui_clear_digests(void);
void gui_check_digests(void);
void gui_set_state(const enum gui_state_e state);
enum gui_state_e gui_get_state(void);
bool gui_is_maximised(void);
void gui_start_hash(void);
void gui_stop_hash(void);
unsigned int gui_add_ud_list(GSList *ud_list, const enum gui_view_e view);
void gui_add_check(const char *check);
void gui_add_text(const char *text);
void gui_error(const char *message);
void gui_run(void);
void gui_deinit(void);

#if GTK_CHECK_VERSION(4, 0, 0)
void gui_set_application(GtkApplication *app);
void gui_set_test_resource(const char *resource_path);
void gui_filechooserbutton_set_uri(const char *uri);
char *gui_filechooserbutton_get_uri(void);
#endif

#endif

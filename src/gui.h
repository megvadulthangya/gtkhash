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

#ifndef GTKHASH_GUI_H
#define GTKHASH_GUI_H

#include <stdbool.h>
#include <gtk/gtk.h>

#include "hash.h"
#include "uri-digest.h"
#include "hash/hash-func.h"

#define GUI_VIEW_IS_VALID(X) (((X) >= 0) && ((X) <= GUI_VIEW_FILE_LIST))
#define GUI_STATE_IS_VALID(X) (((X) >= 0) && ((X) <= GUI_STATE_BUSY))

enum gui_view_e {
    GUI_VIEW_INVALID = -1,
    GUI_VIEW_FILE,
    GUI_VIEW_TEXT,
    GUI_VIEW_FILE_LIST,
};

enum gui_state_e {
    GUI_STATE_INVALID = -1,
    GUI_STATE_IDLE,
    GUI_STATE_BUSY,
};

extern struct gui_s {
    GtkWindow *window;
#if GTK_CHECK_VERSION(4, 0, 0)
    GtkWidget *menuitem_open, *menuitem_save_as, *menuitem_quit;
    GtkWidget *menuitem_edit;
    GtkWidget *menuitem_cut, *menuitem_copy, *menuitem_paste;
    GtkWidget *menuitem_delete, *menuitem_select_all, *menuitem_prefs;
    GtkWidget *menuitem_about;
    GtkWidget *radiomenuitem_file, *radiomenuitem_text, *radiomenuitem_file_list;
    GtkWidget *toolbar;
    GtkWidget *toolbutton_add, *toolbutton_remove, *toolbutton_clear;
#else
    GtkMenuItem *menuitem_open, *menuitem_save_as, *menuitem_quit;
    GtkMenuItem *menuitem_edit;
    GtkMenuItem *menuitem_cut, *menuitem_copy, *menuitem_paste;
    GtkMenuItem *menuitem_delete, *menuitem_select_all, *menuitem_prefs;
    GtkMenuItem *menuitem_about;
    GtkRadioMenuItem *radiomenuitem_file, *radiomenuitem_text, *radiomenuitem_file_list;
    GtkToolbar *toolbar;
    GtkToolButton *toolbutton_add, *toolbutton_remove, *toolbutton_clear;
#endif
    GtkBox *vbox_single, *vbox_list;
    GtkBox *hbox_input, *hbox_output;
    GtkBox *vbox_outputlabels, *vbox_digests_file, *vbox_digests_text;
    GtkEntry *entry_text, *entry_check_file, *entry_check_text;
    GtkEntry *entry_hmac_file, *entry_hmac_text;
#if GTK_CHECK_VERSION(4, 0, 0)
    GtkWidget *filechooserbutton;
#else
    GtkFileChooserButton *filechooserbutton;
#endif
    GtkToggleButton *togglebutton_hmac_file, *togglebutton_hmac_text;
    GtkLabel *label_file, *label_text;
    GtkTreeView *treeview;
    GtkTreeSelection *treeselection;
    GtkTreeModel *treemodel;
    GtkListStore *liststore;
#if GTK_CHECK_VERSION(4, 0, 0)
    GtkWidget *menu_treeview;
    GtkWidget *menuitem_treeview_add, *menuitem_treeview_remove;
    GtkWidget *menuitem_treeview_clear;
    GtkWidget *menu_treeview_copy;
    GtkWidget *menuitem_treeview_copy;
    GtkWidget *menuitem_treeview_show_toolbar;
#else
    GtkMenu *menu_treeview;
    GtkMenuItem *menuitem_treeview_add, *menuitem_treeview_remove;
    GtkMenuItem *menuitem_treeview_clear;
    GtkMenu *menu_treeview_copy;
    GtkMenuItem *menuitem_treeview_copy;
    GtkMenuItem *menuitem_treeview_show_toolbar;
#endif
    GtkSeparator *hseparator_buttons;
    GtkProgressBar *progressbar;
    GtkButton *button_hash, *button_stop;
    GtkDialog *dialog;
    GtkGrid *dialog_grid;
    GtkToggleButton *dialog_togglebutton_show_hmac;
    GtkComboBox *dialog_combobox;
    GtkButton *dialog_button_close;
    enum gui_view_e view;
    struct {
        GtkToggleButton *button;
#if GTK_CHECK_VERSION(4, 0, 0)
        GtkButton *label_file;
#else
        GtkModelButton *label_file;
#endif
        GtkLabel *label_text;
        GtkEntry *entry_file, *entry_text;
#if GTK_CHECK_VERSION(4, 0, 0)
        GtkWidget *menuitem_treeview_copy;
#else
        GtkMenuItem *menuitem_treeview_copy;
#endif
    } hash_widgets[HASH_FUNCS_N];
} gui;

void gui_init(void);
unsigned int gui_add_ud_list(GSList *ud_list, enum gui_view_e view);
void gui_add_check(const char *check);
void gui_add_text(const char *text);
void gui_error(const char *message);
void gui_run(void);
void gui_deinit(void);
void gui_set_view(enum gui_view_e view);
void gui_set_digest_format(enum digest_format_e format);
enum digest_format_e gui_get_digest_format(void);
const uint8_t *gui_get_hmac_key(size_t *key_size);
void gui_enable_hash_func(enum hash_func_e id);
void gui_update_hash_func_labels(bool hmac_enabled);
void gui_update_hash_funcs(void);
void gui_update(void);
void gui_clear_digests(void);
void gui_check_digests(void);
void gui_set_state(enum gui_state_e state);
bool gui_is_maximised(void);
void gui_start_hash(void);
void gui_stop_hash(void);

#if GTK_CHECK_VERSION(4, 0, 0)
void gui_set_application(GtkApplication *app);
void gui_filechooserbutton_set_uri(const char *uri);
char *gui_filechooserbutton_get_uri(void);
extern GFile *gui_filechooser_selected_file;
#endif

#endif
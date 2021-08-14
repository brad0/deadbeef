/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Alexey Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <assert.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include "../../../gettext.h"
#include "callbacks.h"
#include "ctmapping.h"
#include "ddblistview.h"
#include "drawing.h"
#include "dspconfig.h"
#include "eq.h"
#include "gtkui.h"
#include "hotkeys.h"
#include "interface.h"
#include "pluginconf.h"
#include "prefwin.h"
#include "prefwinplayback.h"
#include "prefwinsound.h"
#include "support.h"
#include "wingeom.h"

int PREFWIN_TAB_INDEX_SOUND = 0;
int PREFWIN_TAB_INDEX_PLAYBACK = 1;
int PREFWIN_TAB_INDEX_DSP = 2;
int PREFWIN_TAB_INDEX_GUI = 3;
int PREFWIN_TAB_INDEX_APPEARANCE = 4;
int PREFWIN_TAB_INDEX_MEDIALIB = 5;
int PREFWIN_TAB_INDEX_NETWORK = 6;
int PREFWIN_TAB_INDEX_HOTKEYS = 7;
int PREFWIN_TAB_INDEX_PLUGINS = 8;

static GtkWidget *prefwin;

enum {
    PLUGIN_LIST_COL_TITLE,
    PLUGIN_LIST_COL_IDX,
    PLUGIN_LIST_COL_BUILTIN,
    PLUGIN_LIST_COL_HASCONFIG
};

static GtkListStore *pluginliststore;
static GtkTreeModelFilter *pluginliststore_filtered;
static GtkMenu *pluginlistmenu;


void
prefwin_init_theme_colors (void) {
    GdkColor clr;
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "bar_background")), ((void)(gtkui_get_bar_background_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "bar_foreground")), ((void)(gtkui_get_bar_foreground_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_dark")), ((void)(gtkui_get_tabstrip_dark_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_mid")), ((void)(gtkui_get_tabstrip_mid_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_light")), ((void)(gtkui_get_tabstrip_light_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_base")), ((void)(gtkui_get_tabstrip_base_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_text")), ((void)(gtkui_get_tabstrip_text_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_playing_text")), ((void)(gtkui_get_tabstrip_playing_text_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_selected_text")), ((void)(gtkui_get_tabstrip_selected_text_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_even_row")), ((void)(gtkui_get_listview_even_row_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_odd_row")), ((void)(gtkui_get_listview_odd_row_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_selected_row")), ((void)(gtkui_get_listview_selection_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_text")), ((void)(gtkui_get_listview_text_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_selected_text")), ((void)(gtkui_get_listview_selected_text_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_playing_text")), ((void)(gtkui_get_listview_playing_text_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_group_text")), ((void)(gtkui_get_listview_group_text_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_column_text")), ((void)(gtkui_get_listview_column_text_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_cursor")), ((void)(gtkui_get_listview_cursor_color (&clr)), &clr));
}

void
prefwin_set_scale (const char *scale_name, int value) {
    GtkWidget *scale = lookup_widget (prefwin, scale_name);
    GSignalMatchType mask = G_SIGNAL_MATCH_DETAIL | G_SIGNAL_MATCH_DATA;
    GQuark detail = g_quark_from_static_string("value_changed");
    g_signal_handlers_block_matched ((gpointer)scale, mask, detail, 0, NULL, NULL, NULL);
    gtk_range_set_value (GTK_RANGE (scale), value);
    g_signal_handlers_unblock_matched ((gpointer)scale, mask, detail, 0, NULL, NULL, NULL);
}

void
prefwin_set_combobox (GtkComboBox *combo, int i) {
    GSignalMatchType mask = G_SIGNAL_MATCH_DETAIL | G_SIGNAL_MATCH_DATA;
    GQuark detail = g_quark_from_static_string("changed");
    g_signal_handlers_block_matched ((gpointer)combo, mask, detail, 0, NULL, NULL, NULL);
    gtk_combo_box_set_active (combo, i);
    g_signal_handlers_unblock_matched ((gpointer)combo, mask, detail, 0, NULL, NULL, NULL);
}

void
prefwin_set_toggle_button (const char *button_name, int value) {
    GtkToggleButton *button = GTK_TOGGLE_BUTTON (lookup_widget (prefwin, button_name));
    GSignalMatchType mask = G_SIGNAL_MATCH_DETAIL | G_SIGNAL_MATCH_DATA;
    GQuark detail = g_quark_from_static_string("toggled");
    g_signal_handlers_block_matched ((gpointer)button, mask, detail, 0, NULL, NULL, NULL);
    gtk_toggle_button_set_active (button, value);
    g_signal_handlers_unblock_matched ((gpointer)button, mask, detail, 0, NULL, NULL, NULL);
}

void
prefwin_set_entry_text (const char *entry_name, const char *text) {
    GtkEntry *entry = GTK_ENTRY (lookup_widget (prefwin, entry_name));
    GSignalMatchType mask = G_SIGNAL_MATCH_DETAIL | G_SIGNAL_MATCH_DATA;
    GQuark detail = g_quark_from_static_string("changed");
    g_signal_handlers_block_matched ((gpointer)entry, mask, detail, 0, NULL, NULL, NULL);
    gtk_entry_set_text (entry, text);
    g_signal_handlers_unblock_matched ((gpointer)entry, mask, detail, 0, NULL, NULL, NULL);
}

void
on_prefwin_response_cb (GtkDialog *dialog,
                        int        response_id,
                        gpointer   user_data) {
    if (response_id != GTK_RESPONSE_CLOSE && response_id != GTK_RESPONSE_DELETE_EVENT) {
        return;
    }

    if (gtkui_hotkeys_changed) {
        GtkWidget *dlg = gtk_message_dialog_new (GTK_WINDOW (prefwin), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("You modified the hotkeys settings, but didn't save your changes."));
        gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (prefwin));
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg), _("Are you sure you want to continue without saving?"));
        gtk_window_set_title (GTK_WINDOW (dlg), _("Warning"));
        int response = gtk_dialog_run (GTK_DIALOG (dlg));
        gtk_widget_destroy (dlg);
        if (response == GTK_RESPONSE_NO) {
            return;
        }
    }

    dsp_setup_free ();
    ctmapping_setup_free ();
    gtk_widget_destroy (prefwin);
    deadbeef->conf_save ();
    prefwin = NULL;
    pluginliststore = NULL;
    pluginliststore_filtered = NULL;
}





static void
_init_gui_misc_tab (void) {
    GtkWidget *w = prefwin;
    prefwin_set_toggle_button("minimize_on_startup", deadbeef->conf_get_int ("gtkui.start_hidden", 0));

    // close_send_to_tray
    prefwin_set_toggle_button("pref_close_send_to_tray", deadbeef->conf_get_int ("close_send_to_tray", 0));

    // hide tray icon
    prefwin_set_toggle_button("hide_tray_icon", deadbeef->conf_get_int ("gtkui.hide_tray_icon", 0));

    // move_to_trash
    prefwin_set_toggle_button("move_to_trash", deadbeef->conf_get_int ("gtkui.move_to_trash", 1));

    // mmb_delete_playlist
    prefwin_set_toggle_button("mmb_delete_playlist", deadbeef->conf_get_int ("gtkui.mmb_delete_playlist", 1));

    // hide_delete_from_disk
    prefwin_set_toggle_button("hide_delete_from_disk", deadbeef->conf_get_int ("gtkui.hide_remove_from_disk", 0));

    // play next song when currently played is deleted
    prefwin_set_toggle_button("skip_deleted_songs", deadbeef->conf_get_int ("gtkui.skip_deleted_songs", 0));

    // auto-rename playlist from folder name
    prefwin_set_toggle_button("auto_name_playlist_from_folder", deadbeef->conf_get_int ("gtkui.name_playlist_from_folder", 1));

    // refresh rate
    int val = deadbeef->conf_get_int ("gtkui.refresh_rate", 10);
    prefwin_set_scale("gui_fps", val);

    // titlebar text
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (w, "titlebar_format_playing")), deadbeef->conf_get_str_fast ("gtkui.titlebar_playing_tf", gtkui_default_titlebar_playing));
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (w, "titlebar_format_stopped")), deadbeef->conf_get_str_fast ("gtkui.titlebar_stopped_tf", gtkui_default_titlebar_stopped));

    // statusbar selection playback time
    prefwin_set_toggle_button ("display_seltime", deadbeef->conf_get_int ("gtkui.statusbar_seltime", 0));

    // enable shift-jis recoding
    prefwin_set_toggle_button("enable_shift_jis_recoding", deadbeef->conf_get_int ("junk.enable_shift_jis_detection", 0));

    // enable cp1251 recoding
    prefwin_set_toggle_button("enable_cp1251_recoding", deadbeef->conf_get_int ("junk.enable_cp1251_detection", 1));

    // enable cp936 recoding
    prefwin_set_toggle_button("enable_cp936_recoding", deadbeef->conf_get_int ("junk.enable_cp936_detection", 0));

    // enable auto-sizing of columns
    prefwin_set_toggle_button("auto_size_columns", deadbeef->conf_get_int ("gtkui.autoresize_columns", 0));

    gtk_spin_button_set_value(GTK_SPIN_BUTTON (lookup_widget (w, "listview_group_spacing")), deadbeef->conf_get_int ("playlist.groups.spacing", 0));

    // fill gui plugin list
    GtkComboBox *combobox = GTK_COMBO_BOX (lookup_widget (w, "gui_plugin"));
    const char **names = deadbeef->plug_get_gui_names ();
    for (int i = 0; names[i]; i++) {
        gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combobox), names[i]);
        if (!strcmp (names[i], deadbeef->conf_get_str_fast ("gui_plugin", "GTK2"))) {
            prefwin_set_combobox (combobox, i);
        }
    }
}

static void
_init_appearance_tab(void) {
    GtkWidget *w = prefwin;
    int override = deadbeef->conf_get_int ("gtkui.override_bar_colors", 0);
    prefwin_set_toggle_button("override_bar_colors", override);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "bar_colors_group"), override);

    // override tabstrip colors
    override = deadbeef->conf_get_int ("gtkui.override_tabstrip_colors", 0);
    prefwin_set_toggle_button("override_tabstrip_colors", override);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "tabstrip_colors_group"), override);

    prefwin_set_toggle_button("tabstrip_playing_bold", deadbeef->conf_get_int ("gtkui.tabstrip_embolden_playing", 0));
    prefwin_set_toggle_button("tabstrip_playing_italic", deadbeef->conf_get_int ("gtkui.tabstrip_italic_playing", 0));
    prefwin_set_toggle_button("tabstrip_selected_bold", deadbeef->conf_get_int ("gtkui.tabstrip_embolden_selected", 0));
    prefwin_set_toggle_button("tabstrip_selected_italic", deadbeef->conf_get_int ("gtkui.tabstrip_italic_selected", 0));

    // get default gtk font
    GtkStyle *style = gtk_widget_get_style (mainwin);
    const char *gtk_style_font = pango_font_description_to_string (style->font_desc);

    gtk_font_button_set_font_name (GTK_FONT_BUTTON (lookup_widget (w, "tabstrip_text_font")), deadbeef->conf_get_str_fast ("gtkui.font.tabstrip_text", gtk_style_font));

    // override listview colors
    override = deadbeef->conf_get_int ("gtkui.override_listview_colors", 0);
    prefwin_set_toggle_button("override_listview_colors", override);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "listview_colors_group"), override);

    // embolden/italic listview text
    prefwin_set_toggle_button("listview_selected_text_bold", deadbeef->conf_get_int ("gtkui.embolden_selected_tracks", 0));
    prefwin_set_toggle_button("listview_selected_text_italic", deadbeef->conf_get_int ("gtkui.italic_selected_tracks", 0));
    prefwin_set_toggle_button("listview_playing_text_bold", deadbeef->conf_get_int ("gtkui.embolden_current_track", 0));
    prefwin_set_toggle_button("listview_playing_text_italic", deadbeef->conf_get_int ("gtkui.italic_current_track", 0));

    gtk_font_button_set_font_name (GTK_FONT_BUTTON (lookup_widget (w, "listview_text_font")), deadbeef->conf_get_str_fast ("gtkui.font.listview_text", gtk_style_font));
    gtk_font_button_set_font_name (GTK_FONT_BUTTON (lookup_widget (w, "listview_group_text_font")), deadbeef->conf_get_str_fast ("gtkui.font.listview_group_text", gtk_style_font));
    gtk_font_button_set_font_name (GTK_FONT_BUTTON (lookup_widget (w, "listview_column_text_font")), deadbeef->conf_get_str_fast ("gtkui.font.listview_column_text", gtk_style_font));

    // colors
    prefwin_init_theme_colors ();
}

static void
_init_network_tab (void) {
    GtkWidget *w = prefwin;
    ctmapping_setup_init (prefwin);

    prefwin_set_toggle_button("pref_network_enableproxy", deadbeef->conf_get_int ("network.proxy", 0));
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (w, "pref_network_proxyaddress")), deadbeef->conf_get_str_fast ("network.proxy.address", ""));
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (w, "pref_network_proxyport")), deadbeef->conf_get_str_fast ("network.proxy.port", "8080"));
    GtkComboBox *combobox = GTK_COMBO_BOX (lookup_widget (w, "pref_network_proxytype"));
    const char *type = deadbeef->conf_get_str_fast ("network.proxy.type", "HTTP");
    if (!strcasecmp (type, "HTTP")) {
        prefwin_set_combobox (combobox, 0);
    }
    else if (!strcasecmp (type, "HTTP_1_0")) {
        prefwin_set_combobox (combobox, 1);
    }
    else if (!strcasecmp (type, "SOCKS4")) {
        prefwin_set_combobox (combobox, 2);
    }
    else if (!strcasecmp (type, "SOCKS5")) {
        prefwin_set_combobox (combobox, 3);
    }
    else if (!strcasecmp (type, "SOCKS4A")) {
        prefwin_set_combobox (combobox, 4);
    }
    else if (!strcasecmp (type, "SOCKS5_HOSTNAME")) {
        prefwin_set_combobox (combobox, 5);
    }
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (w, "proxyuser")), deadbeef->conf_get_str_fast ("network.proxy.username", ""));
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (w, "proxypassword")), deadbeef->conf_get_str_fast ("network.proxy.password", ""));

    char ua[100];
    deadbeef->conf_get_str ("network.http_user_agent", "deadbeef", ua, sizeof (ua));
    prefwin_set_entry_text("useragent", ua);
}

static void
_init_plugins_tab (void) {
    GtkWidget *w = prefwin;
    GtkTreeView *tree = GTK_TREE_VIEW (lookup_widget (w, "pref_pluginlist"));
    GtkCellRenderer *rend_text = gtk_cell_renderer_text_new ();
    // Order is: title, index, builtin, hasconfig
    GtkListStore *store = gtk_list_store_new (4, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_BOOLEAN);
    pluginliststore = store;
    GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes (_("Title"), rend_text, "text", PLUGIN_LIST_COL_TITLE, "weight", PLUGIN_LIST_COL_BUILTIN, NULL);
    gtk_tree_view_append_column (tree, col);
    gtk_tree_view_set_headers_visible (tree, FALSE);
    g_object_set (G_OBJECT (rend_text), "ellipsize", PANGO_ELLIPSIZE_END, NULL);

    DB_plugin_t **plugins = deadbeef->plug_get_list ();
    int i;
    const char *plugindir = deadbeef->get_system_dir (DDB_SYS_DIR_PLUGIN);
    for (i = 0; plugins[i]; i++) {
        GtkTreeIter it;
        const char *pluginpath;
        gtk_list_store_append (store, &it);
        pluginpath = deadbeef->plug_get_path_for_plugin_ptr (plugins[i]);
        if (!pluginpath) {
            pluginpath = plugindir;
        }
        gtk_list_store_set (store, &it,
                            PLUGIN_LIST_COL_TITLE, plugins[i]->name,
                            PLUGIN_LIST_COL_IDX, i,
                            PLUGIN_LIST_COL_BUILTIN, strstr(pluginpath, plugindir) ? PANGO_WEIGHT_NORMAL : PANGO_WEIGHT_BOLD,
                            PLUGIN_LIST_COL_HASCONFIG, plugins[i]->configdialog ? 1 : 0,
                            -1);
    }
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE(store), PLUGIN_LIST_COL_TITLE, GTK_SORT_ASCENDING);

    // Create a filtered model, then we switch between the two models to show/hide configurable plugins
    pluginliststore_filtered = GTK_TREE_MODEL_FILTER(gtk_tree_model_filter_new (GTK_TREE_MODEL(store), NULL));
    gtk_tree_model_filter_set_visible_column (pluginliststore_filtered, PLUGIN_LIST_COL_HASCONFIG);

    gtk_tree_view_set_model (tree, GTK_TREE_MODEL (store));

    pluginlistmenu = GTK_MENU(create_plugin_list_popup_menu ());
    gtk_menu_attach_to_widget (GTK_MENU (pluginlistmenu), GTK_WIDGET (tree), NULL);

    // We do this here so one can leave the tabs visible in the Glade designer for convenience.
    GtkNotebook *notebook = GTK_NOTEBOOK (lookup_widget (w, "plugin_notebook"));
    gtk_notebook_set_show_tabs(notebook, FALSE);
    gtk_notebook_set_current_page(notebook, 0);

    // Some styling changes since Glade doesn't support setting it
#if GTK_CHECK_VERSION(3,12,0)
    GtkButtonBox *bbox = GTK_BUTTON_BOX (lookup_widget (w, "plugin_tabbtn_hbtnbox"));
    gtk_button_box_set_layout (bbox, GTK_BUTTONBOX_EXPAND);
#endif
}

static void
_init_prefwin() {
    if (prefwin != NULL) {
        return;
    }
    GtkWidget *w = prefwin = create_prefwin ();

    // hide unavailable tabs
    if (!deadbeef->plug_get_for_id ("hotkeys")) {
        gtk_notebook_remove_page (GTK_NOTEBOOK (lookup_widget (prefwin, "notebook")), 7);
        PREFWIN_TAB_INDEX_HOTKEYS = -1;
    }
    if (!deadbeef->plug_get_for_id ("medialib")) {
        gtk_notebook_remove_page (GTK_NOTEBOOK (lookup_widget (prefwin, "notebook")), 5);
        PREFWIN_TAB_INDEX_MEDIALIB = -1;
    }

    gtk_window_set_transient_for (GTK_WINDOW (w), GTK_WINDOW (mainwin));

    deadbeef->conf_lock ();

    // output plugin selection
    prefwin_init_sound_tab (prefwin);

    // replaygain_mode
    prefwin_init_playback_tab(prefwin);

    // dsp
    dsp_setup_init (prefwin);

    // minimize_on_startup
    _init_gui_misc_tab ();

    // override bar colors
    _init_appearance_tab();

    // network
    // content-type mapping dialog
    _init_network_tab ();

    // list of plugins
    _init_plugins_tab ();

    // hotkeys
    if (PREFWIN_TAB_INDEX_HOTKEYS != -1) {
        prefwin_init_hotkeys (prefwin);
    }

    deadbeef->conf_unlock ();

    g_signal_connect (GTK_DIALOG (prefwin), "response", G_CALLBACK (on_prefwin_response_cb), NULL);

    gtk_window_set_modal (GTK_WINDOW (prefwin), FALSE);
    gtk_window_set_position (GTK_WINDOW (prefwin), GTK_WIN_POS_CENTER_ON_PARENT);
}

void
prefwin_run (int tab_index) {
    _init_prefwin();

    if (tab_index != -1) {
        gtk_notebook_set_current_page(GTK_NOTEBOOK (lookup_widget (prefwin, "notebook")), tab_index);
    }

#if GTK_CHECK_VERSION(2,28,0)
    gtk_window_present_with_time (GTK_WINDOW(prefwin), (guint32)(g_get_monotonic_time() / 1000));
#else
    gtk_window_present_with_time (prefwin, GDK_CURRENT_TIME);
#endif
}

void
on_only_show_plugins_with_configuration1_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *w = prefwin;
    GtkTreeView *tree = GTK_TREE_VIEW (lookup_widget (w, "pref_pluginlist"));
    int active = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem));
    if (active) {
       gtk_tree_view_set_model (tree, GTK_TREE_MODEL (pluginliststore_filtered));
    } else {
       gtk_tree_view_set_model (tree, GTK_TREE_MODEL (pluginliststore));
    }
}

gboolean
on_pref_pluginlist_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    if (event->button == 3) {
        gtk_menu_popup (GTK_MENU (pluginlistmenu), NULL, NULL, NULL, NULL, event->button, gtk_get_current_event_time());
        return TRUE;
    }

    return FALSE;
}


void
on_pref_replaygain_source_mode_changed (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    int active = gtk_combo_box_get_active (combobox);
    deadbeef->conf_set_int ("replaygain.source_mode", active);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}


void
on_pref_replaygain_processing_changed  (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    uint32_t flags = 0;
    int idx = gtk_combo_box_get_active (combobox);
    if (idx == 1) {
        flags = DDB_RG_PROCESSING_GAIN;
    }
    if (idx == 2) {
        flags = DDB_RG_PROCESSING_GAIN | DDB_RG_PROCESSING_PREVENT_CLIPPING;
    }
    if (idx == 3) {
        flags = DDB_RG_PROCESSING_PREVENT_CLIPPING;
    }

    deadbeef->conf_set_int ("replaygain.processing_flags", flags);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}


void
on_replaygain_preamp_value_changed     (GtkRange        *range,
                                        gpointer         user_data)
{
    float val = gtk_range_get_value (range);
    deadbeef->conf_set_float ("replaygain.preamp_with_rg", val);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_global_preamp_value_changed     (GtkRange        *range,
                                    gpointer         user_data)
{
    float val = gtk_range_get_value (range);
    deadbeef->conf_set_float ("replaygain.preamp_without_rg", val);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_minimize_on_startup_clicked     (GtkButton       *button,
                                   gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
    deadbeef->conf_set_int ("gtkui.start_hidden", active);
    if (active == 1) {
        prefwin_set_toggle_button("hide_tray_icon", 0);
        deadbeef->conf_set_int ("gtkui.hide_tray_icon", 0);
    }
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_move_to_trash_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
    deadbeef->conf_set_int ("gtkui.move_to_trash", active);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_pref_close_send_to_tray_clicked     (GtkButton       *button,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
    deadbeef->conf_set_int ("close_send_to_tray", active);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_hide_tray_icon_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (togglebutton);
    deadbeef->conf_set_int ("gtkui.hide_tray_icon", active);
    if (active == 1) {
        prefwin_set_toggle_button("minimize_on_startup", 0);
        deadbeef->conf_set_int ("gtkui.start_hidden", 0);
    }
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_mmb_delete_playlist_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("gtkui.mmb_delete_playlist", gtk_toggle_button_get_active (togglebutton));
}

void
gtkui_conf_get_str (const char *key, char *value, int len, const char *def) {
    deadbeef->conf_get_str (key, def, value, len);
}

void plugin_pref_prop_changed_cb(ddb_pluginprefs_dialog_t *make_dialog_conf) {
    apply_conf (GTK_WIDGET (make_dialog_conf->containerbox), &make_dialog_conf->dialog_conf, 0);
}

void
on_pref_pluginlist_cursor_changed      (GtkTreeView     *treeview,
                                        gpointer         user_data)
{
    GtkTreePath *path;
    GtkTreeViewColumn *col;
    gtk_tree_view_get_cursor (treeview, &path, &col);
    if (!path || !col) {
        // reset
        return;
    }
    GtkTreeIter iter;
    GtkTreeModel *model;
    int idx;
    model = gtk_tree_view_get_model (treeview);
    if (!gtk_tree_model_get_iter (model, &iter, path)) {
        return;
    }

    gtk_tree_model_get (model, &iter, PLUGIN_LIST_COL_IDX, &idx, -1);
    
    DB_plugin_t **plugins = deadbeef->plug_get_list ();
    DB_plugin_t *p = plugins[idx];
    
    assert (p);
    GtkWidget *w = prefwin;
    assert (w);

    char s[20];
    snprintf (s, sizeof (s), "%d.%d", p->version_major, p->version_minor);
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (w, "plug_version")), s);

    if (p->descr) {
        GtkTextView *tv = GTK_TEXT_VIEW (lookup_widget (w, "plug_description"));

        GtkTextBuffer *buffer = gtk_text_buffer_new (NULL);

        gtk_text_buffer_set_text (buffer, p->descr, (gint)strlen(p->descr));
        gtk_text_view_set_buffer (GTK_TEXT_VIEW (tv), buffer);
        g_object_unref (buffer);
    }

    GtkWidget *link = lookup_widget (w, "weblink");
    if (p->website) {
        gtk_link_button_set_uri (GTK_LINK_BUTTON(link), p->website);
        gtk_widget_set_sensitive (link, TRUE);
    }
    else {
        gtk_link_button_set_uri (GTK_LINK_BUTTON(link), "");
        gtk_widget_set_sensitive (link, FALSE);
    }

    GtkTextView *lictv = GTK_TEXT_VIEW (lookup_widget (w, "plug_license"));
    if (p->copyright) {
        GtkTextBuffer *buffer = gtk_text_buffer_new (NULL);

        gtk_text_buffer_set_text (buffer, p->copyright, (gint)strlen(p->copyright));
        gtk_text_view_set_buffer (GTK_TEXT_VIEW (lictv), buffer);
        g_object_unref (buffer);
    }
    else {
        gtk_text_view_set_buffer(lictv, NULL);
    }

    GtkWidget *plugin_actions_btnbox = lookup_widget (w, "plugin_actions_btnbox");

    GtkWidget *container = (lookup_widget (w, "plug_conf_dlg_viewport"));
    GtkWidget *child = gtk_bin_get_child (GTK_BIN (container));
    if (child)
        gtk_widget_destroy(child);

    if (p->configdialog) {
         ddb_dialog_t conf = {
            .title = p->name,
            .layout = p->configdialog,
            .set_param = deadbeef->conf_set_str,
            .get_param = gtkui_conf_get_str,
        };
        ddb_pluginprefs_dialog_t make_dialog_conf = {
            .dialog_conf = conf,
            .parent = prefwin,
            .prop_changed = plugin_pref_prop_changed_cb,
        };
        GtkWidget *box = gtk_vbox_new(FALSE, 0);
        gtk_widget_show (box);
        if (user_data == (void *)1) {
            apply_conf (box, &conf, 1);
        }
        make_dialog_conf.containerbox = box;
        gtk_container_add (GTK_CONTAINER (container), box);
        gtkui_make_dialog (&make_dialog_conf);

        gtk_widget_show (plugin_actions_btnbox);
    } else {
        gtk_widget_hide (plugin_actions_btnbox);
    }
}

void
on_plugin_conf_reset_btn_clicked       (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *w = prefwin;
    GtkTreeView *treeview = GTK_TREE_VIEW (lookup_widget (w, "pref_pluginlist"));
    on_pref_pluginlist_cursor_changed(treeview, (void *)1);
}

void
on_pref_pluginlist_row_activated       (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data)
{
    GtkWidget *w = prefwin;
    GtkNotebook *notebook = GTK_NOTEBOOK (lookup_widget (w, "plugin_notebook"));
    gtk_notebook_set_current_page (notebook, 0);
}

void
on_plugin_conf_tab_btn_clicked         (GtkRadioButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *w = prefwin;
    GtkNotebook *notebook = GTK_NOTEBOOK (lookup_widget (w, "plugin_notebook"));
    gtk_notebook_set_current_page (notebook, 0);
}


void
on_plugin_info_tab_btn_clicked         (GtkRadioButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *w = prefwin;
    GtkNotebook *notebook = GTK_NOTEBOOK (lookup_widget (w, "plugin_notebook"));
    gtk_notebook_set_current_page (notebook, 1);
}


void
on_plugin_license_tab_btn_clicked      (GtkRadioButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *w = prefwin;
    GtkNotebook *notebook = GTK_NOTEBOOK (lookup_widget (w, "plugin_notebook"));
    gtk_notebook_set_current_page (notebook, 2);
}

//  This code is only here so you can programmatically switch page
//  of the notebook and have the radio buttons update accordingly.
void
on_plugin_notebook_switch_page         (GtkNotebook     *notebook,
                                        GtkWidget       *page,
                                        guint            page_num,
                                        gpointer         user_data)
{
    GtkWidget *w = prefwin;
    GtkToggleButton *plugin_conf_tab_btn = GTK_TOGGLE_BUTTON (lookup_widget (w, "plugin_conf_tab_btn"));
    GtkToggleButton *plugin_info_tab_btn = GTK_TOGGLE_BUTTON (lookup_widget (w, "plugin_info_tab_btn"));
    GtkToggleButton *plugin_license_tab_btn = GTK_TOGGLE_BUTTON (lookup_widget (w, "plugin_license_tab_btn"));

    GSignalMatchType mask = G_SIGNAL_MATCH_DETAIL | G_SIGNAL_MATCH_DATA;
    GQuark detail = g_quark_from_static_string ("switch_page");
    g_signal_handlers_block_matched ((gpointer)notebook, mask, detail, 0, NULL, NULL, NULL);

    switch (page_num) {
        case 0:
        gtk_toggle_button_set_active (plugin_conf_tab_btn, 1);
        break;
        case 1:
        gtk_toggle_button_set_active (plugin_info_tab_btn, 1);
        break;
        case 2:
        gtk_toggle_button_set_active (plugin_license_tab_btn, 1);
    }
    g_signal_handlers_unblock_matched ((gpointer)notebook, mask, detail, 0, NULL, NULL, NULL);
}

static void
override_set_helper (GtkToggleButton  *togglebutton, const char* conf_str, const char *group_name)
{
    int active = gtk_toggle_button_get_active (togglebutton);
    deadbeef->conf_set_int (conf_str, active);
    gtk_widget_set_sensitive (lookup_widget (prefwin, group_name), active);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, (uintptr_t)conf_str, 0, 0);
    gtkui_init_theme_colors ();
    prefwin_init_theme_colors ();
}

static void
font_set_helper (GtkFontButton *fontbutton, const char* conf_str)
{
    deadbeef->conf_set_str (conf_str, gtk_font_button_get_font_name (fontbutton));
    gtkui_init_theme_colors ();
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, (uintptr_t)conf_str, 0, 0);
}

static int
font_style_set_helper (GtkToggleButton  *togglebutton, const char* conf_str)
{
    int active = gtk_toggle_button_get_active (togglebutton);
    deadbeef->conf_set_int (conf_str, active);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, (uintptr_t)conf_str, 0, 0);
    return active;
}

static void
color_set_helper (GtkColorButton *colorbutton, const char* conf_str)
{
    if (conf_str == NULL) {
        return;
    }
    GdkColor clr;
    gtk_color_button_get_color (colorbutton, &clr);
    char str[100];
    snprintf (str, sizeof (str), "%d %d %d", clr.red, clr.green, clr.blue);
    deadbeef->conf_set_str (conf_str, str);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, (uintptr_t)conf_str, 0, 0);
    gtkui_init_theme_colors ();
}
void
on_tabstrip_light_color_set            (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.tabstrip_light");
}


void
on_tabstrip_mid_color_set              (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.tabstrip_mid");
}


void
on_tabstrip_dark_color_set             (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.tabstrip_dark");
}

void
on_tabstrip_base_color_set             (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.tabstrip_base");
}

void
on_tabstrip_text_color_set             (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.tabstrip_text");
}

void
on_tabstrip_selected_text_color_set    (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.tabstrip_selected_text");
}

void
on_tabstrip_playing_bold_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    gtkui_tabstrip_embolden_playing = font_style_set_helper (togglebutton, "gtkui.tabstrip_embolden_playing");
}

void
on_tabstrip_playing_italic_toggled     (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    gtkui_tabstrip_italic_playing = font_style_set_helper (togglebutton, "gtkui.tabstrip_italic_playing");
}

void
on_tabstrip_selected_bold_toggled      (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    gtkui_tabstrip_embolden_selected = font_style_set_helper (togglebutton, "gtkui.tabstrip_embolden_selected");
}

void
on_tabstrip_selected_italic_toggled    (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    gtkui_tabstrip_italic_selected = font_style_set_helper (togglebutton, "gtkui.tabstrip_italic_selected");
}

void
on_tabstrip_text_font_set              (GtkFontButton   *fontbutton,
                                        gpointer         user_data)
{
    font_set_helper (fontbutton, "gtkui.font.tabstrip_text");
}

void
on_tabstrip_playing_text_color_set     (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.tabstrip_playing_text");
}

void
on_bar_foreground_color_set            (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.bar_foreground");
    eq_redraw ();
}


void
on_bar_background_color_set            (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.bar_background");
    eq_redraw ();
}

void
on_override_listview_colors_toggled    (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    override_set_helper (togglebutton, "gtkui.override_listview_colors", "listview_colors_group");
}


void
on_listview_even_row_color_set         (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.listview_even_row");
}

void
on_listview_odd_row_color_set          (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.listview_odd_row");
}

void
on_listview_selected_row_color_set     (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.listview_selection");
}

void
on_listview_text_color_set             (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.listview_text");
}


void
on_listview_selected_text_color_set    (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.listview_selected_text");
}

void
on_listview_cursor_color_set           (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.listview_cursor");
}

void
on_listview_playing_text_color_set     (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.listview_playing_text");
}

void
on_listview_group_text_color_set       (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.listview_group_text");
}

void
on_listview_group_text_font_set        (GtkFontButton   *fontbutton,
                                        gpointer         user_data)
{
    font_set_helper (fontbutton, "gtkui.font.listview_group_text");
}

void
on_listview_text_font_set              (GtkFontButton   *fontbutton,
                                        gpointer         user_data)
{
    font_set_helper (fontbutton, "gtkui.font.listview_text");
}

void
on_listview_playing_text_bold_toggled  (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    gtkui_embolden_current_track = font_style_set_helper (togglebutton, "gtkui.embolden_current_track");
}

void
on_listview_playing_text_italic_toggled (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    gtkui_italic_current_track = font_style_set_helper (togglebutton, "gtkui.italic_current_track");
}

void
on_listview_selected_text_bold_toggled (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    gtkui_embolden_selected_tracks = font_style_set_helper (togglebutton, "gtkui.embolden_selected_tracks");
}

void
on_listview_selected_text_italic_toggled (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    gtkui_italic_selected_tracks = font_style_set_helper (togglebutton, "gtkui.italic_selected_tracks");
}

void
on_listview_column_text_color_set      (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.listview_column_text");
}

void
on_listview_column_text_font_set       (GtkFontButton   *fontbutton,
                                        gpointer         user_data)
{
    font_set_helper (fontbutton, "gtkui.font.listview_column_text");
}

void
on_override_bar_colors_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    override_set_helper (togglebutton, "gtkui.override_bar_colors", "bar_colors_group");
    eq_redraw ();
}

void
on_override_tabstrip_colors_toggled    (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    override_set_helper (togglebutton, "gtkui.override_tabstrip_colors", "tabstrip_colors_group");
}

void
on_pref_network_proxyaddress_changed   (GtkEditable     *editable,
                                        gpointer         user_data)
{
    deadbeef->conf_set_str ("network.proxy.address", gtk_entry_get_text (GTK_ENTRY (editable)));
}


void
on_pref_network_enableproxy_clicked    (GtkButton       *button,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("network.proxy", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)));
}


void
on_pref_network_proxyport_changed      (GtkEditable     *editable,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("network.proxy.port", atoi (gtk_entry_get_text (GTK_ENTRY (editable))));
}


void
on_pref_network_proxytype_changed      (GtkComboBox     *combobox,
                                        gpointer         user_data)
{

    int active = gtk_combo_box_get_active (combobox);
    switch (active) {
    case 0:
        deadbeef->conf_set_str ("network.proxy.type", "HTTP");
        break;
    case 1:
        deadbeef->conf_set_str ("network.proxy.type", "HTTP_1_0");
        break;
    case 2:
        deadbeef->conf_set_str ("network.proxy.type", "SOCKS4");
        break;
    case 3:
        deadbeef->conf_set_str ("network.proxy.type", "SOCKS5");
        break;
    case 4:
        deadbeef->conf_set_str ("network.proxy.type", "SOCKS4A");
        break;
    case 5:
        deadbeef->conf_set_str ("network.proxy.type", "SOCKS5_HOSTNAME");
        break;
    default:
        deadbeef->conf_set_str ("network.proxy.type", "HTTP");
        break;
    }
}

void
on_proxyuser_changed                   (GtkEditable     *editable,
                                        gpointer         user_data)
{
    deadbeef->conf_set_str ("network.proxy.username", gtk_entry_get_text (GTK_ENTRY (editable)));
}


void
on_proxypassword_changed               (GtkEditable     *editable,
                                        gpointer         user_data)
{
    deadbeef->conf_set_str ("network.proxy.password", gtk_entry_get_text (GTK_ENTRY (editable)));
}

void
on_hide_delete_from_disk_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (togglebutton));
    deadbeef->conf_set_int ("gtkui.hide_remove_from_disk", active);
}

void
on_skip_deleted_songs_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (togglebutton));
    deadbeef->conf_set_int ("gtkui.skip_deleted_songs", active);
}

void
on_titlebar_format_playing_changed     (GtkEditable     *editable,
                                        gpointer         user_data)
{
    deadbeef->conf_set_str ("gtkui.titlebar_playing_tf", gtk_entry_get_text (GTK_ENTRY (editable)));
    gtkui_titlebar_tf_init ();
    gtkui_set_titlebar (NULL);
}


void
on_titlebar_format_stopped_changed     (GtkEditable     *editable,
                                        gpointer         user_data)
{
    deadbeef->conf_set_str ("gtkui.titlebar_stopped_tf", gtk_entry_get_text (GTK_ENTRY (editable)));
    gtkui_titlebar_tf_init ();
    gtkui_set_titlebar (NULL);
}

void
on_cli_add_to_playlist_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (togglebutton));
    deadbeef->conf_set_int ("cli_add_to_specific_playlist", active);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "cli_playlist_name"), active);
}


void
on_cli_playlist_name_changed           (GtkEditable     *editable,
                                        gpointer         user_data)
{
    deadbeef->conf_set_str ("cli_add_playlist_name", gtk_entry_get_text (GTK_ENTRY (editable)));
}

void
on_resume_last_session_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (togglebutton));
    deadbeef->conf_set_int ("resume_last_session", active);
}

void
on_enable_shift_jis_recoding_toggled   (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (togglebutton));
    deadbeef->conf_set_int ("junk.enable_shift_jis_detection", active);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_enable_cp1251_recoding_toggled      (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (togglebutton));
    deadbeef->conf_set_int ("junk.enable_cp1251_detection", active);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}


void
on_enable_cp936_recoding_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (togglebutton));
    deadbeef->conf_set_int ("junk.enable_cp936_detection", active);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}


void
on_auto_name_playlist_from_folder_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (togglebutton));
    deadbeef->conf_set_int ("gtkui.name_playlist_from_folder", active);
}

static void
show_copyright_window (const char *text, const char *title, GtkWidget **pwindow) {
    if (*pwindow) {
        return;
    }
    GtkWidget *widget = *pwindow = create_helpwindow ();
    g_object_set_data (G_OBJECT (widget), "pointer", pwindow);
    g_signal_connect (widget, "delete_event", G_CALLBACK (on_gtkui_info_window_delete), pwindow);
    gtk_window_set_title (GTK_WINDOW (widget), title);
    gtk_window_set_transient_for (GTK_WINDOW (widget), GTK_WINDOW (prefwin));
    GtkWidget *txt = lookup_widget (widget, "helptext");
    GtkTextBuffer *buffer = gtk_text_buffer_new (NULL);

    gtk_text_buffer_set_text (buffer, text, (gint)strlen(text));
    gtk_text_view_set_buffer (GTK_TEXT_VIEW (txt), buffer);
    g_object_unref (buffer);
    gtk_widget_show (widget);
}

static GtkWidget *copyright_window;

void
on_plug_copyright_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkTreeView *treeview = GTK_TREE_VIEW(lookup_widget (prefwin, "pref_pluginlist"));
    GtkTreePath *path;
    GtkTreeViewColumn *col;
    gtk_tree_view_get_cursor (treeview, &path, &col);
    if (!path || !col) {
        // reset
        return;
    }
    int *indices = gtk_tree_path_get_indices (path);
    DB_plugin_t **plugins = deadbeef->plug_get_list ();
    DB_plugin_t *p = plugins[*indices];
    g_free (indices);
    assert (p);

    if (p->copyright) {
        show_copyright_window (p->copyright, "Copyright", &copyright_window);
    }
}

gboolean
on_prefwin_configure_event             (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data)
{
    wingeom_save (widget, "prefwin");
    return FALSE;
}


gboolean
on_prefwin_window_state_event          (GtkWidget       *widget,
                                        GdkEventWindowState *event,
                                        gpointer         user_data)
{
    wingeom_save_max (event, widget, "prefwin");
    return FALSE;
}


void
on_prefwin_realize                     (GtkWidget       *widget,
                                        gpointer         user_data)
{
    wingeom_restore (widget, "prefwin", -1, -1, -1, -1, 0);
}

void
on_gui_plugin_changed                  (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    gchar *txt = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (combobox));
    if (txt) {
        deadbeef->conf_set_str ("gui_plugin", txt);
        g_free (txt);
    }
}

void
on_gui_fps_value_changed           (GtkRange        *range,
                                        gpointer         user_data)
{
    int val = gtk_range_get_value (range);
    deadbeef->conf_set_int ("gtkui.refresh_rate", val);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_ignore_archives_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{

    deadbeef->conf_set_int ("ignore_archives", gtk_toggle_button_get_active (togglebutton));
}

void
on_reset_autostop_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playlist.stop_after_current_reset", gtk_toggle_button_get_active (togglebutton));
}

void
on_reset_autostopalbum_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playlist.stop_after_album_reset", gtk_toggle_button_get_active (togglebutton));
}



void
on_convert8to16_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("streamer.8_to_16", gtk_toggle_button_get_active (togglebutton));
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_convert16to24_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("streamer.16_to_24", gtk_toggle_button_get_active (togglebutton));
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_useragent_changed                   (GtkEditable     *editable,
                                        gpointer         user_data)
{
    deadbeef->conf_set_str ("network.http_user_agent", gtk_entry_get_text (GTK_ENTRY (editable)));
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_auto_size_columns_toggled           (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("gtkui.autoresize_columns", gtk_toggle_button_get_active (togglebutton));
}

void
on_display_seltime_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("gtkui.statusbar_seltime", gtk_toggle_button_get_active (togglebutton));
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_listview_group_spacing_value_changed
                                        (GtkSpinButton   *spinbutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playlist.groups.spacing", gtk_spin_button_get_value_as_int (spinbutton));
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, (uintptr_t)"playlist.groups.spacing", 0, 0);
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_modified (plt);
        deadbeef->plt_unref(plt);
    }
}
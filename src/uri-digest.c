/*
 *   Copyright (C) 2007-2016 Tristan Heaven <tristan@tristanheaven.net>
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
#include <glib.h>
#include <gio/gio.h>

#include "uri-digest.h"

struct uri_digest_s *uri_digest_new(char *uri, char *digest)
{
	struct uri_digest_s *ud = g_new(struct uri_digest_s, 1);
	ud->uri = uri;
	ud->digest = digest;

	return ud;
}

void uri_digest_free_full(struct uri_digest_s *ud)
{
	if (!ud)
		return;

	if (ud->uri) {
		g_free(ud->uri);
		ud->uri = NULL;
	}
	if (ud->digest) {
		g_free(ud->digest);
		ud->digest = NULL;
	}

	g_free(ud);
}

GSList *uri_digest_list_from_uri_list(GSList *uris)
{
	if (!uris)
		return NULL;

	GSList *ud_list = NULL;

	do {
		ud_list = g_slist_prepend(ud_list, uri_digest_new(uris->data, NULL));
	} while ((uris = g_slist_next(uris)));

	return g_slist_reverse(ud_list);
}

GSList *uri_digest_list_from_uri_strv(char **uris)
{
	if (!uris)
		return NULL;

	GSList *ud_list = NULL;

	for (int i = 0; uris[i]; i++)
		ud_list = g_slist_prepend(ud_list, uri_digest_new(uris[i], NULL));

	return g_slist_reverse(ud_list);
}

void uri_digest_list_free(GSList *ud_list)
{
	g_slist_free_full(ud_list, g_free);
}

void uri_digest_list_free_full(GSList *ud_list)
{
	g_slist_free_full(ud_list, (GDestroyNotify)uri_digest_free_full);
}

GSList *uri_digest_list_from_files_recursive(GFile *file)
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
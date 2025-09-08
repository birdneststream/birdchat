/* HexChat
 * Copyright (C) 2024 URL Handler for GTK TextBuffer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef HEXCHAT_URL_HANDLER_H
#define HEXCHAT_URL_HANDLER_H

#include <gtk/gtk.h>
#include "gtk-xtext-view.h"

G_BEGIN_DECLS

/* URL detection patterns */
typedef struct {
    const gchar *pattern;
    gint priority;
} UrlPattern;

/* URL match result */
typedef struct {
    gchar *url;
    gint start_pos;
    gint end_pos;
    gint pattern_id;
} UrlMatch;

/* URL handler initialization and cleanup */
void url_handler_init (void);
void url_handler_cleanup (void);

/* URL detection in text */
GSList *url_handler_find_urls (const gchar *text, gsize text_len);
void url_handler_free_matches (GSList *matches);

/* GTK integration */
void url_handler_setup_text_view (GtkXTextView *xtext_view);
void url_handler_apply_tags (GtkXTextView *xtext_view, 
                            GtkTextBuffer *buffer,
                            const gchar *text,
                            GtkTextIter *start_iter,
                            GtkTextIter *end_iter);

/* URL opening */
void url_handler_open_url (const gchar *url);

G_END_DECLS

#endif /* HEXCHAT_URL_HANDLER_H */
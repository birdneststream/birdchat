/* HexChat
 * Copyright (C) 2024 IRC Formatter for GTK TextBuffer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef HEXCHAT_IRC_FORMATTER_H
#define HEXCHAT_IRC_FORMATTER_H

#include <gtk/gtk.h>
#include <time.h>
#include "gtk-xtext-view.h"

G_BEGIN_DECLS

/* IRC text formatting state */
typedef struct {
    gboolean bold;
    gboolean italic;
    gboolean underline;
    gboolean strikethrough;
    gboolean reverse;
    gboolean hidden;
    int fg_color;
    int bg_color;
} IrcFormatState;

/* Formatted text segment */
typedef struct {
    gchar *text;
    gsize length;
    IrcFormatState format;
} IrcTextSegment;

/* Parser result */
typedef struct {
    GSList *segments;  /* List of IrcTextSegment */
    time_t timestamp;
} IrcFormattedText;

/* Public API */
IrcFormattedText *irc_formatter_parse (const unsigned char *text, int len, time_t stamp);
void irc_formatter_free (IrcFormattedText *formatted);

void irc_formatter_apply_to_buffer (GtkXTextView *xtext_view, 
                                   GtkTextBuffer *buffer,
                                   GtkTextIter *iter,
                                   IrcFormattedText *formatted);

/* Utility functions */
void irc_format_state_reset (IrcFormatState *state);
IrcFormatState *irc_format_state_copy (const IrcFormatState *state);

G_END_DECLS

#endif /* HEXCHAT_IRC_FORMATTER_H */
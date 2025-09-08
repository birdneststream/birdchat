/* HexChat
 * Copyright (C) 2024 IRC Formatter for GTK TextBuffer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <gtk/gtk.h>

#include "../common/hexchat.h"
#include "irc-formatter.h"
#include "url-handler.h"
#include "gtk-xtext-view.h"

/* IRC color parsing */
static int
parse_color_code (const unsigned char **text_ptr, int *fg, int *bg)
{
    const unsigned char *text = *text_ptr;
    char color_buf[4];
    int pos = 0;
    int colors_found = 0;
    
    *fg = -1;
    *bg = -1;
    
    /* Parse foreground color */
    while (pos < 2 && isdigit(*text)) {
        color_buf[pos++] = *text++;
    }
    
    if (pos > 0) {
        color_buf[pos] = '\0';
        *fg = atoi(color_buf);
        /* mIRC supports 0-99 colors */
        if (*fg > 99) *fg = 99;
        colors_found++;
    }
    
    /* Check for background color (comma separator) */
    if (*text == ',') {
        text++;
        pos = 0;
        
        /* Parse background color */
        while (pos < 2 && isdigit(*text)) {
            color_buf[pos++] = *text++;
        }
        
        if (pos > 0) {
            color_buf[pos] = '\0';
            *bg = atoi(color_buf);
            /* mIRC supports 0-99 colors */
            if (*bg > 99) *bg = 99;
            colors_found++;
        }
    }
    
    *text_ptr = text;
    return colors_found;
}

/* Reset format state to defaults */
void 
irc_format_state_reset (IrcFormatState *state)
{
    state->bold = FALSE;
    state->italic = FALSE;
    state->underline = FALSE;
    state->strikethrough = FALSE;
    state->reverse = FALSE;
    state->hidden = FALSE;
    state->fg_color = -1;  /* Default foreground */
    state->bg_color = -1;  /* Default background */
}

/* Copy format state */
IrcFormatState *
irc_format_state_copy (const IrcFormatState *state)
{
    IrcFormatState *copy = g_malloc(sizeof(IrcFormatState));
    *copy = *state;
    return copy;
}

/* Create a new text segment */
static IrcTextSegment *
irc_text_segment_new (const unsigned char *text, gsize length, const IrcFormatState *state)
{
    IrcTextSegment *segment = g_malloc(sizeof(IrcTextSegment));
    
    /* Convert to UTF-8 if needed */
    if (g_utf8_validate((const gchar *)text, length, NULL)) {
        segment->text = g_strndup((const gchar *)text, length);
    } else {
        segment->text = g_locale_to_utf8((const gchar *)text, length, NULL, NULL, NULL);
        if (!segment->text) {
            segment->text = g_strdup(""); /* fallback */
        }
    }
    
    segment->length = strlen(segment->text);
    segment->format = *state;
    
    return segment;
}

/* Free a text segment */
static void
irc_text_segment_free (IrcTextSegment *segment)
{
    if (segment) {
        g_free(segment->text);
        g_free(segment);
    }
}

/* Parse IRC formatted text into segments */
IrcFormattedText *
irc_formatter_parse (const unsigned char *text, int len, time_t stamp)
{
    if (!text || len <= 0) return NULL;
    
    IrcFormattedText *result = g_malloc0(sizeof(IrcFormattedText));
    result->timestamp = stamp;
    result->segments = NULL;
    
    IrcFormatState current_state;
    irc_format_state_reset(&current_state);
    
    const unsigned char *start = text;
    const unsigned char *current = text;
    const unsigned char *end = text + len;
    
    while (current < end) {
        unsigned char ch = *current;
        
        /* Check for IRC formatting codes */
        switch (ch) {
            case ATTR_BOLD: /* \002 */
                /* Save any text before this code */
                if (current > start) {
                    IrcTextSegment *segment = irc_text_segment_new(start, current - start, &current_state);
                    result->segments = g_slist_append(result->segments, segment);
                }
                /* Toggle bold */
                current_state.bold = !current_state.bold;
                current++;
                start = current;
                break;
                
            case ATTR_COLOR: /* \003 */
                /* Save any text before this code */
                if (current > start) {
                    IrcTextSegment *segment = irc_text_segment_new(start, current - start, &current_state);
                    result->segments = g_slist_append(result->segments, segment);
                }
                
                /* Parse color codes */
                current++; /* Skip the \003 */
                int fg, bg;
                int colors = parse_color_code(&current, &fg, &bg);
                
                if (colors == 0) {
                    /* No colors specified - reset to default */
                    current_state.fg_color = -1;
                    current_state.bg_color = -1;
                } else {
                    if (fg != -1) current_state.fg_color = fg;
                    if (bg != -1) current_state.bg_color = bg;
                }
                
                start = current;
                break;
                
            case ATTR_ITALICS: /* \035 */
                if (current > start) {
                    IrcTextSegment *segment = irc_text_segment_new(start, current - start, &current_state);
                    result->segments = g_slist_append(result->segments, segment);
                }
                current_state.italic = !current_state.italic;
                current++;
                start = current;
                break;
                
            case ATTR_UNDERLINE: /* \037 */
                if (current > start) {
                    IrcTextSegment *segment = irc_text_segment_new(start, current - start, &current_state);
                    result->segments = g_slist_append(result->segments, segment);
                }
                current_state.underline = !current_state.underline;
                current++;
                start = current;
                break;
                
            case ATTR_STRIKETHROUGH: /* \036 */
                if (current > start) {
                    IrcTextSegment *segment = irc_text_segment_new(start, current - start, &current_state);
                    result->segments = g_slist_append(result->segments, segment);
                }
                current_state.strikethrough = !current_state.strikethrough;
                current++;
                start = current;
                break;
                
            case ATTR_REVERSE: /* \026 */
                if (current > start) {
                    IrcTextSegment *segment = irc_text_segment_new(start, current - start, &current_state);
                    result->segments = g_slist_append(result->segments, segment);
                }
                current_state.reverse = !current_state.reverse;
                current++;
                start = current;
                break;
                
            case ATTR_HIDDEN: /* \010 */
                if (current > start) {
                    IrcTextSegment *segment = irc_text_segment_new(start, current - start, &current_state);
                    result->segments = g_slist_append(result->segments, segment);
                }
                current_state.hidden = !current_state.hidden;
                current++;
                start = current;
                break;
                
            case ATTR_RESET: /* \017 */
                if (current > start) {
                    IrcTextSegment *segment = irc_text_segment_new(start, current - start, &current_state);
                    result->segments = g_slist_append(result->segments, segment);
                }
                irc_format_state_reset(&current_state);
                current++;
                start = current;
                break;
                
            case ATTR_BEEP: /* \007 - skip beep */
                if (current > start) {
                    IrcTextSegment *segment = irc_text_segment_new(start, current - start, &current_state);
                    result->segments = g_slist_append(result->segments, segment);
                }
                current++;
                start = current;
                break;
                
            default:
                /* Regular character - continue */
                current++;
                break;
        }
    }
    
    /* Add any remaining text */
    if (current > start) {
        IrcTextSegment *segment = irc_text_segment_new(start, current - start, &current_state);
        result->segments = g_slist_append(result->segments, segment);
    }
    
    return result;
}

/* Free formatted text */
void
irc_formatter_free (IrcFormattedText *formatted)
{
    if (!formatted) return;
    
    GSList *iter = formatted->segments;
    while (iter) {
        irc_text_segment_free((IrcTextSegment *)iter->data);
        iter = iter->next;
    }
    g_slist_free(formatted->segments);
    g_free(formatted);
}

/* Apply formatting to GtkTextBuffer */
void
irc_formatter_apply_to_buffer (GtkXTextView *xtext_view,
                              GtkTextBuffer *buffer,
                              GtkTextIter *iter,
                              IrcFormattedText *formatted)
{
    if (!formatted || !formatted->segments) return;
    
    /* Add timestamp if enabled and timestamp exists */
    if (formatted->timestamp > 0) {
        struct tm *tm_ptr = localtime(&formatted->timestamp);
        if (tm_ptr) {
            gchar timestamp_str[32];
            strftime(timestamp_str, sizeof(timestamp_str), "[%H:%M:%S] ", tm_ptr);
            
            /* Insert timestamp in a muted color */
            GtkTextMark *start_mark = gtk_text_buffer_create_mark(buffer, NULL, iter, TRUE);
            gtk_text_buffer_insert(buffer, iter, timestamp_str, -1);
            
            /* Apply muted color to timestamp */
            GtkTextIter start_iter, end_iter;
            gtk_text_buffer_get_iter_at_mark(buffer, &start_iter, start_mark);
            end_iter = *iter;
            
            /* Use a dimmed version of the default text color for timestamps */
            if (xtext_view->color_tags[8]) { /* Gray color from mIRC palette */
                gtk_text_buffer_apply_tag(buffer, xtext_view->color_tags[8], &start_iter, &end_iter);
            }
            
            gtk_text_buffer_delete_mark(buffer, start_mark);
        }
    }
    
    GSList *segment_iter = formatted->segments;
    
    while (segment_iter) {
        IrcTextSegment *segment = (IrcTextSegment *)segment_iter->data;
        
        if (segment->length > 0 && segment->text && !segment->format.hidden) {
            /* Insert the text */
            GtkTextMark *start_mark = gtk_text_buffer_create_mark(buffer, NULL, iter, TRUE);
            gtk_text_buffer_insert(buffer, iter, segment->text, segment->length);
            
            /* Get iterators for the inserted text */
            GtkTextIter start_iter, end_iter;
            gtk_text_buffer_get_iter_at_mark(buffer, &start_iter, start_mark);
            end_iter = *iter;
            
            /* Apply URL tags first (they can overlap with other formatting) */
            url_handler_apply_tags(xtext_view, buffer, segment->text, &start_iter, &end_iter);
            
            /* Build list of tags to apply */
            GSList *tags_to_apply = NULL;
            
            /* Text formatting tags */
            if (segment->format.bold) {
                tags_to_apply = g_slist_prepend(tags_to_apply, xtext_view->bold_tag);
            }
            if (segment->format.italic) {
                tags_to_apply = g_slist_prepend(tags_to_apply, xtext_view->italic_tag);
            }
            if (segment->format.underline) {
                tags_to_apply = g_slist_prepend(tags_to_apply, xtext_view->underline_tag);
            }
            if (segment->format.strikethrough) {
                tags_to_apply = g_slist_prepend(tags_to_apply, xtext_view->strikethrough_tag);
            }
            
            /* Color tags */
            if (segment->format.reverse) {
                /* Swap fg/bg colors for reverse */
                if (segment->format.bg_color >= 0 && segment->format.bg_color < XTEXT_COLS) {
                    tags_to_apply = g_slist_prepend(tags_to_apply, 
                                                  xtext_view->color_tags[segment->format.bg_color]);
                }
                if (segment->format.fg_color >= 0 && segment->format.fg_color < XTEXT_COLS) {
                    tags_to_apply = g_slist_prepend(tags_to_apply,
                                                  xtext_view->bg_color_tags[segment->format.fg_color]);
                }
            } else {
                /* Normal color assignment */
                if (segment->format.fg_color >= 0 && segment->format.fg_color < XTEXT_COLS) {
                    tags_to_apply = g_slist_prepend(tags_to_apply, 
                                                  xtext_view->color_tags[segment->format.fg_color]);
                }
                if (segment->format.bg_color >= 0 && segment->format.bg_color < XTEXT_COLS) {
                    tags_to_apply = g_slist_prepend(tags_to_apply,
                                                  xtext_view->bg_color_tags[segment->format.bg_color]);
                }
            }
            
            /* Apply all tags - but check they're valid first */
            GSList *tag_iter = tags_to_apply;
            while (tag_iter) {
                GtkTextTag *tag = (GtkTextTag *)tag_iter->data;
                if (tag && GTK_IS_TEXT_TAG(tag)) {
                    gtk_text_buffer_apply_tag(buffer, tag, &start_iter, &end_iter);
                }
                tag_iter = tag_iter->next;
            }
            
            g_slist_free(tags_to_apply);
            gtk_text_buffer_delete_mark(buffer, start_mark);
        }
        
        segment_iter = segment_iter->next;
    }
}
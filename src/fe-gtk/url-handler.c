/* HexChat
 * Copyright (C) 2024 URL Handler for GTK TextBuffer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <glib.h>

#include "../common/hexchat.h"
#include "../common/fe.h"
#include "../common/url.h"
#include "url-handler.h"
#include "gtk-xtext-view.h"
#include "fe-gtk.h"

/* URL patterns in order of priority */
static UrlPattern url_patterns[] = {
    /* HTTP/HTTPS URLs */
    {"https?://[^\\s<>\"]+[^\\s<>\".,:;!?]", 1},
    /* FTP URLs */
    {"ftp://[^\\s<>\"]+[^\\s<>\".,:;!?]", 2},
    /* Email addresses */
    {"[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}", 3},
    /* IRC channels */
    {"#[a-zA-Z0-9_-]+", 4},
    /* File paths (basic) */
    {"file://[^\\s<>\"]+", 5},
    /* Generic protocols */
    {"[a-zA-Z][a-zA-Z0-9+.-]*://[^\\s<>\"]+[^\\s<>\".,:;!?]", 6},
    {NULL, 0}
};

static GRegex **compiled_patterns = NULL;
static gint num_patterns = 0;

/* Initialize URL handler */
void
url_handler_init (void)
{
    if (compiled_patterns) return; /* Already initialized */
    
    /* Count patterns */
    num_patterns = 0;
    while (url_patterns[num_patterns].pattern) {
        num_patterns++;
    }
    
    /* Compile regex patterns */
    compiled_patterns = g_malloc0(sizeof(GRegex*) * num_patterns);
    
    {
        gint i;
        for (i = 0; i < num_patterns; i++) {
        GError *error = NULL;
        compiled_patterns[i] = g_regex_new(url_patterns[i].pattern,
                                          G_REGEX_CASELESS | G_REGEX_OPTIMIZE,
                                          0, &error);
        if (error) {
            g_warning("Failed to compile URL pattern %s: %s", 
                     url_patterns[i].pattern, error->message);
            g_error_free(error);
            compiled_patterns[i] = NULL;
        }
        }
    }
}

/* Cleanup URL handler */
void
url_handler_cleanup (void)
{
    if (!compiled_patterns) return;
    
    {
        gint i;
        for (i = 0; i < num_patterns; i++) {
        if (compiled_patterns[i]) {
            g_regex_unref(compiled_patterns[i]);
        }
        }
    }
    g_free(compiled_patterns);
    compiled_patterns = NULL;
    num_patterns = 0;
}

/* Free URL match */
static void
url_match_free (UrlMatch *match)
{
    if (match) {
        g_free(match->url);
        g_free(match);
    }
}

/* Comparison function for sorting matches */
static gint
url_match_compare (const UrlMatch *a, const UrlMatch *b)
{
    return a->start_pos - b->start_pos;
}

/* Find URLs in text */
GSList *
url_handler_find_urls (const gchar *text, gsize text_len)
{
    GSList *matches = NULL;
    gint pattern_idx;
    
    if (!text || !compiled_patterns) return NULL;
    
    for (pattern_idx = 0; pattern_idx < num_patterns; pattern_idx++) {
        GMatchInfo *match_info;
        
        if (!compiled_patterns[pattern_idx]) continue;
        
        if (g_regex_match_full(compiled_patterns[pattern_idx], text, text_len,
                              0, 0, &match_info, NULL)) {
            
            while (g_match_info_matches(match_info)) {
                gchar *match_text = g_match_info_fetch(match_info, 0);
                gint start_pos, end_pos;
                
                if (g_match_info_fetch_pos(match_info, 0, &start_pos, &end_pos)) {
                    /* Check if this URL overlaps with existing matches */
                    gboolean overlaps = FALSE;
                    GSList *iter = matches;
                    while (iter) {
                        UrlMatch *existing = (UrlMatch *)iter->data;
                        if ((start_pos < existing->end_pos && end_pos > existing->start_pos)) {
                            /* Overlap detected - keep higher priority (lower number) */
                            if (url_patterns[pattern_idx].priority < url_patterns[existing->pattern_id].priority) {
                                /* Remove existing, add new */
                                matches = g_slist_remove(matches, existing);
                                url_match_free(existing);
                            } else {
                                /* Keep existing */
                                overlaps = TRUE;
                            }
                            break;
                        }
                        iter = iter->next;
                    }
                    
                    if (!overlaps) {
                        UrlMatch *new_match = g_malloc0(sizeof(UrlMatch));
                        new_match->url = match_text;
                        new_match->start_pos = start_pos;
                        new_match->end_pos = end_pos;
                        new_match->pattern_id = pattern_idx;
                        matches = g_slist_append(matches, new_match);
                    } else {
                        g_free(match_text);
                    }
                } else {
                    g_free(match_text);
                }
                
                g_match_info_next(match_info, NULL);
            }
        }
        g_match_info_free(match_info);
    }
    
    /* Sort matches by position */
    matches = g_slist_sort(matches, (GCompareFunc)url_match_compare);
    
    return matches;
}

/* Free URL matches */
void
url_handler_free_matches (GSList *matches)
{
    GSList *iter = matches;
    while (iter) {
        url_match_free((UrlMatch *)iter->data);
        iter = iter->next;
    }
    g_slist_free(matches);
}

/* Handle URL click events */
static gboolean
url_handler_button_press (GtkWidget *text_view, GdkEventButton *event, gpointer user_data)
{
    if (event->type == GDK_BUTTON_PRESS && event->button == 1) {
        GtkTextView *view = GTK_TEXT_VIEW(text_view);
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(view);
        GtkTextIter iter;
        
        /* Get iterator at click position */
        gint x, y;
        gtk_text_view_window_to_buffer_coords(view, GTK_TEXT_WINDOW_WIDGET,
                                            event->x, event->y, &x, &y);
        gtk_text_view_get_iter_at_location(view, &iter, x, y);
        
        /* Check if click is on URL tag */
        GSList *tags = gtk_text_iter_get_tags(&iter);
        GSList *tag_iter = tags;
        
        while (tag_iter) {
            GtkTextTag *tag = (GtkTextTag *)tag_iter->data;
            gchar *tag_name = NULL;
            g_object_get(tag, "name", &tag_name, NULL);
            
            if (tag_name && strcmp(tag_name, "url") == 0) {
                /* Found URL tag - get the URL text */
                GtkTextIter start = iter, end = iter;
                
                if (!gtk_text_iter_starts_tag(&start, tag))
                    gtk_text_iter_backward_to_tag_toggle(&start, tag);
                if (!gtk_text_iter_ends_tag(&end, tag))
                    gtk_text_iter_forward_to_tag_toggle(&end, tag);
                
                gchar *url_text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
                
                if (url_text) {
                    url_handler_open_url(url_text);
                    g_free(url_text);
                }
                
                g_free(tag_name);
                g_slist_free(tags);
                return TRUE; /* Event handled */
            }
            
            g_free(tag_name);
            tag_iter = tag_iter->next;
        }
        
        g_slist_free(tags);
    }
    
    return FALSE; /* Event not handled */
}

/* Handle cursor changes over URLs */
static gboolean
url_handler_motion_notify (GtkWidget *text_view, GdkEventMotion *event, gpointer user_data)
{
    GtkTextView *view = GTK_TEXT_VIEW(text_view);
    GtkTextIter iter;
    GtkXTextView *xtext_view = GTK_XTEXT_VIEW(user_data);
    
    /* Get iterator at cursor position */
    gint x, y;
    gtk_text_view_window_to_buffer_coords(view, GTK_TEXT_WINDOW_WIDGET,
                                        event->x, event->y, &x, &y);
    gtk_text_view_get_iter_at_location(view, &iter, x, y);
    
    /* Check if cursor is over URL tag */
    GSList *tags = gtk_text_iter_get_tags(&iter);
    GSList *tag_iter = tags;
    gboolean over_url = FALSE;
    
    while (tag_iter) {
        GtkTextTag *tag = (GtkTextTag *)tag_iter->data;
        gchar *tag_name = NULL;
        g_object_get(tag, "name", &tag_name, NULL);
        
        if (tag_name && strcmp(tag_name, "url") == 0) {
            over_url = TRUE;
            g_free(tag_name);
            break;
        }
        
        g_free(tag_name);
        tag_iter = tag_iter->next;
    }
    
    g_slist_free(tags);
    
    /* Set cursor */
    GdkWindow *window = gtk_text_view_get_window(view, GTK_TEXT_WINDOW_TEXT);
    if (over_url) {
        gdk_window_set_cursor(window, xtext_view->hand_cursor);
    } else {
        gdk_window_set_cursor(window, NULL);
    }
    
    return FALSE;
}

/* Setup URL handling for text view */
void
url_handler_setup_text_view (GtkXTextView *xtext_view)
{
    if (!xtext_view || !xtext_view->text_view) return;
    
    /* Initialize URL handler if needed */
    url_handler_init();
    
    /* Connect event handlers */
    g_signal_connect(xtext_view->text_view, "button-press-event",
                    G_CALLBACK(url_handler_button_press), xtext_view);
    g_signal_connect(xtext_view->text_view, "motion-notify-event",
                    G_CALLBACK(url_handler_motion_notify), xtext_view);
                    
    /* Enable motion events */
    gtk_widget_add_events(GTK_WIDGET(xtext_view->text_view), 
                         GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK);
}

/* Apply URL tags to text buffer */
void
url_handler_apply_tags (GtkXTextView *xtext_view,
                       GtkTextBuffer *buffer,
                       const gchar *text,
                       GtkTextIter *start_iter,
                       GtkTextIter *end_iter)
{
    if (!text || !xtext_view || !buffer) return;
    
    GSList *matches = url_handler_find_urls(text, strlen(text));
    if (!matches) return;
    
    GSList *match_iter = matches;
    while (match_iter) {
        UrlMatch *match = (UrlMatch *)match_iter->data;
        
        /* Calculate iterators for this URL */
        GtkTextIter url_start = *start_iter;
        GtkTextIter url_end = *start_iter;
        
        gtk_text_iter_forward_chars(&url_start, match->start_pos);
        gtk_text_iter_forward_chars(&url_end, match->end_pos);
        
        /* Apply URL tag */
        gtk_text_buffer_apply_tag(buffer, xtext_view->url_tag, &url_start, &url_end);
        
        match_iter = match_iter->next;
    }
    
    url_handler_free_matches(matches);
}

/* Open URL using system default handler */
void
url_handler_open_url (const gchar *url)
{
    if (!url) return;
    
    GError *error = NULL;
    
    /* Use gtk_show_uri_on_window for GTK3 */
    #if GTK_CHECK_VERSION(3, 22, 0)
        gtk_show_uri_on_window(NULL, url, GDK_CURRENT_TIME, &error);
    #else
        gtk_show_uri(NULL, url, GDK_CURRENT_TIME, &error);
    #endif
    
    if (error) {
        g_warning("Failed to open URL %s: %s", url, error->message);
        g_error_free(error);
        
        /* Fallback: try using hexchat's URL handling */
        fe_open_url(url);
    }
}
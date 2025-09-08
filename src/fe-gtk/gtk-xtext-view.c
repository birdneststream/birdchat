/* HexChat
 * Copyright (C) 1998-2010 Peter Zelezny.
 * Copyright (C) 2009-2013 Berke Viktor.
 * Copyright (C) 2024 Migration to GtkTextView
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <gtk/gtk.h>

#include "../common/hexchat.h"
#include "../common/fe.h"
#include "../common/util.h"
#include "gtk-xtext-view.h"
#include "irc-formatter.h"
#include "url-handler.h"
#include "fe-gtk.h"

/* Signals */
enum {
	WORD_CLICK,
	SET_SCROLL_ADJUSTMENTS,
	LAST_SIGNAL
};

static guint xtext_signals[LAST_SIGNAL];

/* Private functions */
static void gtk_xtext_view_class_init (GtkXTextViewClass *klass);
static void gtk_xtext_view_init (GtkXTextView *xtext);
static void gtk_xtext_view_finalize (GObject *object);
static void gtk_xtext_view_create_tags (GtkXTextView *xtext);
static void gtk_xtext_view_setup_text_view (GtkXTextView *xtext);

/* Buffer functions */
static XTextBuffer *xtext_buffer_new_internal (GtkXTextView *xtext);
static void xtext_buffer_free_internal (XTextBuffer *buf);

G_DEFINE_TYPE (GtkXTextView, gtk_xtext_view, GTK_TYPE_SCROLLED_WINDOW)

static void
gtk_xtext_view_class_init (GtkXTextViewClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gtk_xtext_view_finalize;

	/* Signals for compatibility */
	xtext_signals[WORD_CLICK] = g_signal_new (
		"word_click",
		G_TYPE_FROM_CLASS (klass),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkXTextViewClass, word_click),
		NULL, NULL,
		g_cclosure_marshal_VOID__POINTER,
		G_TYPE_NONE, 2,
		G_TYPE_STRING,
		GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

	xtext_signals[SET_SCROLL_ADJUSTMENTS] = g_signal_new (
		"set_scroll_adjustments",
		G_TYPE_FROM_CLASS (klass),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkXTextViewClass, set_scroll_adjustments),
		NULL, NULL,
		g_cclosure_marshal_VOID__OBJECT,
		G_TYPE_NONE, 2,
		GTK_TYPE_ADJUSTMENT,
		GTK_TYPE_ADJUSTMENT);
}

static void
gtk_xtext_view_init (GtkXTextView *xtext)
{
	/* Initialize defaults */
	xtext->max_lines = 1000;
	xtext->auto_indent = TRUE;
	xtext->separator = FALSE;
	xtext->marker = FALSE;
	xtext->wordwrap = TRUE;
	xtext->ignore_hidden = FALSE;
	xtext->max_auto_indent = 256;
	xtext->urlcheck_function = NULL;

	/* Create cursors - use display aware version for GTK3 */
	GdkDisplay *display = gdk_display_get_default ();
	xtext->hand_cursor = gdk_cursor_new_for_display (display, GDK_HAND2);
	xtext->resize_cursor = gdk_cursor_new_for_display (display, GDK_SB_H_DOUBLE_ARROW);

	/* Initialize palette to defaults (will be set by gtk_xtext_set_palette) */
	memset (xtext->palette, 0, sizeof (xtext->palette));

	/* Setup the scrolled window */
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (xtext),
	                                GTK_POLICY_NEVER,
	                                GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (xtext),
	                                     GTK_SHADOW_IN);

	/* Create text view and buffer */
	gtk_xtext_view_setup_text_view (xtext);
	
	/* Create text formatting tags */
	gtk_xtext_view_create_tags (xtext);

	/* Setup URL handling */
	url_handler_setup_text_view (xtext);

	/* Create default buffer */
	xtext->buffer = xtext_buffer_new_internal (xtext);
	xtext->orig_buffer = xtext->buffer;
	xtext->selection_buffer = NULL;
}

static void
gtk_xtext_view_setup_text_view (GtkXTextView *xtext)
{
	/* Create text view and buffer */
	xtext->text_view = GTK_TEXT_VIEW (gtk_text_view_new ());
	xtext->text_buffer = gtk_text_view_get_buffer (xtext->text_view);
	
	/* Store the tag table for creating additional buffers */
	xtext->tag_table = gtk_text_buffer_get_tag_table (xtext->text_buffer);

	/* Configure text view */
	gtk_text_view_set_editable (xtext->text_view, FALSE);
	gtk_text_view_set_cursor_visible (xtext->text_view, FALSE);
	gtk_text_view_set_wrap_mode (xtext->text_view, GTK_WRAP_WORD_CHAR);
	gtk_text_view_set_left_margin (xtext->text_view, 3);
	gtk_text_view_set_right_margin (xtext->text_view, 3);

	/* Add to scrolled window */
	gtk_container_add (GTK_CONTAINER (xtext), GTK_WIDGET (xtext->text_view));

	/* Get scroll adjustment for compatibility */
	xtext->adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (xtext));

	/* Show the text view */
	gtk_widget_show (GTK_WIDGET (xtext->text_view));
}

/* Constructor - Create a new XText widget */
GtkWidget *
gtk_xtext_view_new (xtext_buffer *buffer)
{
	GtkXTextView *xtext = g_object_new (GTK_TYPE_XTEXT, NULL);
	
	/* If a buffer was provided, use it instead of the default */
	if (buffer) {
		xtext->buffer = buffer;
		xtext->orig_buffer = buffer;
		buffer->xtext_view = xtext;
		buffer->xtext = (GtkXText *)xtext; /* For compatibility */
	}
	
	return GTK_WIDGET (xtext);
}

static void
gtk_xtext_view_create_tags (GtkXTextView *xtext)
{
	GtkTextBuffer *buffer = xtext->text_buffer;
	int i;

	/* Create formatting tags */
	xtext->bold_tag = gtk_text_buffer_create_tag (buffer, "bold",
	                                              "weight", PANGO_WEIGHT_BOLD,
	                                              NULL);

	xtext->italic_tag = gtk_text_buffer_create_tag (buffer, "italic",
	                                                "style", PANGO_STYLE_ITALIC,
	                                                NULL);

	xtext->underline_tag = gtk_text_buffer_create_tag (buffer, "underline",
	                                                   "underline", PANGO_UNDERLINE_SINGLE,
	                                                   NULL);

	xtext->strikethrough_tag = gtk_text_buffer_create_tag (buffer, "strikethrough",
	                                                       "strikethrough", TRUE,
	                                                       NULL);

	xtext->url_tag = gtk_text_buffer_create_tag (buffer, "url",
	                                             "underline", PANGO_UNDERLINE_SINGLE,
	                                             "foreground", "blue",
	                                             NULL);

	xtext->search_highlight_tag = gtk_text_buffer_create_tag (buffer, "search_highlight",
	                                                          "background", "yellow",
	                                                          "foreground", "black",
	                                                          NULL);

	/* Create color tags - will be populated by gtk_xtext_set_palette */
	for (i = 0; i < XTEXT_COLS; i++) {
		char tag_name[32];
		
		/* Foreground color tags */
		snprintf (tag_name, sizeof (tag_name), "fg_color_%d", i);
		xtext->color_tags[i] = gtk_text_buffer_create_tag (buffer, tag_name, NULL);
		
		/* Background color tags */
		snprintf (tag_name, sizeof (tag_name), "bg_color_%d", i);
		xtext->bg_color_tags[i] = gtk_text_buffer_create_tag (buffer, tag_name, NULL);
	}
}

static void
gtk_xtext_view_finalize (GObject *object)
{
	GtkXTextView *xtext = GTK_XTEXT_VIEW (object);

	/* Free buffers */
	if (xtext->buffer) {
		xtext_buffer_free_internal (xtext->buffer);
	}
	if (xtext->selection_buffer && xtext->selection_buffer != xtext->buffer) {
		xtext_buffer_free_internal (xtext->selection_buffer);
	}

	/* Free font */
	if (xtext->font) {
		pango_font_description_free (xtext->font);
	}

	/* Free pixmap */
	if (xtext->background_pixmap) {
		g_object_unref (xtext->background_pixmap);
	}

	/* Free cursors */
	if (xtext->hand_cursor) {
		g_object_unref (xtext->hand_cursor);
	}
	if (xtext->resize_cursor) {
		g_object_unref (xtext->resize_cursor);
	}

	G_OBJECT_CLASS (gtk_xtext_view_parent_class)->finalize (object);
}

/* Buffer management */
static XTextBuffer *
xtext_buffer_new_internal (GtkXTextView *xtext)
{
	XTextBuffer *buf = g_malloc0 (sizeof (XTextBuffer));
	
	buf->xtext_view = xtext;
	/* Each buffer gets its own text buffer */
	buf->text_buffer = gtk_text_buffer_new(xtext->tag_table);
	buf->xtext = (GtkXText *)xtext;  /* Compatibility pointer */
	buf->max_lines = 1000;
	buf->num_lines = 0;
	buf->indent = 0;
	buf->marker_state = MARKER_WAS_NEVER_SET;
	buf->marker_seen = FALSE;
	buf->time_stamp = TRUE;
	buf->needs_recalc = FALSE;
	buf->search_text = NULL;
	buf->search_nee = NULL;
	buf->search_lnee = 0;

	return buf;
}

static void
xtext_buffer_free_internal (XTextBuffer *buf)
{
	if (!buf) return;

	/* Free search data */
	g_free (buf->search_text);
	g_free (buf->search_nee);
	if (buf->search_re) {
		g_regex_unref (buf->search_re);
	}
	if (buf->search_found) {
		g_list_free (buf->search_found);
	}
	
	/* Clean up the text buffer */
	if (buf->text_buffer) {
		g_object_unref(buf->text_buffer);
	}
	
	buf->text_buffer = NULL;

	g_free (buf);
}

/* Public API implementation */
GType
gtk_xtext_get_type (void)
{
	return gtk_xtext_view_get_type ();
}

GtkWidget *
gtk_xtext_new (GdkRGBA palette[], int separator)
{
	GtkXTextView *xtext = g_object_new (GTK_TYPE_XTEXT, NULL);

	if (palette) {
		gtk_xtext_set_palette (GTK_XTEXT (xtext), palette);
	}

	gtk_xtext_set_show_separator (GTK_XTEXT (xtext), separator);

	return GTK_WIDGET (xtext);
}

void
gtk_xtext_append (xtext_buffer *buf, unsigned char *text, int len, time_t stamp)
{
	GtkTextBuffer *text_buffer;
	GtkTextIter iter;
	IrcFormattedText *formatted;
	
	if (!buf || !text || len <= 0) return;

	text_buffer = buf->text_buffer;

	/* Parse IRC formatting */
	formatted = irc_formatter_parse (text, len, stamp);
	if (!formatted) return;

	/* Get end iterator */
	gtk_text_buffer_get_end_iter (text_buffer, &iter);
	
	/* Apply formatted text to buffer */
	irc_formatter_apply_to_buffer (buf->xtext_view, text_buffer, &iter, formatted);
	
	/* Add newline */
	gtk_text_buffer_insert (text_buffer, &iter, "\n", 1);

	buf->num_lines++;

	/* Handle line limits - batch delete for better performance */
	if (buf->max_lines > 0 && buf->num_lines > buf->max_lines) {
		GtkTextIter start_iter, end_iter;
		int lines_to_delete = buf->num_lines - buf->max_lines;
		
		/* Delete multiple lines at once for better performance */
		gtk_text_buffer_get_start_iter (text_buffer, &start_iter);
		gtk_text_buffer_get_iter_at_line (text_buffer, &end_iter, lines_to_delete);
		gtk_text_buffer_delete (text_buffer, &start_iter, &end_iter);
		
		buf->num_lines = buf->max_lines;
	}

	/* Always force scroll to bottom */
	GtkTextMark *end_mark = gtk_text_buffer_get_insert(text_buffer);
	gtk_text_buffer_get_end_iter (text_buffer, &iter);
	gtk_text_buffer_move_mark(text_buffer, end_mark, &iter);
	gtk_text_view_scroll_mark_onscreen(buf->xtext_view->text_view, end_mark);

	/* Cleanup */
	irc_formatter_free (formatted);
}

void
gtk_xtext_append_indent (xtext_buffer *buf,
                         unsigned char *left_text, int left_len,
                         unsigned char *right_text, int right_len,
                         time_t stamp)
{
	GtkTextBuffer *text_buffer;
	GtkTextIter iter;
	IrcFormattedText *left_formatted = NULL, *right_formatted = NULL;
	
	if (!buf) return;
	if (!left_text && !right_text) return;

	text_buffer = buf->text_buffer;
	gtk_text_buffer_get_end_iter (text_buffer, &iter);

	/* Handle left text (nick/timestamp part) */
	if (left_text && left_len > 0) {
		left_formatted = irc_formatter_parse (left_text, left_len, stamp);
		if (left_formatted) {
			irc_formatter_apply_to_buffer (buf->xtext_view, text_buffer, &iter, left_formatted);
			
			/* Add separator */
			if (right_text && right_len > 0) {
				gtk_text_buffer_insert (text_buffer, &iter, " ", 1);
			}
		}
	}

	/* Handle right text (message part) with proper indentation */
	if (right_text && right_len > 0) {
		right_formatted = irc_formatter_parse (right_text, right_len, 0);
		if (right_formatted) {
			irc_formatter_apply_to_buffer (buf->xtext_view, text_buffer, &iter, right_formatted);
		}
	}

	/* Add newline */
	gtk_text_buffer_insert (text_buffer, &iter, "\n", 1);
	buf->num_lines++;

	/* Handle line limits */
	if (buf->max_lines > 0 && buf->num_lines > buf->max_lines) {
		GtkTextIter start_iter, end_iter;
		int lines_to_delete = buf->num_lines - buf->max_lines;
		
		gtk_text_buffer_get_start_iter (text_buffer, &start_iter);
		gtk_text_buffer_get_iter_at_line (text_buffer, &end_iter, lines_to_delete);
		gtk_text_buffer_delete (text_buffer, &start_iter, &end_iter);
		
		buf->num_lines = buf->max_lines;
	}

	/* Always force scroll to bottom */
	GtkTextMark *end_mark = gtk_text_buffer_get_insert(text_buffer);
	gtk_text_buffer_get_end_iter (text_buffer, &iter);
	gtk_text_buffer_move_mark(text_buffer, end_mark, &iter);
	gtk_text_view_scroll_mark_onscreen(buf->xtext_view->text_view, end_mark);

	/* Cleanup */
	if (left_formatted) irc_formatter_free (left_formatted);
	if (right_formatted) irc_formatter_free (right_formatted);
}

void
gtk_xtext_clear (xtext_buffer *buf, int lines)
{
	if (!buf) return;

	if (lines == 0) {
		/* Clear all */
		gtk_text_buffer_set_text (buf->text_buffer, "", 0);
		buf->num_lines = 0;
	} else {
		/* Clear specific number of lines from end */
		GtkTextIter start_iter, end_iter;
		int total_lines = gtk_text_buffer_get_line_count (buf->text_buffer);
		int start_line = MAX (0, total_lines - lines);
		
		gtk_text_buffer_get_iter_at_line (buf->text_buffer, &start_iter, start_line);
		gtk_text_buffer_get_end_iter (buf->text_buffer, &end_iter);
		gtk_text_buffer_delete (buf->text_buffer, &start_iter, &end_iter);
		
		buf->num_lines = start_line;
	}
}

void
gtk_xtext_refresh (GtkXText *xtext)
{
	/* GtkTextView handles refreshing automatically */
	if (xtext && xtext->text_view) {
		gtk_widget_queue_draw (GTK_WIDGET (xtext->text_view));
	}
}

int
gtk_xtext_set_font (GtkXText *xtext, char *name)
{
	PangoFontDescription *font_desc;
	
	if (!xtext || !name) return 0;

	font_desc = pango_font_description_from_string (name);
	if (!font_desc) return 0;

	/* Free old font */
	if (xtext->font) {
		pango_font_description_free (xtext->font);
	}
	
	xtext->font = font_desc;

	/* Apply to text view */
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	gtk_widget_override_font (GTK_WIDGET (xtext->text_view), font_desc);
	#pragma GCC diagnostic pop

	return 1;
}

void
gtk_xtext_set_background (GtkXText *xtext, GdkPixbuf *pixmap)
{
	if (!xtext) return;

	/* Store reference to pixmap */
	if (xtext->background_pixmap) {
		g_object_unref (xtext->background_pixmap);
	}
	
	if (pixmap) {
		xtext->background_pixmap = g_object_ref (pixmap);
	} else {
		xtext->background_pixmap = NULL;
	}
}

void
gtk_xtext_set_palette (GtkXText *xtext, GdkRGBA palette[])
{
	int i;
	
	if (!xtext || !palette) return;

	/* Store palette */
	memcpy (xtext->palette, palette, sizeof (GdkRGBA) * XTEXT_COLS);

	/* Update color tags */
	for (i = 0; i < XTEXT_COLS; i++) {
		g_object_set (xtext->color_tags[i], 
		              "foreground-rgba", &palette[i],
		              NULL);
		g_object_set (xtext->bg_color_tags[i],
		              "background-rgba", &palette[i], 
		              NULL);
	}
}

/* Buffer management API */
xtext_buffer *
gtk_xtext_buffer_new (GtkXText *xtext)
{
	if (!xtext) return NULL;
	return xtext_buffer_new_internal (xtext);
}

void
gtk_xtext_buffer_free (xtext_buffer *buf)
{
	xtext_buffer_free_internal (buf);
}

void
gtk_xtext_buffer_show (GtkXText *xtext, xtext_buffer *buf, int render)
{
	if (!xtext || !buf) return;
	
	/* Switch active buffer */
	xtext->buffer = buf;
	buf->xtext_view = xtext;
	
	/* Switch the text view to use this buffer's text buffer */
	gtk_text_view_set_buffer(xtext->text_view, buf->text_buffer);
	
	/* Update the xtext->text_buffer reference to the new buffer */
	xtext->text_buffer = buf->text_buffer;
	
	/* Always scroll to bottom when switching buffers */
	GtkTextIter iter;
	GtkTextMark *end_mark = gtk_text_buffer_get_insert(buf->text_buffer);
	gtk_text_buffer_get_end_iter(buf->text_buffer, &iter);
	gtk_text_buffer_move_mark(buf->text_buffer, end_mark, &iter);
	gtk_text_view_scroll_mark_onscreen(xtext->text_view, end_mark);
	
	if (render) {
		gtk_xtext_refresh (xtext);
	}
}

gboolean
gtk_xtext_is_empty (xtext_buffer *buf)
{
	if (!buf) return TRUE;
	return (buf->num_lines == 0);
}

/* Settings API */
void gtk_xtext_set_indent (GtkXText *xtext, gboolean indent) {
	if (xtext) xtext->auto_indent = indent;
}

void gtk_xtext_set_max_indent (GtkXText *xtext, int max_auto_indent) {
	if (xtext) xtext->max_auto_indent = max_auto_indent;
}

void gtk_xtext_set_max_lines (GtkXText *xtext, int max_lines) {
	if (xtext) {
		xtext->max_lines = max_lines;
		if (xtext->buffer) xtext->buffer->max_lines = max_lines;
	}
}

void gtk_xtext_set_show_marker (GtkXText *xtext, gboolean show_marker) {
	if (xtext) xtext->marker = show_marker;
}

void gtk_xtext_set_show_separator (GtkXText *xtext, gboolean show_separator) {
	if (xtext) xtext->separator = show_separator;
}

void gtk_xtext_set_thin_separator (GtkXText *xtext, gboolean thin_separator) {
	/* Placeholder */
}

void gtk_xtext_set_time_stamp (xtext_buffer *buf, gboolean timestamp) {
	if (buf) buf->time_stamp = timestamp;
}

void gtk_xtext_set_urlcheck_function (GtkXText *xtext, int (*urlcheck_function) (GtkWidget *, char *)) {
	if (xtext) xtext->urlcheck_function = urlcheck_function;
}

void gtk_xtext_set_wordwrap (GtkXText *xtext, gboolean word_wrap) {
	if (xtext) {
		xtext->wordwrap = word_wrap;
		gtk_text_view_set_wrap_mode (xtext->text_view, 
		                             word_wrap ? GTK_WRAP_WORD_CHAR : GTK_WRAP_NONE);
	}
}

void gtk_xtext_set_error_function (GtkXText *xtext, void (*error_function) (int)) {
	/* Placeholder */
}

/* Search functionality - stub */
textentry *gtk_xtext_search (GtkXText *xtext, const gchar *text, gtk_xtext_search_flags flags, GError **err) {
	if (err) *err = g_error_new_literal(0, 0, "Search not implemented");
	return NULL;
}

void gtk_xtext_reset_marker_pos (GtkXText *xtext) {
	/* Stub */
}

int gtk_xtext_moveto_marker_pos (GtkXText *xtext) {
	return 0;
}

void gtk_xtext_check_marker_visibility (GtkXText *xtext) {
	/* Stub */
}

void gtk_xtext_save (GtkXText *xtext, int fh) {
	GtkTextIter start, end;
	gchar *text;
	
	if (!xtext || !xtext->buffer) return;
	
	gtk_text_buffer_get_bounds(xtext->buffer->text_buffer, &start, &end);
	text = gtk_text_buffer_get_text(xtext->buffer->text_buffer, &start, &end, FALSE);
	
	if (text) {
		write(fh, text, strlen(text));
		g_free(text);
	}
}

void gtk_xtext_copy_selection (GtkXText *xtext) {
	if (!xtext) return;
	
	GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	gtk_text_buffer_copy_clipboard(xtext->buffer->text_buffer, clipboard);
}

int gtk_xtext_lastlog (xtext_buffer *out, xtext_buffer *search_area) {
	return 0;
}

void gtk_xtext_foreach (xtext_buffer *buf, GtkXTextForeach func, void *data) {
	GtkTextIter start, end;
	gchar *text;
	
	if (!buf || !func) return;
	
	gtk_text_buffer_get_bounds(buf->text_buffer, &start, &end);
	text = gtk_text_buffer_get_text(buf->text_buffer, &start, &end, FALSE);
	
	if (text) {
		func((GtkXText*)buf->xtext_view, (unsigned char*)text, data);
		g_free(text);
	}
}

void gtk_xtext_set_marker_last (struct session *sess) {
	/* Stub */
}
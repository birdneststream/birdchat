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

#ifndef HEXCHAT_GTK_XTEXT_VIEW_H
#define HEXCHAT_GTK_XTEXT_VIEW_H

#include <gtk/gtk.h>
#include <time.h>

G_BEGIN_DECLS

/* Keep original xtext.h constants for compatibility */
#define GTK_TYPE_XTEXT              (gtk_xtext_view_get_type ())
#define GTK_XTEXT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GTK_TYPE_XTEXT, GtkXTextView))
#define GTK_XTEXT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_XTEXT, GtkXTextViewClass))
#define GTK_IS_XTEXT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GTK_TYPE_XTEXT))
#define GTK_IS_XTEXT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_XTEXT))
#define GTK_XTEXT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_XTEXT, GtkXTextViewClass))

#define GTK_XTEXT_VIEW(object)      (G_TYPE_CHECK_INSTANCE_CAST ((object), GTK_TYPE_XTEXT, GtkXTextView))
#define GTK_XTEXT_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_XTEXT, GtkXTextViewClass))

/* IRC formatting constants */
#define ATTR_BOLD				'\002'
#define ATTR_COLOR			'\003'
#define ATTR_BLINK			'\006'
#define ATTR_BEEP				'\007'
#define ATTR_HIDDEN			'\010'
#define ATTR_ITALICS2		'\011'
#define ATTR_RESET			'\017'
#define ATTR_REVERSE			'\026'
#define ATTR_ITALICS			'\035'
#define ATTR_STRIKETHROUGH	'\036'
#define ATTR_UNDERLINE		'\037'

/* Color palette constants */
#define XTEXT_MIRC_COLS 99
#define XTEXT_COLS (XTEXT_MIRC_COLS + 5)
#define XTEXT_MARK_FG (XTEXT_MIRC_COLS)
#define XTEXT_MARK_BG (XTEXT_MIRC_COLS + 1)
#define XTEXT_FG (XTEXT_MIRC_COLS + 2)
#define XTEXT_BG (XTEXT_MIRC_COLS + 3)
#define XTEXT_MARKER (XTEXT_MIRC_COLS + 4)
#define XTEXT_MAX_COLOR (XTEXT_MIRC_COLS + 9)

/* Forward declarations for compatibility */
typedef struct _GtkXTextView GtkXTextView;
typedef struct _GtkXTextViewClass GtkXTextViewClass;
typedef struct _XTextBuffer XTextBuffer;

/* Maintain old names for compatibility */
typedef GtkXTextView GtkXText;
typedef GtkXTextViewClass GtkXTextClass;
typedef XTextBuffer xtext_buffer;

/* Search flags are defined in common/hexchat.h */

/* Marker reset reasons */
typedef enum marker_reset_reason_e {
	MARKER_WAS_NEVER_SET,
	MARKER_IS_SET,
	MARKER_RESET_MANUALLY,
	MARKER_RESET_BY_KILL,
	MARKER_RESET_BY_CLEAR
} marker_reset_reason;

/* Text offsets for search */
typedef union offsets_u {
	struct offsets_s {
		guint16	start;
		guint16	end;
	} o;
	guint32 u;
} offsets_t;

/* Legacy textentry for compatibility (will be phased out) */
typedef struct textentry {
	struct textentry *next;
	struct textentry *prev;
	unsigned char *str;
	time_t stamp;
	gint16 str_width;
	gint16 str_len;
	gint16 mark_start;
	gint16 mark_end;
	gint16 indent;
	gint16 left_len;
	guchar tag;
	GList *marks;
} textentry;

/* Buffer structure that wraps GtkTextBuffer */
struct _XTextBuffer {
	GtkXTextView *xtext_view;
	GtkTextBuffer *text_buffer;
	
	/* Legacy compatibility - points to the same widget */
	GtkXText *xtext;
	
	/* Buffer state */
	int max_lines;
	int num_lines;
	int indent;
	
	/* Marker support */
	GtkTextMark *marker_pos;
	marker_reset_reason marker_state;
	gboolean marker_seen;
	
	/* Search support */
	GList *search_found;
	gchar *search_text;
	gchar *search_nee;		/* prepared needle to look in haystack for */
	gint search_lnee;		/* its length */
	gtk_xtext_search_flags search_flags;
	GRegex *search_re;
	
	/* Settings */
	gboolean time_stamp;
	gboolean needs_recalc;
};

/* Main widget structure */
struct _GtkXTextView {
	GtkScrolledWindow parent;
	
	/* Core components */
	GtkTextView *text_view;
	GtkTextBuffer *text_buffer;
	GtkTextTagTable *tag_table;
	
	/* Scroll adjustments for compatibility */
	GtkAdjustment *adj;
	
	/* Buffers */
	XTextBuffer *buffer;
	XTextBuffer *orig_buffer;
	XTextBuffer *selection_buffer;
	
	/* Text formatting tags */
	GtkTextTag *bold_tag;
	GtkTextTag *italic_tag;
	GtkTextTag *underline_tag;
	GtkTextTag *strikethrough_tag;
	GtkTextTag *url_tag;
	GtkTextTag *search_highlight_tag;
	GtkTextTag *color_tags[XTEXT_COLS];
	GtkTextTag *bg_color_tags[XTEXT_COLS];
	
	/* Color palette */
	GdkRGBA palette[XTEXT_COLS];
	
	/* Font and styling */
	PangoFontDescription *font;
	GdkPixbuf *background_pixmap;
	
	/* Settings */
	int max_lines;
	gboolean auto_indent;
	gboolean separator;
	gboolean marker;
	gboolean wordwrap;
	gboolean ignore_hidden;
	int max_auto_indent;
	
	/* URL handling */
	int (*urlcheck_function) (GtkWidget *xtext, char *word);
	
	/* Cursors */
	GdkCursor *hand_cursor;
	GdkCursor *resize_cursor;
};

struct _GtkXTextViewClass {
	GtkScrolledWindowClass parent_class;
	
	/* Signals */
	void (*word_click) (GtkXTextView *xtext, char *word, GdkEventButton *event);
	void (*set_scroll_adjustments) (GtkXTextView *xtext, GtkAdjustment *hadj, GtkAdjustment *vadj);
};

/* API Functions - maintain exact same signatures for compatibility */
GType gtk_xtext_get_type (void);
GtkWidget *gtk_xtext_new (GdkRGBA palette[], int separator);

/* Text operations */
void gtk_xtext_append (xtext_buffer *buf, unsigned char *text, int len, time_t stamp);
void gtk_xtext_append_indent (xtext_buffer *buf,
                              unsigned char *left_text, int left_len,
                              unsigned char *right_text, int right_len,
                              time_t stamp);
void gtk_xtext_clear (xtext_buffer *buf, int lines);
void gtk_xtext_refresh (GtkXText *xtext);

/* Styling */
int gtk_xtext_set_font (GtkXText *xtext, char *name);
void gtk_xtext_set_background (GtkXText *xtext, GdkPixbuf *pixmap);
void gtk_xtext_set_palette (GtkXText *xtext, GdkRGBA palette[]);

/* Search and navigation */
textentry *gtk_xtext_search (GtkXText *xtext, const gchar *text, gtk_xtext_search_flags flags, GError **err);
void gtk_xtext_reset_marker_pos (GtkXText *xtext);
int gtk_xtext_moveto_marker_pos (GtkXText *xtext);
void gtk_xtext_check_marker_visibility (GtkXText *xtext);

/* Buffer management */
xtext_buffer *gtk_xtext_buffer_new (GtkXText *xtext);
void gtk_xtext_buffer_free (xtext_buffer *buf);
void gtk_xtext_buffer_show (GtkXText *xtext, xtext_buffer *buf, int render);
gboolean gtk_xtext_is_empty (xtext_buffer *buf);

/* Settings */
void gtk_xtext_set_indent (GtkXText *xtext, gboolean indent);
void gtk_xtext_set_max_indent (GtkXText *xtext, int max_auto_indent);
void gtk_xtext_set_max_lines (GtkXText *xtext, int max_lines);
void gtk_xtext_set_show_marker (GtkXText *xtext, gboolean show_marker);
void gtk_xtext_set_show_separator (GtkXText *xtext, gboolean show_separator);
void gtk_xtext_set_thin_separator (GtkXText *xtext, gboolean thin_separator);
void gtk_xtext_set_time_stamp (xtext_buffer *buf, gboolean timestamp);
void gtk_xtext_set_urlcheck_function (GtkXText *xtext, int (*urlcheck_function) (GtkWidget *, char *));
void gtk_xtext_set_wordwrap (GtkXText *xtext, gboolean word_wrap);
void gtk_xtext_set_error_function (GtkXText *xtext, void (*error_function) (int));

/* Utility functions */
void gtk_xtext_save (GtkXText *xtext, int fh);
void gtk_xtext_copy_selection (GtkXText *xtext);
int gtk_xtext_lastlog (xtext_buffer *out, xtext_buffer *search_area);

/* Iteration support */
typedef void (*GtkXTextForeach) (GtkXText *xtext, unsigned char *text, void *data);
void gtk_xtext_foreach (xtext_buffer *buf, GtkXTextForeach func, void *data);

/* Session support (forward declaration) */
struct session;
void gtk_xtext_set_marker_last (struct session *sess);

G_END_DECLS

#endif /* HEXCHAT_GTK_XTEXT_VIEW_H */
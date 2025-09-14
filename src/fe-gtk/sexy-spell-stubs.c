/* GTK3 Migration: Temporary stubs for sexy spell entry functions */

#include "config.h"
#include <gtk/gtk.h>

/* Minimal SexySpellEntry implementation for GTK3 */

#define SEXY_TYPE_SPELL_ENTRY            (sexy_spell_entry_get_type())
#define SEXY_SPELL_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), SEXY_TYPE_SPELL_ENTRY, SexySpellEntry))
#define SEXY_IS_SPELL_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), SEXY_TYPE_SPELL_ENTRY))
#define SEXY_SPELL_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), SEXY_TYPE_SPELL_ENTRY, SexySpellEntryClass))
#define SEXY_IS_SPELL_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), SEXY_TYPE_SPELL_ENTRY))

typedef struct _SexySpellEntry      SexySpellEntry;
typedef struct _SexySpellEntryClass SexySpellEntryClass;

struct _SexySpellEntry {
    GtkEntry parent_instance;
};

struct _SexySpellEntryClass {
    GtkEntryClass parent_class;
    gboolean (*word_check)(SexySpellEntry *entry, const gchar *word);
};

enum {
    WORD_CHECK,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE(SexySpellEntry, sexy_spell_entry, GTK_TYPE_ENTRY)

static gboolean
default_word_check(SexySpellEntry *entry, const gchar *word)
{
    /* GTK3: Always return FALSE (no spell errors) for stub */
    return FALSE;
}

static void
sexy_spell_entry_class_init(SexySpellEntryClass *klass)
{
    klass->word_check = default_word_check;
    
    signals[WORD_CHECK] = g_signal_new("word_check",
                                       G_TYPE_FROM_CLASS(klass),
                                       G_SIGNAL_RUN_LAST,
                                       G_STRUCT_OFFSET(SexySpellEntryClass, word_check),
                                       NULL, NULL,
                                       g_cclosure_marshal_generic,
                                       G_TYPE_BOOLEAN, 1,
                                       G_TYPE_STRING);
}

static void
sexy_spell_entry_init(SexySpellEntry *entry)
{
    /* GTK3: Basic initialization */
}

GtkWidget *sexy_spell_entry_new(void)
{
    return g_object_new(SEXY_TYPE_SPELL_ENTRY, NULL);
}

void sexy_spell_entry_set_checked(GtkWidget *entry, gboolean checked)
{
	/* GTK3: Stub - spell checking disabled */
	(void)entry;
	(void)checked;
}

void sexy_spell_entry_set_parse_attributes(GtkWidget *entry, gboolean parse)
{
	/* GTK3: Stub - attribute parsing disabled */
	(void)entry;
	(void)parse;
}

void sexy_spell_entry_deactivate_language(GtkWidget *entry)
{
	/* GTK3: Stub - language deactivation disabled */
	(void)entry;
}

void sexy_spell_entry_activate_default_languages(GtkWidget *entry)
{
	/* GTK3: Stub - language activation disabled */
	(void)entry;
}

void create_input_style(void)
{
	/* GTK3: Stub - CSS styling handled elsewhere */
}
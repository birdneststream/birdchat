/* X-Chat
 * Copyright (C) 1998 Peter Zelezny.
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "fe-gtk.h"
#include "palette.h"

#include "../common/hexchat.h"
#include "../common/util.h"
#include "../common/cfgfiles.h"
#include "../common/typedef.h"


GdkRGBA colors[MAX_COL+1] = {
	/* Standard 99-color IRC palette */
	{1.0, 1.0, 1.0, 1.0}, /* 0 white */
	{0.0, 0.0, 0.0, 1.0}, /* 1 black */
	{0.0, 0.0, 0.498, 1.0}, /* 2 blue */
	{0.0, 0.576, 0.0, 1.0}, /* 3 green */
	{1.0, 0.0, 0.0, 1.0}, /* 4 red */
	{0.498, 0.0, 0.0, 1.0}, /* 5 light red */
	{0.612, 0.0, 0.612, 1.0}, /* 6 purple */
	{0.988, 0.498, 0.0, 1.0}, /* 7 orange */
	{1.0, 1.0, 0.0, 1.0}, /* 8 yellow */
	{0.0, 0.988, 0.0, 1.0}, /* 9 light green */
	{0.0, 0.576, 0.576, 1.0}, /* 10 aqua */
	{0.0, 1.0, 1.0, 1.0}, /* 11 light aqua */
	{0.0, 0.0, 0.988, 1.0}, /* 12 light blue */
	{1.0, 0.0, 1.0, 1.0}, /* 13 light purple */
	{0.498, 0.498, 0.498, 1.0}, /* 14 grey */
	{0.824, 0.824, 0.824, 1.0}, /* 15 light grey */
	{0.278, 0.0, 0.0, 1.0}, /* 16 */
	{0.278, 0.129, 0.0, 1.0}, /* 17 */
	{0.278, 0.278, 0.0, 1.0}, /* 18 */
	{0.196, 0.278, 0.0, 1.0}, /* 19 */
	{0.0, 0.278, 0.0, 1.0}, /* 20 */
	{0.0, 0.278, 0.173, 1.0}, /* 21 */
	{0.0, 0.278, 0.278, 1.0}, /* 22 */
	{0.0, 0.153, 0.278, 1.0}, /* 23 */
	{0.0, 0.0, 0.278, 1.0}, /* 24 */
	{0.18, 0.0, 0.278, 1.0}, /* 25 */
	{0.278, 0.0, 0.278, 1.0}, /* 26 */
	{0.278, 0.0, 0.165, 1.0}, /* 27 */
	{0.455, 0.0, 0.0, 1.0}, /* 28 */
	{0.455, 0.227, 0.0, 1.0}, /* 29 */
	{0.455, 0.455, 0.0, 1.0}, /* 30 */
	{0.318, 0.455, 0.0, 1.0}, /* 31 */
	{0.0, 0.455, 0.0, 1.0}, /* 32 */
	{0.0, 0.455, 0.286, 1.0}, /* 33 */
	{0.0, 0.455, 0.455, 1.0}, /* 34 */
	{0.0, 0.251, 0.455, 1.0}, /* 35 */
	{0.0, 0.0, 0.455, 1.0}, /* 36 */
	{0.294, 0.0, 0.455, 1.0}, /* 37 */
	{0.455, 0.0, 0.455, 1.0}, /* 38 */
	{0.455, 0.0, 0.271, 1.0}, /* 39 */
	{0.71, 0.0, 0.0, 1.0}, /* 40 */
	{0.71, 0.388, 0.0, 1.0}, /* 41 */
	{0.71, 0.71, 0.0, 1.0}, /* 42 */
	{0.49, 0.71, 0.0, 1.0}, /* 43 */
	{0.0, 0.71, 0.0, 1.0}, /* 44 */
	{0.0, 0.71, 0.443, 1.0}, /* 45 */
	{0.0, 0.71, 0.71, 1.0}, /* 46 */
	{0.0, 0.388, 0.71, 1.0}, /* 47 */
	{0.0, 0.0, 0.71, 1.0}, /* 48 */
	{0.459, 0.0, 0.71, 1.0}, /* 49 */
	{0.71, 0.0, 0.71, 1.0}, /* 50 */
	{0.71, 0.0, 0.42, 1.0}, /* 51 */
	{1.0, 0.0, 0.0, 1.0}, /* 52 */
	{1.0, 0.549, 0.0, 1.0}, /* 53 */
	{1.0, 1.0, 0.0, 1.0}, /* 54 */
	{0.698, 1.0, 0.0, 1.0}, /* 55 */
	{0.0, 1.0, 0.0, 1.0}, /* 56 */
	{0.0, 1.0, 0.627, 1.0}, /* 57 */
	{0.0, 1.0, 1.0, 1.0}, /* 58 */
	{0.0, 0.549, 1.0, 1.0}, /* 59 */
	{0.0, 0.0, 1.0, 1.0}, /* 60 */
	{0.647, 0.0, 1.0, 1.0}, /* 61 */
	{1.0, 0.0, 1.0, 1.0}, /* 62 */
	{1.0, 0.0, 0.596, 1.0}, /* 63 */
	{1.0, 0.349, 0.349, 1.0}, /* 64 */
	{1.0, 0.706, 0.349, 1.0}, /* 65 */
	{1.0, 1.0, 0.443, 1.0}, /* 66 */
	{0.812, 1.0, 0.376, 1.0}, /* 67 */
	{0.435, 1.0, 0.435, 1.0}, /* 68 */
	{0.396, 1.0, 0.788, 1.0}, /* 69 */
	{0.427, 1.0, 1.0, 1.0}, /* 70 */
	{0.349, 0.706, 1.0, 1.0}, /* 71 */
	{0.349, 0.349, 1.0, 1.0}, /* 72 */
	{0.769, 0.349, 1.0, 1.0}, /* 73 */
	{1.0, 0.4, 1.0, 1.0}, /* 74 */
	{1.0, 0.349, 0.737, 1.0}, /* 75 */
	{1.0, 0.612, 0.612, 1.0}, /* 76 */
	{1.0, 0.827, 0.612, 1.0}, /* 77 */
	{1.0, 1.0, 0.612, 1.0}, /* 78 */
	{0.886, 1.0, 0.612, 1.0}, /* 79 */
	{0.612, 1.0, 0.612, 1.0}, /* 80 */
	{0.612, 1.0, 0.859, 1.0}, /* 81 */
	{0.612, 1.0, 1.0, 1.0}, /* 82 */
	{0.612, 0.827, 1.0, 1.0}, /* 83 */
	{0.612, 0.612, 1.0, 1.0}, /* 84 */
	{0.863, 0.612, 1.0, 1.0}, /* 85 */
	{1.0, 0.612, 1.0, 1.0}, /* 86 */
	{1.0, 0.58, 0.827, 1.0}, /* 87 */
	{0.0, 0.0, 0.0, 1.0}, /* 88 */
	{0.075, 0.075, 0.075, 1.0}, /* 89 */
	{0.157, 0.157, 0.157, 1.0}, /* 90 */
	{0.212, 0.212, 0.212, 1.0}, /* 91 */
	{0.302, 0.302, 0.302, 1.0}, /* 92 */
	{0.396, 0.396, 0.396, 1.0}, /* 93 */
	{0.506, 0.506, 0.506, 1.0}, /* 94 */
	{0.624, 0.624, 0.624, 1.0}, /* 95 */
	{0.737, 0.737, 0.737, 1.0}, /* 96 */
	{0.886, 0.886, 0.886, 1.0}, /* 97 */
	{1.0, 1.0, 1.0, 1.0}, /* 98 */

	/* system colors */
	{0.827, 0.843, 0.812, 1.0}, /* 99 marktext Fore (white) */
	{0.125, 0.29, 0.529, 1.0}, /* 100 marktext Back (blue) */
	{0.145, 0.164, 0.17, 1.0}, /* 101 foreground (black) */
	{0.98, 0.98, 0.972, 1.0}, /* 102 background (white) */
	{0.561, 0.224, 0.008, 1.0}, /* 103 marker line (red) */
	{0.204, 0.396, 0.643, 1.0}, /* 104 tab New Data (dark red) */
	{0.306, 0.604, 0.024, 1.0}, /* 105 tab Nick Mentioned (blue) */
	{0.808, 0.361, 0.0, 1.0}, /* 106 tab New Message (red) */
	{0.533, 0.541, 0.522, 1.0}, /* 107 away user (grey) */
	{0.643, 0.0, 0.0, 1.0}, /* 108 spell checker color (red) */
};

void
palette_alloc (GtkWidget * widget)
{
	/* GTK3: Color allocation is automatic, no manual allocation needed */
	(void)widget; /* suppress unused parameter warning */
}

void
palette_load (void)
{
	int i, j, fh;
	char prefname[256];
	struct stat st;
	char *cfg;
	guint16 red, green, blue;

	fh = hexchat_open_file ("colors.conf", O_RDONLY, 0, 0);
	if (fh != -1)
	{
		fstat (fh, &st);
		cfg = g_malloc0 (st.st_size + 1);
		read (fh, cfg, st.st_size);

		/* old theme format writes colors [0, THEME_MAX_MIRC_COLORS) and [256, ...) */
		g_snprintf (prefname, sizeof prefname, "color_%d", THEME_MAX_MIRC_COLS);
		if (!cfg_get_color (cfg, prefname, &red, &green, &blue))
		{
			/* old theme detected, migrate low local colors [0, 32) */
			for (i = 0; i < THEME_MAX_MIRC_COLS; i++)
			{
				g_snprintf (prefname, sizeof prefname, "color_%d", i);
				if (cfg_get_color (cfg, prefname, &red, &green, &blue))
				{
					colors[i].red = red / 65535.0;
					colors[i].green = green / 65535.0;
					colors[i].blue = blue / 65535.0;
					colors[i].alpha = 1.0;
				}
			}
			/* migrate high system colors mapped at [256, ...) */
			for (i = 256, j = COL_START_SYS; j < MAX_COL+1; i++, j++)
			{
				g_snprintf (prefname, sizeof prefname, "color_%d", i);
				if (cfg_get_color (cfg, prefname, &red, &green, &blue))
				{
					colors[j].red = red / 65535.0;
					colors[j].green = green / 65535.0;
					colors[j].blue = blue / 65535.0;
					colors[j].alpha = 1.0;
				}
			}
		}
		else
		{
			/* new theme format, read colors [0, 99) directly and system colors at [256, ...) */
			for (i = 0; i < MIRC_COLS; i++)
			{
				g_snprintf (prefname, sizeof prefname, "color_%d", i);
				if (cfg_get_color (cfg, prefname, &red, &green, &blue))
				{
					colors[i].red = red / 65535.0;
					colors[i].green = green / 65535.0;
					colors[i].blue = blue / 65535.0;
					colors[i].alpha = 1.0;
				}
			}
			/* system colors are still mapped at [256, ...) */
			for (i = 256, j = COL_START_SYS; j < MAX_COL+1; i++, j++)
			{
				g_snprintf (prefname, sizeof prefname, "color_%d", i);
				if (cfg_get_color (cfg, prefname, &red, &green, &blue))
				{
					colors[j].red = red / 65535.0;
					colors[j].green = green / 65535.0;
					colors[j].blue = blue / 65535.0;
					colors[j].alpha = 1.0;
				}
			}
		}
		g_free (cfg);
		close (fh);
	}
}

void
palette_save (void)
{
	int i, j, fh;
	char prefname[256];

	fh = hexchat_open_file ("colors.conf", O_TRUNC | O_WRONLY | O_CREAT, 0600, XOF_DOMODE);
	if (fh != -1)
	{
		/* save all 99 mIRC colors directly */
		for (i = 0; i < MIRC_COLS; i++)
		{
			g_snprintf (prefname, sizeof prefname, "color_%d", i);
			cfg_put_color (fh, (guint16)(colors[i].red * 65535), (guint16)(colors[i].green * 65535), (guint16)(colors[i].blue * 65535), prefname);
		}

		/* system colors are still mapped at 256+ */
		for (i = 256, j = COL_START_SYS; j < MAX_COL+1; i++, j++)
		{
			g_snprintf (prefname, sizeof prefname, "color_%d", i);
			cfg_put_color (fh, (guint16)(colors[j].red * 65535), (guint16)(colors[j].green * 65535), (guint16)(colors[j].blue * 65535), prefname);
		}

		close (fh);
	}
}

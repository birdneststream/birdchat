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


GdkColor colors[MAX_COL+1] = {
	/* Standard 99-color IRC palette */
	{0, 0xffff, 0xffff, 0xffff}, /* 0 white */
	{0, 0x0000, 0x0000, 0x0000}, /* 1 black */
	{0, 0x0000, 0x0000, 0x7f7f}, /* 2 blue */
	{0, 0x0000, 0x9393, 0x0000}, /* 3 green */
	{0, 0xffff, 0x0000, 0x0000}, /* 4 red */
	{0, 0x7f7f, 0x0000, 0x0000}, /* 5 light red */
	{0, 0x9c9c, 0x0000, 0x9c9c}, /* 6 purple */
	{0, 0xfcfc, 0x7f7f, 0x0000}, /* 7 orange */
	{0, 0xffff, 0xffff, 0x0000}, /* 8 yellow */
	{0, 0x0000, 0xfcfc, 0x0000}, /* 9 light green */
	{0, 0x0000, 0x9393, 0x9393}, /* 10 aqua */
	{0, 0x0000, 0xffff, 0xffff}, /* 11 light aqua */
	{0, 0x0000, 0x0000, 0xfcfc}, /* 12 light blue */
	{0, 0xffff, 0x0000, 0xffff}, /* 13 light purple */
	{0, 0x7f7f, 0x7f7f, 0x7f7f}, /* 14 grey */
	{0, 0xd2d2, 0xd2d2, 0xd2d2}, /* 15 light grey */
	{0, 0x4747, 0x0000, 0x0000}, /* 16 */
	{0, 0x4747, 0x2121, 0x0000}, /* 17 */
	{0, 0x4747, 0x4747, 0x0000}, /* 18 */
	{0, 0x3232, 0x4747, 0x0000}, /* 19 */
	{0, 0x0000, 0x4747, 0x0000}, /* 20 */
	{0, 0x0000, 0x4747, 0x2c2c}, /* 21 */
	{0, 0x0000, 0x4747, 0x4747}, /* 22 */
	{0, 0x0000, 0x2727, 0x4747}, /* 23 */
	{0, 0x0000, 0x0000, 0x4747}, /* 24 */
	{0, 0x2e2e, 0x0000, 0x4747}, /* 25 */
	{0, 0x4747, 0x0000, 0x4747}, /* 26 */
	{0, 0x4747, 0x0000, 0x2a2a}, /* 27 */
	{0, 0x7474, 0x0000, 0x0000}, /* 28 */
	{0, 0x7474, 0x3a3a, 0x0000}, /* 29 */
	{0, 0x7474, 0x7474, 0x0000}, /* 30 */
	{0, 0x5151, 0x7474, 0x0000}, /* 31 */
	{0, 0x0000, 0x7474, 0x0000}, /* 32 */
	{0, 0x0000, 0x7474, 0x4949}, /* 33 */
	{0, 0x0000, 0x7474, 0x7474}, /* 34 */
	{0, 0x0000, 0x4040, 0x7474}, /* 35 */
	{0, 0x0000, 0x0000, 0x7474}, /* 36 */
	{0, 0x4b4b, 0x0000, 0x7474}, /* 37 */
	{0, 0x7474, 0x0000, 0x7474}, /* 38 */
	{0, 0x7474, 0x0000, 0x4545}, /* 39 */
	{0, 0xb5b5, 0x0000, 0x0000}, /* 40 */
	{0, 0xb5b5, 0x6363, 0x0000}, /* 41 */
	{0, 0xb5b5, 0xb5b5, 0x0000}, /* 42 */
	{0, 0x7d7d, 0xb5b5, 0x0000}, /* 43 */
	{0, 0x0000, 0xb5b5, 0x0000}, /* 44 */
	{0, 0x0000, 0xb5b5, 0x7171}, /* 45 */
	{0, 0x0000, 0xb5b5, 0xb5b5}, /* 46 */
	{0, 0x0000, 0x6363, 0xb5b5}, /* 47 */
	{0, 0x0000, 0x0000, 0xb5b5}, /* 48 */
	{0, 0x7575, 0x0000, 0xb5b5}, /* 49 */
	{0, 0xb5b5, 0x0000, 0xb5b5}, /* 50 */
	{0, 0xb5b5, 0x0000, 0x6b6b}, /* 51 */
	{0, 0xffff, 0x0000, 0x0000}, /* 52 */
	{0, 0xffff, 0x8c8c, 0x0000}, /* 53 */
	{0, 0xffff, 0xffff, 0x0000}, /* 54 */
	{0, 0xb2b2, 0xffff, 0x0000}, /* 55 */
	{0, 0x0000, 0xffff, 0x0000}, /* 56 */
	{0, 0x0000, 0xffff, 0xa0a0}, /* 57 */
	{0, 0x0000, 0xffff, 0xffff}, /* 58 */
	{0, 0x0000, 0x8c8c, 0xffff}, /* 59 */
	{0, 0x0000, 0x0000, 0xffff}, /* 60 */
	{0, 0xa5a5, 0x0000, 0xffff}, /* 61 */
	{0, 0xffff, 0x0000, 0xffff}, /* 62 */
	{0, 0xffff, 0x0000, 0x9898}, /* 63 */
	{0, 0xffff, 0x5959, 0x5959}, /* 64 */
	{0, 0xffff, 0xb4b4, 0x5959}, /* 65 */
	{0, 0xffff, 0xffff, 0x7171}, /* 66 */
	{0, 0xcfcf, 0xffff, 0x6060}, /* 67 */
	{0, 0x6f6f, 0xffff, 0x6f6f}, /* 68 */
	{0, 0x6565, 0xffff, 0xc9c9}, /* 69 */
	{0, 0x6d6d, 0xffff, 0xffff}, /* 70 */
	{0, 0x5959, 0xb4b4, 0xffff}, /* 71 */
	{0, 0x5959, 0x5959, 0xffff}, /* 72 */
	{0, 0xc4c4, 0x5959, 0xffff}, /* 73 */
	{0, 0xffff, 0x6666, 0xffff}, /* 74 */
	{0, 0xffff, 0x5959, 0xbcbc}, /* 75 */
	{0, 0xffff, 0x9c9c, 0x9c9c}, /* 76 */
	{0, 0xffff, 0xd3d3, 0x9c9c}, /* 77 */
	{0, 0xffff, 0xffff, 0x9c9c}, /* 78 */
	{0, 0xe2e2, 0xffff, 0x9c9c}, /* 79 */
	{0, 0x9c9c, 0xffff, 0x9c9c}, /* 80 */
	{0, 0x9c9c, 0xffff, 0xdbdb}, /* 81 */
	{0, 0x9c9c, 0xffff, 0xffff}, /* 82 */
	{0, 0x9c9c, 0xd3d3, 0xffff}, /* 83 */
	{0, 0x9c9c, 0x9c9c, 0xffff}, /* 84 */
	{0, 0xdcdc, 0x9c9c, 0xffff}, /* 85 */
	{0, 0xffff, 0x9c9c, 0xffff}, /* 86 */
	{0, 0xffff, 0x9494, 0xd3d3}, /* 87 */
	{0, 0x0000, 0x0000, 0x0000}, /* 88 */
	{0, 0x1313, 0x1313, 0x1313}, /* 89 */
	{0, 0x2828, 0x2828, 0x2828}, /* 90 */
	{0, 0x3636, 0x3636, 0x3636}, /* 91 */
	{0, 0x4d4d, 0x4d4d, 0x4d4d}, /* 92 */
	{0, 0x6565, 0x6565, 0x6565}, /* 93 */
	{0, 0x8181, 0x8181, 0x8181}, /* 94 */
	{0, 0x9f9f, 0x9f9f, 0x9f9f}, /* 95 */
	{0, 0xbcbc, 0xbcbc, 0xbcbc}, /* 96 */
	{0, 0xe2e2, 0xe2e2, 0xe2e2}, /* 97 */
	{0, 0xffff, 0xffff, 0xffff}, /* 98 */

	/* system colors */
	{0, 0xd3d3, 0xd7d7, 0xcfcf}, /* 99 marktext Fore (white) */
	{0, 0x2020, 0x4a4a, 0x8787}, /* 100 marktext Back (blue) */
	{0, 0x2512, 0x29e8, 0x2b85}, /* 101 foreground (black) */
	{0, 0xfae0, 0xfae0, 0xf8c4}, /* 102 background (white) */
	{0, 0x8f8f, 0x3939, 0x0202}, /* 103 marker line (red) */
	{0, 0x3434, 0x6565, 0xa4a4}, /* 104 tab New Data (dark red) */
	{0, 0x4e4e, 0x9a9a, 0x0606}, /* 105 tab Nick Mentioned (blue) */
	{0, 0xcece, 0x5c5c, 0x0000}, /* 106 tab New Message (red) */
	{0, 0x8888, 0x8a8a, 0x8585}, /* 107 away user (grey) */
	{0, 0xa4a4, 0x0000, 0x0000}, /* 108 spell checker color (red) */
};

void
palette_alloc (GtkWidget * widget)
{
	int i;
	static int done_alloc = FALSE;
	GdkColormap *cmap;

	if (!done_alloc)		  /* don't do it again */
	{
		done_alloc = TRUE;
		cmap = gtk_widget_get_colormap (widget);
		for (i = MAX_COL; i >= 0; i--)
			gdk_colormap_alloc_color (cmap, &colors[i], FALSE, TRUE);
	}
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
					colors[i].red = red;
					colors[i].green = green;
					colors[i].blue = blue;
				}
			}
			/* migrate high system colors mapped at [256, ...) */
			for (i = 256, j = COL_START_SYS; j < MAX_COL+1; i++, j++)
			{
				g_snprintf (prefname, sizeof prefname, "color_%d", i);
				if (cfg_get_color (cfg, prefname, &red, &green, &blue))
				{
					colors[j].red = red;
					colors[j].green = green;
					colors[j].blue = blue;
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
					colors[i].red = red;
					colors[i].green = green;
					colors[i].blue = blue;
				}
			}
			/* system colors are still mapped at [256, ...) */
			for (i = 256, j = COL_START_SYS; j < MAX_COL+1; i++, j++)
			{
				g_snprintf (prefname, sizeof prefname, "color_%d", i);
				if (cfg_get_color (cfg, prefname, &red, &green, &blue))
				{
					colors[j].red = red;
					colors[j].green = green;
					colors[j].blue = blue;
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
			cfg_put_color (fh, colors[i].red, colors[i].green, colors[i].blue, prefname);
		}

		/* system colors are still mapped at 256+ */
		for (i = 256, j = COL_START_SYS; j < MAX_COL+1; i++, j++)
		{
			g_snprintf (prefname, sizeof prefname, "color_%d", i);
			cfg_put_color (fh, colors[j].red, colors[j].green, colors[j].blue, prefname);
		}

		close (fh);
	}
}

/*
 * editpolicy_color.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2009  NTT DATA CORPORATION
 *
 * Version: 1.6.7   2009/04/01
 *
 */
#include "ccstools.h"

/* Prototypes */

static void editpolicy_color_save(const _Bool flg);

/* Main functions */

#ifdef COLOR_ON

void editpolicy_color_init(void)
{
	static struct color_env_t {
		enum color_pair	tag;
		short int fore;
		short int back;
		const char *name;
	} color_env[] = {
		{ DOMAIN_HEAD,      COLOR_BLACK,
		  COLOR_GREEN,      "DOMAIN_HEAD" },
		{ DOMAIN_CURSOR,    COLOR_BLACK,
		  COLOR_GREEN,      "DOMAIN_CURSOR" },
		{ SYSTEM_HEAD,      COLOR_WHITE,
		  COLOR_BLUE,       "SYSTEM_HEAD" },
		{ SYSTEM_CURSOR,    COLOR_WHITE,
		  COLOR_BLUE,       "SYSTEM_CURSOR" },
		{ EXCEPTION_HEAD,   COLOR_BLACK,
		  COLOR_CYAN,       "EXCEPTION_HEAD" },
		{ EXCEPTION_CURSOR, COLOR_BLACK,
		  COLOR_CYAN,       "EXCEPTION_CURSOR" },
		{ ACL_HEAD,         COLOR_BLACK,
		  COLOR_YELLOW,     "ACL_HEAD" },
		{ ACL_CURSOR,       COLOR_BLACK,
		  COLOR_YELLOW,     "ACL_CURSOR" },
		{ PROFILE_HEAD,     COLOR_WHITE,
		  COLOR_RED,        "PROFILE_HEAD" },
		{ PROFILE_CURSOR,   COLOR_WHITE,
		  COLOR_RED,        "PROFILE_CURSOR" },
		{ MANAGER_HEAD,     COLOR_WHITE,
		  COLOR_GREEN,      "MANAGER_HEAD" },
		{ MANAGER_CURSOR,   COLOR_WHITE,
		  COLOR_GREEN,      "MANAGER_CURSOR" },
		{ MEMORY_HEAD,      COLOR_BLACK,
		  COLOR_YELLOW,     "MEMORY_HEAD" },
		{ MEMORY_CURSOR,    COLOR_BLACK,
		  COLOR_YELLOW,     "MEMORY_CURSOR" },
		{ NORMAL,           COLOR_WHITE,
		  COLOR_BLACK,      NULL }
	};
	FILE *fp = fopen(CCSTOOLS_CONFIG_FILE, "r");
	int i;
	if (!fp)
		goto use_default;
	get();
	while (freadline(fp)) {
		char *cp;
		if (!str_starts(shared_buffer, "editpolicy.line_color "))
			continue;
		cp = strchr(shared_buffer, '=');
		if (!cp)
			continue;
		*cp++ = '\0';
		normalize_line(shared_buffer);
		normalize_line(cp);
		if (!*shared_buffer || !*cp)
			continue;
		for (i = 0; color_env[i].name; i++) {
			short int fore;
			short int back;
			if (strcmp(shared_buffer, color_env[i].name))
				continue;
			if (strlen(cp) != 2)
				break;
			fore = (*cp++) - '0'; /* foreground color */
			back = (*cp) - '0';   /* background color */
			if (fore < 0 || fore > 7 || back < 0 || back > 7)
				break;
			color_env[i].fore = fore;
			color_env[i].back = back;
			break;
		}
	}
	put();
	fclose(fp);
use_default:
	start_color();
	for (i = 0; color_env[i].name; i++) {
		struct color_env_t *colorp = &color_env[i];
		init_pair(colorp->tag, colorp->fore, colorp->back);
	}
	init_pair(DISP_ERR, COLOR_RED, COLOR_BLACK); /* error message */
}

static void editpolicy_color_save(const _Bool flg)
{
	static attr_t save_color = NORMAL;
	if (flg)
		save_color = getattrs(stdscr);
	else
		attrset(save_color);
}

void editpolicy_color_change(const attr_t attr, const _Bool flg)
{
	if (flg)
		attron(COLOR_PAIR(attr));
	else
		attroff(COLOR_PAIR(attr));
}

void editpolicy_attr_change(const attr_t attr, const _Bool flg)
{
	if (flg)
		attron(attr);
	else
		attroff(attr);
}

void editpolicy_sttr_save(void)
{
	editpolicy_color_save(true);
}

void editpolicy_sttr_restore(void)
{
	editpolicy_color_save(false);
}

int editpolicy_color_head(const int screen)
{
	switch (screen) {
	case SCREEN_DOMAIN_LIST:
		return DOMAIN_HEAD;
	case SCREEN_SYSTEM_LIST:
		return SYSTEM_HEAD;
	case SCREEN_EXCEPTION_LIST:
		return EXCEPTION_HEAD;
	case SCREEN_PROFILE_LIST:
		return PROFILE_HEAD;
	case SCREEN_MANAGER_LIST:
		return MANAGER_HEAD;
	case SCREEN_MEMINFO_LIST:
		return MEMORY_HEAD;
	default:
		return ACL_HEAD;
	}
}

int editpolicy_color_cursor(const int screen)
{
	switch (screen) {
	case SCREEN_DOMAIN_LIST:
		return DOMAIN_CURSOR;
	case SCREEN_SYSTEM_LIST:
		return SYSTEM_CURSOR;
	case SCREEN_EXCEPTION_LIST:
		return EXCEPTION_CURSOR;
	case SCREEN_PROFILE_LIST:
		return PROFILE_CURSOR;
	case SCREEN_MANAGER_LIST:
		return MANAGER_CURSOR;
	case SCREEN_MEMINFO_LIST:
		return MEMORY_CURSOR;
	default:
		return ACL_CURSOR;
	}
}

void editpolicy_line_draw(const int screen)
{
	static int before_current[MAXSCREEN] = { -1, -1, -1, -1,
						 -1, -1, -1, -1 };
	static int before_y[MAXSCREEN]       = { -1, -1, -1, -1,
						 -1, -1, -1, -1 };
	int current = editpolicy_get_current();
	int y;
	int x;

	if (current == EOF)
		return;

	getyx(stdscr, y, x);
	if (-1 < before_current[screen] &&
	    current != before_current[screen]){
		move(header_lines + before_y[screen], 0);
		chgat(-1, A_NORMAL, NORMAL, NULL);
	}

	move(y, x);
	chgat(-1, A_NORMAL, editpolicy_color_cursor(screen), NULL);
	touchwin(stdscr);

	before_current[screen] = current;
	before_y[screen] = current_y[screen];
}

#else

void editpolicy_color_init(void)
{
}
void editpolicy_color_change(const attr_t attr, const _Bool flg)
{
}
void editpolicy_attr_change(const attr_t attr, const _Bool flg)
{
}
void editpolicy_sttr_save(void)
{
}
void editpolicy_sttr_restore(void)
{
}
int editpolicy_color_head(const int screen)
{
}
int editpolicy_color_cursor(const int screen)
{
}
void editpolicy_line_draw(const int screen)
{
}

#endif

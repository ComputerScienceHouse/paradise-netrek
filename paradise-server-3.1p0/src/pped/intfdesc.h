/*
 * interface.h
 */

#ifndef INTFDESC_H
#define INTFDESC_H

enum desc_type {
	DT_INT,		/* plain int */
	DT_FLOAT,	/* plain float */
	DT_CHAR16,	/* name/passwd */
	DT_PWD,		/* name/passwd */
	DT_TICKS,	/* (int) seconds/10 */
	DT_RANK,	/* (int) rank */
	DT_ROYAL	/* (int) royalty */
};

struct inter_desc {
	int num;
	char *name;
	enum desc_type type;
	int offset;
};

#define OFFSET(field) ((char *)(&((struct statentry*)0)->field) - (char *)0)

struct inter_desc idesc_tab[] = {
    {  0, "Name",	DT_CHAR16,	OFFSET(name[0]) },
    {  1, "Password",	DT_PWD,		OFFSET(password[0]) },
    {  2, "Rank",	DT_RANK,	OFFSET(stats.st_rank) },
    {  3, "Royalty",	DT_ROYAL,	OFFSET(stats.st_royal) },
    {  4, "Genocides",	DT_INT,		OFFSET(stats.st_genocides) },
    {  5, "Max kills",	DT_FLOAT,	OFFSET(stats.st_tmaxkills) },
    {  6, "DI",		DT_FLOAT, 	OFFSET(stats.st_di) },
    {  7, "Kills",	DT_INT,		OFFSET(stats.st_tkills) },
    {  8, "Losses",	DT_INT,		OFFSET(stats.st_tlosses) },
    {  9, "Armies bombed", DT_INT,	OFFSET(stats.st_tarmsbomb) },
    { 10, "Resources bombed", DT_INT,	OFFSET(stats.st_tresbomb) },
    { 11, "Dooshes",	DT_INT,		OFFSET(stats.st_tdooshes) },
    { 12, "Ticks",	DT_TICKS,	OFFSET(stats.st_tticks) },
    { 13, "SB kills",	DT_INT,		OFFSET(stats.st_sbkills) },
    { 14, "SB losses",  DT_INT,		OFFSET(stats.st_sblosses) },
    { 15, "SB ticks",	DT_TICKS,	OFFSET(stats.st_sbticks) },
    { 16, "SB maxkills", DT_FLOAT,	OFFSET(stats.st_sbmaxkills) },
    { 17, "WB kills",	DT_INT,		OFFSET(stats.st_wbkills) },
    { 18, "WB losses",  DT_INT,		OFFSET(stats.st_wblosses) },
    { 19, "WB ticks",	DT_TICKS,	OFFSET(stats.st_wbticks) },
    { 20, "WB maxkills", DT_FLOAT,	OFFSET(stats.st_wbmaxkills) },
    { 21, "JS planets", DT_INT,		OFFSET(stats.st_jsplanets) },
    { 22, "JS ticks",	DT_TICKS,	OFFSET(stats.st_jsticks) },
};

#define NUMDESC 23

#endif /* INTFDESC_H */

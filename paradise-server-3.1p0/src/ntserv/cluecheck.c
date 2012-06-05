/*------------------------------------------------------------------
  Copyright 1989		Kevin P. Smith
				Scott Silvey

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies.

  NETREK II -- Paradise

  Permission to use, copy, modify, and distribute this software and
  its documentation, or any derivative works thereof,  for any 
  NON-COMMERCIAL purpose and without fee is hereby granted, provided
  that this copyright notice appear in all copies.  No
  representations are made about the suitability of this software for
  any purpose.  This software is provided "as is" without express or
  implied warranty.

	Xtrek Copyright 1986			Chris Guthrie
	Netrek (Xtrek II) Copyright 1989	Kevin P. Smith
						Scott Silvey
	Paradise II (Netrek II) Copyright 1993	Larry Denys
						Kurt Olsen
						Brandon Gillespie
		                Copyright 2000  Bob Glamm

--------------------------------------------------------------------*/

#include "config.h"
#include "proto.h"
#include "ntserv.h"

#ifdef CLUECHECK1

/* ----------------- ROBS CLUECHECK -------------------- */

static char	clueword[40];

static void
set_clue_word(char *word)
{
  strncpy(clueword, word, sizeof(clueword));
  clueword[sizeof(clueword)-1] = 0;
}

#ifdef MOTD_SUPPORT
static char	*motdstring=0;
static int	motdlen=0;

static int	line, wordn;

/* structures for the internal representation of the MOTD */
#define LINESPERPAGE	38

struct word {
    char	*s;
};

struct line {
    struct word *words;
  int	nwords;
};

struct page {
    struct line	lines[LINESPERPAGE];
  int	nlines;
    struct page *next;
};

struct page	*motdhead=0;


static void 
free_word(struct word *wd)
{
  free(wd->s);
}

static void
free_line(struct line *ln)
{
  int	i;
  for (i=0; i<ln->nwords; i++)
    free_word(&ln->words[i]);
  free(ln->words);
}

static void
free_page(struct page *pg)
{
  int	i;
  for (i=0; i<LINESPERPAGE; i++) {
    free_line(&pg->lines[i]);
  }
}

static void 
free_motdstruct(void)
{
    struct page	*temp;
    while (motdhead) {
	temp = motdhead;
	motdhead = temp->next;

	free_page(temp);
	free(temp);
    }
}
#endif

/* read the MOTD into core so we can parse it later */
void
init_motdbuf(char *fname)
{
#ifdef MOTD_SUPPORT
    struct stat	stats;
    FILE	*fp;

    if (motdstring!=0) {
	free_motdstruct();
	free(motdstring);
	motdstring=0;
	motdlen = 0;
    }

    if (!configvals->cluecheck ||
	configvals->cluecheck != CC_MOTD)
      return;			/* don't waste the memory if we're not
				   clue checking from the MOTD*/

    if (0>stat(fname, &stats)) {
	perror("statting file");
	return;
    }

    motdlen=stats.st_size;

    motdstring = (char*)malloc(motdlen+1);
    fp = fopen(fname, "r");
    if (0==fp) {
	perror("opening file");
	exit(1);
    }
    fread(motdstring, motdlen, 1, fp);
#endif
}

static int page;
static int	isfirst;

#ifdef MOTD_SUPPORT


#define MAXATTEMPTS	10
static void
find_suitable_motd_word(void)
{
    int	attempts;
    int	pagecount;
    int	i;
    struct page	*currp;


    for (pagecount=0, currp = motdhead;
	 currp;
	 pagecount++, currp = currp->next)
	;

    if (pagecount>10)
	pagecount=10;

    for (attempts=0; pagecount>1 && attempts<MAXATTEMPTS; attempts++) {
	/* the first page is excluded.  Everybody sees that */
	page = 1+ lrand48()%(pagecount-1);

	for (i=0, currp=motdhead; i<page; i++, currp = currp->next)
	    ;
	if (currp->nlines<1)
	    continue;
	if (lrand48()&1) {
	    /* get first word */
	    isfirst=1;
	    for (line=0; line<currp->nlines; line++)
		if (currp->lines[line].nwords>0)
		    break;
	    if (line>=currp->nlines)
		continue;
	    wordn = 0;
	} else {
	    /* get last word */
	    isfirst=0;
	    for (line=currp->nlines-1; line>=0; line--)
		if (currp->lines[line].nwords>0)
		    break;
	    if (line<0)
		continue;
	    wordn = currp->lines[line].nwords-1;
	}
	set_clue_word(currp->lines[line].words[wordn].s);
	/*    printf("%s word on page %d (line %d word %d) is %s\n",
	      isfirst?"first":"last", page+1, line+1, wordn+1, clueword); */
	return;
    }
    page = -1;
}

#if 0
/* for the day when we have line numbers in the MOTD. */
void find_suitable_word2()
{
    int	attempts;
    int	pagecount;
    int	i;
    struct page	*currp;


    for (pagecount=0, currp = motdhead; currp; pagecount++, currp = currp->next)
	;

    for (attempts=0; attempts<10; attempts++) {
	page = lrand48()%pagecount;
	for (i=0, currp=motdhead; i<page; i++, currp = currp->next)
	    ;
	if (currp->nlines<1)
	    continue;
	line = lrand48()%currp->nlines;
	if (currp->lines[line].nwords<1)
	    continue;
	wordn = lrand48() % currp->lines[line].nwords;
	set_clue_word(currp->lines[line].words[wordn].s);
	printf("word on page %d line %d word %d is %s\n",
	       page+1, line+1, wordn+1, clueword);
	return;
    }
    clueword[0] = 0;
}
#endif

#if 0
/* useful for debugging the MOTD parsing routine */
void printout_motd()
{
    struct page	*currp;
    int	i,j;
    for (currp=motdhead; currp; currp=currp->next) {
	for (i=0; i<currp->nlines; i++){
	    for (j=0; j<currp->lines[i].nwords; j++) {
		printf("%s ", currp->lines[i].words[j].s);
	    }
	    printf("\n");
	}
	printf("\014");
    }
}
#endif


static void
parse_motd(void)
{
    struct page **currp;
    int	idx;

    if (motdhead)
	return;

    currp = &motdhead;

    idx=0;
    while (idx<motdlen) {
	int	validword;
	char	*wordbegin;

	validword=1;

	/* skip whitespace */
	while (!isalpha(motdstring[idx]) && motdstring[idx]!='\n' && idx<motdlen)
	    idx++;
	if (idx>=motdlen)
	    break;

	if (0== *currp) {
	    *currp = malloc(sizeof(**currp));
	    (*currp)->nlines=1;
	    (*currp)->lines[0].nwords=0;
	    (*currp)->next = 0;
	}

	if (motdstring[idx]=='\n') {
	    idx++;
	    if (0==strncmp(&motdstring[idx], "\t@@@", 4))
		break;
	    else if (0==strncmp(&motdstring[idx], "\t@@b", 4)) {
		if (*currp)
		    currp = &(*currp)->next;
		idx += 4;
	    } else {
		struct line	*currl = &(*currp)->lines[(*currp)->nlines-1];
		currl->words = realloc(currl->words, sizeof(*currl->words)*currl->nwords);
		if ( (*currp)->nlines >= LINESPERPAGE )
		    currp = &(*currp)->next;
		else {
		    (*currp)->lines[(*currp)->nlines].nwords=0;
		    (*currp)->nlines++;
		}
	    }
	    continue;
	}
	wordbegin=&motdstring[idx];
	while (isalpha(motdstring[idx]) && idx<motdlen) {
#if 0
	    if (!isalpha(motdstring[idx]))
		validword=0;
#endif
	    idx++;
	}

	if (0 && !validword)
	    continue;

	{
	    struct line	*currl = &(*currp)->lines[(*currp)->nlines-1];
	    int	len;

	    if (currl->nwords==0) {
		int	size=40;
		int	j;
		currl->words = malloc(sizeof(struct word)*size);
		for (j=0; j<size; j++) {
		    currl->words[j].s = 0;
		}
	    }
	    len = (&motdstring[idx])- wordbegin;
	    currl->words[currl->nwords].s = malloc(len+1);
	    strncpy(currl->words[currl->nwords].s, wordbegin, len);
	    currl->words[currl->nwords].s[len] = 0;
	    currl->nwords++;
	}
    }
}
#endif

/**********************************************************************/

static char **phrases=0;
static int  num_phrases=0;

static void
parse_clue_phrases(void)
{
    char	*s;
    int	size;

    if (phrases)
	return;

    phrases = (char**)malloc(sizeof(*phrases)*(size=20));

    for (s=cluephrase_storage; *s; s += strlen(s)+1) {
	phrases[num_phrases] = s;
	num_phrases++;
	if (num_phrases>=size)
	    phrases = (char**)realloc(phrases, sizeof(*phrases)*(size*=2));
    }

    phrases = (char**)realloc(phrases, sizeof(*phrases)*num_phrases);
}

/**********************************************************************/


#define BERATE(msg)	pmessage( (msg), me->p_no, MINDIV, "   CC")

/* print the message that tells the person how to respond to the clue check */
static void
remind_cluecheck(void)
{
    char	buf[120];
    BERATE("This is a clue check!  You must send yourself the message");
    BERATE("     cluecheck [phrase]");
    if (page>=0) {
	sprintf(buf, "where [phrase] is the %s word on page %d of the MOTD.",
		isfirst?"first":"last", page+1);
    } else {
	/* man, the MOTD had no good words */
	sprintf(buf, "where [phrase] is %s", clueword);
    }
    BERATE(buf);
    if (me->p_cluecountdown>60)
	sprintf(buf, "If you don't answer within %g minutes, you will be kicked out of", me->p_cluecountdown/(60.0*10));
    else
	sprintf(buf, "If you don't answer within %d seconds, you will be kicked out of", me->p_cluecountdown/10);
    BERATE(buf);
    BERATE("the game and publicly humiliated.");
}

/* if other methods of getting clue words fail, then we've always got
   this list of words */
static char *fallback_cluewords[] = {
    
  /* terms: */
  "bomb", "ogg", "scum", "stoneage", "scout", "taxi", "base",
  "buttorp", "flee", "planet", "star", "warp", "impulse", "hive",
  "repair", "shipyard", "fuel", "arable", "metal", "dilithium",
  "standard", "thin", "tainted", "toxic", "phaser", "torp", "photon",
  "plasma", "missile", "fighter", "tractor", "pressor",
  
  /* what quarks are made of: */
  "satan", "beer", "jesus", "sex", "cthulhu",
  
  /* two food groups: */
  /* "grilledcheesesandwich", annoyed too many people */ "ketchup", "caffeine",
  
  /* the men: */
  /*"a fungusamongus",   people have difficulty with this one */
  "Bubbles", "Hammor", "Key", "Kaos", "Lynx",
  "Thought", "Brazilian", "Ogre",

  /* the big five: */
  "Paradise", "Arctica", "Aedile", "Eden", "Minuet",
  
  /* what you are: */
  "twink"
};

#define	NUM_CLUEWORDS	( sizeof(fallback_cluewords) \
			 / sizeof(*fallback_cluewords))

/* Once in a great while (hour?) the server demands a clue check from
   the player.  This makes sure you are paying attention. */
static void
demand_clue(void)
{
    clueword[0] = 0;
    page = -1;
    switch (configvals->cluesource) {
    case CC_MOTD:
#ifdef MOTD_SUPPORT
      parse_motd();
    
      find_suitable_motd_word();
#endif
      break;
    case CC_PHRASE_LIST_FILE:
      parse_clue_phrases();

      if (num_phrases) {
	  set_clue_word(phrases[lrand48()%num_phrases]);
      }
      break;			/* uh, NYI */
    case CC_COMPILED_IN_PHRASE_LIST:
      break;			/* that's actually the fallback case below: */
    }
    if (*clueword == 0)		/* didn't find one! */
      set_clue_word(fallback_cluewords[lrand48()%NUM_CLUEWORDS]);

    me->p_cluecountdown = configvals->cluetime * TICKSPERSEC;

    remind_cluecheck();
}

/* every tick, check the person's clue status */
void
countdown_clue(void)
{
    if (me->p_status==POUTFIT || me->p_status==PTQUEUE)
	return;
    if (me->p_cluedelay>0) {
	me->p_cluedelay--;
	if (me->p_cluedelay<1
	    && (me->p_stats.st_cluesuccess<25
		|| lrand48()%20<1)) {
	    /* uhoh, time for another cluecheck */
	    demand_clue();
	}
    } else if (me->p_cluecountdown>0) {
	char	buf[120];

	/* under the gun here */
	me->p_cluecountdown--;
	if (me->p_cluecountdown>0)
	    return;

	/* uhoh, we have a twink */

	me->p_status=PEXPLODE;
	me->p_explode=10;
	me->p_whydead= KQUIT;

	sprintf(buf, "%s (%s) was blasted out of existence due to terminal",
		me->p_name, twoletters(me));
	pmessage(buf, -1, MALL, MSERVA);
	pmessage("stupidity (failure to read messages).", -1, MALL, MSERVA);
	pmessage("Let this be a lesson to the rest of you twinks!", -1, MALL, MSERVA);
    } else {
	me->p_cluedelay = 40;
    }
}

/* the person sent themselves the message "cluecheck..." */
int
accept_cluecheck(char *word)
{
  int	i;
  char	buf[120];

  if (me->p_cluedelay>0) {
      sprintf(buf, "Don't worry %s.  You aren't under a clue check yet.",
	      me->p_name);
      BERATE(buf);
  } else if (*word) {
    for (i=0; word[i]&& clueword[i]; i++) {
      if (tolower(word[i]) != tolower(clueword[i]))
	break;
    }

    if (word[i] || clueword[i]) {
      sprintf(buf, "Nice try, %s.  Guess again.  It's not %s.",
	      me->p_name, word);
      BERATE(buf);
      BERATE("Send yourself the message `cluecheck' if you need another hint.");
    } else {
      sprintf(buf, "Good show, %s.  I won't bother you again for a while.",
	      me->p_name);
      BERATE(buf);
      me->p_cluedelay = (configvals->cluedelay/2) +
	lrand48()%(configvals->cluedelay/2);
      me->p_cluedelay *= TICKSPERSEC;

      me->p_stats.st_cluesuccess++;
    }
  } else {
    remind_cluecheck();
  }
  return 1;
}

/**********************************************************************/

#endif /* CLUECHECK1 */

#ifdef CLUECHECK2

/* ---------[ CLUECHECK2 -> Brandons hacked version of CLUECHECK1 ]--------- */

/*
// I munged this (sorry Rob).  Using me->p_cluecountdown:
//     if it is 0 you have not been checked yet
//     if it is 1 your time is up.
//     if it is -1 you should not be checked.
//     if it is anything greater than one you are under the timer
*/

/* -------------------------[ Globals (eep) ]------------------------- */

#define TellLINE { pmessage("", me->p_no, MINDIV, "***! Cluecheck !**! Cluecheck !**! Cluecheck !**! Cluecheck !**! Cluecheck !***"); }

#define NUM_CLUEWORDS    (sizeof(fallback_cluewords) / \
                          sizeof(*fallback_cluewords))
#define CLUE_GETOUTOFIT 7

static char           **phrases=0;
static int            num_phrases=0;
static char           clueword[40];

/*
// if other methods of getting clue words fail, then we've always got
// this list of words
*/

static char *fallback_cluewords[] = {
  /* make it simple, if they want more they can make a .cluecheck file */
  "bomb", "ogg", "scum", "stoneage", "scout", "taxi", "base",
  "buttorp", "flee", "planet", "star", "warp", "impulse", "hive",
  "repair", "shipyard", "fuel", "arable", "metal", "dilithium",
  "standard", "thin", "tainted", "toxic", "phaser", "torp", "photon",
  "plasma", "missile", "fighter", "tractor", "pressor",
};

/* -------------------------[ Functions ]------------------------- */

static void
set_clue_word(char *word)
{
  strncpy(clueword, word, sizeof(clueword));
  clueword[sizeof(clueword)-1] = 0;
}

static void
parse_clue_phrases(void)
{
    char    *s;
    int    size;

    if (phrases)
    return;

    phrases = (char**)malloc(sizeof(*phrases)*(size=20));

    for (s=cluephrase_storage; *s; s += strlen(s)+1) {
    phrases[num_phrases] = s;
    num_phrases++;
    if (num_phrases>=size)
        phrases = (char**)realloc(phrases, sizeof(*phrases)*(size*=2));
    }

    phrases = (char**)realloc(phrases, sizeof(*phrases)*num_phrases);
}

/*
// print the message that tells the person how to respond to the clue check
*/
static void
remind_cluecheck(void)
{
    char    buf[120];

    pmessage("", me->p_no, MINDIV, "*******************************************************************************");
    TellLINE;
    sprintf(buf, "            Send yourself: \"cluecheck %s\"", clueword);
    pmessage(buf, me->p_no, MINDIV, "");

    if (me->p_cluecountdown>(60*TICKSPERSEC)) {
        sprintf(buf,
                "  Answer within %.2g minutes or be ejected from the game.",
                (me->p_cluecountdown/(60.0*TICKSPERSEC)));
        pmessage(buf, me->p_no, MINDIV, "");
    } else {
        sprintf(buf,
                "** ANSWER ** within %d seconds or be ejected from the game.",
                (me->p_cluecountdown/10));
        pmessage(buf, me->p_no, MINDIV, "");
    }
    pmessage("", me->p_no, MINDIV, "*******************************************************************************");

}

/* called the first time */
static void
demand_clue(void)
{
    char    buf[255];
    char    syou[32];
    clueword[0] = 0;

    sprintf(syou, "%s->YOU", SERVNAME);
    /* higher than rank CLUE_GETOUTOFIT  get ... out of it */
    if (me->p_stats.st_rank > CLUE_GETOUTOFIT  /* if my rank is high enough */
        || me->p_stats.st_royal == GODLIKE + 1 /* or I'm Q */ ) {
        TellLINE;
        pmessage("Due to your ranking status, I will let you off the hook.",
                 me->p_no, MINDIV, syou);
        me->p_cluecountdown = -1;
        return;
    } else if (me->p_stats.st_cluesuccess > configvals->cluecheck) {
        TellLINE;
        sprintf(buf, "You have passed the cluecheck the required %d times.",
                configvals->cluecheck);
        pmessage(buf, me->p_no, MINDIV, syou);
        me->p_cluecountdown = -1;
        return;
    }

    parse_clue_phrases();

    if (num_phrases)
        set_clue_word(phrases[lrand48()%num_phrases]);
    if (*clueword == 0)                  /* didn't find one! */
      set_clue_word(fallback_cluewords[lrand48()%NUM_CLUEWORDS]);
    me->p_cluecountdown = configvals->cluetime * TICKSPERSEC;

    remind_cluecheck();
}


/* every tick, check the person's clue status */
void
countdown_clue(void)
{
    if (me->p_cluecountdown != -1) {
        if (me->p_status==POUTFIT
            || me->p_status==PTQUEUE
            || me->p_status==POBSERVE)
           return;

        /* let bases off the hook */
        if (me->p_ship.s_type==5 || me->p_ship.s_type==9)
            return;

        if (me->p_cluedelay > 0) {
            me->p_cluedelay--;
            return;
        }

        /* is it greater than one? */
        if (me->p_cluecountdown > 0) {
            char    buf[255];

            /* under the gun here */
            me->p_cluecountdown--;

            if (me->p_cluecountdown == 150*TICKSPERSEC
                || me->p_cluecountdown == 60*TICKSPERSEC
                || me->p_cluecountdown == 10*TICKSPERSEC) {
                remind_cluecheck();
                return;
            }

            /* is it still greater than 1? */
            else if (me->p_cluecountdown>0)
                return;

            /* uhoh, we have a twink */
            me->p_status=PEXPLODE;
            me->p_explode=10;
            me->p_whydead= KQUIT;
            me->p_cluecountdown = -1;

            sprintf(buf, "%s (%s) was ejected for failing a cluecheck.",
                    me->p_name, twoletters(me));
            pmessage(buf, SERVNAME, MALL, " ** CLUECHECK **");
        } else {
            /* they aren't -1, they are greater than 1, so they havn't
               been checked yet */
            demand_clue();
        }
    }
}


/*
// the person sent themselves the message "cluecheck..."
// called in controls (message.c)
*/
int
accept_cluecheck(char *word)
{
    int     i;
    char    buf[120];
    char    syou[32];

    sprintf(syou, "%s->YOU", SERVNAME);
    if (me->p_cluecountdown == -1 || !me->p_cluecountdown) {
        pmessage("You are not under a cluecheck.",
                 me->p_no, MINDIV, syou);
        return;
    } else if (*word) {
        for (i=0; word[i]&& clueword[i]; i++) {
            if (tolower(word[i]) != tolower(clueword[i]))
            break;
        }

        if (word[i] || clueword[i]) {
            sprintf(buf, "Nice try, guess again.  It is not \"%s\".", word);
            pmessage(buf, me->p_no, MINDIV, syou);
            pmessage(
               "Send the message \"cluecheck\" to yourself for another hint.",
               me->p_no, MINDIV, syou);
        } else {
            sprintf(buf,
                    "Good show %s.  I won't bother you again for a while.",
                    me->p_name);
            pmessage(buf, me->p_no, MINDIV, syou);
            me->p_stats.st_cluesuccess++;
            me->p_cluecountdown = -1;
        }
    } else {
        remind_cluecheck();
    }
    sprintf(buf, "You have passed the cluecheck %d times.",
            me->p_stats.st_cluesuccess);
    pmessage(buf, me->p_no, MINDIV, syou);
    return 1;
}

#endif  /* CLUECHECK2 */

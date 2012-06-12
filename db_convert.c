#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <string.h>

// Taken from src/include/struct.h and adapted to be compatible with a
// database built on a 32-bit machine.
struct stats {
    int32_t st_genocides;	/* number of genocides participated in */
    float   st_tmaxkills;	/* max kills ever */
    float   st_di;		/* total destruction inflicted for all time */
    int32_t st_tkills;		/* Kills in tournament play */
    int32_t st_tlosses;		/* Losses in tournament play */
    int32_t st_tarmsbomb;	/* Tournament armies bombed */
    int32_t st_tresbomb;	/* resources bombed off */
    int32_t st_tdooshes;	/* armies killed while being carried */
    int32_t st_tplanets;	/* Tournament planets conquered */
    int32_t st_tticks;		/* Tournament ticks */
    int32_t st_sbkills;		/* Kills as starbase */
    int32_t st_sblosses;	/* Losses as starbase */
    int32_t st_sbticks;		/* Time as starbase */
    float   st_sbmaxkills;	/* Max kills as starbase */
    int32_t st_wbkills;		/* Kills as warbase */
    int32_t st_wblosses;	/* Losses as warbase */
    int32_t st_wbticks;		/* Time as warbase */
    float   st_wbmaxkills;	/* Max kills as warbase */
    int32_t st_jsplanets;	/* planets assisted with in JS */
    int32_t st_jsticks;		/* ticks played as a JS */
    int32_t st_lastlogin;	/* Last time this player was played */
    int32_t st_flags;		/* Misc option flags */
    int32_t st_cluesuccess;	/* how many times you passed a clue check */
    char    st_pad[92];		/* space for expansion */
    int32_t st_rank;		/* Ranking of the player */
    int32_t st_royal;		/* royaly, specialty, rank */
} __attribute__((__packed__));

// Taken from src/include/struct.h and adapted to be compatible with a
// database built on a 32-bit machine.
struct statentry {
    char    name[16];		/* player's name */
    char    password[16];	/* player's password */
    struct stats stats;		/* player's stats */
} __attribute__((__packed__));

void print_stats( struct stats *st ) {
	printf(
		"struct stats (%p) {\n"
		"\t\t.st_genocides = %" PRId32 ",\n"
		"\t\t.st_tmaxkills = %f,\n"
		"\t\t.st_di = %f,\n"
		"\t\t.st_tkills = %" PRId32 ",\n"
		"\t\t.st_tlosse = %" PRId32 ",\n"
		"\t\t.st_tarmsbomb = %" PRId32 ",\n"
		"\t\t.st_tresbomb = %" PRId32 ",\n"
		"\t\t.st_tdooshes = %" PRId32 ",\n"
		"\t\t.st_tplanets = %" PRId32 ",\n"
		"\t\t.st_tticks = %" PRId32 ",\n"
		"\t\t.st_sbkills = %" PRId32 ",\n"
		"\t\t.st_sblosses = %" PRId32 ",\n"
		"\t\t.st_sbticks = %" PRId32 ",\n"
		"\t\t.st_sbmaxkills = %f,\n"
		"\t\t.st_wbkills = %" PRId32 ",\n"
		"\t\t.st_wblosses = %" PRId32 ",\n"
		"\t\t.st_wbticks = %" PRId32 ",\n"
		"\t\t.st_wbmaxkills = %f,\n"
		"\t\t.st_jsplanets = %" PRId32 ",\n"
		"\t\t.st_jsticks = %" PRId32 ",\n"
		"\t\t.st_lastlogin = %" PRId32 ",\n"
		"\t\t.st_flags = %" PRId32 ",\n"
		"\t\t.st_cluesuccess = %" PRId32 ",\n"
		"\t\t.st_rank = %" PRId32 ",\n"
		"\t\t.st_royal = %" PRId32 "\n"
		"\t}\n",
		st,
		st->st_genocides,
		st->st_tmaxkills,
		st->st_di,
		st->st_tkills,
		st->st_tlosses,
		st->st_tarmsbomb,
		st->st_tresbomb,
		st->st_tdooshes,
		st->st_tplanets,
		st->st_tticks,
		st->st_sbkills,
		st->st_sblosses,
		st->st_sbticks,
		st->st_sbmaxkills,
		st->st_wbkills,
		st->st_wblosses,
		st->st_wbticks,
		st->st_wbmaxkills,
		st->st_jsplanets,
		st->st_jsticks,
		st->st_lastlogin,
		st->st_flags,
		st->st_cluesuccess,
		st->st_rank,
		st->st_royal
	);
}

void print_statentry( struct statentry *st ) {
		printf(
			"struct statentry (%p) {\n"
			"\t.name = \"%s\",\n"
			"\t.password = \"%s\",\n"
			"\t.stats = ",
			st, st->name, st->password
		);
		print_stats(&st->stats);
		printf("}\n");
}

int main( int argc, const char *argv[] ) {
	// The database's floats are 4 bytes.
	assert(sizeof(float) == 4);
	if( argc != 3 ) {
		fprintf(stderr, "Usage: %s db_in db_out\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	FILE *db = fopen(argv[1], "r");
	if( db == NULL ) {
		perror("fopen");
		exit(EXIT_FAILURE);
	}
	FILE *out = fopen(argv[2], "w");
	if( out == NULL ) {
		perror("fopen");
		exit(EXIT_FAILURE);
	}
#if 0
	printf("sizeof(struct statentry) = %zd\n", sizeof(struct statentry));
	printf("sizeof(struct stats) = %zd\n", sizeof(struct stats));
#endif

	while( true ) {
		struct statentry st;
		memset(&st, 0, sizeof(struct statentry));
		size_t count = fread(&st, sizeof(struct statentry), 1, db);
		if( count < 1 ) {
			if( feof(db) ) {
				if( count == 0 ) {
					break;
				} else {
					fprintf(stderr, "Short read\n");
					exit(EXIT_FAILURE);
				}
			} else if( ferror(db) ) {
				perror("fread");
				exit(EXIT_FAILURE);
			} else {
				assert(false);
			}
		}
		st.stats.st_genocides = htonl(st.stats.st_genocides);
		st.stats.st_tmaxkills = htonl(st.stats.st_tmaxkills);
		st.stats.st_di = htonl(st.stats.st_di);
		st.stats.st_tkills = htonl(st.stats.st_tkills);
		st.stats.st_tlosses = htonl(st.stats.st_tlosses);
		st.stats.st_tarmsbomb = htonl(st.stats.st_tarmsbomb);
		st.stats.st_tresbomb = htonl(st.stats.st_tresbomb);
		st.stats.st_tdooshes = htonl(st.stats.st_tdooshes);
		st.stats.st_tplanets = htonl(st.stats.st_tplanets);
		st.stats.st_tticks = htonl(st.stats.st_tticks);
		st.stats.st_sbkills = htonl(st.stats.st_sbkills);
		st.stats.st_sblosses = htonl(st.stats.st_sblosses);
		st.stats.st_sbticks = htonl(st.stats.st_sbticks);
		st.stats.st_sbmaxkills = htonl(st.stats.st_sbmaxkills);
		st.stats.st_wbkills = htonl(st.stats.st_wbkills);
		st.stats.st_wblosses = htonl(st.stats.st_wblosses);
		st.stats.st_wbticks = htonl(st.stats.st_wbticks);
		st.stats.st_wbmaxkills = htonl(st.stats.st_wbmaxkills);
		st.stats.st_jsplanets = htonl(st.stats.st_jsplanets);
		st.stats.st_jsticks = htonl(st.stats.st_jsticks);
		st.stats.st_lastlogin = htonl(st.stats.st_lastlogin);
		st.stats.st_flags = htonl(st.stats.st_flags);
		st.stats.st_cluesuccess = htonl(st.stats.st_cluesuccess);
		st.stats.st_rank = htonl(st.stats.st_rank);
		st.stats.st_royal = htonl(st.stats.st_royal);

#if 0
		printf("sizeof(float)=%zu\n", sizeof(float));
		printf("sizeof(long)=%zu\n", sizeof(long));
		printf("sizeof(int)=%zu\n", sizeof(int));
#endif
		print_statentry(&st);

		if( fwrite(&st, sizeof(struct statentry), 1, out) != 1 ) {
			perror("fwrite");
			exit(EXIT_FAILURE);
		}
	}

	fclose(db);
	fclose(out);

	return EXIT_SUCCESS;
}

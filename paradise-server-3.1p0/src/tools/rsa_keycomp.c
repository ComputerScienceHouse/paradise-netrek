/*
 * keycomp.c
 * Compile a "termcap"-like description of RSA keys into the .reserved 
 * RSA binary file.
 */

#include "config.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include "defs.h"
#include "struct.h"

/* ONLY IF YOU HAVE  char key_name[2*KEYSIZE] in struct rsa_key. */
/* #define RSA_KEYNAME */
/* comment out as needed */

#define RSA_KEY_EXCLUDE_FILE	"etc/rsa-exclude"
#define RSA_MOTDLIST		"rsa_motdlist"

#define SEP			':'

#define CLIENT_TYPE_FIELD	"ct="
#define CREATOR_FIELD		"cr="
#define CREATED_FIELD           "cd="
#define ARCH_TYPE_FIELD		"ar="
#define COMMENTS_FIEILD		"cm="
#define GLOBAL_KEY_FIELD	"gk="
#define PUBLIC_KEY_FIELD	"pk="

struct  motd_keylist { 
   char *client;
   int	num_arch;
   char **arch;
};

/* rsa_keycomp.c */
void backup P((char *f));
struct rsa_key *comp_key P((FILE *fi, int *err));
int read_key P((FILE *fi, char *bp));
int kgetkeyname P((char *src, char *dest));
int kgetstr P((char *src, char *field, char *dest));
int kgetkey P((char *src, char *field, unsigned char *dest));
char **read_exclude_file P((char *n));
int excluded P((char *keyname));
void sort_motdlist P((int nk));
void store_keydesc P((struct motd_keylist *keys, char *client, char *arch));
void usage P((char *p));

int 	line=1;
char	*keyf = NULL;
char	*reservedf = RSA_KEY_FILE;
char	*excludef = RSA_KEY_EXCLUDE_FILE;
char	*motdlist = RSA_MOTDLIST;
char	**exclude_list = NULL;

int
main(int argc, char **argv)
{
   FILE				*fi, *fo, *motd_fo;
   register struct rsa_key	*k;
   register int			i, c;
   int				err;

   i=1;
   while(i < argc){
      if(strcmp(argv[i], "-e")==0){
	 if(++i < argc)
	    excludef = argv[i];
	 else
	    usage(argv[0]);
      }
      else if(strcmp(argv[i], "-o")==0){
	 if(++i < argc)
	    reservedf = argv[i];
	 else
	    usage(argv[0]);
      }
      else if(strcmp(argv[i], "-m")==0){
	 if(++i < argc)
	    motdlist = argv[i];
	 else
	    usage(argv[0]);
      }
      keyf = argv[i++];
   }
   if(!keyf) usage(argv[0]);

   exclude_list = read_exclude_file(excludef);
   
   if(!(fi=fopen(keyf, "r"))){
      perror(keyf);
      exit(1);
   }

   backup(reservedf);

   if(!(fo=fopen(reservedf, "w"))){
      perror(reservedf);
      exit(1);
   }
   if(!(motd_fo=fopen(motdlist, "w"))){
      perror(motdlist);
      exit(1);
   }

   err = 0;
   c=0;

   while(!err){
      k=comp_key(fi, &err);
      if(k){
	 c++;
	 if(fwrite((char *)k, sizeof(struct rsa_key), 1, fo) != 1){
	    perror("fwrite");
	    exit(1);
	 }
	 fprintf(motd_fo, "\"%s\" \"%s\"\n", k->client_type,
					     k->architecture);
	 free((char *)k);
      }
   }
   
   fclose(fi);
   fclose(fo);
   fclose(motd_fo);

   sort_motdlist(c);

   printf("%s: compiled %d keys to \"%s\".\n", argv[0], c, reservedf);
}

void
backup(char *f)
{
   char		*backup_f;
   struct stat	sbuf;
   
   if(stat(f, &sbuf) < 0)
      return;
   backup_f = (char *)malloc(strlen(f)+2);
   if(!backup_f) { perror("malloc"); exit(1); }
   sprintf(backup_f, "%s~", f);
   (void)link(f, backup_f);
   (void)unlink(f);
   free((char *)backup_f);
}

/*
 * Read an ascii key representation from the current position in the
 * input file and create and return a struct rsa_key.
 */

struct rsa_key *
comp_key(FILE *fi, int *err)
{
   char			ibuf[BUFSIZ], obuf[BUFSIZ];
   struct rsa_key	*key;
   char			keyname[2*KEY_SIZE];

   *err = 0;

   if(read_key(fi, ibuf) <= 0){
      *err = 1;	/* not necessarily an error but we want the program to quit */
      return NULL;
   }

   key = (struct rsa_key *)malloc(sizeof(struct rsa_key));
   if(!key) { perror("malloc"); exit(1); }

   if(!kgetkeyname(ibuf, keyname)){
      fprintf(stderr, "%s: No key name for entry around line %d\n",
	 keyf, line);
      *err = 1;
      return NULL;
   }
   if(excluded(keyname)){
      printf("key \"%s\" excluded (found in \"%s\").\n", keyname, excludef);
      return NULL;
   }
   /*
   printf("read key \"%s\"\n", keyname);
   */
#ifdef RSA_KEYNAME
   strncpy(key->key_name, keyname, 2*KEY_SIZE-1);
   key->key_name[2*KEY_SIZE-1] = 0;
#endif

   /* could use tgetstr here but then we'd have to link with libtermlib */

   if(!kgetstr(ibuf, CLIENT_TYPE_FIELD, obuf)){
      fprintf(stderr, "%s: No client type given for key %s at line %d\n",
	 keyf, keyname, line);
      *err = 1;
      return NULL;
   }
   strncpy(key->client_type, obuf, 31);
   key->client_type[31] = '\0';

   if(!kgetstr(ibuf, ARCH_TYPE_FIELD, obuf)){
      fprintf(stderr, "%s: No architecture given for key %s at line %d\n",
	 keyf, keyname, line);
      *err = 1;
      return NULL;
   }
   strncpy(key->architecture, obuf, 31);
   key->architecture[31] = '\0';

   if(!kgetkey(ibuf, GLOBAL_KEY_FIELD, key->global)){
      fprintf(stderr, "%s: No global key given for key %s at line %d\n",
	 keyf, keyname, line);
      *err = 1;
      return NULL;
   }

   if(!kgetkey(ibuf, PUBLIC_KEY_FIELD, key->public)){
      fprintf(stderr, "%s: No public key given for key %s at line %d\n",
	 keyf, keyname, line);
      *err = 1;
      return NULL;
   }

   return key;
}

int
read_key(FILE *fi, char *bp)
{
   register char	*cp;
   register int		c;

   do {
      cp = bp;
      while((c=getc(fi))!=EOF){
	 if(c == '\n'){
	    line ++;
	    while(cp > bp && isspace(cp[-1])) cp--;
	    if(cp > bp && cp[-1] == '\\'){
	       cp--;
	       continue;
	    }
	    break;
	 }
	 if(cp >= bp+BUFSIZ){
	    fprintf(stderr, "%s: entry exceeded %d chars at line %d\n",
	       keyf, BUFSIZ, line);
	    return -1;
	 }
	 else
	    *cp++ = c;
      }
   } while(bp[0] == '#' && c != EOF);
   *cp = 0;

   return c!= EOF;
}

/*
 * Extract the key name descriptor from an entry buffer
 * "Key Name:"
 */

int
kgetkeyname(char *src, char *dest)
{
   int	l = (int) (strchr(src,SEP)-src);
   strncpy(dest, src, l);
   dest[l] = 0;
   return 1;
}

/*
 * Place contents of specified field entry into destination string.
 * "pk=Text Text Text:"
 */

int
kgetstr(char *src, char *field, char *dest)
{
   char	*s = strstr(src, field);
   int	l;
   if(!s)
      return 0;
   
   s += strlen(field);
   l = (int ) (strchr(s,SEP)-s);
   strncpy(dest, s, l);
   dest[l] = 0;
   return 1;
}

/*
 * Place contents of specified binary field entry into destination string.
 * "pk=Text Text Text:"
 */

int
kgetkey(char *src, char *field, unsigned char *dest)
{
   char			unencoded_dest[KEY_SIZE*2+1],
			uc[3];
   register char	*s;
   unsigned int		c;

   if(!kgetstr(src, field, unencoded_dest))
      return 0;
   
   /* convert encoded binary to binary */
   s = unencoded_dest;
   while(*s){
      uc[0] = *s++;
      uc[1] = *s++;
      uc[2] = 0;
      sscanf(uc, "%x", &c);
      *dest++ = (unsigned char)c;
   }
   return 1;
}

char **
read_exclude_file(char *n)
{
   char		buf[80];
   char		**s, *nl;
   register	l;
   FILE		*fi = fopen(n, "r");
   if(!fi){
      /*
      perror(n);
      */
      return NULL;
   }

   l=0;
   while(fgets(buf, 79, fi))
      l++;
   fclose(fi);
   s = (char **) malloc(sizeof(char *) * (l+1));
   if(!s) { perror("malloc"); exit(1); }
   fi = fopen(n, "r");
   l = 0;
   while(fgets(buf, 79, fi)){
      if((nl = strrchr(buf, '\n')))
	 *nl = 0;
      s[l++] = strdup(buf);
   }
   s[l] = NULL;
   fclose(fi);
   return s;
}

int
excluded(char *keyname)
{
   register char	**s;

   if(!exclude_list) return 0;

   for(s=exclude_list; *s; s++){
      if(strcmp(*s, keyname)==0)
	 return 1;
   }
   return 0;
}

   /* inefficient but what the heck.. */

void
sort_motdlist(int nk)
{
   char			buf[80];
   struct  motd_keylist *keys, *k;
   char			client[80], arch[80];
   register		i;
   FILE			*fi = fopen(motdlist, "r"), *fo;

   if(!fi) { perror(motdlist); return; }

   keys = (struct motd_keylist *) malloc(sizeof(struct motd_keylist)*nk);
   bzero(keys, sizeof(struct motd_keylist) * nk);
   for(i=0; i< nk; i++){
      keys[i].arch = (char **) malloc(sizeof(char *) * nk);
      bzero(keys[i].arch, sizeof(char *) * nk);
   }

   while(fgets(buf, 79, fi)){
      if(sscanf(buf, "\"%[^\"]\" \"%[^\"]", client, arch) != 2)
	 abort();	/* DEBUG */
      store_keydesc(keys, client, arch);
   }
   fclose(fi);

   fo = fopen(motdlist, "w");
   if(!fo) { perror(motdlist); return; }
   for(k=keys; k-keys<nk && k->client; k++){
      fprintf(fo, 
      "------------------------[   %s   ]------------------------\n",
      k->client);
      for(i=0; i< k->num_arch; i ++){
	 fprintf(fo, "%-36s", k->arch[i]);
	 i++;
	 if(i < k->num_arch){
	    fprintf(fo, "%s\n", k->arch[i]);
	 }
	 else
	    fprintf(fo, "\n");
      }
   }
   fclose(fo);
}

void
store_keydesc(struct motd_keylist *keys, char *client, char *arch)
{
   register struct motd_keylist	*k;

   for(k=keys; k->client; k++){
      if(strcmp(k->client, client)==0){
	 k->arch[k->num_arch] = strdup(arch);
	 k->num_arch++;
	 return;
      }
   }
   k->client = strdup(client);
   k->arch[0] = strdup(arch);
   k->num_arch++;
}

void
usage(char *p)
{
   fprintf(stderr, 
      "usage: %s [-e excludefile] [-m motdlist] [-o reservedfile] keycap\n",
      p);
   exit(0);
}


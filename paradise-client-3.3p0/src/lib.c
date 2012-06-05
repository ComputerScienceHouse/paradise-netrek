#include "config.h"
#include "conftime.h"
#include <ctype.h>
#include "str.h"

/* common subroutines + library function replacements for a few functions
   that systems may not have */

char *
strupper(char *s)
{
  for(; *s; s++)
    *s = toupper(*s);
  return(s);
}

char *
strlower(char *s)
{
  for(; *s; s++)
    *s = tolower(*s);
  return(s);
}

#ifndef HAVE_STRDUP
char *
strdup(const char *p)
{
  char *x, *q = p;

  while(*q++)
    ;

  x = malloc(p - q + 1);
  q = p;
  while(*x++ = *q++)
    ;

  return(x);
}
#endif /* HAVE_STRDUP */

#ifndef HAVE_GETTIMEOFDAY
int
gettimeofday(struct timeval *tp, struct timezone *unused)
{
  tp->tv_sec = time(NULL);
  tp->tv_usec = 0;
  return(0);
}
#endif /* HAVE_GETTIMEOFDAY */

#ifndef HAVE_STRSTR
char *
strstr(const char *s1, const char *s2)
{
  char *s;
  int length;

  length = strlen(s2);
  for(s = s1; *s; s++)
  {
    if(*s == *s2 && strncmp(s, s2, length) == 0)
      return(s);
  }
  return(NULL);
}
#endif

#ifndef HAVE_UNAME
int
uname(struct utsname *name)
{
/* 
   example code to fill this in properly:
   #ifdef ARCH
     strcpy(name->sysname, "MyArch");
     strcpy(name->release, "12.34");
     return(0);
   #endif
*/
  strcpy(name->sysname, "Generic");
  strcpy(name->release, "");
  return(0);
}
#endif


/* tools.c - shell escape, graphic toolsw - 10/10/93
 * 
 * copyright 1993 Kurt Siegl <siegl@risc.uni-linz.ac.at> Free to use, hack, etc.
 * Just keep these credits here. Use of this code may be dangerous to your
 * health and/or system. Its use is at your own risk. I assume no
 * responsibility for damages, real, potential, or imagined, resulting  from
 * the use of it.
 * 
 */

#include "config.h"
#include <stdio.h>
#include <signal.h>
#include "str.h"

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"

void showToolsWin P((void));

void
sendTools(char *str)
{
  char    pipebuf[100];
  FILE   *pipefp;
  int     len;

  if (!W_IsMapped(toolsWin))
    showToolsWin();
  signal(SIGCHLD, SIG_DFL);
  if (shelltools && (pipefp = popen(str, "r")) != NULL)
    {
      while (fgets(pipebuf, 80, pipefp) != NULL)
	{
	  len = strlen(pipebuf);
	  if (pipebuf[len - 1] == '\n')
	    pipebuf[len - 1] = '\0';
	  W_WriteText(toolsWin, 0, 0, textColor, pipebuf,
		      strlen(pipebuf), W_RegularFont);
	}
      pclose(pipefp);
    }
  else
    W_WriteText(toolsWin, 0, 0, textColor, str, strlen(str), W_RegularFont);
}

void
showToolsWin(void)
{
  if (W_IsMapped(toolsWin))
    W_UnmapWindow(toolsWin);
  else
    W_MapWindow(toolsWin);
}

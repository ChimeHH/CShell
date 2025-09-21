/*----------------------------------------------------------
File Name  : xxx.c/.h
Description:
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>


#include "cshell_def.h"


void trim_right(char *strToTrim)
{
  register char *    strCursor = NULL;    /* string cursor */

  strCursor = strToTrim + strlen(strToTrim) - 1;

  while (strCursor > strToTrim)
  {
    if (isspace((long)(*strCursor)))
      strCursor--;
    else
      break;
  }

  if (strCursor == strToTrim)
  {

    if (isspace((long)(*strCursor)))   /* whole string is white space */
    {
      *strCursor = EOS;
      return;
    }
  }

  /* Normal return, non-empty string */
  *(strCursor+1) = EOS;

  return;
}

int run_cmd(const char* input)
{
  static int count = 0;
  
  char inLine [MAX_SHELL_LINE + 1];
  strcpy(inLine, input);

  trim_right(inLine);  
  strcat(inLine, "\n");

  printf("\n$%d/> %s\n", ++count, inLine);
  read_cmd(inLine);
  yyparse();
  end_cmd();

  return 0;
}


long run_script(const char* input, int *ex, int *sk)
{
  int i;
  int status = -1;

  if(*ex) return -1;

  for (i = 0; isspace(input[i]); i++);

  if (input[i] != '#' && input [i] != EOS)
  { 
    if(!strcmp(&input[i], "exit")) { *ex=1; }
    else if(!strcmp(&input[i], "=pod")) { *sk=1; } 
    else if(!strcmp(&input[i], "=cut")) { *sk=0; }

    if(!*ex && !*sk)
    {
      status = run_cmd(&input [i]);
    }
  }

  return (status);
}


int shell_call(char *cmd)
{  
  int len;
  static char input[MAX_SHELL_LINE];

  loadsymbols();

  while(' ' == *cmd) cmd++;

  strncpy(input, cmd, MAX_SHELL_LINE);

  trim_right(input);
  
  len = strlen(input);
  if(len < 1) return 0;
    
  if(input[0] == 0x1b) return 0; //ESC

  if(input[0]=='<')
  {      
    FILE *fp;
    char fname[64];
  
    strncpy(fname, &input[1], 63);
    fname[63] = 0;

    if((fp=fopen(fname,"rt"))==NULL)
    {
      printf("Cannot open input file!\n");
      return -1;
    }
    int ex = 0;
    int sk = 0;
    while (fgets(input,1024,fp) !=NULL)
    {      
      if(strlen(input) < 1) continue;    
      run_script(input, &ex, &sk);
      input[0] = 0;        
    }
  
    fclose(fp);
  }
  else
  {
    run_cmd(input);
    input[0] = 0;
  }

  return 0;
}


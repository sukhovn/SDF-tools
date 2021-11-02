%{
/* lexer for get_param */
/* $Header: /home/cvs/rnpl/src/gpar.l,v 1.1.1.1 2013/07/09 00:38:27 cvs Exp $ */
/* Copyright (c) 1998 by Robert Marsa */

/* MWC: 2001/05/16:  Changed gpar_flush_buffer -> yy_flush_buffer */
/* MWC: 2003/07/22:  Changed yy_current_buffer -> YY_CURRENT_BUFFER */
/* MWC: 2008/05/15:  Added default rule to swallow unparsed characters,
                     which otherwise were getting echoed to stdout! */

#include <stdlib.h>
#include <string.h>
#include "gpar.h"
#include "gpar.tab.h"

int my_gparinput(char *buf, int max_size);

#undef YY_INPUT
#define YY_INPUT(b,r,ms) (r=my_gparinput(b,ms))

#undef gparwrap

%}

word ([a-zA-Z_][a-zA-Z_0-9]*)

%%
%{
  extern int sg_param_type;
  extern int first_tok;
  
  if(first_tok){
    first_tok=0;
    if(YY_CURRENT_BUFFER) yy_flush_buffer(YY_CURRENT_BUFFER); 
    return sg_param_type;
  }
  
%}


[ \t] ; /* ignore whitespace */

^[^=]+$ ; /* ignore lines without = */

\n ;

^[ \t]*#.*$ ; /* ignore comments */

(:=) {
  gparlval.inum=ASSIGNOP;
  return ASSIGNOP;
}

"=" {
  gparlval.inum=EQUALS;
  return EQUALS;
}

"-" {
  gparlval.inum=MINUS;
  return MINUS;
}

"*" {
  gparlval.inum=TIMES;
  return TIMES;
}

\/ {
  gparlval.inum=DIVIDE;
  return DIVIDE;
}

(\[) {
  gparlval.inum=OBRACK;
  return OBRACK;
}

(\]) {
  gparlval.inum=CBRACK;
  return CBRACK;
}

(\,) {
  gparlval.inum=COMMA;
  return COMMA;
}

([0-9]+) {
  gparlval.inum = atoi((const char *)gpartext);
  return INUM;
}

(([0-9]*)(\.[0-9]+)?([eE][+-]?[0-9]+)?) {
  sscanf((const char *)gpartext,"%lf",&gparlval.num);
  return NUM;
}

{word}((\.{word})|("->"{word}))* {
  gparlval.str=(char *)malloc(strlen((const char *)gpartext)+1);
  strcpy(gparlval.str,(const char *)gpartext);
  return IDEN;
}

(\"[^\"\n]*\") {
  gparlval.str = (char *)malloc(strlen((const char *)gpartext)-1);
  strncpy(gparlval.str,(const char *)gpartext+1,strlen((const char *)gpartext)-2);
  gparlval.str[strlen((const char *)gpartext)-2]=0;
  return STR;
}

"<<"{word} {
  gparlval.str=(char *)malloc(strlen((const char *)gpartext-1));
  strcpy(gparlval.str,(const char *)gpartext+2);
  return FNAME;
}

. ; /* Swallow everything else; otherwise chars are echoed to stdout */

%%

extern char *sg_param_ptr, *sg_param_end;

int my_gparinput(char *buf, int max_size)
{
  int n;
  
  n=(max_size < (sg_param_end-sg_param_ptr)) ? max_size : (sg_param_end-sg_param_ptr);
  if(n>0){
    memcpy(buf,sg_param_ptr,n);
    sg_param_ptr+=n;
  }
  return n;
}

extern int inputerror;

void gparerror(const char *msg)
{
  /*fprintf(stderr,"GPARERROR: %s at '%s'\n",msg,gpartext);*/
  inputerror=1;
}

int gparwrap()
{
  return 1;
}

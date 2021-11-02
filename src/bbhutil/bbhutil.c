/* $Header: /home/cvs/rnpl/src/bbhutil.c,v 1.2 2014/06/12 18:13:01 cvs Exp $ */
/* HDF support routines */
/* parameter fetching routines */
/* These routines are written by Robert L. Marsa */
/* Copyright (c) 1995,1996 by Robert L. Marsa */

/* Modified by Matthew W Choptuik, Dec 1996 to use int32 types  */
/* consistently in interaction with SD interface (netcdf).      */
/* Conversion routines bbh_int32_copy_int, etc. added. */

/* Modified by Matthew W Choptuik, Aug 1997 to use (char *) casts */
/* on strdup() calls to appease 'xlc'.                            */

/* Modified by Matthew W Choptuik, Sep 2003 so that get_param and  */
/* get_param_nc save state (parameter file name and file handle)   */
/* to minimize amount of file open/closes, particularly in context */
/* of massively parallel execution.                                */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef HAVE_HDF
#include <mfhdf.h>
#endif
#include <stdarg.h>
#include <errno.h>
#include <bbhutil.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <regex.h>

/* For communication with gpar.tab.c for parsing assignments of vectors */
/* with arbitrary number of elements. */

void **Gpar_pr = (void **) NULL;
int  *Gpar_prsize = (int *) NULL;

#ifdef HAVE_HDF
void bbh_int32_copy_int(int32 *in,int *out,int n) {
   int   i;
   for( i = 0; i < n; i++ ) out[i] = in[i];
}

void bbh_intn_copy_int(intn *in,int *out,int n) {
   int   i;
   for( i = 0; i < n; i++ ) out[i] = in[i];
}

void bbh_int_copy_int32(int *in,int32 *out,int n) {
   int   i;
   for( i = 0; i < n; i++ ) out[i] = in[i];
}

void bbh_int_copy_intn(intn *in,intn *out,int n) {
   int   i;
   for( i = 0; i < n; i++ ) out[i] = in[i];
}
#endif

char *lowcase(char *s)
{
   char *q;

   for(q=(char *)s;*q;q++)
      *q=tolower(*q);
   return s;
}

/* MWC: 2014-05-11 ... Change sense of logic so that users don't need 
   to specify -DHAVE_STRDUP in the usual case that system has strdup.
*/

#ifdef DONT_HAVE_STRDUP
#ifndef _ALL_SOURCE
char *strdup(const char *s)
#else
char *strdup(char *s)
#endif
{
   char *t;
   
   if(s){
      t=(char *)malloc(sizeof(char)*(strlen(s)+1));
      if(t!=NULL)
         strcpy(t,s);
      return t;
   }else return NULL;
}
#endif

int strcmp_nc(const char *s1, const char *s2)
{
   char *q1,*q2;
   int r;

#ifndef _ALL_SOURCE
   q1=strdup(s1);
   q2=strdup(s2);
#else
   q1=strdup((char *)s1);
   q2=strdup((char *)s2);
#endif
   lowcase(q1);
   lowcase(q2);
    if(q1!=NULL && q2!=NULL)
      r=strcmp(q1,q2);
    else r=strcmp(s1,s2);
   if(q1) free(q1);
   if(q2) free(q2);
   return r;
}

void rdvcpy(double *r,double *s, int len)
{
   int i;

   if(r!=s && r && s)
      for(i=0;i<len;i++)
      r[i]=s[i];
}

double rdvprod(double *r, int len)
{
   double d=1.0;
   int i;
   
   if(r){
      for(i=0;i<len;i++)
         d*=r[i];
   }
   return d;
}

int rivprod(int *r, int len)
{
   int d=1.0;
   int i;
   
   if(r)
      for(i=0;i<len;i++)
         d*=r[i];
   return d;
}

void rivcpy(int *r, int *s, int len)
{
   int i;
   if(r!=s && r && s)
      for(i=0;i<len;i++)
         r[i]=s[i];
}

void rdvramp(double *r, int len, double min, double inc)
{
   double v;
   int i;

   if(r)
      for(v=min,i=0;i<len;i++,v+=inc)
         r[i]=v;
}

void rdvaddc(double *r, double c1, double *s1, double c2, double *s2, int len)
{
   int i;

   if(r && s1 && s2 && r!=s1 && r!=s2)
      for(i=0;i<len;i++)
         r[i]=c1*s1[i] + c2*s2[i];
}

double rdvupdatemean(double *r, double *s1, double *s2, int len)
{
  int i;
   double old,norm=0.0,norm2=0.0;

  if(r && s1 && s2 && r!=s1 && r!=s2){
    for(i=0;i<len;i++){
        old=r[i];
      r[i]=0.5*(s1[i] + s2[i]);
         norm+=(old-r[i])*(old-r[i]);
         norm2+=r[i]*r[i];
      }
      norm=sqrt(norm/norm2);
   }
   return norm;
}

/* returns the l2norm of a vector */
double l2norm(int N, double *vec)
{
  int i;
  double n;

  for(n=0.0,i=0;i<N;i++)
    n+=vec[i]*vec[i];
   if(N>0)
      return sqrt(n / (double) N);
   else return 1.0;
}

double rdvmin(double *v, int n) {
   double rval = 0.0;
   int i;
   if( n ) {
      rval = v[0];
      for( i = 1; i < n; i++ ) {
         rval = v[i] < rval ? v[i] : rval;
      }
   }
   return rval;
}

double rdvmax(double *v, int n) {
   double rval = 0.0;
   int i;
   if( n ) {
      rval = v[0];
      for( i = 1; i < n; i++ ) {
         rval = v[i] > rval ? v[i] : rval;
      }
   }
   return rval;
}


/* returns a vector of size doubles;  prints error message if not enough memory */
double * vec_alloc(int size)
{
  double *q;

  q=(double *)malloc(sizeof(double)*size);
  if(q==NULL){
    fprintf(stderr,"Out of memory error in vec_alloc!\n");
      exit(0);
   }
  return q;
}

int * ivec_alloc(int size)
{
  int *q;

  q=(int *)malloc(sizeof(int)*size);
  if(q==NULL){
    fprintf(stderr,"Out of memory error in ivec_alloc!\n");
    exit(0);
  }
  return q;
}

char * cvec_alloc(int size)
{
  char *q;

  q=(char *)malloc(sizeof(char)*size);
  if(q==NULL){
    fprintf(stderr,"Out of memory error in cvec_alloc!\n");
    exit(0);
  }
  return q;
}

/* returns a vector of size doubles named name;
   prints error message and exits if not enough memory */
double * vec_alloc_n(int size, char * name)
{
  double *q;

  q=(double *)malloc(sizeof(double)*size);
  if(q==NULL){
    fprintf(stderr,"Out of memory error in vec_alloc_n!\n");
    fprintf(stderr,"Unable to malloc %d bytes for <%s>.\n",size,name);
    exit(0);
  }
  return q;
}

/* returns a vector of size ints named name;
   prints error message and exits if not enough memory */
int * ivec_alloc_n(int size, char * name)
{
  int *q;

  q=(int *)malloc(sizeof(int)*size);
  if(q==NULL){
    fprintf(stderr,"Out of memory error in ivec_alloc_n!\n");
    fprintf(stderr,"Unable to malloc %d bytes for <%s>.\n",size,name);
    exit(0);
  }
  return q;
}

/* returns a vector of size chars named name;
   prints error message and exits if not enough memory */
char * cvec_alloc_n(int size, char * name)
{
  char *q;

  q=(char *)malloc(sizeof(char)*size);
  if(q==NULL){
    fprintf(stderr,"Out of memory error in ivec_alloc_n!\n");
    fprintf(stderr,"Unable to malloc %d bytes for <%s>.\n",size,name);
    exit(0);
  }
  return q;
}

#define BSIZE 4096

/* 2008-05-22:  Changed all return(0) -> return(-1) as part of */
/* rationalization of return codes of parameter fetching routines. */

int read_ascii_int(const char *fname, int *p, const int len)
{
  char string[BSIZE];
  int found=1,line;
  FILE *fp;
   int ltrace=0;
   
  fp=fopen(fname,"r");
  if(fp==NULL){
    fprintf(stderr,"read_ascii_int: Unable to open file <%s>.\n",fname);
    return(-1);
  }
   
   for(line=0;found && line<len && fgets(string,BSIZE,fp);line++){
      found=sscanf(string,"%d",p+line);
   }
   if(line<len){
      fprintf(stderr,"read_ascii_int: expected %d ints, only found %d\n",len,line);
      return(-1);
   }else if(!found){
      fprintf(stderr,"read_ascii_int: bad value on line %d\n",line);
      return(-1);
   }
   fclose(fp);
   return(1);
}

/* 2008-05-22:  Changed all return(0) -> return(-1) as part of */
/* rationalization of return codes of parameter fetching routines. */

int read_ascii_double(const char *fname, double *p, const int len)
{
  char string[BSIZE];
  int found=1,line;
  FILE *fp;
   int ltrace=0;
   
   if(ltrace)
      fprintf(stderr,"read_ascii_double: fname=<%s> len=%d\n",fname,len);
  fp=fopen(fname,"r");
  if(fp==NULL){
    fprintf(stderr,"read_ascii_double: Unable to open file <%s>.\n",fname);
    return(-1);
  }
   
   for(line=0;found && line<len && fgets(string,BSIZE,fp);line++){
      if(ltrace)
         fprintf(stderr,"read_ascii_double: %d : <%s>\n",line,string);
      found=sscanf(string,"%lg",p+line);
   }
   if(line<len){
      fprintf(stderr,"read_ascii_double: expected %d doubles, only found %d\n",len,line);
      return(-1);
   }else if(!found){
      fprintf(stderr,"read_ascii_double: bad value on line %d\n",line);
      return(-1);
   }
   fclose(fp);
   return(1);
}

/* 2008-05-22:  Changed all return(0) -> return(-1) as part of */
/* rationalization of return codes of parameter fetching routines. */

int read_ascii_string(const char *fname, char **p, const int len)
{
  char string[BSIZE];
  int found=1,line;
  FILE *fp;
   int ltrace=0;
   
  fp=fopen(fname,"r");
  if(fp==NULL){
    fprintf(stderr,"read_ascii_string: Unable to open file <%s>.\n",fname);
    return(-1);
  }
   if(p) free(p);
   p=(char **)malloc(sizeof(char *)*len);
   if(p==NULL){
      fprintf(stderr,"read_ascii_string: Can't get memory!\n");
      exit(0);
   }
   for(line=0;found && line<len && fgets(string,BSIZE,fp);line++){
      *(p+line)=cvec_alloc_n(strlen(string)+1,"read_ascii_string:");
      strcpy(*(p+line),string);
   }
   if(line<len){
      fprintf(stderr,"read_ascii_string: expected %d strings, only found %d\n",len,line);
      return(-1);
   }
   fclose(fp);
   return(1);
}

/* 2008-05-22:  Changed all return(0) -> return(-1) as part of */
/* rationalization of return codes of parameter fetching routines. */

int read_ascii_ivec(const char *fname, int **p, const int len, const int flag)
{
   return -1;
}

/* 2008-05-22:  Changed all return(0) -> return(-1) as part of */
/* rationalization of return codes of parameter fetching routines. */

int read_ascii_fvec(const char *fname, double **p, const int len, const int flag)
{
  char string[BSIZE];
  int found=1,line;
   double *q;
  FILE *fp;
   int ltrace=0;
   
  fp=fopen(fname,"r");
  if(fp==NULL){
    fprintf(stderr,"read_ascii_string: Unable to open file <%s>.\n",fname);
    return(-1);
  }
   for(line=0;fgets(string,BSIZE,fp);line++);
   rewind(fp);
   if(line>len){
      if(flag){
         if(*p) free(*p);
         *p=vec_alloc_n(line+1,"read_ascii_fvec:");
      }else{
         fprintf(stderr,"read_ascii_fvec: input too long for Fortran data type\n");
         return(-1);
      }
   }
   q=*p;
   *q=(double)(-line);
   q++;
   for(line=0;found && fgets(string,BSIZE,fp);line++){
      found=sscanf(string,"%lg",q+line);
   }
   if(!found){
      fprintf(stderr,"read_ascii_fvec: bad value on line %d\n",line);
      return(-1);
   }
   fclose(fp);
   return(1);
}

int sget_fvec_param(const char *string, const char *name, 
                   double *p, int size)
{
  return sget_param(string,name,"fvec",size,(void *)p,0);
}

int sget_ivec_param(const char *string, const char *name, 
                   int *p, int size)
{
  return sget_param(string,name,"ivec",size,(void *)p,0);
}

int sget_int_param(const char *string, const char *name, 
                   int *p, int size)
{
  return sget_param(string,name,"long",size,(void *)p,0);
}

int sget_real_param(const char *string, const char *name, 
                    double *p, int size)
{
  return sget_param(string,name,"double",size,(void *)p,0);
}

int sget_str_param(const char *string, const char *name,
                   char **p, int size)
{
  return sget_param(string,name,"string",size,(void *)p,0);
}

int get_fvec_param(const char *p_file, const char *name, 
                   double *p, int size)
{
  return get_param_nc(p_file,name,"fvec",size,(void *)p);
}

int get_ivec_param(const char *p_file, const char *name, 
                   int *p, int size)
{
  return get_param_nc(p_file,name,"ivec",size,(void *)p);
}

int get_int_param(const char *p_file, const char *name, 
                   int *p, int size)
{
  return get_param_nc(p_file,name,"long",size,(void *)p);
}

int get_real_param(const char *p_file, const char *name, 
                    double *p, int size)
{
  return get_param_nc(p_file,name,"double",size,(void *)p);
}

int get_str_param(const char *p_file, const char *name,
                   char **p, int size)
{
  return get_param_nc(p_file,name,"string",size,(void *)p);
} 

int get_int_param_v(const char *p_file, const char *name, 
                   int **pp, int *psize)
{
  return get_param_nc_v(p_file,name,"long",psize,(void **)pp);
}

int get_real_param_v(const char *p_file, const char *name, 
                    double **pp, int *psize)
{
  return get_param_nc_v(p_file,name,"double",psize,(void **)pp);
}

int get_str_param_v(const char *p_file, const char *name,
                   char ***pp, int *psize)
{
  return get_param_nc_v(p_file,name,"string",psize,(void **)pp);
} 

#define MAX_LINE_LEN  32767
/* Front ends to 'is_param_assigned_in_file_0' for case-sensitive
   and insensitive searches respectively.  
*/
int is_param_assigned_in_file(char *fname, char *param) {
   return is_param_assigned_in_file_0(fname,param,1);
}

int is_param_assigned_in_file_nc(char *fname, char *param) {
   return is_param_assigned_in_file_0(fname,param,0);
}

/* Checks for an 'assignment' of the form 

   <param> = ...
   <param> := ...

   in <fname> using POSIX 'regex' functions. 
  
   <cs> controls case-sensitivity of search (cs=0 -> case-insensitive)
*/

int is_param_assigned_in_file_0(char *fname, char *param, int cs) {
   char       R[] = "is_param_assigned_in_file_0";
   FILE      *fp;
   char       line[MAX_LINE_LEN];
   int        ltrace = 0;

   char       regex[1024];
   regex_t    reg;
   int        cflags = REG_EXTENDED | REG_NEWLINE;
   int        rc;
   int        nmatch = 1;
   regmatch_t match[1];
   int        eflags = 0;

   if( !cs ) cflags = cflags | REG_ICASE;

   if( ltrace ) {
      fprintf(stderr,"%s: fname='%s' param='%s' cflags=%d\n",
         R,fname,param,cflags);
   }
   if( ! (fp = fopen(fname,"r")) ) return 0;

   sprintf(regex,"^[[:space:]]*%s[[:space:]]*(:=|=)[[:space:]]*[^[:space:]]",
      param); 
   if( ltrace ) {
      fprintf(stderr,"%s: regex='%s'\n",R,regex);
   }
   rc = regcomp(&reg,regex,cflags);
   if( rc ) {
      fprintf(stderr,"%s: Compilation of regular expression '%s' failed!\n",
         R,regex);
      return 0;
   }

   while( fgets(line,MAX_LINE_LEN,fp) ) {
      rc = regexec(&reg,line,nmatch,match,eflags);
      if( !rc ) {
         if( ltrace ) {
            fprintf(stderr,"%s: line='%s' matches regex='%s'\n",R,line,regex);
         }
         return 1;
      }
   }
   fclose(fp);

   return 0;
}


/* search a string for a line of the form
   name := value.
   Assign value to p.
    returns 1 if parameter found and assigned
    returns -1 if parameter not found in file
    returns 0 and prints message if error
*/
#define IVEL 4
#define FVEL 4

/* 
   MWC: 2008-05-15
   Added check for assignment of parameter in file since lexer/grammar can't 
   distinguish between missing or bad parameter assignment
*/
int get_param(const char *p_file, const char *name, 
               char *type, int size, void *p)
{
   static char *l_p_file = (char *) NULL;
   static FILE *l_fp     = (FILE *) NULL;

   char string[BSIZE];
   int rc=0,line=0,in_file=0;
   FILE *fp;
   int ltrace=0;

   void *pr = (void *) NULL;
   int rsize = 0;

   if(ltrace) {
      fprintf(stderr,"get_param: p_file=<%s> name=<%s> type=<%s> size=%d\n",
              p_file,name,type,size);
   }

   if( !p_file ) {
      fprintf(stderr,"get_param: Unable to open file %s.\n",p_file);
      return(0);
   }

   if( !(in_file = is_param_assigned_in_file((char *) p_file,(char *) name)) ) {
      if( ltrace ) {
         fprintf(stderr,"get_param: parameter='%s' not found in file '%s'.\n",
            name,p_file);
      }
      return(0);
   }

   if( !l_p_file || (p_file && strcmp(p_file,l_p_file)) ) {
      if( l_fp )  {
         fclose(l_fp);
         if( ltrace ) {
            fprintf(stderr,"get_param: Closed parameter file='%s'\n",l_p_file);
         }
         l_fp = (FILE *) NULL;
      }
      if( l_p_file ) {
         free(l_p_file);
         l_p_file = (char *) NULL;
      }
      l_p_file = strdup(p_file);
      if( !l_p_file ) {
         fprintf(stderr,"get_param: strdup(%s) failed.\n",p_file);
         return(0);
      }
      l_fp = fopen(l_p_file,"r");
      if( !l_fp ) {
         fprintf(stderr,"get_param: Unable to open file %s.\n",l_p_file);
         return(0);
      }
      if( ltrace ) {
         fprintf(stderr,"get_param: Opened parameter file='%s'\n",l_p_file);
      }
   }

   fp = l_fp;
   rewind(fp);
   
	while(fgets(string,BSIZE,fp) && (rc != 1)){
		line++;
		rc=sget_param(string,name,type,size,p,1);
		if( rc < 0 ){
			fprintf(stderr,"get_param: Error at or near line %d\n",line);
			return(rc);
		 }
	}
	if( rc == 0 && in_file ) rc = -1;
	return(rc);
}

/* same as above except not case sensitive
   and won't allocate memory for ivecs
   for use from FORTRAN */
/* 
   MWC: 2008-05-15
   Added check for assignment of parameter in file since lexer/grammar can't 
   distinguish between missing or bad parameter assignment
*/
int get_param_nc(const char *p_file, const char *name, 
                  char *type, int size, void *p)
{
   static char *l_p_file = (char *) NULL;
   static FILE *l_fp     = (FILE *) NULL;

  char string[BSIZE];
  int rc=0,line=0,in_file=0;
  FILE *fp;
  int ltrace = 0;

	void *pr = (void *) NULL;
  int rsize = 0;
   
   if(ltrace) {
      fprintf(stderr,"get_param_nc: p_file=<%s> name=<%s> type=<%s> size=%d\n",
              p_file,name,type,size);
   }

   if( !p_file ) {
      fprintf(stderr,"get_param_nc: Unable to open file %s.\n",p_file);
      return(0);
   }

   if( !(in_file = is_param_assigned_in_file_nc((char *) p_file,(char *) name)) ) {
      if( ltrace ) {
         fprintf(stderr,"get_param_nc: parameter='%s' not found in file '%s'.\n",
            name,p_file);
      }
      return(0);
   }

   if( !l_p_file || (p_file && strcmp(p_file,l_p_file)) ) {
      if( l_fp )  {
         fclose(l_fp);
         if( ltrace ) {
            fprintf(stderr,"get_param_nc: Closed parameter file='%s'\n",l_p_file);
         }
         l_fp = (FILE *) NULL;
      }
      if( l_p_file ) {
         free(l_p_file);
         l_p_file = (char *) NULL;
      }
      l_p_file = strdup(p_file);
      if( !l_p_file ) {
         fprintf(stderr,"get_param_nc: strdup(%s) failed.\n",p_file);
         return(0);
      }
      l_fp = fopen(l_p_file,"r");
      if( !l_fp ) {
         fprintf(stderr,"get_param_nc: Unable to open file %s.\n",l_p_file);
         return(0);
      }
      if( ltrace ) {
         fprintf(stderr,"get_param_nc: Opened parameter file='%s'\n",l_p_file);
      }
   }

   fp = l_fp;
   rewind(fp);
   
	while(fgets(string,BSIZE,fp) && (rc != 1)){
		line++;
		rc=sget_param(string,name,type,size,p,0);
		if( rc < 0 ){
			fprintf(stderr,"get_param_nc: Error at or near line %d\n",line);
			return(rc);
		 }
	}
	if( rc == 0 && in_file ) rc = -1;
	return(rc);
}

int get_param_nc_v(const char *p_file, const char *name, 
                  char *type, int *psize, void **pp)
{
   static char *l_p_file = (char *) NULL;
   static FILE *l_fp     = (FILE *) NULL;

  char string[BSIZE];
  int rc=0,line=0,in_file=0;
  FILE *fp;
  void *p;
  int ltrace = 0; 
   
   if(ltrace) {
      fprintf(stderr,"get_param_nc_v: p_file=<%s> name=<%s> type=<%s>\n",
              p_file,name,type);
   }

   if( !p_file ) {
      fprintf(stderr,"get_param_nc_v: Unable to open file %s.\n",p_file);
      return(0);
   }

   if( !(in_file = is_param_assigned_in_file_nc((char *) p_file,(char *) name)) ) {
      if( ltrace ) {
         fprintf(stderr,"get_param_nc_v: parameter='%s' not found in file '%s'.\n",
            name,p_file);
      }
      return(0);
   }

   if( !l_p_file || (p_file && strcmp(p_file,l_p_file)) ) {
      if( l_fp )  {
         fclose(l_fp);
         if( ltrace ) {
            fprintf(stderr,"get_param_nc_v: Closed parameter file='%s'\n",l_p_file);
         }
         l_fp = (FILE *) NULL;
      }
      if( l_p_file ) {
         free(l_p_file);
         l_p_file = (char *) NULL;
      }
      l_p_file = strdup(p_file);
      if( !l_p_file ) {
         fprintf(stderr,"get_param_nc_v: strdup(%s) failed.\n",p_file);
         return(0);
      }
      l_fp = fopen(l_p_file,"r");
      if( !l_fp ) {
         fprintf(stderr,"get_param_nc_v: Unable to open file %s.\n",l_p_file);
         return(0);
      }
      if( ltrace ) {
         fprintf(stderr,"get_param_nc_v: Opened parameter file='%s'\n",l_p_file);
      }
   }

   fp = l_fp;
   rewind(fp);
   
	Gpar_pr = pp;
	Gpar_prsize = psize;
	while(fgets(string,BSIZE,fp) && (rc != 1)){
		line++;
		rc = sget_param(string,name,type,-1,p,0);
		if( rc < 0 ){
			fprintf(stderr,"get_param_nc_v: Error at or near line %d\n",line);
			return(rc);
		 }
	}
	if( rc == 0 && in_file ) rc = -1;
	return(rc);
}

int do_ivec(const int it, const int niter, int *ivec)
{
  int n,i,ot,qmod,bg,set=0;
   int ltrace=0;
  
   if(ltrace)
      fprintf(stderr,"do_ivec: it=%d, niter=%d\n",it,niter);
   qmod=100000000;
   if(qmod<niter) qmod=niter+1;
   bg=0;
   n=ivec[0];
   for(i=0;i<n;i++){
      if(it>ivec[IVEL*i+2])
         ivec[IVEL*i+4]=1;
      if(!ivec[IVEL*i+4]){
         if(it>=ivec[IVEL*i+1] && it<=ivec[IVEL*i+2] && 
             ivec[IVEL*i+3]<qmod){
            qmod=ivec[IVEL*i+3];
            bg=ivec[IVEL*i+1];
            set=1;
         }
      }
   }
   if(qmod==0)
      qmod=1;
   if(ltrace)
      fprintf(stderr,"do_ivec: it=%d, bg=%d, qmod=%d\n",it,bg,qmod);
   if(set && ((it-bg)%qmod==0))
      ot=1;
   else ot=0;
   if(ltrace)
      fprintf(stderr,"do_ivec: ot=%d\n",ot);
  return ot;
}

void fixup_ivec(const int min, const int max, const int lev, int *ivec)
{
   int n,i;
   
   n=ivec[0];
   for(i=0;i<n;i++){
      if(ivec[IVEL*i+2]==-2)
         ivec[IVEL*i+2]=ivec[IVEL*i+1];
      if(ivec[IVEL*i+1]==-1)
         ivec[IVEL*i+1]=min;
      else ivec[IVEL*i+1]*=(int)pow(2.0,(double)lev);
      if(ivec[IVEL*i+2]==-1)
         ivec[IVEL*i+2]=max;
      else ivec[IVEL*i+2]*=(int)pow(2.0,(double)lev);
      if(ivec[IVEL*i+1]!=ivec[IVEL*i+2])
         ivec[IVEL*i+3]*=(int)pow(2.0,(double)lev);
   }
}

void fixup_fvec(double *fv)
{
   double *nfv,*cfv,cval;
   int pos,done,i,j;
   
   if(fv[0]>0.0){
      cfv=fv;
      cval=1.0e20;
      nfv=vec_alloc((int)cfv[0]*FVEL+1);
      nfv[0]=cfv[0];
      for(i=0,j=-1;i<cfv[0];i++){
         cfv[i*FVEL+1+3]=0;
         if(cfv[i*FVEL+1]<cval){
            cval=cfv[i*FVEL+1];
            j=i;
         }
      }
      if(j==-1){
         fprintf(stderr,"fixup_fvec: error: threshold too small\n");
         exit(0);
      }
      cfv[j*FVEL+1+3]=1.0;
      nfv[1]=cfv[j*FVEL+1];
      nfv[2]=cfv[j*FVEL+1+1];
      nfv[3]=cfv[j*FVEL+1+2];
      nfv[4]=0;
      pos=1;
      done=0;
      while(!done){
         cval=1.0e20;
         j=-1;
         for(i=0;i<cfv[0];i++){
            if(cfv[i*FVEL+1+3]<1.0 && cfv[i*FVEL+1]<cval){
               cval=cfv[i*FVEL+1];
               j=i;
            }
         }
         if(j==-1){
            fprintf(stderr,"fixup_fvec: error: threshold too small\n");
            exit(0);
         }
         cfv[j*FVEL+1+3]=1.0;
         if(cfv[j*FVEL+1]>nfv[(pos-1)*FVEL+1+1]){ /* above */
            nfv[pos*FVEL+1]=cfv[j*FVEL+1];
            nfv[pos*FVEL+1+1]=cfv[j*FVEL+1+1];
            nfv[pos*FVEL+1+2]=cfv[j*FVEL+1+2];
            nfv[pos*FVEL+1+3]=0;
            pos++;
         }else if(cfv[j*FVEL+1+1]<nfv[(pos-1)*FVEL+1+1]){ /* inside */
            fprintf(stderr,"fixup_fvec: error: ranges can't overlap\n");
            exit(0);
         }else{/* straddle */
            fprintf(stderr,"fixup_fvec: error: ranges can't be nested\n");
            exit(0);
         }
      }
      rdvcpy(cfv,nfv,(int)nfv[0]*FVEL+1);
      free(nfv);
   }
}

int size_fvec(double *fv)
{
   int i,l;
   
   if(fv[0]<0.0)
      return -fv[0];
   else{
      for(l=0,i=0;i<(int)fv[0];i++){
         l+=(int)fv[i*FVEL+1+2];
      }
      return l;
   }
}

double get_fvec(double *fv, const int i)
{
   int j,l;
   double val,inc;
   
   if(fv[0]<0.0){
      if(i>(-fv[0])) /* index out of range */
        val=fv[1];
      else val=fv[i+1];
   }else{
      l=0,j=-1;
      do{
         j++;
         l+=(int)fv[j*FVEL+1+2];
      }while(j<(int)(fv[0]-1) && l<i+1);
      if(l<i) /* index out of range */
         val=fv[j*FVEL+1+1];
      else if(fv[j*FVEL+1+2]==1)
         val=fv[j*FVEL+1];
      else{
         inc=(fv[j*FVEL+1+1]-fv[j*FVEL+1])/(fv[j*FVEL+1+2]-1);
         val=fv[j*FVEL+1];
         for(l-=(int)fv[j*FVEL+1+2];l<i;l++,val+=inc);
      }
   }
   return val;
}

/* takes constant args and returns array */
double *gft_make_bbox(int rank, ...)
{
   va_list ap;
   int i,l;
   static double bb[2*MAXRANK];
   int ltrace=0;
   
   if(ltrace)
      fprintf(stderr,"gft_make_bbox: rank=%d\n",rank);
   if(rank>MAXRANK || rank<=0){
      fprintf(stderr,"gft_make_bbox: invalid rank=%d.\n",rank);
      return(NULL);
   }else{
      l=2*rank;
      va_start(ap,rank);
      for(i=0;i<l;i++){
         bb[i]=va_arg(ap,double);
         if(ltrace)
            fprintf(stderr,"gft_make_bbox: bb[%d]=%.16g\n",i,bb[i]);
      }
      va_end(ap);
      return(bb);
   }
}

/*   returns full function name:
         func_name(c0,c1,c2,...,t);
      rank is the spacial rank
      cnames[rank] contains the spacial coordinate names
*/
char *gft_make_full_func_name(char *func_name, int rank, char **cnames)
{
   char *n,*p;
   int len=0,j,i;
   
   if(!func_name || !cnames || rank<=0)
      n=NULL;
   else{ 
      len=strlen(func_name);
      for(i=0;i<rank;i++)
         len+=strlen(cnames[i]);
      if((n=(char *)malloc(sizeof(char)*(len+5)))==NULL){
         fprintf(stderr,
                     "gft_make_full_func_name: can't get memory for new string.\n");
         n=NULL;
      }else{
         strcpy(n,func_name);
         i=strlen(n);
         n[i++]='(';
         for(j=0;j<rank;j++)
            i+=sprintf(n+i,"%s,",cnames[j]);
         sprintf(n+i,"t)");
      }
   }
   return n;   
}

void gft_close(const char *nm)
{
#ifdef HAVE_HDF
   if(getenv("BBHHDF"))
      gft_close_hdf(nm);
   else
#endif
      gft_close_sdf_stream(nm);
}

void gft_close_all(void)
{
#ifdef HAVE_HDF
   if(getenv("BBHHDF"))
      gft_close_all_hdf();
   else
#endif
      gft_close_all_sdf();
}

#ifdef HAVE_HDF

static int call=0,len,top,mode;
static hdf_data *dfiles;
static int32 d_type_=DFNT_FLOAT64;
static int gOne_file=0,gFid=-1;
static char *gOF_name=NULL;

int gft_init(void)
{
   int ind;
   char *ev;
   int ltrace=0;
   
   if(!call){
      len=32;
      dfiles=NULL;
      if(!gft_newstruct(&dfiles,len)){
         fprintf(stderr,"gft_init: memory error\n");
         return(0);
      }else{
         call=1;
         top=ind=-1;
      }
      ev=getenv("GFT_HDFAPPEND");
      if(ev!=NULL)
         mode=0;
      else mode=1;
#ifdef HAVE_ATEXIT
      if(atexit(gft_close_all_hdf)!=0){
         fprintf(stderr,"gft_init: unable to register gft_close_all_hdf (atexit)\n");
      }
#endif
#ifdef HAVE_ATABORT
      if(atabort(gft_close_all_hdf)!=0){
         fprintf(stderr,"gft_init: unable to register gft_close_all_hdf (atabort)\n");
      }
#endif
   }
   return 1;
}

void gft_del(void)
{
   int i;
   
   if(call){
      for(i=0;i<top;i++){
         if(dfiles[i].state)
            SDend((int32)dfiles[i].fid);
         if(dfiles[i].fn){
            free(dfiles[i].fn);
            dfiles[i].fn=NULL;
         }
         if(dfiles[i].gfn){
            free(dfiles[i].gfn);
            dfiles[i].gfn=NULL;
         }
         if(dfiles[i].tmv){
            free(dfiles[i].tmv);
            dfiles[i].tmv=NULL;
         }
      }
      free(dfiles);
   }
}

void gft_clear(const int ind)
{
   int ltrace=0;

   if(ltrace != 0){
      fprintf(stderr,"gft_clear: call=%d  ind=%d top=%d\n",call,ind,top);
   }
   if(call && ind<=top){
      dfiles[ind].rank=0;
      if(dfiles[ind].tmv){
         free(dfiles[ind].tmv);
         dfiles[ind].tmv=NULL;
      }
   }
}

/* returns pointer to malloced string containing [a-zA-Z0-9_./] 
   or NULL upon failure */
char * gft_make_file_name(const char *s)
{
   char *n,*p;
   
   if(!s)
      n=NULL;
   else if((n=(char *)malloc(sizeof(char)*(strlen(s)+5)))==NULL){
      fprintf(stderr,"gft_make_file_name: can't get memory for new string.\n");
      n=NULL;
   }else{
      for(p=n;*s;s++)
         if(isalnum(*s) || *s=='_' || *s=='.' || *s=='/')
            *(p++)=*s;
      if(*(p-4)!='.' || *(p-3)!='h' || *(p-2)!='d' || *(p-1)!='f'){
         *(p++)='.';
         *(p++)='h';
         *(p++)='d';
         *(p++)='f';
      }
      *p=0;
   }
   return n;
}

/* returns pointer to gfunc name without unique suffix */
char * gft_strip_suf(const char *s)
{
   char *n,*p;
   int ltrace=0;
   
   if(ltrace)
      fprintf(stderr,"gft_strip_suf: s=<%s>\n",s);
   if(!s)
      n=NULL;
   else if((n=(char *)malloc(sizeof(char)*(strlen(s)+5)))==NULL){
      fprintf(stderr,"gft_strip_suf: can't get memory for new string.\n");
      n=NULL;
   }else{
      strcpy(n,s);
      p=strchr(n,'$');
      if(p)
         *p=0;
   }
   return n;
}

/* search the array list for gfn.  If found, return index, else return -1 */
int gft_search_list(const char *gfn, hdf_data *list, int n)
{
   int i;
   int done;
   int ltrace=0;
      
   if(list && gfn && n<=top+1){
      for(i=0,done=0;i<n && !done;i++){
         if(list[i].gfn)
            done=!strcmp(gfn,list[i].gfn);
         if(ltrace){
            fprintf(stderr,"gft_search_list: done=%d, i=%d, list[%d].gfn=<%s>, fn=<%s>\n",
            done,i,i,list[i].gfn,list[i].fn);
         }
      }
      if(ltrace)
         fprintf(stderr,"gft_search_list: n=%d, i=%d\n",n,i);
      if(done)
         return i-1;
      else return -1;
   }else return -1;
}

/* search the array list for fn.  If found, return index, else return -1 */
int gft_search_list_id(const char *fn, hdf_data *list, int n)
{
   int i;
   int done;
   int ltrace=0;
      
   if(list && fn && n<=top+1){
      for(i=0,done=0;i<n && !done;i++){
         if(list[i].fn)
            done=!strcmp(fn,list[i].fn);
         if(ltrace){
            fprintf(stderr,"gft_search_list_id: done=%d, i=%d, list[%d].gfn=<%s>, fn=<%s>\n",
            done,i,i,list[i].gfn,list[i].fn);
         }
      }
      if(ltrace)
         fprintf(stderr,"gft_search_list_id: n=%d, i=%d\n",n,i);
      if(done)
         return i-1;
      else return -1;
   }else return -1;
}

void gft_dump_hdf_data(hdf_data *df)
{
   int i;
   
   if(df){
      if(df->fn)
         fprintf(stderr,"fn=<%s>\n",df->fn);
      else fprintf(stderr,"fn=NULL\n");
      if(df->gfn)
         fprintf(stderr,"gfn=<%s>\n",df->gfn);
      else fprintf(stderr,"gfn=NULL\n");
      fprintf(stderr,"version=%d\n",df->vers);
      fprintf(stderr,"mode=%d\n",df->mode);
      fprintf(stderr,"fid=%d\n",df->fid);
      fprintf(stderr,"state=%d\n",df->state);
      fprintf(stderr,"sdsnum=%d\n",df->sdsnum);
      fprintf(stderr,"nslabs=%d\n",df->nslabs);
      fprintf(stderr,"rank=%d\n",df->rank);
   }
}

int gft_newstruct(hdf_data **df, int len)
{
   int i,ol;
   hdf_data *tmpd;
   
   if(*df){ /* already allocated */
      ol=len/2;
      tmpd=(hdf_data *)malloc(sizeof(hdf_data)*len);
      if(!tmpd){
         fprintf(stderr,"gft_newstruct: can't malloc new array.\n");
         errno=1;
         return(0);
      }
      for(i=0;i<ol;i++){
         tmpd[i].fn=(*df)[i].fn;
         tmpd[i].gfn=(*df)[i].gfn;
         tmpd[i].mode=(*df)[i].mode;
         tmpd[i].fid=(*df)[i].fid;
         tmpd[i].vers=(*df)[i].vers;
         tmpd[i].state=(*df)[i].state;
         tmpd[i].sdsnum=(*df)[i].sdsnum;
         tmpd[i].nslabs=(*df)[i].nslabs;
         tmpd[i].rank=(*df)[i].rank;
         tmpd[i].tmv=(*df)[i].tmv;
         tmpd[i].mtime=(*df)[i].mtime;
      }
      for(i=ol;i<len;i++){
         tmpd[i].fn=NULL;
         tmpd[i].gfn=NULL;
         tmpd[i].mode=-1;
         tmpd[i].fid=-1;
         tmpd[i].vers=0;
         tmpd[i].state=0;
         tmpd[i].sdsnum=0;
         tmpd[i].nslabs=0;
         tmpd[i].rank=0;
         tmpd[i].tmv=NULL;
         tmpd[i].mtime=0;
      }
      free(*df);
      *df=tmpd;
      return(1);
   }else{ /* new */
      *df=(hdf_data *)malloc(sizeof(hdf_data)*len);
      if(!*df){
         fprintf(stderr,"gft_newstruct: can't malloc new array.\n");
         errno=1;
         return(0);
      }
      for(i=0;i<len;i++){
         (*df)[i].fn=NULL;
         (*df)[i].gfn=NULL;
         (*df)[i].mode=-1;
         (*df)[i].fid=-1;
         (*df)[i].vers=0;
         (*df)[i].state=0;
         (*df)[i].sdsnum=0;
         (*df)[i].nslabs=0;
         (*df)[i].rank=0;
         (*df)[i].tmv=NULL;
         (*df)[i].mtime=0;
      }
      return(1);
   }
}

void gft_close_all_hdf(void)
{
   int i;
   
   if(call){
      for(i=0;i<=top;i++)
         if(dfiles[i].state){
            if(!gOne_file || (gOne_file && dfiles[i].fid!=gFid))
               SDend((int32)dfiles[i].fid);
            dfiles[i].state=0;
         }
      if(gOne_file){
         SDend((int32)gFid);
         gFid=-1;
      }
   }
}

int gft_set_d_type(int type)
{
   if(type==DFNT_FLOAT64 || type==DFNT_INT32){
      d_type_=type;
   }
   return d_type_;
}

int gft_check_file(const int fid, const char *nm)
{
   int32 aind,v=1;
   int ltrace=0;
   
   if(SDfindattr((int32)fid,"rnpl_file")==-1){
      fprintf(stderr,"gft_check_file: <%s> is not an RNPL file.\n",nm);
      errno=1;
      return 0;
   }
   if((aind=SDfindattr((int32)fid,"rnpl_hdf_vers"))==-1){
      v=1;
      if(ltrace)
         fprintf(stderr,"gft_check_file: version=1\n");
   }else{
      if(SDreadattr((int32)fid,(int32)aind,(void *)&v)==-1){
         fprintf(stderr,"gft_check_file: can't read version from <%s>\n",nm);
         v=1;
         if(ltrace)
            fprintf(stderr,"gft_check_file: version=1\n");
      }
   }
   return v;
}

int gft_set_file_rnpl(const int fid, const char *nm, const int vers)
{
   int32 tmp=1;
   int32 v;
   
   if(SDsetattr((int32)fid,"rnpl_file",DFNT_INT32,(int32)1,(void *)&tmp)==-1){
      fprintf(stderr,"gft_set_file_rnpl: can't create attribute in HDF file <%s>\n",nm);
      return 0;
   }
   v=vers;
   if(SDsetattr((int32)fid,"rnpl_hdf_vers",DFNT_INT32,(int32)1,(void *)&v)==-1){
      fprintf(stderr,"gft_set_file_rnpl: can't create version in HDF file <%s>\n",nm);
      return 0;
   }
   return 1;
}

/*
   Set the I/O system to single file mode
   All data sets are written to a single file
   All data sets are read from a single file 
*/
void gft_set_single_hdf(const char *nm)
{
   gft_close_all_hdf();
   gOne_file=1;
   if(gOF_name)
      free(gOF_name);
   gOF_name=gft_make_file_name(nm);
}

/*
   Set the I/O system to multi file mode
*/
void gft_set_multi_hdf(void)
{
   if(gOne_file){
      gOne_file=0;
      gft_close_all_hdf();
      if(gOF_name){
         free(gOF_name);
         gOF_name=NULL;
      }
   }
}

/* create file and return index or -1 on failure */
/* if file has been accessed this run, don't overwrite */
/* if file exists and mode==0, don't overwrite */
/* inNm is grid function name */
int gft_create(const char *inNm)
{
   static int first_call=1;
   int sr,ind,nf=1;
   int32 amode;
   struct stat sbuf;
   char *nm;
   int ltrace=0;
   
   if(ltrace)
      fprintf(stderr,"gft_create: call=%d inNm=<%s>\n",call,inNm);   
   
   if(gOne_file){
#ifndef _ALL_SOURCE
      nm=strdup(inNm);
#else
      nm=strdup((char *)inNm);
#endif
   }else{
      nm=gft_make_file_name(inNm);
   }
   if(ltrace)
      fprintf(stderr,"gft_create: nm=<%s>\n",nm);
   if(!call){
      if(gft_init()){
         top=ind=0;
         dfiles[ind].fn=nm;
#ifndef _ALL_SOURCE
         dfiles[ind].gfn=strdup(inNm);
#else
         dfiles[ind].gfn=strdup((char *)inNm);
#endif
      }else{
         fprintf(stderr,"gft_create: memory error\n");
         return(-1);
      }
   }else{
      if((ind=gft_search_list(inNm,dfiles,top+1))!=-1){
         nf=0;
      }else{ /* new file */
         top++;
         if(top >= len){
           len*=2;
            if(!gft_newstruct(&dfiles,len)){
               fprintf(stderr,"gft_create: memory error\n");
               return(-1);
            }
         }
         ind=top;
         dfiles[ind].fn=nm;
#ifndef _ALL_SOURCE
         dfiles[ind].gfn=strdup(inNm);
#else
         dfiles[ind].gfn=strdup((char *)inNm);
#endif
         if(gOne_file && !first_call)
            nf=0;
      }
   }
   if(ltrace)
      fprintf(stderr,"gft_create: ind=%d, fn=<%s>, gfn=<%s>\n",ind,dfiles[ind].fn,
      dfiles[ind].gfn);
   first_call=0;
   if(!nf || !mode)
      amode=DFACC_RDWR;
   else amode=DFACC_CREATE;
   if(!nf)
      gft_clear(ind);
   
   if(gOne_file){
      if(gFid==-1){
         gFid=SDstart(gOF_name,amode);
         if(gFid==-1 && amode==DFACC_CREATE){
            fprintf(stderr,"gft_create: unable to create file <%s>\n",gOF_name);
            return(-1);
         }else if(gFid==-1){
            amode=DFACC_CREATE;
            gFid=SDstart(gOF_name,amode);
            if(gFid==-1){
               fprintf(stderr,"gft_create: unable to create file <%s>\n",gOF_name);
               return(-1);
            }
         }
         if(!gft_set_file_rnpl(gFid,gOF_name,3)){
            return(-1);
         }
      }
      dfiles[ind].state=1;
      dfiles[ind].mode=amode;
      dfiles[ind].fid=gFid;
   }else{
      if(!dfiles[ind].state){
         dfiles[ind].fid=SDstart(dfiles[ind].fn,amode);
         if(dfiles[ind].fid==-1 && amode==DFACC_CREATE){
            fprintf(stderr,"gft_create: unable to create file <%s>\n",
                        dfiles[ind].fn);
            return(-1);
         }else if(dfiles[ind].fid==-1){
            amode=DFACC_CREATE;
            dfiles[ind].fid=SDstart(dfiles[ind].fn,amode);
            if(dfiles[ind].fid==-1){
               fprintf(stderr,"gft_create: unable to create file <%s>\n",
                           dfiles[ind].fn);
               return(-1);
            }else{
               dfiles[ind].state=1;
               dfiles[ind].mode=amode;
            }
         }else{
            dfiles[ind].state=1;
            dfiles[ind].mode=amode;
         }
      }else if(dfiles[ind].mode!=DFACC_CREATE && dfiles[ind].mode!=DFACC_RDWR){ /* file already open */
         SDend((int32)dfiles[ind].fid);
         dfiles[ind].state=0;
         dfiles[ind].fid=SDstart(dfiles[ind].fn,amode);
         if(dfiles[ind].fid==-1){
            fprintf(stderr,"gft_create: unable to create file <%s>\n",
                        dfiles[ind].fn);
            return(-1);
         }else{
            dfiles[ind].state=1;
            dfiles[ind].mode=amode;
         }
      }
      if(!gft_set_file_rnpl(dfiles[ind].fid,dfiles[ind].fn,3)){
         gft_close_hdf(inNm);
         return(-1);
      }
      if((sr=stat(dfiles[ind].fn,&sbuf))==-1){
         fprintf(stderr,"gft_create: unable to get modification time for <%s>\n",
                     dfiles[ind].fn);
         fprintf(stderr,"gft_create: stat returned %d\n",sr);
         return(-1);
      }else{
         dfiles[ind].mtime=sbuf.st_mtime;
      }
   }
   return ind;
}

/* open file and return index or -1 on failure */
/* nm is gfunc name */
/* file must be previously accessed with gft_read_name */
int gft_open_rd(const char *nm)
{
   int ind,sr,nw=1;
   struct stat sbuf;
   int ltrace=0;
   
   if(ltrace)
      fprintf(stderr,"gft_open_rd: call=%d nm=<%s>\n",call,nm);   
   
   if(!call){
      fprintf(stderr,"gft_open_rd: gft_read_name must be called to initialize\n");
      return(-1);
   }else{
      if((ind=gft_search_list(nm,dfiles,top+1))==-1){ /* new file */
         fprintf(stderr,"gft_open_rd: gft_read_name must be called to initialize\n");
         return(-1);
      }
   }
   if(gOne_file){
      if(gFid==-1){
         gFid=SDstart(gOF_name,(int32)DFACC_RDONLY);
         if(gFid==-1){
            fprintf(stderr,"gft_open_rd: unable to open file <%s>\n",gOF_name);
            return(-1);
         }
      }
      dfiles[ind].state=1;
      dfiles[ind].mode=DFACC_RDONLY;
      dfiles[ind].fid=gFid;
      if((dfiles[ind].vers=gft_check_file(gFid,gOF_name))==0)
         return(-1);
   }else{
      if(!dfiles[ind].state){
         dfiles[ind].fid=SDstart(dfiles[ind].fn,(int32)DFACC_RDONLY);
         if(dfiles[ind].fid==-1){
            fprintf(stderr,"gft_open_rd: unable to open file <%s>\n",
                        dfiles[ind].fn);
            return(-1);
         }else{
            dfiles[ind].state=1;
            dfiles[ind].mode=DFACC_RDONLY;
         }
         nw=0;
      }else if(dfiles[ind].mode!=DFACC_RDONLY){ /* file already open */
         SDend((int32)dfiles[ind].fid);
         dfiles[ind].state=0;
         dfiles[ind].fid=SDstart(dfiles[ind].fn,(int32)DFACC_RDONLY);
         if(dfiles[ind].fid==-1){
            fprintf(stderr,"gft_open_rd: unable to open file <%s>\n",
                        dfiles[ind].fn);
            return(-1);
         }else{
            dfiles[ind].state=1;
            dfiles[ind].mode=DFACC_RDONLY;
         }
         nw=0;
      }
      if((dfiles[ind].vers=gft_check_file(dfiles[ind].fid,dfiles[ind].fn))==0){
         gft_close_hdf(nm);
         return(-1);
      }
      if((sr=stat(dfiles[ind].fn,&sbuf))==-1){
         fprintf(stderr,"gft_open_rd: unable to get modification time for <%s>\n",
                     dfiles[ind].fn);
         fprintf(stderr,"gft_open_rd: stat returned %d\n",sr);
         return(-1);
      }else{
         if(ltrace != 0){
            fprintf(stderr,"gft_open_rd: sbuf.st_mtime=%d\n",sbuf.st_mtime);
            fprintf(stderr,"gft_open_rd: mtime=%d\n",dfiles[ind].mtime);
         }
         if(sbuf.st_mtime > dfiles[ind].mtime){
            gft_clear(ind);
            if(nw){
               SDend((int32)dfiles[ind].fid);
               dfiles[ind].state=0;
               dfiles[ind].fid=SDstart(dfiles[ind].fn,(int32)DFACC_RDONLY);
               if(dfiles[ind].fid==-1){
                  fprintf(stderr,"gft_open_rd: unable to open file <%s>\n",
                              dfiles[ind].fn);
                  return(-1);
               }else{
                  dfiles[ind].state=1;
                  dfiles[ind].mode=DFACC_RDONLY;
               }
            }
         }
         dfiles[ind].mtime=sbuf.st_mtime;
      }
   }
   return ind;
}

/* like gft_create except for idata files */
/* nm is file name */
int gft_create_id(const char *nm)
{
   int sr,ind,nf=1;
   int32 amode;
   struct stat sbuf;
   int ltrace=0;
   
   if(ltrace)
      fprintf(stderr,"gft_create_id: call=%d nm=<%s>\n",call,nm);   
   
   if(!call){
      if(gft_init()){
         top=ind=0;
#ifndef _ALL_SOURCE
         dfiles[ind].fn=strdup(nm);
#else
         dfiles[ind].fn=strdup((char *)nm);
#endif
      }else{
         fprintf(stderr,"gft_create: memory error\n");
         return(-1);
      }
   }else{
      if((ind=gft_search_list_id(nm,dfiles,top+1))!=-1){
         nf=0;
      }else{ /* new file */
         top++;
         if(top >= len){
           len*=2;
            if(!gft_newstruct(&dfiles,len)){
               fprintf(stderr,"gft_create: memory error\n");
               return(-1);
            }
         }
         ind=top;
#ifndef _ALL_SOURCE
         dfiles[ind].fn=strdup(nm);
#else
         dfiles[ind].fn=strdup((char *)nm);
#endif
      }
   }
   if(ltrace)
      fprintf(stderr,"gft_create_id: ind=%d, fn=<%s>, gfn=<%s>\n",ind,
      dfiles[ind].fn,dfiles[ind].gfn);
   if(!nf || !mode)
      amode=DFACC_RDWR;
   else amode=DFACC_CREATE;
   if(!nf)
      gft_clear(ind);
   if(!dfiles[ind].state){
      dfiles[ind].fid=SDstart(dfiles[ind].fn,amode);
      if(dfiles[ind].fid==-1 && amode==DFACC_CREATE){
         fprintf(stderr,"gft_create: unable to create file <%s>\n",
                     dfiles[ind].fn);
         return(-1);
      }else if(dfiles[ind].fid==-1){
         amode=DFACC_CREATE;
         dfiles[ind].fid=SDstart(dfiles[ind].fn,amode);
         if(dfiles[ind].fid==-1){
            fprintf(stderr,"gft_create: unable to create file <%s>\n",
                        dfiles[ind].fn);
            return(-1);
         }else{
            dfiles[ind].state=1;
            dfiles[ind].mode=amode;
         }
      }else{
         dfiles[ind].state=1;
         dfiles[ind].mode=amode;
      }
   }else if(dfiles[ind].mode!=DFACC_CREATE && dfiles[ind].mode!=DFACC_RDWR){ /* file already open */
      SDend((int32)dfiles[ind].fid);
      dfiles[ind].state=0;
      dfiles[ind].fid=SDstart(dfiles[ind].fn,amode);
      if(dfiles[ind].fid==-1){
         fprintf(stderr,"gft_create: unable to create file <%s>\n",
                     dfiles[ind].fn);
         return(-1);
      }else{
         dfiles[ind].state=1;
         dfiles[ind].mode=amode;
      }
   }
   if(!gft_set_file_rnpl(dfiles[ind].fid,dfiles[ind].fn,3)){
      gft_close_hdf(nm);
      return(-1);
   }
   if((sr=stat(dfiles[ind].fn,&sbuf))==-1){
      fprintf(stderr,"gft_create: unable to get modification time for <%s>\n",
                  dfiles[ind].fn);
      fprintf(stderr,"gft_create: stat returned %d\n",sr);
      return(-1);
   }else{
      dfiles[ind].mtime=sbuf.st_mtime;
   }
   return ind;
}

/* like gft_open_rd, but for idata files */
/* nm is file name */
int gft_open_rd_id(const char *nm)
{
   int ind,sr,nw=1;
   struct stat sbuf;
   int ltrace=0;
   
   if(ltrace)
      fprintf(stderr,"gft_open_rd_id: call=%d nm=<%s>\n",call,nm);   
   
   if(!call){
      if(gft_init()){
         top=ind=0;
#ifndef _ALL_SOURCE
         dfiles[ind].fn=strdup(nm);
#else
         dfiles[ind].fn=strdup((char *)nm);
#endif
      }else{
         fprintf(stderr,"gft_open_rd_id: memory error\n");
         return(-1);
      }
   }else{
      if((ind=gft_search_list_id(nm,dfiles,top+1))==-1){ /* new file */
         if(ltrace)
            fprintf(stderr,"gft_open_rd_id: new file\n");
         top++;
         if(top >= len){
           len*=2;
            if(!gft_newstruct(&dfiles,len)){
               fprintf(stderr,"gft_open_rd_id: memory error\n");
               return(-1);
            }
         }
         ind=top;
#ifndef _ALL_SOURCE
         dfiles[ind].fn=strdup(nm);
#else
         dfiles[ind].fn=strdup((char *)nm);
#endif
      }
   }
   if(ltrace)
      fprintf(stderr,"gft_open_rd_id: ind=%d, fn=<%s>, gfn=<%s>\n",ind,
      dfiles[ind].fn,dfiles[ind].gfn);
   if(!dfiles[ind].state){
      dfiles[ind].fid=SDstart(dfiles[ind].fn,(int32)DFACC_RDONLY);
      if(dfiles[ind].fid==-1){
         fprintf(stderr,"gft_open_rd_id: unable to open file <%s>\n",
                     dfiles[ind].fn);
         return(-1);
      }else{
         dfiles[ind].state=1;
         dfiles[ind].mode=DFACC_RDONLY;
      }
      nw=0;
   }else if(dfiles[ind].mode!=DFACC_RDONLY){ /* file already open */
      SDend((int32)dfiles[ind].fid);
      dfiles[ind].state=0;
      dfiles[ind].fid=SDstart(dfiles[ind].fn,(int32)DFACC_RDONLY);
      if(dfiles[ind].fid==-1){
         fprintf(stderr,"gft_open_rd_id: unable to open file <%s>\n",
                     dfiles[ind].fn);
         return(-1);
      }else{
         dfiles[ind].state=1;
         dfiles[ind].mode=DFACC_RDONLY;
      }
      nw=0;
   }
   if((dfiles[ind].vers=gft_check_file(dfiles[ind].fid,dfiles[ind].fn))==0){
      gft_close_hdf(nm);
      return(-1);
   }
   if((sr=stat(dfiles[ind].fn,&sbuf))==-1){
      fprintf(stderr,"gft_open_rd_id: unable to get modification time for <%s>\n",
                  dfiles[ind].fn);
      fprintf(stderr,"gft_open_rd_id: stat returned %d\n",sr);
      return(-1);
   }else{
      if(ltrace != 0){
         fprintf(stderr,"gft_open_rd_id: sbuf.st_mtime=%d\n",sbuf.st_mtime);
         fprintf(stderr,"gft_open_rd_id: mtime=%d\n",dfiles[ind].mtime);
      }
      if(sbuf.st_mtime > dfiles[ind].mtime){
         gft_clear(ind);
         if(nw){
            SDend((int32)dfiles[ind].fid);
            dfiles[ind].state=0;
            dfiles[ind].fid=SDstart(dfiles[ind].fn,(int32)DFACC_RDONLY);
            if(dfiles[ind].fid==-1){
               fprintf(stderr,"gft_open_rd_id: unable to open file <%s>\n",
                           dfiles[ind].fn);
               return(-1);
            }else{
               dfiles[ind].state=1;
               dfiles[ind].mode=DFACC_RDONLY;
            }
         }
      }
      dfiles[ind].mtime=sbuf.st_mtime;
   }
   return ind;
}

/* nm is gfunc name */
void gft_close_hdf(const char *nm)
{
   int ind;
   int ltrace=0;
   
   if(ltrace)
     fprintf(stderr,"gft_close_hdf(%s)\n",nm);
   if(call){
      if((ind=gft_search_list(nm,dfiles,top+1))!=-1){
         if(ltrace)
            fprintf(stderr,"gft_close_hdf[1]: ind=%d\n",ind);
         if(dfiles[ind].state){
            if(ltrace)
               fprintf(stderr,"gft_close_hdf: calling SDend\n");
            if(gOne_file && dfiles[ind].fid==gFid){
               gFid=-1;
            }
            SDend((int32)dfiles[ind].fid);
            dfiles[ind].state=0;
         }
      }else if((ind=gft_search_list_id(nm,dfiles,top+1))!=-1){
         if(ltrace)
            fprintf(stderr,"gft_close_hdf[2]: ind=%d\n",ind);
         if(dfiles[ind].state){
            if(ltrace)
               fprintf(stderr,"gft_close_hdf: calling SDend\n");
            if(gOne_file && dfiles[ind].fid==gFid){
               gFid=-1;
            }
            SDend((int32)dfiles[ind].fid);
            dfiles[ind].state=0;
         }
      }
   }
}

/* reads time level of 1st grid function */
/* read time level from hdf file.  Create storage */
int GFT_extract(const char *fn, /* in: file name */
               const int tlev, /* in: time level to retrieve (1 - ntlev) */
               char **func_name, /* name of this grid function */
               int *ntlev, /* number of time levels available in file */
               double **tvec, /* vector of time values */
               double *time, /* time of this slice */
               int *drank, /* rank of data set */
               int **shape, /* shape of data set */
               char ***cnames, /* names of coordinates */
               double ***coords, /* values of coordinates */
               double **data ) /* actual data */
{
  int32 gattrs,nslabs;
   int fid;
  int32 sds_id;
   int32 start[MAXRANK];
  int32 size[MAXRANK];
   int i;
  int32 rank;
   int ind;
  int32 sind,ln,sinc;
  int32 type,attrs,sz,dim_id,dattrs;
  char aname[60],name[60],*st,*fname;
  char gfname[128];
   double t;
   int done;
  int ltrace=0;
   
   if(tlev<0){ /* signal to free storage */
      if(*func_name)
         free(*func_name);
      if(*tvec)
         free(*tvec);
      if(*shape)
         free(*shape);
      if(*data)
         free(*data);
      for(i=0;i<*drank;i++){
         if(cnames[0][i])
            free(cnames[0][i]);
         if(coords[0][i])
            free(coords[0][i]);
      }
      if(*cnames)
         free(*cnames);
      if(*coords)
         free(*coords);
      return(1);
   }
   fname=gft_make_file_name(fn);
   ind=gft_open_rd_id(fname);
  if(ind==-1){
    fprintf(stderr,"gft_extract: Unable to open <%s>.\n",fname);
    errno=1;
    return(0);
  }
   fid=dfiles[ind].fid;
   if( ltrace ) {
         fprintf(stderr,"gft_extract: fid: <%d>\n",fid);
         fprintf(stderr,"gft_extract: tmv: <%x>\n",dfiles[ind].tmv);
         fprintf(stderr,"gft_extract: vers: <%d>\n",dfiles[ind].vers);
   }
   if(!gft_read_hdf_name(fname,1,name)){
      fprintf(stderr,"GFT_extract: Can't get grid function name\n");
      errno=20;
      return(0);
   }
   if(dfiles[ind].tmv==NULL){ /* get file info */
      if(ltrace){
         fprintf(stderr,"gft_extract: initializing\n");
      }
      if(SDfileinfo((int32)fid,&nslabs,&gattrs)==-1){
         fprintf(stderr,"gft_extract: Can't get file info.\n");
         errno=2;
         return(0);
      }
      dfiles[ind].nslabs=nslabs;
      if( ltrace ) {
         fprintf(stderr,"gft_extract: nslabs: <%d> gattrs: <%d>\n",nslabs,gattrs);
      }
      /* read first slice */
      sind=0;
     if((sds_id=SDselect((int32)fid,(int32)sind))==-1){
       fprintf(stderr,"gft_extract: Can't select sds number %d.\n",sind);
       errno=3;
       return(0);
     }
     if(SDgetinfo(sds_id,aname,&rank,size,&type,&attrs)==-1){
       fprintf(stderr,"gft_extract: Can't get info about 1st sds\n");
       errno=4;
       return(0);
     }
      dfiles[ind].rank=rank;
      if(type!=DFNT_FLOAT64){
         fprintf(stderr,"gft_extract: data set type is wrong.\n");
         errno=5;
         return(0);
      }
      if(attrs!=1){
         fprintf(stderr,"gft_extract: wrong type of HDF.\n");
         errno=6;
         return(0);
      }
      switch(dfiles[ind].vers){
         case 1 : 
            *ntlev=nslabs-rank;
            sinc=1;
            break;
         case 2 :
            *ntlev=nslabs/(rank+1);
            sinc=rank+1;
            break;
         case 3 :
            *ntlev=nslabs/(rank+1);
            break;
      }
      dfiles[ind].tmv=vec_alloc(*ntlev);
     SDattrinfo((int32)sds_id,(int32)0,aname,&type,&sz);
      if(sz!=1 || type!=DFNT_FLOAT64){
         fprintf(stderr,"gft_extract: wrong type of HDF.\n");
         errno=8;
         return(0);
      }
      if(SDreadattr((int32)sds_id,(int32)0,(void *)&t)==-1){
         fprintf(stderr,"gft_extract: Can't get time value.\n");
         errno=9;
         return(0);
      }
      dfiles[ind].tmv[0]=t;
      SDendaccess((int32)sds_id);
      if(dfiles[ind].vers==3){
         i=0;
         done=0;
         do{
            ++i;
            sprintf(gfname,"%s[%d]",name,i);
            if((sind=SDnametoindex((int32)fid,gfname))!=-1){
               if((sds_id=SDselect((int32)fid,(int32)sind))==-1){
                  fprintf(stderr,"gft_extract: Can't select sds number %d.\n",sind);
                  errno=10;
                  return(0);
               }
               SDreadattr((int32)sds_id,(int32)0,&t);
               dfiles[ind].tmv[i]=t;
               SDendaccess((int32)sds_id);
            }else done=1;
         }while(!done);
      }else{
         for(i=1,sind=rank+1;sind<nslabs;sind+=sinc,i++){
            if((sds_id=SDselect((int32)fid,(int32)sind))==-1){
               fprintf(stderr,"gft_extract: Can't select sds number %d.\n",sind);
               errno=10;
               return(0);
            }
            SDreadattr((int32)sds_id,(int32)0,&t);
            dfiles[ind].tmv[i]=t;
            SDendaccess((int32)sds_id);
         }
      }
      dfiles[ind].nslabs=*ntlev;
   }
   rank=dfiles[ind].rank;
   *ntlev=dfiles[ind].nslabs;
   switch(dfiles[ind].vers){
      case 1: 
         sind=(tlev==1)?0:tlev+rank-1;
         break;
      case 2:
         sind=(tlev-1)*(rank+1);
         break;
      case 3:
         sprintf(gfname,"%s[%d]",name,tlev-1);
         if((sind=SDnametoindex((int32)fid,gfname))==-1){
            fprintf(stderr,"GFT_extract: can't find grid function <%s>\n",gfname);
            errno=21;
            return(0);
         }
         break;
   }
   if(sind < 0 || sind >=nslabs){
      fprintf(stderr,"gft_extract: slice number %d out of range %d--%d\n",
            tlev,1,*ntlev);
      errno=11;
      return(0);
   }
   if(ltrace){
      fprintf(stderr,"gft_extract: rank=%d, nslabs=%d\n",rank,nslabs);
      fprintf(stderr,"gft_extract: version=%d\n",dfiles[ind].vers);
      fprintf(stderr,"gft_extract: *ntlev=%d, sind=%d\n",*ntlev,sind);
    gft_dump_hdf_data(&dfiles[ind]);
   }
   if((sds_id=SDselect((int32)fid,(int32)sind))==-1){
      fprintf(stderr,"gft_extract: Can't select sds number %d.\n",sind);
      errno=12;
      return(0);
   }
   if(SDgetinfo((int32)sds_id,aname,&rank,size,&type,&attrs)==-1){
      fprintf(stderr,"gft_extract: Can't get info about sds %d\n",sind);
      errno=13;
      return(0);
   }
   for(ln=1,i=0;i<rank;i++)
      ln*=size[i];
   *func_name=(char *)malloc(sizeof(char)*(strlen(name)+1));
   if(*func_name==NULL){
      fprintf(stderr,"gft_extract: Can't allocate space for data set name.\n");
      errno=14;
      return(0);
   }
   strcpy(*func_name,name);
   *tvec=vec_alloc(*ntlev);
   rdvcpy(tvec[0],dfiles[ind].tmv,*ntlev);
   *drank=rank;
   *shape=ivec_alloc(rank);
   for(i=0;i<rank;i++){
      shape[0][i]=size[i];
      start[i]=0;
   }
   *cnames=(char **)malloc(sizeof(char *)*rank);
   if(*cnames==NULL){
      fprintf(stderr,"gft_extract: Can't allocate space for *cnames.\n");
      errno=16;
      return(0);
   }
   *coords=(double **)malloc(sizeof(double *)*rank);
   if(*coords==NULL){
      fprintf(stderr,"gft_extract: Can't allocate space for *coords.\n");
      errno=17;
      return(0);
   }   
   for(i=0;i<rank;i++){
      dim_id=SDgetdimid(sds_id,(int32)i);
      if(dim_id!=-1){
         if(SDdiminfo((int32)dim_id,aname,&sz,&type,&dattrs)==-1){
            fprintf(stderr,"gft_extract: Can't get dimension info.\n");
            errno=18;
            return(0);
         }
         if(type==0){
            fprintf(stderr,"gft_extract: Bad dimension %d.\n",i);
            errno=19;
            return(0);
         }
         cnames[0][i]=(char *)malloc(sizeof(char)*(strlen(aname)+1));
         if(cnames[0][i]==NULL){
            fprintf(stderr,"gft_extract: Can't allocate memory for dim name %d.\n",i);
            errno=20;
            return(0);
         }
         st=strrchr(aname,'[');
         if(st){
            strncpy(cnames[0][i],aname,(int)(st-aname));
            cnames[0][i][(int)(st-aname)]=0;
         }else{
            strcpy(cnames[0][i],aname);
         }
         if(sz!=size[i]){
            fprintf(stderr,"gft_extract: Dim %d has size %d while shape=%d.\n",i,sz,
                        size[i]);
            errno=21;
            return(0);
         }
         coords[0][i]=(double *)malloc(sizeof(double)*sz);
         if(coords[0][i]==NULL){
            fprintf(stderr,"gft_extract: Can't allocate memory for dim %d.\n",i);
            errno=22;
            return(0);
         }
         if(SDgetdimscale((int32)dim_id,(void *)coords[0][i])==-1){
            fprintf(stderr,"gft_extract: Can't read dim scale %d.\n",i);
            errno=23;
            return(0);
         }
      }else{
         fprintf(stderr,"gft_extract: Can't get dim id %d.\n",i);
         errno=24;
         return(0);
      }
   }
   if(SDreadattr((int32)sds_id,(int32)0,(void *)&t)==-1){
      fprintf(stderr,"gft_extract: Can't get time value.\n");
      errno=25;
      return(0);
   }
   *time=t;
   *data=vec_alloc(ln);
   if(SDreaddata((int32)sds_id,start,NULL,size,(void *)(*data))==-1){
      fprintf(stderr,"gft_extract: Can't read data.\n");
      errno=27;
      return(0);
   }
   SDendaccess((int32)sds_id);
   return 1;
}

int gft_write_hdf(const char *func_name, /* name of this grid function */
                        double time, /* time of this slice */
                        int *shape, /* shape of data set */
                        int rank, /* rank of data set */
                        double *data ) /* actual data */
{
   int ltrace=0;
   if(ltrace)
      fprintf(stderr,"gft_write: func_name=<%s> time=.16%g shape=%x rank=%d data=%x\n",
                  func_name,time,shape,rank,data);
   return gft_write_hdf_brief(func_name,time,shape,rank,data);
}

int gft_write_hdf_brief(const char *func_name, /* name of this grid function */
                                 double time, /* time of this slice */
                                 int *shape, /* shape of data set */
                                 int rank, /* rank of data set */
                                 double *data ) /* actual data */
{
   char cnames[24];
   double *coords,*c;
   int i,j,sz,ret;
   int ltrace=0;
   
   if(ltrace){
      fprintf(stderr,"gft_write_hdf_brief: rank=%d  shape=( ",rank);
      for(i=0;i<rank;i++)
         fprintf(stderr,"%d ",shape[i]);
      fprintf(stderr,")\n");
   }
   if(rank<=0){
      if(ltrace)
         fprintf(stderr,"gft_write_hdf_brief: calling write_full\n");
      return(gft_write_hdf_full(func_name,time,shape,NULL,rank,NULL,data));
   }
   switch(rank){
      case 1: strcpy(cnames,"x");
         break;
      case 2: strcpy(cnames,"x|y");
         break;
      case 3: strcpy(cnames,"x|y|z");
         break;
      case 4: strcpy(cnames,"x|y|z|w");
         break;
      default: strcpy(cnames,"x|y|z|w|u");
         break;
   }
   for(sz=0,i=0;i<rank;i++){
      sz+=shape[i];
   }
   if(ltrace)
      fprintf(stderr,"gft_write_hdf_brief: trying to malloc %d doubles\n",sz);
   if((coords=(double *)malloc(sizeof(double)*sz))==NULL){
      fprintf(stderr,"gft_write_hdf_brief: can't get memory for coordinates.\n");
      errno=11;
      return(0);
   }
   for(c=coords,i=0;i<rank;i++){
      for(j=0;j<shape[i];j++,c++)
         *c=(double)j;
   }
   ret=gft_write_hdf_full(func_name,time,shape,cnames,rank,coords,data);
   if(coords)
      free(coords);
   return(ret);
}

int gft_write_hdf_bbox(const char *func_name, /* name of this grid function */
                              double time, /* time of this slice */
                              int *shape, /* shape of data set */
                              int rank, /* rank of data set */
                              double *coords, /* values of coordinate bboxes */
                              double *data ) /* actual data */
{
   char cnames[24];
   double *ncoords,*nc,dc;
   int i,j,sz,ret;
   
   if(rank<=0){
      return(gft_write_hdf_full(func_name,time,shape,NULL,rank,NULL,data));
   }
   switch(rank){
      case 1: strcpy(cnames,"x");
         break;
      case 2: strcpy(cnames,"x|y");
         break;
      case 3: strcpy(cnames,"x|y|z");
         break;
      case 4: strcpy(cnames,"x|y|z|w");
         break;
      default: strcpy(cnames,"x|y|z|w|u");
         break;
   }
   for(sz=0,i=0;i<rank;i++){
      sz+=shape[i];
   }
   if((ncoords=(double *)malloc(sizeof(double)*sz))==NULL){
      fprintf(stderr,"gft_write: can't get memory for coordinates.\n");
      errno=11;
      return(0);
   }
   for(nc=ncoords,i=0;i<rank;i++){
      dc=(coords[2*i+1]-coords[2*i])/(shape[i]-1);
      for(j=0;j<shape[i];j++,nc++)
         *nc=j*dc+coords[2*i];
   }
   ret=gft_write_hdf_full(func_name,time,shape,cnames,rank,ncoords,data);
   if(ncoords)
      free(ncoords);
   return(ret);
}

/* rank <=0 and func_name==NULL means close all open files */
/* rank <=0 means close file */
int gft_write_hdf_full(const char *func_name, /* name of this grid function */
                              double time, /* time of this slice */
                              int *shape, /* shape of data set */
                              char *cnames, /* names of coordinates */
                              int rank, /* rank of data set */
                              double *coords, /* values of coordinates */
                              double *data ) /* actual data */
{
  int32 nslabs, gattrs;
  int i,j,k,ind;
  int32 start[MAXRANK];
  int32 lshape[MAXRANK];
  int fid,sds_id,dim_id;
  char name[60],cnm[50],ccnm[50],stp[12],*nm;
  int mode;
   double *crds;
   int ltrace=0;
   
   if(ltrace){
      fprintf(stderr,"gft_write_full: call=%d top=%d\n",call,top);
   }
      
   if(rank<=0){ /* close all files and return */
      if(ltrace)
         fprintf(stderr,"gft_write_full: closing files\n");
      if(func_name == NULL){
         gft_close_all_hdf();
      }else{
/*         nm=gft_make_file_name(func_name);
         gft_close_hdf(nm);*/
         gft_close_hdf(func_name);
      }
      return(1);
   }
/*   nm=gft_make_file_name(func_name); */
   ind=gft_create(func_name);
   if(ind==-1){
      fprintf(stderr,"gft_write_full: unable to open file for <%s>\n",func_name);
      errno=1;
      return(0);
   }
   if(ltrace)
      fprintf(stderr,"gft_write_full: ind=%d\n",ind);
      
   fid=dfiles[ind].fid;
   if(SDfileinfo((int32)fid,&nslabs,&gattrs)==-1){
      fprintf(stderr,"gft_write_full: Can't get file info.\n");
      errno=0;
      return(0);
   }
   if(ltrace)
      fprintf(stderr,"gft_write_full: rank=%d fid=%d\n",rank,fid);
  for(i=0;i<rank;start[i++]=0);
   sprintf(name,"%s[%d]",func_name,dfiles[ind].sdsnum);
   if(ltrace)
      fprintf(stderr,"gft_write_full: name=<%s>\n",name);
   bbh_int_copy_int32(shape,lshape,rank);
   if((sds_id=SDcreate((int32)fid,name,d_type_,(int32)rank,lshape))==-1){
      fprintf(stderr,"gft_write_full: can't create <%s>\n",name);
      errno=2;
      return(0);
   }
   if(ltrace)
      fprintf(stderr,"gft_write_full: created sds\n");
   if(SDwritedata((int32)sds_id,start,NULL,lshape,(void *)data)==-1){
      fprintf(stderr,"gft_write: can't write data to <%s>\n",name);
      errno=3;
      return(0);
   }
   if(ltrace)
      fprintf(stderr,"gft_write_full: wrote data\n");
   if(SDsetattr((int32)sds_id,"t",DFNT_FLOAT64,(int32)1,(void *)&time)==-1){
      fprintf(stderr,"gft_write: can't write time to <%s>\n",name);
      errno=4;
      return(0);
   }
   if(ltrace)
      fprintf(stderr,"gft_write_full: set time\n");
   for(crds=coords,k=0;k<rank;k++){
      if((dim_id=SDgetdimid((int32)sds_id,(int32)k))==-1){
         fprintf(stderr,"gft_write: <%s>, can't get id for dim %d\n",name,k);
         errno=5;
         return(0);
      }
      if(ltrace){
         fprintf(stderr,"gft_write_full: dim_id=%d k=%d\n",dim_id,k);
         fprintf(stderr,"gft_write_full: cnames=<%s>\n",cnames);
      }
      switch(k){
         case 0 :
            sscanf(cnames,"%[^|]",ccnm);
            break;
         case 1 :
            sscanf(cnames,"%*[^|]|%[^|]",ccnm);
            break;
         case 2 :
            sscanf(cnames,"%*[^|]|%*[^|]|%s",ccnm);
            break;
      }
      sprintf(cnm,"%s[%d]",ccnm,dfiles[ind].sdsnum);
      if(ltrace){
         fprintf(stderr,"gft_write_full: ccnm=<%s>\n",ccnm);
         fprintf(stderr,"gft_write_full: cnm=<%s>\n",cnm);
      }
      if(SDsetdimname((int32)dim_id,cnm)==-1){
         fprintf(stderr,"gft_write: <%s>, can't set name for dim %d\n",name,k);
         errno=6;
         return(0);
      }
      if(ltrace){
         fprintf(stderr,"  shape[%d]=%d\n",k,shape[k]);
      }
      if(SDsetdimscale((int32)dim_id,(int32)shape[k],DFNT_FLOAT64,(void *)crds)==-1){
         fprintf(stderr,"gft_write: <%s>, can't set scale for dim %d\n",name,k);
         errno=7;
         return(0);
      }
      crds+=shape[k];
   }
   dfiles[ind].sdsnum++;
   if(ltrace)
      fprintf(stderr,"gft_write_full: exiting\n");
   SDendaccess((int32)sds_id);
/*   if(nm) free(nm);*/
   return(1);
}

/* globals for read routines */
static int32 gRd_shape[MAXRANK];
static char gRd_name[128];

int gft_read_init(const int ind, const char *nm, const int level)
{
   int32 sdind,sds_id;
   int32 rank,gattrs,attrs,type,nslabs;
   char gfname[128],*strip;
   
   if(SDfileinfo((int32)dfiles[ind].fid,&nslabs,&gattrs)==-1){
      fprintf(stderr,"gft_read_init: Can't get file info.\n");
      errno=1;
      return(-1);
   }
   switch(dfiles[ind].vers){
      case 1:
/*         sdind=(level==1)?0:level-1+rank;*/
         sdind=0;
         break;
      case 2:
/*         sdind=(level-1)*(rank+1);*/
         sdind=0;
         break;
      case 3:
         sprintf(gfname,"%s[%d]",nm,level-1);
         if((sdind=SDnametoindex((int32)dfiles[ind].fid,gfname))==-1){
            /*fprintf(stderr,"gft_read_init: unable to find <%s>\n",gfname);*/
            errno=2;
            return(-1);
         }
         break;
   }
   if((sds_id=SDselect((int32)dfiles[ind].fid,sdind))==-1){
/*      fprintf(stderr,"gft_read_init: Can't select sds %d.\n",sdind);*/
      errno=3;
      return(-1);
   }
   if(SDgetinfo((int32)sds_id,gRd_name,&rank,gRd_shape,&type,&attrs)==-1){
      fprintf(stderr,"gft_read_init: Can't get info about sds %d\n",sdind);
      errno=4;
      return(-1);
   }
   strip=strrchr(gRd_name,'[');
   if(strip && *strip=='['){
      *strip='\0';
   }
   dfiles[ind].rank=rank;
   if(type!=DFNT_FLOAT64){
      fprintf(stderr,"gft_read_init: data set type is wrong.\n");
      errno=5;
      return(-1);
   }
   SDendaccess(sds_id);
   return (int)sdind;
}

/* for these five read routines, storage must be pre-allocated */
/* returns the shape of time level level of grid function gf_name */
int gft_read_hdf_shape(const char *gf_name, /* IN: */
                                 const int level, /* IN: */ /* 1-n */
                                 int *shape) /* OUT: */
{
   int ind;
   char *gfn;
   int ltrace=0;

   ind=gft_open_rd(gf_name);
   if(ind==-1){
      fprintf(stderr,"gft_read_shape: unable to open file for <%s>\n",gf_name);
      errno=1;
      return(0);
   }
   gfn=gft_strip_suf(gf_name);
   if(ltrace)
      fprintf(stderr,"gft_read_shape: gf_name=<%s> gfn=<%s>\n",gf_name,gfn);
   if(gft_read_init(ind,gfn,level)==-1){
      fprintf(stderr,"gft_read_shape: Can't get info.\n");
      errno=2;
      return(0);
   }
  bbh_int32_copy_int(gRd_shape,shape,dfiles[ind].rank);
   free(gfn);
   return(1);
}

/* returns the rank of time level level of grid function gf_name */
int gft_read_hdf_rank(const char *gf_name, /* IN: */
                                 const int level, /* IN: */
                                 int *drank) /* OUT: */
{
   int ind;
   char *gfn;
   
   ind=gft_open_rd(gf_name);
   if(ind==-1){
      fprintf(stderr,"gft_read_rank: unable to open file for <%s>\n",gf_name);
      errno=1;
      return(0);
   }
   gfn=gft_strip_suf(gf_name);
   if(gft_read_init(ind,gfn,level)==-1){
      fprintf(stderr,"gft_read_rank: Can't get info.\n");
      errno=2;
      return(0);
   }
   *drank=dfiles[ind].rank;
   free(gfn);
   return(1);
}

/* returns the name of the nth grid function in a file */
int gft_read_hdf_name(const char *file_name, /* IN: */
                                 const int n, /* IN: */
                                 char *name) /* OUT: */
{
   static int msuf=0;
   char suf[5];
/* Local vector for .hdf ... */
  int32 lshape[MAXRANK];
   int32 type,rank,attrs;
   int32 nslabs, gattrs,sds_id,sind;
   char *nm;
   int ind,fid,nnames,ind2;
   char fname[128],*strip;
   char cname[128];
   
   nm=gft_make_file_name(file_name);
   ind=gft_open_rd_id(nm);
   if(ind==-1){
      fprintf(stderr,"gft_read_name: unable to open file <%s>\n",nm);
      errno=1;
      return(0);
   }
   fid=dfiles[ind].fid;
   if(SDfileinfo((int32)fid,&nslabs,&gattrs)==-1){
      fprintf(stderr,"gft_read_name: Can't get file info.\n");
      errno=2;
      return(0);
   }
  if((sds_id=SDselect((int32)fid,(int32)0))==-1){
    fprintf(stderr,"gft_read_name: Can't select 1st sds.\n");
    errno=3;
    return(0);
  }
  if(SDgetinfo(sds_id,fname,&rank,lshape,&type,&attrs)==-1){
    fprintf(stderr,"gft_read_name: Can't get info about 1st sds\n");
    errno=4;
    return(0);
  }
   dfiles[ind].rank=rank;
   if(type!=DFNT_FLOAT64){
      fprintf(stderr,"gft_read_name: data set type is wrong.\n");
      errno=5;
      return(0);
   }
   sind=rank+1;
   strcpy(cname,fname);
   strip=strrchr(cname,'[');
   if(strip && *strip=='[')
      *strip='\0';
   nnames=1;
   while(sind<nslabs && nnames<n){
     if((sds_id=SDselect((int32)fid,sind))==-1){
       fprintf(stderr,"gft_read_name: Can't select sds %d.\n",sind);
       errno=3;
       return(0);
     }
     if(SDgetinfo(sds_id,fname,&rank,lshape,&type,&attrs)==-1){
       fprintf(stderr,"gft_read_name: Can't get info about sds %d\n",sind);
       errno=4;
       return(0);
     }
     strip=strrchr(fname,'[');
      if(strip && *strip=='[')
         *strip='\0';
      if(strcmp(cname,fname)){
         nnames++;
         strcpy(cname,fname);
      }
      sind+=(rank+1);
   }
   if(nnames<n){
      fprintf(stderr,"gft_read_name: bad parameter n=%d\n",n);
      fprintf(stderr,"gft_read_name: file <%s> only contains %d grid functions\n",
              nm,nnames);
      errno=6;
      return(0);
   }
   strcpy(name,cname);
#ifndef _ALL_SOURCE
   dfiles[ind].gfn=strdup(name);
#else
   dfiles[ind].gfn=strdup((char *)name);
#endif
   ind2=gft_search_list(name,dfiles,top+1);
   if(ind!=ind2){
      sprintf(suf,"$%d",msuf++);
      strcat(name,suf);
      free(dfiles[ind].gfn);
#ifndef _ALL_SOURCE
      dfiles[ind].gfn=strdup(name);
#else
      dfiles[ind].gfn=strdup((char *)name);
#endif
   }
   return(1);
}

/* reads info for time level level of grid function gf_name */
int gft_read_hdf_full(const char *gf_name, /* IN: name of this grid function */
                              int level, /* IN: time level */
                              int *shape, /* OUT: shape of data set */
                              char *cnms, /* OUT: names of coordinates */
                              int rank, /* IN: rank of data set */
                              double *time, /* OUT: time value */
                              double *coords, /* OUT: values of coordinates */
                              double *data ) /* OUT: actual data */
{
  int i,j,k,ind,nlev;
  int32 start[MAXRANK];
  int32 sds_id,dim_id,sind;
  char name[60],*strip,*gfn,cnames[4][24];
  int32 sz,type,dattrs;
   double *crds,t;
   int ltrace=0;
   
   if(ltrace){
      fprintf(stderr,"gft_read_full: call=%d top=%d\n",call,top);
   }
      
   ind=gft_open_rd(gf_name);
   if(ind==-1){
      fprintf(stderr,"gft_read_full: unable to open file for <%s>\n",gf_name);
      errno=1;
      return(0);
   }
   
   if(ltrace)
      fprintf(stderr,"gft_read_full: ind=%d\n",ind);
   
   gfn=gft_strip_suf(gf_name);   
   if((sind=gft_read_init(ind,gfn,level))==-1){
/*      fprintf(stderr,"gft_read_full: Can't get info for <%s>.\n",gf_name);*/
      errno=2;
      return(0);
   }
   if(rank!=dfiles[ind].rank){
      fprintf(stderr,"gft_read_full: input rank=%d, while data set rank=%d\n",
             rank,dfiles[ind].rank);
      errno=3;
      return(0);
   }
   if(shape)
      bbh_int32_copy_int(gRd_shape,shape,rank);
  for(i=0;i<rank;i++){
      start[i]=0;
   }
   if((sds_id=SDselect((int32)dfiles[ind].fid,sind))==-1){
      fprintf(stderr,"gft_read_full: Can't select sds number %d.\n",sind);
      errno=12;
      return(0);
   }
   if(coords){
      for(crds=coords,i=0;i<rank;i++){
         dim_id=SDgetdimid(sds_id,(int32)i);
         if(dim_id!=-1){
            if(SDdiminfo(dim_id,name,&sz,&type,&dattrs)==-1){
               fprintf(stderr,"gft_extract: Can't get dimension info.\n");
               errno=18;
               return(0);
            }
            if(type==0){
               fprintf(stderr,"gft_read_full: Bad dimension %d.\n",i);
               errno=19;
               return(0);
            }
            strip=strrchr(name,'[');
            if(strip){
               strncpy(cnames[i],name,(int)(strip-name));
               cnames[i][(int)(strip-name)]=0;
            }else{
               strcpy(cnames[i],name);
            }
            if(sz!=shape[i]){
               fprintf(stderr,"gft_read_full: Dim %d has size %d while shape=%d.\n",
                           i,sz,shape[i]);
               errno=21;
               return(0);
            }
            if(SDgetdimscale(dim_id,(VOIDP)crds)==-1){
               fprintf(stderr,"gft_read_full: Can't read dim scale %d.\n",i);
               errno=23;
               return(0);
            }
         }else{
            fprintf(stderr,"gft_read_full: Can't get dim id %d.\n",i);
            errno=24;
            return(0);
         }
         crds+=shape[i];
      }
   }
   switch(rank){
      case 1 :
         sprintf(cnms,"%s",cnames[0]);
         break;
      case 2 :
         sprintf(cnms,"%s|%s",cnames[0],cnames[1]);
         break;
      case 3 :
         sprintf(cnms,"%s|%s|%s",cnames[0],cnames[1],cnames[2]);
         break;
   }
   
   if(SDreadattr(sds_id,(int32)0,&t)==-1){
      fprintf(stderr,"gft_read_full: Can't get time value.\n");
      errno=25;
      return(0);
   }
   *time=t;
   if(SDreaddata(sds_id,start,NULL,gRd_shape,(void *)(data))==-1){
      fprintf(stderr,"gft_read_full: Can't read data.\n");
      errno=27;
      return(0);
   }
   SDendaccess(sds_id);
   free(gfn);
   if(ltrace) fprintf(stderr,"gft_read_full: exiting normally\n");
   return 1;
}

int gft_read_hdf_brief(const char *gf_name, /* IN: name of this file */
                              int level, /* IN: time level */
                              double *data ) /* OUT: actual data */
{
   double time;
   int rank;
   int ltrace=0;
   
   if(ltrace){
      fprintf(stderr,"gft_read_brief: call=%d top=%d\n",call,top);
   }

   if(gft_read_hdf_rank(gf_name,level,&rank))
      return gft_read_hdf_full(gf_name, level, NULL, NULL, rank, &time, NULL, data);
   else return 0;
}

#endif /* HAVE_HDF */

/* initial data support */

int gft_read_id_str_p(const char *file_name, const char *param_name,
                                    char **param, int nparam)
{
#ifdef HAVE_HDF
  int aind,ind,fid,i;
  char name[256],*nm;
   
#ifndef _ALL_SOURCE
   nm=strdup(file_name);
#else
   nm=strdup((char *)file_name);
#endif
   if((ind=gft_open_rd_id(nm))==-1){
      fprintf(stderr,"gft_read_id_str_p: can't open file <%s>\n",nm);
      errno=1;
      return(0);
   }
   fid=dfiles[ind].fid;
   for(i=0;i<nparam;i++){
      sprintf(name,"%s[%d]",param_name,i);
      if((aind=SDfindattr((int32)fid,name))==-1){
         fprintf(stderr,"gft_read_id_str_p: can't find attribute <%s>\n",name);
         errno=2;
         return(0);
      }
      if(SDreadattr((int32)fid,(int32)aind,(void *)param[i])==-1){
         fprintf(stderr,"gft_read_id_str_p: can't read attribute <%s>\n",name);
         errno=2;
         return(0);
      }
   }
#endif
   return 1;
}

int gft_read_id_int_p(const char *file_name, const char *param_name,
                                    int *param, int nparam)
{
#ifdef HAVE_HDF
  int aind,ind,fid;
  char *nm;
   
#ifndef _ALL_SOURCE
   nm=strdup(file_name);
#else
   nm=strdup((char *)file_name);
#endif
   if((ind=gft_open_rd_id(nm))==-1){
      fprintf(stderr,"gft_read_id_int_p: can't open file <%s>\n",nm);
      errno=1;
      return(0);
   }
   fid=dfiles[ind].fid;
   if((aind=SDfindattr((int32)fid,(char *)param_name))==-1){
      fprintf(stderr,"gft_read_id_int_p: can't find attribute <%s>\n",param_name);
      errno=2;
      return(0);
   }
   if(SDreadattr((int32)fid,(int32)aind,(void *)param)==-1){
      fprintf(stderr,"gft_read_id_int_p: can't read attribute <%s>\n",param_name);
      errno=3;
      return(0);
   }
#endif
   return 1;
}

int gft_read_id_float_p(const char *file_name, const char *param_name,
                                    double *param, int nparam)
{
#ifdef HAVE_HDF
  int aind,ind,fid;
  char *nm;
   
#ifndef _ALL_SOURCE
   nm=strdup(file_name);
#else
   nm=strdup((char *)file_name);
#endif
   if((ind=gft_open_rd_id(nm))==-1){
      fprintf(stderr,"gft_read_id_float_p: can't open file <%s>\n",nm);
      errno=1;
      return(0);
   }
   fid=dfiles[ind].fid;
   if((aind=SDfindattr((int32)fid,(char *)param_name))==-1){
      fprintf(stderr,"gft_read_id_float_p: can't find attribute <%s>\n",param_name);
      errno=2;
      return(0);
   }
   if(SDreadattr((int32)fid,(int32)aind,(void *)param)==-1){
      fprintf(stderr,"gft_read_id_float_p: can't read attribute <%s>\n",param_name);
      errno=3;
      return(0);
   }
#endif
   return 1;
}

int gft_read_2idata(const char *file_name, /* name of idata file */
                              const char *func_name, /* name of this grid function */
                              int *shape, /* shape of data set */
                              int rank, /* rank of data set */
                              double *datanm1,
                              double *datan ) /* actual data */
{
#ifdef HAVE_HDF
  int i,ind;
  int32 sdind;
  int32 start[MAXRANK], lshape[MAXRANK];
  int fid,sds_id;
  char name[60];
   int ltrace=0;
   
   if((ind=gft_open_rd_id(file_name))==-1){
      fprintf(stderr,"gft_read_2idata: can't open file <%s>\n",file_name);
      errno=1;
      return(0);
   }
   fid=dfiles[ind].fid;
  for(i=0;i<rank;start[i++]=0);
   sprintf(name,"%s[0]",func_name);
   if((sdind=SDnametoindex((int32)fid,name))==-1){
      fprintf(stderr,"gft_read_2idata: unable to find <%s> in <%s>\n",name,file_name);
      errno=2;
      return(0);
   }
   if((sds_id=SDselect((int32)fid,(int32)sdind))==-1){
      fprintf(stderr,"gft_read_2idata: can't select sds\n");
      errno=3;
      return(0);
   }
   bbh_int_copy_int32(shape,lshape,rank);
   if(SDreaddata((int32)sds_id,start,NULL,lshape,(void *)datanm1)==-1){
      fprintf(stderr,"gft_read_2idata: can't read data from <%s>\n",name);
      errno=4;
      return(0);
   }
   SDendaccess((int32)sds_id);
   sprintf(name,"%s[1]",func_name);
   if((sdind=SDnametoindex((int32)fid,name))==-1){
      fprintf(stderr,"gft_read_2idata: unable to find <%s> in <%s>\n",name,file_name);
      errno=2;
      return(0);
   }
   if((sds_id=SDselect((int32)fid,(int32)sdind))==-1){
      fprintf(stderr,"gft_read_2idata: can't select sds\n");
      errno=3;
      return(0);
   }
   if(SDreaddata((int32)sds_id,start,NULL,lshape,(void *)datan)==-1){
      fprintf(stderr,"gft_read_2idata: can't read data from <%s>\n",name);
      errno=4;
      return(0);
   }
   SDendaccess((int32)sds_id);
#endif
   return(1);
}

int gft_read_1idata(const char *file_name, /* name of idata file */
                              const char *func_name, /* name of this grid function */
                              int *shape, /* shape of data set */
                              int rank, /* rank of data set */
                              double *datan ) /* actual data */
{
#ifdef HAVE_HDF
  int i,ind;
  int32 sdind;
  int32 start[MAXRANK], lshape[MAXRANK];
  int fid,sds_id;
  char name[60];
   int ltrace=0;
   
   if((ind=gft_open_rd_id(file_name))==-1){
      fprintf(stderr,"gft_read_1idata: can't open file <%s>\n",file_name);
      errno=1;
      return(0);
   }
   fid=dfiles[ind].fid;
  for(i=0;i<rank;start[i++]=0);
   sprintf(name,"%s[0]",func_name);
   if((sdind=SDnametoindex((int32)fid,name))==-1){
      fprintf(stderr,"gft_read_1idata: unable to find <%s> in <%s>\n",name,file_name);
      errno=2;
      return(0);
   }
   if((sds_id=SDselect((int32)fid,(int32)sdind))==-1){
      fprintf(stderr,"gft_read_1idata: can't select sds\n");
      errno=3;
      return(0);
   }
   if(SDreaddata((int32)sds_id,start,NULL,lshape,(void *)datan)==-1){
      fprintf(stderr,"gft_read_1idata: can't read data from <%s>\n",name);
      errno=4;
      return(0);
   }
   SDendaccess((int32)sds_id);
#endif
   return(1);
}

int gft_read_idata(const char *file_name, /* name of idata file */
                              const char *func_name, /* name of this grid function */
                              int *shape, /* shape of data set */
                              int rank, /* rank of data set */
                              double *data) /* actual data */
{
#ifdef HAVE_HDF
  int i,ind;
  int32 sdind;
  int32 start[MAXRANK], lshape[MAXRANK];
  int fid,sds_id;
   int ltrace=0;
   
   if((ind=gft_open_rd_id(file_name))==-1){
      fprintf(stderr,"gft_read_idata: can't open file <%s>\n",file_name);
      errno=1;
      return(0);
   }
   fid=dfiles[ind].fid;
  for(i=0;i<rank;start[i++]=0);
   if((sdind=SDnametoindex((int32)fid,(char *)func_name))==-1){
      fprintf(stderr,"gft_read_idata: unable to find <%s> in <%s>\n",func_name,file_name);
      errno=2;
      return(0);
   }
   if((sds_id=SDselect((int32)fid,(int32)sdind))==-1){
      fprintf(stderr,"gft_read_idata: can't select sds\n");
      errno=3;
      return(0);
   }
   bbh_int_copy_int32(shape,lshape,rank);
   if(SDreaddata((int32)sds_id,start,NULL,lshape,(void *)data)==-1){
      fprintf(stderr,"gft_read_1idata: can't read data from <%s>\n",func_name);
      errno=4;
      return(0);
   }
   SDendaccess((int32)sds_id);
#endif
   return(1);
}

int gft_write_id_str_p(const char *file_name, const char *param_name,
                                    char **param, int nparam)
{
#ifdef HAVE_HDF
  int ind,fid,i;
  char name[256];
   
   if((ind=gft_create_id(file_name))==-1){
      fprintf(stderr,"gft_write_id_str_p: can't open file <%s>\n",file_name);
      errno=1;
      return(0);
   }
   fid=dfiles[ind].fid;
   for(i=0;i<nparam;i++){
      sprintf(name,"%s[%d]",param_name,i);
      if(SDsetattr((int32)fid,name,DFNT_CHAR8,(int32)(strlen(param[i])+1),(void *)param[i])==-1){
         fprintf(stderr,"gft_write_id_str_p: can't set attribute <%s>\n",name);
         errno=2;
         return(0);
      }
   }
#endif
   return 1;
}

int gft_write_id_int_p(const char *file_name, const char *param_name,
                                    int *param, int nparam)
{
#ifdef HAVE_HDF
  int ind,fid;
   
   if((ind=gft_create_id(file_name))==-1){
      fprintf(stderr,"gft_write_id_int_p: can't open file <%s>\n",file_name);
      errno=1;
      return(0);
   }
   fid=dfiles[ind].fid;
   if(SDsetattr((int32)fid,(char *)param_name,DFNT_INT32,(int32)nparam,(void *)param)==-1){
      fprintf(stderr,"gft_write_id_int_p: can't set attribute <%s>\n",param_name);
      errno=2;
      return(0);
   }
#endif
   return 1;
}

int gft_write_id_float_p(const char *file_name, const char *param_name,
                                    double *param, int nparam)
{
#ifdef HAVE_HDF
  int ind,fid;
   
   if((ind=gft_create_id(file_name))==-1){
      fprintf(stderr,"gft_write_id_float_p: can't open file <%s>\n",file_name);
      errno=1;
      return(0);
   }
   fid=dfiles[ind].fid;
   if(SDsetattr((int32)fid,(char *)param_name,DFNT_FLOAT64,(int32)nparam,(void *)param)==-1){
      fprintf(stderr,"gft_write_id_float_p: can't set attribute <%s>\n",param_name);
      errno=2;
      return(0);
   }
#endif
   return 1;
}

int gft_write_2idata(const char *file_name, /* name of idata file */
                           const   char *func_name, /* name of this grid function */
                              int *shape, /* shape of data set */
                              int rank, /* rank of data set */
                              double *datanm1,
                              double *datan ) /* actual data */
{
#ifdef HAVE_HDF
  int i,ind;
  int32 start[MAXRANK];
  int32 lshape[MAXRANK];
  int fid,sds_id;
  char name[60];
   int ltrace=0;
   
   if((ind=gft_create_id(file_name))==-1){
      fprintf(stderr,"gft_write_2idata: can't open file <%s>\n",file_name);
      errno=1;
      return(0);
   }
   fid=dfiles[ind].fid;
  for(i=0;i<rank;start[i++]=0);
   sprintf(name,"%s[0]",func_name);
   bbh_int_copy_int32(shape,lshape,rank);
   if((sds_id=SDcreate((int32)fid,name,d_type_,(int32)rank,lshape))==-1){
      fprintf(stderr,"gft_write_2idata: can't create %s\n",name);
      errno=2;
      return(0);
   }
   if(SDwritedata((int32)sds_id,start,NULL,lshape,(void *)datanm1)==-1){
      fprintf(stderr,"gft_write_2idata: can't write data to %s\n",name);
      errno=3;
      return(0);
   }
   SDendaccess((int32)sds_id);
   sprintf(name,"%s[1]",func_name);
   bbh_int_copy_int32(shape,lshape,rank);
   if((sds_id=SDcreate((int32)fid,name,d_type_,(int32)rank,lshape))==-1){
      fprintf(stderr,"gft_write_2idata: can't create %s\n",name);
      errno=2;
      return(0);
   }
   if(SDwritedata((int32)sds_id,start,NULL,lshape,(void *)datan)==-1){
      fprintf(stderr,"gft_write_2idata: can't write data to %s\n",name);
      errno=3;
      return(0);
   }
   SDendaccess((int32)sds_id);
#endif
   return(1);
}

int gft_write_1idata(const char *file_name, /* name of idata file */
                           const   char *func_name, /* name of this grid function */
                              int *shape, /* shape of data set */
                              int rank, /* rank of data set */
                              double *datan ) /* actual data */
{
#ifdef HAVE_HDF
  int i,ind;
  int32 start[MAXRANK];
  int32 lshape[MAXRANK];
  int fid,sds_id;
  char name[60];
   int ltrace=0;
   
   if((ind=gft_create_id(file_name))==-1){
      fprintf(stderr,"gft_write_1idata: can't open file <%s>\n",file_name);
      errno=1;
      return(0);
   }
   fid=dfiles[ind].fid;
  for(i=0;i<rank;start[i++]=0);
   sprintf(name,"%s[0]",func_name);
   bbh_int_copy_int32(shape,lshape,rank);
   if((sds_id=SDcreate((int32)fid,name,d_type_,(int32)rank,lshape))==-1){
      fprintf(stderr,"gft_write_1idata: can't create %s\n",name);
      errno=2;
      return(0);
   }
   if(SDwritedata((int32)sds_id,start,NULL,lshape,(void *)datan)==-1){
      fprintf(stderr,"gft_write_1idata: can't write data to %s\n",name);
      errno=3;
      return(0);
   }
   SDendaccess((int32)sds_id);
#endif
   return(1);
}

int gft_write_idata(const char *file_name, /* name of idata file */
                              const char *func_name, /* name of this grid function */
                              int *shape, /* shape of data set */
                              int rank, /* rank of data set */
                              double *data) /* actual data */
{
#ifdef HAVE_HDF
  int i,ind;
  int32 lstart[MAXRANK];
  int32 lshape[MAXRANK];
  int fid,sds_id;
   int ltrace=0;
   
   if((ind=gft_create_id(file_name))==-1){
      fprintf(stderr,"gft_write_idata: can't open file <%s>\n",file_name);
      errno=1;
      return(0);
   }
   fid=dfiles[ind].fid;
  for(i=0;i<rank;lstart[i++]=0);
   bbh_int_copy_int32(shape,lshape,rank);
   if((sds_id=SDcreate((int32)fid,(char *)func_name,d_type_,(int32)rank,lshape))==-1){
      fprintf(stderr,"gft_write_idata: can't create %s\n",func_name);
      errno=2;
      return(0);
   }
   if(SDwritedata((int32)sds_id,lstart,NULL,lshape,(void *)data)==-1){
      fprintf(stderr,"gft_write_1idata: can't write data to %s\n",func_name);
      errno=3;
      return(0);
   }
   SDendaccess((int32)sds_id);
#endif
   return(1);
}


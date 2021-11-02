%{
/* grammar for get_param */
/* $Header: /home/cvs/rnpl/src/gpar.y,v 1.1.1.1 2013/07/09 00:38:27 cvs Exp $ */
/* Copyright (c) 1998 by Robert L. Marsa */

/* Modifications 2008-05 to implement get_param_{int,real,str}_v, which return */
/* an arbitrary number of {int's, real's,str's} from a parameter file (vector */
/* specification assumed). */

/* Modifications 2008-05 to rationalize error return codes for get_param_... */
/* routines. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gpar.h"

assignment myasgn;

ivel_list * new_ivel_list();
fvel_list * new_fvel_list();
value_list * new_value_list();
int gotit;

%}

%union {
  int inum;
  double num;
  char *str;
  value val;
  ivel_list *ivl;
  fvel_list *fvl;
  value_list *vl;
  assignment asgn;
}

%token <str> STR IDEN FNAME
%token <num> NUM
%token <inum> INUM ASSIGNOP EQUALS MINUS TIMES DIVIDE COMMA OBRACK CBRACK
%token LONG DOUBLE STRING IVEC FVEC VECTOR SCALAR

%type <asgn> assignment
%type <val> number inumber string ivec fvec file
%type <inum> ival becomes
%type <vl> inumber_list number_list string_list
%type <num> fval

%%

assignment:  LONG { 
              gotit=0; 
            } /* nothing */
  |  DOUBLE { 
      gotit=0; 
    } /* nothing */  
  |  STRING { 
      gotit=0; 
    } /* nothing */  
  |  IVEC { 
      gotit=0; 
    } /* nothing */  
  |  FVEC { 
      gotit=0; 
    } /* nothing */  
  |  LONG IDEN becomes inumber {
      myasgn.name=$2;
      myasgn.val=$4;
      myasgn.type=SCALAR;
      gotit=1;
    }
  |  DOUBLE IDEN becomes number {
      myasgn.name=$2;
      myasgn.val=$4;
      myasgn.type=SCALAR;
      gotit=1;
    }
  |  STRING IDEN becomes string {
      myasgn.name=$2;
      myasgn.val=$4;
      myasgn.type=SCALAR;
      gotit=1;
    }
  |  IVEC IDEN becomes ivec {
      myasgn.name=$2;
      myasgn.val=$4;
      myasgn.type=SCALAR;
      gotit=1;
    }
  |  FVEC IDEN becomes fvec {
      myasgn.name=$2;
      myasgn.val=$4;
      myasgn.type=SCALAR;
      gotit=1;
    }
  |  LONG IDEN becomes file {
      myasgn.name=$2;
      myasgn.val=$4;
      myasgn.type=SCALAR;
      gotit=1;
    }
  |  DOUBLE IDEN becomes file {
      myasgn.name=$2;
      myasgn.val=$4;
      myasgn.type=SCALAR;
      gotit=1;
    }
  |  STRING IDEN becomes file {
      myasgn.name=$2;
      myasgn.val=$4;
      myasgn.type=SCALAR;
      gotit=1;
    }
  |  IVEC IDEN becomes file {
      myasgn.name=$2;
      myasgn.val=$4;
      myasgn.type=SCALAR;
      gotit=1;
    }
  |  FVEC IDEN becomes file {
      myasgn.name=$2;
      myasgn.val=$4;
      myasgn.type=SCALAR;
      gotit=1;
    }
  |  LONG IDEN becomes OBRACK inumber_list CBRACK {
      myasgn.name=$2;
      myasgn.vl=$5;
      myasgn.type=VECTOR;
      gotit=1;
    }
  |  DOUBLE IDEN becomes OBRACK number_list CBRACK {
      myasgn.name=$2;
      myasgn.vl=$5;
      myasgn.type=VECTOR;
      gotit=1;
    }
  |  STRING IDEN becomes OBRACK string_list CBRACK {
      myasgn.name=$2;
      myasgn.vl=$5;
      myasgn.type=VECTOR;
      gotit=1;
    }
  ;

becomes: ASSIGNOP | EQUALS
  ;
  
inumber:  INUM {
          $$.type=INUM;
          $$.data.inum=$1;
        }
  |  MINUS INUM {
      $$.type=INUM;
      $$.data.inum=-$2;
    }
  ;
  
number:  NUM  {
          $$.type=NUM;
          $$.data.num=$1;
        }
  |  MINUS NUM {
      $$.type=NUM;
      $$.data.num=-$2;
    }
  |  INUM {
      $$.type=NUM;
      $$.data.num=(double)$1;
    }
  |  MINUS INUM {
      $$.type=NUM;
      $$.data.num=(double)(-$2);
    }
  ;
  
string:  STR {
          $$.type=STR;
          $$.data.str=$1;
        }
  ;
  
ivec:  ival {
        $$.type=IVEC;
        $$.data.ivec=new_ivel_list();
        $$.data.ivec->ivel[0]=$1;
        $$.data.ivec->ivel[1]=-2;
        $$.data.ivec->ivel[2]=1;
        $$.data.ivec->len=1;
      }
  |  ival MINUS ival {
      $$.type=IVEC;
      $$.data.ivec=new_ivel_list();
      $$.data.ivec->ivel[0]=$1;
      $$.data.ivec->ivel[1]=$3;
      $$.data.ivec->ivel[2]=1;
      $$.data.ivec->len=1;
    }
  |  ival MINUS ival DIVIDE INUM {
      $$.type=IVEC;
      $$.data.ivec=new_ivel_list();
      $$.data.ivec->ivel[0]=$1;
      $$.data.ivec->ivel[1]=$3;
      $$.data.ivec->ivel[2]=$5;
      $$.data.ivec->len=1;
    }
  |  ivec COMMA ival {
      $$.type=IVEC;
      $$.data.ivec=new_ivel_list();
      $$.data.ivec->ivel[0]=$3;
      $$.data.ivec->ivel[1]=-2;
      $$.data.ivec->ivel[2]=1;
      $$.data.ivec->next=$1.data.ivec;
      $$.data.ivec->len=1+$1.data.ivec->len;
    }
  |  ivec COMMA ival MINUS ival {
      $$.type=IVEC;
      $$.data.ivec=new_ivel_list();
      $$.data.ivec->ivel[0]=$3;
      $$.data.ivec->ivel[1]=$5;
      $$.data.ivec->ivel[2]=1;
      $$.data.ivec->next=$1.data.ivec;
      $$.data.ivec->len=1+$1.data.ivec->len;
    }
  |  ivec COMMA ival MINUS ival DIVIDE INUM {
      $$.type=IVEC;
      $$.data.ivec=new_ivel_list();
      $$.data.ivec->ivel[0]=$3;
      $$.data.ivec->ivel[1]=$5;
      $$.data.ivec->ivel[2]=$7;
      $$.data.ivec->next=$1.data.ivec;
      $$.data.ivec->len=1+$1.data.ivec->len;
    }
  ;

fvec:  fval {
        $$.type=FVEC;
        $$.data.fvec=new_fvel_list();
        $$.data.fvec->fvel[0]=$1;
        $$.data.fvec->fvel[1]=$1;
        $$.data.fvec->fvel[2]=1;
        $$.data.fvec->len=1;
      }
  |  fval MINUS fval DIVIDE INUM {
      $$.type=FVEC;
      $$.data.fvec=new_fvel_list();
      $$.data.fvec->fvel[0]=$1;
      $$.data.fvec->fvel[1]=$3;
      $$.data.fvec->fvel[2]=$5;
      $$.data.fvec->len=1;
    }
  | fvec COMMA fval {
      $$.type=FVEC;
      $$.data.fvec=new_fvel_list();
      $$.data.fvec->fvel[0]=$3;
      $$.data.fvec->fvel[1]=$3;
      $$.data.fvec->fvel[2]=1;
      $$.data.fvec->next=$1.data.fvec;
      $$.data.fvec->len=1+$1.data.fvec->len;
    }
  |  fvec COMMA fval MINUS fval DIVIDE INUM {
      $$.type=FVEC;
      $$.data.fvec=new_fvel_list();
      $$.data.fvec->fvel[0]=$3;
      $$.data.fvec->fvel[1]=$5;
      $$.data.fvec->fvel[2]=$7;
      $$.data.fvec->next=$1.data.fvec;
      $$.data.fvec->len=1+$1.data.fvec->len;
    }
  ;

file: FNAME {
        $$.type=FNAME;
        $$.data.str=$1;
      }
  ;

inumber_list:  inumber {
                $$=new_value_list();
                $$->val=$1;
                $$->len=1;
              }
  | inumber_list inumber {
      $$=new_value_list();
      $$->val=$2;
      $$->next=$1;
      $$->len=1+$1->len;
    }
  ;

number_list:  number {
                $$=new_value_list();
                $$->val=$1;
                $$->len=1;
              }
  | number_list number {
      $$=new_value_list();
      $$->val=$2;
      $$->next=$1;
      $$->len=1+$1->len;
    }
  ;

string_list:  string {
                $$=new_value_list();
                $$->val=$1;
                $$->len=1;
              }
  | string_list string {
      $$=new_value_list();
      $$->val=$2;
      $$->next=$1;
      $$->len=1+$1->len;
    }
  ;

ival: INUM {
        $$=$1;
      }
  |  TIMES {
      $$=-1;
    }
  ;
  
fval:  NUM  {
          $$=$1;
        }
  |  MINUS NUM {
      $$=-$2;
    }
  |  INUM {
      $$=(double)$1;
    }
  |  MINUS INUM {
      $$=(double)(-$2);
    }
  ;
  
%%

ivel_list * new_ivel_list()
{
  ivel_list *iv;
  
  iv=(ivel_list *)malloc(sizeof(ivel_list));
  if(iv==NULL){
    fprintf(stderr,"new_ivel_list: can't get memory.\n");
    exit(0);
  }
  iv->next=NULL;
  iv->len=0;
  return iv;
}

fvel_list * new_fvel_list()
{
  fvel_list *iv;
  
  iv=(fvel_list *)malloc(sizeof(fvel_list));
  if(iv==NULL){
    fprintf(stderr,"new_fvel_list: can't get memory.\n");
    exit(0);
  }
  iv->next=NULL;
  iv->len=0;
  return iv;
}

value_list * new_value_list()
{
  value_list *vl;
  
  vl=(value_list *)malloc(sizeof(value_list));
  if(vl==NULL){
    fprintf(stderr,"new_value_list: can't get memory.\n");
    exit(0);
  }
  vl->next=NULL;
  vl->len=0;
  return vl;
}

#define IVEL 4
#define FVEL 4

char *sg_param_buf;
char *sg_param_ptr;
char *sg_param_end;
int sg_param_type,first_tok;
int inputerror;

/* Set size < 1 to determine size from parameter definition (vector assumed), which 
   is returned in prsize ... */

int sget_param(const char *string, const char *name,
               char *type, int size, void *p, int cs) 
{
   int *q;
   double *r;
   value_list *vl;
   ivel_list *il;
   fvel_list *fl;
   int found=0,i;
   int typeerr=0;
   int ltrace=0;
   int varsize;

   int l_size;
  
   if(ltrace) {
      fprintf(stderr,"string=<%s> name=<%s> type=<%s> size=%d cs=%d\n",
         string,name,type,size,cs);
	}

   if( size < 0 ) {
      l_size = -size;
      varsize = 1;
   } else {
      varsize = 0;
   }
   l_size = (size >= 0 ) ? size : -size;
            
   sg_param_buf=(char *)string;
   sg_param_ptr=sg_param_buf;
   sg_param_end=sg_param_buf+strlen(string);

   if(!strcmp(type,"long"))
      sg_param_type=LONG;
   else if(!strcmp(type,"double"))
      sg_param_type=DOUBLE;
   else if(!strcmp(type,"string"))
      sg_param_type=STRING;
   else if(!strcmp(type,"ivec"))
      sg_param_type=IVEC;
   else if(!strcmp(type,"fvec"))
      sg_param_type=FVEC;
   else {
      fprintf(stderr,"sget_param: error: unknown type <%s>\n",type);
      return(-1);
   }

   first_tok=1;
   inputerror=0;
   gotit=0;
   gparparse();
  
   if(ltrace)
      fprintf(stderr,"sget_param: gotit=%d inputerror=%d\n",gotit,inputerror);
  
   if(!inputerror && gotit){
      if(ltrace)
         fprintf(stderr,"sget_param: myasgn.name=<%s> name=<%s>\n",myasgn.name,name);
      if((cs && !strcmp(myasgn.name,name)) || (!cs && !strcmp_nc(myasgn.name,name))){
         if(ltrace)
            fprintf(stderr,"sget_param: found <%s>\n",name);
         found=1;
         switch(myasgn.type){
         case SCALAR:
            if(myasgn.val.type!=FNAME && l_size!=1 && (sg_param_type==LONG || 
               sg_param_type==DOUBLE || sg_param_type==STRING)){
               fprintf(stderr,"sget_param: expecting a vector value for <%s>\n",name);
               return(-1);
            }
            if(ltrace) fprintf(stderr,"sget_param: l_size=%d\n",l_size);
            switch(sg_param_type){
            case LONG:
               if(myasgn.val.type==INUM)
                  *((int *)p)=myasgn.val.data.inum;
               else if(myasgn.val.type==FNAME)
                  return(read_ascii_int(myasgn.val.data.str,(int *)p,l_size));
               else typeerr=1;
               break;
            case DOUBLE:
               if(myasgn.val.type==INUM)
                  *((double *)p)=(double)myasgn.val.data.inum;
               else if(myasgn.val.type==NUM)
                  *((double *)p)=myasgn.val.data.num;
               else if(myasgn.val.type==FNAME){
                  if(ltrace)
                     fprintf(stderr,"sget_param: FNAME=<%s>\n",myasgn.val.data.str);
                  return(read_ascii_double(myasgn.val.data.str,(double *)p,l_size));
               }else typeerr=1;
               break;
            case STRING:
               if(myasgn.val.type==STR){
                  if(*((char **)p))
                     free(*((char **)p));
                  *((char **)p)=cvec_alloc(strlen(myasgn.val.data.str)+1);
                  strcpy(*((char **)p),myasgn.val.data.str);
                  free(myasgn.val.data.str);
               }else if(myasgn.val.type==FNAME){
                  return(read_ascii_string(myasgn.val.data.str,(char **)p,l_size));
               }else typeerr=1;
               break;
            case IVEC:
               if(ltrace)
                  fprintf(stderr,"sget_param: looking for IVEC\n");
               if(myasgn.val.type==IVEC){
                  if(ltrace) fprintf(stderr,"sget_param: found IVEC\n");
                  if(cs){
                     if(ltrace)
                        fprintf(stderr,"sget_param: len=%d\n",myasgn.val.data.ivec->len);
                     if(l_size<IVEL*myasgn.val.data.ivec->len+1){
                        if(*((int **)p)!=NULL) free(*((int **)p));
								if(ltrace) fprintf(stderr,"sget_param: Allocating IVEC storage\n");
                        *((int **)p)=ivec_alloc(IVEL*myasgn.val.data.ivec->len+1);
                     }
                     q=(int *)*((int **)p);
                  }else{
                     if(l_size<IVEL*myasgn.val.data.ivec->len+1){
                        fprintf(stderr,"sget_param: ivec too long for FORTRAN data type.\n");
                        return(-1);
                     }
                     q=(int *)p;
                  }
                  if(ltrace)
                     fprintf(stderr,"sget_param: setting ivec len to %d\n",
                        myasgn.val.data.ivec->len);
                  q[0]=myasgn.val.data.ivec->len;
                  for(i=myasgn.val.data.ivec->len-1,il=myasgn.val.data.ivec;i>=0;i--,il=il->next){
                     rivcpy(q+i*IVEL+1,il->ivel,3);
                     q[IVEL*i+1+3]=0;
                  }
               }else if(myasgn.val.type==FNAME){
                  return(read_ascii_ivec(myasgn.val.data.str,(int **)p,l_size,cs));
               }else typeerr=1;
               break;
            case FVEC:
               if(myasgn.val.type==FVEC){
                  if(cs){
                     if(l_size<FVEL*myasgn.val.data.fvec->len+1){
                        if(*((double **)p)!=NULL) free(*((double **)p));
                        *((double **)p)=vec_alloc(FVEL*myasgn.val.data.fvec->len+1);
                     }
                     r=(double *)*((double **)p);
                  }else{
                     if(l_size<FVEL*myasgn.val.data.fvec->len+1){
                        fprintf(stderr,"sget_param: fvec too long for FORTRAN data type.\n");
                        return(-1);
                     }
                     r=(double *)p;
                  }
                  r[0]=(double)myasgn.val.data.fvec->len;
                  for(i=myasgn.val.data.fvec->len-1,fl=myasgn.val.data.fvec;i>=0;i--,fl=fl->next){
                     rdvcpy(r+i*FVEL+1,fl->fvel,3);
                     r[FVEL*i+1+3]=0;
                  }
               }else if(myasgn.val.type==FNAME){
                  return(read_ascii_fvec(myasgn.val.data.str,(double **)p,l_size,cs));
               }else typeerr=1;
               break;
            }
            break;
         case VECTOR:
            if( varsize ) {
               switch(sg_param_type){
               case LONG:
                  *Gpar_prsize = myasgn.vl->len;
                  for(vl=myasgn.vl,i=*Gpar_prsize-1;i>=0;i--,vl=vl->next){
                     if(vl->val.type!=INUM) {
                        typeerr = 1;
                        break;
                     }
                  }
                  if( !typeerr ) {
                     if( *Gpar_pr = (void *) malloc(*Gpar_prsize * sizeof(int)) ) {
                        for( vl=myasgn.vl, i=*Gpar_prsize-1; i>=0; i--, vl=vl->next ) {
                           if( ltrace ) fprintf(stderr,"Assigning v[%d]=%d\n",i,vl->val.data.inum);
                           ((int *) (*Gpar_pr))[i] = vl->val.data.inum;
                        }
                     } else {
                        fprintf(stderr,"sget_param: Error allocating %d int's.\n",*Gpar_prsize);
                        *Gpar_prsize = 0;
                     }
                  } else {
                     fprintf(stderr,"sget_param: Error parsing int in vector assignment.\n");
                     *Gpar_prsize = 0;
                  }
                  break;
               case DOUBLE:
                  *Gpar_prsize = myasgn.vl->len;
                  for(vl=myasgn.vl,i=*Gpar_prsize-1;i>=0;i--,vl=vl->next){
                     if(vl->val.type!=NUM && vl->val.type!=INUM) {
                        typeerr = 1;
                        break;
                     }
                  }
                  if( !typeerr ) {
                     if( *Gpar_pr = (void *) malloc(*Gpar_prsize * sizeof(double)) ) {
                        for( vl=myasgn.vl, i=*Gpar_prsize-1; i>=0; i--, vl=vl->next ) {
                           if(vl->val.type==NUM) {
                              if( ltrace ) fprintf(stderr,"Assigning v[%d]=%g\n",i,vl->val.data.num);
                              ((double *) (*Gpar_pr))[i] = vl->val.data.num;
                           } else if(vl->val.type==INUM) {
                              if( ltrace ) fprintf(stderr,"Assigning v[%d]=%d\n",i,vl->val.data.inum);
                              ((double *) (*Gpar_pr))[i] = vl->val.data.inum;
                           }
                        }
                     } else {
                        fprintf(stderr,"sget_param: Error allocating %d double's.\n",*Gpar_prsize);
                        *Gpar_prsize = 0;
                     }
                  } else {
                     fprintf(stderr,"sget_param: Error parsing double in vector assignment.\n");
                     *Gpar_prsize = 0;
                  }
                  break;
               case STRING:
                  *Gpar_prsize = myasgn.vl->len;
                  for(vl=myasgn.vl,i=*Gpar_prsize-1;i>=0;i--,vl=vl->next){
                     if(vl->val.type!=NUM && vl->val.type!=STR) {
                        typeerr = 1;
                        break;
                     }
                  }
                  if( !typeerr ) {
                     if( *Gpar_pr = (void *) malloc(*Gpar_prsize * sizeof(char *)) ) {
                        for( vl=myasgn.vl, i=*Gpar_prsize-1; i>=0; i--, vl=vl->next ) {
                           if( ltrace ) fprintf(stderr,"Assigning v[%d]='%s'\n",i,vl->val.data.str);
                           ((char **) (*Gpar_pr))[i] = strdup(vl->val.data.str);
                        }
                     } else {
                        fprintf(stderr,"sget_param: Error allocating %d char *'s.\n",*Gpar_prsize);
                        *Gpar_prsize = 0;
                     }
                  } else {
                     fprintf(stderr,"sget_param: Error parsing string in vector assignment.\n");
                     *Gpar_prsize = 0;
                  }
                  break;
               }
            } else {
               switch(sg_param_type){
               case LONG:
                  if(l_size!=myasgn.vl->len){
                     fprintf(stderr,"sget_param: expecting length %d, but read %d\n",l_size,myasgn.vl->len);
                     return(-2);
                  }
                  for(vl=myasgn.vl,i=l_size-1;i>=0;i--,vl=vl->next){
                     if(vl->val.type==INUM)
                        ((int *)p)[i]=vl->val.data.inum;
                     else typeerr=1;
                  }
                  break;
               case DOUBLE:
                  if(l_size!=myasgn.vl->len){
                     fprintf(stderr,"sget_param: expecting length %d, but read %d\n",l_size,myasgn.vl->len);
                     return(-2);
                  }
                  for(vl=myasgn.vl,i=l_size-1;i>=0;i--,vl=vl->next){
                     if(vl->val.type==NUM)
                        ((double *)p)[i]=vl->val.data.num;
                     else if(vl->val.type==INUM)
                        ((double *)p)[i]=(double)(vl->val.data.inum);
                     else typeerr=1;
                  }
                  break;
               case STRING:
                  if(l_size!=myasgn.vl->len){
                     fprintf(stderr,"sget_param: expecting length %d, but read %d\n",l_size,myasgn.vl->len);
                     return(-2);
                  }
                  for(vl=myasgn.vl,i=l_size-1;i>=0;i--,vl=vl->next){
                     if(vl->val.type==STR){
                        if(*((char **)p+i)!=NULL)
                           free(*((char **)p+i));
                        *((char**)p+i)=cvec_alloc(strlen(vl->val.data.str)+1);
                        strcpy(*((char**)p+i),vl->val.data.str);
                     }else typeerr=1;
                  }
                  break;
               }
            }
            break;
         }
         if(typeerr){
            fprintf(stderr,"sget_param: read bad value for <%s>.  Expected %s\n",name,type);
            return(-1);
         }
      }
   }
   if(!found)
      return(0);
   return(1);
}


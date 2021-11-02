/* sdf.c */
/* I/O routines for raw data files */
/* Copyright (c) 1997 by Robert L. Marsa and Matthew W. Choptuik */
/* Some code borrowed from bbhio.c, written by Matthew W. Choptuik */
/* $Header: /home/cvs/rnpl/src/sdf.c,v 1.5 2014/07/14 23:01:47 cvs Exp $ */
/* Modified by MWC, July 2001, added utility function 'gft_verify_file'  */
/* Modified by MWC, March 2010, trying to get direct to DV working */
/* 2012-04-02: Modified by MWC gft_make_sdf_name() so that '+' is a valid character in name. */
/* 2012-04-19: Added utility function 'gft_is_sdf_file' */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <bbhutil.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "sdf_priv.h"
#include "cliser.h"
#include "DVault.h"

static char SDFC1[] = {"x"};
static char SDFC2[] = {"x|y"};
static char SDFC3[] = {"x|y|z"};
static char SDFC4[] = {"x|y|z|w"};
static char SDFC5[] = {"x|y|z|w|v"};
static char *SDFCnames[] = {SDFC1, SDFC2, SDFC3, SDFC4, SDFC5};
static double BBH_bbox[]={-1.0,1.0,-1.0,1.0,-1.0,1.0,-1.0,1.0};

static gft_sdf_file_data *gsfd_root;

static enum { SDF_Single, SDF_Multi } sdf_file_mode=SDF_Multi;
static char sdf_global_fname[BBH_MAXSTRINGLEN];

gft_sdf_file_index **gsfipvec_alloc(const int size)
{
   gft_sdf_file_index **gp;
   
   gp=(gft_sdf_file_index **)malloc(size*sizeof(gft_sdf_file_index *));
   if(gp==NULL){
      fprintf(stderr,"gsfipvec_alloc(%d): can't allocate memory\n",size);
      exit(0);
   }
   return gp;
}

/* dynamic array routines */
dynarray *new_dynarray(void)
{
#define DCHUNK 512
   dynarray *da;
   
   da=(dynarray *)malloc(sizeof(dynarray));
   if(da==NULL){
      fprintf(stderr,"new_dynarray: can't allocate memory\n");
   }else{
      da->data=vec_alloc_n(DCHUNK,"new_dynarray:");
      da->clen=0;
      da->mlen=DCHUNK;
      da->csize=DCHUNK;
   }
   return da;
#undef DCHUNK
}

void delete_dynarray(dynarray *da)
{
   if(da){
      if(da->data) free(da->data);
      free(da);
   }
}

void add_item(dynarray *da, const double it)
{
   double *td;
   int i;
   
   if(da){
      if(da->clen>=da->mlen){
         td=vec_alloc_n(da->mlen+da->csize,"add_item");
         for(i=0;i<da->clen;i++)
            td[i]=da->data[i];
         free(da->data);
         da->data=td;
         da->mlen+=da->csize;
      }
      da->data[da->clen]=it;
      da->clen++;
   }
}

double get_item(dynarray *da, const int i)
{
   double d;
   if(da){
      if(i>=0 && i<da->clen)
         d=da->data[i];
      else d=0.0;
   }else d=0.0;
   return d;
}

int dynarray_len(dynarray *da)
{
   int l;
   
   if(da) l=da->clen;
   else l=-1;
   return l;
}

gsfiarray *new_gsfiarray(void)
{
#define DCHUNK 512
   gsfiarray *da;
   
   da=(gsfiarray *)malloc(sizeof(gsfiarray));
   if(da==NULL){
      fprintf(stderr,"new_gsfiarray: can't allocate memory\n");
   }else{
      da->data=gsfipvec_alloc(DCHUNK);
      da->clen=0;
      da->mlen=DCHUNK;
      da->csize=DCHUNK;
   }
   return da;
#undef DCHUNK
}

void delete_gsfiarray(gsfiarray *da)
{
   int i;
   if(da){
      for(i=0;i<da->clen;i++)
         gsfi_delete(da->data[i]);
      if(da->data) free(da->data);
      free(da);
   }
}

void add_gsfiitem(gsfiarray *da, gft_sdf_file_index *it)
{
   gft_sdf_file_index **td;
   int i;
   
   if(da){
      if(da->clen>=da->mlen){
         td=gsfipvec_alloc(da->mlen+da->csize);
         for(i=0;i<da->clen;i++)
            td[i]=da->data[i];
         free(da->data);
         da->data=td;
         da->mlen+=da->csize;
      }
      da->data[da->clen]=it;
      da->clen++;
   }
}

gft_sdf_file_index *get_gsfiitem(gsfiarray *da, const int i)
{
   gft_sdf_file_index *d;
   int ltrace=0;
   
   if(ltrace){
      fprintf(stderr,"get_gsfiitem(%p,%d)\n",da,i);
      if(da)
         fprintf(stderr,"  da->clen=%d, da->mlen=%d, da->csize=%d\n",
              da->clen,da->mlen,da->csize);
   }
   if(da){
      if(i>=0 && i<da->clen)
         d=da->data[i];
      else d=NULL;
   }else d=NULL;
   return d;
}

int gsfiarray_len(gsfiarray *da)
{
   int l;
   
   if(da) l=da->clen;
   else l=-1;
   return l;
}

/* gft_sdf_file_index routines */
void gsfi_insert(gft_sdf_file_index *root, gft_sdf_file_index *gp)
{
   int cmp;
   gft_sdf_file_index *gq;
   
   gq=root;
   if(gq){
      cmp=strcmp(gp->name,gq->name);
      if(cmp<0){
         if(gq->left)
            gsfi_insert(gq->left,gp);
         else gq->left=gp;
      }else if(cmp>0){
         if(gq->right)
            gsfi_insert(gq->right,gp);
         else gq->right=gp;
      }else if(gp->time < gq->time){
         if(gq->left)
            gsfi_insert(gq->left,gp);
         else gq->left=gp;
      }else if(gp->time > gq->time){
         if(gq->right)
            gsfi_insert(gq->right,gp);
         else gq->right=gp;
      }else if(gq->right)
         gsfi_insert(gq->right,gp);
      else gq->right=gp;
   }/* else gsfd_root=gp; */
}

gft_sdf_file_index *gsfi_new(void)
{
   gft_sdf_file_index *gp;
   
   gp=malloc(sizeof(gft_sdf_file_index));
   if(gp==NULL){
      fprintf(stderr,"Can't allocate new gft_sdf_file_index\n");
     exit(0);
   }
   gp->position=0;
   gp->name=NULL;
   gp->tag=NULL;
   gp->rank=0;
   gp->shape=NULL;
   gp->time=0.0;
   gp->bbox=NULL;
   gp->left=NULL;
   gp->right=NULL;
   return gp;
}

void gsfi_delete(gft_sdf_file_index *gp)
{
   if(gp){
      gsfi_delete(gp->left);
      gp->left=NULL;
      gsfi_delete(gp->right);
      gp->right=NULL;
      if(gp->name)
         free(gp->name);
      gp->name=NULL;
      if(gp->tag)
         free(gp->tag);
      gp->tag=NULL;
      if(gp->shape)
         free(gp->shape);
      if(gp->bbox)
         free(gp->bbox);
      free(gp);
   }
}

gft_sdf_file_data *gsfd_parent(gft_sdf_file_data *root, gft_sdf_file_data *gp)
{
   gft_sdf_file_data *gq,*p;
   int cmp;
   
   gq=root;
   if(gq && gq!=gp){
      cmp=strcmp(gp->gfname,gq->gfname);
      if(cmp<0){
         if(gq->left!=gp)
            p=gsfd_parent(gq->left,gp);
         else p=gq;
      }else if(cmp>0){
         if(gq->right!=gp)
            p=gsfd_parent(gq->right,gp);
         else p=gq;
      }
   }else p=NULL;
   return p;
}

/*
  returns the parent (p) of the successor to gp
  the successor is p->left unless p==gp in which case it is p->right
  assumes gp is not NULL and gp->right is not NULL
*/
gft_sdf_file_data *gsfd_successor(gft_sdf_file_data *gp)
{
   gft_sdf_file_data *p;
   
   if(gp->right->left){
      for(p=gp->right;p->left->left;p=p->left);
   }else p=gp;
   return p;
}

gft_sdf_file_data *gsfd_rotate_r(gft_sdf_file_data *p, gft_sdf_file_data *g)
{
   gft_sdf_file_data *r=NULL;
   
   if(p==NULL){
      gsfd_root=g->left;
      g->left=gsfd_root->right;
      gsfd_root->right=g;
      r=gsfd_root;
   }else if(p->left==g){
      p->left=g->left;
      g->left=p->left->right;
      p->left->right=g;
      r=p->left;
   }else if(p->right==g){
      p->right=g->left;
      g->left=p->right->right;
      p->right->right=g;
      r=p->right;
   }
   return r;
}

gft_sdf_file_data *gsfd_rotate_l(gft_sdf_file_data *p, gft_sdf_file_data *g)
{
   gft_sdf_file_data *r=NULL;
   
   if(p==NULL){
      gsfd_root=g->right;
      g->right=gsfd_root->left;
      gsfd_root->left=g;
      r=gsfd_root;
   }else if(p->left==g){
      p->left=g->right;
      g->right=p->left->left;
      p->left->left=g;
      r=p->left;
   }else if(p->right==g){
      p->right=g->right;
      g->right=p->right->left;
      p->right->left=g;
      r=p->right;
   }
   return r;
}

void gsfd_insert(gft_sdf_file_data *root, gft_sdf_file_data *gp)
{
   int cmp;
   gft_sdf_file_data *gq;
   
   gq=root;
   if(gq){
      cmp=strcmp(gp->gfname,gq->gfname);
      if(cmp<0){
         if(gq->left)
            gsfd_insert(gq->left,gp);
         else gq->left=gp;
      }else if(cmp>0){
         if(gq->right)
            gsfd_insert(gq->right,gp);
         else gq->right=gp;
      }
   }else gsfd_root=gp;
}

gft_sdf_file_data *gsfd_remove(gft_sdf_file_data *root, gft_sdf_file_data *gp)
{
  gft_sdf_file_data *gq,*p,*s;
   
   if(gp==root){
      if(!gp->left && !gp->right){
         p=NULL;
      }else if(!gp->right){
         p=gsfd_rotate_r(NULL,gp);
      }else p=gsfd_rotate_l(NULL,gp);
   }else p=gsfd_parent(root,gp);
   if(p==NULL){
      gsfd_root=NULL;
   }else{
      if(gp->right){
         gq=gsfd_successor(gp);
         if(gq==gp){
            s=gq->right;
            gq->right=s->right;
         }else{
            s=gq->left;
            gq->left=s->right;
         }
         if(p->left==gp){
            p->left=s;
         }else{
            p->right=s;
         }
         s->left=gp->left;
         s->right=gp->right;
         gp->left=NULL;
         gp->right=NULL;
      }else{
         if(p->left==gp){
            p->left=gp->left;
         }else{
            p->right=gp->left;
         }
         gp->left=NULL;
      }
   }
   return gp;
}

gft_sdf_file_data *gsfd_find(gft_sdf_file_data *root, const char *gfname)
{
   int cmp;
   gft_sdf_file_data *gq,*fnd;
   
   gq=root;
   fnd=NULL;
   if(gq){
      cmp=strcmp(gfname,gq->gfname);
      if(cmp<0){
         fnd=gsfd_find(gq->left,gfname);
      }else if(cmp>0){
         fnd=gsfd_find(gq->right,gfname);
      }else{
         fnd=gq;
      }
   }
   return fnd;
}

gft_sdf_file_data *gsfd_new(void)
{
   gft_sdf_file_data *gp;
   
   gp=malloc(sizeof(gft_sdf_file_data));
   if(gp==NULL){
      fprintf(stderr,"Can't allocate new gft_sdf_file_data\n");
     exit(0);
   }
   gp->gfname=NULL;
   gp->fname=NULL;
   gp->fp=NULL;
   gp->state=StrClosed;
   gp->mtime=0;
   gp->changed=0;
   gp->left=NULL;
   gp->right=NULL;
   gp->tarray=NULL;
   gp->idx=NULL;
   return gp;
}

void gsfd_delete(gft_sdf_file_data *gp)
{
   if(gp){
      gsfd_delete(gp->left);
      gp->left=NULL;
      gsfd_delete(gp->right);
      gp->right=NULL;
      gsfd_close(gp);
      if(gp->gfname)
         free(gp->gfname);
      gp->gfname=NULL;
      if(gp->fname)
         free(gp->fname);
      gp->fname=NULL;
      delete_dynarray(gp->tarray);
      free(gp);
   }
}

void gsfd_close(gft_sdf_file_data *gp)
{
   if(gp->state != StrClosed){
      fclose(gp->fp);
      gp->state=StrClosed;
   }
}

void gsfd_close_all(gft_sdf_file_data *gp)
{
   if(gp){
      gsfd_close_all(gp->left);
      gsfd_close_all(gp->right);
      gsfd_close(gp);
   }
}

int gsfd_file_changed(gft_sdf_file_data *gp)
{
   int sr,ret=0;
   struct stat sbuf;
   int ltrace=0;
   
   if(gp){
      if((sr=stat(gp->fname,&sbuf))==-1){
         fprintf(stderr,"gft_file_changed: unable to get modification time for <%s>\n",
                     gp->fname);
         fprintf(stderr,"gft_file_changed: stat returned %d\n",sr);
         gp->mtime=0;
         gp->changed=0;
      }else{
         if(ltrace)
            fprintf(stderr,"gsfd_file_changed: got mtime\n");
         if(gp->mtime<sbuf.st_mtime){
            gp->mtime=sbuf.st_mtime;
            gp->changed=1;
            if(ltrace)
               fprintf(stderr,"gsfd_file_changed: file changed\n");
         }
      }
      if(gp->changed){
         gsfd_close(gp);
         ret=1;
         gp->changed=0;
         if(ltrace)
            fprintf(stderr,"gsfd_file_changed: closed file\n");
      }
   }
   return ret;
}

int gsfd_make_index(gft_sdf_file_data *gp)
{
   gft_sdf_file_index *fi;
   int csize,dsize,version,ret,nlev;
   char *dpname,*cnm,*tag;
   int *shp,rank,i;
   double time,*bbox,*coords,*dt;
   int ltrace=0;
   
   if(ltrace)
      fprintf(stderr,"gsfd_make_index: \n");
   if(gp==NULL)
      return(0);
   rewind(gp->fp);
   if(gp->idx)
      delete_gsfiarray(gp->idx);
   gp->idx=new_gsfiarray();
   if(gp->idx==NULL){
      fprintf(stderr,"gsfd_make_index: can't allocate array for index\n");
      errno=1;
      return(0);
   }
   i=0;
   do{
      fi=gsfi_new();
      fi->index=i++;
      fi->position=ftell(gp->fp);
      ret=low_read_sdf_stream(1, gp->fp, &time, &version, &rank,
                                          &dsize, &csize, &dpname, &cnm, &tag,
                                          &shp, &bbox, &coords, &dt);
      if(ret){
         fi->name=dpname;
         fi->tag=tag;
         fi->rank=rank;
         fi->shape=shp;
         fi->time=time;
         fi->bbox=bbox;
         add_gsfiitem(gp->idx,fi);
         if(cnm) free(cnm);
         if(coords) free(coords);
         if(dt) free(dt);
      }else{
         if(ltrace){
            fprintf(stderr,"gsfd_makd_index: low_read returned 0\n");
            fprintf(stderr,"gsfd_makd_index: fi=%p\n",fi);
         }
         if(fi) free(fi);
      }
   }while(ret);
   if(gp->tarray){ 
      delete_dynarray(gp->tarray);
   }
   gp->tarray=new_dynarray();
   if(gp->tarray==NULL){
      fprintf(stderr,"gsfd_make_index: can't allocate array for times\n");
      errno=2;
      return(0);
   }
   nlev=gsfiarray_len(gp->idx);
   for(i=0;i<nlev;i++){
      add_item(gp->tarray,(get_gsfiitem(gp->idx,i))->time);
   }
   rewind(gp->fp);
   if(ltrace)
      fprintf(stderr,"gsfd_make_index: returning\n");
   return(1);
}

int gft_make_index(const char *gfname)
{
   gft_sdf_file_data *gp;
   
   gp=gft_open_sdf_stream(gfname);
   return(gsfd_make_index(gp));
}

/*
   Set the I/O system to single file mode
   All data sets are written to a single file
   All data sets are read from a single file 
*/
void gft_set_single_sdf(const char *nm)
{
   gsfd_delete(gsfd_root);
   gsfd_root=NULL;
   sdf_file_mode=SDF_Single;
   strcpy(sdf_global_fname,nm);
}

/*
   Set the I/O system to multi file mode
*/
void gft_set_multi_sdf(void)
{
   if(sdf_file_mode==SDF_Single){
      gsfd_delete(gsfd_root);
      gsfd_root=NULL;
      sdf_file_mode=SDF_Multi;
      sdf_global_fname[0]='\0';
   }
}

void gft_set_single(const char *nm)
{
#ifdef HAVE_HDF
   if(getenv("BBHHDF"))
      gft_set_single_hdf(nm);
#endif
   gft_set_single_sdf(nm);
}

void gft_set_multi(void)
{
#ifdef HAVE_HDF
   if(getenv("BBHHDF"))
      gft_set_multi_hdf();
#endif
   gft_set_multi_sdf();
}

/* 2012-04-02: Modified by MWC so that '+' is a valid character in name. */
char *gft_make_sdf_name(const char *gfname)
{
   char *n,*p;
   const char *s;
   int ltrace=0;
   
   if(ltrace)
      fprintf(stderr,"gft_make_sdf_name(<%s>)\n",gfname);
   if(sdf_file_mode==SDF_Multi){
      s=gfname;
      if(!s)
         n=NULL;
      else{
         n=cvec_alloc_n(strlen(s)+5,"gft_make_sdf_name:n");
         for(p=n;*s;s++)
            if(isalnum(*s) || *s=='_' || *s=='.' || *s=='=' 
						|| *s=='/' || *s=='-' || *s=='+' )
               *(p++)=*s;
         if(*(p-4)!='.' || *(p-3)!='s' || *(p-2)!='d' || *(p-1)!='f'){
            *(p++)='.';
            *(p++)='s';
            *(p++)='d';
            *(p++)='f';
         }
         *p=0;
      }
   }else{
#ifndef _ALL_SOURCE
      n=strdup(sdf_global_fname);
#else
      n=strdup((char *)sdf_global_fname);
#endif
   }
   return n;
}

int gft_file_changed(const char *gfname)
{
   gft_sdf_file_data *gp;
   int ltrace=0;
   
   gp=gsfd_find(gsfd_root,gfname);
   if(ltrace){
      fprintf(stderr,"gft_file_changed: gp=%p\n",gp);
   }
   return(gsfd_file_changed(gp));
}

gft_sdf_file_data *gft_create_sdf_stream(const char *gfname)
{
   gft_sdf_file_data *gp;
   struct stat sbuf;
   int sr;
   int ltrace=0;
   
   if(ltrace)
      fprintf(stderr,"gft_create_sdf_stream(<%s>)\n",gfname);
      
   gp=gsfd_find(gsfd_root,gfname);
   
   if(ltrace)
      fprintf(stderr,"gft_create_sdf_stream gp=%x\n",gp);
      
   if(!gp){
      if(sdf_file_mode==SDF_Multi || !gsfd_root){
         gp=gsfd_new();
#ifndef _ALL_SOURCE
         gp->gfname=strdup(gfname);
#else
         gp->gfname=strdup((char *)gfname);
#endif
         gp->fname=gft_make_sdf_name(gfname);
         gsfd_insert(gsfd_root,gp);
      }else{
         gp=gsfd_root;
      }
   }
  if(gp->state==StrRead){
      fclose(gp->fp);
      gp->state=StrClosed;
   }
   if(gp->state==StrClosed){
     if(ltrace)
         fprintf(stderr,"gft_create_sdf_stream: file closed\n");
      if(getenv("GFT_APPEND"))
      gp->fp=fopen(gp->fname,"a+");
      else gp->fp=fopen(gp->fname,"w+");
      if(gp->fp==NULL){
/*
         fprintf(stderr,"gft_create_sdf_stream: can't open <%s> for writing \n",
                 gp->fname);
*/
         errno=1;
         return NULL;
      }
      gp->state=StrWrite;
      if((sr=stat(gp->fname,&sbuf))==-1){
         fprintf(stderr,"gft_create_sdf_stream: unable to get modification time for <%s>\n",
                     gp->fname);
         fprintf(stderr,"gft_create_sdf_stream: stat returned %d\n",sr);
         gp->mtime=0;
         return(gp);
      }else{
         if(gp->mtime<sbuf.st_mtime){
            gp->mtime=sbuf.st_mtime;
         }
      }
   }
   return gp;
}

gft_sdf_file_data *gft_open_sdf_stream(const char *gfname)
{
   gft_sdf_file_data *gp;
   struct stat sbuf;
   int sr;
   int ltrace=0;
   
   if(ltrace)
      fprintf(stderr,"gft_open_sdf_stream(<%s>)\n",gfname);
   
   gp=gsfd_find(gsfd_root,gfname);

   if(ltrace)
      fprintf(stderr,"gft_open_sdf_stream gp=%x\n",gp);

   if(!gp){
      if(sdf_file_mode==SDF_Multi || !gsfd_root){
         gp=gsfd_new();
#ifndef _ALL_SOURCE
         gp->gfname=strdup(gfname);
#else
         gp->gfname=strdup((char *)gfname);
#endif
         gp->fname=gft_make_sdf_name(gfname);
         gsfd_insert(gsfd_root,gp);
      }else{
         gp=gsfd_root;
      }
   }
   if(ltrace)
      fprintf(stderr,"gft_open_sdf_stream: fname=<%s>\n",gp->fname);
   if(gp->state==StrWrite){
      fclose(gp->fp);
      gp->state=StrClosed;
   }
   if(gp->state==StrClosed){
     if(ltrace)
         fprintf(stderr,"gft_open_sdf_stream: file <%s> is closed\n",gp->fname);
      gp->fp=fopen(gp->fname,"r");
      if(gp->fp==NULL){
         if(ltrace)
            fprintf(stderr,"gft_open_sdf_stream: can't open <%s> for reading\n",
                 gp->fname);
         errno=1;
         return NULL;
      }
      gp->state=StrRead;
      if((sr=stat(gp->fname,&sbuf))==-1){
         fprintf(stderr,"gft_open_sdf_stream: unable to get modification time for <%s>\n",
                     gp->fname);
         fprintf(stderr,"gft_open_sdf_stream: stat returned %d\n",sr);
         gp->mtime=0;
         return(gp);
      }else{
         if(gp->mtime<sbuf.st_mtime){
            gp->mtime=sbuf.st_mtime;
         }
      }
   }
  if(ltrace)
      fprintf(stderr,"gft_open_sdf_stream: returning\n");
   return gp;
}

/*
  mid level routine to write one record to sdf stream
   allows all fields except magic to be written separately
  inputs:
     everything
*/
int mid_write_sdf_stream(const char *gfname, const double time, const int rank, 
                         const int dsize, const int csize, const char *cnames, 
                                     const char *tag, const int *shape, const double *cds, const double *data)
{
   gft_sdf_file_data *gp;
      
   gp=gft_create_sdf_stream(gfname);
   if(gp==NULL){
      fprintf(stderr,"mid_write_sdf_stream: unable to create sdf stream for grid function <%s>\n",gfname);
      errno=1;
      return(0);
   }

   return(low_write_sdf_stream(gp->fp,gfname,time,rank,dsize,csize,cnames,tag,shape,cds,data));
}

/*
  mid level routine to read one record from sdf stream
  if the alloc_flag is 1, the routine allocates memory for
  all arrays, if 0, it assumes the memory has been preallocated
  inputs:
     alloc_flag
     gfname
  outputs:
     everything else
*/
int mid_read_sdf_stream(const int alloc_flag, const char *gfname, double *time, int *version, 
                        int *rank, int *dsize, int *csize, char **dpname, char **cnames,
                                    char **tag, int **shape, double **bbox, double **cds, double **data)
{
   gft_sdf_file_data *gp;
   int ret;
   int ltrace=0;
      
   if(ltrace)
     fprintf(stderr,"mid_read_sdf_stream: gfname=<%s>\n",gfname);
   gp=gft_open_sdf_stream(gfname);
   if(gp==NULL){
      fprintf(stderr,"mid_read_sdf_stream: unable to open sdf stream for grid function <%s>\n",gfname);
      errno=0;
      return(0);
   }

  ret=low_read_sdf_stream(alloc_flag,gp->fp,time,version,rank,dsize,csize,dpname,cnames,
                              tag,shape,bbox,cds,data);
   if(ltrace)
     fprintf(stderr,"mid_read_sdf_stream: returning %d\n",ret);
   return ret;
}

/*
  base level routine to write one record to sdf stream
  inputs:
     everything
*/
int low_write_sdf_stream(FILE *fp, const char *gfname, const double time, const int rank, 
                         const int dsize, const int csize, const char *cnames, 
                                     const char *tag, const int *shape, const double *cds, const double *data)
{
   int i,ret,cl,pl,tl;
   double *shp=NULL,*bbox=NULL;
   const double *p;
   bbh_sdf_header head;
   int ltrace=0;
#ifdef CRAYNOIEEE
   int type,bitoff,num;
   double *c=NULL,*d=NULL,t,*sh=NULL,*bb=NULL,csz,dsz,tln,pln,cln,rk,vr;
#endif
#ifndef WORDS_BIGENDIAN
   struct bs {
      unsigned h,l;
   } *tbs;
   unsigned bst;
   double *c=NULL,*d=NULL;
#endif
   
   if(rank>0){
      shp=vec_alloc_n(rank,"low_write_sdf_stream:shp");
      for(i=0;i<rank;i++)
         shp[i]=shape[i];
      bbox=vec_alloc_n(2*rank,"low_write_sdf_stream:shp");
      if(csize==2*rank){
         rdvcpy(bbox,(double *)cds,2*rank);
      }else{
         for(i=0,p=cds;i<rank;i++){
            bbox[2*i]=*p;
            bbox[2*i+1]=p[shape[i]-1];
            p+=shape[i];
         }
      }
   }
   pl=gfname ? strlen(gfname)+1 : 0;
   cl=cnames ? strlen(cnames)+1 : 0;
   tl=tag ? strlen(tag)+1 : 0;
   if(pl==1) pl=0;
   if(cl==1) cl=0;
   if(tl==1) tl=0;

#ifdef CRAYNOIEEE
   if(rank>0){
      sh=vec_alloc_n(rank,"low_write_sdf_stream:sh");
      bb=vec_alloc_n(2*rank,"low_write_sdf_stream:sh");
   }
   if(csize>0)
      c=vec_alloc_n(csize,"low_write_sdf_stream:c");
   if(dsize>0)
      d=vec_alloc_n(dsize,"low_write_sdf_stream:d");
   vr=SDFVERSION;
   rk=rank;
   csz=csize;
   dsz=dsize;
   pln=pl;
   cln=cl;
   tln=tl;
   type=8;bitoff=0;num=1;
   CRAY2IEG(&type,&num,&head.time,&bitoff,&time);
   CRAY2IEG(&type,&num,&head.version,&bitoff,&vr);
   CRAY2IEG(&type,&num,&head.rank,&bitoff,&rk);
   CRAY2IEG(&type,&num,&head.dsize,&bitoff,&dsz);
   CRAY2IEG(&type,&num,&head.csize,&bitoff,&csz);
   CRAY2IEG(&type,&num,&head.pnlen,&bitoff,&pln);
   CRAY2IEG(&type,&num,&head.cnlen,&bitoff,&cln);
   CRAY2IEG(&type,&num,&head.tglen,&bitoff,&tln);
   num=2*rank;
   if(rank>0){
      CRAY2IEG(&type,&num,bb,&bitoff,bbox);
      CRAY2IEG(&type,&rank,sh,&bitoff,shp);
   }
   if(csize>0)
      CRAY2IEG(&type,&csize,c,&bitoff,cds);
   if(dsize>0)
      CRAY2IEG(&type,&dsize,d,&bitoff,data);
#else
#ifndef WORDS_BIGENDIAN
   if(csize>0)
      c=vec_alloc_n(csize,"low_write_sdf_stream:c");
   if(dsize>0)
      d=vec_alloc_n(dsize,"low_write_sdf_stream:d");
   rdvcpy(c,(double *)cds,csize);
   rdvcpy(d,(double *)data,dsize);
#endif

/* Apparent GCC 3.3.1 compiler bug discovered by Frank Loeffler necessitates 
   the following hack ... */

   head.time=time + 0.0;
   head.version=SDFVERSION;
   head.rank=rank;
   head.dsize=dsize;
   head.csize=csize;
   head.pnlen=pl;
   head.cnlen=cl;
   head.tglen=tl;
#endif

   if(ltrace){
      fprintf(stderr,"low_write_sdf_stream: time=%g\n",head.time);
      fprintf(stderr,"low_write_sdf_stream: version=%g\n",head.version);
      fprintf(stderr,"low_write_sdf_stream: rank=%g\n",head.rank);
      fprintf(stderr,"low_write_sdf_stream: dsize=%g\n",head.dsize);
      fprintf(stderr,"low_write_sdf_stream: csize=%g\n",head.csize);
   }

#ifndef WORDS_BIGENDIAN
/* swap header */
   tbs=(struct bs *)&head.time;
   bst=tbs->h;
   tbs->h=htonl(tbs->l);
   tbs->l=htonl(bst);
   tbs=(struct bs *)&head.version;
   bst=tbs->h;
   tbs->h=htonl(tbs->l);
   tbs->l=htonl(bst);
   tbs=(struct bs *)&head.rank;
   bst=tbs->h;
   tbs->h=htonl(tbs->l);
   tbs->l=htonl(bst);
   tbs=(struct bs *)&head.dsize;
   bst=tbs->h;
   tbs->h=htonl(tbs->l);
   tbs->l=htonl(bst);
   tbs=(struct bs *)&head.csize;
   bst=tbs->h;
   tbs->h=htonl(tbs->l);
   tbs->l=htonl(bst);
   tbs=(struct bs *)&head.pnlen;
   bst=tbs->h;
   tbs->h=htonl(tbs->l);
   tbs->l=htonl(bst);
   tbs=(struct bs *)&head.cnlen;
   bst=tbs->h;
   tbs->h=htonl(tbs->l);
   tbs->l=htonl(bst);
   tbs=(struct bs *)&head.tglen;
   bst=tbs->h;
   tbs->h=htonl(tbs->l);
   tbs->l=htonl(bst);
   for(i=0;i<rank;i++){
      tbs=(struct bs *)&(shp[i]);
      bst=tbs->h;
      tbs->h=htonl(tbs->l);
      tbs->l=htonl(bst);
   }
   for(i=0;i<2*rank;i++){
      tbs=(struct bs *)&(bbox[i]);
      bst=tbs->h;
      tbs->h=htonl(tbs->l);
      tbs->l=htonl(bst);
   }
   for(i=0;i<csize;i++){
      tbs=(struct bs *)&(c[i]);
      bst=tbs->h;
      tbs->h=htonl(tbs->l);
      tbs->l=htonl(bst);
   }
   for(i=0;i<dsize;i++){
      tbs=(struct bs *)&(d[i]);
      bst=tbs->h;
      tbs->h=htonl(tbs->l);
      tbs->l=htonl(bst);
   }
#endif

   ret=fwrite(&head,sizeof(bbh_sdf_header),1,fp);
   if(!ret) goto cleanup;
   if(pl>0){
      ret=fwrite(gfname,sizeof(char),pl,fp);
      if(!ret) goto cleanup;
   }
   if(cl>0){
      ret=fwrite(cnames,sizeof(char),cl,fp);
      if(!ret) goto cleanup;
   }

   if(rank>0){
#ifdef CRAYNOIEEE
      ret=fwrite(bb,sizeof(double),2*rank,fp);
      if(!ret) goto cleanup;
      ret=fwrite(sh,sizeof(double),rank,fp);
      if(!ret) goto cleanup;
#else
      ret=fwrite(bbox,sizeof(double),2*rank,fp);
      if(!ret) goto cleanup;
      ret=fwrite(shp,sizeof(double),rank,fp);
      if(!ret) goto cleanup;
#endif
   }
   if(tl>0){
      ret=fwrite(tag,sizeof(char),tl,fp);
      if(!ret) goto cleanup;
   }

#if defined(CRAYNOIEEE) || !defined(WORDS_BIGENDIAN)
   if(csize>0){
      ret=fwrite(c,sizeof(double),csize,fp);
      if(!ret) goto cleanup;
   }
   if(dsize>0){
      ret=fwrite(d,sizeof(double),dsize,fp);
      if(!ret) goto cleanup;
   }
#else
   if(csize>0){
      ret=fwrite(cds,sizeof(double),csize,fp);
      if(!ret) goto cleanup;
   }
   if(dsize>0){
      ret=fwrite(data,sizeof(double),dsize,fp);
      if(!ret) goto cleanup;
   }
#endif

   ret=1;
cleanup:
#ifdef CRAYNOIEEE
   if(bb) free(bb);
   if(sh) free(sh);
#endif
#if defined(CRAYNOIEEE) || !defined(WORDS_BIGENDIAN)
   if(c) free(c);
   if(d) free(d);
#endif
   if(shp)free(shp);
   if(bbox)free(bbox);
   return(ret);
}

/*
  base level routine to read one record from sdf stream
  if the alloc_flag is 1, the routine allocates memory for
  all arrays, if 0, it assumes the memory has been preallocated
  inputs:
     alloc_flag
     fp
  outputs:
     everything else
*/
int low_read_sdf_stream(const int alloc_flag, FILE *fp, double *time, int *version, 
                        int *rank, int *dsize, int *csize, char **dpname, char **cnames,
                                    char **tag, int **shape, double **bbox, double **cds, double **data)
{
   double *shp=NULL;
   bbh_sdf_header head;
   int i,ret,pnlen,cnlen,tglen;
   int ltrace=0;
#ifdef CRAYNOIEEE
   int type,bitoff,num;
   double *c=NULL,*d=NULL,t,*bb=NULL,*sh=NULL,vr,rk,dsz,csz,tln,pln,cln;
#endif
#ifndef WORDS_BIGENDIAN
   struct bs {
      unsigned h,l;
   } *tbs;
   unsigned bst;
#endif
   
   if(ltrace)
      fprintf(stderr,"low_read_sdf_stream\n");
      
   ret=fread(&head,sizeof(bbh_sdf_header),1,fp);
   if(!ret) goto cleanup;

  if(alloc_flag){ 
      *tag=NULL;
     *dpname=NULL;
     *cnames=NULL;
      *bbox=NULL;
     *shape=NULL;
     *cds=NULL;
     *data=NULL;
  }
   if(ltrace)
      fprintf(stderr,"low_read_sdf_stream: past storage\n");

#ifdef CRAYNOIEEE
   type=8;num=1;bitoff=0;
   IEG2CRAY(&type,&num,&head.time,&bitoff,&t);
   IEG2CRAY(&type,&num,&head.version,&bitoff,&vr);
   IEG2CRAY(&type,&num,&head.rank,&bitoff,&rk);
   IEG2CRAY(&type,&num,&head.dsize,&bitoff,&dsz);
   IEG2CRAY(&type,&num,&head.csize,&bitoff,&csz);
   IEG2CRAY(&type,&num,&head.pnlen,&bitoff,&pln);
   IEG2CRAY(&type,&num,&head.cnlen,&bitoff,&cln);
   IEG2CRAY(&type,&num,&head.tglen,&bitoff,&tln);
   *time=t;
   *version=vr;
   *rank=rk;
   *dsize=dsz;
   *csize=csz;
   pnlen=pln;
   cnlen=cln;
   tglen=tln;
#else
#ifndef WORDS_BIGENDIAN
/* swap header */
   tbs=(struct bs *)&head.time;
   bst=tbs->h;
   tbs->h=ntohl(tbs->l);
   tbs->l=ntohl(bst);
   tbs=(struct bs *)&head.version;
   bst=tbs->h;
   tbs->h=ntohl(tbs->l);
   tbs->l=ntohl(bst);
   tbs=(struct bs *)&head.rank;
   bst=tbs->h;
   tbs->h=ntohl(tbs->l);
   tbs->l=ntohl(bst);
   tbs=(struct bs *)&head.dsize;
   bst=tbs->h;
   tbs->h=ntohl(tbs->l);
   tbs->l=ntohl(bst);
   tbs=(struct bs *)&head.csize;
   bst=tbs->h;
   tbs->h=ntohl(tbs->l);
   tbs->l=ntohl(bst);
   tbs=(struct bs *)&head.pnlen;
   bst=tbs->h;
   tbs->h=ntohl(tbs->l);
   tbs->l=ntohl(bst);
   tbs=(struct bs *)&head.cnlen;
   bst=tbs->h;
   tbs->h=ntohl(tbs->l);
   tbs->l=ntohl(bst);
   tbs=(struct bs *)&head.tglen;
   bst=tbs->h;
   tbs->h=ntohl(tbs->l);
   tbs->l=ntohl(bst);
#endif /* LITTLEENDIAN */
   *time=head.time;
   *version=head.version;
   *rank=head.rank;
   *dsize=head.dsize;
   *csize=head.csize;
   pnlen=head.pnlen;
   cnlen=head.cnlen;
   tglen=head.tglen;
#endif /* CRAYNOIEEE */

   if(*version!=SDFVERSION){
      fprintf(stderr,"low_read_sdf_stream: incompatible file version=%d\n",
         *version);
      errno=1;
      return(0);
   }

   if(ltrace){
      fprintf(stderr,"low_read_sdf_stream: read header\n");
      fprintf(stderr,"low_read_sdf_stream: rank=%d dsize=%d csize=%d\n",
      *rank,*dsize,*csize);
      fprintf(stderr,"low_read_sdf_stream: pnlen=%d, cnlen=%d, tglen=%d\n",
      pnlen,cnlen,tglen);
   }

   if(*rank>0)
      shp=vec_alloc_n(*rank,"low_read_sdf_stream:shp");
   if(alloc_flag){
      if(pnlen>0)*dpname=cvec_alloc_n(pnlen,"low_read_sdf_stream:dpname");
      if(cnlen>0)*cnames=cvec_alloc_n(cnlen,"low_read_sdf_stream:cnames");
      if(tglen>0)*tag=cvec_alloc_n(tglen,"low_read_sdf_stream:tag");
      if(*rank>0){
         *bbox=vec_alloc_n(*rank*2,"low_read_sdf_stream:bbox");
         *shape=ivec_alloc_n(*rank,"low_read_sdf_stream:shape");
      }
      if(*csize>0)
         *cds=vec_alloc_n(*csize,"low_read_sdf_stream:cds");
      if(*dsize>0)
         *data=vec_alloc_n(*dsize,"low_read_sdf_stream:data");
   }

   if(ltrace)
      fprintf(stderr,"low_read_sdf_stream: past allocations\n");

   if(pnlen>0){
      ret=fread(*dpname,sizeof(char),pnlen,fp);
      if(!ret) goto cleanup;
   }
   if(cnlen>0){
      ret=fread(*cnames,sizeof(char),cnlen,fp);
      if(!ret) goto cleanup;
   }
   if(*rank>0){
#ifdef CRAYNOIEEE
      sh=vec_alloc_n(*rank,"low_read_sdf_stream:sh");
      bb=vec_alloc_n(*rank*2,"low_read_sdf_stream:bb");
      ret=fread(bb,sizeof(double),*rank*2,fp);
      if(!ret) goto cleanup;
      ret=fread(sh,sizeof(double),*rank,fp);
      if(!ret) goto cleanup;
      type=8;bitoff=0;num=*rank*2;
      IEG2CRAY(&type,&num,bb,&bitoff,*bbox);
      IEG2CRAY(&type,rank,sh,&bitoff,shp);
#else
      ret=fread(*bbox,sizeof(double),*rank*2,fp);
      if(!ret) goto cleanup;
      ret=fread(shp,sizeof(double),*rank,fp);
      if(!ret) goto cleanup;
#endif
   }
   if(tglen>0){
      ret=fread(*tag,sizeof(char),tglen,fp);
      if(!ret) goto cleanup;
   }
   
#ifdef CRAYNOIEEE
   type=8;bitoff=0;
   if(*csize>0){
      c=vec_alloc_n(*csize,"low_read_sdf_stream:c");
      ret=fread(c,sizeof(double),*csize,fp);
      if(!ret) goto cleanup;
      IEG2CRAY(&type,csize,c,&bitoff,*cds);
   }
   if(*dsize>0){
      d=vec_alloc_n(*dsize,"low_read_sdf_stream:d");
      ret=fread(d,sizeof(double),*dsize,fp);
      if(!ret) goto cleanup;
      IEG2CRAY(&type,dsize,d,&bitoff,*data);
   }
#else
   if(*csize>0){
      ret=fread(*cds,sizeof(double),*csize,fp);
      if(!ret) goto cleanup;
   }
   if(*dsize>0){
      ret=fread(*data,sizeof(double),*dsize,fp);
      if(!ret) goto cleanup;
   }
#endif
#ifndef WORDS_BIGENDIAN
   for(i=0;i<*rank;i++){
      tbs=(struct bs *)&(shp[i]);
      bst=tbs->h;
      tbs->h=ntohl(tbs->l);
      tbs->l=ntohl(bst);
   }
   for(i=0;i<2* *rank;i++){
      tbs=(struct bs *)&(bbox[0][i]);
      bst=tbs->h;
      tbs->h=ntohl(tbs->l);
      tbs->l=ntohl(bst);
   }
   for(i=0;i<*csize;i++){
      tbs=(struct bs *)&(cds[0][i]);
      bst=tbs->h;
      tbs->h=ntohl(tbs->l);
      tbs->l=ntohl(bst);
   }
   for(i=0;i<*dsize;i++){
      tbs=(struct bs *)&(data[0][i]);
      bst=tbs->h;
      tbs->h=ntohl(tbs->l);
      tbs->l=ntohl(bst);
   }
#endif

   for(i=0;i<*rank;i++)
      shape[0][i]=shp[i];
   
   if(ltrace)
      fprintf(stderr,"low_read_sdf_stream: read data\n");
   
   ret=1;
cleanup:
#ifdef CRAYNOIEEE
   if(bb) free(bb);
   if(sh) free(sh);
   if(c) free(c);
   if(d) free(d);
#endif
   if(shp)free(shp);
   if(ltrace)
      fprintf(stderr,"low_read_sdf_stream: exiting with %d\n",ret);
  return(ret);
}

int gft_send_sdf_stream(char *gfname, double time, int *shape,
                        char *cnames, int rank, double *coords, double *data)
{
   int i,dsize,csize,ret;
   int cs;
   FILE *fp;
   int ltrace=0;
      
  for(i=0,dsize=1,csize=0;i<rank;i++){
    dsize*=shape[i];
    csize+=shape[i];
  }

   if(ltrace){
      fprintf(stderr,"gft_send_sdf_stream: calling low_write\n");
      fprintf(stderr,"  gfname=<%s>, time=%g, rank=%d, cnames=<%s>\n",
      gfname,time,rank,cnames);
   }

  if((cs=ser0_connect(getenv("BBHHOST"),BBH_PORT0)) < 0){
    fprintf(stderr,"gft_send_sdf_stream_brief: Connect to '%s' failed (%d)\n",getenv("BBHHOST"),cs);
    return 0;
  } else {
    if( ltrace ) {
      fprintf(stderr,"gft_send_sdf_stream_brief: Connect to '%s' on Port (%d)\n",getenv("BBHHOST"),cs);
    }
  } 
   fp=fdopen(cs,"w");
  if(fp==NULL){
    fprintf(stderr,"gft_send_sdf_stream_brief: fdopen(%d) failed\n",cs);
    return 0;
  } 

  ret=low_write_sdf_stream(fp, gfname, time, rank, dsize, csize,
                                       cnames, "", shape, coords, data);
   fclose(fp);
   shutdown(cs,2);
  return(ret);
}

int gft_send_sdf_stream_bbox(char *gfname, double time, int *shape,
                             int rank, double *coords, double *data)
{
   int i,dsize,csize,ret;
   int cs;
   FILE *fp;
   
   csize=2*rank;
  for(i=0,dsize=1;i<rank;i++)
    dsize*=shape[i];

  if((cs=ser0_connect(getenv("BBHHOST"),BBH_PORT0)) < 0){
    fprintf(stderr,"gft_send_sdf_stream_brief: Connect to '%s' failed (%d)\n",getenv("BBHHOST"),cs);
    return 0;
  }
   fp=fdopen(cs,"w");
  if(fp==NULL){
    fprintf(stderr,"gft_send_sdf_stream_brief: fdopen(%d) failed\n",cs);
    return 0;
  }

  ret=low_write_sdf_stream(fp, gfname, time, rank, dsize, csize,
                                      SDFCnames[rank-1], "", shape, coords, data);
   fclose(fp);
   shutdown(cs,2);
   return(ret);
}

int gft_send_sdf_stream_brief(char *gfname, double time, int *shape,
                              int rank, double *data)
{
   int i,dsize,csize,ret;
   int cs;
   FILE *fp;
   
   csize=2*rank;
  for(i=0,dsize=1;i<rank;i++)
    dsize*=shape[i];

  if((cs=ser0_connect(getenv("BBHHOST"),BBH_PORT0)) < 0){
    fprintf(stderr,"gft_send_sdf_stream_brief: Connect to '%s' failed (%d)\n",getenv("BBHHOST"),cs);
    return 0;
  }
   fp=fdopen(cs,"w");
  if(fp==NULL){
    fprintf(stderr,"gft_send_sdf_stream_brief: fdopen(%d) failed\n",cs);
    return 0;
  }

  ret=low_write_sdf_stream(fp, gfname, time, rank, dsize, csize,
                                      SDFCnames[rank-1], "", shape, BBH_bbox, data);
   fclose(fp);
   shutdown(cs,2);
   return(ret);
}

int gft_sendm_sdf_stream(char *gfname, double *time, int *shape,
                         char **cnames, int nt, int *rank, 
                                     double *coords, double *data)
{
   int i,nds,dsize,csize,ret;
   int cs,*shp;
   double *dt,*cds;
   char **cn;
   FILE *fp;
   int ltrace=0;
   
   if((cs=ser0_connect(getenv("BBHHOST"),BBH_PORT0)) < 0){
      fprintf(stderr,"gft_sendm_sdf_stream_brief: Connect to '%s' failed (%d)\n",getenv("BBHHOST"),cs);
      return 0;
   }
   fp=fdopen(cs,"w");
   if(fp==NULL){
      fprintf(stderr,"gft_sendm_sdf_stream_brief: fdopen(%d) failed\n",cs);
      return 0;
   }
   shp=shape;
   cds=coords;
   dt=data;
   cn=cnames;
   for(ret=1,nds=0;ret && nds<nt;nds++){
      for(i=0,dsize=1,csize=0;i<rank[nds];i++){
         dsize*=shp[i];
         csize+=shp[i];
      }
   
      if(ltrace){
         fprintf(stderr,"gft_sendm_sdf_stream: calling low_write\n");
         fprintf(stderr,"  gfname=<%s>, time=%g, rank=%d, cnames=<%s>\n",
         gfname,time[nds],rank[nds],*cn);
      }
      
      ret=low_write_sdf_stream(fp, gfname, time[nds], rank[nds], dsize, csize,
                                           *cn, "", shp, cds, dt);
      shp+=rank[nds];
      cds+=csize;
      dt+=dsize;
      cn++;
   }
   fclose(fp);
   shutdown(cs,2);
  return(ret);
}

int gft_sendm_sdf_stream_bbox(char *gfname, double *time, int *shape,
                             int nt, int *rank, double *coords, double *data)
{
   int i,dsize,csize,ret;
   int cs,nds;
   FILE *fp;
   int *shp;
   double *dt,*cds;
   
  if((cs=ser0_connect(getenv("BBHHOST"),BBH_PORT0)) < 0){
    fprintf(stderr,"gft_send_sdf_stream_brief: Connect to '%s' failed (%d)\n",getenv("BBHHOST"),cs);
    return 0;
  }
   fp=fdopen(cs,"w");
  if(fp==NULL){
    fprintf(stderr,"gft_send_sdf_stream_brief: fdopen(%d) failed\n",cs);
    return 0;
  }
   shp=shape;
   cds=coords;
   dt=data;
   for(ret=1,nds=0;ret && nds<nt;nds++){
      csize=2*rank[nds];
      for(i=0,dsize=1;i<rank[nds];i++)
         dsize*=shp[i];
   
   
      ret=low_write_sdf_stream(fp, gfname, time[nds], rank[nds], dsize, csize,
                                           SDFCnames[rank[nds]-1], "", shp, cds, dt);
      shp+=rank[nds];
      cds+=csize;
      dt+=dsize;
   }
   fclose(fp);
   shutdown(cs,2);
   return(ret);
}

int gft_sendm_sdf_stream_brief(char *gfname, double *time, int *shape,
                               int nt, int *rank, double *data)
{
   int i,dsize,csize,ret;
   int cs,nds;
   FILE *fp;
   int *shp;
   double *dt;
   
  if((cs=ser0_connect(getenv("BBHHOST"),BBH_PORT0)) < 0){
    fprintf(stderr,"gft_send_sdf_stream_brief: Connect to '%s' failed (%d)\n",getenv("BBHHOST"),cs);
    return 0;
  }
   fp=fdopen(cs,"w");
  if(fp==NULL){
    fprintf(stderr,"gft_send_sdf_stream_brief: fdopen(%d) failed\n",cs);
    return 0;
  }

   shp=shape;
   dt=data;
   for(ret=1,nds=0;ret && nds<nt;nds++){
      csize=2*rank[nds];
      for(i=0,dsize=1;i<rank[nds];i++)
         dsize*=shape[i];
   
      ret=low_write_sdf_stream(fp, gfname, time[nds], rank[nds], dsize, csize,
                                           SDFCnames[rank[nds]-1], "", shape, BBH_bbox, dt);
      shp+=rank[nds];
      dt+=dsize;
   }
   fclose(fp);
   shutdown(cs,2);
   return(ret);
}

int gft_write_sdf_stream(char *gfname, double time, int *shape,
                         char *cnames, int rank, double *coords, double *data)
{
   int i,dsize,csize,ret;
   int ltrace=0;
      
  for(i=0,dsize=1,csize=0;i<rank;i++){
    dsize*=shape[i];
    csize+=shape[i];
  }

   if(ltrace){
      fprintf(stderr,"gft_write_sdf_stream: calling low_write\n");
      fprintf(stderr,"  gfname=<%s>, time=%g, rank=%d, cnames=<%s>\n",
      gfname,time,rank,cnames);
   }

  ret=mid_write_sdf_stream(gfname, time, rank, dsize, csize,
                                       cnames, "", shape, coords, data);
  return(ret);
}

int gft_write_sdf_stream_bbox(char *gfname, double time, int *shape,
                              int rank, double *coords, double *data)
{
   int i,dsize,csize,ret;
   
   csize=2*rank;
  for(i=0,dsize=1;i<rank;i++)
    dsize*=shape[i];

  ret=mid_write_sdf_stream(gfname, time, rank, dsize, csize,
                                      SDFCnames[rank-1], "", shape, coords, data);
   return(ret);
}

int gft_write_sdf_stream_brief(char *gfname, double time, int *shape,
                               int rank, double *data)
{
   int i,dsize,csize,ret;
   
   csize=2*rank;
  for(i=0,dsize=1;i<rank;i++)
    dsize*=shape[i];

  ret=mid_write_sdf_stream(gfname, time, rank, dsize, csize,
                                      SDFCnames[rank-1], "", shape, BBH_bbox, data);
   return(ret);
   
}

int gft_writem_sdf_stream(char *gfname, double *time, int *shape, char **cnames,
                          int nt, int *rank, double *coords, double *data)
{
   int i,dsize,csize,ret=1;
   
   int nds,ds,*shp;
   double *dt,*cds;
   char **cnms;
   
   shp=shape;
   dt=data;
   cds=coords;
   cnms=cnames;
   for(nds=0;ret && nds<nt;nds++){
      csize=2*rank[nds];
      for(i=0,dsize=1;i<rank[nds];i++)
         dsize*=shp[i];
      ret*=gft_write_sdf_stream(gfname,time[nds],shp,*cnms,rank[nds],cds,dt);
      dt+=dsize;
      cds+=csize;
      shp+=rank[nds];
      cnms++;
   }
   return ret;
}

int gft_writem_sdf_stream_bbox(char *gfname, double *time, int *shape,
                               int nt, int *rank, double *coords, double *data)
{
   int i,dsize,csize,ret=1;
   
   int nds,ds,*shp;
   double *dt,*cds;
   
   shp=shape;
   dt=data;
   cds=coords;
   for(nds=0;ret && nds<nt;nds++){
      csize=2*rank[nds];
      for(i=0,dsize=1;i<rank[nds];i++)
         dsize*=shp[i];
      ret*=gft_write_sdf_stream_bbox(gfname,time[nds],shp,rank[nds],cds,dt);
      dt+=dsize;
      cds+=csize;
      shp+=rank[nds];
   }
   return ret;
}

int gft_writem_sdf_stream_brief(char *gfname, double *time, int *shape,
                                int nt, int *rank, double *data)
{
   int ret=1,i;
   int nds,ds,*shp;
   double *dt;
   
   shp=shape;
   dt=data;
   for(nds=0;ret && nds<nt;nds++){
      for(ds=1,i=0;i<rank[nds];i++)
         ds*=shp[i];
      ret*=gft_write_sdf_stream_brief(gfname,time[nds],shp,rank[nds],dt);
      dt+=ds;
      shp+=rank[nds];
   }
   return ret;
}

int gft_read_sdf_stream(const char *gfname, double *time, int *shape,
                        char *cnames, int *rank, double *coords, double *data)
{
   int i,csize,dsize,version,ret;
   int *shp;
   double *bbox,*cds,*dt,*p;
   char *dpname,*cnm,*tag;
   int ltrace=0;
   
   if(ltrace)
      fprintf(stderr,"gft_read_sdf_stream\n");
   
   ret=mid_read_sdf_stream(1, gfname, time, &version, rank,
                          &dsize, &csize, &dpname, &cnm, &tag,
                          &shp, &bbox, &cds, &dt);

   if(ret){
      rdvcpy(data,dt,dsize);
      rivcpy(shape,shp,*rank);
   }
   
   /* take care of bounding box case */
   if(csize > *rank * 2){
      rdvcpy(coords,cds,csize);
   }else{
      for(p=coords,i=0;i<*rank;i++){
         rdvramp(p,shape[i],bbox[2*i],(bbox[2*i+1]-bbox[2*i])/(shape[i]-1));
         p+=shape[i];
      }
   }
      
  if(ret && cnm){
      strcpy(cnames,cnm);
   }
  
  if(cnm) free(cnm);
  if(dpname) free(dpname);
   if(tag) free(tag);
   if(bbox) free(bbox);
   if(shp) free(shp);
   if(cds) free(cds);
   if(dt) free(dt);
  
  return(ret);
}

int gft_read_sdf_stream_brief(const char *gfname, double *time, int *shape,
                              int *rank, double *data)
{
   int csize,dsize,version,ret;
   char *dpname,*cnm,*tag;
   int *shp;
   double *bbox,*coords,*dt;
   int ltrace=0;
   
   if(ltrace)
      fprintf(stderr,"gft_read_sdf_stream_brief: gfname=<%s>\n",gfname);
   
   ret=mid_read_sdf_stream(1, gfname, time, &version, rank,
                          &dsize, &csize, &dpname, &cnm, &tag,
                          &shp, &bbox, &coords, &dt);

  if(ltrace){
     fprintf(stderr,"gft_read_sdf_stream_brief: time=%g rank=%d\n",*time,*rank);
      fprintf(stderr,"gft_read_sdf_stream_brief: dt=%p coords=%p\n",dt,coords);
      fprintf(stderr,"gft_read_sdf_stream_brief: dsize=%d,csize=%d\n",dsize,csize);
      fprintf(stderr,"gft_read_sdf_stream_brief: dpname=<%s>\n",dpname);
      fprintf(stderr,"gft_read_sdf_stream_brief: cnm=<%s>\n",cnm);
      fprintf(stderr,"gft_read_sdf_stream_brief: tag=<%s>\n",tag);
   }
   if(ret){
      rdvcpy(data,dt,dsize);
      rivcpy(shape,shp,*rank);
   }
  if(cnm) free(cnm);
  if(dpname) free(dpname);
   if(bbox) free(bbox);
   if(shp) free(shp);
   if(coords) free(coords);
   if(dt) free(dt);
  
  return(ret);
}

int gft_read_sdf_rank(const char *gf_name, const int level, int *rank)
{
   int i,csize,dsize,version,ret;
   int *shp;
   double *bbox,*cds,*dt,time;
   char *dpname,*cnm,*tag;
   gft_sdf_file_data *gp;
   gft_sdf_file_index *fi;
   long pos;
   int ltrace=0;
   
   if(ltrace)
      fprintf(stderr,"gft_read_sdf_rank\n");
   
   gp=gft_open_sdf_stream(gf_name);
   if(gp==NULL){
      fprintf(stderr,"gft_read_sdf_rank: can't open <%s> for reading\n",gf_name);
      errno=0;
      return(0);
   }
   
   if(gp->idx==NULL || gsfd_file_changed(gp)){ /* new file or modified */
      if(gp->idx!=NULL){ /* file changed --> file closed */
         gp=gft_open_sdf_stream(gf_name);
         if(gp==NULL){
            fprintf(stderr,"gft_read_sdf_rank: can't reopen <%s> for reading\n",gf_name);
            errno=0;
            return(0);
         }
      }
      ret=gsfd_make_index(gp);
      if(ret==0){
         fprintf(stderr,"gft_read_sdf_rank: can't make index for <%s>\n",gf_name);
         errno=2;
         return(0);
      }
   }
   fi=get_gsfiitem(gp->idx,level-1);
   if(fi==NULL){
/*    fprintf(stderr,"gft_read_sdf_rank: level %d out of range\n",level);  */
      errno=3;
      return(0);
   }
   
  pos=fi->position;

  if(fseek(gp->fp,pos,SEEK_SET)){
     fprintf(stderr,"gft_read_sdf_rank: fseek failed\n");
     errno=10;
     return(0);
  }
   ret=low_read_sdf_stream(1, gp->fp, &time, &version, rank,
                          &dsize, &csize, &dpname, &cnm, &tag,
                          &shp, &bbox, &cds, &dt);

  if(cnm) free(cnm);
  if(dpname) free(dpname);
   if(tag) free(tag);
   if(bbox) free(bbox);
   if(shp) free(shp);
   if(cds) free(cds);
   if(dt) free(dt);
  
  return(ret);
}

int gft_read_sdf_shape(const char *gf_name, const int level, int *shape)
{
   int i,csize,dsize,version,ret;
   int *shp,rank;
   double *bbox,*cds,*dt,time;
   char *dpname,*cnm,*tag;
   gft_sdf_file_data *gp;
   gft_sdf_file_index *fi;
   long pos;
   int ltrace=0;
   
   if(ltrace)
      fprintf(stderr,"gft_read_sdf_rank\n");
   
   gp=gft_open_sdf_stream(gf_name);
   if(gp==NULL){
      fprintf(stderr,"gft_read_sdf_rank: can't open <%s> for reading\n",gf_name);
      errno=0;
      return(0);
   }
   
   if(gp->idx==NULL || gsfd_file_changed(gp)){ /* new file or modified */
      if(gp->idx!=NULL){ /* file changed --> file closed */
         gp=gft_open_sdf_stream(gf_name);
         if(gp==NULL){
            fprintf(stderr,"gft_read_sdf_rank: can't reopen <%s> for reading\n",gf_name);
            errno=0;
            return(0);
         }
      }
      ret=gsfd_make_index(gp);
      if(ret==0){
         fprintf(stderr,"gft_read_sdf_rank: can't make index for <%s>\n",gf_name);
         errno=2;
         return(0);
      }
   }
   fi=get_gsfiitem(gp->idx,level-1);
   if(fi==NULL){
/*    fprintf(stderr,"gft_read_sdf_rank: level %d out of range\n",level);  */
      errno=3;
      return(0);
   }
   
  pos=fi->position;

  if(fseek(gp->fp,pos,SEEK_SET)){
     fprintf(stderr,"gft_read_sdf_rank: fseek failed\n");
     errno=10;
     return(0);
  }
   ret=low_read_sdf_stream(1, gp->fp, &time, &version, &rank,
                          &dsize, &csize, &dpname, &cnm, &tag,
                          &shp, &bbox, &cds, &dt);

   if(ret)
      rivcpy(shape,shp,rank);
      
  if(cnm) free(cnm);
  if(dpname) free(dpname);
   if(tag) free(tag);
   if(bbox) free(bbox);
   if(shp) free(shp);
   if(cds) free(cds);
   if(dt) free(dt);
  
  return(ret);
}

int gft_read_sdf_bbox(const char *gf_name, const int level, double *bbox)
{
   int i,csize,dsize,version,ret;
   int *shp,rank;
   double *l_bbox,*cds,*dt,time;
   char *dpname,*cnm,*tag;
   gft_sdf_file_data *gp;
   gft_sdf_file_index *fi;
   long pos;
   int ltrace=0;
   
   if(ltrace)
      fprintf(stderr,"gft_read_sdf_rank\n");
   
   gp=gft_open_sdf_stream(gf_name);
   if(gp==NULL){
      fprintf(stderr,"gft_read_sdf_rank: can't open <%s> for reading\n",gf_name);
      errno=0;
      return(0);
   }
   
   if(gp->idx==NULL || gsfd_file_changed(gp)){ /* new file or modified */
      if(gp->idx!=NULL){ /* file changed --> file closed */
         gp=gft_open_sdf_stream(gf_name);
         if(gp==NULL){
            fprintf(stderr,"gft_read_sdf_rank: can't reopen <%s> for reading\n",gf_name);
            errno=0;
            return(0);
         }
      }
      ret=gsfd_make_index(gp);
      if(ret==0){
         fprintf(stderr,"gft_read_sdf_rank: can't make index for <%s>\n",gf_name);
         errno=2;
         return(0);
      }
   }
   fi=get_gsfiitem(gp->idx,level-1);
   if(fi==NULL){
/*    fprintf(stderr,"gft_read_sdf_rank: level %d out of range\n",level);  */
      errno=3;
      return(0);
   }
   
  pos=fi->position;

  if(fseek(gp->fp,pos,SEEK_SET)){
     fprintf(stderr,"gft_read_sdf_rank: fseek failed\n");
     errno=10;
     return(0);
  }
   ret=low_read_sdf_stream(1, gp->fp, &time, &version, &rank,
                          &dsize, &csize, &dpname, &cnm, &tag,
                          &shp, &l_bbox, &cds, &dt);

   if(ret)
      rdvcpy(bbox,l_bbox,2*rank);
      
  if(cnm) free(cnm);
  if(dpname) free(dpname);
   if(tag) free(tag);
   if(l_bbox) free(l_bbox);
   if(shp) free(shp);
   if(cds) free(cds);
   if(dt) free(dt);
  
  return(ret);
}

int gft_read_sdf_name(const char *file_name, const int n, char *name)
{
   int i,csize,dsize,version,ret;
   int *shp,rank;
   double *bbox,*cds,*dt,time;
   char *dpname,*cnm,*tag;
   gft_sdf_file_data *gp;
   gft_sdf_file_index *fi;
   long pos;
   int ltrace=0;
   
   if(ltrace)
      fprintf(stderr,"gft_read_sdf_rank\n");
   
   gp=gft_open_sdf_stream(file_name);
   if(gp==NULL){
      fprintf(stderr,"gft_read_sdf_name: can't open <%s> for reading\n",file_name);
      errno=0;
      return(0);
   }
   
   if(gp->idx==NULL || gsfd_file_changed(gp)){ /* new file or modified */
      if(gp->idx!=NULL){ /* file changed --> file closed */
         gp=gft_open_sdf_stream(file_name);
         if(gp==NULL){
            fprintf(stderr,"gft_read_sdf_name: can't reopen <%s> for reading\n",file_name);
            errno=0;
            return(0);
         }
      }
      ret=gsfd_make_index(gp);
      if(ret==0){
         fprintf(stderr,"gft_read_sdf_name: can't make index for <%s>\n",file_name);
         errno=2;
         return(0);
      }
   }
   fi=get_gsfiitem(gp->idx,n-1);
   if(fi==NULL){
/*    fprintf(stderr,"gft_read_sdf_name: level %d out of range\n",n);  */
      errno=3;
      return(0);
   }
   
  pos=fi->position;

  if(fseek(gp->fp,pos,SEEK_SET)){
     fprintf(stderr,"gft_read_sdf_name: fseek failed\n");
     errno=10;
     return(0);
  }
   ret=low_read_sdf_stream(1, gp->fp, &time, &version, &rank,
                          &dsize, &csize, &dpname, &cnm, &tag,
                          &shp, &bbox, &cds, &dt);

   if(dpname) strcpy(name,dpname);
  if(cnm) free(cnm);
  if(dpname) free(dpname);
   if(tag) free(tag);
   if(bbox) free(bbox);
   if(shp) free(shp);
   if(cds) free(cds);
   if(dt) free(dt);
  
  return(ret);
}

int gft_read_sdf_ntlevs(const char *file_name)
{
   int ntlevs,ret;
   gft_sdf_file_data *gp;
   
   gp=gft_open_sdf_stream(file_name);
   if(gp==NULL){
      fprintf(stderr,"gft_read_sdf_ntlevs: can't open <%s> for reading\n",
              file_name);
      errno=0;
      return(0);
   }
   
   if(gp->idx==NULL || gsfd_file_changed(gp)){ /* new file or modified */
      if(gp->idx!=NULL){ /* file changed --> file closed */
         gp=gft_open_sdf_stream(file_name);
         if(gp==NULL){
            fprintf(stderr,"gft_read_sdf_ntlevs: can't reopen <%s> for reading\n",
                    file_name);
            errno=0;
            return(0);
         }
      }
      ret=gsfd_make_index(gp);
      if(ret==0){
         fprintf(stderr,"gft_read_sdf_ntlevs: can't make index for <%s>\n",
                 file_name);
         errno=2;
         return(0);
      }
   }
  ntlevs=dynarray_len(gp->tarray);
   return(ntlevs);
}

int gft_read_sdf_full(const char *gf_name, int level, int *shape, char *cnames, 
                    int rank, double *time, double *coords, double *data)
{
   int i,csize,dsize,version,ret;
   int *shp;
   double *bbox,*cds,*dt,*p;
   char *dpname,*cnm,*tag;
   gft_sdf_file_data *gp;
   gft_sdf_file_index *fi;
   long pos;
   int ltrace=0;
   
   if(ltrace)
      fprintf(stderr,"gft_read_sdf_full\n");
   
   gp=gft_open_sdf_stream(gf_name);
   if(gp==NULL){
      fprintf(stderr,"gft_read_sdf_full: can't open <%s> for reading\n",gf_name);
      errno=0;
      return(0);
   }
   
   if(gp->idx==NULL || gsfd_file_changed(gp)){ /* new file or modified */
      if(gp->idx!=NULL){ /* file changed --> file closed */
         gp=gft_open_sdf_stream(gf_name);
         if(gp==NULL){
            fprintf(stderr,"gft_read_sdf_full: can't reopen <%s> for reading\n",gf_name);
            errno=0;
            return(0);
         }
      }
      if(ltrace)
         fprintf(stderr,"gft_read_sdf_full: making index for <%s>\n",gf_name);
      ret=gsfd_make_index(gp);
      if(ret==0){
         fprintf(stderr,"gft_read_sdf_full: can't make index for <%s>\n",gf_name);
         errno=2;
         return(0);
      }
   }
   if(ltrace)
      fprintf(stderr,"gft_read_sdf_full: level=%d\n",level);
   
   fi=get_gsfiitem(gp->idx,level-1);
   if(fi==NULL){
/*    fprintf(stderr,"gft_read_sdf_full: level %d out of range\n",level);  */
      errno=3;
      return(0);
   }
   
  pos=fi->position;

   if(ltrace)
      fprintf(stderr,"gft_read_sdf_full: pos=%d\n",pos);
      
  if(fseek(gp->fp,pos,SEEK_SET)){
     fprintf(stderr,"gft_read_sdf_full: fseek failed\n");
     errno=10;
     return(0);
  }
   if(ltrace)
      fprintf(stderr,"gft_read_sdf_full: calling low_read\n");
   ret=low_read_sdf_stream(1, gp->fp, time, &version, &rank,
                          &dsize, &csize, &dpname, &cnm, &tag,
                          &shp, &bbox, &cds, &dt);

   if(ltrace)
      fprintf(stderr,"gft_read_sdf_full: rank=%d\n",rank);
   if(ret){
      rdvcpy(data,dt,dsize);
      rivcpy(shape,shp,rank);
   }

   /* take care of bounding box case */
   if(csize > rank * 2){
      rdvcpy(coords,cds,csize);
   }else{
      for(p=coords,i=0;i<rank;i++){
         rdvramp(p,shape[i],bbox[2*i],(bbox[2*i+1]-bbox[2*i])/(shape[i]-1));
         p+=shape[i];
      }
   }

   if(ltrace)
      fprintf(stderr,"gft_read_sdf_full: ready for cnames\n");
  if(ret && cnm){
      strcpy(cnames,cnm);
   }
  
   if(ltrace)
      fprintf(stderr,"gft_read_sdf_full: done with cnames\n");

  if(cnm) free(cnm);
  if(dpname) free(dpname);
   if(tag) free(tag);
   if(bbox) free(bbox);
   if(shp) free(shp);
   if(cds) free(cds);
   if(dt) free(dt);
  
  return(ret);
}

int gft_read_sdf_brief(const char *gf_name, int level,  double *data)
{
   int i,csize,dsize,version,ret;
   int *shp,rank;
   double *bbox,*cds,*dt,time;
   char *dpname,*cnm,*tag;
   gft_sdf_file_data *gp;
   gft_sdf_file_index *fi;
   long pos;
   int ltrace=0;
   
   if(ltrace)
      fprintf(stderr,"gft_read_sdf_brief\n");
   
   gp=gft_open_sdf_stream(gf_name);
   if(gp==NULL){
      fprintf(stderr,"gft_read_sdf_full: can't open <%s> for reading\n",gf_name);
      errno=0;
      return(0);
   }
   
   if(gp->idx==NULL || gsfd_file_changed(gp)){ /* new file or modified */
      if(gp->idx!=NULL){ /* file changed --> file closed */
         gp=gft_open_sdf_stream(gf_name);
         if(gp==NULL){
            fprintf(stderr,"gft_read_sdf_full: can't reopen <%s> for reading\n",gf_name);
            errno=0;
            return(0);
         }
      }
      ret=gsfd_make_index(gp);
      if(ret==0){
         fprintf(stderr,"gft_read_sdf_full: can't make index for <%s>\n",gf_name);
         errno=2;
         return(0);
      }
   }
   fi=get_gsfiitem(gp->idx,level-1);
   if(fi==NULL){
/*    fprintf(stderr,"gft_read_sdf_full: level %d out of range\n",level);  */
      errno=3;
      return(0);
   }
   
  pos=fi->position;

  if(fseek(gp->fp,pos,SEEK_SET)){
     fprintf(stderr,"gft_read_sdf_full: fseek failed\n");
     errno=10;
     return(0);
  }
   ret=low_read_sdf_stream(1, gp->fp, &time, &version, &rank,
                          &dsize, &csize, &dpname, &cnm, &tag,
                          &shp, &bbox, &cds, &dt);

   if(ret){
      rdvcpy(data,dt,dsize);
   }
   
  if(cnm) free(cnm);
  if(dpname) free(dpname);
   if(tag) free(tag);
   if(bbox) free(bbox);
   if(shp) free(shp);
   if(cds) free(cds);
   if(dt) free(dt);
  
  return(ret);
}

void gft_close_sdf_stream(const char *gfname)
{
   gft_sdf_file_data *gp;
   int ltrace=0;
   
   if(ltrace)
      fprintf(stderr,"gft_close_sdf_stream\n");
   gp=gsfd_find(gsfd_root, gfname);
   if(gp) gsfd_close(gp);
}

void gft_close_all_sdf(void)
{
   gsfd_close_all(gsfd_root);
}

/* interface for Matt's explorer modules
   does not have the same interface as the HDF GFT_extract
    assumes file contains one grid function and that
    all time levels are uniform
*/
int gft_extract_sdf(const char *fn, /* in: file name */
               const int tlev, /* in: time level to retrieve (1 - ntlev) */
               char **func_name, /* name of this grid function */
               int *ntlev, /* number of time levels available in file */
               double **tvec, /* vector of time values */
               double *time, /* time of this slice */
               int *drank, /* rank of data set */
               int **shape, /* shape of data set */
               int *csize, /* size of coordinates */
               char **cnames, /* names of coordinates (packed) */
               double **coords, /* values of coordinates (packed) */
               double **data ) /* actual data */
{
   int dsize,version;
   int i,ret;
   char *tag=NULL;
   double *bbox=NULL;
   long pos;
   gft_sdf_file_data *gp;
  int ltrace=0;
   
   if(tlev<0){ /* signal to free storage */
      if(*func_name)free(*func_name);
      if(*tvec)free(*tvec);
      if(*shape)free(*shape);
      if(*cnames)free(*cnames);
      if(*coords)free(*coords);
      if(*data)free(*data);
      *func_name=NULL;
      *tvec=NULL;
      *shape=NULL;
      *cnames=NULL;
      *coords=NULL;
      *data=NULL;
      return(1);
   }
   *ntlev=0;
   
   if(ltrace){
      fprintf(stderr,"gft_extract_sdf: tlev=%d fn=<%s>\n",tlev,fn);
   }
   
   gp=gft_open_sdf_stream(fn);
   if(gp==NULL){
      fprintf(stderr,"gft_extract_sdf: can't open <%s> for reading\n",fn);
      errno=0;
      return(0);
   }
   
   if(gp->idx==NULL || gsfd_file_changed(gp)){ /* new file or modified */
      if(ltrace){
         fprintf(stderr,"gft_extract_sdf: fn=<%s>\n",fn);
         fprintf(stderr,"gft_extract_sdf: file_changed=%d\n",gft_file_changed(fn));
      }
      if(gp->idx!=NULL){ /* file changed --> file closed */
         gp=gft_open_sdf_stream(fn);
         if(gp==NULL){
            fprintf(stderr,"gft_extract_sdf: can't reopen <%s> for reading\n",fn);
            errno=0;
            return(0);
         }
      }
      if(ltrace){
         fprintf(stderr,"gft_extract_sdf: initializing <%s>\n",fn);
      }
      ret=gsfd_make_index(gp);
      if(ret==0){
         fprintf(stderr,"gft_extract_sdf: can't make index for <%s>\n",fn);
         errno=2;
         return(0);
      }
      *ntlev=gsfiarray_len(gp->idx);
      if(ltrace){
         fprintf(stderr,"gft_extract_sdf: ntlevs=%d\n",*ntlev);
      }
      
   }

  *ntlev=dynarray_len(gp->tarray);

   if(ltrace)
      fprintf(stderr,"gft_extract_sdf: ntlevs=%d\n",*ntlev);
        
   if(tlev>*ntlev){
   /*
      fprintf(stderr,"gft_extract_sdf: time level %d is out of range 1-%d.\n",tlev,
      *ntlev);
   */
      errno=11;
      return(0);
   }

  *tvec=vec_alloc_n(*ntlev,"gft_extract_sdf:tvec");
  rdvcpy(*tvec,gp->tarray->data,*ntlev);
   
  pos=get_gsfiitem(gp->idx,tlev-1)->position;

  if(fseek(gp->fp,pos,SEEK_SET)){
     fprintf(stderr,"gft_extract_sdf: lseek failed\n");
     errno=10;
     return(0);
  }
   
   if(ltrace) fprintf(stderr,"gft_extract_sdf: reading gfunc...\n");
   ret=low_read_sdf_stream(1, gp->fp, time, &version, drank,
                      &dsize, csize, func_name, cnames, &tag,
                      shape, &bbox, coords, data);
   
   if(tag)free(tag);
   if(bbox)free(bbox);
   if(ltrace)
      fprintf(stderr,"gft_extract_sdf: cnames=<%s>\n",*cnames);
   return ret;
}

int GFT_extract2(const char *fn, /* in: file name */
               const int tlev, /* in: time level to retrieve (1 - ntlev) */
               char **func_name, /* name of this grid function */
               int *ntlev, /* number of time levels available in file */
               double **tvec, /* vector of time values */
               double *time, /* time of this slice */
               int *drank, /* rank of data set */
               int **shape, /* shape of data set */
               char ***cnames, /* names of coordinates */
               double ***coords, /* values of coordinates */
               double **data) /* actual data */
{
   int ret,i,csize;
   char *cnm;
   double *cd,*d;
   int ltrace=0;
   
   if(ltrace) fprintf(stderr,"GFT_extract2\n");
   if(tlev<0){
      for(i=0;i<*drank;i++){
         if(cnames[0][i]) free(cnames[0][i]);
         if(coords[0][i]) free(coords[0][i]);
      }
      if(*cnames) free(*cnames);
      if(*coords) free(*coords);
      cnm=NULL;
      cd=NULL;
   }
   ret=gft_extract_sdf(fn,tlev,func_name,ntlev,tvec,time,drank,shape,&csize,
                       &cnm,&cd,data);
   if(ltrace) fprintf(stderr,"GFT_extract2: gft_extract_sdf returned %d %d\n",
                      ret,*ntlev);

/* Ensure that there *are* coordinate names associated with the dataset ... */

   if( !cnm ) {
      switch ( *drank ) {
      case 1:
         cnm = strdup("x");
         break;
      case 2:
         cnm = strdup("x|y");
         break;
      case 3:
         cnm = strdup("x|y|z");
         break;
      }
   }

   if(ret && tlev>0){
     if(ltrace) { fprintf(stderr,"GFT_extract2: In if\n"); fflush(stderr); }
      *cnames=(char **)malloc(sizeof(char *)* *drank);
      if(!*cnames){
         fprintf(stderr,"GFT_extract2: Can't allocate memory for cnames\n");
         errno=15;
         return(0);
      }
     if(ltrace) { fprintf(stderr,"GFT_extract2: Allocated cnames\n"); fflush(stderr); }
      *coords=(double **)malloc(sizeof(double *)* *drank);
      if(!*coords){
         fprintf(stderr,"GFT_extract2: Can't allocate memory for coords\n");
         errno=15;
         return(0);
      }
     if(ltrace) { fprintf(stderr,"GFT_extract2: Allocated coords ... sleeping ..."); fflush(stderr); sleep(1); fprintf(stderr,"GFT_extract2: done\n"); }
     if(ltrace) { fprintf(stderr,"GFT_extract2: drank = %d\n",*drank); fflush(stderr); }
      for(i=0;i<*drank;i++){
         if( ltrace ) { fprintf(stderr,"GFT_extract2: i = %d\n",i);  fflush(stderr); sleep(1); }
         cnames[0][i]=cvec_alloc_n(strlen(cnm)+1,"GFT_extract2:cnames");
         coords[0][i]=vec_alloc_n((*shape)[i],"GFT_extract2:coords");
      }
      
     if(ltrace) fprintf(stderr,"GFT_extract2: finished allocations\n");  fflush(stderr);
      /* take care of bounding box case */
      if(csize > *drank * 2){
         for(i=0,d=cd;i<*drank;i++){
            rdvcpy(coords[0][i],d,(*shape)[i]);
            d+=(*shape)[i];
         }
      }else{
         for(i=0;i<*drank;i++)
            rdvramp(coords[0][i],(*shape)[i],cd[2*i],(cd[2*i+1]-cd[2*i])/((*shape)[i] - 1));
      }
      
     if(ltrace) fprintf(stderr,"GFT_extract2: setup coordinates\n");
      switch(*drank){
         case 1 :
            strcpy(cnames[0][0],cnm);
            break;
         case 2 :
            sscanf(cnm,"%[^|]|%s",cnames[0][0],cnames[0][1]);
            break;
         case 3 :
            sscanf(cnm,"%[^|]|%[^|]|%s",cnames[0][0],cnames[0][1],cnames[0][2]);
            break;
         case 4 :
            sscanf(cnm,"%[^|]|%[^|]|%[^|]|%s",cnames[0][0],cnames[0][1],cnames[0][2],cnames[0][3]);
            break;
         default:
            fprintf(stderr,"GFT_extract: rank = %d is not supported\n",*drank);
            errno=2;
            return(0);
      }
     if(ltrace) fprintf(stderr,"GFT_extract2: setup coordinate names\n");
      if(cnm) free(cnm);
      if(cd) free(cd);
   }
   return ret;
}

int gft_write_id_gf(char *gfname, int *shape, int rank, double *data)
{
   return gft_write_sdf_stream_brief(gfname,0.0,shape,rank,data);
}

int gft_write_id_int(char *pname, int *param, int nparam)
{
   int i,j,ret;
   char *op=NULL;
   int ltrace=0;
   
   if(ltrace)
      fprintf(stderr,"gft_write_id_int: pname=<%s> nparam=%d\n",pname,nparam);
   op=cvec_alloc_n(nparam*16+strlen(pname)+5,"gft_write_id_int:op");
   j=sprintf(op,"%s:=",pname);
   if(nparam>1)
      j+=sprintf(op+j,"[ ");
   for(i=0;i<nparam;i++)
      j+=sprintf(op+j,"%d ",param[i]);
   if(nparam>1)
   sprintf(op+j,"]");
   ret=mid_write_sdf_stream(pname,0.0,0,0,0,"",op,NULL,NULL,NULL);
   if(op)free(op);
   return ret;
}

/* June 1 2000, MWC:  Fixed bug: %g format loses precision */ 

int gft_write_id_float_old(char *pname, double *param, int nparam)
{
   int i,j,ret;
   char *op=NULL;
   int ltrace=0;
   
   if(ltrace)
      fprintf(stderr,"gft_write_id_float: pname=<%s> nparam=%d\n",pname,nparam);
   
   op=cvec_alloc_n(nparam*16+strlen(pname)+5,"gft_write_id_float:op");
   j=sprintf(op,"%s:=",pname);
   if(nparam>1)
      j+=sprintf(op+j,"[ ");
   for(i=0;i<nparam;i++)
      j+=sprintf(op+j,"%g ",param[i]);
   if(nparam>1)
      sprintf(op+j,"]");
   if( ltrace ) {
     fprintf(stderr,"gft_write_id_float: op=<%s> (%d)\n",op,strlen(op));
   }
   ret=mid_write_sdf_stream(pname,0.0,0,0,0,"",op,NULL,NULL,NULL);
   if(op)free(op);
   return ret;
}

int gft_write_id_float(char *pname, double *param, int nparam)
{
   int i,j,ret;
   char *op=NULL;
   int ltrace=0;
   
   if(ltrace)
      fprintf(stderr,"gft_write_id_float: pname=<%s> nparam=%d\n",pname,nparam);
   
   op=cvec_alloc_n(nparam*32+strlen(pname)+5,"gft_write_id_float:op");
   j=sprintf(op,"%s:=",pname);
   if(nparam>1)
      j+=sprintf(op+j,"[ ");
   for(i=0;i<nparam;i++)
      j+=sprintf(op+j,"%24.16e ",param[i]);
   if(nparam>1)
      sprintf(op+j,"]");
   if( ltrace ) {
     fprintf(stderr,"gft_write_id_float: op=<%s> (%d)\n",op,strlen(op));
   }
   ret=mid_write_sdf_stream(pname,0.0,0,0,0,"",op,NULL,NULL,NULL);
   if(op)free(op);
   return ret;
}

int gft_write_id_str(char *pname, char **param, int nparam)
{
   int i,j,size,ret;
   char *op=NULL;
   int ltrace=0;
   
   if(ltrace)
      fprintf(stderr,"gft_write_id_str: pname=<%s> nparam=%d\n",pname,nparam);
   
   for(size=0,i=0;i<nparam;i++)
      size+=strlen(param[i]);
   op=cvec_alloc_n(size+3*nparam+strlen(pname)+6,"gft_write_id_str:op");
   j=sprintf(op,"%s:=",pname);
   if(nparam>1)
      j+=sprintf(op+j,"[ ");
   for(i=0;i<nparam;i++)
      j+=sprintf(op+j,"\"%s\" ",param[i]);
   if(nparam>1)
      sprintf(op+j,"]");
   ret=mid_write_sdf_stream(pname,0.0,0,0,0,"",op,NULL,NULL,NULL);
   if(op)free(op);
   return ret;
}

int gft_read_id_gf(const char *gfname, int *shape, int *rank, double *data)
{
   double time;
   int ret;
   int ltrace=0;
   
   if(ltrace)
     fprintf(stderr,"gft_read_id_gf: gfname=<%s>\n",gfname);
   ret=gft_read_sdf_stream_brief(gfname,&time,shape,rank,data);
   if(ltrace)
     fprintf(stderr,"gft_read_id_gf: returning %d\n",ret);
   return ret;
}

int gft_read_id_int(const char *pname, int *param, int nparam)
{
   double time, *bbox=NULL, *cds=NULL, *data=NULL;
   int version,rank,dsize,csize,*shape=NULL;
   char *dpname=NULL,*cnames=NULL,*tag=NULL;
   int ret,i;
   gft_sdf_file_data *gp;
   int ltrace=0;
   
   if(ltrace)
      fprintf(stderr,"gft_read_id_int: pname=<%s> nparam=%d\n",pname,nparam);
   gp=gft_open_sdf_stream(pname);
   rewind(gp->fp);
   do{
      ret=low_read_sdf_stream(1,gp->fp,&time,&version,&rank,&dsize,&csize,
                                          &dpname,&cnames,&tag,&shape,&bbox,&cds,&data);
      if(rank || strcmp(dpname,pname)){
         if(dpname) free(dpname);
         if(cnames) free(cnames);
         if(tag) free(tag);
         if(shape) free(shape);
         if(bbox) free(bbox);
         if(cds) free(cds);
         if(data) free(data);
      }
   }while(ret && (rank || strcmp(dpname,pname)));
   if(ret){
      if(ltrace)
         fprintf(stderr,"gft_read_id_int: dpname=<%s>\n tag=<%s>\n",dpname,tag);
      ret=sget_param(tag,(char *)pname,"long",nparam,param,0);
      if(ret==-1)
         fprintf(stderr,"gft_read_id_int: can't find <%s> in <%s>\n",pname,tag);
      if(dpname) free(dpname);
      if(cnames) free(cnames);
      if(tag) free(tag);
      if(shape) free(shape);
      if(bbox) free(bbox);
      if(cds) free(cds);
      if(data) free(data);
   }else{
      fprintf(stderr,"gft_read_id_int: WARNING: can't find <%s> in file\n",pname);
   }
   return(ret);
}

int gft_read_id_float(const char *pname, double *param, int nparam)
{
   double time, *bbox=NULL, *cds=NULL, *data=NULL;
   int version,rank,dsize,csize,*shape=NULL;
   char *dpname=NULL,*cnames=NULL,*tag=NULL;
   int ret,i;
   gft_sdf_file_data *gp;
   int ltrace=0;
   
   if(ltrace)
      fprintf(stderr,"gft_read_id_float: pname=<%s> nparam=%d\n",pname,nparam);
   gp=gft_open_sdf_stream(pname);
   rewind(gp->fp);
   do{
      ret=low_read_sdf_stream(1,gp->fp,&time,&version,&rank,&dsize,&csize,
                                          &dpname,&cnames,&tag,&shape,&bbox,&cds,&data);
      if(rank || strcmp(dpname,pname)){
         if(dpname) free(dpname);
         if(cnames) free(cnames);
         if(tag) free(tag);
         if(shape) free(shape);
         if(bbox) free(bbox);
         if(cds) free(cds);
         if(data) free(data);
      }
   }while(ret && (rank || strcmp(dpname,pname)));
   if(ret){
      if(ltrace)
         fprintf(stderr,"gft_read_id_float: dpname=<%s>\n tag=<%s>\n",dpname,tag);
      ret=sget_param(tag,(char *)pname,"double",nparam,param,0);
      if(ret==-1)
         fprintf(stderr,"gft_read_id_float: can't find <%s> in <%s>\n",pname,tag);
      if(dpname) free(dpname);
      if(cnames) free(cnames);
      if(tag) free(tag);
      if(shape) free(shape);
      if(bbox) free(bbox);
      if(cds) free(cds);
      if(data) free(data);
   }else{
      fprintf(stderr,"gft_read_id_float: WARNING: can't find <%s> in file\n",pname);
   }
   return(ret);
}

int gft_read_id_str(const char *pname, char **param, int nparam)
{
   double time, *bbox=NULL, *cds=NULL, *data=NULL;
   int version,rank,dsize,csize,*shape=NULL;
   char *dpname=NULL,*cnames=NULL,*tag=NULL;
   int ret,i;
   gft_sdf_file_data *gp;
   int ltrace=0;
   
   if(ltrace)
      fprintf(stderr,"gft_read_id_str: pname=<%s> nparam=%d\n",pname,nparam);
   gp=gft_open_sdf_stream(pname);
   rewind(gp->fp);
   do{
      ret=low_read_sdf_stream(1,gp->fp,&time,&version,&rank,&dsize,&csize,
                                          &dpname,&cnames,&tag,&shape,&bbox,&cds,&data);
      if(rank || strcmp(dpname,pname)){
         if(dpname) free(dpname);
         if(cnames) free(cnames);
         if(tag) free(tag);
         if(shape) free(shape);
         if(bbox) free(bbox);
         if(cds) free(cds);
         if(data) free(data);
      }
   }while(ret && (rank || strcmp(dpname,pname)));
   if(ret){
      if(ltrace)
         fprintf(stderr,"gft_read_id_str: dpname=<%s>\n tag=<%s>\n",dpname,tag);
      ret=sget_param(tag,(char *)pname,"string",nparam,param,0);
      if(ret==-1)
         fprintf(stderr,"gft_read_id_str: can't find <%s> in <%s>\n",pname,tag);
      if(dpname) free(dpname);
      if(cnames) free(cnames);
      if(tag) free(tag);
      if(shape) free(shape);
      if(bbox) free(bbox);
      if(cds) free(cds);
      if(data) free(data);
   }else{
      fprintf(stderr,"gft_read_id_str: WARNING: can't find <%s> in file\n",pname);
   }
   return(ret);
}

int gft_read_rank(const char *gf_name, const int level, int *rank)
{
   int ret=1;
#ifdef HAVE_HDF
   if(getenv("BBHHDF"))
      ret=gft_read_hdf_rank(gf_name,level,rank);
   else
#endif
      ret=gft_read_sdf_rank(gf_name,level,rank);
   return ret;
}

int gft_read_shape(const char *gf_name, const int level, int *shape)
{
   int ret=1;
#ifdef HAVE_HDF
   if(getenv("BBHHDF"))
      ret=gft_read_hdf_shape(gf_name,level,shape);
   else
#endif
      ret=gft_read_sdf_shape(gf_name,level,shape);
   return ret;
}

int gft_read_bbox(const char *gf_name, const int level, double *bbox)
{
   int ret=1;
#ifdef HAVE_HDF
   if(getenv("BBHHDF"))
      fprintf(stderr,"gft_read_bbox: HDF version of this routine is not implemented.\n"), ret = 0;
   else 
#endif
      ret=gft_read_sdf_bbox(gf_name,level,bbox);
   return ret;
}


int gft_read_name(const char *file_name, const int n, char *name)
{
   int ret=1;
#ifdef HAVE_HDF
   if(getenv("BBHHDF"))
      ret=gft_read_hdf_name(file_name,n,name);
   else
#endif
      ret=gft_read_sdf_name(file_name,n,name);
   return ret;
}

int gft_read_full(const char *gf_name, int level, int *shape, char *cnames, 
                    int rank, double *time, double *coords, double *data)
{
   int ret=1;
#ifdef HAVE_HDF
   if(getenv("BBHHDF"))
      ret=gft_read_hdf_full(gf_name,level,shape,cnames,rank,time,coords,data);
   else
#endif
      ret=gft_read_sdf_full(gf_name,level,shape,cnames,rank,time,coords,data);
   return ret;
}

int gft_out_slice(char *func_name, double time, int *shape, 
                  char *cnames, int rank, double *coords, double *data, 
                           char *slvec)
{
   int nrank,*nshape;
   double *ncoords,*ndata;
   char *iv;
   return 1;
}

int gft_read_brief(const char *gf_name, int level,  double *data )
{
   int ret=1;
#ifdef HAVE_HDF
   if(getenv("BBHHDF"))
      ret=gft_read_hdf_brief(gf_name,level,data);
   else
#endif
      ret=gft_read_sdf_brief(gf_name,level,data);
   return ret;
}

int gft_out_full(char *func_name, double time, int *shape, 
                 char *cnames, int rank, double *coords, double *data)
{
   int   ltrace = 0;
	int   cs;
	FILE *stream;

   int   ret = 1;
   
  if(getenv("BBHHOST")){
      if(getenv("BBHSDF")){
         ret=gft_send_sdf_stream(func_name,time,shape,cnames,rank,coords,data);
#ifdef HAVE_LIBSV
      }else if(getenv("BBHSV")){
         if( getenv("JSERHOST") ) {
            if( ltrace ) {
               fprintf(stderr,"gft_out_full: Transmitting '%s' to SCIVIS @ %s\n",
                       func_name,getenv("JSERHOST"));
               fprintf(stderr,"              Time: %g  Rank: %d  cnames <%s>\n",
                       time,rank,cnames);
               if( rank == 2 ) {
                  fprintf(stderr,"              Shape: (%d,%d)\n",shape[0],shape[1]);
               }
            }
            ret=gft_out_full_sv(func_name,time,shape,cnames,rank,coords,data);
         } else {
            ret=0;
            fprintf(stderr,"gft_out_full: please set the environment variable JSERHOST\n");
            fprintf(stderr,"  to the name of the machine which is running scivis.  Try\n");
            fprintf(stderr,"setenv JSERHOST \"%s\"\n",getenv("BBHHOST"));
            if(rank==3 && !getenv("ISOHOST")){
               fprintf(stderr,"To send 3D data sets, you need to also\n");
               fprintf(stderr,"setenv ISOHOST \"%s\"\n",getenv("BBHHOST"));
            }
         }
#endif
    }else{
        fprintf(stderr,"gft_out_full: no server type has been selected.\n");
         fprintf(stderr,"  please either\nsetenv BBHSDF on\n");
         fprintf(stderr,"  for output to an SDF server, or\nsetenv BBHSV on\n");
         fprintf(stderr,"  for ouptut to scivis.\n");
      }
#ifdef HAVE_HDF
  } else if(getenv("BBHHDF")){
    ret=gft_write_hdf_full(func_name,time,shape,cnames,rank,coords,data);
#endif
   } else if( getenv("DVHOST") && getenv("DVI") ) {
     ltrace && fprintf(stderr,"gft_out_full: DVHOST=%s\n",getenv("DVHOST"));
     ltrace && fprintf(stderr,"gft_out_full: DVI=%s\n",getenv("DVI"));

     ltrace && fprintf(stderr,"gft_out_full: gft_dv_port()=%d\n",gft_dv_port());
		gft_dv_standard_init(&cs,&stream,func_name,"DVHOST","DVPORT");
		1 && fprintf(stderr,"gft_out_full: cs=%d\n",cs);
		if( cs >= 0 ) {
			/* TODO:  Figure out dsize, csize, ...
			ret=low_write_sdf_stream(stream,func_name,time,rank,
					dsize,csize,cnames,"",shape,coords,data);
			*/
			gft_dv_standard_clean_up(cs,stream);
		}
   } else{
    ret=gft_write_sdf_stream(func_name,time,shape,cnames,rank,coords,data);
  }
   return ret;
}

int gft_out_bbox(char *func_name, double time, int *shape, int rank,
                 double *coords, double *data)
{
   int ret=1;
   
  if(getenv("BBHHOST")){
      if(getenv("BBHSDF"))
         ret=gft_send_sdf_stream_bbox(func_name,time,shape,rank,coords,data);
#ifdef HAVE_LIBSV
      else if(getenv("BBHSV"))
         if(getenv("JSERHOST"))
            ret=gft_out_bbox_sv(func_name,time,shape,rank,coords,data);
         else{
            ret=0;
            fprintf(stderr,"gft_out_bbox: please set the environment variable JSERHOST\n");
            fprintf(stderr,"  to the name of the machine which is running scivis.  Try\n");
            fprintf(stderr,"setenv JSERHOST \"%s\"\n",getenv("BBHHOST"));
            if(rank==3 && !getenv("ISOHOST")){
               fprintf(stderr,"To send 3D data sets, you need to also\n");
               fprintf(stderr,"setenv ISOHOST \"%s\"\n",getenv("BBHHOST"));
            }
         }
#endif
    else{
        fprintf(stderr,"gft_out_bbox: no server type has been selected.\n");
         fprintf(stderr,"  please either\nsetenv BBHSDF on\n");
         fprintf(stderr,"  for output to an SDF server, or\nsetenv BBHSV on\n");
         fprintf(stderr,"  for ouptut to scivis.\n");
      }
#ifdef HAVE_HDF
  }else if(getenv("BBHHDF")){
    ret=gft_write_hdf_bbox(func_name,time,shape,rank,coords,data);
#endif
   }else{
    ret=gft_write_sdf_stream_bbox(func_name,time,shape,rank,coords,data);
  }
   return ret;
}

int gft_out_brief(char *func_name, double time, int *shape, int rank, 
                  double *data)
{
   int ret=1;
   
  if(getenv("BBHHOST")){
      if(getenv("BBHSDF"))
         ret=gft_send_sdf_stream_brief(func_name,time,shape,rank,data);
#ifdef HAVE_LIBSV
      else if(getenv("BBHSV"))
         if(getenv("JSERHOST"))
            ret=gft_out_brief_sv(func_name,time,shape,rank,data);
         else{
            ret=0;
            fprintf(stderr,"gft_out_brief: please set the environment variable JSERHOST\n");
            fprintf(stderr,"  to the name of the machine which is running scivis.  Try\n");
            fprintf(stderr,"setenv JSERHOST \"%s\"\n",getenv("BBHHOST"));
            if(rank==3 && !getenv("ISOHOST")){
               fprintf(stderr,"To send 3D data sets, you need to also\n");
               fprintf(stderr,"setenv ISOHOST \"%s\"\n",getenv("BBHHOST"));
            }
         }
#endif
    else{
        fprintf(stderr,"gft_out_brief: no server type has been selected.\n");
         fprintf(stderr,"  please either\nsetenv BBHSDF on\n");
         fprintf(stderr,"  for output to an SDF server, or\nsetenv BBHSV on\n");
         fprintf(stderr,"  for ouptut to scivis.\n");
      }
#ifdef HAVE_HDF
  }else if(getenv("BBHHDF")){
    ret=gft_write_hdf_bbox(func_name,time,shape,rank,BBH_bbox,data);    
#endif
    }else{
    ret=gft_write_sdf_stream_brief(func_name,time,shape,rank,data);
  }
   return ret;
}

int gft_out(char *func_name, double time,int *shape, int rank, 
            double *data)
{
   return gft_out_brief(func_name,time,shape,rank,data);
}

int gft_out_set_bbox(double *bbox, int rank)
{
   int ret=1;
   
   if(rank>0 && rank<5){
      rdvcpy(BBH_bbox,bbox,2*rank);
   }else ret=0;
   return ret;
}

int gft_outm(const char *func_name, double *time, int *shape, const int nt,
             int *rank, double *data)
{
   return gft_outm_brief(func_name,time,shape,nt,rank,data);
}

int gft_outm_brief(const char *func_name, double *time, int *shape, const int nt, 
                   int *rank, double *data)
{
   int ret=1;
   
  if(getenv("BBHHOST")){
      if(getenv("BBHSDF"))
         ret=gft_sendm_sdf_stream_brief((char *)func_name,time,shape,nt,rank,data);
#ifdef HAVE_LIBSV
      else if(getenv("BBHSV"))
         if(getenv("JSERHOST"))
            ret=gft_outm_brief_sv(func_name,time,shape,nt,rank,data);
         else{
            ret=0;
            fprintf(stderr,"gft_outm_brief: please set the environment variable JSERHOST\n");
            fprintf(stderr,"  to the name of the machine which is running scivis.  Try\n");
            fprintf(stderr,"setenv JSERHOST \"%s\"\n",getenv("BBHHOST"));
            if(!getenv("ISOHOST")){
               fprintf(stderr,"If you are trying to send 3D data sets, you need to also\n");
               fprintf(stderr,"setenv ISOHOST \"%s\"\n",getenv("BBHHOST"));
            }
         }
#endif
    else{
        fprintf(stderr,"gft_outm_brief: no server type has been selected.\n");
         fprintf(stderr,"  please either\nsetenv BBHSDF on\n");
         fprintf(stderr,"  for output to an SDF server, or\nsetenv BBHSV on\n");
         fprintf(stderr,"  for ouptut to scivis.\n");
      }
    }else{
      ret=gft_writem_sdf_stream_brief((char *)func_name,time,shape,nt,rank,data);
  }
   return ret;
}

int gft_outm_bbox(const char *func_name, double *time, int *shape, const int nt, 
                  int *rank, double *coords, double *data)
{
   int ret=1;
   
  if(getenv("BBHHOST")){
      if(getenv("BBHSDF"))
         ret=gft_sendm_sdf_stream_bbox((char *)func_name,time,shape,nt,rank,coords,data);
#ifdef HAVE_LIBSV
      else if(getenv("BBHSV"))
         if(getenv("JSERHOST"))
            ret=gft_outm_bbox_sv(func_name,time,shape,nt,rank,coords,data);
         else{
            ret=0;
            fprintf(stderr,"gft_outm_bbox: please set the environment variable JSERHOST\n");
            fprintf(stderr,"  to the name of the machine which is running scivis.  Try\n");
            fprintf(stderr,"setenv JSERHOST \"%s\"\n",getenv("BBHHOST"));
            if(!getenv("ISOHOST")){
               fprintf(stderr,"If you are trying to send 3D data sets, you need to also\n");
               fprintf(stderr,"setenv ISOHOST \"%s\"\n",getenv("BBHHOST"));
            }
         }
#endif
    else{
        fprintf(stderr,"gft_outm_bbox: no server type has been selected.\n");
         fprintf(stderr,"  please either\nsetenv BBHSDF on\n");
         fprintf(stderr,"  for output to an SDF server, or\nsetenv BBHSV on\n");
         fprintf(stderr,"  for ouptut to scivis.\n");
      }
   }else{
    ret=gft_writem_sdf_stream_bbox((char *)func_name,time,shape,nt,rank,coords,data);
  }
   return ret;
}

int gft_outm_full(const char *func_name, double *time, int *shape, char **cnames, 
                  const int nt, int *rank, double *coords, double *data)
{
   int ret=1;
   
  if(getenv("BBHHOST")){
      if(getenv("BBHSDF"))
         ret=gft_sendm_sdf_stream((char *)func_name,time,shape,cnames,nt,rank,coords,data);
#ifdef HAVE_LIBSV
      else if(getenv("BBHSV"))
         if(getenv("JSERHOST"))
            ret=gft_outm_full_sv(func_name,time,shape,cnames,nt,rank,coords,data);
         else{
            ret=0;
            fprintf(stderr,"gft_outm_full: please set the environment variable JSERHOST\n");
            fprintf(stderr,"  to the name of the machine which is running scivis.  Try\n");
            fprintf(stderr,"setenv JSERHOST \"%s\"\n",getenv("BBHHOST"));
            if(!getenv("ISOHOST")){
               fprintf(stderr,"If you are trying to send 3D data sets, you need to also\n");
               fprintf(stderr,"setenv ISOHOST \"%s\"\n",getenv("BBHHOST"));
            }
         }
#endif
    else{
        fprintf(stderr,"gft_outm_full: no server type has been selected.\n");
         fprintf(stderr,"  please either\nsetenv BBHSDF on\n");
         fprintf(stderr,"  for output to an SDF server, or\nsetenv BBHSV on\n");
         fprintf(stderr,"  for ouptut to scivis.\n");
      }
   }else{
    ret=gft_writem_sdf_stream((char *)func_name,time,shape,cnames,nt,rank,coords,data);
  }
   return ret;
}

char *gft_verify_file(char *tag, int trace) {
   static char P[] = "gft_verify_file";

   char                *fname = (char *) NULL;
   gft_sdf_file_data   *gp;
   int                  ltrace = 0;

   ltrace = (trace > 1); 
   fname = strdup(tag); gft_set_single(fname); gp=gft_open_sdf_stream(fname);
   if( gp == NULL ) {
      if( ltrace ) fprintf(stderr,"%s SDF open of %s failed.\n",P,fname);
      free(fname);
      gft_set_multi(); 
      fname = gft_make_sdf_name(tag); gp=gft_open_sdf_stream(fname);
      if( gp == NULL ) {
        if( ltrace ) fprintf(stderr,"%s SDF open of %s failed.\n",P,fname);
        fprintf(stderr,"%s: Cannot open sdf file '%s' or '%s'\n",P,tag,
                gft_make_sdf_name(tag));
         free(fname);
         fname = (char *) NULL;
      }
   }
   if( ltrace ) fprintf(stderr,"%s: --> single mode, name '%s'\n",P,fname);
   if( fname )  gft_set_single(fname);
   if( trace && fname) {
      fprintf(stderr,"%s: SDF stream '%s' opened ('%s' requested)\n",
              P,fname,tag);
   }
   return fname;
}

gft_sdf_file_data *gft_open_sdf_file(char *fname) {
   int ltrace = 0;
   gft_sdf_file_data *gp;

   gp = gft_open_sdf_stream(fname);
   if( gp && ltrace) fprintf(stderr,"gft_open_sdf_stream(%s) succeeded\n",
                             fname);
   if( gp == NULL ) {
      gp = gft_open_sdf_stream(gft_make_sdf_name(fname));
      if( ltrace)  fprintf(stderr,"gft_open_sdf_stream(%s) %s\n",
                           gft_make_sdf_name(fname),
                           gp ? "succeeded" : "failed");
   }
   return gp;
}

int gft_is_sdf_file(char *fname) {
	gft_sdf_file_data *gp;
	int rval = 0;

	if( (gp = gft_open_sdf_stream(gft_make_sdf_name(fname))) ) {
		rval = 1;
		gft_close_sdf_stream(fname);
	} 
	return rval;
}

/* Particle output interface
 * MWC, UBC, 2005.03
 *
 * Particle data is stored in array with dims 
 * 
 *    fortran(np,3+nf)
 *
 * May not be best idea to hardcode 3d, but will do so anyway
 *
 *    nf := "Number of field vars"
 *
 * Examples
 *
 *    nf = 0:   Just the particle coords
 *    nf = 1:   Particle coords + 1 DOF per particle (e.g. pressure, temperature)
 *    nf = n:   Particle coords + n DOF per particle (e.g. pressure AND temperature ...)
 *
 * Uses gft_out_bbox, where bbox is defined as bbox of particle data (computed as side 
 * effect of call
 *    */

int gft_out_part_xyz(const char *func_name, double t, 
      double *x, double *y, double *z,int np) {
   static char R[] ="gft_out_part_xyz";

   /* Hard coded dimensionality! */
   static int d=3;

   int i, rc;

   int ltrace = 0;

   int    *shape = (int *) NULL;
   double *bbox = (double *) NULL;
   double *data = (double *) NULL;


   if( !np ) {
      return -1;
   }
   if( !(bbox = (double *) malloc(2*d*sizeof(double))) ) {
      fprintf(stderr,"%s: bbox malloc failed!\n",R);
      rc = -1;
      goto Cleanup;
   }
   if( !(shape = (int *) malloc(2*sizeof(int))) ) {
      fprintf(stderr,"%s: shape malloc failed!\n",R);
      rc = -1;
      goto Cleanup;
   }
   shape[0] = np;
   shape[1] = 3;

   bbox[0] = rdvmin(x,np);
   bbox[1] = rdvmax(x,np);

   bbox[2] = rdvmin(y,np);
   bbox[3] = rdvmax(y,np);

   bbox[4] = rdvmin(z,np);
   bbox[5] = rdvmax(z,np);

   if( ltrace ) {
      fprintf(stderr,"%s:  t=%g\n",R,t);        
      fprintf(stderr,"%s:  np=%d\n",R,np);        
      fprintf(stderr,"%s:  SDF shape=[%d %d]\n",R,shape[0],shape[1]);        
      fprintf(stderr,"%s:  SDF bbox=[%g %g %g %g %g %g]\n",
            R, bbox[0], bbox[1], bbox[2], bbox[3], bbox[4], bbox[5]);
   }
   /* Create and initialize temporary array for SDF output.  Note again that shape is 
    * (np,3+nf) AND IN FORTRAN STORAGE ORDER */

   if( !(data = (double *) malloc(d*np*sizeof(double))) ) {
      fprintf(stderr,"%s: data malloc failed!\n",R);
      rc = -1;
      goto Cleanup;
   }

   for( i = 0; i < np; i++ ) {
      data[i]        = x[i];
      data[np + i]   = y[i];
      data[2*np + i] = z[i];
   }

   if( ltrace ) fprintf(stderr,"%s: calling gft_out_bbox\n",R);
   rc = gft_out_bbox((char *) func_name,t,shape,2,bbox,data);
   if( ltrace ) fprintf(stderr,"%s: gft_out_bbox returns %d\n",R,rc);

   /* Free temporary storage */
Cleanup:
   if( bbox ) free(bbox);
   if( shape ) free(shape);
   if( data ) free(data);

   return rc;

}

int gft_out_part_xyzf(const char *func_name, double t, double *xyzf, int np, int nf) {
   static char R[] ="gft_out_part_xyz";

   /* Hard coded dimensionality! */
   static int d=3;

   int i, j, rc;

   int ltrace = 0;

   int    *shape = (int *) NULL;
   double *bbox = (double *) NULL;


   if( !np ) {
      return -1;
   }
   if( !(bbox = (double *) malloc(2*d*sizeof(double))) ) {
      fprintf(stderr,"%s: bbox malloc failed!\n",R);
      rc = -1;
      goto Cleanup;
   }
   if( !(shape = (int *) malloc(2*sizeof(int))) ) {
      fprintf(stderr,"%s: shape malloc failed!\n",R);
      rc = -1;
      goto Cleanup;
   }
   shape[0] = np;
   shape[1] = 3 + nf;

   bbox[0] = rdvmin(xyzf,np);
   bbox[1] = rdvmax(xyzf,np);

   bbox[2] = rdvmin(xyzf+np,np);
   bbox[3] = rdvmax(xyzf+np,np);

   bbox[4] = rdvmin(xyzf+2*np,np);
   bbox[5] = rdvmax(xyzf+2*np,np);

   if( ltrace ) {
      fprintf(stderr,"%s:  t=%g\n",R,t);        
      fprintf(stderr,"%s:  np=%d\n",R,np);        
      fprintf(stderr,"%s:  nf=%d\n",R,nf);        
      fprintf(stderr,"%s:  SDF shape=[%d %d]\n",R,shape[0],shape[1]);        
      fprintf(stderr,"%s:  SDF bbox=[%g %g %g %g %g %g]\n",
            R, bbox[0], bbox[1], bbox[2], bbox[3], bbox[4], bbox[5]);
   }
   /* Can just dump data directly in this case. */

   if( ltrace ) fprintf(stderr,"%s: calling gft_out_bbox\n",R);
   rc = gft_out_bbox((char *) func_name,t,shape,2,bbox,xyzf);
   if( ltrace ) fprintf(stderr,"%s: gft_out_bbox returns %d\n",R,rc);

   /* Free temporary storage */
Cleanup:
   if( bbox ) free(bbox);
   if( shape ) free(shape);

   return rc;

}

int gft_bbh_port(int default_bbh_port) {
	int ltrace = 0;
	int bbh_port;

	if( getenv("BBHPORT") ) {
		if( !sscanf(getenv("BBHPORT"),"%d",&bbh_port) ) {
			bbh_port = default_bbh_port;
		}
	} else {
		bbh_port = default_bbh_port;
	}
	ltrace && fprintf(stderr,"gft_bbh_port: BBHPORT=%d\n",bbh_port);

	return bbh_port;
}

int gft_dv_port(void) {
	int ltrace = 0;
	int dv_port, default_dv_port = 5006;

	if( ! (getenv("DVPORT") && (sscanf(getenv("DVPORT"),"%d",&dv_port) == 1)) )
		dv_port = default_dv_port;
	ltrace && fprintf(stderr,"gft_dv_port: DVPORT=%d\n",dv_port);
	return dv_port;
}

/*-------------------------------------------------------------------------*/
/* Next two routines ... */
/* History: DV/util/common_fncs.c */

void gft_dv_standard_init(int *cs,FILE **stream,char *name,char *DVHOST, char *DVPORT)
{
   int DV_PORT;
   int ltrace=0;
	static int first_mess = 1;

   if (!getenv(DVHOST)) 
   {
      printf("%s: Environment variable %s not set\n",name,DVHOST);
      exit(-1);
   }
                                          
   if( ! (getenv(DVPORT) && (sscanf(getenv(DVPORT),"%d",&DV_PORT) == 1)) )
   {
      if (!(strcmp(DVPORT,"DVRPORT"))) DV_PORT = DEFAULT_DVR_PORT;
      else DV_PORT = DEFAULT_DV_PORT;
   }
   if( ltrace ) printf("standard_init: DV_PORT = %d\n",DV_PORT);

   if ((*cs=ser0_connect(getenv(DVHOST),DV_PORT))<0) 
   {
		if( first_mess ) {
			fprintf(stderr,
					"%s: Could not connect to DV on '%s'\n",name,getenv(DVHOST));
			first_mess = 0;
		}
   }

   if (!(*stream=fdopen(*cs,"w"))) 
   {
		if( first_mess ) {
			fprintf(stderr,"%s: fdopen(%d) (DV socket) failed\n",name,*cs);
			first_mess = 0;
		}
   }
   return;
}


void gft_dv_standard_clean_up(int cs,FILE *stream)
{
   fclose(stream);
   close(cs);
}     
/*-------------------------------------------------------------------------*/

int gft_set_append(void) {
   return setenv("GFT_APPEND", "on", 1);
}

int gft_unset_append(void) {
   return unsetenv("GFT_APPEND");
}

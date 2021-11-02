/* $Header: /home/cvs/rnpl/src/bbhutil.h,v 1.3 2019/10/13 23:53:48 cvs Exp $ */
/* bbhutil.h */
/* Copyright (c) 1995 by Robert Marsa */
#ifndef _BBHUTIL_H
#define _BBHUTIL_H

#include <sdf.h>
#include <sys/types.h>
#include <sys/sysmacros.h>

#define MAXRANK 5

#ifdef __cplusplus
extern "C" {
#endif

/* For communication with gpar.tab.c for parsing assignments of vectors */
/* with arbitrary number of elements. */

extern void **Gpar_pr;
extern int  *Gpar_prsize;

char *lowcase(char *s);

#ifdef DONT_HAVE_STRDUP
char *strdup(const char *s);
#endif

int strcmp_nc(const char *s1, const char *s2);

void rdvcpy(double *r,double *s, int len);
double rdvprod(double *r, int len);
int rivprod(int *r, int len);
void rivcpy(int *r, int *s, int len);
void rdvramp(double *r, int len, double min, double inc);
void rdvaddc(double *r, double c1, double *s1, double c2, double *s2, int len);
double rdvupdatemean(double *r, double *s1, double *s2, int len);

int read_ascii_int(const char *fname, int *p, const int len);
int read_ascii_double(const char *fname, double *p, const int len);
int read_ascii_string(const char *fname, char **p, const int len);
int read_ascii_ivec(const char *fname, int **p, const int len, const int flag);
int read_ascii_fvec(const char *fname, double **p, const int len, const int flag);

/* return l2 norm of vec */
double l2norm(int N, double *vec);

/* Extrema of vectors */
double rdvmin(double *v, int n);
double rdvmax(double *v, int n);

/* allocate a vector of size doubles, print message if
   not enough memory */
double * vec_alloc(int size);

/* returns a vector of size ints;  
   prints error message and exits if not enough memory */
int * ivec_alloc(int size);
char * cvec_alloc(int size);

/* allocate a vector of size doubles, named name.  Print
   error message and exit if not enough memory */
double * vec_alloc_n(int size, char *name);

/* returns a vector of size ints named name;  
   prints error message and exits if not enough memory */
int * ivec_alloc_n(int size, char *name);
char * cvec_alloc_n(int size, char *name);

int sget_fvec_param(const char *string, const char *name,
                    double *p, int size);

int sget_ivec_param(const char *string, const char *name,
                    int *p, int size);

int sget_int_param(const char *string, const char *name,
                   int *p, int size);

int sget_real_param(const char *string, const char *name,
                    double *p, int size);

int sget_str_param(const char *string, const char *name,
                   char **p, int size);

int get_fvec_param(const char *p_file, const char *name,
                    double *p, int size);

int get_ivec_param(const char *p_file, const char *name,
                    int *p, int size);

int get_int_param(const char *p_file, const char *name,
                   int *p, int size);

int get_real_param(const char *p_file, const char *name,
                    double *p, int size);

int get_str_param(const char *p_file, const char *name,
                   char **p, int size);

int get_int_param_v(const char *p_file, const char *name, 
                   int **pp, int *psize);

int get_real_param_v(const char *p_file, const char *name, 
                    double **pp, int *psize);

int get_str_param_v(const char *p_file, const char *name,
                   char ***pp, int *psize);

int is_param_assigned_in_file(char *fname, char *param);
int is_param_assigned_in_file_nc(char *fname, char *param);
int is_param_assigned_in_file_0(char *fname, char *param, int cs);

int sget_param(const char *string, const char *name,
               char *type, int size, void *p, int cs);
/* search ascii file for a line of the form
   name := value.
   Assign value to p.
*/
int get_param(const char *p_file, const char *name,
               char *type, int size, void *p);

int get_param_nc(const char *p_file, const char *name,
                   char *type, int size, void *p);

int get_param_nc_v(const char *p_file, const char *name,
                   char *type, int *psize, void **pp);

int do_ivec(const int it, const int niter, int *ivec);
void fixup_ivec(const int min, const int max, const int lev, int *ivec);

void fixup_fvec(double *fv);
int size_fvec(double *fv);
double get_fvec(double *fv, const int i);

/* takes constant args and returns array */
double *gft_make_bbox(int rank, ...);

/*  returns full function name:
      func_name(c0,c1,c2,...,t);
    rank is the spacial rank
    cnames[rank] contains the spacial coordinate names
*/
char *gft_make_full_func_name(char *func_name, int rank, char **cnames);
void gft_close(const char *nm);
void gft_close_all(void);

#ifdef HAVE_HDF

typedef struct {
  char *fn;
  char *gfn;
  int vers;
  int mode;
  int fid;
  int state;
  int sdsnum;
  int nslabs;
  int rank;
  double *tmv;
  time_t mtime;
} hdf_data;

int gft_init(void);
void gft_del(void);
void gft_clear(const int ind);
char * gft_make_file_name(const char *s);
char * gft_strip_suf(const char *s);
int gft_search_list(const char *gfn, hdf_data *list, int n);
int gft_search_list_id(const char *fn, hdf_data *list, int n);
void gft_dump_hdf_data(hdf_data *df);
int gft_newstruct(hdf_data **df, int len);
void gft_close_all_hdf(void);
int gft_set_d_type(int type);
int gft_check_file(const int fid, const char *nm);
int gft_set_file_rnpl(const int fid, const char *nm, const int vers);
void gft_set_single_hdf(const char *nm);
void gft_set_multi_hdf(void);
int gft_create(const char *nm);
int gft_open_rd(const char *nm);
int gft_create_id(const char *nm);
int gft_open_rd_id(const char *nm);
void gft_close_hdf(const char *nm);

int GFT_extract(const char *fname, /* in: file name */
          const int tlev, /* in: time level to retrieve (-ntlev - -1 : 1 - ntlev) */
          char **func_name, /* name of this grid function */
          int *ntlev, /* number of time levels available in file */
          double **tvec, /* vector of time values */
          double *time, /* time of this slice */
          int *drank, /* rank of data set */
          int **shape, /* shape of data set */
          char ***cnames, /* names of coordinates */
          double ***coords, /* values of coordinates */
          double **data ); /* actual data */

int gft_write_hdf(const char *func_name, /* name of this grid function */
                double time, /* time of this slice */
                int *shape, /* shape of data set */
                int rank, /* rank of data set */
                double *data ); /* actual data */

int gft_write_hdf_brief(const char *func_name, /* name of this grid function */
                      double time, /* time of this slice */
                      int *shape, /* shape of data set */
                      int rank, /* rank of data set */
                      double *data ); /* actual data */

int gft_write_hdf_bbox(const char *func_name, /* name of this grid function */
                    double time, /* time of this slice */
                    int *shape, /* shape of data set */
                    int rank, /* rank of data set */
                    double *coords, /* values of coordinate bboxes */
                    double *data ); /* actual data */

int gft_write_hdf_full(const char *func_name, /* name of this grid function */
                    double time, /* time of this slice */
                    int *shape, /* shape of data set */
                    char *cnames, /* names of coordinates */
                    int rank, /* rank of data set */
                    double *coords, /* values of coordinates */
                    double *data ); /* actual data */

int gft_read_hdf_shape(const char *gf_name, /* IN: */
                      const int level, /* IN: */
                      int *shape); /* OUT: */

int gft_read_hdf_name(const char *file_name, /* IN: */
                      const int n, /* IN: */
                      char *name); /* OUT: */

int gft_read_hdf_rank(const char *gf_name, /* IN: */
                      const int level, /* IN: */
                      int *drank); /* OUT: */

int gft_read_hdf_full(const char *gf_name, /* IN: name of this file */
                    int level, /* IN: time level */
                    int *shape, /* OUT: shape of data set */
                    char *cnames, /* OUT: names of coordinates */
                    int rank, /* IN: rank of data set */
                    double *time, /* OUT: time value */
                    double *coords, /* OUT: values of coordinates */
                    double *data ); /* OUT: actual data */

int gft_read_hdf_brief(const char *gf_name, /* IN: name of this file */
                    int level, /* IN: time level */
                    double *data ); /* OUT: actual data */

#endif /* HAVE_HDF */

int gft_read_id_str_p(const char *file_name, const char *param_name,
                        char **param, int nparam);
int gft_read_id_int_p(const char *file_name, const char *param_name,
                        int *param, int nparam);
int gft_read_id_float_p(const char *file_name, const char *param_name,
                        double *param, int nparam);
int gft_read_2idata(const char *file_name, /* name of idata file */
                    const char *func_name, /* name of this grid function */
                    int *shape, /* shape of data set */
                    int rank, /* rank of data set */
                    double *datanm1,
                    double *datan ); /* actual data */
int gft_read_1idata(const char *file_name, /* name of idata file */
                    const char *func_name, /* name of this grid function */
                    int *shape, /* shape of data set */
                    int rank, /* rank of data set */
                    double *datan ); /* actual data */
int gft_read_idata(const char *file_name, /* name of idata file */
                    const char *func_name, /* name of this grid function */
                    int *shape, /* shape of data set */
                    int rank, /* rank of data set */
                    double *data); /* actual data */

int gft_write_id_str_p(const char *file_name, const char *param_name,
                        char **param, int nparam);
int gft_write_id_int_p(const char *file_name, const char *param_name,
                        int *param, int nparam);
int gft_write_id_float_p(const char *file_name, const char *param_name,
                        double *param, int nparam);
int gft_write_2idata(const char *file_name, /* name of idata file */
                    const char *func_name, /* name of this grid function */
                    int *shape, /* shape of data set */
                    int rank, /* rank of data set */
                    double *datanm1,
                    double *datan ); /* actual data */
int gft_write_1idata(const char *file_name, /* name of idata file */
                    const char *func_name, /* name of this grid function */
                    int *shape, /* shape of data set */
                    int rank, /* rank of data set */
                    double *datan ); /* actual data */
int gft_write_idata(const char *file_name, /* name of idata file */
                    const char *func_name, /* name of this grid function */
                    int *shape, /* shape of data set */
                    int rank, /* rank of data set */
                    double *data); /* actual data */

#ifdef __cplusplus
}
#endif

#endif /* _BBHUTIL_H */

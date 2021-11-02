/* sdf.h */
/* I/O routines for sdf files */
/* Copyright (c) 1997 by Robert Marsa */
/* $Header: /home/cvs/rnpl/src/sdf.h,v 1.1.1.1 2013/07/09 00:38:27 cvs Exp $ */
#include <stdio.h>

#ifndef _SDF_H
#define _SDF_H

#define BBH_PORT0 5005
#define SDFVERSION 1

#ifdef __cplusplus
extern "C" {
#endif

void gft_set_single_sdf(const char *nm);
void gft_set_multi_sdf(void);
void gft_set_single(const char *nm);
void gft_set_multi(void);
char *gft_make_sdf_name(const char *gfname);
                        
int gft_send_sdf_stream(char *gfname, double time, int *shape,
                        char *cnames, int rank, double *coords, double *data);
int gft_send_sdf_stream_bbox(char *gfname, double time, int *shape,
                             int rank, double *coords, double *data);
int gft_send_sdf_stream_brief(char *gfname, double time, int *shape,
                              int rank, double *data);
int gft_write_sdf_stream(char *gfname, double time, int *shape,
                         char *cnames, int rank, double *coords, double *data);
int gft_write_sdf_stream_bbox(char *gfname, double time, int *shape,
                              int rank, double *coords, double *data);
int gft_write_sdf_stream_brief(char *gfname, double time, int *shape,
                               int rank, double *data);
int gft_read_sdf_stream(const char *gfname, double *time, int *shape,
                        char *cnames, int *rank, double *coords, double *data);
int gft_read_sdf_stream_brief(const char *gfname, double *time, int *shape,
                              int *rank, double *data);
int gft_read_sdf_shape(const char *gf_name, const int level, int *shape);
int gft_read_sdf_bbox(const char *gf_name, const int level, double *bbox);
int gft_read_sdf_rank(const char *gf_name, const int level, int *rank);
int gft_read_sdf_name(const char *file_name, const int n, char *name);
int gft_read_sdf_ntlevs(const char *file_name);
int gft_read_sdf_full(const char *gf_name, /* IN: name of this file */
                    int level, /* IN: time level */
                    int *shape, /* OUT: shape of data set */
                    char *cnames, /* OUT: names of coordinates */
                    int rank, /* IN: rank of data set */
                    double *time, /* OUT: time value */
                    double *coords, /* OUT: values of coordinates */
                    double *data ); /* OUT: actual data */

int gft_read_sdf_brief(const char *gf_name, /* IN: name of this file */
                    int level, /* IN: time level */
                    double *data ); /* OUT: actual data */

void gft_close_sdf_stream(const char *gfname);
void gft_close_all_sdf(void);

int gft_extract_sdf(const char *fn, /* in: file name */
          const int tlev, /* in: time level to retrieve (1 - ntlev) */
          char **func_name, /* name of this grid function */
          int *ntlev, /* number of time levels available in file */
          double **tvec, /* vector of time values */
          double *time, /* time of this slice */
          int *drank, /* rank of data set */
          int **shape, /* shape of data set */
          int *csize, /* size of coords */
          char **cnames, /* names of coordinates (packed) */
          double **coords, /* values of coordinates (packed) */
          double **data); /* actual data */

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
          double **data); /* actual data */

int gft_write_id_gf(char *gfname, int *shape, int rank, double *data);
int gft_write_id_int(char *pname, int *param, int nparam);
int gft_write_id_float(char *pname, double *param, int nparam);
int gft_write_id_str(char *pname, char **param, int nparam);

int gft_read_id_gf(const char *gfname, int *shape, int *rank, double *data);
int gft_read_id_int(const char *pname, int *param, int nparam);
int gft_read_id_float(const char *pname, double *param, int nparam);
int gft_read_id_str(const char *pname, char **param, int nparam);

int gft_out_slice(char *func_name, double time, int *shape, 
                  char *cnames, int rank, double *coords, double *data, 
                  char *slvec);
int gft_out_full(char *func_name, double time, int *shape, 
                 char *cnames, int rank, double *coords, double *data);
int gft_out_bbox(char *func_name, double time, int *shape, int rank,
                 double *coords, double *data);
int gft_out_brief(char *func_name, double time, int *shape, int rank, 
                  double *data);
int gft_out(char *func_name, double time,int *shape, int rank, double *data);
int gft_out_set_bbox(double *bbox, int rank);

int gft_outm(const char *func_name, double *time, int *shape, const int nt, 
             int *rank, double *data);
int gft_outm_brief(const char *func_name, double *time, int *shape, const int nt, 
                   int *rank, double *data);
int gft_outm_bbox(const char *func_name, double *time, int *shape, const int nt, 
                  int *rank, double *coords, double *data);
int gft_outm_full(const char *func_name, double *time, int *shape, char **cnames, 
                  const int nt, int *rank, double *coords, double *data); 

int gft_read_full(const char *gf_name, /* IN: name of this file */
                    int level, /* IN: time level */
                    int *shape, /* OUT: shape of data set */
                    char *cnames, /* OUT: names of coordinates */
                    int rank, /* IN: rank of data set */
                    double *time, /* OUT: time value */
                    double *coords, /* OUT: values of coordinates */
                    double *data ); /* OUT: actual data */

int gft_read_brief(const char *gf_name, /* IN: name of this file */
                    int level, /* IN: time level */
                    double *data ); /* OUT: actual data */

int gft_read_shape(const char *gf_name, const int level, int *shape);
int gft_read_rank(const char *gf_name, const int level, int *rank);
int gft_read_name(const char *file_name, const int n, char *name);
int gft_read_bbox(const char *file_name, const int n,double *bbox);

char *gft_verify_file(char *tag, int ltrace);
int gft_is_sdf_file(char *fname);

int gft_out_part_xyz(const char *func_name, double t, double *x, double *y, double *z,int np);
int gft_out_part_xyzf(const char *func_name, double t, double *xyzf, int np, int nf);

int gft_bbh_port(int default_bbh_port);
int gft_dv_port(void);
void gft_dv_standard_init(int *cs,FILE **stream,char *name,char *DVHOST, char *DVPORT);
void gft_dv_standard_clean_up(int cs,FILE *stream);

#ifdef __cplusplus
}
#endif

#endif

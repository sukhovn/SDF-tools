/* sdf_f.h */
/* I/O routines for sdf files */
/* fortran interfaces */
/* Copyright (c) 1997 by Robert Marsa */
/* $Header: /home/cvs/rnpl/src/sdf_f.h,v 1.2 2014/07/14 22:47:39 cvs Exp $ */

void gft_set_single(const char *nm);
void gft_set_multi(void);
                        
int gft_write_id_gf(const char *gfname, int *shape, int rank, double *data);
int gft_write_id_int(const char *pname, int *param, int nparam);
int gft_write_id_float(const char *pname, double *param, int nparam);
int gft_write_id_str(const char *pname, char **param, int nparam);

int gft_read_id_gf(const char *gfname, int *shape, int *rank, double *data);
int gft_read_id_int(const char *pname, int *param, int nparam);
int gft_read_id_float(const char *pname, double *param, int nparam);
int gft_read_id_str(const char *pname, char **param, int nparam);

int gft_out_full(const char *func_name, double time, int *shape, 
                 const char *cnames, int rank, double *coords, double *data);
int gft_out_bbox(const char *func_name, double time, int *shape, int rank,
                 double *coords, double *data);
int gft_out_brief(const char *func_name, double time, int *shape, int rank, 
                  double *data);
int gft_out(const char *func_name, double time,int *shape, int rank, double *data);
int gft_out_set_bbox(double *bbox, int rank);

int gft_outm(const char *func_name, double *time, int *shape, const int nt, 
             int *rank, double *data);
int gft_outm_brief(const char *func_name, double *time, int *shape, const int nt, 
                   int *rank, double *data);
int gft_outm_bbox(const char *func_name, double *time, int *shape, const int nt, 
                  int *rank, double *coords, double *data);
int gft_outm_full(const char *func_name, double *time, int *shape, const char **cnames, 
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
int gft_read_bbox(const char *file_name, const int n,double *bbox);
int gft_read_rank(const char *gf_name, const int level, int *rank);
int gft_read_name(const char *file_name, const int n, char *name);
int gft_read_sdf_ntlevs(const char *file_name);

/* MWC: Particle output interface added March 2005 */
int gft_out_part_xyz(const char *func_name, double t, double *x, double *y, double *z,int np);
int gft_out_part_xyzf(const char *func_name, double t, double *xyzf, int np, int nf);

/* MWC: gft_set_append, gft_unset_append added July 2014 */
int gft_set_append(void);
int gft_unset_append(void);

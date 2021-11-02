/* $Header: /home/cvs/rnpl/src/bbhutil_f.h,v 1.1.1.1 2013/07/09 00:38:27 cvs Exp $ */
/* bbhutil_f.h */
/* Copyright (c) 1995,1996,1997 by Robert Marsa */

void rdvcpy(double *r,double *s, int len);
double rdvprod(double *r, int len);
int rivprod(int *r, int len);
void rivcpy(int *r, int *s, int len);
void rdvramp(double *r, int len, double min, double inc);
void rdvaddc(double *r, double c1, double *s1, double c2, double *s2, int len);
double rdvupdatemean(double *r, double *s1, double *s2, int len);

double l2norm(int N, double *vec);

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

int do_ivec(const int it, const int niter, int *ivec);
void fixup_ivec(const int min, const int max, const int lev, int *ivec);

void fixup_fvec(double *fv);
int size_fvec(double *fv);
double get_fvec(double *fv, const int i);

void gft_close_all();
void gft_close(const char *nm);

#ifdef HAVE_HDF

int gft_read_id_str_p(const char *file_name, const char *param_name,
                        char **param, int nparam);
int gft_read_id_int_p(const char *file_name, const char *param_name,
                        int *param, int nparam);
int gft_read_id_float_p(const char *file_name, const char *param_name,
                        int *param, int nparam);
int gft_read_2idata(const char *file_name, const char *func_name,
                    int *shape, int rank, 
                    double *datanm1, double *datan); 
int gft_read_1idata(const char *file_name, const char *func_name, 
                    int *shape, int rank, double *datan); 
int gft_read_idata(const char *file_name, const char *func_name, 
                    int *shape, int rank, double *data); 

int gft_write_id_str_p(const char *file_name, const char *param_name,
                        char **param, int nparam);
int gft_write_id_int_p(const char *file_name, const char *param_name,
                        int *param, int nparam);
int gft_write_id_float_p(const char *file_name, const char *param_name,
                        double *param, int nparam);
int gft_write_2idata(const char *file_name, const char *func_name, 
                    int *shape, int rank, 
                    double *datanm1, double *datan); 
int gft_write_1idata(const char *file_name, const char *func_name, 
                    int *shape, int rank, double *datan);
int gft_write_idata(const char *file_name, const char *func_name, 
                    int *shape, int rank, double *data);

#endif /* HAVE_HDF */


#include <bbhutil.h>

//Reading routines

//Reads number of dimensions
int gft_read_rank(const char *gf_name, const int level, int *rank);

//Reads dimension sizes
int gft_read_shape(const char *gf_name, const int level, int *shape);

//Reads full sdf file
int gft_read_full(const char *gf_name, int level, int *shape, char *cnames, int rank, double *time, double *coords, double *data);

//Writing routines

//Writes data in the full format
// int gft_out_full(const char *func_name, double time, int *shape, char *cnames, int rank, double *coords, double *data);

//Writes data stored in a bounding box
// int gft_out_bbox(const char *func_name, double time, int *shape, int rank, double *coords, double *data);
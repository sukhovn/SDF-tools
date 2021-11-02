#include "bbhutil.h"

typedef struct iv{
  int len;
  int ivel[3];
  struct iv *next;
} ivel_list;

typedef struct fv{
  int len;
  double fvel[3];
  struct fv *next;
} fvel_list;

typedef struct {
  int type;
  union{
    int inum;
    double num;
    char *str;
    ivel_list *ivec;
    fvel_list *fvec;
  } data;
} value;

typedef struct vl{
  int len;
  value val;
  struct vl *next;
} value_list;

typedef struct {
  char *name;
  int type;
  value val;
  value_list *vl;
} assignment;



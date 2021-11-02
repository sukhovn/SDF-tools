#ifndef _DV_H
#define _DV_H 1
/*========================================================================================*/
/* DVault.h                                                                                   */
/*========================================================================================*/

/* #define DV_PORT 5010 */
/* port 5005 is BBHHOST's default */
#define DEFAULT_DV_PORT 5005
#define DEFAULT_DVR_PORT 5006

extern int DV_PORT;

#define DV_CALC_TAG "__DV_Calc_program__"

extern int stop_dv_service;
extern int refresh_GUI_now;
extern const int no_GUI;

#endif /* _DV_H */

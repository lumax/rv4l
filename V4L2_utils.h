/*
Bastian Ruppert
*/

#ifndef __V4L2_UTILS_H__
#define __V4L2_UTILS_H__
#ifdef __cplusplus
extern "C" {
#endif

int getV4L2QueryCtrl(int fd,int id,struct v4l2_queryctrl * pqueryctrl);
int getV4L2_Value(int fd,int id, int * theValue);
int setV4L2_Value(int fd,int id,int value);

#ifdef __cplusplus
}
#endif
#endif /* __V4L2_UTILS_H__ */

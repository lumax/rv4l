#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "videodev.h"

int getV4L2QueryCtrl(int fd,int id,struct v4l2_queryctrl * pqueryctrl)
{
  memset (pqueryctrl, 0, sizeof (struct v4l2_queryctrl));
  pqueryctrl->id = id;
  if (-1 == ioctl (fd, VIDIOC_QUERYCTRL, pqueryctrl))
    {
      if (errno != EINVAL)
	{
	  perror ("VIDIOC_QUERYCTRL");
	  return -1;
	}
      else
	{
	  printf ("getMaxValue id: %i is not supported\n",id);
	  return -1;
	}
    }
  else if (pqueryctrl->flags & V4L2_CTRL_FLAG_DISABLED)
    {
      printf ("getMaxValue id %i is not supported (disabled)\n",id);
      return -1;
    }
  else
    {
      return 0;
    }
}


int getV4L2_Value(int fd,int id, int * theValue)
{
  struct v4l2_control control;
  memset (&control, 0, sizeof (control));
  control.id = id;

  if(0 == ioctl (fd, VIDIOC_G_CTRL, &control))
    {
      *theValue = control.value;
      return 0;
    }     
  return -1;
}


int setV4L2_Value(int fd,int id,int value)
{
  struct v4l2_control control;

  memset (&control, 0, sizeof (control));
  control.id = id;
  control.value = value;

  if (-1 == ioctl (fd, VIDIOC_S_CTRL, &control))
    {
      perror ("VIDIOC_S_CTRL for V4L2_CID_DO_WHITE_BALANCE failed\n");
      return -1;
    }
  return 0;
} 

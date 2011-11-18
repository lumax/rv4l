/*
 *   
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/videodev.h>

static int enum_frame_intervals(int dev, __u32 pixfmt, __u32 width, __u32 height)
{
	int ret;
	struct v4l2_frmivalenum fival;

	memset(&fival, 0, sizeof(fival));
	fival.index = 0;
	fival.pixel_format = pixfmt;
	fival.width = width;
	fival.height = height;
	printf("\tTime interval between frame: ");
	while ((ret = ioctl(dev, VIDIOC_ENUM_FRAMEINTERVALS, &fival)) == 0) {
		if (fival.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
				printf("%u/%u, ",
						fival.discrete.numerator, fival.discrete.denominator);
		} else if (fival.type == V4L2_FRMIVAL_TYPE_CONTINUOUS) {
				printf("{min { %u/%u } .. max { %u/%u } }, ",
						fival.stepwise.min.numerator, fival.stepwise.min.numerator,
						fival.stepwise.max.denominator, fival.stepwise.max.denominator);
				break;
		} else if (fival.type == V4L2_FRMIVAL_TYPE_STEPWISE) {
				printf("{min { %u/%u } .. max { %u/%u } / "
						"stepsize { %u/%u } }, ",
						fival.stepwise.min.numerator, fival.stepwise.min.denominator,
						fival.stepwise.max.numerator, fival.stepwise.max.denominator,
						fival.stepwise.step.numerator, fival.stepwise.step.denominator);
				break;
		}
		fival.index++;
	}
	printf("\n");
	if (ret != 0 && errno != EINVAL) {
		printf("ERROR enumerating frame intervals: %d\n", errno);
		return errno;
	}

	return 0;
}

static int enum_frame_sizes(int dev, __u32 pixfmt)
{
	int ret;
	struct v4l2_frmsizeenum fsize;

	memset(&fsize, 0, sizeof(fsize));
	fsize.index = 0;
	fsize.pixel_format = pixfmt;
	while ((ret = ioctl(dev, VIDIOC_ENUM_FRAMESIZES, &fsize)) == 0) {
		if (fsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
			printf("{ discrete: width = %u, height = %u }\n",
					fsize.discrete.width, fsize.discrete.height);
			ret = enum_frame_intervals(dev, pixfmt,
					fsize.discrete.width, fsize.discrete.height);
			if (ret != 0)
				printf("  Unable to enumerate frame sizes.\n");
		} else if (fsize.type == V4L2_FRMSIZE_TYPE_CONTINUOUS) {
			printf("{ continuous: min { width = %u, height = %u } .. "
					"max { width = %u, height = %u } }\n",
					fsize.stepwise.min_width, fsize.stepwise.min_height,
					fsize.stepwise.max_width, fsize.stepwise.max_height);
			printf("  Refusing to enumerate frame intervals.\n");
			break;
		} else if (fsize.type == V4L2_FRMSIZE_TYPE_STEPWISE) {
			printf("{ stepwise: min { width = %u, height = %u } .. "
					"max { width = %u, height = %u } / "
					"stepsize { width = %u, height = %u } }\n",
					fsize.stepwise.min_width, fsize.stepwise.min_height,
					fsize.stepwise.max_width, fsize.stepwise.max_height,
					fsize.stepwise.step_width, fsize.stepwise.step_height);
			printf("  Refusing to enumerate frame intervals.\n");
			break;
		}
		fsize.index++;
	}
	if (ret != 0 && errno != EINVAL) {
		printf("ERROR enumerating frame sizes: %d\n", errno);
		return errno;
	}

	return 0;
}

static int enum_frame_formats(int dev)
{
	int ret;
	struct v4l2_fmtdesc fmt;

	memset(&fmt, 0, sizeof(fmt));
	fmt.index = 0;
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	while ((ret = ioctl(dev, VIDIOC_ENUM_FMT, &fmt)) == 0) {
		fmt.index++;
		printf("{ pixelformat = '%c%c%c%c', description = '%s' }\n",
				fmt.pixelformat & 0xFF, (fmt.pixelformat >> 8) & 0xFF,
				(fmt.pixelformat >> 16) & 0xFF, (fmt.pixelformat >> 24) & 0xFF,
				fmt.description);
		ret = enum_frame_sizes(dev, fmt.pixelformat);
		if (ret != 0)
			printf("  Unable to enumerate frame sizes.\n");
	}
	if (errno != EINVAL) {
		printf("ERROR enumerating frame formats: %d\n", errno);
		return errno;
	}

	return 0;
}

#ifdef LMAA2000
//Example 1-9. Changing controls

struct v4l2_queryctrl queryctrl;
struct v4l2_control control;

memset (&queryctrl, 0, sizeof (queryctrl));
queryctrl.id = V4L2_CID_BRIGHTNESS;

if (-1 == ioctl (fd, VIDIOC_QUERYCTRL, &queryctrl)) {
        if (errno != EINVAL) {
                perror ("VIDIOC_QUERYCTRL");
                exit (EXIT_FAILURE);
        } else {
                printf ("V4L2_CID_BRIGHTNESS is not supported\n");
        }
} else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
        printf ("V4L2_CID_BRIGHTNESS is not supported\n");
} else {
        memset (&control, 0, sizeof (control));
        control.id = V4L2_CID_BRIGHTNESS;
        control.value = queryctrl.default_value;

        if (-1 == ioctl (fd, VIDIOC_S_CTRL, &control)) {
                perror ("VIDIOC_S_CTRL");
                exit (EXIT_FAILURE);
        }
}

memset (&control, 0, sizeof (control));
control.id = V4L2_CID_CONTRAST;

if (0 == ioctl (fd, VIDIOC_G_CTRL, &control)) {
        control.value += 1;

        /* The driver may clamp the value or return ERANGE, ignored here */

        if (-1 == ioctl (fd, VIDIOC_S_CTRL, &control)
            && errno != ERANGE) {
                perror ("VIDIOC_S_CTRL");
                exit (EXIT_FAILURE);
        }
/* Ignore if V4L2_CID_CONTRAST is unsupported */
} else if (errno != EINVAL) {
        perror ("VIDIOC_G_CTRL");
        exit (EXIT_FAILURE);
}

control.id = V4L2_CID_AUDIO_MUTE;
control.value = TRUE; /* silence */

// Errors ignored 
ioctl (fd, VIDIOC_S_CTRL, &control);

#endif

static void v4l2_queryctrlToSring(struct v4l2_queryctrl *qc)
{

  printf("name: %s\n",qc->name);

  printf("flags:\n");
  if(qc->flags&V4L2_CTRL_FLAG_DISABLED)
    printf("V4L2_CTRL_FLAG_DISABLED\n");
  if(qc->flags&V4L2_CTRL_FLAG_GRABBED)
   printf("V4L2_CTRL_FLAG_GRABBED\n");    
  if(qc->flags&V4L2_CTRL_FLAG_READ_ONLY)
   printf("V4L2_CTRL_FLAG_READ_ONLY\n");
  if(qc->flags&V4L2_CTRL_FLAG_UPDATE)
   printf("V4L2_CTRL_FLAG_UPDATE\n");
  if(qc->flags&V4L2_CTRL_FLAG_INACTIVE)
   printf("V4L2_CTRL_FLAG_INACTIVE\n");
  if(qc->flags&V4L2_CTRL_FLAG_SLIDER)
   printf("V4L2_CTRL_FLAG_SLIDER\n");

}


static struct v4l2_queryctrl queryctrl;
static struct v4l2_querymenu querymenu;

static int enumerate_menu (int fd)
  {
    printf ("  Menu items:\n");
    
    memset (&querymenu, 0, sizeof (querymenu));
    querymenu.id = queryctrl.id;
    
    for (querymenu.index = queryctrl.minimum;
	 querymenu.index <= queryctrl.maximum;
	 querymenu.index++) {
      if (0 == ioctl (fd, VIDIOC_QUERYMENU, &querymenu)) {
	printf ("  %s\n", querymenu.name);
      } else {
	perror ("VIDIOC_QUERYMENU");
	return -1;
      }
    }
    return 0;
  }

static int EnumeratingControls(int fd)
{  
  printf("--Begin EnumeratingControls\n");
  memset (&queryctrl, 0, sizeof (queryctrl));
  
  for (queryctrl.id = V4L2_CID_BASE;
       queryctrl.id < V4L2_CID_LASTP1;
       queryctrl.id++) {
    if (0 == ioctl (fd, VIDIOC_QUERYCTRL, &queryctrl)) {
      if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
	continue;
      
      printf ("Control %s\n", queryctrl.name);
      //v4l2_queryctrlToSring(&queryctrl);
      
      if (queryctrl.type == V4L2_CTRL_TYPE_MENU)
	enumerate_menu (fd);
    } else {
      if (errno == EINVAL)
	continue;
      
      perror ("VIDIOC_QUERYCTRL");
      return -1;
    }
  }
  
  for (queryctrl.id = V4L2_CID_PRIVATE_BASE;;
       queryctrl.id++) {
    if (0 == ioctl (fd, VIDIOC_QUERYCTRL, &queryctrl)) {
      if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
	continue;
      
      printf ("Control %s\n", queryctrl.name);
      
      if (queryctrl.type == V4L2_CTRL_TYPE_MENU)
	enumerate_menu (fd);
    } else {
      if (errno == EINVAL)
	break;
      
      perror ("VIDIOC_QUERYCTRL");
      return -1;
    }
  }
  printf("--End EnumeratingControls\n");
  return 0;
}

static int ListVideoStandards(int fd)
{
  struct v4l2_input input;
  struct v4l2_standard standard;
  
  memset (&input, 0, sizeof (input));
  
  if (-1 == ioctl (fd, VIDIOC_G_INPUT, &input.index)) {
    perror ("VIDIOC_G_INPUT");
    return -1;
  }
  
  if (-1 == ioctl (fd, VIDIOC_ENUMINPUT, &input)) {
    perror ("VIDIOC_ENUM_INPUT");
    return -1;
  }

  printf ("Current input %s supports:\n", input.name);
  
  memset (&standard, 0, sizeof (standard));
  standard.index = 0;
  
  while (0 == ioctl (fd, VIDIOC_ENUMSTD, &standard)) {
    if (standard.id & input.std)
      printf ("%s\n", standard.name);
    
    standard.index++;
  }
  
/* EINVAL indicates the end of the enumeration, which cannot be
   empty unless this device falls under the USB exception. */
  
  if (errno != EINVAL || standard.index == 0) {
    perror ("VIDIOC_ENUMSTD");
    return -1;
  }
  return 0;
}

static int VideoStandardInfo(int fd)
{
  v4l2_std_id std_id;
struct v4l2_standard standard;

if (-1 == ioctl (fd, VIDIOC_G_STD, &std_id)) {
        /* Note when VIDIOC_ENUMSTD always returns EINVAL this
           is no video device or it falls under the USB exception,
           and VIDIOC_G_STD returning EINVAL is no error. */
  if(errno==EINVAL)
    {
      printf(" it falls under the USB exception, and VIDIOC_G_STD returning EINVAL is no error?\n");
    }
        perror ("VIDIOC_G_STD");
        return -1;
}

memset (&standard, 0, sizeof (standard));
standard.index = 0;

while (0 == ioctl (fd, VIDIOC_ENUMSTD, &standard)) {
        if (standard.id & std_id) {
               printf ("Current video standard: %s\n", standard.name);
               return 0;
        }

        standard.index++;
}

/* EINVAL indicates the end of the enumeration, which cannot be
   empty unless this device falls under the USB exception. */

if (errno == EINVAL || standard.index == 0) {
        perror ("VIDIOC_ENUMSTD");
        return -1;
}
 return 0;
}

static int openDevice(char * devname)
{
  int fd;
  int index;
  struct v4l2_input input;
  printf("--Begin openDevice\n");
   if ((fd = open(devname, O_RDWR)) == -1) {
	perror("ERROR opening V4L interface \n");
	return -1;
    }

if (-1 == ioctl (fd, VIDIOC_G_INPUT, &index)) {
        perror ("VIDIOC_G_INPUT");
        return -1;
}

memset (&input, 0, sizeof (input));
input.index = index;

if (-1 == ioctl (fd, VIDIOC_ENUMINPUT, &input)) {
        perror ("VIDIOC_ENUMINPUT");
        return -1;
}

 printf ("Current input: %s\n", input.name);
 printf ("Current v4l2_std_id: %i / 2 = Camera\n", input.std);
 printf ("Current status: %x\n", input.status);
 printf ("Current type: %x\n", input.type);
 printf ("Current audioset: %x\n", input.audioset);
 printf ("Current tuner: %x\n", input.tuner);
 printf("--End openDevice\n");
 return fd;
}


static int QueryControl(int fd)
{

struct v4l2_queryctrl queryctrl;
struct v4l2_querymenu querymenu;

printf("Funktion QueryControl:\n");

memset (&queryctrl, 0, sizeof (queryctrl));

for (queryctrl.id = V4L2_CID_BASE;
     queryctrl.id < V4L2_CID_LASTP1;
     queryctrl.id++) {
        if (0 == ioctl (fd, VIDIOC_QUERYCTRL, &queryctrl)) {
                if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                        continue;

                printf ("Control %s\n", queryctrl.name);

                if (queryctrl.type == V4L2_CTRL_TYPE_MENU)
                        enumerate_menu (fd);
        } else {
                if (errno == EINVAL)
                        continue;

                perror ("VIDIOC_QUERYCTRL");
                return -1;
        }
}

for (queryctrl.id = V4L2_CID_PRIVATE_BASE;;
     queryctrl.id++) {
        if (0 == ioctl (fd, VIDIOC_QUERYCTRL, &queryctrl)) {
                if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                        continue;

                printf ("Control %s\n", queryctrl.name);

                if (queryctrl.type == V4L2_CTRL_TYPE_MENU)
                        enumerate_menu (fd);
        } else {
                if (errno == EINVAL)
                        break;

                perror ("VIDIOC_QUERYCTRL");
                return -1;
        }
}
 return 0;
}

int setWhiteBalanceTempAuto(int fd,int bWert)
{
  struct v4l2_queryctrl queryctrl;
  struct v4l2_control control;
  memset (&queryctrl, 0, sizeof (queryctrl));
  queryctrl.id = V4L2_CID_AUTO_WHITE_BALANCE;
  
if (-1 == ioctl (fd, VIDIOC_QUERYCTRL, &queryctrl)) {
  if (errno != EINVAL) {
    perror ("VIDIOC_QUERYCTRL");
    return -1;
  } else {
    printf ("V4L2_CID_AUTO_WHITE_BALANCE is not supported\n");
  }
 } else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
  printf ("V4L2_CID_AUTO_WHITE_BALANCE is not supported (disabled)\n");
 } else {
  memset (&control, 0, sizeof (control));
  control.id = V4L2_CID_AUTO_WHITE_BALANCE;
  if(bWert)
    control.value = 1;
  else
    control.value = 0;
  
  if (-1 == ioctl (fd, VIDIOC_S_CTRL, &control)) {
    perror ("VIDIOC_S_CTRL for V4L2_CID_AUTO_WHITE_BALANCE failed\n");
    return -1;
  }
 }
 return 0;
}

int getWhiteBalanceTempAuto(int fd)
{
  struct v4l2_queryctrl queryctrl;
  struct v4l2_control control;
  memset (&control, 0, sizeof (control));
  control.id = V4L2_CID_AUTO_WHITE_BALANCE;
  
  if (0 == ioctl (fd, VIDIOC_G_CTRL, &control))
    {
      return control.value;
    }
  else
    {
      return -1;
    }
 }

int main (int argi , char ** args)
{

  int fd =0;
  
  if(argi==2)
    fd = openDevice("/dev/video1");
  else
    fd =  openDevice("/dev/video0");
  if(fd<0)
    {
      printf("error openDevice");
      return -1;
    }

  printf("getWhiteBalanceTempAuto : %i\n",getWhiteBalanceTempAuto(fd));
  printf("setWhiteBalanceTempAuto : %i\n",setWhiteBalanceTempAuto(fd,0));
  printf("getWhiteBalanceTempAuto : %i\n",getWhiteBalanceTempAuto(fd));
  
  //printf("##VideoStandardInfo(int fd): %i\n",VideoStandardInfo(fd));
  //printf("##ListVideoStandards(int fd): %i\n",ListVideoStandards(fd));
  printf("EnumeratinControls returned %i\n",EnumeratingControls(fd));
  //printf("##enum_frame_formats(int dev): %i\n",enum_frame_formats(fd));

  //enumerate_menu (fd);
  //QueryControl(fd);
  return 0;
}

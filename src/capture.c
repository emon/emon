#include <machine/ioctl_meteor.h>
#include <machine/ioctl_bt848.h>
#include "capture.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <signal.h>

extern int errno;

static unsigned long int Captures_field_masks[3]={
  0L,
  METEOR_GEO_EVEN_ONLY,
  METEOR_GEO_ODD_ONLY};
static unsigned long int Captures_oformat[4]={
  METEOR_GEO_RGB16,
  METEOR_GEO_RGB24,
  METEOR_GEO_RGB16,
  METEOR_GEO_RGB24};
static unsigned int Captures_pixel_size[4]={
  3,
  3,
  2,
  4};
static unsigned int Captures_board_pixel_size[4]={
  2,
  4,
  2,
  4};

CAPTURE *CaptureOpen(const char *device){
  struct meteor_geomet geo;
  unsigned long c;
  unsigned int s;
  CAPTURE *capture=(CAPTURE*)malloc(sizeof(CAPTURE));

  if(device==NULL){
    if((capture->board = open("/dev/bktr0",O_RDONLY)) < 0){
      fprintf(stderr,"open faild: %d\n",errno);
      exit(1);
    }
  } else {
    if((capture->board = open(device,O_RDONLY)) < 0){
      fprintf(stderr,"open faild: %d\n",errno);
      exit(1);
    }
  }
  capture->cap_continue=0;

  geo.frames=1;
  capture->format = CAPTURE_FORMAT_RGB24;
  capture->field_mode = CAPTURE_FRAME_MODE;
  geo.oformat = Captures_oformat[capture->format] | Captures_field_masks[capture->field_mode];
  capture->col = geo.columns=640;
  capture->row = geo.rows=480;
  if (ioctl(capture->board, METEORSETGEO, &geo) < 0) {
    fprintf(stderr,"ioctl faild[%d]:%d\n",__LINE__,errno);
    exit(1);
  }

  c = METEOR_FMT_NTSC;
  if (ioctl(capture->board, METEORSFMT, &c) < 0) {
    fprintf(stderr,"ioctl faild[%d]:%d\n",__LINE__,errno);
    exit(1);
  }
  c = METEOR_INPUT_DEV0;
  if (ioctl(capture->board, METEORSINPUT, &c) < 0) {
    fprintf(stderr,"ioctl faild[%d]:%d\n",__LINE__,errno);
    exit(1);
  }

  s=SIGUSR2;
  if (ioctl(capture->board, METEORSSIGNAL, &s) < 0) {
    fprintf(stderr,"ioctl faild[%d]:%d\n",__LINE__,errno);
    exit(1);
  }
  signal(s,SIG_IGN);

  capture->board_frame_size = geo.columns*geo.rows*4;
  capture->frame_size = geo.columns*geo.rows*3;

  capture->board_buffer = (unsigned char *)mmap((caddr_t)0,capture->board_frame_size,
					   PROT_READ,MAP_SHARED,
					   capture->board,(off_t)0);
  return capture;
}

void CaptureClose(CAPTURE *capture){
  int c=METEOR_CAP_STOP_CONT;
  if(capture->cap_continue!=0){
    if (ioctl(capture->board, METEORCAPTUR, &c) < 0) {
      fprintf(stderr,"ioctl faild[%d]:%d\n",__LINE__,errno);
      exit(1);
    }
    capture->cap_continue=0;
  }
  munmap(capture->board_buffer,capture->board_frame_size);
  close(capture->board);
  free(capture);
}

void CaptureSetInput(CAPTURE *capture,int input){
  unsigned long i=(unsigned long int)input;

  if (ioctl(capture->board, METEORSINPUT, &i) < 0) {
    printf("ioctl failed: %d\n", errno);
    exit(1);
  }
}
int CaptureGetInput(CAPTURE *capture){
  unsigned long i;
  if (ioctl(capture->board, METEORGINPUT, &i) < 0) {
    printf("ioctl failed: %d\n", errno);
    exit(1);
  }
}


void CaptureSetSize(CAPTURE *capture,int x,int y){
  struct meteor_geomet geo;
  capture->col = geo.columns = x;
  capture->row = geo.rows = y;
  geo.frames = 1;
  geo.oformat = Captures_oformat[capture->format] | Captures_field_masks[capture->field_mode];
  if (ioctl(capture->board, METEORSETGEO, &geo) < 0) {
    fprintf(stderr,"ioctl faild[%d]:%d\n",__LINE__,errno);
    exit(1);
  }
  munmap(capture->board_buffer,capture->board_frame_size);
  capture->board_frame_size = x*y*Captures_board_pixel_size[capture->format];
  capture->frame_size = x*y*Captures_pixel_size[capture->format];

  capture->board_buffer = (unsigned char *)mmap((caddr_t)0,capture->board_frame_size,
					   PROT_READ,MAP_SHARED,
					   capture->board,(off_t)0);
}

void CaptureGetSize(CAPTURE *capture,int *x, int *y){
  *x = capture->col;
  *y = capture->row;
}

void CaptureSetFormat(CAPTURE *capture,int form){
  struct meteor_geomet geo;
  geo.columns = capture->col;
  geo.rows = capture->row;
  geo.frames = 1;
  capture->format = form;
  geo.oformat = Captures_oformat[capture->format] | Captures_field_masks[capture->field_mode];
  if (ioctl(capture->board, METEORSETGEO, &geo) < 0) {
    fprintf(stderr,"ioctl faild[%d]:%d\n",__LINE__,errno);
    exit(1);
  }
  munmap(capture->board_buffer,capture->board_frame_size);
  capture->board_frame_size = capture->col*capture->row*Captures_board_pixel_size[capture->format];
  capture->frame_size = capture->col*capture->row*Captures_pixel_size[capture->format];

  capture->board_buffer = (unsigned char *)mmap((caddr_t)0,capture->board_frame_size,
					   PROT_READ,MAP_SHARED,
					   capture->board,(off_t)0);
}

int CaptureGetFormat(CAPTURE *capture){
  return capture->format;
}

void CaptureSetFieldMode(CAPTURE *capture,int mode){
  struct meteor_geomet geo;
  capture->field_mode = mode;

  geo.columns = capture->col;
  geo.rows = capture->row;
  geo.frames = 1;
  geo.oformat = Captures_oformat[capture->format] | Captures_field_masks[capture->field_mode];

  if (ioctl(capture->board, METEORSETGEO, &geo) < 0) {
    fprintf(stderr,"ioctl faild[%d]:%d\n",__LINE__,errno);
    exit(1);
  }
}
int CaptureGetFieldMode(CAPTURE *capture){
  return capture->field_mode;
}

int CaptureGetFrameSize(CAPTURE *capture){
  return capture->frame_size;
}

void CaptureSetFps(CAPTURE *capture,int fps){
  unsigned short int f=(unsigned short int)(fps);
  if (ioctl(capture->board, METEORSFPS, &f) < 0) {
    fprintf(stderr,"ioctl faild[%d]:%d\n",__LINE__,errno);
    exit(1);
  }
}

int CaptureGetFps(CAPTURE *capture){
  unsigned short f;
  if (ioctl(capture->board, METEORGFPS, &f) < 0) {
    fprintf(stderr,"ioctl faild[%d]:%d\n",__LINE__,errno);
    exit(1);
  }
  return (int)(f);
}

void CaptureSetHue(CAPTURE *capture,int cc){
  signed short c=(signed char)(cc);
  if (ioctl(capture->board, METEORSHUE, &c) < 0) {
    fprintf(stderr,"ioctl faild[%d]:%d\n",__LINE__,errno);
    exit(1);
  }
}

int CaptureGetHue(CAPTURE *capture){
  signed short c;
  if (ioctl(capture->board, METEORGHUE, &c) < 0) {
    fprintf(stderr,"ioctl faild[%d]:%d\n",__LINE__,errno);
    exit(1);
  }
  return (int)(c);
}

void CaptureSetGain(CAPTURE *capture,int cc){
  unsigned char c=(signed char)(cc);
  if (ioctl(capture->board, METEORSCHCV, &c) < 0) {
    fprintf(stderr,"ioctl faild[%d]:%d\n",__LINE__,errno);
    exit(1);
  }
}

int CaptureGetGain(CAPTURE *capture){
  unsigned char c;
  if (ioctl(capture->board, METEORGCHCV, &c) < 0) {
    fprintf(stderr,"ioctl faild[%d]:%d\n",__LINE__,errno);
    exit(1);
  }
  return (int)(c);
}

void CaptureSetBrightness(CAPTURE *capture,int cc){
  unsigned char c=(unsigned char)(cc);
  if (ioctl(capture->board, METEORSBRIG, &c) < 0) {
    fprintf(stderr,"ioctl faild[%d]:%d\n",__LINE__,errno);
    exit(1);
  }
}

int CaptureGetBrightness(CAPTURE *capture){
  unsigned short c;
  if (ioctl(capture->board, METEORGHUE, &c) < 0) {
    fprintf(stderr,"ioctl faild[%d]:%d\n",__LINE__,errno);
    exit(1);
  }
  return (int)(c);
}

void CaptureSetSaturation(CAPTURE *capture,int cc){
  unsigned char c=(unsigned char)(cc);
  if (ioctl(capture->board, METEORSCSAT, &c) < 0) {
    fprintf(stderr,"ioctl faild[%d]:%d\n",__LINE__,errno);
    exit(1);
  }
}

int CaptureGetSaturation(CAPTURE *capture){
  unsigned short c;
  if (ioctl(capture->board, METEORGCSAT, &c) < 0) {
    fprintf(stderr,"ioctl faild[%d]:%d\n",__LINE__,errno);
    exit(1);
  }
  return (int)(c);
}

void CaptureSetContrast(CAPTURE *capture,int cc){
  unsigned char c=(unsigned char)(cc);
  if (ioctl(capture->board, METEORSCONT, &c) < 0) {
    fprintf(stderr,"ioctl faild[%d]:%d\n",__LINE__,errno);
    exit(1);
  }
}

int CaptureGetContrast(CAPTURE *capture){
  unsigned short c;
  if (ioctl(capture->board, METEORGCONT, &c) < 0) {
    fprintf(stderr,"ioctl faild[%d]:%d\n",__LINE__,errno);
    exit(1);
  }
  return (int)(c);
}

static void Capturecapmemcpy(unsigned char *buffer,
			unsigned char *board_buffer,
			int size, int format){
  unsigned char *board_p=board_buffer,*mem_p=buffer;
  int i;
  int fsize;
  switch(format){
  case CAPTURE_FORMAT_RGB16:
    for(i=0;i<size;i++){
      *(mem_p++) = ((*(board_p+1)) & 0x7c) << 1;
      *(mem_p++) = (((*(board_p+1)) & 0x03) << 6) | (((*board_p) & 0xe0) >> 2);
      *(mem_p++) = ((*board_p) & 0x1f) << 3;
      board_p+=2;
    }
    break;
  case CAPTURE_FORMAT_RGB24:
    for(i=0;i<size;i++){
      *(mem_p++) = *(board_p+2);
      *(mem_p++) = *(board_p+1);
      *(mem_p++) = *(board_p);
      board_p+=4;
    }
    break;
  case CAPTURE_FORMAT_RGB16P:
  case CAPTURE_FORMAT_RGB24P:
    fsize = size*Captures_board_pixel_size[format];
    memcpy(buffer,board_buffer,fsize);
    break;
  }
}

void CaptureGetSingleImage(CAPTURE *capture,unsigned char *buffer){
  int c=METEOR_CAP_SINGLE;
  if (ioctl(capture->board, METEORCAPTUR, &c) < 0) {
    fprintf(stderr,"ioctl faild[%d]:%d\n",__LINE__,errno);
    exit(1);
  }
  Capturecapmemcpy(buffer,capture->board_buffer,capture->col*capture->row,capture->format);
}
void CaptureStart(CAPTURE *capture){
  int c=METEOR_CAP_CONTINOUS;
  if(capture->cap_continue==0){
    if (ioctl(capture->board, METEORCAPTUR, &c) < 0) {
      fprintf(stderr,"ioctl faild[%d]:%d\n",__LINE__,errno);
      exit(1);
    }
    capture->cap_continue=1;
  }
}
void CaptureEnd(CAPTURE *capture){
  int c=METEOR_CAP_STOP_CONT;
  if(capture->cap_continue!=0){
    if (ioctl(capture->board, METEORCAPTUR, &c) < 0) {
      fprintf(stderr,"ioctl faild[%d]:%d\n",__LINE__,errno);
      exit(1);
    }
    capture->cap_continue=0;
  }
}
void CaptureGetImageAsync(CAPTURE *capture,unsigned char *buffer,int frame_num){
  int i;
  for(i=0;i<frame_num;i++)
    Capturecapmemcpy(buffer+capture->frame_size*i,
	      capture->board_buffer,capture->col*capture->row,capture->format);
}


static int Captures_frame_num;
static int Captures_frame_cc;
static int Captures_capture_flag=0;
static int Captures_frame_size;
static int Captures_size;
static int Captures_format;
static unsigned char *Captures_board_buffer;
static unsigned char *Captures_buffer;


static void Capturesignal_capture(int sig){
  if(Captures_frame_cc<Captures_frame_num){
    Capturecapmemcpy(Captures_buffer+Captures_frame_size*Captures_frame_cc,
	      Captures_board_buffer,Captures_size,Captures_format);
    Captures_frame_cc++;
  } else {
    signal(sig,SIG_IGN);
    Captures_capture_flag = 0;
  }
}

void CaptureGetImageNoWait(CAPTURE *capture,unsigned char *buffer,int frame_num){
  Captures_frame_num=frame_num;
  Captures_frame_cc=0;
  Captures_frame_size = capture->frame_size;
  Captures_size = capture->col*capture->row;
  Captures_format = capture->format;
  Captures_board_buffer = capture->board_buffer;
  Captures_buffer = buffer;
  Captures_capture_flag = 1;
  signal(SIGUSR2,&Capturesignal_capture);
}
int CaptureTest(CAPTURE *capture){
  return Captures_capture_flag;
}
void CaptureWait(CAPTURE *capture){
  while(Captures_capture_flag!=0) usleep(1);
}
void CaptureGetImageSync(CAPTURE *capture,unsigned char *buffer,int frame_num){
  CaptureWait(capture);
  CaptureGetImageNoWait(capture,buffer,frame_num);
  CaptureWait(capture);
}

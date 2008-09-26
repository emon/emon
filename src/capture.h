/* -*- Mode: C++ -*- */
#ifndef ___CAPTURE_METEOR_BT848_H___
#define ___CAPTURE_METEOR_BT848_H___
/* ���ߡ��η���������Τϥ饤�֥��������� */
typedef struct {
  int board;
  unsigned char *board_buffer;
  int board_frame_size;
  int frame_size;
  int col,row;
  int format;
  int field_mode;
  int cap_continue;
} CAPTURE;
typedef CAPTURE BT848;
typedef CAPTURE METEOR;

/* ����ץ���ܡ��ɤν������device �ˤϥǥХ����ե�������Ϥ���
   NULL �ΤȤ��� "/dev/bktr0" */
CAPTURE *CaptureOpen(const char *device);
/* ����ץ���ܡ��ɤν�λ����λ����ɬ���Ƥ� */
void CaptureClose(CAPTURE *);

/* ����ü�Ҥ��ڤ��Ѥ� */
#define CAPTURE_INPUT_DEV0         0x01000 /* camera input 0 -- default */
#define CAPTURE_INPUT_DEV1         0x02000 /* camera input 1 */
#define CAPTURE_INPUT_DEV2         0x04000 /* camera input 2 */
#define CAPTURE_INPUT_DEV3         0x08000 /* camera input 2 */
#define CAPTURE_INPUT_DEV4         0x0a000
#define CAPTURE_INPUT_DEV5         0x06000

#ifndef BT848_INPUT_DEV0
#define BT848_INPUT_DEV0              CAPTURE_INPUT_DEV0
#endif
#ifndef BT848_INPUT_DEV1
#define BT848_INPUT_DEV1              CAPTURE_INPUT_DEV1
#endif
#ifndef BT848_INPUT_DEV2
#define BT848_INPUT_DEV2              CAPTURE_INPUT_DEV2
#endif
#ifndef BT848_INPUT_DEV_RCA
#define BT848_INPUT_DEV_RCA           CAPTURE_INPUT_DEV0 /* ����ݥ��å� */
#endif
#ifndef BT848_INPUT_DEV_COMPOSITE
#define BT848_INPUT_DEV_COMPOSITE     CAPTURE_INPUT_DEV0 /* ����ݥ��å� */
#endif
#ifndef BT848_INPUT_DEV_SVIDEO
#define BT848_INPUT_DEV_SVIDEO        CAPTURE_INPUT_DEV5 /* S */
#endif
#ifndef BT848_INPUT_DEV_SEPARATE
#define BT848_INPUT_DEV_SEPARATE      CAPTURE_INPUT_DEV5 
#endif

#ifndef METEOR_INPUT_DEV0
#define METEOR_INPUT_DEV0          CAPTURE_INPUT_DEV0
#endif
#ifndef METEOR_INPUT_DEV1
#define METEOR_INPUT_DEV1          CAPTURE_INPUT_DEV1
#endif
#ifndef METEOR_INPUT_DEV2
#define METEOR_INPUT_DEV2          CAPTURE_INPUT_DEV2
#endif
#ifndef METEOR_INPUT_DEV3
#define METEOR_INPUT_DEV3          CAPTURE_INPUT_DEV3
#endif
#ifndef METEOR_INPUT_DEV_RCA
#define METEOR_INPUT_DEV_RCA       CAPTURE_INPUT_DEV0
#endif
#ifndef METEOR_INPUT_DEV_COMPOSITE
#define METEOR_INPUT_DEV_COMPOSITE CAPTURE_INPUT_DEV0
#endif
#ifndef METEOR_INPUT_DEV_RGB
#define METEOR_INPUT_DEV_RGB       CAPTURE_INPUT_DEV4
#endif
#ifndef METEOR_INPUT_DEV_SVIDEO
#define METEOR_INPUT_DEV_SVIDEO    CAPTURE_INPUT_DEV5
#endif
#ifndef METEOR_INPUT_DEV_SEPARATE
#define METEOR_INPUT_DEV_SEPARATE  CAPTURE_INPUT_DEV5
#endif


void CaptureSetInput(CAPTURE *,int);
int CaptureGetInput(CAPTURE *);

/* ���ϥ����� */
void CaptureSetSize(CAPTURE *,int,int);
void CaptureGetSize(CAPTURE *,int *,int *);

/* �����ե����ޥ�
   �Ǹ�� P ���դ��ե����ޥȤ� BT848 ��������ե����ޥȤ��ΤޤޤʤΤǡ�
   ¿����®��ž���Ǥ��ޤ���*/
                                 /* 0:zero,x:unknown,r:red,g:green,b:blue */
#define CAPTURE_FORMAT_RGB16        0 /* rrrrr000 ggggg000 bbbbb000 */
#define CAPTURE_FORMAT_RGB24        1 /* rrrrrrrr gggggggg bbbbbbbb */
#define CAPTURE_FORMAT_RGB16P       2 /* gggbbbbb xrrrrrgg */
#define CAPTURE_FORMAT_RGB24P       3 /* bbbbbbbb gggggggg rrrrrrrr 00000000 */

#define BT848_FORMAT_RGB16             CAPTURE_FORMAT_RGB16
#define BT848_FORMAT_RGB24             CAPTURE_FORMAT_RGB24
#define BT848_FORMAT_RGB16P            CAPTURE_FORMAT_RGB16P
#define BT848_FORMAT_RGB24P            CAPTURE_FORMAT_RGB24

#define METEOR_FORMAT_RGB16             CAPTURE_FORMAT_RGB16
#define METEOR_FORMAT_RGB24             CAPTURE_FORMAT_RGB24
#define METEOR_FORMAT_RGB16P            CAPTURE_FORMAT_RGB16P
#define METEOR_FORMAT_RGB24P            CAPTURE_FORMAT_RGB24


void CaptureSetFormat(CAPTURE *,int);
int CaptureGetFormat(CAPTURE *);

/* �����ߥե�����ɤλ��� */
/* EVEN SIZE �ޤ��� ODD SIDE �μ����ߤϲ����������� 320x240 �ʲ��Τ�
   ���Τߵ�ǽ���ޤ���*/
#define CAPTURE_FRAME_MODE      0      
#define CAPTURE_EVEN_SIDE_ONLY  1
#define CAPTURE_ODD_SIDE_ONLY   2
#define BT848_FRAME_MODE      0      
#define BT848_EVEN_SIDE_ONLY  1
#define BT848_ODD_SIDE_ONLY   2
#define METEOR_FRAME_MODE      0      
#define METEOR_EVEN_SIDE_ONLY  1
#define METEOR_ODD_SIDE_ONLY   2
void CaptureSetFieldMode(CAPTURE *,int);
int CaptureGetFieldMode(CAPTURE *);

/* ���� 1�ե졼��Υ����� */
int CaptureGetFrameSize(CAPTURE *);

/* ����ץ���졼�ȡ�Ʊ������ץ���ΤȤ��Τ߰�̣����� */
void CaptureSetFps(CAPTURE *,int);
int CaptureGetFps(CAPTURE *);

void CaptureSetHue(CAPTURE *,int);
int CaptureGetHue(CAPTURE *);

void CaptureSetGain(CAPTURE *,int);
int CaptureGetGain(CAPTURE *);

void CaptureSetBrightness(CAPTURE *,int);
int CaptureGetBrightness(CAPTURE *);

void CaptureSetSaturation(CAPTURE *,int);
int CaptureGetSaturation(CAPTURE *);

void CaptureSetContrast(CAPTURE *,int);
int CaptureGetContrast(CAPTURE *);


/* ------------------- �Ż߲襭��ץ��� ---------------------------- */
/* �Ż߲���������� */
void CaptureGetSingleImage(CAPTURE *,unsigned char *);

/* ------------------- ư�襭��ץ��� ---------------------------- */
/*  ����ץ��㳫�� */
void CaptureStart(CAPTURE *);
/*  ����ץ��㽪λ */
void CaptureEnd(CAPTURE *);
/*  ��Ʊ���⡼�ɤǲ���������� */
void CaptureGetImageAsync(CAPTURE *,unsigned char *,int);
/*  Ʊ���⡼�ɤǲ���������� */
void CaptureGetImageSync(CAPTURE *,unsigned char *,int);
#define CaputureGetImage(gv,buf,num) CaptureGetImageSync(gv,buf,num)
/*  Ʊ���⡼�ɤǼ����ߡ���λ���Ԥ��ʤ� */
void CaptureGetImageNoWait(CAPTURE *,unsigned char *,int);
/*  �����߽�λ�ޤ��Ԥ� */
void CaptureWait(CAPTURE *);
/*  �������椬��λ���Ƥ��뤫�ɤ����ƥ��Ȥ��� */
int CaptureTest(CAPTURE *);
#endif

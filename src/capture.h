/* -*- Mode: C++ -*- */
#ifndef ___CAPTURE_METEOR_BT848_H___
#define ___CAPTURE_METEOR_BT848_H___
/* ダミーの型定義，実体はライブラリの中で定義 */
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

/* キャプチャボードの初期化，device にはデバイスファイルを渡す．
   NULL のときは "/dev/bktr0" */
CAPTURE *CaptureOpen(const char *device);
/* キャプチャボードの終了，終了時に必ず呼ぶ */
void CaptureClose(CAPTURE *);

/* 入力端子の切り変え */
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
#define BT848_INPUT_DEV_RCA           CAPTURE_INPUT_DEV0 /* コンポジット */
#endif
#ifndef BT848_INPUT_DEV_COMPOSITE
#define BT848_INPUT_DEV_COMPOSITE     CAPTURE_INPUT_DEV0 /* コンポジット */
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

/* 入力サイズ */
void CaptureSetSize(CAPTURE *,int,int);
void CaptureGetSize(CAPTURE *,int *,int *);

/* 画像フォーマト
   最後に P が付くフォーマトは BT848 が取り込むフォーマトそのままなので，
   多少高速に転送できます．*/
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

/* 取り込みフィールドの指定 */
/* EVEN SIZE または ODD SIDE の取り込みは画像サイズが 320x240 以下のと
   きのみ機能します．*/
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

/* 画像 1フレームのサイズ */
int CaptureGetFrameSize(CAPTURE *);

/* キャプチャレート，同期キャプチャのときのみ意味を持つ */
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


/* ------------------- 静止画キャプチャ ---------------------------- */
/* 静止画を一枚取り込む */
void CaptureGetSingleImage(CAPTURE *,unsigned char *);

/* ------------------- 動画キャプチャ ---------------------------- */
/*  キャプチャ開始 */
void CaptureStart(CAPTURE *);
/*  キャプチャ終了 */
void CaptureEnd(CAPTURE *);
/*  非同期モードで画像を取り込む */
void CaptureGetImageAsync(CAPTURE *,unsigned char *,int);
/*  同期モードで画像を取り込む */
void CaptureGetImageSync(CAPTURE *,unsigned char *,int);
#define CaputureGetImage(gv,buf,num) CaptureGetImageSync(gv,buf,num)
/*  同期モードで取り込み，終了を待たない */
void CaptureGetImageNoWait(CAPTURE *,unsigned char *,int);
/*  取り込み終了まで待つ */
void CaptureWait(CAPTURE *);
/*  取り込み中が終了しているかどうかテストする */
int CaptureTest(CAPTURE *);
#endif

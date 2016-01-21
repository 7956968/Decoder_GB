/******************************************************************************

  Copyright (C), 
  
	******************************************************************************
	File Name     : disp_api.h
	Version       : Initial Draft
	Author        : 
	Created       : 2010/06/09
	Description   : preview module header file
	History       :
	1.Date        : 
    Author        : 
    Modification  : Created file
	
******************************************************************************/

#ifndef __DISP_API_H__
#define __DISP_API_H__

#include "global_api.h"
#include "global_def.h"
#include "global_err.h"
#include "global_msg.h"
#include "global_str.h"
#include "prv_err.h"
#include "prv_comm.h"
#if defined(SN9234H1)
#include "mkp_vd.h"
#else
#include "hi_comm_vpss.h"	
#endif
#include "hi_tde_type.h"
#include "hi_common.h"

#include "hi_comm_sys.h"
#include "hi_comm_vb.h"
#include "hi_comm_ai.h"
#include "hi_comm_ao.h"
#include "hi_comm_aio.h"
#include "hi_comm_vi.h"
#include "hi_comm_vo.h"
#include "hi_comm_vdec.h"
#include "hi_comm_adec.h"
#include "hi_comm_video.h"
#include "hi_type.h"

#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "mpi_ao.h"
#include "mpi_vdec.h"
#include "mpi_adec.h"
#include "mpi_vb.h"


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

typedef enum PreviewMode_enum PRV_PREVIEW_MODE_E;
/************************ MACROS HEAR *************************/

#define STATIC static		//���Ʊ��ļ��ں����Ƿ������ⲿ����

#if (CHANNEL_NUM==16)
#define PRV_VO_MAX_MOD SixteenScene//VO_MAX_DEV_NUM				//����������ģʽ
#elif (CHANNEL_NUM==8)
#define PRV_VO_MAX_MOD NineScene//VO_MAX_DEV_NUM				//����������ģʽ
#elif (CHANNEL_NUM==4)
#define PRV_VO_MAX_MOD FourScene//VO_MAX_DEV_NUM				//����������ģʽ
#endif


#define PRV_VO_CHN_NUM CHANNEL_NUM//16 //VIU_MAX_CHN_NUM				//ÿ������豸��ͨ����
#define PRV_VI_CHN_NUM 4 //VIU_MAX_CHN_NUM_PER_DEV		//ÿ�������豸��ͨ����
#define PRV_VO_MAX_DEV 		PRV_VO_DEV_NUM  //VO_MAX_PHY_DEV				/* max physical device number(HD,AD,SD) */

#define PRV_DFLT_CHN_PRIORITY 1						//Ԥ��ͨ�����õ�Ĭ�����ȼ�

#define PRV_PREVIEW_LAYOUT_DIV 12					//Ԥ�����浥λ�ָ���


#define PRV_656_DEV		2							//�ɼ������豸2
#define PRV_656_DEV_1		3							//�ɼ������豸3

#define PRV_HD_DEV				0							//���������豸ID��
#define PRV_IMAGE_SIZE_W		720
#define PRV_IMAGE_SIZE_H_P		576
#define PRV_IMAGE_SIZE_H_N		480

#define OSD_ALARM_LAYER	0		//��Ƶ��ʧͼ���
#define OSD_REC_LAYER	1		//��ʱ¼��ͼ���
#define OSD_TIME_LAYER	2		//ʱ��ͼ���
#define OSD_NAME_LAYER	3		//ͨ������ͼ���
#define OSD_CLICKVISUAL_LAYER	4

#define DISP_PIP_X		250		//��ʾ����������С�������ʼ����x�����1024*768
#define DISP_PIP_Y		204		//��ʾ����������С�������ʼ����y�����1024*768
#define DISP_PIP_W		365		//��ʾ����������С�������ʼ����w�����1024*768
#define DISP_PIP_H		275		//��ʾ����������С�������ʼ����h�����1024*768

#define PRV_CVBS_EDGE_CUT_W 30
#define PRV_CVBS_EDGE_CUT_H 16


#define DISP_DOUBLE_DISP	1	//˫����ʾ
#define DISP_NOT_DOUBLE_DISP	0	//������ʾ

#define OSD_CLEAR_LAYER	25
#if defined(SN9234H1)
#define SPOT_VO_DEV			SD
#else
#define SPOT_VO_DEV			DSD0
#endif
#define SPOT_VO_CHAN		0
#define SPOT_PCI_CHAN		9

#define PRV_CHAN_NUM 		(PRV_VO_CHN_NUM/PRV_CHIP_NUM)		//Ԥ��ģ��ͨ������
#define PRV_CTRL_VOCHN	PRV_VO_CHN_NUM + 1

#if defined(SN9234H1)
typedef enum hiVOU_DEV_E
{
	HD = 0,
	DHD0 = 0,	
	AD = 1,
	SD = 2,
	VOU_DEV_BUTT
}VOU_DEV_E;
#elif defined(Hi3535)
typedef enum hiVOU_DEV_E
{
	HD = 0,
	DHD0 = 0,
	DHD1 = 1,
	DSD0 = 2
//	VOU_DEV_BUTT
}VOU_DEV_E;
typedef enum hiVOU_LAY_E
{
	
	VHD0 = 0,
	VHD1 = 1,
	PIP  = 2,
	VSD0 = 3
//	VOU_DEV_BUTT
}VOU_LAY_E;
#else
typedef enum hiVOU_DEV_E
{	
	HD = 0,
	DHD0 = 0,
	DHD1 = 1,
	DSD0 = 2,
	DSD1 = 3,
	DSD2 = 4,
	DSD3 = 5,
	DSD4 = 6,
	DSD5 = 7
//	VOU_DEV_BUTT
}VOU_DEV_E;
#endif

#if defined(Hi3535)
typedef enum hiHIFB_GRAPHIC_LAYER_E
{
	G4 = 0,
	G1 = 1,
	G2 = 2,
	G3 = 3,
	G0 = 4,
	HC = 4,
	HIFB_GRAPHIC_LAYER_BUTT
}HIFB_GRAPHIC_LAYER_E;
#else
typedef enum hiHIFB_GRAPHIC_LAYER_E
{
	G0 = 0,
	G1 = 1,
	G2 = 2,
	G3 = 3,
	G4 = 4,
	HC = 4,
	HIFB_GRAPHIC_LAYER_BUTT
}HIFB_GRAPHIC_LAYER_E;
#endif

typedef enum CtrlFlag_enum
{
	PRV_CTRL_REGION_SEL = 0,	//����ѡ�����
	PRV_CTRL_ZOOM_IN,			//���ӷŴ����
	PRV_CTRL_PTZ,				//��̨����
	PRV_CTRL_PB,				//�طſ���
	PRV_CTRL_BUTT
}PRV_CTRL_FLAG_E;	//����״̬��־ö�����Ͷ���
typedef enum TimeOut_Type_enum
{
	PRV_INIT = 0,		//��ʼ��
	PRV_LAY_OUT,			//�����л�
}PRV_TIMEROUT_TYPE_E;	//Ԥ��״̬ö�����Ͷ���

typedef struct __PRV_STATE_INFO_S__
{
	HI_BOOL					bIsInit;					//Ԥ����ʼ��״̬
	HI_BOOL					bslave_IsInit;					//Ԥ����Ƭ��ʼ��״̬
	HI_U8					bIsReply;	//Ԥ����Ƭ�ظ�״̬��
	HI_U8					bIsNpfinish;	//Ԥ���Ƿ�NP�л����
	HI_BOOL					bIsOsd_Init;				//OSD�Ƿ��Ѿ���ʼ��
	HI_BOOL					bIsRe_Init;				//OSD�Ƿ����³�ʼ��
	HI_BOOL					bIsVam_rsp;				//�Ƿ�ظ�VAM��ʼ�������Ϣ
	HI_U8					TimeoutCnt;			//��ʱ����
	HI_U8					bIsSlaveConfig;			//��Ƭ�Ƿ�������״̬
	HI_U8					bIsTimerState;			//�Ƿ��ڶ�ʱ״̬
	HI_U8					TimerType;			//��ʱ����
	HI_S32					f_timer_handle;			//��ʱ�����
	HI_U8					g_slave_OK;			//��Ƭ���ص�״̬
	HI_U8					g_zoom_first_in;			//���ӷŴ���ν����־
	SN_MSG 					*Prv_msg_Cur;
}PRV_STATE_INFO_S,*PPRV_STATE_INFO_S;		//Ԥ��ģ��״̬���ṹ��
typedef struct __PRV_VO_SLAVE_STAT_S__
{
	PRV_PREVIEW_MODE_E		enPreviewMode;					//Ԥ��ģʽ
	HI_S32					s32PreviewIndex;				//�໭��Ԥ����ʼλ��
	HI_S32					s32SingleIndex;					//������Ԥ����ʼλ��
	HI_BOOL					bIsSingle;						//�Ƿ��ǵ�����
	HI_U8					enVideoNorm;						//N\P��ʽ
	PRM_AREA_HIDE 			cover_info[PRV_CHAN_NUM];		//��Ƭ�ڸ���Ϣ
	HI_U8 					slave_OSD_off_flag[PRV_CHAN_NUM];//��ƬOSD״̬��־λ
	Preview_Point  			slave_OSD_Rect[PRV_CHAN_NUM][REGION_NUM];		//��ƬOSDλ��
	HI_U32 					f_rec_srceen_h[REC_OSD_GROUP][PRV_CHAN_NUM];
	HI_U32				 	f_rec_srceen_w[REC_OSD_GROUP][PRV_CHAN_NUM];
	
	HI_U32 					slave_BmpData_name_w[REC_OSD_GROUP][PRV_CHAN_NUM];			//��Ƭͨ������ͼƬ��
	HI_U32 					slave_BmpData_name_h[REC_OSD_GROUP][PRV_CHAN_NUM];			//��Ƭͨ������ͼƬ��
	HI_U32 					slave_BmpData_name_size[REC_OSD_GROUP][PRV_CHAN_NUM];			//��Ƭͨ������ͼƬ���ݴ�С
}PRV_VO_SLAVE_STAT_S,*PPRV_VO_SLAVE_STAT_S;		//��Ƭ��Ҫ��Ϣ�ṹ�嶨��

typedef struct __PRV_VO_DEV_STAT_S__
{
	VO_PUB_ATTR_S			stVoPubAttr;					//VO�豸��������
	VO_VIDEO_LAYER_ATTR_S	stVideoLayerAttr;				//VO�豸��Ƶ������
	PRV_PREVIEW_STAT_E		enPreviewStat;					//Ԥ��״̬
	PRV_PREVIEW_MODE_E		enPreviewMode;					//Ԥ��ģʽ
	PRV_CTRL_FLAG_E			enCtrlFlag;						//����״̬��־
	VO_CHN					as32ChnOrder[PRV_PREVIEW_MODE_NUM][SEVENINDEX];	//Ԥ��ͨ��˳�򣬰���ͨ���Ƿ����أ���Чͨ��ֵ��ʾ���ظ�λ�õ�ͨ����
	VO_CHN					as32ChnpollOrder[PRV_PREVIEW_MODE_NUM][SEVENINDEX];	//Ԥ��ͨ��˳�򣬰���ͨ���Ƿ����أ���Чͨ��ֵ��ʾ���ظ�λ�õ�ͨ����
	AO_CHN					AudioChn[7];//��������ʹ�ã�4��Ԥ��ģʽ�¶�Ӧ����Ƶͨ��
	HI_S32					s32PreviewIndex;				//�໭��Ԥ����ʼλ��
	HI_S32					s32SingleIndex;					//������Ԥ����ʼλ��
	HI_S32					s32AlarmChn;					//��������ͨ��
	HI_S32					s32CtrlChn;						//����״̬ͨ��
	HI_BOOL					bIsAlarm;						//�Ƿ���ͨ������
	HI_BOOL					bIsSingle;						//�Ƿ��ǵ�����(�������У��ô˱��������ǵ����滹��˫�����뵥����)
	HI_BOOL					s32DoubleIndex;					//�Ƿ������˫��
	RECT_S					Pip_rect;						//���л���λ��
}PRV_VO_DEV_STAT_S,*PPRV_VO_DEV_STAT_S;		//VO�豸���Ժ�״̬��Ϣ�ṹ�嶨��

#if 0
#define NOVIDEO_FILE "/res/pic_704_576_p420_novideo.yuv"
#define NOVIDEO_FILE_EN "/res/pic_704_576_p420_novideo_en.yuv"

#define NOVIDEO_FILE_P "/res/pic_704_576_p420_novideo.yuv"
#define NOVIDEO_FILE_P_EN "/res/pic_704_576_p420_novideo_en.yuv"

#define NOVIDEO_FILE_N "/res/pic_704_480_p420_novideo.yuv"
#define NOVIDEO_FILE_N_EN "/res/pic_704_480_p420_novideo_en.yuv"
#endif

extern char   *weekday[];
extern char   *weekday_en[];

extern HI_U32 s_u32GuiWidthDflt;							//Ĭ��GUI��
extern HI_U32 s_u32GuiHeightDflt;							//Ĭ��GUI��
extern VO_DEV s_VoDevCtrlDflt;
extern HI_U32 g_Max_Vo_Num;
extern PRV_VO_SLAVE_STAT_S s_slaveVoStat;


//////////////////////////////////////������///////////////////////////////
#define	BUFFER_NUM				200


//����Ƶ�ź�ͼƬ��Ϣ
#define NVR_NOVIDEO_FILE		"/res/nvr_novideo.jpg"
#define DVS_NOCONFIG_FILE		"/res/dvs_noconfig.jpg"
#define NVR_NOVIDEO_FILE_1	"/res/nvr_novideo.jpg"
#define NVR_NOVIDEO_FILE_2	"/res/nvr_novideo2.jpg"
#define NVR_NOVIDEO_FILE_TEST_1	"/res/gif-1.jpg"
#define NVR_NOVIDEO_FILE_TEST_2	"/res/gif-2.jpg"

#define NOVIDEO_IMAGWIDTH  		352 		//ͼ����
#define NOVIDEO_IMAGHEIGHT 		80  		//ͼ����
#define VLOSSPICBUFFSIZE 		(NOVIDEO_IMAGWIDTH * NOVIDEO_IMAGHEIGHT)      //�������ݴ�С

#define NOVIDEO_VDECWIDTH   	352     //Ϊ����Ƶ�ͺŴ����Ľ������� 
#define NOVIDEO_VDECHEIGHT  	288      //Ϊ����Ƶ�ͺŴ����Ľ�������


//#define CHIPNUM 2
#define DecAdec  				10//��Ƶ����ͨ��
#define DetVLoss_VdecChn 		30//��Ƶ��ʧͼƬ���ݽ���ͨ��
#define NoConfig_VdecChn 		(DetVLoss_VdecChn + 1)//δ����ͼƬ����ͨ��

#define H264ENC 				96
#define JPEGENC 				26

#define TimeInterval 			(500)//������Ƶ��������ʱ����,����������ʱ����Ҫͬ��(ms)
#define MAXFRAMELEN 			(4*1024*1024)//(2.5*1024*1024)

#if(DEV_TYPE == DEV_SN_9234_H_1)
#define RefFrameNum 			4
#else
#define RefFrameNum 			4
#endif

typedef struct
{
	HI_U32 height;    // ��Ƶ���ݸ� 
	HI_U32 width;  // ��Ƶ���ݿ�
	HI_U32 vdoType;  //��Ƶ���ݱ�������
	HI_U32 framerate;  //��Ƶ��֡�� 	
}PRV_VIDEOINFO;

typedef struct
{
	HI_S32 adoType;	//��Ƶ���ݱ������� 
	HI_U32 samrate;	// ��Ƶ���ݲ�����
	HI_U32 soundchannel;  // ��Ƶ����������
	HI_U32 PtNumPerFrm;	  //ÿ֡���������
	//unsigned  int bitwide;   // ��Ƶ����λ��
}PRV_AUDIOINFO;

typedef struct
{
	int pre_frame[DEV_CHANNEL_NUM];
	int is_same[DEV_CHANNEL_NUM];
	int is_first[DEV_CHANNEL_NUM];
	int is_disgard[DEV_CHANNEL_NUM];
	HI_S32 TotalLength1[DEV_CHANNEL_NUM];
	HI_U8* VideoDataStream1[DEV_CHANNEL_NUM];
	HI_U8* VideoDataStream2[DEV_CHANNEL_NUM];
}g_PRVSendChnInfo; 

typedef struct
{
	HI_S32 IsLocalVideo;		//������Ƶ�����ʶ��1:������Ƶ��0:IPC��Ƶ
	HI_S32 VoChn;				// ��Ӧ���ͨ�����	
	HI_S32 SlaveId;				// Ԥ����ͨ������ƬId  0:��Ƭ 1:��1�� 2:��,2��3:��3 
	
	//���³�ԱΪ����ͨ��ר��
	HI_S32 VdecChn;				//��Ӧ����ͨ����DetVLoss_Vdec(30):����Ƶ������>=0����Ӧ����Ч����ͨ��
	HI_S32 VdecCap;				//��Ӧ��ͨ�������ܴ�С��ȡ���ڷֱ���	
	HI_S32 IsBindVdec[PRV_VO_DEV_NUM];//�Ƿ��Vo.-1:δ�󶨣�0:��DetVLoss_Vdec(30)��1:����������ͨ����VI:0(���Ƭ����)
	UINT8 CurChnIndex; 			// ��ӦIPC��Ƶͨ���ţ���ʼֵΪ0
	UINT8 IsHaveVdec;			//�Ƿ񴴽�����ͨ����0:û�У�1:��
	UINT8 IsConnect;			//�Ƿ���IPC�豸����
	UINT8 bIsStopGetVideoData;	// ��ǰͨ����ͣ��ȡ����,1:��ͣ��0:���� 
	UINT8 bIsWaitIFrame;		//�Ƿ�ӹ���Client�������л�ȡ��һ��I֡
	UINT8 bIsWaitGetIFrame;		//�Ƿ��Ԥ���������л�ȡ��һ��I֡
	UINT8 bIsDouble;			//�Ƿ�˫��
	UINT8 PrvType;				// Ԥ�����͸ı�0:�ޱ仯 ��1:�����ͣ�2:ʵʱ��
	UINT8 MccCreateingVdec;
	UINT8 MccReCreateingVdec;
	UINT8 IsChooseSlaveId;
	UINT8 IsDiscard;				//���ܳ���ʱ����Ӧͨ�����ݶ���	
	UINT8 bIsPBStat;				//��ʱ�ط�״̬
	HI_U32 u32RefFrameNum;			//�ο�֡��Ŀ
	PRV_VIDEOINFO VideoInfo;		//IPC��Ƶ������Ϣ
	PRV_AUDIOINFO AudioInfo;		//IPC��Ƶ������Ϣ
}g_PRVVoChnInfo;   //��������ͨ����״̬��Ϣ�ṹ��

typedef struct
{
	UINT8 FirstHaveVideoData[MAX_IPC_CHNNUM];//д��һ֡��Ƶ���ݱ�ʶ
	UINT8 FirstHaveAudioData[MAX_IPC_CHNNUM];//д��һ����Ƶ���ݱ�ʶ
	UINT8 FirstGetData[MAX_IPC_CHNNUM];//��ȡ��һ֡��Ƶ���ݱ�ʶ
	UINT8 IsGetFirstData[MAX_IPC_CHNNUM];
	UINT8 IsStopGetVideoData[MAX_IPC_CHNNUM];
	UINT8 BeginSendData[MAX_IPC_CHNNUM];
	UINT8 bIsPBStat_StopWriteData[MAX_IPC_CHNNUM];//�ط�״̬�£�FTPC�ݲ�д����
	UINT8 bIsPBStat_BeyondCap[MAX_IPC_CHNNUM];
	int VideoDataCount[MAX_IPC_CHNNUM];
	int AudioDataCount[MAX_IPC_CHNNUM];
	int SendAudioDataCount[MAX_IPC_CHNNUM];
	HI_S64 VideoDataTimeLag[MAX_IPC_CHNNUM];
	HI_S64 AudioDataTimeLag[MAX_IPC_CHNNUM];
}g_PRVVoChnState;

typedef struct
{
	HI_U64	PreGetVideoPts;		//�ӻ������0ȡ��ǰһ���ݵ�PTS(δת��)
	HI_U64	CurGetVideoPts;		//�ӻ������0ȡ����ǰ���ݵ�PTS(δת��)
	HI_U64  PreVideoPts;  		//�ñ��ػ���ȡ��ǰһ���ݵ�PTS (δת��)
	HI_U64  CurVideoPts;   		//�ñ��ػ���ȡ����ǰ���ݵ�PTS(δת��)
	HI_U64  PreSendPts;  		//���������͵�ǰһ���ݵ�PTS(��ת��)
	HI_U64	CurSendPts;			//���������͵ĵ�ǰ���ݵ�PTS(��ת��)
	HI_U64  PreVoChnPts;		//���ǰһ���ݵ�PTS	
	HI_U64	CurVoChnPts;		//�����ǰ���ݵ�PTS	
	HI_U64	DevicePts;			//��Ϊ�������ݷ��͵�ǰ֡��PTS 
	HI_U64  DeviceIntervalPts;	//����PTS֡���
	HI_U64	IntervalPts;		//������֡�ͽ��������ݵ�PTS��� 
	HI_U64  ChangeIntervalPts;	//PTS����仯ʱ����Ӧ�����ݵ�ʱ���
	HI_U64	FirstVideoPts;		//��һ֡��Ƶ���ݵ�ʱ���
	HI_U64	FirstAudioPts;		//��һ֡��Ƶ���ݵ�ʱ���
	HI_U64	PreGetAudioPts;
	HI_U64  BaseVideoPts;		//���ڼ���500ms�������ݵ���ʼʱ�䣬�طŲ���
	HI_U64  IFrameOffectPts;  	// ����I֡�������΢��Ϊ��λ 
	HI_U64  pFrameOffectPts; 	// ����P֡�������΢��Ϊ��λ 
	HI_U64  CurShowPts;        	//�طŽ�������ʾʱ�������Ҫת��Ϊ����ʱ�� 
	time_t  StartPts;
	time_t 	CurTime;
	PRM_ID_TIME QueryStartTime;	//����ط�ʱ����ѯ�ļ�����ʼʱ��
	PRM_ID_TIME QueryFinalTime;	//����ط�ʱ����ѯ�ļ�����ʼʱ��
	PRM_ID_TIME StartPrm;		//��ǰ�ط��ļ�����ʼʱ��	���л��ļ�ʱ�����
	PRM_ID_TIME EndPrm;		//��ǰ�ط��ļ�����ʼʱ��	���л��ļ�ʱ�����
	
}g_PRVPtsinfo;   /* ʱ����Ϣ�ṹ��*/

typedef struct
{
	UINT8 SlaveCreatingVdec[DEV_CHANNEL_NUM];//��Ƭ�Ƿ����ڴ�������ͨ��
	UINT8 SlaveIsDesingVdec[DEV_CHANNEL_NUM];//��Ƭ�Ƿ��������ٽ���ͨ��
	
}g_PRVSlaveState;

 typedef struct
{
	int  SlaveId;
	int  s32StreamChnIDs;//����Ƭ��������ID��
	int  EncType;//����ͨ����������
	int  chn;//����ͨ���ţ���ʼֵ0
	int  VoChn;//��Ӧ�����ͨ���ţ�����ֵΪchn+LOCALVEDIONUM
	int  VdecChn;
	int  height;//�ֱ���:��
	int  width;//�ֱ���:��
	int  VdecCap;
}PRV_MccCreateVdecReq;

typedef struct
{
	int SlaveId;		 /* ��ƬId */
	int VoChn;
	int VdecChn;
	int Result; 	 /* �ط���Ӧ��-1��ʾʧ�ܣ�0��ʾ�ɹ� */
}PRV_MccCreateVdecRsp;

typedef struct
{
	int VoChn;
	int VdecChn;	/* ��ʾ��Ҫ���´�����ͨ���� */
	int height;		/* �µĸ߿�*/
	int width;
}PRV_ReCreateVdecIND;

typedef struct
{
	int SlaveId;
	int VoChn;
	int VdecChn;    /* ��ʾ��Ҫ���´�����ͨ���� */
	int height; 	/* �µĸ߿�*/
	int width;	
	int EncType;
	int VdecCap;
}PRV_MccReCreateVdecReq;

typedef struct
{
	int SlaveId;		 /* ��ƬId */
	int VoChn;
	int VdecChn;      /* ��ʾ��Ҫ���´�����ͨ���� */
	int height; 	// �µĸ߿�
	int width;
	int VdecCap;
	int Result;      /* ����ͨ�������0���ɹ�  -1��ʧ�� */
}PRV_MccReCreateVdecRsp;

typedef struct
{
	int chn;
	int NewPtNumPerFrm;
}PRV_ReCreateAdecIND;

typedef struct
{
	int VoChn;
}PRV_MccGetPtsReq;

typedef struct
{
	int VoChn;
	HI_U64 u64CurPts;
	int Result;
}PRV_MccGetPtsRsp;

typedef struct
{
	int VdecChn;
}PRV_MccDestroyVdecReq;

typedef struct
{
	int VdecChn;
	int Result;
}PRV_MccDestroyVdecRsp;

typedef struct
{
	int VdecChn;
}PRV_MccReSetVdecIND;

typedef struct
{
	int VdecChn;
}PRV_MccQueryVdecReq;

typedef struct
{
	int VdecChn;
	HI_U32 DecodeStreamFrames;
	HI_U32 LeftStreamFrames;
}PRV_MccQueryVdecRsp;

extern int IsAdecBindAo;
extern int IsCreateAdec;
extern int IsAudioOpen;

extern g_PRVVoChnInfo VochnInfo[DEV_CHANNEL_NUM];//IPC ��Ƶ���ͨ����ͨ����Ϣ
extern g_PRVSlaveState SlaveState;
extern g_PRVPtsinfo PtsInfo[MAX_IPC_CHNNUM];
extern g_PRVVoChnState VoChnState;
extern int g_PrvType;
extern int CurSlaveCap;
extern int CurMasterCap;
extern int CurCap;
extern int Achn;
extern int IsCreatingAdec;
extern int CurIPCCount;
extern int CurSlaveChnCount;
extern HI_U32 PtNumPerFrm;
extern int MasterToSlaveChnId;
extern pthread_mutex_t send_data_mutex;
extern sem_t sem_VoPtsQuery, sem_PlayPause, sem_PrvGetData, sem_PrvSendData, sem_PBGetData, sem_PBSendData;

extern AVPacket *PRV_OldVideoData[DEV_CHANNEL_NUM];
extern VDEC_CHN PRV_OldVdec[DEV_CHANNEL_NUM];
extern int PRV_CurIndex;
extern int PRV_SendDataLen;
extern int IsUpGrade;
extern time_t Probar_time[DEV_CHANNEL_NUM];

//////////////////////////////////////������end///////////////////////////////


//////////////////////////////////////Զ�̻ط�///////////////////////////////
#define G726_BPS MEDIA_G726_16K         /* MEDIA_G726_16K, MEDIA_G726_24K ... */
#define AMR_FORMAT AMR_FORMAT_MMS  /* AMR_FORMAT_MMS, AMR_FORMAT_IF1, AMR_FORMAT_IF2*/
#define AMR_MODE AMR_MODE_MR74         /* AMR_MODE_MR122, AMR_MODE_MR102 ... */
#define AUDIO_ADPCM_TYPE ADPCM_TYPE_DVI4

#define  VOMAX   (36)                          /* ��Ƶ���ͨ�����ֵ */
#define  MAXSLAVECNT    (3)			/* ����Ƭ��Ŀ */
#define  CLIPCNT    (4)			   /*  ����������*/ 
#define  CLIPFILECNT   (2)				/* ��󱣴��ļ��б��� */ 
#define AOCHN      (0)           			   /* ��Ƶ���ͨ���� */
#define ADECHN     (1)                        /* ��Ƶ����ͨ�� */
#define MaxVoDevId    (2)				   /* ��Ƶ����豸2 */
#define VOWIDTH        (1280)               /* ��׼VO��� */
#define VOHEIGH        (720)               /* ��׼VO�߶� */
#define SLAVEWIDTH    (PRV_BT1120_SIZE_W)
#define SLAVEHEIGHT   (PRV_BT1120_SIZE_H)
#define Pal_QCifVdecWidth  (176)      /* P��QCif������ */
#define Pal_QCifVdecHeight  (144)      /* P��QCif����߶� */
#define Pal_D1VdecWidth	(704)          /* P��D1 ������*/
#define Pal_D1VdecHeight	 (576)        /* P��D1����߶� */
#define _720PVdecWidth 	(1280)
#define _720PVdecHeight	(720)
#define _1080PVdecWidth 	(1920)
#define _1080PVdecHeight	(1080)
#define D1_PIXEL 	(Pal_D1VdecWidth * Pal_D1VdecHeight)
#define _720P_PIXEL ((_720PVdecWidth + 10) * (_720PVdecHeight + 10))
#define _1080P_PIXEL ((_1080PVdecWidth + 10) * (_1080PVdecHeight + 10))
#define PalDec_IntervalPts      (40000)   // P��������֡����ʱ������


#define buffersize  (_1080PVdecHeight * _1080PVdecWidth *2) /* ����������ݻ����ַ��С */
#define TESTPICBUFFSIZE		(Pal_D1VdecWidth * Pal_D1VdecHeight * 2) 
#define DEC_PCIV_VDEC_STREAM_BUF_LEN   (2 * 1024 * 1024)


enum READ_DATA_TYPE
{
	TYPE_NORMAL =1,	// ��ͨ����
	TYPE_NEXTIFRAME,	// ��һ��I֡
	TYPE_PREIFRAME	//��һ��I֡
}READ_DATA_TYPE;

enum SYN_STATE
{
	SYN_NOPLAY = 1,     // δ��ʼ�ط�״̬����ʼ״̬��
	SYN_WAITPLAY,       // �ȴ� 
	SYN_PLAYING,        //���ڻط� 
	SYN_OVER            // ���� 
}SYN_STATE;//ͨ���ط�״̬

enum  PLAY_STATE
{
	PLAY_EXIT = 1,     	// ����ͨԤ������
	PLAY_INSTANT,		//���ڼ�ʱ�ط�ͨ��(��ʱ�ط�״̬)
	PLAY_ENTER,     	//����طŽ��棬��δ��ʼ����
	PLAY_PROCESS,     	//ֻҪ��ͨ���յ��ط����ݣ���Ϊ����״̬
	PLAY_STOP,     		//����ͨ��������ɻ�ֹͣ����
}PRCO_STATE;//�ط�״̬

typedef struct         
{
	UINT8 PlayBackState;	//�ط�״̬
	UINT8 IsSingle;			//�Ƿ�ͨ���ط�
   	UINT8 DBClickChn;        //��·�Ŵ���ͨ�� 
   	UINT8 bISDB;             //�Ƿ�·�Ŵ�ط� 0�� 1��
   	UINT8 FullScreenId;    	//�Ƿ�ȫ����ʾ
   	UINT8 ImagCount;		//�طŻ�����
   	UINT8 IsPlaySound;
	UINT8 IsPause;
	UINT8 InstantPbChn;
	UINT8 ZoomChn;
	UINT8 IsZoom;
   	HI_S32 SubWidth;
	HI_S32 SubHeight;	
}g_PlayInfo; //�ط���Ϣ�ṹ�� 
typedef struct
{
   UINT8 FullScreenId;
   UINT8 ImagCount;
   UINT8 Single;
   UINT8 VoChn;
   UINT8 flag;
}PRV_MccPBCtlReq;
typedef struct
{
  int result;
  UINT8 flag;
}PRV_MccPBCtlRsp;


typedef struct
{
	UINT8 CurPlayState;     // ��ǰ�ط�״̬
	UINT8 CurSpeedState;    // ��ǰ�ط��ٶ�
	UINT8 QuerySlaveId;
	UINT8 SendDataType;		 //��ǰͨ������������ 
	UINT8 RealType;			//ʵʱ�ط����� 
	UINT8 SynState;         //��·�ط�ͨ��������״̬
	UINT8 bIsResetFile;
}g_ChnPlayStateInfo;  //����ͨ���ط���Ϣ�ṹ��



typedef struct
{
	UINT8   SlaveId;		 	// ��ƬId 
	UINT8   IsSingle;		    //�Ƿ�ͨ���ط�
	UINT8   ImageCount;         // �طŻ����� 
	UINT8   StreamChnIDs;		
	UINT16  subwidth;
	UINT16  subheight;
	UINT8   reserve[4];
} PlayBack_MccOpenReq;

typedef struct
{
	UINT8 SlaveId;		 //��ƬId 
	int result_DEV; 	 // �ط���Ӧ��-1��ʾʧ�ܣ�0��ʾ�ɹ� 
	UINT8 reserve[2];
} PlayBack_MccOpenRsp;

typedef struct
{
	int subwidth;
	int subheight;
	int bIsSingle; 
	int FullScreenId;
}PlayBack_MccFullScreenReq;

typedef struct
{
	int SlaveId;
}PlayBack_MccFullScreenRsp;

typedef struct
{
	int VoChn;
}PlayBack_MccGetVdecVoInfoReq;

typedef struct
{
	int VoChn;
	int Result;
	HI_U32 LeftStreamFrames;
	HI_U32 DecodeStreamFrames;
	HI_U64 u64CurPts;	
}PlayBack_MccGetVdecVoInfoRsp;

typedef struct
{
	int VoChn;
}PlayBack_MccQueryStateReq;

typedef struct
{
	int VoChn;
}PlayBack_MccQueryStateRsp;

typedef struct
{
	int bIsFullScreen;
	int bIsSingleShow;
	int VoChn;
	int bIsRsp;
}PlayBack_MccZoomReq;

typedef struct
{
	int SlaveId;
	int result;
}PlayBack_MccZoomRsp;

typedef struct
{
	int VoChn;
}PlayBack_MccCleanVoChnReq;

 typedef struct
 {
	 int VoChn;
 }PlayBack_MccPauseReq;

 typedef struct
 {
	 int VoChn;
	 int result;
 }PlayBack_MccPauseRsp;

 typedef struct
{
	int  SlaveId;
	int  EncType;			//����ͨ����������
	int  VoChn;
	int  VdecChn;
	int  s32VdecHeight;		//�ֱ���:��
	int  s32VdecWidth;		//�ֱ���:��
}PlayBack_MccPBCreateVdecReq;

typedef struct
{
	int SlaveId;		 	//��ƬId 
	int VoChn;
	int VdecChn;
	int Result; 	 		//�ط���Ӧ��-1��ʾʧ�ܣ�0��ʾ�ɹ� 
}PlayBack_MccPBCreateVdecRsp;


typedef struct
{
	int SlaveId;
	int EncType;
	int VoChn;
	int VdecChn;
	int s32VdecHeight;		//�ֱ���:��
	int s32VdecWidth;		//�ֱ���:��
}PlayBack_MccPBReCreateVdecReq;

typedef struct
{
	int VdecChn;      	//��ʾ��Ҫ���´�����ͨ���� 
	int Result;      	//����ͨ�������0���ɹ�  -1��ʧ�� 
}PlayBack_MccPBReCreateVdecRsp;

typedef struct
{
	int VdecChn;
}PlayBack_MccPBDestroyVdecReq;

typedef struct
{
	int VdecChn;
	int Result;
}PlayBack_MccPBDestroyVdecRsp;


typedef struct
{
	int  SlaveId;		 /* ��ƬId */
	int  ImShowCount;         /* �طŻ����� */
	int  subwidth;
	int  subheight;
	int  IsPlay[DEV_DEC_TOTALVOCHN];  /* �����ͨ���طű�ʶ��1��ʾ�طţ�0��ʾ���ط� */
	int  PosId[DEV_DEC_TOTALVOCHN];   /* �������ͨ��λ�� */
}PlayBack_MccReSetVoAttrReq;

typedef struct
{
	int  SlaveId;
	int  VoChn;
}PlayBack_MccProsBarReq;

typedef struct
{
	int  SlaveId;
	int  VoChn;
	int  Result;
}PlayBack_MccProsBarRsp;

//////////////////////////////////////Զ�̻ط�end////////////////////////////


//----------------------------------------------
//OSD�ַ���ʾ�ӿں���
//
//
int PRV_GetVoChnIndex(int VoChn);
int PRV_VoChnIsInCurLayOut(VO_DEV VoDev, VO_CHN VoChn);

int Prv_OSD_Show(unsigned char devid,unsigned char on);
int OSD_Set_Rec_Range_NP(unsigned char np_flag);
int OSD_Get_Rec_Range_Ch(unsigned char rec_group,int chn,int w,int h);
int OSD_Get_Preview_param(unsigned char devid,int w,int h,unsigned char ch_num,enum PreviewMode_enum prv_mode,unsigned char *pOrder);
int OSD_Set_NameType( int *pNameType);
int OSD_Compare_NameType( int *pNameType);
int OSD_Update_GroupName();

//int Prv_Set_Flicker(unsigned char on);

int Prv_Disp_OSD(unsigned char devid);
int Prv_OSD_Close_fb(unsigned char devid);
int Prv_OSD_Open_fb(unsigned char devid);	

int OSD_init(unsigned char time_type);
int OSD_Set_Time(char * str, char * qstr);
int OSD_Ctl(unsigned char ch,unsigned char on,unsigned char type);
//int OSD_Set_Time_xy(unsigned char ch,int x,int y);
int OSD_Set_Ch(unsigned char ch, char * str);
//int OSD_Set_CH_xy(unsigned char ch,int x,int y);
int OSD_Set_xy(unsigned char ch,int name_x,int name_y,int time_x,int time_y);

int Prv_Rec_Slave_OsdStr_Create(unsigned char ch,PRV_VO_SLAVE_STAT_S *p_Slave_info);



//***********************************************
//OSD_MASK
//
//
int OSD_Mask_update(unsigned char ch,const PRM_SCAPE_RECT* prect,unsigned char cov_num);
int OSD_Mask_Ctl(unsigned char ch,unsigned char on);
int OSD_Mask_disp_Ctl(unsigned char ch,unsigned char on);
int OSD_Mask_Ch_init(unsigned char ch);
int OSD_Mask_init(PPRV_VO_SLAVE_STAT_S pslave);
int OSD_Mask_Ch_Close(unsigned char ch);
int OSD_Mask_Close(void);
HI_S32 PRV_ReCreateVdecChn(HI_S32 chn, HI_S32 EncType, HI_S32 new_height, HI_S32 new_width, HI_U32 u32RefFrameNum, HI_S32 NewVdeCap);

int PRV_GetDoubleIndex();
HI_S32 PRV_GetPrvStat();
int PRV_GetDoubleToSingleIndex();
HI_S32 PRV_CreateVdecChn_EX(HI_S32 chn);
HI_BOOL PRV_GetVoiceTalkState();
HI_VOID PRV_FindMasterChnReChooseSlave(int ExtraCap, int index, int TmpIndex[]);
int PRV_FindMaster_Min(int ExtraCap, int index, int TmpIndex[]);
HI_VOID PRV_MasterChnReChooseSlave(int index);
void PRV_PBStateInfoInit(int chn);
void PRV_PBPlayInfoInit();
HI_S32 PRV_AUDIO_AoBindAdec(AUDIO_DEV AoDev, AO_CHN AoChn, ADEC_CHN AdChn);
HI_S32 PRV_PreviewInit(HI_VOID);
int PRV_SetPreviewVoDevInMode(int s32ChnCount);
void PlayBack_GetPlaySize(HI_U32 *Width, HI_U32 *Height);
HI_S32 PlayBack_QueryPbStat(HI_U32 Vochn);

extern int tw2865Init(unsigned char sysmode);
extern int Preview_GetAVstate(unsigned char ch);
extern HI_S32 PRV_TW2865_CfgV(VIDEO_NORM_E enVideoMode,VI_WORK_MODE_E enWorkMode);
extern HI_S32 PRV_TW2865_CfgAudio(AUDIO_SAMPLE_RATE_E enSample);
extern int Preview_SetVideo_x(unsigned char ch,unsigned char x_data);
extern int Preview_SetVideo_y(unsigned char ch,unsigned char y_data);
extern int Preview_SetVideo_Hue(unsigned char ch,unsigned char hue_data);
extern int Preview_SetVideo_Cont(unsigned char ch,unsigned char cont_data);
extern int Preview_SetVideo_Brt(unsigned char ch,unsigned char brt_data);
extern int Preview_SetVideo_Sat(unsigned char ch,unsigned char sat_data);
extern int Preview_SetVideoParam(unsigned char ch,const PRM_DISPLAY_CFG_CHAN *pInfo);
extern int GetVideoInputInfo(unsigned char ch,unsigned char *pInput_mode);

extern int tw2865_master_ain5_cfg(char flag);
extern int tw2865_ain_cfg(unsigned int chip, unsigned char mask);
extern int tw2865_master_pb_cfg(char flag);
extern int tw2865_master_tk_pb_switch(char flag);
extern int tw2865_special_reg_check(void);

extern HI_BOOL PRV_GetVoiceTalkState();

extern int PRV_Set_AudioMap(int ch, int pAudiomap);
extern int PRV_Set_AudioPreview_Enable(const BOOLEAN *pbAudioPreview);
extern HI_S32 PRV_AudioPreviewCtrl(const unsigned char *pchn, HI_U32 u32ChnNum);
extern HI_S32 PRV_GetVoPrvMode(VO_DEV VoDev, PRV_PREVIEW_MODE_E *pePreviewMode);//�ɹ�����HI_SUCCESS,ʧ�ܷ���HI_FAILURE
//extern HI_S32 PRV_GetPrvMode(PRV_PREVIEW_MODE_E *pePreviewMode);//�ɹ�����HI_SUCCESS,ʧ�ܷ���HI_FAILURE
extern HI_S32 PRV_GetVoChnRect(VO_DEV VoDev, VO_CHN VoChn, RECT_S *pstRect);
extern HI_S32 PRV_GetVoDevDispSize(VO_DEV VoDev, SIZE_S *pstSize);
extern HI_S32 PRV_GetVoDevImgSize(VO_DEV VoDev, SIZE_S *pstSize);
extern HI_S32 PRV_DisableDigChnAudio(HI_VOID);
extern HI_S32 PRV_EnableDigChnAudio(HI_VOID);
extern HI_S32 PRV_EnableAudioPreview(HI_VOID);
extern HI_S32 PRV_DisableAudioPreview(HI_VOID);
extern HI_S32 PRV_GetLocalAudioState(int chn);
extern HI_S32 PRV_LocalAudioOutputChange(int chn);
extern int PRV_TestAi(void);
extern HI_S32 PRV_TkPbSwitch(HI_S32 s32Flag);

extern HI_S32 PRV_BindGuiVo(int dev);//��G4ͼ�β㵽����豸��devֻ��Ϊ0-HD��1-AD

extern int PRV_OSD_SetGuiShow(unsigned int flag);
extern int OSD_G1_close(void);
extern int OSD_G1_open(void);
extern int Get_Fb_param_exit(void);

extern int Fb_clear_step1(void);
extern int Fb_clear_step2(void);
extern void set_TimeOsd_xy();
extern int get_OSD_param_init(PPRV_VO_SLAVE_STAT_S pSlave_state);
#if defined(Hi3531)||defined(Hi3535)
HI_S32 PRV_VO_UnBindVpss(VO_DEV VoDev,VO_CHN VoChn,VPSS_GRP VpssGrp,VPSS_CHN VpssChn);
HI_S32 PRV_VO_BindVpss(VO_DEV VoDev, VO_CHN VoChn, VPSS_GRP VpssGrp, VPSS_CHN VpssChn);
extern int FBSetAlphaKey(unsigned char alpha,int enable);
#endif
extern int Slave_Get_Time_InitInfo(Rec_Osd_Time_Info *pSlave_Time_Info,int Info_len);
extern void ScmChnCtrlReq(int flag);
extern int Ftpc_PlayFileCurTime(UINT8 ImageId, PRM_ID_TIME *StartPrmTime, PRM_ID_TIME *EndPrmTime);
extern int Ftpc_PlayFileCurTime_Sec(UINT8 ImageId, time_t *StartTime, time_t *EndTime);
extern int Ftpc_PlayFileAllTime(UINT8 ImageId, PRM_ID_TIME *StartPrmTime, PRM_ID_TIME *EndPrmTime);
extern HI_S32 PRV_AUDIO_AoUnbindAdec(AUDIO_DEV AoDev, AO_CHN AoChn, ADEC_CHN AdChn);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif

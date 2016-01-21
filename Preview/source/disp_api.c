/******************************************************************************

  Copyright (C), Star-Net

 ******************************************************************************
  File Name     : disp_api.c
  Version       : Initial Draft
  Author        : 
  Created       : 2010/06/09
  Description   : preview module implement file
  History       : ���6116������Ӧ���޸ģ����ý��治ͬ����������Ҫ�����޸�
  1.Date        : 2011.3.11
    Author      : chenyao
    Modification: Created file
	Modification: NVR	by luofeng
	Modification: Decorder	by luofeng
******************************************************************************/


/************************ HEADERS HEAR *************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/mman.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mpi_pciv.h>
#include <mpi_vi.h>
#include <hi_comm_vo.h>
#include "disp_api.h"
#include "dec_api.h"
#include "PlayBack_api.h"
#include "mdin241.h"
#include "hifb.h"
#include "sample_common.h"

#define PRV_BT1120_SIZE_H_P	PRV_BT1120_SIZE_H
#define PRV_BT1120_SIZE_H_N	PRV_BT1120_SIZE_H
#define ALIGN_BACK(x, a)              ((a) * (((x) / (a))))
STATIC HI_S32 PRV_VoInit(HI_VOID);

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

//4·guiʹ�õ��豸��Ϊ1��������ͬ��ʱ��Ӱ�첻����ҪһЩƥ�䣬���ǲ�ͬ��ʱ���Ͳ����ˣ���Ҫ�Ķ�

/************************ DATA TYPE HEAR *************************/
//�Ƿ�����Ƭ
#if defined(SN6116HE) || defined(SN6116LE) || defined(SN6108HE) || defined(SN8616D_LE)|| defined(SN8616M_LE) || defined(SN9234H1)
#define SN_SLAVE_ON
#endif

//��Ƭ��Ϣ���ͱ�־
#define SN_SLAVE_MSG 1
#if defined(Hi3531)||defined(Hi3535)
#define PIP_VIDEOLAYER
#endif



/************************ GLOBALS HEAR *************************/
#if defined(SN6104) || defined(SN8604D) || defined(SN8604M) || defined(SN6108) || defined(SN8608D) || defined(SN8608M)
static MPP_SYS_CONF_S s_stSysConfDflt = {
	.u32AlignWidth = 64,

};
#else
static MPP_SYS_CONF_S s_stSysConfDflt = {
	.u32AlignWidth = 16,	/* 16 or 64 */	
};
#endif


extern int ScmGetIsListCtrlStat();

/************************************************************************/
/*   ��Hi3511��Hi3512 ý�崦���������ָ��.pdf�� P(6-9):

	��Ƶ����صĹ��ܣ���Ҫ��ý��ҵ���ṩ��������ڴ棬�����ڴ�ķ���ͻ��գ���
	�ַ����ڴ滺��ص����ã��������ڴ���Դ�ڸ���ý�崦��ģ���к���ʹ�á�MPP ϵ
	ͳ��Hi3511/Hi3512 �����ڴ���MMZ��Media Memory Zone�������������ڴ洴����
	Ƶ����أ�������ɴ�С��ȡ������ַ�����Ļ������ɡ����軺��������
	BlkCnt ��ʾ��������С��BlkSize ��ʾ����ô����һ������صĹ��̼�Ϊ��MMZ ��
	���СΪBlkCnt%BlkSize �����������ڴ�ռ䣬�ٽ�����صȷ�ΪBlkCnt ������顣
	���л������MPP ϵͳ��VB ģ���������ҵ��ģ�����������뻺����Ի�ȡ��Ӧ
	�ڴ���Դ��

	����ط�Ϊ��������غ�˽�л���أ���ͼ6-3 ��ʾ��MPP ϵͳ��ʼ��ǰ�������ȳ�
	ʼ�����й�������أ����û�����HI_MPI_VB_SetConf �ӿڶԹ���������ڵĻ����
	�����ͻ�����С�������ã��ٵ���HI_MPI_VB_Init �ӿڽ�����س�ʼ����ϵͳ�ڲ�
	��MMZ �������ڴ洴�����й�������أ���MPP ϵͳ��ʼ��֮�󣬸�ģ����ݾ���
	ҵ�񴴽�˽�л�����Է��������Դ�����⣬�û�����HI_MPI_VB_CreatePool ������
	�����Ҳ��˽�л���ء�
  
	����������ڵĻ������Ҫ��VI��VENC ��PCIV ģ�黺��ͼ��Buffer��ϵͳ������
	�й����������Ѱ�����е���ӽ���С�Ļ���鹩��ģ��ʹ�ã���˸��ݾ���ҵ����
	�ó����ʴ�С�Ļ��������Ч������ϵͳ�ڴ���Դ������ϵͳ����ʱVI ����D1��
	Half-D1��CIF �ȴ�С��ͨ�����ã���ô����Ҫ�ֱ����ó���Щ��С�Ļ���أ���ͼ��
	���ظ�ʽYUV420 Ϊ����BlkSize = Stride * Height * 1.5��

	�������������������һ������Ϊ������л����ĸ��������������Ŀ����������
	�ܵ���MPP ģ�������ҵ���ܵ�Ӱ�죨�����޷�����ͼ�񡢱��붪֡�ȣ���Ŀǰ����
	��������԰�������ԭ�����ã�
	# ÿ�� VI ͨ����Ҫ3 ��VI ֡ͼ���С�Ļ���顣
	# ÿ����Ƶ����ͨ������Ҫ 3 ����������С�Ļ���飨���˫������������������
	���ǲ�һ���ģ���
	# ÿ���� VI �󶨵�VO ͨ����Ҫ2 ��VI ͨ����С�Ļ���顣
	# ��Ƭÿ�� PCIV ͨ����Ҫ�Ļ���������Ϊ���õ�PCI ��Buffer ������������
	СΪ���õ�PCI Ŀ��ͼ���С��
	# ��Ҫ�����û����� HI_MPI_VB_GetBlock �ӹ���������л�ȡ�Ļ������ռ�õ�
	��Դ��

	���ڻ������Ա����ģ��乲�ã����ʵ����Ҫ�Ļ������ܱ�����˵����Ҫ�١�
	һ������£��ο�����˵���������ü��ɣ����絥Ƭ1 ·CIF �ı����Ԥ��ҵ����Ҫ
	��CIF ��С��������Ϊ��3+3+2=8 �飻������ڴ���Դ�������нϸ�Ҫ�󣬿��Ը�
	��ʵ��ҵ�񸺺ɽ�ÿͨ���Ļ������Ŀ����1~2 �飬����ͨ���鿴/proc/umap/vb ����
	��ϵͳʵ����������Ƶ����ص�ʹ�������ͨ���鿴/proc/umap/vi �е�VbFail �˽�VI
	�Ƿ��л�ȡ�����ʧ�ܡ�

	˽�л������Ҫ������Ƶ���롢��Ƶ����ģ���ڲ�Ϊÿһ·ͨ������֡ͼ��Buffer��
	��˽�л������Դ�ڿ���MMZ �ռ䣬����MPP ϵͳ��Ҳ�ᵥ����MMZ �л�ȡ��
	�棬�������MMZ �ռ䲻�㣬�����´���ͨ���ȹ�������ʧ�ܣ�����û�����MMZ
	ʱ�����˹���������⻹ҪԤ��һ���ڴ�ռ䡣ĿǰMPP ϵͳ�ڳ��������������Ҫ
	����MMZ �ڴ���Դ�������£�
	# ����һ· H.264 ����ͨ������ҪMMZ �ռ�Ϊͼ���С%4��
	# ����һ· H.264 ����ͨ������ҪMMZ �ռ�Ϊͼ���С%���ο�֡��Ŀ+4����
	# ����һ· MJPEG ����ͨ������ҪMMZ �ռ�Լ200K ���ҡ�
	# ����һ· MJPEG ����ͨ������ҪMMZ �ռ�Ϊͼ���С%3��
	# ��������Ƶ��MD��VPP ��ģ��Ҳ��Ҫ��MMZ �������ʵ��ڴ棬�ܹ�Լ5M ���ҡ�
                                                                  */
/************************************************************************/
/* 16CIF Series Products: 
 *     VI: 6D1 + 10CIF  
 *     VencGroup: 16CIF(��������)
 *     VoChn: 3Dev 16Chn + 16Chn + 1Chn
 *     PCIV: none
*/

#if defined(SN9234H1)
static VB_CONF_S s_stVbConfDflt = {
	.u32MaxPoolCnt = VB_MAX_POOLS,
	.astCommPool = {
		//	{704 * 576 * 2, 16 * 2},		/*VB[0]: D1*/
		//	{1280 * 720 * 2, 8 * 2},		/*VB[0]: 720P*/
		//	{1920 * 1088 * 2, 4 * 2},	/*VB[0]: 1080P*/
				
		//	{704 * 576 * 2, 10},		/*VB[1]: 2CIF*/
		//	{352 * 288 * 1.5, 80},			/*VB[2]: CIF*/

		//	{174 * 144 * 1.5, 24},			/*VB[3]: 1/16D1*/
		//	{720 * 576 * 2, 10},			/*VB[4]: D1*/
		//	{240 * 192 * 2, 0},			/*VB[5]: 1/9D1*/
		//	{360 * 288 * 2, 0},			/*VB[6]: 1/4D1*/
		//	{800 * 600 * 2, 0}				/*VB[7]: 800*600*/
		//	{PRV_BT1120_SIZE_W * PRV_BT1120_SIZE_H* 2, 25},			/*PCI DMA*/
			{1920 * 1088 * 2, 10},			/*PCI DMA*/
		
 		//	{1280 * 1024 * 2, 25},			/*VB[3]: 1280*1024*/
		// 	{1440 * 900 * 2, 0},			/*VB[4]: 1440*900*/
		// 	{1366 * 768 * 2, 0},			/*VB[5]: 1366*768*/
		// 	{1024 * 768 * 2, 2},			/*VB[6]: 1024*768*/
		// 	{800 * 600 * 2, 0}				/*VB[7]: 800*600*/
		},
};

#else
static VB_CONF_S s_stVbConfDflt = {
	.u32MaxPoolCnt = VB_MAX_POOLS,
	.astCommPool = {
			{720 * 576 * 2, 5},
		//	{720 * 576 * 1.5, 16 * 2},	/*VB[0]: D1*/
		//	{1280 * 720 * 2, 8 * 2},		/*VB[0]: 720P*/
		//	{1920 * 1080 * 2, 4 * 2},		/*VB[0]: 1080P*/
				
		//	{704 * 576 * 2, 10},			/*VB[1]: 2CIF*/
		//	{352 * 288 * 1.5, 80},			/*VB[2]: CIF*/

		//	{174 * 144 * 1.5, 24},			/*VB[3]: 1/16D1*/
		//	{720 * 576 * 2, 10},			/*VB[4]: D1*/
		//	{240 * 192 * 2, 0},			/*VB[5]: 1/9D1*/
		//	{360 * 288 * 2, 0},			/*VB[6]: 1/4D1*/
		//	{800 * 600 * 2, 0}				/*VB[7]: 800*600*/
		//	{PRV_BT1120_SIZE_W * PRV_BT1120_SIZE_H* 1.5, 25},			/*PCI DMA*/
 		//	{1280 * 1024 * 2, 3},			/*VB[3]: 1280*1024*/
		// 	{1440 * 900 * 2, 0},			/*VB[4]: 1440*900*/
		// 	{1366 * 768 * 2, 0},			/*VB[5]: 1366*768*/
		// 	{1024 * 768 * 2, 2},			/*VB[6]: 1024*768*/
		// 	{800 * 600 * 2, 0}				/*VB[7]: 800*600*/
		},
	
};
#endif

static PRV_STATE_INFO_S s_State_Info =
{
	.bIsInit = 0,
	.bslave_IsInit =0,
	.bIsReply = 1,
	.bIsNpfinish = 0,
	.bIsOsd_Init = 0,
	.bIsRe_Init = 0,
	.bIsVam_rsp = 1,	//	Ĭ��״̬Ϊ�ѻظ���Ϣ
	.TimeoutCnt = 0,
	.bIsTimerState = 0,
	.bIsSlaveConfig = 0,
	.f_timer_handle = -1,
	.g_zoom_first_in = 0,
	.Prv_msg_Cur = NULL,
};
/*static*/ PRV_VO_SLAVE_STAT_S s_slaveVoStat={
	.enPreviewMode = PRV_VO_MAX_MOD,
	.s32PreviewIndex = 0,
	.s32SingleIndex = 0,
	.bIsSingle = HI_FALSE,
	.enVideoNorm = 0,  //N/P��ʽ���ã�0-PAL, 1-NTSC
};

#if defined(SN9234H1)
static PRV_VO_DEV_STAT_S s_astVoDevStatDflt[] = {
	{/* HD */
		.stVoPubAttr = {
				.u32BgColor = 0x000000,									//�豸����ɫ����ʾ����RGB888��
				.enIntfType = VO_INTF_BT1120,								//�ӿ����͵�������
				.enIntfSync = VO_OUTPUT_1080P25,					//�ӿ�ʱ�������
				.stSyncInfo = {0},										//�Զ���ӿ�ʱ��ṹ��
			},
		.stVideoLayerAttr = {
				.stDispRect		= {0, 0, 1024, 768},					//��Ƶ��ʾ������νṹ�塣
				.stImageSize	= {PRV_IMAGE_SIZE_W, PRV_IMAGE_SIZE_H_P},							//ͼ��ֱ��ʽṹ�壬���ϳɻ���ߴ硣
				.u32DispFrmRt	= 25,									//��Ƶ��ʾ֡�ʡ�
				.enPixFormat	= PIXEL_FORMAT_YUV_SEMIPLANAR_422,		//��Ƶ���������ظ�ʽ��SPYCbCr420 ����SPYCbCr422��
				.s32PiPChn		= VO_DEFAULT_CHN,						//����ϳ�·����ʶ��Ĭ��ֵΪVO_DEFAULT_CHN��
			},
		
		.enPreviewStat = PRV_STAT_NORM,
		.enPreviewMode = PRV_VO_MAX_MOD,
		.enCtrlFlag = PRV_CTRL_BUTT,			 
		.s32PreviewIndex = 0,
		.s32SingleIndex = 0,
		.s32AlarmChn = 0,
		.s32CtrlChn = 0,
		.bIsAlarm = HI_FALSE,
		.bIsSingle = HI_FALSE,
	}/* END HD */,

	{/* AD */
		.stVoPubAttr = {
				.u32BgColor = 0x000000,
				.enIntfType = VO_INTF_CVBS,
				.enIntfSync = VO_OUTPUT_PAL,
				.stSyncInfo = {0},
			},
		.stVideoLayerAttr = {
				.stDispRect		= {0, 0, 720-PRV_CVBS_EDGE_CUT_W*2, 576-PRV_CVBS_EDGE_CUT_H*2},/*�˴�����SN6104��CVBS�ϵ�GUI������ʾ��Χ��BUG��*/
				.stImageSize	= {PRV_IMAGE_SIZE_W, PRV_IMAGE_SIZE_H_P},
				.u32DispFrmRt	= 25,
				.enPixFormat	= PIXEL_FORMAT_YUV_SEMIPLANAR_420,
				.s32PiPChn		= VO_DEFAULT_CHN,
			},
		.enPreviewStat = PRV_STAT_NORM,
		.enPreviewMode = PRV_VO_MAX_MOD,
		.enCtrlFlag = PRV_CTRL_BUTT,			 
		.s32PreviewIndex = 0,
		.s32SingleIndex = 0,
		.s32AlarmChn = 0,
		.s32CtrlChn = 0,
		.bIsAlarm = HI_FALSE,
		.bIsSingle = HI_FALSE,
	}/* END AD */,

	{/* SD */
		.stVoPubAttr = {
				.u32BgColor = 0x000000,
				.enIntfType = VO_INTF_CVBS,
				.enIntfSync = VO_OUTPUT_PAL,
				.stSyncInfo = {0},
			},
		.stVideoLayerAttr = {
				.stDispRect		= {0, 0, 720-PRV_CVBS_EDGE_CUT_W*2, 576-PRV_CVBS_EDGE_CUT_H*2},	
				.stImageSize	= {PRV_IMAGE_SIZE_W, PRV_IMAGE_SIZE_H_P},
				.u32DispFrmRt	= 25,
				.enPixFormat	= PIXEL_FORMAT_YUV_SEMIPLANAR_420,
				.s32PiPChn		= VO_DEFAULT_CHN,
			},
		.enPreviewStat = PRV_STAT_NORM,
		.enPreviewMode = SingleScene,//SixteenScene,
		.enCtrlFlag = PRV_CTRL_BUTT,
		.s32PreviewIndex = 0,
		.s32SingleIndex = 0,
		.s32AlarmChn = 0,
		.s32CtrlChn = 0,
		.bIsAlarm = HI_FALSE,
		.bIsSingle = HI_FALSE,
	}/* END SD */
};
#elif defined(SN9300)
static PRV_VO_DEV_STAT_S s_astVoDevStatDflt[] = {
	{/* HD */


		.stVoPubAttr = {
				.u32BgColor = 0x000000,									//�豸����ɫ����ʾ����RGB888��
				.enIntfType = VO_INTF_VGA |VO_INTF_HDMI,								//�ӿ����͵�������
				.enIntfSync = VO_OUTPUT_1024x768_60,					//�ӿ�ʱ�������
				.stSyncInfo = {0},										//�Զ���ӿ�ʱ��ṹ��
			},
		.stVideoLayerAttr = {
				.stDispRect		= {0, 0, 1024, 768},					//��Ƶ��ʾ������νṹ�塣
				.stImageSize	= {1024, 768},							//ͼ��ֱ��ʽṹ�壬���ϳɻ���ߴ硣
				.u32DispFrmRt	= 25,									//��Ƶ��ʾ֡�ʡ�
				.enPixFormat	= PIXEL_FORMAT_YUV_SEMIPLANAR_420,		//��Ƶ���������ظ�ʽ��SPYCbCr420 ����SPYCbCr422��?
				.bDoubleFrame = HI_FALSE,
				.bClusterMode = HI_FALSE,
			},
		
		.enPreviewStat = PRV_STAT_NORM,
		.enPreviewMode = PRV_VO_MAX_MOD,
		.enCtrlFlag = PRV_CTRL_BUTT,			 
		.s32PreviewIndex = 0,
		.s32SingleIndex = 0,
		.s32AlarmChn = 0,
		.s32CtrlChn = 0,
		.bIsAlarm = HI_FALSE,
		.bIsSingle = HI_FALSE,
	}/* END HD */,

	{/* AD */
		.stVoPubAttr = {
				.u32BgColor = 0x000000,									//�豸����ɫ����ʾ����RGB888��
				.enIntfType = VO_INTF_VGA | VO_INTF_HDMI,								//�ӿ����͵�������
				.enIntfSync = VO_OUTPUT_1080P60,					//�ӿ�ʱ�������
				.stSyncInfo = {0},										//�Զ���ӿ�ʱ��ṹ��
			},
		.stVideoLayerAttr = {
				.stDispRect		= {0, 0, 1024, 768},					//��Ƶ��ʾ������νṹ�塣
				.stImageSize	= {PRV_IMAGE_SIZE_W, PRV_IMAGE_SIZE_H_P},							//ͼ��ֱ��ʽṹ�壬���ϳɻ���ߴ硣
				.u32DispFrmRt	= 25,									//��Ƶ��ʾ֡�ʡ�
				.enPixFormat	= PIXEL_FORMAT_YUV_SEMIPLANAR_420,		//��Ƶ���������ظ�ʽ��SPYCbCr420 ����SPYCbCr422��?
				.bDoubleFrame = HI_FALSE,
				.bClusterMode = HI_FALSE,
			},
		.enPreviewStat = PRV_STAT_NORM,
		.enPreviewMode = PRV_VO_MAX_MOD,
		.enCtrlFlag = PRV_CTRL_BUTT,			 
		.s32PreviewIndex = 0,
		.s32SingleIndex = 0,
		.s32AlarmChn = 0,
		.s32CtrlChn = 0,
		.bIsAlarm = HI_FALSE,
		.bIsSingle = HI_FALSE,
	}/* END AD */,

	{/* SD */
		.stVoPubAttr = {
				.u32BgColor = 0x000000,
				.enIntfType = VO_INTF_CVBS,
				.enIntfSync = VO_OUTPUT_PAL,
				.stSyncInfo = {0},
			},
		.stVideoLayerAttr = {
				.stDispRect		= {0, 0, 720-PRV_CVBS_EDGE_CUT_W*2, 576-PRV_CVBS_EDGE_CUT_H*2},	
				.stImageSize	= {PRV_IMAGE_SIZE_W, PRV_IMAGE_SIZE_H_P},
				.u32DispFrmRt	= 25,
				.enPixFormat	= PIXEL_FORMAT_YUV_SEMIPLANAR_420,
				.bDoubleFrame = HI_FALSE,
				.bClusterMode = HI_FALSE,
			},
		.enPreviewStat = PRV_STAT_NORM,
		.enPreviewMode = SingleScene,//SixteenScene,
		.enCtrlFlag = PRV_CTRL_BUTT,
		.s32PreviewIndex = 0,
		.s32SingleIndex = 0,
		.s32AlarmChn = 0,
		.s32CtrlChn = 0,
		.bIsAlarm = HI_FALSE,
		.bIsSingle = HI_FALSE,
	}
};

#else
static PRV_VO_DEV_STAT_S s_astVoDevStatDflt[] = {
	{/* DHD0 */

		.stVoPubAttr = {
				.u32BgColor = 0x000000,									//�豸����ɫ����ʾ����RGB888��
				.enIntfType = VO_INTF_VGA |VO_INTF_HDMI,								//�ӿ����͵�������
				.enIntfSync = VO_OUTPUT_1024x768_60,					//�ӿ�ʱ�������
				.stSyncInfo = {0},										//�Զ���ӿ�ʱ��ṹ��
				.bDoubleFrame = HI_FALSE,
			},
		.stVideoLayerAttr = {
				.stDispRect		= {0, 0, 1024, 768},					//��Ƶ��ʾ������νṹ�塣
				.stImageSize	= {1024, 768},							//ͼ��ֱ��ʽṹ�壬���ϳɻ���ߴ硣
				.u32DispFrmRt	= 25,									//��Ƶ��ʾ֡�ʡ�
				.enPixFormat	= PIXEL_FORMAT_YUV_SEMIPLANAR_420,		//��Ƶ���������ظ�ʽ��SPYCbCr420 ����SPYCbCr422��?
			},

		
		.enPreviewStat = PRV_STAT_NORM,
		.enPreviewMode = PRV_VO_MAX_MOD,
		.enCtrlFlag = PRV_CTRL_BUTT,			 
		.s32PreviewIndex = 0,
		.s32SingleIndex = 0,
		.s32AlarmChn = 0,
		.s32CtrlChn = 0,
		.bIsAlarm = HI_FALSE,
		.bIsSingle = HI_FALSE,
	}/* END DHD0 */,

	{/* DHD1 */
		.stVoPubAttr = {
				.u32BgColor = 0x000000,									//�豸����ɫ����ʾ����RGB888��
				.enIntfType = VO_INTF_VGA | VO_INTF_HDMI,								//�ӿ����͵�������
				.enIntfSync = VO_OUTPUT_1080P50,					//�ӿ�ʱ�������
				.stSyncInfo = {0},										//�Զ���ӿ�ʱ��ṹ��
				.bDoubleFrame = HI_FALSE,
			},
		.stVideoLayerAttr = {
				.stDispRect		= {0, 0, 1024, 768},					//��Ƶ��ʾ������νṹ�塣
				.stImageSize	= {PRV_IMAGE_SIZE_W, PRV_IMAGE_SIZE_H_P},							//ͼ��ֱ��ʽṹ�壬���ϳɻ���ߴ硣
				.u32DispFrmRt	= 25,									//��Ƶ��ʾ֡�ʡ�
				.enPixFormat	= PIXEL_FORMAT_YUV_SEMIPLANAR_420,		//��Ƶ���������ظ�ʽ��SPYCbCr420 ����SPYCbCr422��?
			},
		.enPreviewStat = PRV_STAT_NORM,
		.enPreviewMode = PRV_VO_MAX_MOD,
		.enCtrlFlag = PRV_CTRL_BUTT,			 
		.s32PreviewIndex = 0,
		.s32SingleIndex = 0,
		.s32AlarmChn = 0,
		.s32CtrlChn = 0,
		.bIsAlarm = HI_FALSE,
		.bIsSingle = HI_FALSE,
	}/* END DHD1*/,

	{/* DSD0 */
		.stVoPubAttr = {
				.u32BgColor = 0x000000,
				.enIntfType = VO_INTF_CVBS,
				.enIntfSync = VO_OUTPUT_PAL,
				.stSyncInfo = {0},
				.bDoubleFrame = HI_FALSE,
			},
		.stVideoLayerAttr = {
				.stDispRect		= {0, 0, 720-PRV_CVBS_EDGE_CUT_W*2, 576-PRV_CVBS_EDGE_CUT_H*2},	
				.stImageSize	= {PRV_IMAGE_SIZE_W, PRV_IMAGE_SIZE_H_P},
				.u32DispFrmRt	= 25,
				.enPixFormat	= PIXEL_FORMAT_YUV_SEMIPLANAR_420,
			},
		.enPreviewStat = PRV_STAT_NORM,
		.enPreviewMode = SingleScene,//SixteenScene,
		.enCtrlFlag = PRV_CTRL_BUTT,
		.s32PreviewIndex = 0,
		.s32SingleIndex = 0,
		.s32AlarmChn = 0,
		.s32CtrlChn = 0,
		.bIsAlarm = HI_FALSE,
		.bIsSingle = HI_FALSE,
	},/* END DSD0 */
	{/* DSD1 */
		.stVoPubAttr = {
				.u32BgColor = 0x000000,
				.enIntfType = VO_INTF_CVBS,
				.enIntfSync = VO_OUTPUT_PAL,
				.stSyncInfo = {0},
				.bDoubleFrame = HI_FALSE,
			},
		.stVideoLayerAttr = {
				.stDispRect		= {0, 0, 720-PRV_CVBS_EDGE_CUT_W*2, 576-PRV_CVBS_EDGE_CUT_H*2},	
				.stImageSize	= {PRV_IMAGE_SIZE_W, PRV_IMAGE_SIZE_H_P},
				.u32DispFrmRt	= 25,
				.enPixFormat	= PIXEL_FORMAT_YUV_SEMIPLANAR_420,
			},
		.enPreviewStat = PRV_STAT_NORM,
		.enPreviewMode = SingleScene,//SixteenScene,
		.enCtrlFlag = PRV_CTRL_BUTT,
		.s32PreviewIndex = 0,
		.s32SingleIndex = 0,
		.s32AlarmChn = 0,
		.s32CtrlChn = 0,
		.bIsAlarm = HI_FALSE,
		.bIsSingle = HI_FALSE,
	},
	{/* DSD2 */
		.stVoPubAttr = {
				.u32BgColor = 0x000000,
				.enIntfType = VO_INTF_CVBS,
				.enIntfSync = VO_OUTPUT_PAL,
				.stSyncInfo = {0},
				.bDoubleFrame = HI_FALSE,
			},
		.stVideoLayerAttr = {
				.stDispRect		= {0, 0, 720-PRV_CVBS_EDGE_CUT_W*2, 576-PRV_CVBS_EDGE_CUT_H*2},	
				.stImageSize	= {PRV_IMAGE_SIZE_W, PRV_IMAGE_SIZE_H_P},
				.u32DispFrmRt	= 25,
				.enPixFormat	= PIXEL_FORMAT_YUV_SEMIPLANAR_420,
			},
		.enPreviewStat = PRV_STAT_NORM,
		.enPreviewMode = SingleScene,//SixteenScene,
		.enCtrlFlag = PRV_CTRL_BUTT,
		.s32PreviewIndex = 0,
		.s32SingleIndex = 0,
		.s32AlarmChn = 0,
		.s32CtrlChn = 0,
		.bIsAlarm = HI_FALSE,
		.bIsSingle = HI_FALSE,
	},
	{/* DSD3 */
		.stVoPubAttr = {
				.u32BgColor = 0x000000,
				.enIntfType = VO_INTF_CVBS,
				.enIntfSync = VO_OUTPUT_PAL,
				.stSyncInfo = {0},
				.bDoubleFrame = HI_FALSE,
			},
		.stVideoLayerAttr = {
				.stDispRect		= {0, 0, 720-PRV_CVBS_EDGE_CUT_W*2, 576-PRV_CVBS_EDGE_CUT_H*2},	
				.stImageSize	= {PRV_IMAGE_SIZE_W, PRV_IMAGE_SIZE_H_P},
				.u32DispFrmRt	= 25,
				.enPixFormat	= PIXEL_FORMAT_YUV_SEMIPLANAR_420,
			},
		.enPreviewStat = PRV_STAT_NORM,
		.enPreviewMode = SingleScene,//SixteenScene,
		.enCtrlFlag = PRV_CTRL_BUTT,
		.s32PreviewIndex = 0,
		.s32SingleIndex = 0,
		.s32AlarmChn = 0,
		.s32CtrlChn = 0,
		.bIsAlarm = HI_FALSE,
		.bIsSingle = HI_FALSE,
	},
	{/* DSD4 */
		.stVoPubAttr = {
				.u32BgColor = 0x000000,
				.enIntfType = VO_INTF_CVBS,
				.enIntfSync = VO_OUTPUT_PAL,
				.stSyncInfo = {0},
				.bDoubleFrame = HI_FALSE,
			},
		.stVideoLayerAttr = {
				.stDispRect		= {0, 0, 720-PRV_CVBS_EDGE_CUT_W*2, 576-PRV_CVBS_EDGE_CUT_H*2},	
				.stImageSize	= {PRV_IMAGE_SIZE_W, PRV_IMAGE_SIZE_H_P},
				.u32DispFrmRt	= 25,
				.enPixFormat	= PIXEL_FORMAT_YUV_SEMIPLANAR_420,
			},
		.enPreviewStat = PRV_STAT_NORM,
		.enPreviewMode = SingleScene,//SixteenScene,
		.enCtrlFlag = PRV_CTRL_BUTT,
		.s32PreviewIndex = 0,
		.s32SingleIndex = 0,
		.s32AlarmChn = 0,
		.s32CtrlChn = 0,
		.bIsAlarm = HI_FALSE,
		.bIsSingle = HI_FALSE,
	},
	{/* DSD5 */
		.stVoPubAttr = {
				.u32BgColor = 0x000000,
				.enIntfType = VO_INTF_CVBS,
				.enIntfSync = VO_OUTPUT_PAL,
				.stSyncInfo = {0},
				.bDoubleFrame = HI_FALSE,
			},
		.stVideoLayerAttr = {
				.stDispRect		= {0, 0, 720-PRV_CVBS_EDGE_CUT_W*2, 576-PRV_CVBS_EDGE_CUT_H*2},	
				.stImageSize	= {PRV_IMAGE_SIZE_W, PRV_IMAGE_SIZE_H_P},
				.u32DispFrmRt	= 25,
				.enPixFormat	= PIXEL_FORMAT_YUV_SEMIPLANAR_420,
			},
		.enPreviewStat = PRV_STAT_NORM,
		.enPreviewMode = SingleScene,//SixteenScene,
		.enCtrlFlag = PRV_CTRL_BUTT,
		.s32PreviewIndex = 0,
		.s32SingleIndex = 0,
		.s32AlarmChn = 0,
		.s32CtrlChn = 0,
		.bIsAlarm = HI_FALSE,
		.bIsSingle = HI_FALSE,
	}
};
#endif

#if defined(SN9234H1)
static VO_CHN_ATTR_S s_stVoChnAttrDflt = {
	.u32Priority	= PRV_DFLT_CHN_PRIORITY,					//��Ƶͨ���������ȼ������ȼ��ߵ����ϲ㡣ȡֵ��Χ��[0, 31]����̬���ԡ�
	.stRect			= {0, 0, 352, 288},							//ͨ��������ʾ��������Ļ�����Ͻ�Ϊԭ�㡣�þ��ε����Ͻ����������2 ���룬�Ҹþ��������������Ļ��Χ֮�ڡ���̬���ԡ�
	.bZoomEnable	= HI_TRUE,									//���ſ��ر�ʶ��ȡֵ��Χ��.. HI_TRUE��������ͼ�����ų�stRect ����ĳߴ�����Ļ����ʾ��.. HI_FALSE������ͼ���ϼ���stRect ����ľ������������ʾ����̬���ԡ�
	.bDeflicker		= HI_FALSE,									//ͨ������˸���ء�ȡֵ��Χ��.. HI_TRUE��������ͼ��������˸�������ʾ��.. HI_FALSE������ͨ������ͼ��������˸����ֱ����ʾ����̬���ԡ�
};
#endif

//static VO_ZOOM_RATIO_S s_astZoomAttrDflt[] = {{0,0,	352,288},{0,0,	352,288}};

#if defined(Hi3520)
static VI_PUB_ATTR_S s_stViDevPubAttrDflt = {
	.enInputMode	= VI_MODE_BT656,							//��Ƶ����ӿ�ģʽ����̬���ԡ�
	.enWorkMode		= VI_WORK_MODE_4D1,							//��Ƶ���빤��ģʽ����̬���ԡ�
	.enViNorm		= VIDEO_ENCODING_MODE_PAL,					//�ӿ���ʽ����̬���ԡ�
	.bIsChromaChn	= HI_FALSE,									//�Ƿ�ɫ��ͨ����
	.bChromaSwap	= HI_FALSE,									//�Ƿ�ɫ�����ݽ�����
};

static VI_CHN_ATTR_S s_stViChnAttrDflt = {
	.stCapRect			= {8, 0, 704, 288},						//ͨ�������������ԡ���̬���ԡ�����Hi3520��Hi3515ý�崦����������ο�2.pdf��P3-11,3-12
	.enCapSel			= VI_CAPSEL_BOTH,						//֡��ѡ�񡣶�̬���ԡ�
	.bDownScale			= HI_FALSE,								//1/2 ˮƽѹ��ѡ�񡣶�̬���ԡ�
	.bChromaResample	= HI_FALSE,								//ɫ���ز���ѡ�񡣶�̬���ԡ�
	.bHighPri			= HI_FALSE,								//�����ȼ�ѡ�񡣶�̬���ԡ�
	.enViPixFormat		= PIXEL_FORMAT_YUV_SEMIPLANAR_420,		//���ظ�ʽ����̬���ԡ�
};

VO_DEV s_VoDevCtrlDflt = HD;								//Ĭ��ʱGUI�����Ƶ�����豸��
static VO_DEV s_VoDevAlarmDflt = HD;						//Ĭ�ϱ������浯����������豸��
VO_DEV s_VoSecondDev = AD;									//��ǰ�豸VO�ĵ�2����豸

#elif defined(Hi3535)
static VI_DEV_ATTR_S s_stViDevPubAttrDflt = {
	.enIntfMode	= VI_MODE_BT656,							//��Ƶ����ӿ�ģʽ����̬���ԡ�
	.enWorkMode = VI_WORK_MODE_1Multiplex,
	.au32CompMask[0] = 0xFF000000,
	.au32CompMask[1] = 0x0,
	.enScanMode = VI_SCAN_INTERLACED,
	.s32AdChnId[0] = -1,
	.s32AdChnId[1] = -1,
	.s32AdChnId[2] = -1,
	.s32AdChnId[3] = -1,
};
static VI_CHN_ATTR_S s_stViChnAttrDflt = {
	.stCapRect			= {8, 0, 704, 288},						//ͨ�������������ԡ���̬���ԡ�����Hi3520��Hi3515ý�崦����������ο�2.pdf��P3-11,3-12
	.enCapSel			= VI_CAPSEL_BOTH,						//֡��ѡ�񡣶�̬���ԡ�
	//.bChromaResample	= HI_FALSE,								//ɫ���ز���ѡ�񡣶�̬���ԡ�
	.enPixFormat		= PIXEL_FORMAT_YUV_SEMIPLANAR_420,		//���ظ�ʽ����̬���ԡ�
	.bMirror = HI_FALSE,
//	.bFilp = HI_FALSE,
//	.bChromaResample = HI_FALSE,
	.s32SrcFrameRate = 25,
	.s32DstFrameRate = 25,
};	
/*static*/ VO_DEV s_VoDevCtrlDflt = DHD0;						//Ĭ��ʱGUI�����Ƶ�����豸��
static VO_DEV s_VoDevAlarmDflt = DHD0;							//Ĭ�ϱ������浯����������豸��
VO_DEV s_VoSecondDev = DSD0;

#else
static VI_DEV_ATTR_S s_stViDevPubAttrDflt = {
	.enIntfMode	= VI_MODE_BT656,							//��Ƶ����ӿ�ģʽ����̬���ԡ�
	.enWorkMode = VI_WORK_MODE_1Multiplex,
	.au32CompMask[0] = 0xFF000000,
	.au32CompMask[1] = 0x0,
	.enScanMode = VI_SCAN_INTERLACED,
	.s32AdChnId[0] = -1,
	.s32AdChnId[1] = -1,
	.s32AdChnId[2] = -1,
	.s32AdChnId[3] = -1,
};

static VI_CHN_ATTR_S s_stViChnAttrDflt = {
	.stCapRect			= {8, 0, 704, 288},						//ͨ�������������ԡ���̬���ԡ�����Hi3520��Hi3515ý�崦����������ο�2.pdf��P3-11,3-12
	.enCapSel			= VI_CAPSEL_BOTH,						//֡��ѡ�񡣶�̬���ԡ�
	.bChromaResample	= HI_FALSE,								//ɫ���ز���ѡ�񡣶�̬���ԡ�
	.enPixFormat		= PIXEL_FORMAT_YUV_SEMIPLANAR_420,		//���ظ�ʽ����̬���ԡ�
	.bMirror = HI_FALSE,
//	.bFilp = HI_FALSE,
	.bChromaResample = HI_FALSE,
	.s32SrcFrameRate = 25,
	.s32FrameRate = 25,
};
/*static*/ VO_DEV s_VoDevCtrlDflt = DHD0;						//Ĭ��ʱGUI�����Ƶ�����豸��
static VO_DEV s_VoDevAlarmDflt = DHD0;							//Ĭ�ϱ������浯����������豸��
VO_DEV s_VoSecondDev = DSD0;									//��ǰ�豸VO�ĵ�2����豸
#endif

HI_U32 s_u32GuiWidthDflt	= 1280;								//Ĭ��GUI��
HI_U32 s_u32GuiHeightDflt	= 1024;								//Ĭ��GUI��

HI_U32 g_Max_Vo_Num=0;
UINT8	PRV_CurDecodeMode = 0;//��ǰ����ģʽ

static HI_S32 s_s32NPFlagDflt = 0;								//ϵͳĬ��N/P��ʽ���ã�0-PAL, 1-NTSC
static HI_S32 s_s32ViWorkMode = 0;//VI�ɼ�ͼ���С���ã�0-D1, 1-CIF
static HI_S32 s_s32VGAResolution = VGA_1024X768;
#if defined(SN9234H1)
static char *s_devfb[] = {"/dev/fb0", "/dev/fb1", "/dev/fb2", "/dev/fb3", "/dev/fb4"};
static int bHaveM240 = 1;
static int CurrertPciv = 9;
static int IsOSDAlarmOn[MAX_IPC_CHNNUM] = {0};//�Ƿ�������Ƶ�źű���ͼ��0:�ޣ�1:��
#else
static char *s_devfb[] = {"/dev/fb0", "/dev/fb1", "/dev/fb2", "/dev/fb3", "/dev/fb4","/dev/fb5","/dev/fb6"};
#endif
UINT8 PB_Full_id = 0;
static HI_S32 OldCtrlChn = -1;
static HI_S32 IsDispInPic = 0;//�Ƿ�����ʾ"��ʾ����"���л���Ϊ�����ֽ���ͨ�����Ʋ���Ϊ1��9��֧
static unsigned char  s_OSD_Time_type = 0;		//osdʱ���ʽ 

static VIDEO_FRAME_INFO_S s_stUserFrameInfo_P;		//����Ƶ�ź�ʱ��P��ʽ�µ�ͼƬ��Ϣ
static VIDEO_FRAME_INFO_S s_stUserFrameInfo_N;		//����Ƶ�ź�ʱ��N��ʽ�µ�ͼƬ��Ϣ

static pthread_mutex_t s_osd_mutex = PTHREAD_MUTEX_INITIALIZER;		//OSDʱ���ź���
static pthread_mutex_t s_Reset_vo_mutex = PTHREAD_MUTEX_INITIALIZER;	
sem_t sem_SendNoVideoPic, sem_VoPtsQuery, sem_PlayPause, sem_PrvGetData, sem_PrvSendData, sem_PBGetData, sem_PBSendData;
PRV_AUDIOINFO CurPlayAudioInfo;//��ǰ������Ƶ����Ƶ����
static int CurAudioChn = -1;//��ǰ������Ƶ��ͨ��
static int PreAudioChn = -1;
static int CurAudioPlayStat = 0;//��ǰ���ŵ���Ƶ״̬(0:��,1:��)
static int IsTest = 0;//�������Ա�־
static int DoubleToSingleIndex = -1;
static int LayoutToSingleChn = -1;//���ּ��̿������ֽ��л���������
static unsigned char OutPutMode = StretchMode;//��ʾģʽ:���졢�ȱ���������
static PRV_DecodeState g_DecodeState[PRV_VO_CHN_NUM];
time_t Probar_time[DEV_CHANNEL_NUM];
PRM_LINKAGE_GROUP_CFG g_PrmLinkAge_Cfg[LINKAGE_MAX_GROUPNUM];

//��ʶ�û��Ѿ�����ѡ�񲥷�ĳ��ͨ������Ƶ����ʱ������
//1�����������µ�һ·IPC��
//2�������л�(�л���ѡ�񲥷���Ƶ��ͨ���ڻ����в���)
//������ѡ�񲥷Ż����е�һ������Ƶ��ͨ������Ƶ������Ĭ�ϼ��������û�ѡ���ͨ������Ƶ
static int IsChoosePlayAudio = 0;
/* �����Ǹ�Ԥ�������趨��Լ����ֵ����PRV_PREVIEW_LAYOUT_DIVΪһ�����͸ߵĵ�λ�� */
int MccCreateingVdecCount = 0;
int MccReCreateingVdecCount = 0;

static RECT_S s_astPreviewLayout1[1] = {
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 0,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 0,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1,
	}
};/* ������ */

static RECT_S s_astPreviewLayout2[2] = {
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 0,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 2/4,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 2/4,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 2/4,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 2/4,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 2/4,
	}
};/* 2���� */
static RECT_S s_astPreviewLayout3[3] = {
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 0,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 0,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 0,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 2/3,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 0,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1,
	}
};/* 3���� */

static RECT_S s_astPreviewLayout4[4] = {
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 0,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 0,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/2,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/2,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 1/2,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 0,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/2,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/2,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 0,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 1/2,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/2,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/2,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 1/2,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 1/2,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/2,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/2,
	}
};/* 4���� */

static RECT_S s_astPreviewLayout5[5] = {
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 0,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 0,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 0,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 2/3,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 0,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 2/3,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 2/3,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 2/3,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
	}
};/* 5���� */

static RECT_S s_astPreviewLayout6[6] = {
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 0,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 0,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 2/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 2/3,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 2/3,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 0,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 2/3,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 0,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 2/3,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 2/3,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 2/3,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 2/3,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
	}
};/* 6���� */

static RECT_S s_astPreviewLayout7[7] = {
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 0,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 0,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 0,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 2/3,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 0,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 2/3,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 2/3,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 2/3,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 2/3,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
	}
};/* 7���� */


static RECT_S s_astPreviewLayout8[8] = {
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 0,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 0,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 3/4,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 3/4,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 3/4,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 0,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 3/4,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 3/4,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 2/4,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 0,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 3/4,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 3/4,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 2/4,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 3/4,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 3/4,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 3/4,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
	}
};/* 8���� */

static RECT_S s_astPreviewLayout9[9] = {
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 0/3,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 0/3,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 0/3,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 2/3,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 0/3,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 0/3,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 2/3,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 0/3,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 2/3,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 2/3,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 2/3,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 2/3,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/3,
	}
};/* 9����*/

static RECT_S s_astPreviewLayout16[16] = {
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 0/4,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 0/4,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 0/4,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 2/4,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 0/4,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 3/4,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 0/4,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 0/4,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 2/4,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 3/4,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 0/4,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 2/4,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 2/4,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 2/4,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 2/4,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 3/4,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 2/4,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 0/4,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 3/4,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 3/4,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 2/4,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 3/4,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
	},
	{
		.s32X		= PRV_PREVIEW_LAYOUT_DIV * 3/4,
		.s32Y		= PRV_PREVIEW_LAYOUT_DIV * 3/4,
		.u32Width	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
		.u32Height	= PRV_PREVIEW_LAYOUT_DIV * 1/4,
	}
};/* 16����*/


/************************ FUNCTIONS HEAR *************************/
#if defined(SN9234H1)
HI_S32 PRV_start_pciv(PCIV_CHN PcivChn);
STATIC HI_S32 PRV_PrevInitSpotVo( HI_U32 u32Index);
STATIC HI_S32 PRV_InitSpotVo(void);
HI_S32 PRV_RefreshSpotOsd(int chan);
#endif

STATIC HI_S32 PRV_Chn2Index(VO_DEV VoDev, VO_CHN VoChn, HI_U32 *pu32Index,VO_CHN *pOrder);
STATIC HI_S32 PRV_DisableAllVoChn(VO_DEV VoDev);
STATIC HI_S32 PRV_EnableAllVoChn(VO_DEV VoDev);

static HI_BOOL s_bIsSysInit = HI_FALSE;//HI_FALSE;

#if defined(SN9234H1)
/*************************************************
Function: //PRV_SetM240_Display
Description: // ����M240����������ֱ���
Calls: //
Called By: // 
Input: // Resolution		�ֱ���
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
int PRV_Init_M240(void)
{

	struct stat buf;
	int result;
	result = stat("/dev/mdin240", &buf);
	if(result != 0)
	{
		perror("Problem getting information");
		if(errno == ENOENT)
		{
			bHaveM240 = 0;
			//printf("-----------Can't find mdin241 device!\n");
			TRACE(SCI_TRACE_HIGH, MOD_PRV, "Can't find mdin241 device!\n");
		}
	}
	if(bHaveM240)
	{
		bHaveM240 = 1;
		s_astVoDevStatDflt[HD].stVoPubAttr.enIntfType 		= VO_INTF_BT1120;
		s_astVoDevStatDflt[HD].stVideoLayerAttr.enPixFormat	= PIXEL_FORMAT_YUV_SEMIPLANAR_422;	
		//printf("Detect mdin241 ok!\n");
	}	
	return 0;
}

/*************************************************
Function: //PRV_SetM240_Display
Description: // ����M240����������ֱ���
Calls: //
Called By: // 
Input: // Resolution		�ֱ���
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
int PRV_SetM240_Display(int Resolution)
{
	int fd;
	_mdin240ioctlpara_ para;

	TRACE(SCI_TRACE_NORMAL, MOD_PRV,"PRV_SetM240_Display = %d\n", Resolution);
	//printf("--------------bHaveM240: %d\n",bHaveM240);
	
	if(!bHaveM240)
		return 0;

	para.Vadj.brightness = 145;
	para.Vadj.contrast = 150;
	para.Vadj.saturation = 128;
	para.Vadj.hue = 128;
	para.Vadj.sharpness = 15;
	
	para.Vadj.r_gain = 128; 
	para.Vadj.g_gain = 128; 
	para.Vadj.b_gain = 128; 
	para.Vadj.r_offset = 128; 
	para.Vadj.g_offset = 128; 
	para.Vadj.b_offset = 128;
	switch(Resolution)
	{
		case VGA_720P:
			//para.Vres.vi = MVI_1024x768p60;
			para.Vres.vi = MVI_720p60;
			para.Vres.vo = MVO_720p60;			
			break;
		case VGA_1080P:
			//para.Vres.vi = MVI_1024x768p60;
			para.Vres.vi = MVI_720p60;
			para.Vres.vo = MVO_1080p60;			
			break;
		case VGA_1024X768:
			//para.Vres.vi = MVI_1024x768p60;
			para.Vres.vi = MVI_720p60;
			para.Vres.vo = MVO_1024x768p60;			
			break;
		case VGA_1280X1024:
			//para.Vres.vi = MVI_1024x768p60;
			para.Vres.vi = MVI_720p60;
			para.Vres.vo = MVO_1280x1024p60;	
			break;
		case VGA_800X600:
			para.Vres.vi = MVI_800x600p60;
			para.Vres.vo = MVO_1024x768p60;			
			break;
		case VGA_1366x768:
			para.Vres.vi = MVI_1440x900p60;
			para.Vres.vo = MVO_1440x900p60;			
			break;
		case VGA_1440x900:
			para.Vres.vi = MVI_1440x900p60;
			para.Vres.vo = MVO_1440x900p60;			
			break;
	}

	fd = open("/dev/mdin240", O_RDWR|O_NONBLOCK);
	if(fd < 0)
	{
		perror("open error!\n");
		//printf("--------------Open /dev/mdin240 error\n");
		return -1;
	}
	ioctl(fd, MDIN241_IOCTL_SET_RESOLUTION, &para);
	//ioctl(fd, MDIN241_IOCTL_SET_PARAMETER, &para);
	close(fd);

	return 0;
}

#endif
#if defined(Hi3531)||defined(Hi3535)
/******************************************************************************
* function : vdec group unbind vpss chn
******************************************************************************/
HI_S32 PRV_VDEC_UnBindVpss(VDEC_CHN VdChn, VPSS_GRP VpssGrp)
{
	HI_S32 s32Ret = HI_SUCCESS;
	MPP_CHN_S stSrcChn;
	MPP_CHN_S stDestChn;

	stSrcChn.enModId = HI_ID_VDEC;
	stSrcChn.s32DevId = 0;
	stSrcChn.s32ChnId = VdChn;

	stDestChn.enModId = HI_ID_VPSS;
	stDestChn.s32DevId = VpssGrp;
	stDestChn.s32ChnId = 0;
	
//	printf("-----------------------PRV_VDEC_UnBindVpss,vdechn:%d,vpssgrp:%d\n",VdChn,VpssGrp);
	s32Ret = HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
	if (s32Ret != HI_SUCCESS)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV,"HI_MPI_SYS_UnBind failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}
/******************************************************************************
* function : Set vpss system memory location
******************************************************************************/
HI_S32 PRV_VPSS_MemConfig()
{
    HI_CHAR * pcMmzName;
    MPP_CHN_S stMppChnVpss;
    HI_S32 s32Ret, i;

    /*vpss group max is 64, not need config vpss chn.*/
    for(i=0;i<64;i++)
    {
        stMppChnVpss.enModId  = HI_ID_VPSS;
        stMppChnVpss.s32DevId = i;
        stMppChnVpss.s32ChnId = 0;

        pcMmzName = NULL;  
     
        /*vpss*/
        s32Ret = HI_MPI_SYS_SetMemConf(&stMppChnVpss, pcMmzName);
        if (HI_SUCCESS != s32Ret)
        {
            TRACE(SCI_TRACE_NORMAL, MOD_PRV,"Vpss HI_MPI_SYS_SetMemConf ERR !\n");
            return HI_FAILURE;
        }
    }
    return HI_SUCCESS;
}


/******************************************************************************
* function : Set system memory location
******************************************************************************/
HI_S32 PRV_VDEC_MemConfig(HI_VOID)
{
    HI_S32 i = 0;
    HI_S32 s32Ret = HI_SUCCESS;

    HI_CHAR * pcMmzName;
    MPP_CHN_S stMppChnVDEC;

    /* VDEC chn max is 32*/
    for(i=0;i<32;i++)
    {
        stMppChnVDEC.enModId = HI_ID_VDEC;
        stMppChnVDEC.s32DevId = 0;
        stMppChnVDEC.s32ChnId = i;

        pcMmzName = NULL;  

        s32Ret = HI_MPI_SYS_SetMemConf(&stMppChnVDEC,pcMmzName);
        if (s32Ret)
        {
            TRACE(SCI_TRACE_NORMAL, MOD_PRV,"HI_MPI_SYS_SetMemConf ERR !\n");
            return HI_FAILURE;
        }
    }  

    return HI_SUCCESS;
}



/******************************************************************************
* function : Set system memory location
******************************************************************************/
HI_S32 PRV_VO_MemConfig(VO_DEV VoDev)
{
    HI_S32 s32Ret = HI_SUCCESS;
    MPP_CHN_S stMppChnVO;
	HI_CHAR * pcMmzName;
    /* config vo dev */
    stMppChnVO.enModId  = HI_ID_VOU;
    stMppChnVO.s32DevId = VoDev;
    stMppChnVO.s32ChnId = 0;
	pcMmzName = NULL; 
    s32Ret = HI_MPI_SYS_SetMemConf(&stMppChnVO, pcMmzName);
    if (s32Ret)
    {
        TRACE(SCI_TRACE_HIGH, MOD_VAM, "HI_MPI_SYS_SetMemConf ERR !\n");
        return HI_FAILURE;
    } 
    
    return HI_SUCCESS;
}

/******************************************************************************
* function : Set region memory location
******************************************************************************/
HI_S32 PRV_RGN_MemConfig(HI_VOID)
{
    HI_S32 i = 0;
    HI_S32 s32Ret = HI_SUCCESS;

    HI_CHAR * pcMmzName;
    MPP_CHN_S stMppChnRGN;

	/*the max chn of vpss,grp and venc is 64*/
    for(i=0; i<RGN_HANDLE_MAX; i++)
    {
        stMppChnRGN.enModId  = HI_ID_RGN;
        stMppChnRGN.s32DevId = 0;
        stMppChnRGN.s32ChnId = 0;

        pcMmzName = NULL;  
   
        s32Ret = HI_MPI_SYS_SetMemConf(&stMppChnRGN,pcMmzName);
        if (s32Ret)
        {
           	TRACE(SCI_TRACE_HIGH, MOD_VAM,"HI_MPI_SYS_SetMemConf ERR !\n");
            return HI_FAILURE;
        }
    }
    
    return HI_SUCCESS;
}


/******************************************************************************
* function : Set venc memory location
******************************************************************************/
HI_S32 PRV_VENC_MemConfig(HI_VOID)
{
    HI_S32 i = 0;
    HI_S32 s32Ret;

    HI_CHAR * pcMmzName;
    MPP_CHN_S stMppChnVENC;
    MPP_CHN_S stMppChnGRP;

    /* group, venc max chn is 64*/
    for(i=0;i<64;i++)
    {
        stMppChnGRP.enModId  = HI_ID_GROUP;
        stMppChnGRP.s32DevId = i;
        stMppChnGRP.s32ChnId = 0;

        stMppChnVENC.enModId = HI_ID_VENC;
        stMppChnVENC.s32DevId = 0;
        stMppChnVENC.s32ChnId = i;

        pcMmzName = NULL;  

        /*grp*/
        s32Ret = HI_MPI_SYS_SetMemConf(&stMppChnGRP,pcMmzName);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_SYS_SetMemConf failed with %#x!\n", s32Ret);
            return HI_FAILURE;
        } 

        /*venc*/
        s32Ret = HI_MPI_SYS_SetMemConf(&stMppChnVENC,pcMmzName);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_SYS_SetMemConf with %#x!\n", s32Ret);
            return HI_FAILURE;
        }
    }
    
    return HI_SUCCESS;
}
#endif


#if defined(SN9234H1)
//UI����������������ý���
int PRV_SetVo_Display(BYTE brightness, BYTE contrast, BYTE saturation, BYTE hue, BYTE sharpness)
{
#if 0
	int fd;
	_mdin240ioctlpara_ para;
	//printf("PRV_SetVo_Display: %d,%d,%d,%d,%d\n", brightness, contrast, saturation, hue, sharpness);
	para.Vadj.brightness = brightness;
	para.Vadj.contrast = contrast;
	para.Vadj.saturation = saturation;
	para.Vadj.hue = hue;
	para.Vadj.sharpness = sharpness;
	para.Vadj.r_gain = 128; 
	para.Vadj.g_gain = 128; 
	para.Vadj.b_gain = 128; 
	para.Vadj.r_offset = 128; 
	para.Vadj.g_offset = 128; 
	para.Vadj.b_offset = 128;
	fd = open("/dev/mdin240", O_RDWR|O_NONBLOCK);
	if(fd < 0)
	{
		perror("open error!\n");
		//printf("--------------Open /dev/mdin240 error\n");
		return -1;
	}
	ioctl(fd, MDIN241_IOCTL_SET_PARAMETER, &para);
	close(fd);
	//printf("end PRV_SetVo_Display");
	return 0;
#endif
	VO_CSC_S stPubCSC;
	stPubCSC.enCSCType = VO_CSC_LUMA;
	stPubCSC.u32Value = brightness;
	CHECK_RET(HI_MPI_VO_SetDevCSC(HD, &stPubCSC));

	stPubCSC.enCSCType = VO_CSC_CONTR;
	if(contrast <= 2)// 1:��ʾ�쳣��0:ȫ�ڣ����
		contrast = 2;
	stPubCSC.u32Value = contrast; 
	CHECK_RET(HI_MPI_VO_SetDevCSC(HD, &stPubCSC));

	stPubCSC.enCSCType = VO_CSC_HUE;	
	if(hue == 99)//99��ʾ�쳣�����
		hue = 100;
	stPubCSC.u32Value = hue; 
	CHECK_RET(HI_MPI_VO_SetDevCSC(HD, &stPubCSC));

	stPubCSC.enCSCType = VO_CSC_SATU;
	stPubCSC.u32Value = saturation; 
	CHECK_RET(HI_MPI_VO_SetDevCSC(HD, &stPubCSC));	
	return 0;

}

#else
//UI����������������ý���
int PRV_SetVo_Display(BYTE brightness, BYTE contrast, BYTE saturation, BYTE hue, BYTE sharpness)
{
	VO_CSC_S stPubCSC;
	stPubCSC.enCscMatrix = VO_CSC_MATRIX_IDENTITY;
	stPubCSC.u32Luma = brightness;
	stPubCSC.u32Contrast = contrast;
	stPubCSC.u32Hue = hue;
	stPubCSC.u32Satuature = saturation;
#if defined(Hi3535)
	CHECK_RET(HI_MPI_VO_SetVideoLayerCSC(VHD0, &stPubCSC));
#else
	CHECK_RET(HI_MPI_VO_SetDevCSC(DHD0, &stPubCSC));
#endif
	
	return 0;
}
#endif

int PRV_GetPlayBackState()
{
	if(s_astVoDevStatDflt[DHD0].enPreviewStat == PRV_STAT_PB)
	{
		return 1;
	}

	return OK;
}

int PRV_GetDoubleIndex()
{
	if(s_astVoDevStatDflt[DHD0].s32DoubleIndex == 1)
	{
		return 1;
	}

	return OK;
}
int PRV_GetSingleIndex()
{
	if(s_astVoDevStatDflt[DHD0].s32DoubleIndex == 1)
	{
		return s_astVoDevStatDflt[DHD0].s32SingleIndex;
	}
	else
	{
		return -1;
	}
}

int PRV_GetDoubleToSingleIndex()
{
	return DoubleToSingleIndex;
}

#if defined(Hi3535)
HI_VOID	SAMPLE_COMM_VDEC_ModCommPoolConf(VB_CONF_S *pstModVbConf, PAYLOAD_TYPE_E enType, SIZE_S *pstSize)
{
    HI_S32 PicSize=0, PmvSize=0,PicSize1=0, PmvSize1=0;
	
	memset(pstModVbConf, 0, sizeof(VB_CONF_S));    
    pstModVbConf->u32MaxPoolCnt = 8;
	
    VB_PIC_BLK_SIZE(pstSize->u32Width, pstSize->u32Height, enType, PicSize);	
    pstModVbConf->astCommPool[0].u32BlkSize = PicSize;
    pstModVbConf->astCommPool[0].u32BlkCnt  = 80;

	/* if the VDEC channel of H264 support to decode B frame, then you should  allocate PmvBuffer */
    VB_PMV_BLK_SIZE(pstSize->u32Width, pstSize->u32Height, PmvSize);
    pstModVbConf->astCommPool[1].u32BlkSize = PmvSize;
    pstModVbConf->astCommPool[1].u32BlkCnt  = 80;

	VB_PIC_BLK_SIZE(1920, 1080, enType, PicSize1);	
    pstModVbConf->astCommPool[2].u32BlkSize = PicSize1;
    pstModVbConf->astCommPool[2].u32BlkCnt  = 20;

	VB_PMV_BLK_SIZE(1920, 1080, PmvSize1);
    pstModVbConf->astCommPool[3].u32BlkSize = PmvSize1;
    pstModVbConf->astCommPool[3].u32BlkCnt  = 20;

	VB_PIC_BLK_SIZE(2048, 1536, enType, PicSize1);	
    pstModVbConf->astCommPool[4].u32BlkSize = PicSize1;
    pstModVbConf->astCommPool[4].u32BlkCnt  = 16;

	VB_PMV_BLK_SIZE(2048, 1536, PmvSize1);
    pstModVbConf->astCommPool[5].u32BlkSize = PmvSize1;
    pstModVbConf->astCommPool[5].u32BlkCnt  = 8;

	VB_PIC_BLK_SIZE(4000, 3000, enType, PicSize1);	
    pstModVbConf->astCommPool[6].u32BlkSize = PicSize1;
    pstModVbConf->astCommPool[6].u32BlkCnt  = 4;

	VB_PMV_BLK_SIZE(4000, 3000, PmvSize1);
    pstModVbConf->astCommPool[7].u32BlkSize = PmvSize1;
    pstModVbConf->astCommPool[7].u32BlkCnt  = 4;

}

HI_S32	SAMPLE_COMM_VDEC_InitModCommVb(VB_CONF_S *pstModVbConf)
{
	HI_S32 ret = HI_SUCCESS;
	ret = HI_MPI_VB_ExitModCommPool(VB_UID_VDEC);
	if(ret != HI_SUCCESS)
	{
		printf("HI_MPI_VB_ExitModCommPool error\n");
		return HI_FAILURE;
	}
	ret = HI_MPI_VB_SetModPoolConf(VB_UID_VDEC, pstModVbConf);
	if(ret != HI_SUCCESS)
	{
		printf("HI_MPI_VB_SetModPoolConf error\n");
		return HI_FAILURE;
	}
	ret = HI_MPI_VB_InitModCommPool(VB_UID_VDEC);
	if(ret != HI_SUCCESS)
	{
		printf("HI_MPI_VB_InitModCommPool error\n");
		return HI_FAILURE;
	}
    return HI_SUCCESS;
}
#endif


/*************************************************
Function: //PRV_SysInit
Description: // ��Ƶ���ʼ��
Calls: //
Called By: // Preview_Init��Ԥ����ʼ������
Input: // ��
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
int PRV_SysInit(void)
{
	CHECK(HI_MPI_SYS_Exit());
#if defined(Hi3535)
	int i = 0;
	for(i=0;i<VB_MAX_USER;i++)
	{
		 HI_MPI_VB_ExitModCommPool(i);
	}
#endif

	CHECK(HI_MPI_VB_Exit());

    CHECK_RET(HI_MPI_VB_SetConf(&s_stVbConfDflt));
	
    CHECK_RET(HI_MPI_VB_Init());

	CHECK_RET(HI_MPI_SYS_SetConf(&s_stSysConfDflt));
    
	CHECK_RET(HI_MPI_SYS_Init());
#if defined(Hi3535)
	HI_S32 s32Ret = HI_SUCCESS; 
	VB_CONF_S stVbConf;
	SIZE_S stSize;
	PAYLOAD_TYPE_E enType;
	enType = PT_H264;
	memset(&stVbConf,0,sizeof(VB_CONF_S));
	stSize.u32Width = 704;
	stSize.u32Height = 576;
	SAMPLE_COMM_VDEC_ModCommPoolConf(&stVbConf, enType, &stSize);	 
	s32Ret = SAMPLE_COMM_VDEC_InitModCommVb(&stVbConf);
	if(s32Ret != HI_SUCCESS)
	{			
		printf("init mod common vb fail for %#x!\n", s32Ret);
		return -1;;
	}
#endif	

#if defined(Hi3531)|| defined(Hi3535)
	PRV_VoInit();  //�����ɫ���	
	CHECK_RET(PRV_VPSS_MemConfig());
	CHECK_RET(PRV_VDEC_MemConfig());
	CHECK_RET(PRV_VO_MemConfig(DHD0));
	CHECK_RET(PRV_RGN_MemConfig());
#endif	
#if defined(Hi3531)
	CHECK_RET(SAMPLE_COMM_VI_MemConfig(SAMPLE_VI_MODE_16_D1));
#endif
//	CHECK_RET(PRV_VENC_MemConfig());
	s_bIsSysInit = HI_TRUE;

	RET_SUCCESS("");
}
/*************************************************
Function: //PRV_SysExit
Description: // ��Ƶ���˳�
Calls: //
Called By: //exit_mpp_sys���ػ�ʱ����
Input: // ��
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_VOID PRV_SysExit(HI_VOID)
{
	CHECK(HI_MPI_SYS_Exit());
	CHECK(HI_MPI_VB_Exit());
}

#if defined(SN9234H1)
STATIC HI_S32 PRV_SetDevCsc(VO_DEV VoDev)
{
	PRM_Vo_DISPLAY_CFG stVo_DispCfg;
	stVo_DispCfg.brightness = 48;
	stVo_DispCfg.contrast = 52;
	stVo_DispCfg.saturation = 60;
	stVo_DispCfg.hue = 42;
	
	if(IsTest == 0)
	{
		if(ERROR == GetParameter(PRM_ID_VO_DISPLAY_CFG, NULL, &stVo_DispCfg, sizeof(PRM_Vo_DISPLAY_CFG), 0, SUPER_USER_ID, NULL))
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Get PRM_ID_VO_DISPLAY_CFG Parameter Error\n");
		}
	}
	PRV_SetVo_Display(stVo_DispCfg.brightness, stVo_DispCfg.contrast, stVo_DispCfg.saturation, stVo_DispCfg.hue, 0);

//	printf("========%d, %d, %d, %d============\n", stVo_DispCfg.brightness, stVo_DispCfg.contrast, stVo_DispCfg.saturation, stVo_DispCfg.hue);
	return HI_SUCCESS;
}
#else
STATIC HI_S32 PRV_SetDevCsc(VO_DEV VoDev)
{
	PRM_Vo_DISPLAY_CFG stVo_DispCfg;
	memset(&stVo_DispCfg,0,sizeof(PRM_Vo_DISPLAY_CFG));
	stVo_DispCfg.Luma = 50;
	stVo_DispCfg.contrast = 50;
	stVo_DispCfg.saturation = 50;
	stVo_DispCfg.hue = 50;
	
	if(IsTest == 0)
	{
		if(ERROR == GetParameter(PRM_ID_VO_DISPLAY_CFG, NULL, &stVo_DispCfg, sizeof(PRM_Vo_DISPLAY_CFG), 0, SUPER_USER_ID, NULL))
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Get PRM_ID_VO_DISPLAY_CFG Parameter Error\n");
		}
	}
	PRV_SetVo_Display(stVo_DispCfg.Luma, stVo_DispCfg.contrast, stVo_DispCfg.saturation, stVo_DispCfg.hue, 0);

//	printf("========Luma:%d, contrast:%d, saturation:%d, hue:%d============\n", stVo_DispCfg.Luma, stVo_DispCfg.contrast, stVo_DispCfg.saturation, stVo_DispCfg.hue);
	return HI_SUCCESS;
}
#endif
#if defined(Hi3531)||defined(Hi3535)
int PRV_SetVga_Display(BYTE brightness, BYTE contrast, BYTE saturation, BYTE hue, BYTE Gain, BYTE sharpness)
{
	VO_VGA_PARAM_S pstVgaParam;
	if(contrast < 10)
		contrast = 10;
#if defined(Hi3535)
	pstVgaParam.stCSC.enCscMatrix = 3;
	pstVgaParam.stCSC.u32Luma = brightness;
	pstVgaParam.stCSC.u32Contrast = contrast; 
	pstVgaParam.stCSC.u32Hue = hue;
	pstVgaParam.stCSC.u32Satuature = saturation;
	pstVgaParam.u32Gain = Gain;
#else
	pstVgaParam.enCscMatrix = 3;
	pstVgaParam.u32Luma = brightness;
	pstVgaParam.u32Contrast = contrast; 
	pstVgaParam.u32Hue = hue;
	pstVgaParam.u32Satuature = saturation;
	pstVgaParam.u32Gain = Gain;
#endif
	HI_MPI_VO_SetVgaParam(DHD0,&pstVgaParam);
	return HI_SUCCESS;
}
STATIC HI_S32 PRV_SetVgaParam(VO_DEV VoDev)
{
	PRM_Vo_DISPLAY_CFG stVo_DispCfg;
	memset(&stVo_DispCfg,0,sizeof(PRM_Vo_DISPLAY_CFG));
	stVo_DispCfg.VgaLuma = 50;
	stVo_DispCfg.Vgacontrast = 50; 
	stVo_DispCfg.Vgahue = 50;
	stVo_DispCfg.Vgasaturation = 50;
	stVo_DispCfg.Gain = 10;
	
	if(IsTest == 0)
	{
		if(ERROR == GetParameter(PRM_ID_VO_DISPLAY_CFG, NULL, &stVo_DispCfg, sizeof(PRM_Vo_DISPLAY_CFG), 0, SUPER_USER_ID, NULL))
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Get PRM_ID_VO_DISPLAY_CFG Parameter Error\n");
		}
	}
	PRV_SetVga_Display(stVo_DispCfg.VgaLuma, stVo_DispCfg.Vgacontrast, stVo_DispCfg.Vgasaturation, stVo_DispCfg.Vgahue,stVo_DispCfg.Gain,0);

//	printf("========VgaLuma:%d, VgaContrast:%d, VgaSatuature:%d, VgaHue:%d,Gain:%d============\n", stVo_DispCfg.VgaLuma, stVo_DispCfg.Vgacontrast, stVo_DispCfg.Vgasaturation, stVo_DispCfg.Vgahue,stVo_DispCfg.Gain);
	return HI_SUCCESS;
}
#endif
#if defined(SN9234H1)
/*************************************************
Function: //PRV_EnableVoDev
Description: //  ���ÿ���ָ����VO�豸����������VO�ϵ�ͨ���Ĳ�����
Calls: //
Called By: //PRV_VoInit��
		//PRV_ResetVoDev��
Input: // ��
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_EnableVoDev(HI_S32 DevId)
{
	if (AD == DevId || SD == DevId)
	{
		//RET_SUCCESS("");
		s_astVoDevStatDflt[DevId].stVoPubAttr.enIntfSync = (0 == s_s32NPFlagDflt) ? VO_OUTPUT_PAL : VO_OUTPUT_NTSC;
	}
	CHECK_RET(HI_MPI_VO_SetPubAttr(DevId, &s_astVoDevStatDflt[DevId].stVoPubAttr));
	if (AD == DevId)
	{	
		VO_SCREEN_FILTER_S filter;
		HI_MPI_VO_GetScreenFilter(DevId, &filter);
//		printf("HI_MPI_VO_GetScreenFilter1 filter.enHFilter = %d, filter.enVFilter=%d\n", filter.enHFilter, filter.enVFilter);
		filter.enHFilter = VO_SCREEN_HFILTER_8M; 
		filter.enVFilter = VO_SCREEN_VFILTER_8M;

		HI_MPI_VO_GetScreenFilter(DevId, &filter);
//		printf("HI_MPI_VO_GetScreenFilter2 filter.enHFilter = %d, filter.enVFilter=%d\n", filter.enHFilter, filter.enVFilter);
		
	}
	CHECK_RET(PRV_SetDevCsc(DevId));
	
	CHECK_RET(HI_MPI_VO_Enable(DevId));

	RET_SUCCESS("");
} 

#else
/*************************************************
Function: //PRV_EnableVoDev
Description: //  ���ÿ���ָ����VO�豸����������VO�ϵ�ͨ���Ĳ�����
Calls: //
Called By: //PRV_VoInit��
		//PRV_ResetVoDev��
Input: // ��
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_EnableVoDev(HI_S32 DevId)
{
	HI_U32 s32Ret=0;
	HI_U32 u32DispBufLen = 12;
	if(DevId != DHD0)
		RET_SUCCESS();
	s32Ret = HI_MPI_VO_SetDispBufLen(DevId, u32DispBufLen);
	if (s32Ret != HI_SUCCESS)
	{
	//	printf("Set display buf len failed with error code %#x!\n", s32Ret);
		return HI_FAILURE;
	}
	if(DevId >= DSD0)
	{
		if(s_s32NPFlagDflt == VIDEO_ENCODING_MODE_PAL)
			s_astVoDevStatDflt[DevId].stVoPubAttr.enIntfSync = VO_OUTPUT_PAL;
		else
			s_astVoDevStatDflt[DevId].stVoPubAttr.enIntfSync = VO_OUTPUT_NTSC;

	}
	CHECK_RET(HI_MPI_VO_SetPubAttr(DevId, &s_astVoDevStatDflt[DevId].stVoPubAttr));
	
	CHECK_RET(HI_MPI_VO_Enable(DevId));
#if defined(Hi3531)	
	HI_U32 u32Toleration = 200;
	s32Ret = HI_MPI_VO_SetPlayToleration (DevId, u32Toleration);
	if (s32Ret != HI_SUCCESS)
	{
	//	printf("Set play toleration failed with error code %#x!\n", s32Ret);
		return HI_FAILURE;
	}
#endif
	CHECK_RET(PRV_SetDevCsc(DevId));

	RET_SUCCESS("");
} 
#endif

/*************************************************
Function: //PRV_DisableVoDev
Description: //  �ر�ָ����VO�豸����������VO�豸�ϵ�ͨ���Ĳ�����
   			���ȵ���PRV_DisableVideoLayer()�ر�VO�ϵ���Ƶ�㡣
Calls: PRV_DisableVideoLayer
Called By: //PRV_VoInit��
		//PRV_ResetVoDev��
Input: // VO�豸ID
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_DisableVoDev(HI_S32 DevId)
{
    CHECK_RET(HI_MPI_VO_Disable(DevId));

	RET_SUCCESS("");
} 

/*************************************************
Function: //PRV_EnableViDev
Description: //   ���ò�����ָ����VI�豸����������VI�ϵ�ͨ���Ĳ�������ȷ��VI���Ѿ��رյģ�
   �������ʧ�ܣ���Ҳû��Ҫ������VI��
Calls: 
Called By: //
Input: // VI�豸ID
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_EnableViDev(VI_DEV ViDev)
{
#if defined(SN9234H1)
	CHECK_RET(HI_MPI_VI_SetPubAttr(ViDev, &s_stViDevPubAttrDflt));
    CHECK_RET(HI_MPI_VI_Enable(ViDev));
#else
	CHECK_RET(HI_MPI_VI_SetDevAttr(ViDev, &s_stViDevPubAttrDflt));
    CHECK_RET(HI_MPI_VI_EnableDev(ViDev));
#endif
	RET_SUCCESS("");
}

/************************************************************************/
/* ��������VI�豸��
                                                                     */
/************************************************************************/
#if 0
STATIC HI_S32 PRV_EnableAllViDev(HI_VOID)
{
	HI_S32 i;

	for (i = 0; i<PRV_VI_DEV_NUM)
	{
		PRV_EnableViDev(i);
	}

	RET_SUCCESS("");
}
#endif
/*************************************************
Function: //PRV_DisableViDev
Description: //�ر�ָ����VI�豸����������VI�ϵ�ͨ���Ĳ�������ȷ��VI�ϵ�ͨ�����Ѿ��ر�
Calls: //
Called By: // Prv_ViInit��Ԥ����ʼ��VI����
Input: //  VI�豸ID
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/ 
STATIC HI_S32 PRV_DisableViDev(VI_DEV ViDev)
{
#if defined(SN9234H1)
    CHECK_RET(HI_MPI_VI_Disable(ViDev));
#else
    CHECK_RET(HI_MPI_VI_DisableDev(ViDev));
#endif
	RET_SUCCESS("");
}

/************************************************************************/
/* �ر�����VI�豸��
                                                                     */
/************************************************************************/
#if 0
STATIC HI_S32 PRV_DisableAllViDev(HI_VOID)
{
	HI_S32 i;

	for (i = 0; i<PRV_VI_DEV_NUM)
	{
		PRV_DisableViDev(i);
	}

	RET_SUCCESS("");
}
#endif


/*************************************************
Function: //PRV_ResetVideoLayer
Description: //��������image��εĴ�С��������ʱ������imagesize��VI����һ��
�����ʾ������
Calls: //
Called By: // Prv_VoInit��Ԥ����ʼ��VI����
Input: //  VO�豸ID
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/ 
HI_S32 PRV_ResetVideoLayer(VO_DEV VoDev)
{
	//if(VoDev >= PRV_VO_MAX_DEV)
#if defined(SN9234H1)
	if(VoDev != HD)
#else
	if(VoDev > DHD0)
#endif		
	{
		RET_SUCCESS("");
	}
	PRV_DisableAllVoChn(VoDev);
	CHECK_RET(HI_MPI_VO_DisableVideoLayer(VoDev));
#if defined(SN9234H1)
	s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width = PRV_BT1120_SIZE_W;
	s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height = PRV_BT1120_SIZE_H;	
	s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width = PRV_BT1120_SIZE_W;
	s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height = PRV_BT1120_SIZE_H;
	s_astVoDevStatDflt[VoDev].stVideoLayerAttr.u32DispFrmRt = (0 == s_s32NPFlagDflt) ? 25 : 30;
#else
	s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.s32X = 0;
	s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.s32Y = 0;
	switch(VoDev)
	{
		case DHD0:
		{
			switch(s_astVoDevStatDflt[VoDev].stVoPubAttr.enIntfSync)
			{
				case VO_OUTPUT_1080P50:
				case VO_OUTPUT_1080P60:
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width = 1920;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height = 1080;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width = 1920;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height = 1080;	
					break;
				case VO_OUTPUT_720P50:
				case VO_OUTPUT_720P60:
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width = 1280;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height = 720;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width = 1280;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height = 720;	
					break;
				case VO_OUTPUT_1024x768_60:
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width = 1024;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height = 768;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width = 1024;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height = 768;	
					break;
				
				case VO_OUTPUT_1280x1024_60:
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width = 1280;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height = 1024;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width = 1280;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height = 1024;	
					break;
				case VO_OUTPUT_800x600_60:
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width = 800;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height = 600;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width = 800;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height = 600;	
					break;
				case VO_OUTPUT_1366x768_60:
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width = 1366;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height = 768;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width = 1366;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height = 768;	
					break;
				case VO_OUTPUT_1440x900_60:
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width = 1440;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height = 900;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width = 1440;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height = 900;	
					break;
				default:
					RET_FAILURE("Unsupport VGA Resolution");
			}
			break;
		}
		case DSD0:
#if defined(Hi3531)
		case DSD1:
#endif
		{
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.s32X = 30;
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.s32Y = 16;
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width = 720;
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height = (s_s32NPFlagDflt == VIDEO_ENCODING_MODE_PAL) ? 576 : 480;
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width -= 2*s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.s32X;
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height -= 2*s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.s32Y;
		
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width;
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height;
			break;
		}
	}
	
	s_astVoDevStatDflt[VoDev].stVideoLayerAttr.u32DispFrmRt = (VIDEO_ENCODING_MODE_PAL == s_s32NPFlagDflt) ? 25 : 30;
#endif
	CHECK_RET(HI_MPI_VO_SetVideoLayerAttr(VoDev, &s_astVoDevStatDflt[VoDev].stVideoLayerAttr));
	CHECK_RET(HI_MPI_VO_EnableVideoLayer(VoDev));
	PRV_EnableAllVoChn(VoDev);
#if defined(Hi3531)||defined(Hi3535)	
	CHECK_RET(PRV_SetDevCsc(VoDev));	
	CHECK_RET(PRV_SetVgaParam(VoDev));
#endif	
	RET_SUCCESS("");
}

#if defined(Hi3531)||defined(Hi3535)
HI_S32 PRV_EnablePipLayer(VO_DEV VoDev)
{
#if defined(Hi3535)
	HI_S32 s32Ret = 0;
	HI_U32 u32Priority = 1;

	VO_VIDEO_LAYER_ATTR_S stPipLayerAttr;
	s32Ret = HI_MPI_VO_BindVideoLayer(PIP,VoDev);
	if (HI_SUCCESS != s32Ret)
	{
		printf("HI_MPI_VO_PipLayerBindDev failed with %#x!\n", s32Ret);
		return -1;
	}
	s32Ret = HI_MPI_VO_GetVideoLayerAttr(PIP,&stPipLayerAttr);
	if (s32Ret != HI_SUCCESS)
	{
		printf("Get pip video layer attributes failed with errno %#x!\n", s32Ret);
		return -1;
	}
	stPipLayerAttr.stDispRect.s32X = 0;
	stPipLayerAttr.stDispRect.s32Y = 0;
	stPipLayerAttr.stDispRect.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height;
	stPipLayerAttr.stDispRect.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width;
	stPipLayerAttr.stImageSize.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height;
	stPipLayerAttr.stImageSize.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width;
	stPipLayerAttr.u32DispFrmRt = 25;
	stPipLayerAttr.enPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
	s32Ret = HI_MPI_VO_SetVideoLayerAttr(PIP,&stPipLayerAttr);
	if (HI_SUCCESS != s32Ret)
	{
		printf("HI_MPI_VO_SetPipLayerAttr failed with %#x!\n", s32Ret);
		return -1;
	}
	s32Ret = HI_MPI_VO_SetVideoLayerPriority(PIP, u32Priority);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VO_SetVideoLayerPriority failed with errno %#x!\n", s32Ret);
		return -1;
	}
	s32Ret = HI_MPI_VO_EnableVideoLayer(PIP);
	if (HI_SUCCESS != s32Ret)
	{
		printf("HI_MPI_VO_EnablePipLayer failed with %#x!\n", s32Ret);
		return -1;
	}
	return 0;
#else
	HI_S32 s32Ret = 0;
	VO_VIDEO_LAYER_ATTR_S stPipLayerAttr;
	s32Ret = HI_MPI_VO_PipLayerBindDev(VoDev);
    if (HI_SUCCESS != s32Ret)
    {
        TRACE(SCI_TRACE_NORMAL, MOD_PRV, "HI_MPI_VO_PipLayerBindDev failed with %#x!\n", s32Ret);
        return -1;
    }
	s32Ret = HI_MPI_VO_GetPipLayerAttr(&stPipLayerAttr);
	if (s32Ret != HI_SUCCESS)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Get pip video layer attributes failed with errno %#x!\n", s32Ret);
		return -1;
	}
 	stPipLayerAttr.stDispRect.s32X = 0;
    stPipLayerAttr.stDispRect.s32Y = 0;
	stPipLayerAttr.stDispRect.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height;
	stPipLayerAttr.stDispRect.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width;
    stPipLayerAttr.stImageSize.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height;
    stPipLayerAttr.stImageSize.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width;
	stPipLayerAttr.u32DispFrmRt = 25;
    stPipLayerAttr.enPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    s32Ret = HI_MPI_VO_SetPipLayerAttr(&stPipLayerAttr);
    if (HI_SUCCESS != s32Ret)
    {
        TRACE(SCI_TRACE_NORMAL, MOD_PRV, "HI_MPI_VO_SetPipLayerAttr failed with %#x!\n", s32Ret);
        return -1;
    }
    
    s32Ret = HI_MPI_VO_EnablePipLayer();
    if (HI_SUCCESS != s32Ret)
    {
        TRACE(SCI_TRACE_NORMAL, MOD_PRV, "HI_MPI_VO_EnablePipLayer failed with %#x!\n", s32Ret);
        return -1;
    }
	return 0;
#endif
}
#endif
/*************************************************
Function: //PRV_EnableVideoLayer
Description: //���ò�����ָ��VO�豸�ϵ���Ƶ�㡣
Calls: //
Called By: // Prv_VoInit��Ԥ����ʼ��VI����
Input: //  VO�豸ID
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/ 
#if defined(SN6104) || defined(SN8604D) || defined(SN8604M) || defined(SN6108) || defined(SN8608D) || defined(SN8608M)
STATIC HI_S32 PRV_EnableVideoLayer(VO_DEV VoDev)
{
	
	//printf("defined SN6108, SN8608D......\n");
	//printf("s_astVoDevStatDflt[VoDev].stVoPubAttr.enIntfSync: %d\n", s_astVoDevStatDflt[VoDev].stVoPubAttr.enIntfSync);
	switch(VoDev)
	{
	case HD:
		{
			switch(s_astVoDevStatDflt[VoDev].stVoPubAttr.enIntfSync)
			{
				case VO_OUTPUT_1024x768_60:
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width = 1024;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height = 768;
					break;
				case VO_OUTPUT_1280x1024_60:
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width = 1280;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height = 1024;
					break;
				case VO_OUTPUT_800x600_60:
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width = 800;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height = 600;
					break;
				case VO_OUTPUT_1440x900_60:
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width = 1440;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height = 900;
					break;
				case VO_OUTPUT_1366x768_60:
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width = 1366;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height = 768;
					break;
				default:
					RET_FAILURE("unknown enIntfSync of HD!!");
			}
#if defined(SN8604D) || defined(SN8604M) || defined(SN8608D) || defined(SN8608M)
			//NVRϵ��HD�豸��ͼ��ֱ�������ʾ�ֱ�����Ϊһ��
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width;
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height;
#else
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width = 720;//s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width;
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height = (0 == s_s32NPFlagDflt) ? 576 : 480;
#endif
			//printf("#############PRV_EnableVideoLayer s_s32NPFlagDflt = %d ,stImageSize h  =%d ,######################\n",s_s32NPFlagDflt,s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height);
		}
		break;
	case AD:
	case SD:
		return;
		s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.s32X = 30;
		s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.s32Y = 16;
		s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width = 720;
		s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height = (0 == s_s32NPFlagDflt) ? 576 : 480;
		s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width -= 2*s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.s32X;
		s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height -= 2*s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.s32Y;

		s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width;
		s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height;
		break;
	default:
		RET_FAILURE("bad  VoDev!!");
	}

	s_astVoDevStatDflt[VoDev].stVideoLayerAttr.u32DispFrmRt = (0 == s_s32NPFlagDflt) ? 25 : 30;

    CHECK_RET(HI_MPI_VO_SetVideoLayerAttr(VoDev, &s_astVoDevStatDflt[VoDev].stVideoLayerAttr));

	CHECK_RET(HI_MPI_VO_EnableVideoLayer(VoDev));

	//printf("width : %d, height : %d\n", s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width, s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height);
	RET_SUCCESS("");
}

#else
STATIC HI_S32 PRV_EnableVideoLayer(VO_DEV VoDev)
{
#if defined(SN9234H1)
	int Resolution  = s_s32VGAResolution;
	
	switch(VoDev)
	{
		case HD:
	 /*	     VO_CSC_S stPubCSC;
	            stPubCSC.enCSCType = VO_CSC_LUMA;
	            stPubCSC.u32Value = 50;
	            CHECK_RET(HI_MPI_VO_SetDevCSC(HD,&stPubCSC));
 
	            stPubCSC.enCSCType = VO_CSC_CONTR;
	            stPubCSC.u32Value = 50;
	            CHECK_RET(HI_MPI_VO_SetDevCSC(HD,&stPubCSC));

	            stPubCSC.enCSCType = VO_CSC_HUE;
	            stPubCSC.u32Value = 50;
	            CHECK_RET(HI_MPI_VO_SetDevCSC(HD,&stPubCSC));

	            stPubCSC.enCSCType = VO_CSC_SATU;
	            stPubCSC.u32Value = 50;
	            CHECK_RET(HI_MPI_VO_SetDevCSC(HD,&stPubCSC));
	*/
#if defined(SN8608D_LE) || defined(SN8608M_LE) || defined(SN8616D_LE) || defined(SN8616M_LE) || defined(SN9234H1)
			//NVRϵ��ͼ��ֱ�������ʾ�ֱ�����Ϊһ��
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width = PRV_BT1120_SIZE_W;
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height = PRV_BT1120_SIZE_H;		 
#else	
			if(s_astVoDevStatDflt[VoDev].bIsSingle)
			{
				s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width = PRV_SINGLE_SCREEN_W;
				s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height = PRV_SINGLE_SCREEN_H;		
			}
			else
			{
				s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width = PRV_BT1120_SIZE_W;
				s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height = PRV_BT1120_SIZE_H;		
			}
#endif
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width = PRV_BT1120_SIZE_W;
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height = PRV_BT1120_SIZE_H;
		
			break;
		case AD:
		case SD:
			RET_SUCCESS("");
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.s32X = PRV_CVBS_EDGE_CUT_W;
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.s32Y = PRV_CVBS_EDGE_CUT_H;
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width = 720;
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height = (0 == s_s32NPFlagDflt) ? 576 : 480;
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width -= 2*s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.s32X;
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height -= 2*s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.s32Y;

			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width;
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height;
			break;
		default:
			RET_FAILURE("bad  VoDev!!");
	}

	if(VoDev == HD)
	{
		PRV_SetM240_Display(Resolution);
	}

#else
	s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.s32X = 0;
	s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.s32Y = 0;
	
	switch(VoDev)
	{
		case DHD0:
		{
			switch(s_astVoDevStatDflt[VoDev].stVoPubAttr.enIntfSync)
			{
				case VO_OUTPUT_1080P50:
				case VO_OUTPUT_1080P60:
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width = 1920;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height = 1080;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width = 1920;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height = 1080;	
					break;
				case VO_OUTPUT_720P50:
				case VO_OUTPUT_720P60:
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width = 1280;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height = 720;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width = 1280;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height = 720;	
					break;
				case VO_OUTPUT_1024x768_60:
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width = 1024;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height = 768;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width = 1024;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height = 768;	
					break;
				
				case VO_OUTPUT_1280x1024_60:
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width = 1280;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height = 1024;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width = 1280;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height = 1024;	
					break;
				case VO_OUTPUT_800x600_60:
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width = 800;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height = 600;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width = 800;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height = 600;	
					break;
				case VO_OUTPUT_1366x768_60:
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width = 1366;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height = 768;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width = 1366;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height = 768;	
					break;
				case VO_OUTPUT_1440x900_60:
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width = 1440;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height = 900;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width = 1440;
					s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height = 900;	
					break;
				default:
					RET_FAILURE("Unsupport VGA Resolution");
			}
			break;
		}
		case DSD0:
#if defined(Hi3531)
		case DSD1:
#endif
		{
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.s32X = 30;
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.s32Y = 16;
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width = 720;
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height = (0 == s_s32NPFlagDflt) ? 576 : 480;
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width -= 2*s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.s32X;
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height -= 2*s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.s32Y;
		
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width;
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height;
			break;
		}
		default:
			RET_FAILURE("bad  VoDev!!");
	}
#endif	
#if defined(SN6108HE) || defined(SN6108LE) || defined(SN6116HE) || defined(SN6116LE)
	s_astVoDevStatDflt[VoDev].stVideoLayerAttr.u32DispFrmRt = (0 == s_s32NPFlagDflt) ? 25 : 30;
#else
	s_astVoDevStatDflt[VoDev].stVideoLayerAttr.u32DispFrmRt = 25;
#endif

	TRACE(SCI_TRACE_NORMAL, MOD_PRV,"PRV_EnableVideoLayer: vo=%d, s_astVoDevStatDflt[VoDev].stVoPubAttr.enIntfSync=%d, (w=%d, h=%d, w=%d, h=%d)\n", VoDev, s_astVoDevStatDflt[VoDev].stVoPubAttr.enIntfSync, 
		s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width,
		s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height,
		s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width,
		s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height);
#if defined(Hi3531)||defined(Hi3535)
	s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.s32X = 0;
	s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.s32Y = 0;
	s_astVoDevStatDflt[VoDev].stVideoLayerAttr.enPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
#endif	
    CHECK_RET(HI_MPI_VO_SetVideoLayerAttr(VoDev, &s_astVoDevStatDflt[VoDev].stVideoLayerAttr));

	CHECK_RET(HI_MPI_VO_EnableVideoLayer(VoDev));
	
#ifdef PIP_VIDEOLAYER
	/*******************PIP*************************************/
	PRV_EnablePipLayer(DHD0);
#endif
#if defined(Hi3535)
	HI_U32 u32Toleration = 200;
	HI_U32 s32Ret = 0;

	s32Ret = HI_MPI_VO_SetPlayToleration (VoDev, u32Toleration);
	if (s32Ret != HI_SUCCESS)
	{
		printf("Set play toleration failed with error code %#x!\n", s32Ret);
		return HI_FAILURE;
	}
#endif

/******************************************************************************/
#if defined(Hi3531)||defined(Hi3535)
	CHECK_RET(PRV_SetDevCsc(VoDev));
#endif
	RET_SUCCESS("");
}
#endif

/*************************************************
Function: //PRV_DisableVideoLayer
Description: //�ر�ָ��VO�豸�ϵ���Ƶ�㡣���ȵ���HI_MPI_VO_DisableChn()�ر�VO�ϵ�����ͨ����
Calls: 
Called By: //
Input: // VO�豸ID
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
HI_S32 PRV_DisableVideoLayer(VO_DEV VoDev)
{
	CHECK_RET(HI_MPI_VO_DisableVideoLayer(VoDev));

#ifdef PIP_VIDEOLAYER
#if defined(Hi3535)
	CHECK_RET(HI_MPI_VO_DisableVideoLayer(PIP));
#else
	CHECK_RET(HI_MPI_VO_DisablePipLayer());
//	CHECK_RET(HI_MPI_VO_PipLayerUnBindDev(VoDev));
#endif
#endif
	RET_SUCCESS("");
}


/*************************************************
Function: //PRV_EnableAllViChn
Description: //���ò�����ָ��VI�豸�ϵ�����ͨ����
Calls: 
Called By: //
Input: // VI�豸ID
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_EnableAllViChn(VI_DEV ViDev)
{
#if defined(SN9234H1)
	HI_S32 i;
	HI_U32 u32SrcFrmRate;

	u32SrcFrmRate = (VIDEO_ENCODING_MODE_PAL == s_stViDevPubAttrDflt.enViNorm) ? 25: 30;
	s_stViChnAttrDflt.stCapRect.u32Height = (0 == s_s32NPFlagDflt) ? 288 : 240;
	s_stViChnAttrDflt.enCapSel = (0 == s_s32ViWorkMode) ? VI_CAPSEL_BOTH : VI_CAPSEL_BOTTOM;
	s_stViChnAttrDflt.bDownScale = (0 == s_s32ViWorkMode) ? HI_FALSE : HI_TRUE;

	for (i = 0; i < PRV_VI_CHN_NUM; i++)
	{
		//printf("###########ViChn = %d ,ViDev = %d######################\n",i,ViDev);
		CHECK_RET(HI_MPI_VI_SetChnAttr(ViDev, i, &s_stViChnAttrDflt));
		CHECK_RET(HI_MPI_VI_EnableChn(ViDev, i));
		CHECK_RET(HI_MPI_VI_SetSrcFrameRate(ViDev, i, u32SrcFrmRate));
		CHECK_RET(HI_MPI_VI_SetFrameRate(ViDev, i, u32SrcFrmRate));
	}

#else
	HI_S32 i=0;
//	HI_U32 u32SrcFrmRate;

//	u32SrcFrmRate = (VIDEO_ENCODING_MODE_PAL == s_stViDevPubAttrDflt.enViNorm) ? 25: 30;
	s_stViChnAttrDflt.stCapRect.u32Height = (0 == s_s32NPFlagDflt) ? 288 : 240;
	s_stViChnAttrDflt.enCapSel = (0 == s_s32ViWorkMode) ? VI_CAPSEL_BOTH : VI_CAPSEL_BOTTOM;
//	s_stViChnAttrDflt.bDownScale = (0 == s_s32ViWorkMode) ? HI_FALSE : HI_TRUE;

	for (i = 0; i < PRV_VI_CHN_NUM; i++)
	{
		//printf("###########ViChn = %d ,ViDev = %d######################\n",i,ViDev);
		CHECK_RET(HI_MPI_VI_SetChnAttr(i, &s_stViChnAttrDflt));
		CHECK_RET(HI_MPI_VI_EnableChn(i));
	//	CHECK_RET(HI_MPI_VI_SetSrcFrameRate(ViDev, i, u32SrcFrmRate));
	//	CHECK_RET(HI_MPI_VI_SetFrameRate(ViDev, i, u32SrcFrmRate));
	}
#endif
	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_DisableAllViChn
Description: // �ر�ָ��VI�豸�ϵ�����ͨ����
Calls: 
Called By: //
Input: // VI�豸ID
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_DisableAllViChn(VI_DEV ViDev)
{
	HI_S32 i=0;

	for (i = 0; i < PRV_VI_CHN_NUM; i++)
	{
#if defined(SN9234H1)		
		CHECK_RET(HI_MPI_VI_DisableChn(ViDev, i));
#else
		CHECK_RET(HI_MPI_VI_DisableChn(i));
#endif
	}

	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_ReadFrame
Description: // ����Ƶ�ź�ͼƬ����   
Calls: 
Called By: //
Input: // fp: ����Ƶ�ź�ͼƬ�ļ�·��
		pY: Y������ʼ��ַ
		pU:U������ʼ��ַ
		pV:V������ʼ��ַ
		width:ͼƬ���
		height:ͼƬ�߶�
		stride:ͼƬY�������
		stride2:ͼƬU\V�������
Output: // ��
Return: //
Others: // ����˵��
************************************************************************/
static HI_VOID PRV_ReadFrame(FILE * fp, HI_U8 * pY, HI_U8 * pU, HI_U8 * pV,
                                              HI_U32 width, HI_U32 height, HI_U32 stride, HI_U32 stride2)
{
    HI_U8 * pDst;

    HI_U32 u32Row;

    pDst = pY;
    for ( u32Row = 0; u32Row < height; u32Row++ )
    {
        fread( pDst, width, 1, fp );
        pDst += stride;
    }
    
    pDst = pU;
    for ( u32Row = 0; u32Row < height/2; u32Row++ )
    {
        fread( pDst, width/2, 1, fp );
        pDst += stride2;
    }
    
    pDst = pV;
    for ( u32Row = 0; u32Row < height/2; u32Row++ )
    {
        fread( pDst, width/2, 1, fp );
        pDst += stride2;
    }
}

/*************************************************
Function: //PRV_PlanToSemi
Description: // convert planar YUV420 to sem-planar YUV420  
Calls: 
Called By: //
Input: // pY: Y������ʼ��ַ
		pU:U������ʼ��ַ
		pV:V������ʼ��ַ
		picWidth:ͼƬ���
		picHeight:ͼƬ�߶�
		yStride:ͼƬY�������
		vStride:ͼƬV�������
		uStride:ͼƬU�������
Output: // ��
Return: //
Others: // ����˵��
************************************************************************/
static HI_S32 PRV_PlanToSemi(HI_U8 *pY, HI_S32 yStride, 
                       HI_U8 *pU, HI_S32 uStride,
                       HI_U8 *pV, HI_S32 vStride, 
                       HI_S32 picWidth, HI_S32 picHeight)
{
    HI_S32 i;
    HI_U8* pTmpU, *ptu;
    HI_U8* pTmpV, *ptv;
    HI_S32 s32HafW = uStride >>1 ;
    HI_S32 s32HafH = picHeight >>1 ;
    HI_S32 s32Size = s32HafW*s32HafH;
        
	if (NULL == pY || NULL == pU || NULL == pV)
	{
		RET_FAILURE("paramter NULL prt !!!");
	}
	
	pTmpU = SN_MALLOC( s32Size ); 
    if (NULL == pTmpU)
    {
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, TEXT_COLOR_RED("SN_MALLOC failed! pTmpU=%#x\n"), (int)pTmpU);
		return HI_FAILURE;
    }
	ptu = pTmpU;
	
	pTmpV = SN_MALLOC( s32Size ); 
    if (NULL == pTmpV)
    {
		SN_FREE(pTmpU);
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, TEXT_COLOR_RED("SN_MALLOC failed! pTmpV=%#x\n"), (int)pTmpV);
		return HI_FAILURE;
    }
	ptv = pTmpV;

    SN_MEMCPY(pTmpU, s32Size, pU, s32Size, s32Size);
    SN_MEMCPY(pTmpV, s32Size, pV, s32Size, s32Size);
    
    for(i = 0;i<s32Size>>1;i++)
    {
        *pU++ = *pTmpV++;
        *pU++ = *pTmpU++;
        
    }
    for(i = 0;i<s32Size>>1;i++)
    {
        *pV++ = *pTmpV++;
        *pV++ = *pTmpU++;        
    }

    SN_FREE( ptu );
    SN_FREE( ptv );

    return HI_SUCCESS;
}

/*************************************************
Function: //PRV_GetVFrame_FromYUV
Description: // ��YUV�ļ���ȡ��Ƶ֡��Ϣ (ע��ֻ֧��planar 420)
Calls: 
Called By: //
Input: // pszYuvFile: ����Ƶ�ź�ͼƬ�ļ�·��
		u32Width:ͼƬ���
		u32Height:ͼƬ�߶�
		u32Stride:ͼƬ�п��
		pstVFrameInfo:����ͼƬ֡��Ϣ
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
/*static*/ HI_S32 PRV_GetVFrame_FromYUV(HI_CHAR *pszYuvFile, HI_U32 u32Width, HI_U32 u32Height,HI_U32 u32Stride, VIDEO_FRAME_INFO_S *pstVFrameInfo)
{
    HI_U32             u32LStride;
    HI_U32             u32CStride;
    HI_U32             u32LumaSize;
    HI_U32             u32ChrmSize;
    HI_U32             u32Size;
    VB_BLK VbBlk;
    HI_U32 u32PhyAddr;
    HI_U8 *pVirAddr;

	/* ��YUV�ļ� */
	FILE *pYUVFile;
	pYUVFile = fopen(pszYuvFile, "rb");
	if (!pYUVFile)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, TEXT_COLOR_RED("open yvu file ") TEXT_COLOR_YELLOW("%s") TEXT_COLOR_RED(" fail!\n"), pszYuvFile);
		RET_FAILURE("");
	}
	else
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, TEXT_COLOR_GREEN("open yuv file ") TEXT_COLOR_YELLOW("%s") TEXT_COLOR_GREEN(" success!\n"), pszYuvFile);
	}

    u32LStride  = u32Stride;
    u32CStride  = u32Stride;
    
    u32LumaSize = (u32LStride * u32Height);
    u32ChrmSize = (u32CStride * u32Height) >> 2;/* YUV 420 */
    u32Size = u32LumaSize + (u32ChrmSize << 1);

    /* alloc video buffer block ---------------------------------------------------------- */
#if defined(SN9234H1)
    VbBlk = HI_MPI_VB_GetBlock(VB_INVALID_POOLID, u32Size);
#else
    VbBlk = HI_MPI_VB_GetBlock(VB_INVALID_POOLID, u32Size,NULL);
#endif
    if (VB_INVALID_HANDLE == VbBlk)
    {
        TRACE(SCI_TRACE_NORMAL, MOD_PRV, "HI_MPI_VB_GetBlock err! size:%d\n",u32Size);
		fclose(pYUVFile);
        return -1;
    }
    u32PhyAddr = HI_MPI_VB_Handle2PhysAddr(VbBlk);
    if (0 == u32PhyAddr)
    {
		fclose(pYUVFile);
        return -1;
    }
    pVirAddr = (HI_U8 *) HI_MPI_SYS_Mmap(u32PhyAddr, u32Size);
    if (NULL == pVirAddr)
    {
		fclose(pYUVFile);
        return -1;
    }

    pstVFrameInfo->u32PoolId = HI_MPI_VB_Handle2PoolId(VbBlk);
    if (VB_INVALID_POOLID == pstVFrameInfo->u32PoolId)
    {
		fclose(pYUVFile);
        return -1;
    }
    TRACE(SCI_TRACE_NORMAL, MOD_PRV, "pool id :%d, phyAddr:%x,virAddr:%x\n" ,pstVFrameInfo->u32PoolId,u32PhyAddr,(int)pVirAddr);
    
    pstVFrameInfo->stVFrame.u32PhyAddr[0] = u32PhyAddr;
    pstVFrameInfo->stVFrame.u32PhyAddr[1] = pstVFrameInfo->stVFrame.u32PhyAddr[0] + u32LumaSize;
    pstVFrameInfo->stVFrame.u32PhyAddr[2] = pstVFrameInfo->stVFrame.u32PhyAddr[1] + u32ChrmSize;

    pstVFrameInfo->stVFrame.pVirAddr[0] = pVirAddr;
    pstVFrameInfo->stVFrame.pVirAddr[1] = pstVFrameInfo->stVFrame.pVirAddr[0] + u32LumaSize;
    pstVFrameInfo->stVFrame.pVirAddr[2] = pstVFrameInfo->stVFrame.pVirAddr[1] + u32ChrmSize;

    pstVFrameInfo->stVFrame.u32Width  = u32Width;
    pstVFrameInfo->stVFrame.u32Height = u32Height;
    pstVFrameInfo->stVFrame.u32Stride[0] = u32LStride;
    pstVFrameInfo->stVFrame.u32Stride[1] = u32CStride;
    pstVFrameInfo->stVFrame.u32Stride[2] = u32CStride;  
    pstVFrameInfo->stVFrame.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;        
    pstVFrameInfo->stVFrame.u32Field = VIDEO_FIELD_FRAME;  

    /* read Y U V data from file to the addr ----------------------------------------------*/
    PRV_ReadFrame(pYUVFile, pstVFrameInfo->stVFrame.pVirAddr[0], 
                               pstVFrameInfo->stVFrame.pVirAddr[1], pstVFrameInfo->stVFrame.pVirAddr[2],
                               pstVFrameInfo->stVFrame.u32Width, pstVFrameInfo->stVFrame.u32Height, 
                               pstVFrameInfo->stVFrame.u32Stride[0], pstVFrameInfo->stVFrame.u32Stride[1] >> 1 );

    /* convert planar YUV420 to sem-planar YUV420 -----------------------------------------*/
    PRV_PlanToSemi(pstVFrameInfo->stVFrame.pVirAddr[0], pstVFrameInfo->stVFrame.u32Stride[0],
                pstVFrameInfo->stVFrame.pVirAddr[1], pstVFrameInfo->stVFrame.u32Stride[1],
                pstVFrameInfo->stVFrame.pVirAddr[2], pstVFrameInfo->stVFrame.u32Stride[1],
                pstVFrameInfo->stVFrame.u32Width, pstVFrameInfo->stVFrame.u32Height);
    
    HI_MPI_SYS_Munmap((HI_VOID*)u32PhyAddr, u32Size);
	fclose(pYUVFile);
    return 0;
}
/*************************************************
Function: //PRV_EnableAllVoChn
Description: // ���ò�����ָ��VO�豸�ϵ�����ͨ����
Calls: 
Called By: //
Input: // // VO�豸ID
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_EnableAllVoChn(VO_DEV VoDev)
{
#if defined(SN9234H1)
	HI_S32 i;

	//for (i=0; i<g_Max_Vo_Num; i++)
	for (i = 0; i < DEV_CHANNEL_NUM; i++)	
	{
		CHECK_RET(HI_MPI_VO_SetChnAttr(VoDev, i, &s_stVoChnAttrDflt));
		//CHECK_RET(HI_MPI_VO_SetZoomInWindow(VoDev, i, &s_astZoomAttrDflt));
		//printf("HI_MPI_VO_EnableChn  VoDev=%d ,i=%d\n",VoDev, i);

		if((VoDev == SPOT_VO_DEV) && (i>0))
		{
			continue;
		}
		CHECK_RET(HI_MPI_VO_EnableChn(VoDev, i));
	}
#else

	HI_S32 i = 0;
	HI_S32 u32Square = sqrt(DEV_CHANNEL_NUM);
	VO_CHN_ATTR_S stChnAttr;
	HI_S32 u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width;
	HI_S32 u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height;

	for (i = 0; i < PRV_VO_CHN_NUM; i++)	
	{
		stChnAttr.stRect.s32X       = ALIGN_BACK((u32Width/u32Square) * (i%u32Square), 2);
        stChnAttr.stRect.s32Y       = ALIGN_BACK((u32Height/u32Square) * (i/u32Square), 2);
        stChnAttr.stRect.u32Width   = ALIGN_BACK(u32Width/u32Square, 2);
        stChnAttr.stRect.u32Height  = ALIGN_BACK(u32Height/u32Square, 2);
        stChnAttr.u32Priority       = 0;
        stChnAttr.bDeflicker        = HI_FALSE;
		//CHECK_RET(HI_MPI_VO_SetChnAttr(VoDev, i, &s_stVoChnAttrDflt));
		CHECK_RET(HI_MPI_VO_SetChnAttr(VoDev, i, &stChnAttr));
		//CHECK_RET(HI_MPI_VO_SetZoomInWindow(VoDev, i, &s_astZoomAttrDflt));
		//printf("HI_MPI_VO_EnableChn  VoDev=%d ,i=%d\n",VoDev, i);

		CHECK_RET(HI_MPI_VO_EnableChn(VoDev, i));
	}
#endif
	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_DisableAllVoChn
Description: // �ر�ָ��VO�豸�ϵ�����ͨ����
Calls: 
Called By: //
Input: // // VO�豸ID
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_DisableAllVoChn(VO_DEV VoDev)
{
	HI_S32 i=0;
//�ر�ͨ��ʱ�����Ե�ͨ����ֹһ��
	for (i = 0; i < VO_MAX_CHN_NUM; i++)
	{
		//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "########HI_MPI_VO_DisableChn VoDev=%d g_Max_Vo_Num=%d,i=%d  !###########\n",VoDev,g_Max_Vo_Num,i);
		CHECK_RET(HI_MPI_VO_DisableChn(VoDev, i));
	}
	

	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_HideAllVoChn
Description: //  ����ָ��VO�豸�ϵ�����ͨ����
Calls: 
Called By: //
Input: // // VO�豸ID
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
HI_S32 PRV_HideAllVoChn(VO_DEV VoDev)
{
	HI_S32 i=0;
#if defined(SN9234H1)
	for (i = 0; i < g_Max_Vo_Num; i++)
#else
	for (i = 0; i < PRV_VO_CHN_NUM; i++)
#endif		
	{
#if defined(Hi3535)
		CHECK_RET(HI_MPI_VO_HideChn(VoDev, i));
#else
		CHECK_RET(HI_MPI_VO_ChnHide(VoDev, i));
#endif
	}

	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_ClearAllVoChnBuf
Description: //  �������VOͨ���ϵ�buffer
Calls: 
Called By: //
Input: // // VO�豸ID
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_ClearAllVoChnBuf(VO_DEV VoDev)
{
	HI_S32 i=0;
	
	for (i=0; i < PRV_VO_CHN_NUM; i++)
	{
		//printf("###########PRV_ClearAllVoChnBuf: VoDev = %d ,i=%d,PRV_CHAN_NUM=%d###################\n",VoDev,i,PRV_CHAN_NUM);
		CHECK_RET(HI_MPI_VO_ClearChnBuffer(VoDev, i, HI_TRUE));
	}
	
	RET_SUCCESS("");
}
#if defined(SN9234H1)
STATIC HI_S32 PRV_ViUnBindAllVoChn(VO_DEV VoDev)
{
		HI_S32 j = 0;
#if defined(SN6116HE)||defined(SN6116LE) ||defined(SN8616D_LE) || defined(SN8616M_LE) || defined(SN9234H1)
	for(j = 0; j < LOCALVEDIONUM; j++)
	{
		if(j < PRV_VI_CHN_NUM)
			CHECK_RET(HI_MPI_VI_UnBindOutput(PRV_656_DEV_1, j, VoDev,  j));
		else if(j >= PRV_VI_CHN_NUM && j < PRV_VI_CHN_NUM * 2)
			CHECK_RET(HI_MPI_VI_UnBindOutput(PRV_656_DEV, j, VoDev,  j));
		else 
			CHECK_RET(HI_MPI_VI_UnBindOutput(PRV_HD_DEV, 0, VoDev,	j));
	}
	
	//printf("########PRV_UnBindAllVoChn  suc!###########\n");
#elif defined(SN6108HE)
	for(j = 0; j < LOCALVEDIONUM; j++)
	{
		if(j < PRV_VI_CHN_NUM)
			CHECK_RET(HI_MPI_VI_UnBindOutput(PRV_656_DEV_1, j, VoDev,  j));
		else 
			CHECK_RET(HI_MPI_VI_UnBindOutput(PRV_HD_DEV, 0, VoDev,	j));
	}
	
#elif defined(SN6108LE) || defined(SN8608D_LE) || defined(SN8608M_LE)
	for(j = 0; j < LOCALVEDIONUM; j++)
	{
		if(j < PRV_VI_CHN_NUM)
			CHECK_RET(HI_MPI_VI_UnBindOutput(PRV_656_DEV_1, j, VoDev,  j));
		else
			CHECK_RET(HI_MPI_VI_UnBindOutput(PRV_656_DEV, j, VoDev,  j));
	}
#else
	HI_S32 i;

	for (i = 0; i < PRV_VI_DEV_NUM; i++)
	{
		for (j = 0; j < ((LOCALVEDIONUM - i * PRV_VI_CHN_NUM) > PRV_VI_CHN_NUM ? PRV_VI_CHN_NUM : (LOCALVEDIONUM - i * PRV_VI_CHN_NUM)); j++)
		{
			//printf("########PRV_UnBindAllVoChn  i=%d,j=%d,VoDev=%d,chn=%d!###########\n",i,j,VoDev,i*PRV_VI_CHN_NUM + j);
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "########PRV_UnBindAllVoChn  i=%d,j=%d,VoDev=%d,chn=%d!###########\n",i,j,VoDev,i*PRV_VI_CHN_NUM + j);
			CHECK_RET(HI_MPI_VI_UnBindOutput(i, j, VoDev, i * PRV_VI_CHN_NUM + j));
		}
	}
	//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "########PRV_UnBindAllVoChn  suc!###########\n");
#endif
	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_VdecUnBindAllVoChn
Description: //  �������VDEC��VO�İ�
Calls: 
Called By: //
Input: // // VO�豸ID
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
***************************************************/
HI_S32 PRV_VdecUnBindAllVoChn1(VO_DEV VoDev)
{
	HI_S32 i = 0;	
	//if(VoDev >= SD || VoDev < 0)
	//	return HI_FAILURE;
	//int index = PRV_GetVoChnIndex(VoChn);
	//if(VochnInfo[index].IsLocalVideo != 0)
	//	return HI_FAILURE;
	for(i = LOCALVEDIONUM; i < DEV_CHANNEL_NUM; i++)
	{
	
		//if((VochnInfo[i].IsBindVdec[VoDev] != -1))
		{
			#if 0
			if(VoDev == SD)	
			{
				CHECK_RET(HI_MPI_VDEC_UnbindOutputChn(VochnInfo[index].VdecChn, VoDev, SPOT_VO_CHAN));
				break;
			}
			#endif
			//printf("------VoDev: %d--------UnBind---i: %d, SlaveId: %d ---VoChn: %d, VdecChn: %d, IsBindVdec: %d\n", VoDev, i, VochnInfo[i].SlaveId, VochnInfo[i].VoChn, VochnInfo[i].VdecChn, VochnInfo[i].IsBindVdec[VoDev]);

			if(VochnInfo[i].SlaveId == PRV_MASTER)
				(HI_MPI_VDEC_UnbindOutputChn(VochnInfo[i].VdecChn, VoDev, VochnInfo[i].VoChn));
			else if(VochnInfo[i].SlaveId > PRV_MASTER)			
				(HI_MPI_VI_UnBindOutput(PRV_HD_DEV, 0, VoDev, VochnInfo[i].VoChn));
			
			VochnInfo[i].IsBindVdec[VoDev] = -1;
		}
	}
	RET_SUCCESS("");
}

HI_S32 PRV_VdecBindAllVoChn(VO_DEV VoDev)
{
	HI_S32 i = 0;	
	//if(VoDev >= SD || VoDev < 0)
	//	return HI_FAILURE;
	//int index = PRV_GetVoChnIndex(VoChn);
	//if(VochnInfo[index].IsLocalVideo != 0)
	//	return HI_FAILURE;
	for(i = LOCALVEDIONUM; i < DEV_CHANNEL_NUM; i++)
	{
	
		//if((VochnInfo[i].IsBindVdec[VoDev] == -1))
		{

			//printf("------VoDev: %d--------Bind---i: %d, SlaveId: %d ---VoChn: %d, VdecChn: %d, IsBindVdec: %d\n", VoDev, i, VochnInfo[i].SlaveId, VochnInfo[i].VoChn, VochnInfo[i].VdecChn, VochnInfo[i].IsBindVdec[VoDev]);
			//ͨ������״̬��(��ʾ����)������Ҫ����������Ƶͨ��(�����л�����)
			if(s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_CTRL
				&& (VochnInfo[i].VdecChn == DetVLoss_VdecChn || VochnInfo[i].VdecChn == NoConfig_VdecChn)			
				&& VoDev == s_VoDevCtrlDflt
				&& VochnInfo[i].VoChn == s_astVoDevStatDflt[VoDev].s32CtrlChn)
			{
				continue;
			}
			if(VochnInfo[i].SlaveId == PRV_MASTER)
				HI_MPI_VDEC_BindOutput(VochnInfo[i].VdecChn, VoDev, VochnInfo[i].VoChn);
			else if(VochnInfo[i].SlaveId > PRV_MASTER)			
				HI_MPI_VI_BindOutput(PRV_HD_DEV, 0, VoDev, VochnInfo[i].VoChn);
			
			VochnInfo[i].IsBindVdec[VoDev] = (VochnInfo[i].VdecChn == DetVLoss_VdecChn || VochnInfo[i].VdecChn == NoConfig_VdecChn) ? 0 : 1;
		}
	}
	
	RET_SUCCESS("");

}

#endif
#if defined(Hi3531)||defined(Hi3535)


int PRV_VPSS_ResetWH(int VpssGrp, int VdChn, int u32MaxW, int u32MaxH)
{
    HI_S32 s32Ret = 0;
	HI_S32 i = 0;	
	VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stGrpAttr;
	VPSS_CHN_ATTR_S stChnAttr;
	VPSS_GRP_PARAM_S stVpssParam;
	if(VdChn == DetVLoss_VdecChn || VdChn == NoConfig_VdecChn)
	{
		u32MaxW = 2048;
		u32MaxH = 1536;
	}
	if(u32MaxW<=2048&&u32MaxH<=1536)
	{
		u32MaxW = 2048;
		u32MaxH = 1536;
	}
	s32Ret = HI_MPI_VPSS_GetGrpAttr(VpssGrp, &stGrpAttr);
	if(s32Ret != HI_SUCCESS)
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV,"HI_MPI_VPSS_GetGrpAttr-------failed with %#x!\n", s32Ret);
		return s32Ret;
	}
	
	//printf("PRV_VPSS_ResetWH,VpssGrp:%d,VdChn:%d,u32MaxW = %d,u32MaxH = %d,stGrpAttr.u32MaxW=%d,stGrpAttr.u32MaxH=%d\n",VpssGrp,VdChn,u32MaxW,u32MaxH,stGrpAttr.u32MaxW,stGrpAttr.u32MaxH);
	if((stGrpAttr.u32MaxW!=u32MaxW || stGrpAttr.u32MaxH!=u32MaxH)&&(stGrpAttr.u32MaxW>2048 ||stGrpAttr.u32MaxH>1536|| u32MaxW>2048||u32MaxH>1536))
	{
#if defined(Hi3535)	
		if(VpssGrp == PRV_CTRL_VOCHN)
		{
			CHECK(PRV_VO_UnBindVpss(PIP, VpssGrp, VpssGrp, VPSS_BSTR_CHN));
		}
		else
		{
			CHECK(PRV_VO_UnBindVpss(DHD0, VpssGrp, VpssGrp, VPSS_BSTR_CHN));
		}

		s32Ret = HI_MPI_VPSS_StopGrp(VpssGrp);
		if (s32Ret != HI_SUCCESS)
		{
			TRACE(SCI_TRACE_HIGH, MOD_PRV,"HI_MPI_VPSS_StopGrp-------failed with %#x!\n", s32Ret);
			return HI_FAILURE;
		}
		for(i = 0; i < VPSS_MAX_CHN_NUM; i++)
		{
			s32Ret = HI_MPI_VPSS_DisableChn(VpssGrp, i);
			if (s32Ret != HI_SUCCESS)
			{
				TRACE(SCI_TRACE_HIGH, MOD_PRV,"HI_MPI_VPSS_DisableChn-------failed with %#x!\n", s32Ret);
				return HI_FAILURE;
			}
		}
		s32Ret = HI_MPI_VPSS_DestroyGrp(VpssGrp);
		if (s32Ret != HI_SUCCESS)
		{
			TRACE(SCI_TRACE_HIGH, MOD_PRV,"HI_MPI_VPSS_DestroyGrp-------failed with %#x!\n", s32Ret);
			return HI_FAILURE;
		}
		
		stGrpAttr.u32MaxW = u32MaxW;
		stGrpAttr.u32MaxH = u32MaxH;
		stGrpAttr.bDciEn = HI_FALSE;
		stGrpAttr.bIeEn = HI_FALSE;
		stGrpAttr.bNrEn = HI_FALSE;
		stGrpAttr.bHistEn = HI_FALSE;
		stGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
		stGrpAttr.enPixFmt = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
		
		/*** create vpss group ***/
		s32Ret = HI_MPI_VPSS_CreateGrp(VpssGrp, &stGrpAttr);
		if (s32Ret != HI_SUCCESS)
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "HI_MPI_VPSS_CreateGrp failed with %#x!\n", s32Ret);
			return HI_FAILURE;
		}
	
		/*** set vpss param ***/
		s32Ret = HI_MPI_VPSS_GetGrpParam(VpssGrp,  &stVpssParam);
		if (s32Ret != HI_SUCCESS)
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "failed with %#x!\n", s32Ret);
			return HI_FAILURE;
		}
		
		s32Ret = HI_MPI_VPSS_SetGrpParam(VpssGrp,  &stVpssParam);
		if (s32Ret != HI_SUCCESS)
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "failed with %#x!\n", s32Ret);
			return HI_FAILURE;
		}
		
		/*** enable vpss chn, with frame ***/
		stChnAttr.bSpEn    = HI_FALSE;
		stChnAttr.bBorderEn = HI_FALSE;
		stChnAttr.stBorder.u32BottomWidth = 0;
		stChnAttr.stBorder.u32LeftWidth = 0;
		stChnAttr.stBorder.u32RightWidth = 0;
		stChnAttr.stBorder.u32TopWidth = 0;
		stChnAttr.stBorder.u32Color = 0;
		
		for(i = 0; i < VPSS_MAX_CHN_NUM; i++)
		{
			VpssChn = i;
			/* Set Vpss Chn attr */
			
			s32Ret = HI_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn, &stChnAttr);
			if (s32Ret != HI_SUCCESS)
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "HI_MPI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
				return HI_FAILURE;
			}
	
			s32Ret = HI_MPI_VPSS_EnableChn(VpssGrp, VpssChn);
			if (s32Ret != HI_SUCCESS)
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "HI_MPI_VPSS_EnableChn failed with %#x\n", s32Ret);
				return HI_FAILURE;
			}
		}
	
		/*** start vpss group ***/
		s32Ret = HI_MPI_VPSS_StartGrp(VpssGrp);
		if (s32Ret != HI_SUCCESS)
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "HI_MPI_VPSS_StartGrp failed with %#x\n", s32Ret);
			return HI_FAILURE;
		}
		if(VpssGrp == PRV_CTRL_VOCHN)
		{
			CHECK(PRV_VO_BindVpss(PIP, VpssGrp, VpssGrp, VPSS_BSTR_CHN));
			CHECK_RET(HI_MPI_VO_ShowChn(PIP, VpssGrp));
			CHECK_RET(HI_MPI_VO_EnableChn(PIP, VpssGrp));
		}
		else
		{
			CHECK(PRV_VO_BindVpss(DHD0, VpssGrp, VpssGrp, VPSS_BSTR_CHN));
			CHECK_RET(HI_MPI_VO_ShowChn(DHD0, VpssGrp));
			CHECK_RET(HI_MPI_VO_EnableChn(DHD0, VpssGrp));
		}
#else	
		
		CHECK(PRV_VO_UnBindVpss(DHD0, VpssGrp, VpssGrp, VPSS_PRE0_CHN));
		s32Ret = HI_MPI_VPSS_StopGrp(VpssGrp);
		if (s32Ret != HI_SUCCESS)
		{
			TRACE(SCI_TRACE_HIGH, MOD_PRV,"HI_MPI_VPSS_StopGrp-------failed with %#x!\n", s32Ret);
			return HI_FAILURE;
		}
		for(i = 0; i < VPSS_MAX_CHN_NUM; i++)
		{
			s32Ret = HI_MPI_VPSS_DisableChn(VpssGrp, i);
			if (s32Ret != HI_SUCCESS)
			{
				TRACE(SCI_TRACE_HIGH, MOD_PRV,"HI_MPI_VPSS_DisableChn-------failed with %#x!\n", s32Ret);
				return HI_FAILURE;
			}
		}
		s32Ret = HI_MPI_VPSS_DestroyGrp(VpssGrp);
		if (s32Ret != HI_SUCCESS)
		{
			TRACE(SCI_TRACE_HIGH, MOD_PRV,"HI_MPI_VPSS_DestroyGrp-------failed with %#x!\n", s32Ret);
			return HI_FAILURE;
		}
		stGrpAttr.u32MaxW = u32MaxW;
		stGrpAttr.u32MaxH = u32MaxH;
		stGrpAttr.bDrEn = HI_FALSE;
		stGrpAttr.bDbEn = HI_FALSE;
		stGrpAttr.bIeEn = HI_TRUE;
		stGrpAttr.bNrEn = HI_TRUE;
		stGrpAttr.bHistEn = HI_FALSE;
		stGrpAttr.enDieMode = VPSS_DIE_MODE_AUTO;
		stGrpAttr.enPixFmt = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
		
		/*** create vpss group ***/
		s32Ret = HI_MPI_VPSS_CreateGrp(VpssGrp, &stGrpAttr);
		if (s32Ret != HI_SUCCESS)
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "HI_MPI_VPSS_CreateGrp failed with %#x!\n", s32Ret);
			return HI_FAILURE;
		}
	
		/*** set vpss param ***/
		s32Ret = HI_MPI_VPSS_GetGrpParam(VpssGrp,  &stVpssParam);
		if (s32Ret != HI_SUCCESS)
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "failed with %#x!\n", s32Ret);
			return HI_FAILURE;
		}
		
		stVpssParam.u32MotionThresh = 0;
		
		s32Ret = HI_MPI_VPSS_SetGrpParam(VpssGrp,  &stVpssParam);
		if (s32Ret != HI_SUCCESS)
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "failed with %#x!\n", s32Ret);
			return HI_FAILURE;
		}
		
		/*** enable vpss chn, with frame ***/
		stChnAttr.bSpEn    = HI_FALSE;
		stChnAttr.bFrameEn = HI_FALSE;
		stChnAttr.stFrame.u32Color[VPSS_FRAME_WORK_LEFT]   = 0xff00;
		stChnAttr.stFrame.u32Color[VPSS_FRAME_WORK_RIGHT]  = 0xff00;
		stChnAttr.stFrame.u32Color[VPSS_FRAME_WORK_BOTTOM] = 0xff00;
		stChnAttr.stFrame.u32Color[VPSS_FRAME_WORK_TOP]    = 0xff00;
		stChnAttr.stFrame.u32Width[VPSS_FRAME_WORK_LEFT]   = 2;
		stChnAttr.stFrame.u32Width[VPSS_FRAME_WORK_RIGHT]  = 2;
		stChnAttr.stFrame.u32Width[VPSS_FRAME_WORK_TOP]    = 2;
		stChnAttr.stFrame.u32Width[VPSS_FRAME_WORK_BOTTOM] = 2;
		for(i = 0; i < VPSS_MAX_CHN_NUM; i++)
		{
			VpssChn = i;
			/* Set Vpss Chn attr */
			
			s32Ret = HI_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn, &stChnAttr);
			if (s32Ret != HI_SUCCESS)
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "HI_MPI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
				return HI_FAILURE;
			}
	
			s32Ret = HI_MPI_VPSS_EnableChn(VpssGrp, VpssChn);
			if (s32Ret != HI_SUCCESS)
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "HI_MPI_VPSS_EnableChn failed with %#x\n", s32Ret);
				return HI_FAILURE;
			}
		}
	
		/*** start vpss group ***/
		s32Ret = HI_MPI_VPSS_StartGrp(VpssGrp);
		if (s32Ret != HI_SUCCESS)
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "HI_MPI_VPSS_StartGrp failed with %#x\n", s32Ret);
			return HI_FAILURE;
		}
		CHECK(PRV_VO_BindVpss(DHD0, VpssGrp, VpssGrp, VPSS_PRE0_CHN));
		CHECK_RET(HI_MPI_VO_ChnShow(DHD0, VpssGrp));
		CHECK_RET(HI_MPI_VO_EnableChn(DHD0, VpssGrp));
#endif	
		
	}
	else
	{
		//printf("The same wh,u32MaxW = %d,u32MaxH = %d\n",u32MaxW,u32MaxH);
	}
	return HI_SUCCESS;
}

HI_S32 PRV_AUDIO_AoBindAdec(AUDIO_DEV AoDev, AO_CHN AoChn, ADEC_CHN AdChn)
{
	HI_S32 s32Ret = 0;
    MPP_CHN_S stSrcChn,stDestChn;

    stSrcChn.enModId = HI_ID_ADEC;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = AdChn;
    stDestChn.enModId = HI_ID_AO;
    stDestChn.s32DevId = AoDev;
    stDestChn.s32ChnId = AoChn;
    
    s32Ret = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn); 
	if (s32Ret != HI_SUCCESS)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_AUDIO_AoBindAdec failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}
	return HI_SUCCESS;
}

/******************************************************************************
* function : Ao unbind Adec
******************************************************************************/
HI_S32 PRV_AUDIO_AoUnbindAdec(AUDIO_DEV AoDev, AO_CHN AoChn, ADEC_CHN AdChn)
{
	HI_S32 s32Ret = 0;
    MPP_CHN_S stSrcChn,stDestChn;

    stSrcChn.enModId = HI_ID_ADEC;
    stSrcChn.s32ChnId = AdChn;
    stSrcChn.s32DevId = 0;
    stDestChn.enModId = HI_ID_AO;
    stDestChn.s32DevId = AoDev;
    stDestChn.s32ChnId = AoChn;
    
    s32Ret =  HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn); 
	if (s32Ret != HI_SUCCESS)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_AUDIO_AoUnbindAdec failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}
	return HI_SUCCESS;
}

HI_S32 PRV_VO_BindVpss(VO_DEV VoDev, VO_CHN VoChn, VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{
    HI_S32 s32Ret = HI_SUCCESS;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

    stSrcChn.enModId = HI_ID_VPSS;
    stSrcChn.s32DevId = VpssGrp;
    stSrcChn.s32ChnId = VpssChn;

    stDestChn.enModId = HI_ID_VOU;
    stDestChn.s32DevId = VoDev;
    stDestChn.s32ChnId = VoChn;

    s32Ret = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (s32Ret != HI_SUCCESS)
    {
        TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_VO_BindVpss failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    return s32Ret;
}

/******************************************************************************
* function : vdec group bind vpss chn
******************************************************************************/
HI_S32 PRV_VDEC_BindVpss(VDEC_CHN VdChn, VPSS_GRP VpssGrp)
{
    HI_S32 s32Ret = HI_SUCCESS;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

    stSrcChn.enModId = HI_ID_VDEC;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = VdChn;

    stDestChn.enModId = HI_ID_VPSS;
    stDestChn.s32DevId = VpssGrp;
    stDestChn.s32ChnId = 0;
	
	//printf("+++++++++++++++++++++++++PRV_VDEC_BindVpss,vdechn:%d,vpssgrp:%d\n",VdChn,VpssGrp);
    s32Ret = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (s32Ret != HI_SUCCESS)
    {
        TRACE(SCI_TRACE_NORMAL, MOD_PRV,"%s line:%d VdChn:%d,VpssGrp:%d,PRV_VDEC_BindVpss HI_MPI_SYS_Bind failed with %#x!\n", __FUNCTION__, __LINE__, VdChn,VpssGrp,s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
}

HI_S32 PRV_VO_UnBindVpss(VO_DEV VoDev,VO_CHN VoChn,VPSS_GRP VpssGrp,VPSS_CHN VpssChn)
{
    HI_S32 s32Ret = HI_SUCCESS;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

    stSrcChn.enModId = HI_ID_VPSS;
    stSrcChn.s32DevId = VpssGrp;
    stSrcChn.s32ChnId = VpssChn;

    stDestChn.enModId = HI_ID_VOU;
    stDestChn.s32DevId = VoDev;
    stDestChn.s32ChnId = VoChn;

    s32Ret = HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    if (s32Ret != HI_SUCCESS)
    {
        TRACE(SCI_TRACE_NORMAL, MOD_PRV, "failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    return s32Ret;
}
HI_S32 PRV_VoUnBindAllVpss(VO_DEV VoDev)
{
	//if(VoDev >= PRV_VO_MAX_DEV)
	if(VoDev > DHD0)
		RET_FAILURE("Invalid VoDev!!!");
	int i = 0;
	VPSS_CHN VpssChn = 0;
	
#if defined(Hi3535)
	VpssChn = VPSS_BSTR_CHN;
#else
	if(VoDev == DHD0)
	{
		VpssChn = VPSS_PRE0_CHN;
	}
	else
	{
		VpssChn = VPSS_BYPASS_CHN;
	}
#endif
	
	for(i = 0; i < PRV_VO_CHN_NUM; i++)
	{
		PRV_VO_UnBindVpss(VoDev, i, i, VpssChn);
	}
	return 0;
}

HI_S32 PRV_VoBindAllVpss(VO_DEV VoDev)
{
	if(VoDev >= PRV_VO_MAX_DEV)
	if(VoDev > DHD0)
		RET_FAILURE("Invalid VoDev!!!");
	int i = 0;
	VPSS_CHN VpssChn = 0;
	
#if defined(Hi3535)
	VpssChn = VPSS_BSTR_CHN;
#else
	if(VoDev == DHD0)
	{
		VpssChn = VPSS_PRE0_CHN;
	}
	else
	{
		VpssChn = VPSS_BYPASS_CHN;
	}
#endif
	
	for(i = 0; i < PRV_VO_CHN_NUM; i++)
	{
		PRV_VO_BindVpss(VoDev, i, i, VpssChn);
	}
	return 0;
}
/*************************************************
Function: //PRV_VdecUnBindAllVoChn
Description: //  �������VDEC��VO�İ�
Calls: 
Called By: //
Input: // // VO�豸ID
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
***************************************************/
HI_S32 PRV_VdecUnBindAllVpss(VO_DEV VoDev)
{
	HI_S32 i = 0;	
	for(i = 0; i < PRV_VO_CHN_NUM; i++)
	{
		if(VochnInfo[i].IsBindVdec[VoDev] != -1)
		{
			PRV_VDEC_UnBindVpss(VochnInfo[i].VdecChn, i);				
			PRV_VPSS_ResetWH(i,VochnInfo[i].VdecChn,704,576);
			VochnInfo[i].IsBindVdec[VoDev] = -1;
		}
	}
	RET_SUCCESS("");
}

HI_S32 PRV_VdecBindAllVpss(VO_DEV VoDev)
{
	HI_S32 i = 0;	
	//if(VoDev >= SD || VoDev < 0)
	//	return HI_FAILURE;
	//int index = PRV_GetVoChnIndex(VoChn);
	//if(VochnInfo[index].IsLocalVideo != 0)
	//	return HI_FAILURE;
	for(i = LOCALVEDIONUM; i < DEV_CHANNEL_NUM; i++)
	{
	
		if((VochnInfo[i].IsBindVdec[VoDev] == -1))
		{

			//printf("------VoDev: %d--------UnBind---i: %d, SlaveId: %d ---VoChn: %d, VdecChn: %d, IsBindVdec: %d\n", VoDev, i, VochnInfo[i].SlaveId, VochnInfo[i].VoChn, VochnInfo[i].VdecChn, VochnInfo[i].IsBindVdec[VoDev]);
			//ͨ������״̬��(��ʾ����)������Ҫ����������Ƶͨ��(�����л�����)
			if(s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_CTRL
				&& VochnInfo[i].VdecChn == DetVLoss_VdecChn				
				&& VoDev == s_VoDevCtrlDflt
				&& VochnInfo[i].VoChn == s_astVoDevStatDflt[VoDev].s32CtrlChn)
			{
				continue;
			}
			if(VochnInfo[i].SlaveId == PRV_MASTER)
			{
				//HI_S32 s32Ret = HI_SUCCESS;
				//printf("VochnInfo[i].VdecChn:%d, VochnInfo[i].VoChn:%d\n", VochnInfo[i].VdecChn, VochnInfo[i].VoChn);
				PRV_VDEC_UnBindVpss(VochnInfo[i].VdecChn, VochnInfo[i].VoChn);
				CHECK(PRV_VDEC_BindVpss(VochnInfo[i].VdecChn, VochnInfo[i].VoChn));
			}
//			else if(VochnInfo[i].SlaveId > PRV_MASTER)			
//				CHECK(HI_MPI_VI_BindOutput(PRV_HD_DEV, 0, VoDev, VochnInfo[i].VoChn));
			
			VochnInfo[i].IsBindVdec[VoDev] = (VochnInfo[i].VdecChn == DetVLoss_VdecChn) ? 0 : 1;
		}
	}
	
	RET_SUCCESS("");

}

/*****************************************************************************
* function : start vpss. VPSS chn with frame
*****************************************************************************/
HI_S32 PRV_VPSS_Start(VPSS_GRP VpssGrp)
{
#if defined(Hi3535)
	VPSS_CHN VpssChn;
	VPSS_GRP_ATTR_S stGrpAttr;
	VPSS_CHN_ATTR_S stChnAttr;
	VPSS_GRP_PARAM_S stVpssParam;
	HI_S32 s32Ret = 0, i = 0;
//	VPSS_GRP_ATTR_S *pstVpssGrpAttr = NULL;
	/*** Set Vpss Grp Attr ***/

	stGrpAttr.u32MaxW = 2048;
	stGrpAttr.u32MaxH = 1536;
	stGrpAttr.bDciEn = HI_FALSE;
	stGrpAttr.bIeEn = HI_FALSE;
	stGrpAttr.bNrEn = HI_FALSE;
	stGrpAttr.bHistEn = HI_FALSE;
	stGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
	stGrpAttr.enPixFmt = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
	
	/*** create vpss group ***/
	s32Ret = HI_MPI_VPSS_CreateGrp(VpssGrp, &stGrpAttr);
	if (s32Ret != HI_SUCCESS)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "HI_MPI_VPSS_CreateGrp failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	/*** set vpss param ***/
	s32Ret = HI_MPI_VPSS_GetGrpParam(VpssGrp,  &stVpssParam);
	if (s32Ret != HI_SUCCESS)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}
	
	s32Ret = HI_MPI_VPSS_SetGrpParam(VpssGrp,  &stVpssParam);
	if (s32Ret != HI_SUCCESS)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}
	
	/*** enable vpss chn, with frame ***/
	stChnAttr.bSpEn    = HI_FALSE;
	stChnAttr.bBorderEn = HI_FALSE;
	stChnAttr.stBorder.u32BottomWidth = 0;
	stChnAttr.stBorder.u32LeftWidth = 0;
	stChnAttr.stBorder.u32RightWidth = 0;
	stChnAttr.stBorder.u32TopWidth = 0;
	stChnAttr.stBorder.u32Color = 0;
	
	for(i = 0; i < VPSS_MAX_CHN_NUM; i++)
	{
		VpssChn = i;
		/* Set Vpss Chn attr */
		
		s32Ret = HI_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn, &stChnAttr);
		if (s32Ret != HI_SUCCESS)
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "HI_MPI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
			return HI_FAILURE;
		}

		s32Ret = HI_MPI_VPSS_EnableChn(VpssGrp, VpssChn);
		if (s32Ret != HI_SUCCESS)
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "HI_MPI_VPSS_EnableChn failed with %#x\n", s32Ret);
			return HI_FAILURE;
		}
	}

	/*** start vpss group ***/
	s32Ret = HI_MPI_VPSS_StartGrp(VpssGrp);
	if (s32Ret != HI_SUCCESS)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "HI_MPI_VPSS_StartGrp failed with %#x\n", s32Ret);
		return HI_FAILURE;
	}
	
	return HI_SUCCESS;
#else

    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stGrpAttr;
    VPSS_CHN_ATTR_S stChnAttr;
    VPSS_GRP_PARAM_S stVpssParam;
    HI_S32 s32Ret = 0, i = 0;
//	VPSS_GRP_ATTR_S *pstVpssGrpAttr = NULL;
    /*** Set Vpss Grp Attr ***/

    //stGrpAttr.u32MaxW = 2048;
    //stGrpAttr.u32MaxH = 1536;
    stGrpAttr.u32MaxW = 2560;
    stGrpAttr.u32MaxH = 1920;
    stGrpAttr.bDrEn = HI_FALSE;
    stGrpAttr.bDbEn = HI_FALSE;
    stGrpAttr.bIeEn = HI_TRUE;
    stGrpAttr.bNrEn = HI_TRUE;
    stGrpAttr.bHistEn = HI_FALSE;
    stGrpAttr.enDieMode = VPSS_DIE_MODE_AUTO;
    stGrpAttr.enPixFmt = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
	
    /*** create vpss group ***/
    s32Ret = HI_MPI_VPSS_CreateGrp(VpssGrp, &stGrpAttr);
    if (s32Ret != HI_SUCCESS)
    {
        TRACE(SCI_TRACE_NORMAL, MOD_PRV, "HI_MPI_VPSS_CreateGrp failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    /*** set vpss param ***/
    s32Ret = HI_MPI_VPSS_GetGrpParam(VpssGrp,  &stVpssParam);
    if (s32Ret != HI_SUCCESS)
    {
        TRACE(SCI_TRACE_NORMAL, MOD_PRV, "failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }
    
    stVpssParam.u32MotionThresh = 0;
    
    s32Ret = HI_MPI_VPSS_SetGrpParam(VpssGrp,  &stVpssParam);
    if (s32Ret != HI_SUCCESS)
    {
        TRACE(SCI_TRACE_NORMAL, MOD_PRV, "failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }
	
    /*** enable vpss chn, with frame ***/
	stChnAttr.bSpEn    = HI_FALSE;
	stChnAttr.bFrameEn = HI_FALSE;
	stChnAttr.stFrame.u32Color[VPSS_FRAME_WORK_LEFT]   = 0xff00;
	stChnAttr.stFrame.u32Color[VPSS_FRAME_WORK_RIGHT]  = 0xff00;
	stChnAttr.stFrame.u32Color[VPSS_FRAME_WORK_BOTTOM] = 0xff00;
	stChnAttr.stFrame.u32Color[VPSS_FRAME_WORK_TOP]    = 0xff00;
	stChnAttr.stFrame.u32Width[VPSS_FRAME_WORK_LEFT]   = 2;
	stChnAttr.stFrame.u32Width[VPSS_FRAME_WORK_RIGHT]  = 2;
	stChnAttr.stFrame.u32Width[VPSS_FRAME_WORK_TOP]    = 2;
	stChnAttr.stFrame.u32Width[VPSS_FRAME_WORK_BOTTOM] = 2;
    for(i = 0; i < VPSS_MAX_CHN_NUM; i++)
    {
        VpssChn = i;
        /* Set Vpss Chn attr */
        
        s32Ret = HI_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn, &stChnAttr);
        if (s32Ret != HI_SUCCESS)
        {
            TRACE(SCI_TRACE_NORMAL, MOD_PRV, "HI_MPI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
            return HI_FAILURE;
        }

        s32Ret = HI_MPI_VPSS_EnableChn(VpssGrp, VpssChn);
        if (s32Ret != HI_SUCCESS)
        {
            TRACE(SCI_TRACE_NORMAL, MOD_PRV, "HI_MPI_VPSS_EnableChn failed with %#x\n", s32Ret);
            return HI_FAILURE;
        }
    }

    /*** start vpss group ***/
    s32Ret = HI_MPI_VPSS_StartGrp(VpssGrp);
    if (s32Ret != HI_SUCCESS)
    {
        TRACE(SCI_TRACE_NORMAL, MOD_PRV, "HI_MPI_VPSS_StartGrp failed with %#x\n", s32Ret);
        return HI_FAILURE;
    }
	
    return HI_SUCCESS;
#endif
}

HI_S32 PRV_VPSS_Stop(VPSS_GRP VpssGrp)
{
    HI_S32 s32Ret = 0;
	HI_S32 i = 0;
	s32Ret = HI_MPI_VPSS_StopGrp(VpssGrp);
	if (s32Ret != HI_SUCCESS)
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV,"HI_MPI_VPSS_StopGrp-------failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}
	for(i = 0; i < VPSS_MAX_CHN_NUM; i++)
	{
		s32Ret = HI_MPI_VPSS_DisableChn(VpssGrp, i);
		if (s32Ret != HI_SUCCESS)
		{
			TRACE(SCI_TRACE_HIGH, MOD_PRV,"HI_MPI_VPSS_DisableChn-------failed with %#x!\n", s32Ret);
			return HI_FAILURE;
		}
	}
	s32Ret = HI_MPI_VPSS_DestroyGrp(VpssGrp);
	if (s32Ret != HI_SUCCESS)
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV,"HI_MPI_VPSS_DestroyGrp-------failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}

#endif


/*************************************************
Function: //PRV_ViInit
Description: //����VI�豸�����ϵ�ͨ����ʼ����
Calls: 
Called By: //
Input: // ��
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_ViInit(HI_VOID)
{
	HI_S32 i = 0;
	if(LOCALVEDIONUM > 0)
	{
#if defined(SN9234H1)
		s_stViDevPubAttrDflt.enViNorm = (0 == s_s32NPFlagDflt) ? VIDEO_ENCODING_MODE_PAL : VIDEO_ENCODING_MODE_NTSC;
		s_stViDevPubAttrDflt.enWorkMode = (0 == s_s32ViWorkMode) ? VI_WORK_MODE_4D1 : VI_WORK_MODE_4HALFD1;

		PRV_TW2865_CfgV(s_stViDevPubAttrDflt.enViNorm, s_stViDevPubAttrDflt.enWorkMode);
#else
		s_stViDevPubAttrDflt.enWorkMode = (0 == s_s32ViWorkMode) ? VI_WORK_MODE_4Multiplex: VI_WORK_MODE_1Multiplex;
#endif
	//����2Ƭ3520����ô��Ҫ��ʼ��0��1��2��3�����豸����ʵ��16D1�����룬����0��1�������ڸ�������
	//����4Ƭ3520�ķ�������ôÿƬ3520ֻ��Ҫ�õ������豸0��1��3�Ϳ����ˣ�����0��1�������ڸ�������
		//��ʼ�������豸
	//#if defined(SN6116HE)||defined(SN6116LE)||defined(SN6108HE)	|| defined(SN8616D_LE)|| defined(SN8616M_LE) || defined(SN9234H1)
#if defined(SN_SLAVE_ON)
		s_stViDevPubAttrDflt.enInputMode = VI_MODE_BT656;
		s_stViDevPubAttrDflt.bChromaSwap = HI_FALSE;
		s_stViDevPubAttrDflt.bIsChromaChn = HI_FALSE;
		s_stViChnAttrDflt.stCapRect.s32X= 8;
		s_stViChnAttrDflt.stCapRect.u32Width= 704;	
		//for(i = PRV_656_DEV; i < PRV_VI_DEV_NUM; i++)
		for(i = (LOCALVEDIONUM > PRV_VI_CHN_NUM ? PRV_656_DEV : PRV_656_DEV_1); i < PRV_VI_DEV_NUM; i++)
#else
		//for(i = 0;i < PRV_VI_DEV_NUM; i++)
		for(i = 0; i < (LOCALVEDIONUM/PRV_VI_CHN_NUM + 1); i++)
#endif
		{
			PRV_DisableAllViChn(i);
			usleep(100000);
			PRV_DisableViDev(i);

			CHECK_RET(PRV_EnableViDev(i));
			CHECK_RET(PRV_EnableAllViChn(i));
		}
	}
//#if defined(SN6116HE)||defined(SN6116LE)||defined(SN6108HE)	|| defined(SN8616D_LE)|| defined(SN8616M_LE)|| defined(SN9234H1)
#if defined(SN_SLAVE_ON)
	HI_U32 u32SrcFrmRate;

	//��ʼ������ӿ������豸
	PRV_DisableAllViChn(PRV_HD_DEV);
	usleep(100000);
	PRV_DisableViDev(PRV_HD_DEV);

	s_stViDevPubAttrDflt.enInputMode = VI_MODE_BT1120_PROGRESSIVE;
	s_stViDevPubAttrDflt.bChromaSwap = HI_TRUE;
	s_stViDevPubAttrDflt.bIsChromaChn = HI_FALSE;
	CHECK_RET(PRV_EnableViDev(PRV_HD_DEV));
	
	u32SrcFrmRate = (VIDEO_ENCODING_MODE_PAL == s_stViDevPubAttrDflt.enViNorm) ? 25: 30;
	s_stViChnAttrDflt.enCapSel = (0 == s_s32ViWorkMode) ? VI_CAPSEL_BOTH : VI_CAPSEL_BOTTOM;
	s_stViChnAttrDflt.bDownScale = (0 == s_s32ViWorkMode) ? HI_FALSE : HI_TRUE;
	s_stViChnAttrDflt.stCapRect.s32X= 0;
	s_stViChnAttrDflt.stCapRect.u32Width= PRV_BT1120_SIZE_W;
	s_stViChnAttrDflt.stCapRect.u32Height = PRV_BT1120_SIZE_H;
	//s_stViChnAttrDflt.stCapRect.u32Width= 704;
	//s_stViChnAttrDflt.stCapRect.u32Height = 576;
	CHECK_RET(HI_MPI_VI_SetChnAttr(PRV_HD_DEV, 0, &s_stViChnAttrDflt));
	CHECK_RET(HI_MPI_VI_EnableChn(PRV_HD_DEV, 0));
	CHECK_RET(HI_MPI_VI_SetSrcFrameRate(PRV_HD_DEV, 0, u32SrcFrmRate));
	CHECK_RET(HI_MPI_VI_SetFrameRate(PRV_HD_DEV, 0, u32SrcFrmRate));
#endif	
	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_VoInit
Description: //����VO�豸�����ϵ���Ƶ���ͨ����ʼ����
Calls: 
Called By: //
Input: // ��
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_VoInit(HI_VOID)
{
#if defined(SN_SLAVE_ON)	
	//���ü���
	if (HI_SUCCESS != HI_MPI_VI_DisableCascade(0,0))
	{
		RET_FAILURE("Disalbe vi cascade filter failed!!");
	}
#endif	

	HI_S32 i;
	//ֻ������DHD0(VGA���)��DSD0(CVBS���)
	for (i = 0; i < PRV_VO_DEV_NUM; i++)
	{
		PRV_DisableAllVoChn(i);
		PRV_DisableVideoLayer(i);
		PRV_DisableVoDev(i);

#if defined(SN9234H1)
		if(i < AD)
#else
		if(i > DHD0)
			continue;
		if(i != DHD0 && i != DSD0)
			continue;
#endif		
		{
		PRV_EnableVoDev(i);
		PRV_EnableVideoLayer(i);
		PRV_EnableAllVoChn(i);
		}
	}

#if defined(SN_SLAVE_ON)	
	//���ü���
	//HI_MPI_VI_EnableCascade(PRV_HD_DEV,0);
	if (HI_SUCCESS != HI_MPI_VI_EnableCascade(0,0))
	{
		RET_FAILURE("Enalbe vi cascade filter failed!!");
	}
#endif	

	RET_SUCCESS("");
}
/*************************************************
Function: //PRV_Msg_Cpy
Description: //���浱ǰ��Ϣ��Ϣ
Calls: 
Called By: //
Input: // msg_req :��Ϣ�ṹ��
Output: // ��
Return: //��
Others: // ����˵��
************************************************************************/
#if  defined(SN_SLAVE_ON)
static int PRV_Msg_Cpy(const SN_MSG *msg_req)
{
	int ret=0;
	if(s_State_Info.Prv_msg_Cur)
	{
		SN_FREE(s_State_Info.Prv_msg_Cur);//�ͷ�����ɵ���Ϣ�ṹ��ָ��
	}	
	s_State_Info.Prv_msg_Cur = (SN_MSG *)SN_MALLOC(sizeof(SN_MSG)+msg_req->size);
	if(s_State_Info.Prv_msg_Cur == NULL)
	{//
		ret = -1;
		return ret;
	}
	s_State_Info.Prv_msg_Cur->size= msg_req->size;
	s_State_Info.Prv_msg_Cur->source = msg_req->source;
	s_State_Info.Prv_msg_Cur->dest= msg_req->dest;
	s_State_Info.Prv_msg_Cur->user= msg_req->user;
	s_State_Info.Prv_msg_Cur->xid= msg_req->xid;
	s_State_Info.Prv_msg_Cur->thread= msg_req->thread;
	s_State_Info.Prv_msg_Cur->msgId= msg_req->msgId;
	if(s_State_Info.Prv_msg_Cur->para)
		SN_MEMCPY((char *)s_State_Info.Prv_msg_Cur->para,msg_req->size,(char *)msg_req->para,msg_req->size,msg_req->size);
	return ret;
}
#endif
/************************************************************************/
/*       ��Ƶ��ʧ�ж�                                           */
/************************************************************************/
static int Loss_State_Pre=0;
static int Loss_State_Cur=0;
extern int FWK_GetLostState();
//static int except_alarm[PRV_CHAN_NUM]={0};  //���������ʽ��ƥ��
int Vedio_Loss_State(unsigned char ch)
{
	int ret=0;
	Loss_State_Cur = FWK_GetLostState();
	ret = Loss_State_Cur;
	return ret;
}
/*************************************************
Function: //PRV_VLossDet
Description: //   ��Ƶ��ʧ����     
Calls: 
Called By: //
Input: //
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ʱ���̣߳�û500�������һ�Σ���������Ƶ�ź�
************************************************************************/

static int PRV_VLossDet(void)
{
    int i,is_lost=-1,is_change=0,try_cnt=0;
	VedioLoss_State  loss_state; 
	//return 0;
    for (i = 0; i < LOCALVEDIONUM; i++)
    {
		try_cnt = 0;
		is_change = (Loss_State_Pre>>i)&0x01;
try:		
        is_lost = Preview_GetAVstate(i);
		if(is_lost == -1)
		{
			RET_FAILURE("Preview_GetAVstate error!!!");
		}
#if 1
//������Ƶ���ȥ������
		if(is_lost != is_change)
		{
			if(try_cnt < 2)
			{
				try_cnt++;
				usleep(10000);	//��ʱ10������ٴβ�ѯ
				goto try;
			}
		}
		/*
		else
		{
			if(try_cnt <2 && try_cnt>0)
			{//���������ͬ��δ�ﵽ���Դ�������ô��������
				try_cnt++;
				usleep(10000);	//��ʱ10������ٴβ�ѯ
				goto try;
			}
		}*/
#endif	
		if(is_lost != is_change)
		{
			if (is_lost)
	        {
#if defined(SN6116HE) ||defined(SN6116LE) || defined(SN6108HE) || defined(SN6108LE) || defined(SN8608D_LE) || defined(SN8608M_LE) || defined(SN8616D_LE) || defined(SN8616M_LE)|| defined(SN9234H1)
	        	if(i < PRV_CHAN_NUM)
	        	{//���Ϊǰ8��ͨ������ô����Ƭ��������Ƶ�źţ�����Ǻ�8·����Ҫ������Ϣ����Ƭ���ô�Ƭ��������Ƶ���ɺ�Ŷ
					if(i>= PRV_VI_CHN_NUM)
					{	
#if defined(SN9234H1)
						//���Ϊͨ��5��8����ô���������豸2
		            	CHECK(HI_MPI_VI_EnableUserPic(PRV_656_DEV, i%PRV_VI_CHN_NUM));
#else
						//���Ϊͨ��5��8����ô���������豸2
		            	CHECK(HI_MPI_VI_EnableUserPic(i%PRV_VI_CHN_NUM));
#endif
					}
					else
					{
#if defined(SN9234H1)
						//���Ϊͨ��1��4����ô���������豸3
						CHECK(HI_MPI_VI_EnableUserPic(PRV_656_DEV_1, i%PRV_VI_CHN_NUM));
#else
						//���Ϊͨ��1��4����ô���������豸3
						CHECK(HI_MPI_VI_EnableUserPic(i%PRV_VI_CHN_NUM));
#endif
					}
	        	}
#if defined(SN_SLAVE_ON)				
				else	
				//���Ϊͨ��9��16����ô������Ϣ����Ƭ��
				//���ȴ���Ƭ������Ϣ
				{
					Prv_Slave_Vloss_Ind slave_req;
					slave_req.chn = i;
					slave_req.state = HI_TRUE;
					//printf("@####################loss i = %d##################\n",i);
					SN_SendMccMessageEx(PRV_SLAVE_1,SUPER_USER_ID, MOD_PRV, MOD_PRV, 0, 0, MSG_ID_PRV_MCC_VLOSS_IND, &slave_req, sizeof(Prv_Slave_Vloss_Ind));		
				}
#endif
#else
				//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "@####################i = %d, vichn=%d, videv=%d##################\n",i, i/PRV_VI_CHN_NUM,i%PRV_VI_CHN_NUM);
#if defined(SN9234H1)
				CHECK(HI_MPI_VI_EnableUserPic(i/PRV_VI_CHN_NUM, i%PRV_VI_CHN_NUM));
#else
				CHECK(HI_MPI_VI_EnableUserPic(i%PRV_VI_CHN_NUM));
#endif
#endif
				Loss_State_Cur = Loss_State_Cur | (1<<i);
	        }
	        else
	        {
#if defined(SN6116HE) ||defined(SN6116LE) || defined(SN6108HE) || defined(SN6108LE) || defined(SN8608D_LE) || defined(SN8608M_LE) || defined(SN8616D_LE) || defined(SN8616M_LE) || defined(SN9234H1)
	        	//printf("@####################i = %d##################\n",i);
	        	if(i< PRV_CHAN_NUM)
	        	{//���Ϊǰ8��ͨ������ô����Ƭȡ������Ƶ�źţ�����Ǻ�8·����Ҫ������Ϣ����Ƭ���ô�Ƭȡ������Ƶ���ɺ�Ŷ
					if(i>= PRV_VI_CHN_NUM)
					{
#if defined(SN9234H1)
		            	CHECK(HI_MPI_VI_DisableUserPic(PRV_656_DEV, i%PRV_VI_CHN_NUM));
#else
		            	CHECK(HI_MPI_VI_DisableUserPic(i%PRV_VI_CHN_NUM));
#endif
					}
					else
					{
#if defined(SN9234H1)
						CHECK(HI_MPI_VI_DisableUserPic(PRV_656_DEV_1, i%PRV_VI_CHN_NUM));
#else						
						CHECK(HI_MPI_VI_DisableUserPic(i%PRV_VI_CHN_NUM));
#endif
					}
	        	}
#if defined(SN_SLAVE_ON)				
				else	
				//���Ϊͨ��9��16����ô������Ϣ����Ƭ��
				//���ȴ���Ƭ������Ϣ
				{
					Prv_Slave_Vloss_Ind slave_req;
					slave_req.chn = i;
					slave_req.state = HI_FALSE;
					//printf("@####################unloss i = %d##################\n",i);
					SN_SendMccMessageEx(PRV_SLAVE_1,0xFFFFFFFF, MOD_PRV, MOD_PRV, 0, 0, MSG_ID_PRV_MCC_VLOSS_IND, &slave_req, sizeof(Prv_Slave_Vloss_Ind));		
				}
#endif				
#else
#if defined(SN9234H1)
				CHECK(HI_MPI_VI_DisableUserPic(i/PRV_VI_CHN_NUM, i%PRV_VI_CHN_NUM));
#else
				CHECK(HI_MPI_VI_DisableUserPic(i%PRV_VI_CHN_NUM));
#endif
#endif
				Loss_State_Cur = Loss_State_Cur &( ~(1<<i));
	        }      
		} 
#if 0		
		if(!is_lost)/*����Ƶ���������ʽ��飡*/
		{
			if(PRV_Compare_Stand(i)>0)
			{
				ch |= 1<<i;
			}
		}
#endif		
    }
	if (Loss_State_Pre != Loss_State_Cur) //�������Ƶ��ʧ����ô������Ƶ��ʧ����
	{
		loss_state.loss_state = Loss_State_Cur;
		SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_ALM, 0, 0, MSG_ID_VEDIO_LOSS_IND, &loss_state, sizeof(loss_state));
		SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_VAM, 0, 0, MSG_ID_VEDIO_LOSS_IND, &loss_state, sizeof(loss_state));
		Loss_State_Pre = Loss_State_Cur;
	}
#if 0	
	if (ch)
	{
		ALM_EXP_ALARM_ST stAlarmInfo;
		stAlarmInfo.u8ExpType = EXP_VIDEO_STANDARD;
		stAlarmInfo.u32Info[0] = ch;
		stAlarmInfo.u32Info[1] = 0;
		SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_ALM, 0, 0, MSG_ID_ALM_EXCEPTION_ALARM_REQ, &stAlarmInfo, sizeof(stAlarmInfo));
	}
#endif	
    return 0;
}

#if defined(SN9234H1)
/*************************************************
Function: //PRV_NVRChnVLossDet
Description: //   ����ͨ����Ƶ��ʧ����     
Calls: 
Called By: //
Input: //
Output: // 
Return: //��
Others: // ʱ������̣߳�ÿ500�������һ�Σ���ʾ/����"����Ƶ�ź�"����ͼƬ
************************************************************************/
void PRV_NVRChnVLossDet()
{
	if(s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat == PRV_STAT_PB || s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat == PRV_STAT_PIC)
		return;
	if(!s_State_Info.bIsOsd_Init)//OSDδ��ʼ��
		return;
	int i = LOCALVEDIONUM, s32Ret = 0;
	for(; i < DEV_CHANNEL_NUM; i++)
	{
		//ָ����ͨ���ڵ�ǰԤ��������
		if(HI_SUCCESS == PRV_VoChnIsInCurLayOut(s_VoDevCtrlDflt, VochnInfo[i].VoChn))
		{
			
			if(VochnInfo[i].VdecChn == DetVLoss_VdecChn//ָ��ͨ������Ƶ�ź�
				&& 0 == IsOSDAlarmOn[i - LOCALVEDIONUM])//��û������Ƶ����ͼ��
			{
				//��ʾ����ͼ��
				s32Ret = OSD_Ctl(VochnInfo[i].VoChn, 1, OSD_ALARM_TYPE);
				if(s32Ret != HI_SUCCESS)
				{
					TRACE(SCI_TRACE_HIGH, MOD_PRV, "PRV_NVRChnVLossDet OSD_Ctl faild 0x%x!\n",s32Ret);
					continue;
				}
				IsOSDAlarmOn[i - LOCALVEDIONUM] = 1;
			}
			else if(VochnInfo[i].VdecChn != DetVLoss_VdecChn//ָ��ͨ������Ƶ�ź�
				&& 1 == IsOSDAlarmOn[i - LOCALVEDIONUM])//����ʾ������Ƶ����ͼ��
			{
				//���ر���ͼ��
				s32Ret = OSD_Ctl(VochnInfo[i].VoChn, 0, OSD_ALARM_TYPE);
				if(s32Ret != HI_SUCCESS)
				{
					TRACE(SCI_TRACE_HIGH, MOD_PRV, "PRV_NVRChnVLossDet OSD_Ctl faild 0x%x!\n",s32Ret);
					continue;
				}
				
				IsOSDAlarmOn[i - LOCALVEDIONUM] = 0;
			}
		}
	}
}

#endif

/*************************************************
Function: //PRV_ReadNvrNoVideoPic
Description: //  ����"��������Ƶ"ͼƬ����Ϣ
Calls: 
Called By: //
Input: // // fileNameͼƬ�ļ���
Output: // Buffer[]ͼƬ��Ϣ
Return: //ͼƬ��Ϣ����
Others: // ����˵��
************************************************************************/
HI_S32 PRV_ReadNvrNoVideoPic(char *fileName, unsigned char Buffer[])
{
	int dataLen = 0;
	
	FILE* fp = NULL;
	fp = fopen(fileName, "r");
	if(fp == NULL)
	{
		perror("fopen");
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Open file: %s failed!\n", fileName);
		return -1;
	}
	dataLen = fread(Buffer, sizeof(char), VLOSSPICBUFFSIZE, fp);
	//printf("----------datalen: %d, fileName: %s\n", dataLen, fileName);
	return dataLen;
}

#if defined(SN9234H1)
/*************************************************
Function: //PRV_NvrNoVideoDet
Description: //  ��������Ƶ�źż�⡣
			����������ƵͼƬר�ý���ͨ�������ͨ����
Calls: 
Called By: //
Input: // // ��
Output: // ��
Return: //��
Others: // ����˵��
************************************************************************/

void PRV_NvrNoVideoDet()
{
	HI_S32 i = 0, j = 0, s32Ret = 0;
#if defined (SN8604D) || defined (SN8604M)
	for(j = LOCALVEDIONUM; j < DEV_CHANNEL_NUM; j++)
	{
		if(VochnInfo[j].VdecChn == DetVLoss_VdecChn/* && VochnInfo[j].IsBindVdec[HD] == -1*/)
		{	
			s32Ret = HI_MPI_VDEC_BindOutput(VochnInfo[j].VdecChn, HD, j);
			#if 0
			if(s32Ret != HI_SUCCESS)
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "------------Vdec: %d Bind Vo: %d fail\n", VochnInfo[j].VdecChn, j);
			}
			else
			{				
				VochnInfo[j].IsBindVdec[HD] = 0;
			}
			#endif
		}
	}
	for(j = LOCALVEDIONUM; j < DEV_CHANNEL_NUM; j++)
	{
		if(VochnInfo[j].VdecChn == DetVLoss_VdecChn /*&& VochnInfo[j].IsBindVdec[s_VoSecondDev] == -1*/)
		{	
			s32Ret = HI_MPI_VDEC_BindOutput(VochnInfo[j].VdecChn, s_VoSecondDev, j);
			#if 0
			if(s32Ret != HI_SUCCESS)
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "------------Vdec: %d Bind Vo: %d fail\n", VochnInfo[j].VdecChn, j);
			}
			else
			{				
				VochnInfo[j].IsBindVdec[s_VoSecondDev] = 0;
			}
			#endif
		}
	}
#else
	for(i = 0; i < PRV_VO_DEV_NUM; i++)
	{
		if(i == SPOT_VO_DEV || i == AD)
			continue;
		for(j = LOCALVEDIONUM; j < DEV_CHANNEL_NUM; j++)
		{
			if(VochnInfo[j].VdecChn == DetVLoss_VdecChn/* && VochnInfo[j].IsBindVdec[i] == -1*/)
			{	
				s32Ret = HI_MPI_VDEC_BindOutput(VochnInfo[j].VdecChn, i, j);
				#if 0
				if(s32Ret != HI_SUCCESS)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "------------Vdec: %d Bind Vo: %d fail\n", VochnInfo[j].VdecChn, j);
				}
				else
				{				
					VochnInfo[j].IsBindVdec[i] = 0;
				}
				#endif
			}
		}
	}
#endif
}

#endif

/*************************************************
Function: //PRV_BindHifbVo
Description: //����GUI�������󶨹�ϵ��
Calls: //
Called By: // Prv_VoInit��Ԥ����ʼ��VI����
Input: // dev: VO�豸ID
		hifb:FB�豸ID
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/ 
static HI_S32 PRV_BindHifbVo(int dev, int hifb)
{
#if defined(SN9234H1)
	VD_BIND_S stBindAtrr;
	int ret;
	int fd;
	
	if (HD != dev && AD != dev)
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV, "bad parameter: dev\n");
		RET_FAILURE("");
	}
	fd = open("/dev/vd", O_RDWR, 0);
	if(fd < 0)
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV, "open vd failed!\n");
		RET_FAILURE("");
	}
	stBindAtrr.DevId = dev;
	stBindAtrr.s32GraphicId = hifb;
	
	ret = ioctl(fd, VD_SET_GRAPHIC_BIND, &stBindAtrr);
	if (ret != 0)
	{
		//fprintf(stderr, "VD_SET_GRAPHIC_BIND error: %d[%#x010]: %s\n", ret, ret, strerror(ret));
		close(fd);
		RET_FAILURE("");
	}
	close(fd);
#elif defined(Hi3535)
	HI_MPI_VO_UnBindGraphicLayer(hifb, dev);
	HI_MPI_VO_UnBindGraphicLayer(3, dev);
	HI_MPI_VO_BindGraphicLayer(hifb, dev);
#else
	VOU_GFX_BIND_LAYER_E enGfxBindLayer;
	if (dev > DSD0)
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV, "bad parameter: dev\n");
		RET_FAILURE("");
	}
	for (enGfxBindLayer = GRAPHICS_LAYER_G4; enGfxBindLayer < GRAPHICS_LAYER_BUTT; ++enGfxBindLayer)
	{
		HI_MPI_VO_GfxLayerUnBindDev(enGfxBindLayer, dev);
	}
	HI_MPI_VO_GfxLayerBindDev(hifb, dev);
#endif	
	RET_SUCCESS("");
}
/*************************************************
Function: //PRV_BindGuiVo
Description: //����GUI�㼴G1ͼ�β������󶨹�ϵ��
Calls: //
Called By: // Prv_VoInit��Ԥ����ʼ��VI����
Input: // dev: VO�豸ID
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/ 
HI_S32 PRV_BindGuiVo(int dev)
{
#if defined(SN9234H1)	
	CHECK_RET(PRV_BindHifbVo(dev, G1));
#endif	
	RET_SUCCESS("");
}
/*************************************************
Function: //PRV_Get_Max_chnnum
Description: //   ���㵱ǰԤ��ģʽ��˳���������ͨ����
Calls: 
Called By: //
Input: //ePreviewMode:Ԥ��ģʽ
		pMax_chn_num:���ص�ǰ���ͨ����
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_Get_Max_chnnum(PRV_PREVIEW_MODE_E ePreviewMode, HI_U32 *pMax_chn_num)
{
	switch(ePreviewMode)
	{		
		case SingleScene:
			*pMax_chn_num = 1;
			break;
		case ThreeScene:
			*pMax_chn_num = 3;
			break;
		case FourScene:
		case LinkFourScene:
			*pMax_chn_num = 4;
			break;
		case FiveScene:
			*pMax_chn_num = 5;
			break;
		case SevenScene:
			*pMax_chn_num = 7;
			break;
		case NineScene:
		case LinkNineScene:
			*pMax_chn_num = 9;
			break;
		case SixteenScene:
			*pMax_chn_num = 16;
			break;
		default:
			RET_FAILURE("Invalid Parameter: enPreviewMode");
	}
		RET_SUCCESS("");
}


int PRV_Check_LinkageGroup(int VoDev,PRV_PREVIEW_MODE_E enPreviewMode)
{
	int i = 0,j = 0,Ret = 0;	
	unsigned int Max_num = 0;
	int MaxVoChn = 0,sqrtVo = 0,VoChn = 0,index0 = 0,index1 = 0,index2 = 0;
	int OsdNameType[DEV_CHANNEL_NUM];
	SN_MEMSET(OsdNameType,0,sizeof(OsdNameType));
	int u32Index = s_astVoDevStatDflt[s_VoDevCtrlDflt].s32PreviewIndex;
	CHECK_RET(PRV_Get_Max_chnnum(enPreviewMode, &Max_num));
	if(PRV_CurDecodeMode == PassiveDecode || ScmGetListCtlState() == 1||s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat != PRV_STAT_NORM||s_astVoDevStatDflt[s_VoDevCtrlDflt].bIsAlarm==HI_TRUE||s_astVoDevStatDflt[s_VoDevCtrlDflt].bIsSingle==HI_TRUE)
	{
		Ret = OSD_Compare_NameType(OsdNameType);
		OSD_Set_NameType(OsdNameType);
		return Ret;
	}
	switch(enPreviewMode)
	{
		case SingleScene:
			MaxVoChn = 1;
			sqrtVo = 0;
			break;
		case TwoScene:
			MaxVoChn = 2;
			sqrtVo = 2;
			break;
		case ThreeScene:
			MaxVoChn = 3;
			sqrtVo = 3;
			break;
		case FourScene:
		case LinkFourScene:
			MaxVoChn = 4;
			sqrtVo = 2;
			break;
		case FiveScene:
			MaxVoChn = 5;
			sqrtVo = 2;
			break;
		case SixScene:
			MaxVoChn = 6;
			sqrtVo = 1;
			break;
		case SevenScene:
			MaxVoChn = 7;
			sqrtVo = 2;
			break;
		case EightScene:
			MaxVoChn = 8;
			sqrtVo = 1;
			break;
		case NineScene:
		case LinkNineScene:
			MaxVoChn = 9;
			sqrtVo = 3;
			break;
		case SixteenScene:
			MaxVoChn = 16;
			sqrtVo = 4;
			break;
		default:
			RET_FAILURE("Invalid Parameter: PRV_UpdateChnPrevState");
	}

	for (i = 0; i < MaxVoChn && u32Index + i < MaxVoChn; i++)
    {
		//��λ���ƺͱ����������£�����ͨ���Ƿ����أ�ͨ��˳����Σ�����ͨ������ʾ
		index0 = -1;
		index1 = -1;
		index2 = -1;
		VoChn = s_astVoDevStatDflt[VoDev].as32ChnOrder[enPreviewMode][(u32Index + i) % Max_num];
		
		//printf("i: %d, u32ChnNum: %d, PRV_CurDecodeMode: %d, Vochn: %d\n", i, u32ChnNum, PRV_CurDecodeMode, VoChn);
		/* �ж�VoChn�Ƿ���Ч */
		if (VoChn < 0 || VoChn >= g_Max_Vo_Num)
		{
			continue;//break;//
		}
		index0 = PRV_GetVoChnIndex(VoChn);
		if(i<MaxVoChn-1)
		{
			VoChn = s_astVoDevStatDflt[VoDev].as32ChnOrder[enPreviewMode][(u32Index + i+1) % Max_num];
			if (VoChn < 0 || VoChn >= g_Max_Vo_Num)
			{
				continue;//break;//
			}
			index1 = PRV_GetVoChnIndex(VoChn);
		}
		if(sqrtVo>2 && i<MaxVoChn-2)
		{
			VoChn = s_astVoDevStatDflt[VoDev].as32ChnOrder[enPreviewMode][(u32Index + i+2) % Max_num];
			if (VoChn < 0 || VoChn >= g_Max_Vo_Num)
			{
				continue;//break;//
			}
			index2 = PRV_GetVoChnIndex(VoChn);
		}
		for(j=0;j<LINKAGE_MAX_GROUPNUM;j++)
		{
			if((g_PrmLinkAge_Cfg[j].DevNum == 3) && (sqrtVo>=3)&&(i%sqrtVo<=sqrtVo-3))
			{
				
				
				if((g_PrmLinkAge_Cfg[j].GroupChn[0].DevChn!=0 && g_PrmLinkAge_Cfg[j].GroupChn[0].SerialNo!=0 \
						&& index0==g_PrmLinkAge_Cfg[j].GroupChn[0].DevChn-1 && SCM_GetChnSwitchSerialNo(index0)==g_PrmLinkAge_Cfg[j].GroupChn[0].SerialNo-1)\
					&&(g_PrmLinkAge_Cfg[j].GroupChn[1].DevChn!=0 && g_PrmLinkAge_Cfg[j].GroupChn[1].SerialNo!=0 \
						&& index1==g_PrmLinkAge_Cfg[j].GroupChn[1].DevChn-1 && SCM_GetChnSwitchSerialNo(index1)==g_PrmLinkAge_Cfg[j].GroupChn[1].SerialNo-1)\
					&&(g_PrmLinkAge_Cfg[j].GroupChn[2].DevChn!=0 && g_PrmLinkAge_Cfg[j].GroupChn[2].SerialNo!=0 \
						&& index2==g_PrmLinkAge_Cfg[j].GroupChn[2].DevChn-1 && SCM_GetChnSwitchSerialNo(index2)==g_PrmLinkAge_Cfg[j].GroupChn[2].SerialNo-1))
				{
					OsdNameType[index0] = j+1;
					OsdNameType[index1] = -1;
					OsdNameType[index2] = -1;
					i += 2;
					break;
				}
			}
			else if((g_PrmLinkAge_Cfg[j].DevNum == 2)&&(sqrtVo>=2)&&(((i%sqrtVo<=sqrtVo-2)&&enPreviewMode!=SevenScene)||((i%sqrtVo!=0)&&enPreviewMode==SevenScene)))
			{
				if(((g_PrmLinkAge_Cfg[j].GroupChn[0].DevChn!=0 && g_PrmLinkAge_Cfg[j].GroupChn[0].SerialNo!=0 && index0==g_PrmLinkAge_Cfg[j].GroupChn[0].DevChn-1 && SCM_GetChnSwitchSerialNo(index0)==g_PrmLinkAge_Cfg[j].GroupChn[0].SerialNo-1)\
						&&(g_PrmLinkAge_Cfg[j].GroupChn[1].DevChn!=0 && g_PrmLinkAge_Cfg[j].GroupChn[1].SerialNo!=0 && index1==g_PrmLinkAge_Cfg[j].GroupChn[1].DevChn-1 && SCM_GetChnSwitchSerialNo(index1)==g_PrmLinkAge_Cfg[j].GroupChn[1].SerialNo-1))\
					||((g_PrmLinkAge_Cfg[j].GroupChn[0].DevChn!=0 && g_PrmLinkAge_Cfg[j].GroupChn[0].SerialNo!=0 && index0==g_PrmLinkAge_Cfg[j].GroupChn[0].DevChn-1 && SCM_GetChnSwitchSerialNo(index0)==g_PrmLinkAge_Cfg[j].GroupChn[0].SerialNo-1)\
						&&(g_PrmLinkAge_Cfg[j].GroupChn[2].DevChn!=0 && g_PrmLinkAge_Cfg[j].GroupChn[2].SerialNo!=0 && index1==g_PrmLinkAge_Cfg[j].GroupChn[2].DevChn-1 && SCM_GetChnSwitchSerialNo(index1)==g_PrmLinkAge_Cfg[j].GroupChn[2].SerialNo-1))\
					||((g_PrmLinkAge_Cfg[j].GroupChn[1].DevChn!=0 && g_PrmLinkAge_Cfg[j].GroupChn[1].SerialNo!=0 && index0==g_PrmLinkAge_Cfg[j].GroupChn[1].DevChn-1 && SCM_GetChnSwitchSerialNo(index0)==g_PrmLinkAge_Cfg[j].GroupChn[1].SerialNo-1)\
						&&(g_PrmLinkAge_Cfg[j].GroupChn[2].DevChn!=0 && g_PrmLinkAge_Cfg[j].GroupChn[2].SerialNo!=0 && index1==g_PrmLinkAge_Cfg[j].GroupChn[2].DevChn-1 && SCM_GetChnSwitchSerialNo(index1)==g_PrmLinkAge_Cfg[j].GroupChn[2].SerialNo-1)))
				{
					if(enPreviewMode == SevenScene)
					{
						printf("SevenScene,i:%d,sqrtVo:%d,%d\n",i,sqrtVo,i%sqrtVo);
						if(i%sqrtVo != 0)
						{
							OsdNameType[index0] = j+1;
							OsdNameType[index1] = -1;
							i++;
							break;
						}
					}
					else
					{
						OsdNameType[index0] = j+1;
						OsdNameType[index1] = -1;
						i++;
						break;
					}
				}
			}
		}
	}

	Ret = OSD_Compare_NameType(OsdNameType);
	OSD_Set_NameType(OsdNameType);
	return Ret;
}

/*************************************************
Function: //PRV_Chn2Index
Description: //   ����ͨ������������λ��
Calls: 
Called By: //
Input: //VoDev:�豸��
		VoChn:ͨ����
		pu32Index:���ص�ǰͨ���Ŷ�Ӧ������
		pOrder : ͨ��˳��
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
****************************************************/
STATIC HI_S32 PRV_Chn2Index(VO_DEV VoDev, VO_CHN VoChn, HI_U32 *pu32Index,VO_CHN *pOrder)
{
	HI_S32 i=0;//,idx=0;
	HI_U32 Max_num;
	if (pu32Index == NULL)
	{
		RET_FAILURE("Invalid Parameter: NULL");
	}
#if defined(SN9234H1)
	if(VoDev == SPOT_VO_DEV|| VoDev == AD)
#else
	if(VoDev > DHD0)
#endif		
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}
#if defined(SN9234H1)
	if (VoDev < 0 || VoDev >= PRV_VO_MAX_DEV)
#else
	if (VoDev < 0 || VoDev > DHD0/*VoDev >= PRV_VO_MAX_DEV*/)
#endif		
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV, TEXT_COLOR_YELLOW("Invalid Parameter: VoDev:%d"), VoDev);
		RET_FAILURE("");
	}

	if (VoChn < 0 || VoChn >= g_Max_Vo_Num)
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV, TEXT_COLOR_YELLOW("Invalid Parameter: VoChn:%d"), VoChn);
	}
	//��ȡ��ǰԤ��ģʽ�����ͨ����
	//CHECK_RET(PRV_Get_Max_chnnum(s_astVoDevStatDflt[VoDev].enPreviewMode,&Max_num));
	//Max_num = SIXINDEX;
	if(PRV_CurDecodeMode == PassiveDecode || ScmGetListCtlState() == 1)
	{
#if defined(SN9234H1)		
		CHECK_RET(PRV_Get_Max_chnnum(s_astVoDevStatDflt[HD].enPreviewMode,&Max_num));
#else
		CHECK_RET(PRV_Get_Max_chnnum(s_astVoDevStatDflt[DHD0].enPreviewMode,&Max_num));
#endif
		if(VoChn < Max_num)
		{
			*pu32Index = VoChn;
			return HI_SUCCESS;
		}
		else
		{
			TRACE(SCI_TRACE_HIGH, MOD_PRV, "PRV_CurDecodeMode == PassiveDecode, VoChn: %d > Max_num: %d\n", VoChn, Max_num);
			return HI_FAILURE;
		}
	}
	Max_num = CHANNEL_NUM;

	for (i = 0; i < Max_num; i++)
	{
		if (VoChn == pOrder[i])
		{
			*pu32Index = i;
			//printf("!!!!!!!!!*pu32Index  = %d  VoChn = %d!!!!!!!!!!!\n",*pu32Index,VoChn);
			break;
		}
	}
	if (SIXINDEX == i)
	{
		RET_FAILURE("VoChn not found! is hiden??");
	}

	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_Point2Index
Description: //   ����ͨ������������λ��
Calls: 
Called By: //
Input: //pstPoint:��ǰ�ĺ�������λ��
		pu32Index:���ص�ǰͨ���Ŷ�Ӧ������
		pOrder : ͨ��˳��
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
****************************************************/
STATIC HI_S32 PRV_Point2Index(const Preview_Point *pstPoint, HI_U32 *pu32Index,VO_CHN *pOrder)
{
	HI_U32 i, u32ChnNum;
	RECT_S *pstLayout = NULL;
	HI_U32 Max_num;
	
	//��ȡ��ǰԤ��ģʽ�����ͨ����
	CHECK_RET(PRV_Get_Max_chnnum(s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewMode,&Max_num));

	if (NULL == pstPoint || pu32Index == NULL)
	{
		RET_FAILURE("Invalid Parameter: NULL");
	}

	if (s_astVoDevStatDflt[s_VoDevCtrlDflt].bIsAlarm)
	{//�Ƿ��ڱ���״̬
		CHECK_RET(PRV_Chn2Index(s_VoDevCtrlDflt, s_astVoDevStatDflt[s_VoDevCtrlDflt].s32AlarmChn, pu32Index,pOrder));
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "---- alarm chn preview ----");
		RET_SUCCESS("");
	}
	if (s_astVoDevStatDflt[s_VoDevCtrlDflt].bIsSingle)
	{//�Ƿ񵥻���ģʽ
		*pu32Index = s_astVoDevStatDflt[s_VoDevCtrlDflt].s32SingleIndex;
		if(PRV_CurDecodeMode == PassiveDecode || ScmGetListCtlState() == 1)
		{
			*pu32Index = DoubleToSingleIndex;
		}
		RET_SUCCESS("");
	}
	if (pstPoint->x < 0 || pstPoint->y <0)
	{
		*pu32Index = s_astVoDevStatDflt[s_VoDevCtrlDflt].s32PreviewIndex;
		if(PRV_CurDecodeMode == PassiveDecode || ScmGetListCtlState() == 1)
		{
			*pu32Index = 0;
		}
		//RET_SUCCESS("---- point not in screen : means using remote control ----");
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, TEXT_COLOR_GREEN("---- point not in screen : means using remote control ----")"X: %d, Y: %d\n", pstPoint->x, pstPoint->y);
		RET_SUCCESS("");
	}

	switch(s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewMode)
	{
		case SingleScene:
			u32ChnNum = 1;
			pstLayout = s_astPreviewLayout1;
			break;
		case TwoScene:
			u32ChnNum = 2;
			pstLayout = s_astPreviewLayout2;
			break;
		case ThreeScene:
			u32ChnNum = 3;
			pstLayout = s_astPreviewLayout3;
			break;
		case FourScene:
		case LinkFourScene:
			u32ChnNum = 4;
			pstLayout = s_astPreviewLayout4;
			break;
		case FiveScene:
			u32ChnNum = 5;
			pstLayout = s_astPreviewLayout5;
			break;
		case SixScene:
			u32ChnNum = 6;
			pstLayout = s_astPreviewLayout6;
			break; 
		case SevenScene:
			u32ChnNum = 7;
			pstLayout = s_astPreviewLayout7;
			break;
		case EightScene:
			u32ChnNum = 8;
			pstLayout = s_astPreviewLayout8;
			break;
		case NineScene:
		case LinkNineScene:
			u32ChnNum = 9;
			pstLayout = s_astPreviewLayout9;
			break;
		case SixteenScene:
			u32ChnNum = 16;
			pstLayout = s_astPreviewLayout16;
			break;
		default:
			RET_FAILURE("Invalid Parameter: enPreviewMode");
	}
	//���ҷ��ϵ�ǰλ�õ�ͨ������ID
	for (i = 0; i<u32ChnNum; i++)
	{
		if (
			(pstLayout[i].s32X * s_u32GuiWidthDflt) / (PRV_PREVIEW_LAYOUT_DIV) <= pstPoint->x
			&& (pstLayout[i].s32Y * s_u32GuiHeightDflt) / PRV_PREVIEW_LAYOUT_DIV <= pstPoint->y
			&& ((pstLayout[i].s32X + pstLayout[i].u32Width) * s_u32GuiWidthDflt) / PRV_PREVIEW_LAYOUT_DIV >= pstPoint->x
			&& ((pstLayout[i].s32Y + pstLayout[i].u32Height) * s_u32GuiHeightDflt) / PRV_PREVIEW_LAYOUT_DIV >= pstPoint->y
			)
		{
			if(PRV_CurDecodeMode == PassiveDecode || ScmGetListCtlState() == 1)
			{
				*pu32Index = i;
				return HI_SUCCESS;
			}
			*pu32Index = i + s_astVoDevStatDflt[s_VoDevCtrlDflt].s32PreviewIndex;
			break;
		}
	}
	//�жϵ�ǰID�Ƿ񳬳���Χ�����ߴ�������ͨ����
	if (u32ChnNum == i || *pu32Index >= Max_num)
	{
		RET_FAILURE("---- Point NOT in any chn! ----");
	}
	if (pOrder[*pu32Index] < 0 || pOrder[*pu32Index] >= g_Max_Vo_Num)
	{
		RET_FAILURE("---- Point is in hiden chn! ----");
	}
	//TRACE(SCI_TRACE_NORMAL, MOD_PRV, TEXT_COLOR_GREEN("%s")": index = " TEXT_COLOR_PURPLE("%d") ", chn = " TEXT_COLOR_PURPLE("%d")  ",x = " TEXT_COLOR_PURPLE("%d") " ,y = " TEXT_COLOR_PURPLE("%d") "\n", __FUNCTION__, *pu32Index, s_astVoDevStatDflt[s_VoDevCtrlDflt].as32ChnOrder[s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewMode][*pu32Index],pstPoint->x,pstPoint->y);
	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_VoChnIsInCurLayOut
Description: //   ָ�����ͨ���Ƿ��ڵ�ǰ��ʾ�Ļ�����
Calls: 
Called By: //
Input: //VoDev:�豸��
		VoChn:ͨ����
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
****************************************************/
//ָ�����ͨ���Ƿ��ڵ�ǰ��ʾ�Ļ�����
int PRV_VoChnIsInCurLayOut(int VoDev, int VoChn)
{
	int u32ChnNum = 0, i = 0;
	PRV_PREVIEW_MODE_E enPreviewMode = s_astVoDevStatDflt[VoDev].enPreviewMode;
	HI_U32 u32Index = (s_astVoDevStatDflt[VoDev].bIsSingle == HI_TRUE) ? s_astVoDevStatDflt[VoDev].s32SingleIndex : s_astVoDevStatDflt[VoDev].s32PreviewIndex;
	HI_U32 Max_num = 0;

#if defined(SN9234H1)
	if(VoDev == SPOT_VO_DEV|| VoDev == AD)
#else
	if(VoDev > DHD0)
#endif		
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}
#if defined(SN9234H1)
	if(VoDev < 0 || VoDev >= PRV_VO_MAX_DEV)
#else
	if (VoDev < 0 || VoDev > DHD0/*VoDev >= PRV_VO_MAX_DEV*/)
#endif
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV, TEXT_COLOR_RED("Invalid VoDev: %d\n"), VoDev);
		RET_FAILURE("");
	}
	if(VoChn < 0 || VoChn >= DEV_CHANNEL_NUM)
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV, TEXT_COLOR_RED("Invalid VoChn: %d\n"), VoChn);
		RET_FAILURE("");
	}
	if(s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_PB || s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_PIC)
		RET_FAILURE("In PB or PIC State");
	if(s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_CTRL)
	{
		if(VoChn == s_astVoDevStatDflt[VoDev].s32CtrlChn)
			return HI_SUCCESS;
		else 
			return HI_FAILURE;
	}
	
	if(s_astVoDevStatDflt[VoDev].bIsAlarm)		
	{
		if(VoChn == s_astVoDevStatDflt[VoDev].s32AlarmChn)
			return HI_SUCCESS;
		else 
			return HI_FAILURE;
	}
	
	if(s_astVoDevStatDflt[VoDev].bIsSingle)
	{
		if((enPreviewMode != SingleScene) && s_astVoDevStatDflt[VoDev].s32DoubleIndex == 0)
			enPreviewMode = SingleScene;
#if(IS_DECODER_DEVTYPE == 1)
		if(PRV_CurDecodeMode == PassiveDecode || ScmGetListCtlState() == 1)
		{
			if(s_astVoDevStatDflt[VoDev].s32DoubleIndex == 1)//˫��״̬���뵥����
			{
				if(VoChn == DoubleToSingleIndex)
					return HI_SUCCESS;
				else
					return HI_FAILURE;

			}
			else//�л��������棬ֻ��ͨ��0�й�
			{
				if(VoChn == 0)
					return HI_SUCCESS;
				else
					return HI_FAILURE;
			}
		}
#endif
		
		if(VoChn == s_astVoDevStatDflt[VoDev].as32ChnOrder[enPreviewMode][s_astVoDevStatDflt[VoDev].s32SingleIndex])
			return HI_SUCCESS;
		else
			return HI_FAILURE;
	}
	if(LayoutToSingleChn == VoChn)
		return HI_SUCCESS;
	//��ȡ��ǰԤ��ģʽ�����ͨ����
	CHECK_RET(PRV_Get_Max_chnnum(s_astVoDevStatDflt[VoDev].enPreviewMode,&Max_num));

	switch(enPreviewMode)
	{
		case SingleScene:
			u32ChnNum = 1;
			break;
		case TwoScene:
			u32ChnNum = 2;
			break;
		case ThreeScene:
			u32ChnNum = 3;
			break;
		case FourScene:
		case LinkFourScene:
			u32ChnNum = 4;
			break;
		case FiveScene:
			u32ChnNum = 5;
			break;
		case SixScene:
			u32ChnNum = 6;
			break;
		case SevenScene:
			u32ChnNum = 7;
			break;
		case EightScene:
			u32ChnNum = 8;
			break;
		case NineScene:
		case LinkNineScene:
			u32ChnNum = 9;
			break;
		case SixteenScene:
			u32ChnNum = 16;
			break;
		default:
			RET_FAILURE("Invalid Parameter: enPreviewMode");
	}
#if(IS_DECODER_DEVTYPE == 1)
	//��������͵�λ�����£�ͨ����˳�����е�
	if(PRV_CurDecodeMode == PassiveDecode || ScmGetListCtlState() == 1)
	{
		if(VoChn < u32ChnNum)
		{
			return HI_SUCCESS;
		}
		else
		{
			return HI_FAILURE;
		}
	}
#endif
	if(s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_NORM)
	{
		for(i = 0; i < u32ChnNum; i++)
		{
			if(VoChn == s_astVoDevStatDflt[VoDev].as32ChnOrder[enPreviewMode][(u32Index+i) % Max_num])
				return HI_SUCCESS;
		}
	}
	return HI_FAILURE;
}
/*************************************************
Function: //PRV_VLossInCurLayOut
Description: //   �жϵ�ǰ���沼�����Ƿ����δ����IPC��ͨ��
Calls: 
Called By: //
Input: //��
Output: //��
Return: //0:����δ����IPC��ͨ��("��������Ƶ")��1:������
Others: // ����˵��
****************************************************/
HI_S32 PRV_VLossInCurLayOut()
{
	int u32ChnNum = 0, i = 0, index = 0;
	VO_CHN VoChn = 0;
#if defined(SN9234H1)
	VO_DEV VoDev = HD;
#else
	VO_DEV VoDev = DHD0;
#endif
	PRV_PREVIEW_MODE_E enPreviewMode = s_astVoDevStatDflt[VoDev].enPreviewMode;
	HI_U32 u32Index = (s_astVoDevStatDflt[VoDev].bIsSingle == HI_TRUE) ? s_astVoDevStatDflt[VoDev].s32SingleIndex : s_astVoDevStatDflt[VoDev].s32PreviewIndex;
	HI_U32 Max_num;
	//��ȡ��ǰԤ��ģʽ�����ͨ����
	CHECK_RET(PRV_Get_Max_chnnum(s_astVoDevStatDflt[VoDev].enPreviewMode,&Max_num));
	if(s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_CTRL)
	{
		index = PRV_GetVoChnIndex(s_astVoDevStatDflt[VoDev].s32CtrlChn);
		if(index < 0)
			RET_FAILURE("------ERR: Invalid Index!");
		if(VochnInfo[index].VdecChn == DetVLoss_VdecChn)
			return HI_SUCCESS;
		else 
			return HI_FAILURE;
	}
	
	if(s_astVoDevStatDflt[VoDev].bIsAlarm)		
	{
		index = PRV_GetVoChnIndex(s_astVoDevStatDflt[VoDev].s32AlarmChn);
		if(index < 0)
			RET_FAILURE("------ERR: Invalid Index!");
		if(VochnInfo[index].VdecChn == DetVLoss_VdecChn)
			return HI_SUCCESS;
		else 
			return HI_FAILURE;
	}
	
	if(s_astVoDevStatDflt[VoDev].bIsSingle)
	{
		if((enPreviewMode != SingleScene) && s_astVoDevStatDflt[VoDev].s32DoubleIndex == 0)
			enPreviewMode = SingleScene;

		index = PRV_GetVoChnIndex(s_astVoDevStatDflt[VoDev].as32ChnOrder[enPreviewMode][s_astVoDevStatDflt[VoDev].s32SingleIndex]);
		if(index < 0)
			RET_FAILURE("------ERR: Invalid Index!");
		if(VochnInfo[index].VdecChn == DetVLoss_VdecChn)
			return HI_SUCCESS;
		else
			return HI_FAILURE;
	}
	else if(LayoutToSingleChn == VoChn)
	{
		index = PRV_GetVoChnIndex(VoChn);
		if(index < 0)
			RET_FAILURE("------ERR: Invalid Index!");
		if(VochnInfo[index].VdecChn == DetVLoss_VdecChn)
			return HI_SUCCESS;
		else
			return HI_FAILURE;
		
	}

	switch(enPreviewMode)
	{
		case SingleScene:
			u32ChnNum = 1;
			break;
		case TwoScene:
			u32ChnNum = 2;
			break;
		case ThreeScene:
			u32ChnNum = 3;
			break;
		case FourScene:
		case LinkFourScene:
			u32ChnNum = 4;
			break;
		case FiveScene:
			u32ChnNum = 5;
			break;
		case SixScene:
			u32ChnNum = 6;
			break;
		case SevenScene:
			u32ChnNum = 7;
			break;
		case EightScene:
			u32ChnNum = 8;
			break;
		case NineScene:
		case LinkNineScene:
			u32ChnNum = 9;
			break;
		case SixteenScene:
			u32ChnNum = 16;
			break;
		default:
			RET_FAILURE("Invalid Parameter: enPreviewMode");
	}

	for(i = 0; i < u32ChnNum; i++)
	{
#if defined(SN9234H1)
		VoChn = s_astVoDevStatDflt[HD].as32ChnOrder[enPreviewMode][(u32Index+i) % Max_num];
#else		
		VoChn = s_astVoDevStatDflt[DHD0].as32ChnOrder[enPreviewMode][(u32Index+i) % Max_num];
#endif
		index = PRV_GetVoChnIndex(VoChn);
		if(index < 0)
			continue;		
		if(VochnInfo[index].VdecChn == DetVLoss_VdecChn)
			return HI_SUCCESS;
	}
	return HI_FAILURE;

}
#if (IS_DECODER_DEVTYPE == 1)

#else
/*************************************************
Function: //PRV_GetValidChnIdx
Description: ������һ������һ������ʾ��ͨ��������
Calls: 
Called By: //
Input: //VoDev:�豸��
		u32Index:��ǰ������
		pu32Index:��������ʾ������
		s32Dir :��һ������һ������
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_GetValidChnIdx(VO_DEV VoDev, HI_U32 u32Index, HI_S32 *pu32Index, HI_S32 s32Dir,VO_CHN *pOrder,VO_CHN *pPollOrder)
{
	int i;
	VO_CHN VoChn;
	HI_U32 Max_num;
		
	//��ȡ��ǰԤ��ģʽ�����ͨ����
	CHECK_RET(PRV_Get_Max_chnnum(s_astVoDevStatDflt[VoDev].enPreviewMode,&Max_num));
	
	if (NULL == pu32Index)
	{
		RET_FAILURE("Null ptr!");
	}
	if (u32Index >= Max_num)
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV, TEXT_COLOR_RED("bad u32Index: %d\n"), u32Index);
		RET_FAILURE("");
	}
#if defined(SN8608D_LE) || defined(SN8608M_LE) || defined(SN8616D_LE) ||defined(SN8616M_LE) || defined(SN9234H1)
	if(VoDev == SPOT_VO_DEV|| VoDev == AD)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}
#endif
	if (VoDev<0 || VoDev>=PRV_VO_MAX_DEV)
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV, TEXT_COLOR_RED("bad VoDev: %d\n"), VoDev);
		RET_FAILURE("");
	}
				
	if (0 == s32Dir)
	{
		for (i=1; i<=Max_num; i++)
		{
			VoChn = pOrder[(i+u32Index)%Max_num];
			if (VoChn>=0 && VoChn<g_Max_Vo_Num)
			{
				if(pPollOrder[(i+u32Index)%Max_num])
				{//�����ǰ������Ҫ��ѯ
					*pu32Index = (i+u32Index)%Max_num;
					//printf("###############VoChn = %d,,,,,,*pu32Index =%d######################\n",VoChn,*pu32Index);
					RET_SUCCESS("");
				}	
			}
		}
	}
	else
	{
		for (i=1; i<=Max_num; i++)
		{
			VoChn = pOrder[(PRV_VO_CHN_NUM+u32Index-i)%Max_num];
			if (VoChn>=0 && VoChn<g_Max_Vo_Num)
			{
				if(pPollOrder[(Max_num+u32Index-i)%Max_num])
				{//�����ǰ������Ҫ��ѯ
					*pu32Index = (PRV_VO_CHN_NUM+u32Index-i)%Max_num;
					RET_SUCCESS("");
				}
			}
		}
	}
	RET_FAILURE("No valid chn available!");
}

/*************************************************
Function: //PRV_GetFirstChn
Description://  ��ȡVO����ĵ�1������ͨ����
Calls: 
Called By: //
Input: //VoDev:�豸��
		pVoChn:���ص�ͨ����
Output: // pVoChn:���ص�ͨ����
Return: //�ɹ�����HI_SUCCESS
		ʧ�ܷ���HI_FAILURE
Others: // ����˵��
************************************************************************/

HI_S32 PRV_GetFirstChn(VO_DEV VoDev, VO_CHN *pVoChn)
{
	if (NULL == pVoChn)
	{
		RET_FAILURE("Null ptr!");
	}
//1���ж�״̬��enPreviewStat
	switch (s_astVoDevStatDflt[VoDev].enPreviewStat)
	{
		case PRV_STAT_NORM:
		{
			if(s_astVoDevStatDflt[VoDev].bIsAlarm)
			{//����״̬
				*pVoChn = s_astVoDevStatDflt[VoDev].s32AlarmChn;
			}
			else if (s_astVoDevStatDflt[VoDev].enPreviewMode == SingleScene || (s_astVoDevStatDflt[VoDev].enPreviewMode != SingleScene  && s_astVoDevStatDflt[VoDev].bIsSingle &&  (s_astVoDevStatDflt[VoDev].s32DoubleIndex==0)))
			{//������ڵ�����״̬
				
				*pVoChn = s_astVoDevStatDflt[VoDev].as32ChnOrder[SingleScene][s_astVoDevStatDflt[VoDev].s32SingleIndex];
			}
			else if(s_astVoDevStatDflt[VoDev].enPreviewMode != SingleScene  && s_astVoDevStatDflt[VoDev].bIsSingle && s_astVoDevStatDflt[VoDev].s32DoubleIndex)
			{//����������˫��״̬��
				
				*pVoChn = s_astVoDevStatDflt[VoDev].as32ChnOrder[s_astVoDevStatDflt[VoDev].enPreviewMode][s_astVoDevStatDflt[VoDev].s32SingleIndex];
			}
			else
			{//������ڶ໭����
				*pVoChn = s_astVoDevStatDflt[VoDev].as32ChnOrder[s_astVoDevStatDflt[VoDev].enPreviewMode][s_astVoDevStatDflt[VoDev].s32PreviewIndex];
			}
		}	
			break;
		case PRV_STAT_PB:
		case PRV_STAT_PIC:	
		{
			RET_FAILURE("in pb or pic stat!");
		}	
			break;
		case PRV_STAT_CTRL:
		{
			*pVoChn = s_astVoDevStatDflt[VoDev].s32CtrlChn;
		}
			break;
		default:
			RET_FAILURE("err stat!");
			break;
	}	
	//printf("######*pVoChn =%d,s_astVoDevStatDflt[VoDev].enPreviewMode=%d,s_astVoDevStatDflt[VoDev].bIsSingle=%d ,s_astVoDevStatDflt[VoDev].s32DoubleIndex =%d ,s_astVoDevStatDflt[VoDev].s32SingleIndex=%d##################\n",*pVoChn ,s_astVoDevStatDflt[VoDev].enPreviewMode,s_astVoDevStatDflt[VoDev].bIsSingle,s_astVoDevStatDflt[VoDev].s32DoubleIndex ,s_astVoDevStatDflt[VoDev].s32SingleIndex);
	RET_SUCCESS("");
}
#endif
/*************************************************
Function: //PRV_GetVoChnIndex
Description: //  �������ͨ���ţ����Ҷ�Ӧ��VochnInfo�����±�
Calls: 
Called By: //
Input: // // VoChn���ͨ����
Output: // ��
Return: //-1δ�ҵ�������:��Ӧ���±�
Others: // ����˵��
************************************************************************/

HI_S32 PRV_GetVoChnIndex(int VoChn)
{
	int index = -1, i = 0;

	for(i = 0; i < DEV_CHANNEL_NUM; i++)
	{
		if(VoChn == VochnInfo[i].VoChn)
		{
			index = i;
			break;
		}
	}
	return index;
}
//
HI_S32 PRV_DisableDigChnAudio(HI_VOID)
{
#if defined(SN9234H1)	
	AUDIO_DEV AoDev = 0;
	AO_CHN AoChn = 0;
	//if(IsAdecBindAo)
	{
		(HI_MPI_AO_UnBindAdec(AoDev, AoChn, DecAdec));
		//IsAdecBindAo = 0;
	}
#else
#if defined(Hi3535)
	AUDIO_DEV AoDev = 0;
#else
	AUDIO_DEV AoDev = 4;
#endif
	AO_CHN AoChn = 0;
	if(IsAdecBindAo)
	{
		CHECK(PRV_AUDIO_AoUnbindAdec(AoDev, AoChn, DecAdec));
		IsAdecBindAo = 0;
	}
#endif	
	Achn = -1;
	return HI_SUCCESS;
}



/*************************************************
Function: //PRV_PlayAudio
Description: //  ������Ƶ
			���ҵ�ǰԤ��ģʽ�µ�һ���л��������ͨ�����һ��ģ��ͨ����
			�����Ŵ�ͨ����Ӧ����Ƶ
Calls: 
Called By: //
Input: // // VoDev����豸
Output: // ��
Return: //�ĵ�������
Others: // ����˵��
************************************************************************/
HI_S32 PRV_PlayAudio(VO_DEV VoDev)
{

	TRACE(SCI_TRACE_NORMAL, MOD_PRV, "%s line:%d, Achn=%d, CurAudioChn=%d, PreAudioChn=%d, IsChoosePlayAudio=%d", __FUNCTION__, __LINE__, Achn, CurAudioChn, PreAudioChn, IsChoosePlayAudio);

	VO_CHN VoChn = 0; 
#if defined(Hi3531)
	AUDIO_DEV AoDev = 4;
#else
	AUDIO_DEV AoDev = 0;
#endif
	AO_CHN AoChn = 0;
	HI_S32 index1 = 0, index2 = 0, bIsVoiceTalkOn = 0;
	HI_U32 Max_num = 0, u32ChnNum = 0;
	PRV_PREVIEW_MODE_E	enPreviewMode = s_astVoDevStatDflt[VoDev].enPreviewMode;

	if(HI_TRUE == PRV_GetVoiceTalkState())
	{
		bIsVoiceTalkOn = 1;
		return HI_SUCCESS;
	}
	//������������Ƶ��������Ч
	if(PRV_CurDecodeMode == PassiveDecode)
		return HI_SUCCESS;
	//��ʱ�ط��£�ֻ�طż�ʱ�ط�ͨ����Ƶ������������Ч
	g_PlayInfo	stPlayInfo;
	PRV_GetPlayInfo(&stPlayInfo);
	if(stPlayInfo.PlayBackState == PLAY_INSTANT)
		return HI_SUCCESS;
	//��ȡ��ǰԤ��ģʽ�����ͨ����
	CHECK_RET(PRV_Get_Max_chnnum(s_astVoDevStatDflt[VoDev].enPreviewMode, &Max_num));
	
	if(s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_PB
		|| s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_PIC)
	{
		Achn = -1;
#if defined(SN9234H1)
		//if(IsAdecBindAo)
		{
			(HI_MPI_AO_UnBindAdec(AoDev, AoChn, DecAdec));
			//IsAdecBindAo = 0;
		}
#else
		if(IsAdecBindAo)
		{
			CHECK(PRV_AUDIO_AoUnbindAdec(AoDev, AoChn, DecAdec));
			IsAdecBindAo = 0;
		}
#endif		
		if(LOCALVEDIONUM > 0)
			PRV_DisableAudioPreview();
		RET_FAILURE("In PB or PIC Stat");
	}
	else if(s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_CTRL)
	{
		VoChn = s_astVoDevStatDflt[VoDev].s32CtrlChn;
		index1 = PRV_GetVoChnIndex(VoChn);
	}	
	else
	{
		if(s_astVoDevStatDflt[VoDev].bIsAlarm)		
		{
			VoChn = s_astVoDevStatDflt[VoDev].s32AlarmChn;
			index1 = PRV_GetVoChnIndex(VoChn);
		}

		else if(s_astVoDevStatDflt[VoDev].bIsSingle)
		{
			//����˫�����뵥����ͻ��沼���л����뵥����
			if(enPreviewMode != SingleScene && s_astVoDevStatDflt[VoDev].s32DoubleIndex == 0)
				enPreviewMode = SingleScene;

			VoChn = s_astVoDevStatDflt[VoDev].as32ChnOrder[enPreviewMode][s_astVoDevStatDflt[VoDev].s32SingleIndex];
			index1 = PRV_GetVoChnIndex(VoChn);
		}
		else if(LayoutToSingleChn >= 0)
		{
			VoChn = LayoutToSingleChn;
			index1 = PRV_GetVoChnIndex(VoChn);			
		}		
		else if(s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_NORM)
		{
			switch(enPreviewMode)
			{
				case SingleScene:
					u32ChnNum = 1;
					VoChn = s_astVoDevStatDflt[VoDev].AudioChn[0];
					break;
				case ThreeScene:
					u32ChnNum = 3;
					VoChn = s_astVoDevStatDflt[VoDev].AudioChn[4];
					break;
				case FourScene:
				case LinkFourScene:
					u32ChnNum = 4;
					VoChn = s_astVoDevStatDflt[VoDev].AudioChn[1];
					break;
				case FiveScene:
					u32ChnNum = 5;
					VoChn = s_astVoDevStatDflt[VoDev].AudioChn[5];
					break;
				case SevenScene:
					u32ChnNum = 7;
					VoChn = s_astVoDevStatDflt[VoDev].AudioChn[6];
					break;
				case NineScene:
				case LinkNineScene:
					u32ChnNum = 9;
					VoChn = s_astVoDevStatDflt[VoDev].AudioChn[2];
					break;
				case SixteenScene:
					u32ChnNum = 16;
					VoChn = s_astVoDevStatDflt[VoDev].AudioChn[3];
					break;
				default:
					RET_FAILURE("Invalid Parameter: enPreviewMode");
			}
			index1 = PRV_GetVoChnIndex(VoChn);			
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "enPreviewMode: %d----index1: %d, VoChn: %d, IsChoosePlayAudio: %d\n", enPreviewMode, index1, VoChn, IsChoosePlayAudio);
			if(index1 < 0 && IsChoosePlayAudio != 1)//δ������δѡ����Ƶͨ��
			{
				Achn = -1;
				CurAudioChn= -1;
#if defined(SN9234H1)
				//if(IsAdecBindAo)
				{
					(HI_MPI_AO_UnBindAdec(AoDev, AoChn, DecAdec));
					//IsAdecBindAo = 0;
				}
#else
				if(IsAdecBindAo)
				{
					CHECK(PRV_AUDIO_AoUnbindAdec(AoDev, AoChn, DecAdec));
					IsAdecBindAo = 0;
				}
#endif
				RET_FAILURE("Not Config Audio Chn");
			}
		
		}
		
		if(index1 < 0 && IsChoosePlayAudio != 1)
			RET_FAILURE("------ERR: Invalid Index1!");
	
	}

	
	if(index1 >= 0 && VochnInfo[index1].IsHaveVdec//Ϊ����ͨ��������Ƶ����ͨ��
		&& VochnInfo[index1].AudioInfo.adoType != -1
		&& !IsCreateAdec)//��һ�δ�����Ƶ����ͨ��
	{		
		//��Ƶ����ͨ��
		//printf("----------Create Audio channel--%d!!\n", Achn);
		if(HI_SUCCESS == PRV_StartAdecAo(VochnInfo[index1]))
		{
			IsCreateAdec = 1;
#if defined(Hi3531)||defined(Hi3535)			
			IsAdecBindAo = 1;
#endif
			Achn = index1;
			CurPlayAudioInfo = VochnInfo[index1].AudioInfo;
		}
		else
		{
			IsCreateAdec = 0;
#if defined(Hi3531)||defined(Hi3535)			
			IsAdecBindAo = 0;
#endif
			Achn = -1;
			RET_FAILURE("Create Adec");
		}
		
		if(bIsVoiceTalkOn)
		{
#if defined(SN9234H1)
			HI_MPI_AO_UnBindAdec(AoDev, AoChn, DecAdec);
#else
			if(IsAdecBindAo)
			{
				CHECK(PRV_AUDIO_AoUnbindAdec(AoDev, AoChn, DecAdec));
				IsAdecBindAo = 0;
			}
#endif			
		}
		
	}
	//�û�����ѡ�񲥷���Ƶ��ͨ���󣬻����л�ʱ�������û�ѡ�����Ƶ
	index2 = PRV_GetVoChnIndex(PreAudioChn);
	if(index2 >= 0
		&& 1 == IsChoosePlayAudio
		&& HI_SUCCESS == PRV_VoChnIsInCurLayOut(VoDev, VochnInfo[index2].VoChn))
	{
		//if(VochnInfo[index2].AudioInfo.adoType != CurPlayAudioInfo.adoType	//�������͸ı�
		//	|| VochnInfo[index2].AudioInfo.PtNumPerFrm != CurPlayAudioInfo.PtNumPerFrm)//ÿ֡����������ı�
		if(VochnInfo[index2].AudioInfo.adoType >= 0)
		{
			if(IsCreateAdec)
			{
				CHECK(PRV_StopAdec());
				CHECK(PRV_StartAdecAo(VochnInfo[index2]));
				IsCreateAdec = 1;
			}
			CurPlayAudioInfo = VochnInfo[index2].AudioInfo;
			
		}
		else
		{
			return HI_FAILURE;
		}
#if defined(SN9234H1)
		//if(!IsAdecBindAo)
		{
			(HI_MPI_AO_BindAdec(AoDev, AoChn, DecAdec));
			//IsAdecBindAo = 1;
		}
#else
		if(!IsAdecBindAo)
		{
			CHECK(PRV_AUDIO_AoBindAdec(AoDev, AoChn, DecAdec));
			
			IsAdecBindAo = 1;
		}
#endif		
		//�û�֮ǰѡ�񲥷ŵ�ͨ���ڵ�ǰ�����У��򱣳ִ˻�������ͨ���Ĳ���״̬
		CurAudioChn = PreAudioChn;
		Achn = ((CurAudioPlayStat>>CurAudioChn)&0x01) ? index2 : -1;
		CurAudioPlayStat = 0;
		if(Achn >= 0)
			CurAudioPlayStat = 1<<CurAudioChn;
		if(bIsVoiceTalkOn)
		{
#if defined(SN9234H1)
			HI_MPI_AO_UnBindAdec(AoDev, AoChn, DecAdec);
#else
			if(IsAdecBindAo)
			{
				CHECK(PRV_AUDIO_AoUnbindAdec(AoDev, AoChn, DecAdec));
				IsAdecBindAo = 0;
			}
#endif			
		}
		return HI_SUCCESS;
	}
	
	if(index1 < 0)
		RET_FAILURE("------ERR: Invalid Index1!");
	
	if(VochnInfo[index1].IsLocalVideo)//ģ��ͨ��
	{	
#if defined(SN9234H1)
		//if(IsAdecBindAo)
		{
			(HI_MPI_AO_UnBindAdec(AoDev, AoChn, DecAdec));
			Achn = -1;
			//IsAdecBindAo = 0;
		}
#else
		if(IsAdecBindAo)
		{
			CHECK(PRV_AUDIO_AoUnbindAdec(AoDev, AoChn, DecAdec));
			Achn = -1;
			IsAdecBindAo = 0;
		}
#endif		
		if(s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_NORM 
			|| s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_CTRL)
		{
			PRV_AudioPreviewCtrl((const unsigned char *)&VoChn, 1);
		}
		else
		{
			PRV_DisableAudioPreview();
		}
	}
	else//����ͨ��
	{
		if(IsAudioOpen //��Ƶ�ܿ��ؿ���
			&& VochnInfo[index1].IsHaveVdec)//������
		{
			if(index1 != Achn)//ͨ���Ÿı�
			{
				//if((VochnInfo[index1].AudioInfo.adoType != 0 && VochnInfo[index1].AudioInfo.adoType != CurPlayAudioInfo.adoType	)//�������͸ı�
				//	|| VochnInfo[index1].AudioInfo.PtNumPerFrm != CurPlayAudioInfo.PtNumPerFrm)//ÿ֡����������ı�
				if(VochnInfo[index1].AudioInfo.adoType > -1)
				{
					if(IsCreateAdec)
					{
						CHECK(PRV_StopAdec());
						IsCreateAdec = 0;
						CHECK(PRV_StartAdecAo(VochnInfo[index1]));
						IsCreateAdec = 1;
					}
					
#if defined(Hi3531)||defined(Hi3535)					
					IsAdecBindAo = 1;
#endif
					CurPlayAudioInfo = VochnInfo[index1].AudioInfo;					
				}
				else
				{
					Achn = -1;
					return HI_FAILURE;
				}
			}			
			CurAudioChn = VochnInfo[index1].VoChn;
			CurAudioPlayStat |= (1<<CurAudioChn);
			
			if(s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_NORM 
				|| s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_CTRL)
			{
#if defined(SN9234H1)
				//if(!IsAdecBindAo)
				{
					(HI_MPI_AO_BindAdec(AoDev, AoChn, DecAdec));
					//IsAdecBindAo = 1;
				}
#else
				if(!IsAdecBindAo)
				{
					CHECK(PRV_AUDIO_AoBindAdec(AoDev, AoChn, DecAdec));
					IsAdecBindAo = 1;
				}
#endif				
			}
			Achn = index1;//��������󶨲�����ֻ����������������ͨ������Ƶ����				

		}
		else
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "IsAudioOpen: %d, VochnInfo[index1].IsHaveVdec: %d\n", IsAudioOpen, VochnInfo[index1].IsHaveVdec);
			CurAudioChn = -1;
			Achn = -1;
		}
		
		if(bIsVoiceTalkOn)
		{
#if defined(SN9234H1)
			HI_MPI_AO_UnBindAdec(AoDev, AoChn, DecAdec);
#else
			if(IsAdecBindAo)
			{
				CHECK(PRV_AUDIO_AoUnbindAdec(AoDev, AoChn, DecAdec));
				IsAdecBindAo = 0;
			}
#endif			
		}
		
	}
	return HI_SUCCESS;
}

//�ṩ��MMI���ã���ȡָ��ͨ���ŵ���Ƶ״̬
//MMI���ݷ���ֵ����Ӧ�ı�ָ��ͨ������Ƶ����Сͼ��״̬(���û򲻿���)
int PRV_GetAudioState(int chn)
{
	if(chn < 0 || chn >= DEV_CHANNEL_NUM)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "---------Invalid channel: %d, line: %d\n", chn, __LINE__);
		return HI_FAILURE;		
	}
	if(PRV_CurDecodeMode == PassiveDecode)
		return HI_FAILURE;
	
	HI_S32 s32Ret = 0, index = 0;
	index = PRV_GetVoChnIndex(chn);
	if(index < 0)
		RET_FAILURE("------ERR: Invalid Index!");

	if(!IsAudioOpen)//��Ƶ�ܿ�������ǹر�
	{	
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Not Open Audio!!\n");
		return HI_FAILURE;
	}
	
	if(PRV_GetVoiceTalkState())
	{	
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "VoiceTalk Open Now!!\n");
		return HI_FAILURE;
	}
	if(VochnInfo[index].IsLocalVideo)//����ģ��ͨ��
	{
		s32Ret = PRV_GetLocalAudioState(chn);
		if(HI_FAILURE == s32Ret)
			return s32Ret;
		else if(CurAudioChn == VochnInfo[index].VoChn)
		{
			if((CurAudioPlayStat>>CurAudioChn)&0x01)//����״̬
				return 1;
			else//û�в���
				return 0;
		}
		
	}
	else//����ͨ��
	{
		if(IsCreateAdec == 0)
		{
			PRV_StopAdec();
			VochnInfo[chn].AudioInfo.PtNumPerFrm = PtNumPerFrm;
			PRV_StartAdecAo(VochnInfo[chn]);	
			IsCreateAdec = 1;
			HI_MPI_ADEC_ClearChnBuf(DecAdec);
		}
		
		if((VochnInfo[index].VdecChn == DetVLoss_VdecChn || VochnInfo[index].VdecChn == NoConfig_VdecChn))//����Ƶ�ź�
		{		
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "---------VochnInfo[index].VdecChn: %d\n", VochnInfo[index].VdecChn);
			return HI_FAILURE;
		}
		if(VochnInfo[index].AudioInfo.adoType < 0 || 0 == VochnInfo[index].AudioInfo.PtNumPerFrm)//��Ӧ����Ƶ�������Ͳ������Ϸ�
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "---------Invalid Audio Para---index: %d, adoType: %d, PtNumPerFrm: %d\n", index, VochnInfo[index].AudioInfo.adoType, VochnInfo[index].AudioInfo.PtNumPerFrm);
			return HI_FAILURE;
		}
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "-----------VoChn: %d, CurAudioChn: %d\n", VochnInfo[index].VoChn, CurAudioChn);
		if(CurAudioChn == VochnInfo[index].VoChn)
		{
			if((CurAudioPlayStat>>CurAudioChn)&0x01)//����״̬
				return 1;
			else//û�в���
				return 0;
		}
	}
	return HI_SUCCESS;
}


//ָ��ͨ������ƵСͼ��Ϊ���ã�����ͨ��������/�ر�ָ��ͨ����Ƶ
HI_S32 PRV_AudioOutputChange(int chn)
{
	int index = 0;
	if(chn < 0 || chn > CHANNEL_NUM)	
	{
		Achn = -1;
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------chn: %d\n", chn);
	}	
	index = PRV_GetVoChnIndex(chn);
	if(index < 0)
		RET_FAILURE("------ERR: Invalid Index!");
	if(VochnInfo[index].IsLocalVideo)//����ģ��ͨ��
	{
		PRV_LocalAudioOutputChange(chn);
		CurAudioChn = VochnInfo[index].VoChn;
		PreAudioChn = CurAudioChn;
		CurAudioPlayStat = 1<<CurAudioChn;
	}
	else
	{	
		if(CurAudioChn != VochnInfo[index].VoChn)//ָ����ͨ�����ǵ�ǰ������Ƶ��ͨ��
		{			
			//if(VochnInfo[index].AudioInfo.adoType != CurPlayAudioInfo.adoType	//�������͸ı�
			//	|| VochnInfo[index].AudioInfo.PtNumPerFrm != CurPlayAudioInfo.PtNumPerFrm)//ÿ֡����������ı�
			if(VochnInfo[index].AudioInfo.adoType >= 0)
			{
				//�ؽ���Ƶ����ͨ��
				if(IsCreateAdec)
				{
					CHECK(PRV_StopAdec());
					IsCreateAdec = 0;
					CHECK(PRV_StartAdecAo(VochnInfo[index]));
					IsCreateAdec = 1;
					HI_MPI_ADEC_ClearChnBuf(DecAdec);
				}
				CurPlayAudioInfo = VochnInfo[index].AudioInfo;
								
			}
			else
			{
				return HI_FAILURE;
			}
			CurAudioChn = VochnInfo[index].VoChn;
			PreAudioChn = CurAudioChn;
			CurAudioPlayStat = 0;//����ѡ�񲥷���Ƶͨ�������֮ǰ����״̬����������
			CurAudioPlayStat = 1<<CurAudioChn;
			Achn = index;
		}
		else//ָ����ͨ���������ڲ�����Ƶ��ͨ��������/�ر�ָ��ͨ����Ƶ
		{
			if((CurAudioPlayStat>>CurAudioChn)&(0x01))//�����ǰ�ǿ����ģ���ر�
			{
				CurAudioPlayStat = 0;
				Achn = -1;	
				CurAudioChn = 0;

			}
			else//��ǰ�ǹرգ�����
			{
				CurAudioPlayStat = 1<<CurAudioChn;			
				Achn = index;			
			}
		}
	}
	//printf("--------Achn: %d, CurAudioChn: %d,  CurAudioPlayStat: %d\n", Achn, CurAudioChn, CurAudioPlayStat);
	RET_SUCCESS("");
}

HI_S32 PRV_EnableDigChnAudio(HI_VOID)
{
#if defined(SN9234H1)
	AUDIO_DEV AoDev = 0;
	AO_CHN AoChn = 0;
	HI_S32 s32Ret = 0;
	//if(!IsAdecBindAo)
	{
		(HI_MPI_AO_BindAdec(AoDev, AoChn, DecAdec));
		//IsAdecBindAo = 1;
	}
#else
#if defined(Hi3535)
	AUDIO_DEV AoDev = 0;
#else
	AUDIO_DEV AoDev = 4;
#endif
	AO_CHN AoChn = 0;
	HI_S32 s32Ret = 0;
	if(!IsAdecBindAo)
	{
		CHECK_RET(PRV_AUDIO_AoBindAdec(AoDev, AoChn, DecAdec));
		IsAdecBindAo = 1;
	}
#endif	
	s32Ret = PRV_GetAudioState(CurAudioChn);//�ж�֮ǰ���ŵ���Ƶͨ������״̬
	if(s32Ret == 1)
		Achn = CurAudioChn;
	else
		Achn = -1;
	
	return HI_SUCCESS;
}
 /*************************************************
  Function: //PRV_ReSetVoRect
  Description: //  ���ݴ����Ԥ����ʾģʽ�Լ���ƵԴ�ķֱ��ʣ����¼���Vo�������ʾ����
  Calls: 
  Called By: //
  Input:int output_mode: Ԥ����ʾģʽ���ο�ö��PRV_PreviewOutMode_enum��
		  width: ��ƵԴ�ֱ��ʵĿ�
 		  hight: ��ƵԴ�ֱ��ʵĸߣ�
  		  src: ��ǰVo����ʾ����ȡ����ͨ����ʾλ�ã�

  Return: /���¼�������ʾ����
  Others: //���漰Vo��ʾ���������ʱ����Ҫ���ݵ�ǰԤ��ģʽ����ƵԴ�ֱ��ʣ����ô˽ӿ����»�ȡVo�ľ�����ʾ����
  ************************************************************************/

RECT_S PRV_ReSetVoRect(int output_mode, int width, int hight, RECT_S src)
{
	int stwidth = 0;
	int sthight = 0; 
	RECT_S dest;

	if(src.u32Width <= 0 || src.u32Height <= 0
		|| width <= 0 || hight <= 0)
		return src;
	//printf("++++++output_mode: %d\n", output_mode);
	//printf("------src.s32X: %d, src.s32Y: %d, src.u32Width: %d, src.u32Height: %d\n", src.s32X, src.s32Y, src.u32Width, src.u32Height);	
	//printf("------width: %d, hightt: %d\n", width, hight);	
	switch(output_mode)
	{
		case StretchMode:
			dest = src;
			break;
		case SameScaleMole:
		{
			if(src.u32Width * hight >= width * src.u32Height)//ͨ�������/�߱�ֵ������ƵԴ�ֱ��ʱ�ֵ
			{
				stwidth = src.u32Height * width / hight;
				dest.s32X      = (src.u32Width - stwidth) / 2 + src.s32X;
				dest.s32Y      = src.s32Y;
				dest.u32Width  = stwidth;
				dest.u32Height = src.u32Height;
			}		
			else
			{
				sthight = src.u32Width * hight / width;
				dest.s32X      = src.s32X;
				dest.s32Y      = (src.u32Height - sthight) / 2 + src.s32Y;
				dest.u32Width  = src.u32Width;
				dest.u32Height = sthight;						
			}
		}
			break;

		case IntelligentMode:
            if(width > hight && src.u32Width < src.u32Height)
            {
                if(src.u32Width * hight >= width * src.u32Height)//ͨ�������/�߱�ֵ������ƵԴ�ֱ��ʱ�ֵ
			    {
				   stwidth = src.u32Height * width / hight;
				   dest.s32X      = (src.u32Width - stwidth) / 2 + src.s32X;
				  dest.s32Y      = src.s32Y;
				  dest.u32Width  = stwidth;
				  dest.u32Height = src.u32Height;
			    }		
			    else
			    {
				  sthight = src.u32Width * hight / width;
				  dest.s32X      = src.s32X;
				  dest.s32Y      = (src.u32Height - sthight) / 2 + src.s32Y;
				  dest.u32Width  = src.u32Width;
				  dest.u32Height = sthight;						
			    }
			}
			else if(width > hight && src.u32Width >= src.u32Height)
			{
				dest = src;
			}
			else if(width < hight && src.u32Width < src.u32Height)
			{
                 dest = src;
			}
			else
			{
				if(src.u32Width * hight >= width * src.u32Height)//ͨ�������/�߱�ֵ������ƵԴ�ֱ��ʱ�ֵ
			    {
				   stwidth = src.u32Height * width / hight;
				   dest.s32X      = (src.u32Width - stwidth) / 2 + src.s32X;
				  dest.s32Y      = src.s32Y;
				  dest.u32Width  = stwidth;
				  dest.u32Height = src.u32Height;
			    }		
			    else
			    {
				  sthight = src.u32Width * hight / width;
				  dest.s32X      = src.s32X;
				  dest.s32Y      = (src.u32Height - sthight) / 2 + src.s32Y;
				  dest.u32Width  = src.u32Width;
				  dest.u32Height = sthight;						
			    }
			}
			break;
			
		default:			
			dest = src;
			break;	
	}
	//printf("------dest.s32X: %d, dest.s32Y: %d, dest.u32Width: %d, dest.u32Height: %d\n", dest.s32X, dest.s32Y, dest.u32Width, dest.u32Height);
	return dest;
}

/*************************************************
Function: //PRV_VLossVdecBindVoChn
Description: //  ������Ƶ����ͨ����VO���ͨ��
Calls: 
Called By: //
Input://VoDev����豸
	//VoChn���ͨ����
	//u32Index���ͨ��VoChn��������
	//enPreviewModeԤ��ģʽ
Output: // ��
Return: //�ĵ�������
Others: //��PRV_PreviewVoDevInMode�ӿ��з����������ͨ���Ͽ����Ӻ�
	    //����Ҫ����PRV_PreviewVoDevInModeˢ������ͨ����ֻ����ô˽ӿ�ˢ�¶Ͽ����ӵ�ͨ��
************************************************************************/
HI_S32 PRV_VLossVdecBindVoChn(VO_DEV VoDev, VO_CHN VoChn, int u32Index, PRV_PREVIEW_MODE_E enPreviewMode)
{
	int i = 0, u32ChnNum = 0, w = 0, h = 0;
	int index = PRV_GetVoChnIndex(VoChn);
	if(index < 0)
		RET_FAILURE("------ERR: Invalid Index!");

#if defined(Hi3531)||defined(Hi3535)	
	if (VoDev < 0 || VoDev > DHD0/*VoDev >= PRV_VO_MAX_DEV*/)
	{
		RET_FAILURE("Invalid Parameter: VoDev or u32Index");
	}
	if(VoDev == DHD1)
		VoDev = DSD0;

	RECT_S stSrcRect,stDestRect;
#endif
	RECT_S *pstLayout = NULL;	
	VO_CHN_ATTR_S stVoChnAttr;
	HI_U32 Max_num = 0;
	
	w = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width;
	h = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height;
	//��ȡ��ǰԤ��ģʽ�����ͨ����
	CHECK_RET(PRV_Get_Max_chnnum(s_astVoDevStatDflt[VoDev].enPreviewMode,&Max_num));

	//���л���ʾ����
	if(s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_CTRL && VoChn == s_astVoDevStatDflt[VoDev].s32CtrlChn)
	{
		if(s_astVoDevStatDflt[VoDev].enCtrlFlag == PRV_CTRL_REGION_SEL && IsDispInPic == 1//"��ʾ����"���л�ʱ�����ӵ�IPC�Ͽ���������ʾ������ʾ"������ͼƬ"
			&& VoDev == s_VoDevCtrlDflt)//���豸����Ҫ��ʾ���л������豸����Ҫ
		{
#if defined(Hi3535)
			CHECK_RET(HI_MPI_VO_GetChnAttr(PIP, VoChn, &stVoChnAttr));
#else
			CHECK_RET(HI_MPI_VO_GetChnAttr(VoDev, VoChn, &stVoChnAttr));
#endif
			stVoChnAttr.stRect.s32X 	 = s_astVoDevStatDflt[VoDev].Pip_rect.s32X * w/s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width;
			stVoChnAttr.stRect.s32Y 	 = s_astVoDevStatDflt[VoDev].Pip_rect.s32Y * h/s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height;
			stVoChnAttr.stRect.u32Width  = 3 + s_astVoDevStatDflt[VoDev].Pip_rect.u32Width * w/s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width;
			stVoChnAttr.stRect.u32Height = 4 + s_astVoDevStatDflt[VoDev].Pip_rect.u32Height * h/s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height;
#if defined(SN9234H1)
			stVoChnAttr.u32Priority = 4;
#else
			stVoChnAttr.u32Priority = 1;
#endif

			//stVoChnAttr.stRect.s32X 	 = stVoChnAttr.stRect.s32X + (stVoChnAttr.stRect.u32Width - (NOVIDEO_IMAGWIDTH * stVoChnAttr.stRect.u32Width / w)) / 2;
			//stVoChnAttr.stRect.s32Y 	 = stVoChnAttr.stRect.s32Y + (stVoChnAttr.stRect.u32Height - (NOVIDEO_IMAGHEIGHT * stVoChnAttr.stRect.u32Height/ h))/ 2;
			//stVoChnAttr.stRect.u32Width  = (NOVIDEO_IMAGWIDTH * stVoChnAttr.stRect.u32Width / w);
			//stVoChnAttr.stRect.u32Height = (NOVIDEO_IMAGHEIGHT * stVoChnAttr.stRect.u32Height / h);
#if defined(Hi3531)||defined(Hi3535)
			if(stVoChnAttr.stRect.u32Height<32)
			{				
				stVoChnAttr.stRect.s32Y = stVoChnAttr.stRect.s32Y - (32-stVoChnAttr.stRect.u32Height)/2;
				stVoChnAttr.stRect.u32Height = 32;
			}
#endif			
			stVoChnAttr.stRect.s32X &= (~0x01);
			stVoChnAttr.stRect.s32Y &= (~0x01);
			stVoChnAttr.stRect.u32Width &= (~0x01);
			stVoChnAttr.stRect.u32Height &= (~0x01);
#if defined(Hi3535)	
			CHECK_RET(HI_MPI_VO_SetChnAttr(PIP, PRV_CTRL_VOCHN, &stVoChnAttr));
#else
			CHECK_RET(HI_MPI_VO_SetChnAttr(VoDev, PRV_CTRL_VOCHN, &stVoChnAttr));
#endif
#if defined(SN9234H1)
			CHECK(HI_MPI_VDEC_BindOutput(/*DetVLoss_VdecChn*/VochnInfo[index].VdecChn, VoDev, PRV_CTRL_VOCHN));			
#else
			PRV_VPSS_ResetWH(PRV_CTRL_VOCHN,NoConfig_VdecChn,704,576);
			CHECK(PRV_VDEC_BindVpss(NoConfig_VdecChn, PRV_CTRL_VOCHN));	
#endif
			sem_post(&sem_SendNoVideoPic);				
			//return HI_SUCCESS;//���豸��������"����Ƶ"ͼƬ�ᱻ���ǣ�Ϊ��Ч��������Ҫ��
		}
		else if(s_astVoDevStatDflt[s_VoDevCtrlDflt].enCtrlFlag == PRV_CTRL_ZOOM_IN && VoDev == s_VoDevCtrlDflt)//���ӷŴ�ʱ�����ӵ�IPC�Ͽ�������С����������ʾ"������ͼƬ"
		{
#if defined(Hi3535)	
			CHECK_RET(HI_MPI_VO_GetChnAttr(PIP, VoChn, &stVoChnAttr));
#else
			CHECK_RET(HI_MPI_VO_GetChnAttr(VoDev, VoChn, &stVoChnAttr));
#endif
			stVoChnAttr.stRect.s32X      = w*3/4;
			stVoChnAttr.stRect.s32Y      = h*3/4;
			stVoChnAttr.stRect.u32Width  = w*1/4;
			stVoChnAttr.stRect.u32Height = h*1/4;

			//stVoChnAttr.stRect.s32X 	 = stVoChnAttr.stRect.s32X + (stVoChnAttr.stRect.u32Width - (NOVIDEO_IMAGWIDTH * stVoChnAttr.stRect.u32Width / w)) / 2;
			//stVoChnAttr.stRect.s32Y 	 = stVoChnAttr.stRect.s32Y + (stVoChnAttr.stRect.u32Height - (NOVIDEO_IMAGHEIGHT * stVoChnAttr.stRect.u32Height/ h))/ 2;
			//stVoChnAttr.stRect.u32Width  = (NOVIDEO_IMAGWIDTH * stVoChnAttr.stRect.u32Width / w);
			//stVoChnAttr.stRect.u32Height = (NOVIDEO_IMAGHEIGHT * stVoChnAttr.stRect.u32Height / h);
#if defined(Hi3531)||defined(Hi3535)
			stVoChnAttr.u32Priority = 1;
			if(stVoChnAttr.stRect.u32Height < 32)
			{
				stVoChnAttr.stRect.s32Y = stVoChnAttr.stRect.s32Y - (32 - stVoChnAttr.stRect.u32Height)/2;
				stVoChnAttr.stRect.u32Height = 32;
			}
#endif			
			stVoChnAttr.stRect.s32X &= (~0x01);
			stVoChnAttr.stRect.s32Y &= (~0x01);
			stVoChnAttr.stRect.u32Width &= (~0x01);
			stVoChnAttr.stRect.u32Height &= (~0x01);
			CHECK_RET(HI_MPI_VO_SetChnAttr(VoDev, PRV_CTRL_VOCHN, &stVoChnAttr));
#if defined(SN9234H1)
			CHECK(HI_MPI_VDEC_BindOutput(/*DetVLoss_VdecChn*/VochnInfo[index].VdecChn, VoDev, PRV_CTRL_VOCHN));
#else
			PRV_VPSS_ResetWH(PRV_CTRL_VOCHN,NoConfig_VdecChn,704,576);
			CHECK(PRV_VDEC_BindVpss(NoConfig_VdecChn, PRV_CTRL_VOCHN));
#endif
			sem_post(&sem_SendNoVideoPic);				
		}
	}

	if((s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_CTRL && VoChn == s_astVoDevStatDflt[VoDev].s32CtrlChn)
		|| (s_astVoDevStatDflt[VoDev].bIsAlarm && VoChn == s_astVoDevStatDflt[VoDev].s32AlarmChn))		
	{
		u32ChnNum = 1;
		pstLayout = s_astPreviewLayout1;
		i = 0;
	}
	else if(s_astVoDevStatDflt[VoDev].bIsSingle)
	{
		if((enPreviewMode != SingleScene) && s_astVoDevStatDflt[VoDev].s32DoubleIndex == 0)
			enPreviewMode = SingleScene;
		if(VoChn == s_astVoDevStatDflt[VoDev].as32ChnOrder[enPreviewMode][s_astVoDevStatDflt[VoDev].s32SingleIndex])
		{
			u32ChnNum = 1;
			pstLayout = s_astPreviewLayout1;
			i = 0;
		}
		if(PRV_CurDecodeMode == PassiveDecode || ScmGetListCtlState() == 1)
		{
			u32ChnNum = 1;
			pstLayout = s_astPreviewLayout1;
			i = 0;
		}
	}
	else if(LayoutToSingleChn == VoChn)
	{
		u32ChnNum = 1;
		pstLayout = s_astPreviewLayout1;
		i = 0;
	}
	else if(s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_NORM
		|| pstLayout == NULL)
	{
	    switch(enPreviewMode)
		{
			case SingleScene:
				u32ChnNum = 1;
				pstLayout = s_astPreviewLayout1;
				break;
			case TwoScene:
				u32ChnNum = 2;
				pstLayout = s_astPreviewLayout2;
				break;
			case ThreeScene:
				u32ChnNum = 3;
				pstLayout = s_astPreviewLayout3;
				break;
			case FourScene:
			case LinkFourScene:
				u32ChnNum = 4;
				pstLayout = s_astPreviewLayout4;
				break;
			case FiveScene:
				u32ChnNum = 5;
				pstLayout = s_astPreviewLayout5;
				break;
			case SixScene:
				u32ChnNum = 6;
				pstLayout = s_astPreviewLayout6;
				break;
			case SevenScene:
				u32ChnNum = 7;
				pstLayout = s_astPreviewLayout7;
				break;
			case EightScene:
				u32ChnNum = 8;
				pstLayout = s_astPreviewLayout8;
				break;
			case NineScene:
			case LinkNineScene:
				u32ChnNum = 9;
				pstLayout = s_astPreviewLayout9;
				break;
			case SixteenScene:
				u32ChnNum = 16;
				pstLayout = s_astPreviewLayout16;
				break;
			default:
				RET_FAILURE("Invalid Parameter: enPreviewMode");
		}
		for(i = 0; i < u32ChnNum && u32Index+i < Max_num; i++)
		{
			if(VoChn == s_astVoDevStatDflt[VoDev].as32ChnOrder[enPreviewMode][(u32Index+i) % Max_num])
			{
				break;
			}
		}
		if(PRV_CurDecodeMode == PassiveDecode || ScmGetListCtlState() == 1)
			i = VoChn;
	}
	CHECK(HI_MPI_VO_GetChnAttr(VoDev, VoChn, &stVoChnAttr));
#if defined(Hi3531)||defined(Hi3535)
	stVoChnAttr.u32Priority = 0;
#endif
	if((enPreviewMode == NineScene || enPreviewMode == LinkNineScene || enPreviewMode == ThreeScene || enPreviewMode == FiveScene
		|| enPreviewMode == SevenScene)&& s_astVoDevStatDflt[VoDev].bIsAlarm!=1 && s_astVoDevStatDflt[VoDev].bIsSingle!=1)
	{
	//���9����Ԥ��ʱ�������л���֮����ڷ�϶������
		while(w%6 != 0)
			w++;
		while(h%6 != 0)
			h++;
	}
	if(pstLayout != NULL && i < u32ChnNum)
	{
		stVoChnAttr.stRect.s32X 	 = (w * pstLayout[i].s32X) / PRV_PREVIEW_LAYOUT_DIV;
		stVoChnAttr.stRect.s32Y 	 = (h * pstLayout[i].s32Y) / PRV_PREVIEW_LAYOUT_DIV;
		stVoChnAttr.stRect.u32Width  = (w * pstLayout[i].u32Width) / PRV_PREVIEW_LAYOUT_DIV;
		stVoChnAttr.stRect.u32Height = (h * pstLayout[i].u32Height) / PRV_PREVIEW_LAYOUT_DIV;
		if(enPreviewMode == NineScene || enPreviewMode == LinkNineScene)
		{ 
			if((i + 1) % 3 == 0)//���һ��
				stVoChnAttr.stRect.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width - stVoChnAttr.stRect.s32X;
			if(i > 5 && i < 9)//���һ��
				stVoChnAttr.stRect.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height - stVoChnAttr.stRect.s32Y;
		}
		if( enPreviewMode == ThreeScene )
		{ 
			if( i == 2)//���һ��
				stVoChnAttr.stRect.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width - stVoChnAttr.stRect.s32X;
			//���һ��
				stVoChnAttr.stRect.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height - stVoChnAttr.stRect.s32Y;
		}
		if( enPreviewMode == FiveScene )
		{ 
			if( i > 1 )//���һ��
				stVoChnAttr.stRect.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width - stVoChnAttr.stRect.s32X;
			if(i==0 || i==1 || i==4)//���һ��
				stVoChnAttr.stRect.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height - stVoChnAttr.stRect.s32Y;
		}
		if(enPreviewMode == SevenScene)
		{ 
			if(i==2 || i==4 || i==6)//���һ��
				stVoChnAttr.stRect.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width - stVoChnAttr.stRect.s32X;
			if(i==0 || i==5 || i==6)//���һ��
				stVoChnAttr.stRect.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height - stVoChnAttr.stRect.s32Y;
		}
		//stVoChnAttr.stRect.s32X 	 = stVoChnAttr.stRect.s32X + (stVoChnAttr.stRect.u32Width - (NOVIDEO_IMAGWIDTH * pstLayout[i].u32Width / PRV_PREVIEW_LAYOUT_DIV)) / 2;
		//stVoChnAttr.stRect.s32Y 	 = stVoChnAttr.stRect.s32Y + (stVoChnAttr.stRect.u32Height - (NOVIDEO_IMAGHEIGHT * pstLayout[i].u32Height / PRV_PREVIEW_LAYOUT_DIV))/ 2;
		//stVoChnAttr.stRect.u32Width  = (NOVIDEO_IMAGWIDTH * pstLayout[i].u32Width) / PRV_PREVIEW_LAYOUT_DIV;
		//stVoChnAttr.stRect.u32Height = (NOVIDEO_IMAGHEIGHT * pstLayout[i].u32Height) / PRV_PREVIEW_LAYOUT_DIV;
#if defined(SN9234H1)
		stVoChnAttr.stRect.s32X 	 &= (~0x01);
		stVoChnAttr.stRect.s32Y 	 &= (~0x01);
		stVoChnAttr.stRect.u32Width  &= (~0x01);
		stVoChnAttr.stRect.u32Height &= (~0x01);

#else
		if(stVoChnAttr.stRect.u32Height < 32)
		{
			stVoChnAttr.stRect.s32Y = stVoChnAttr.stRect.s32Y - (32 - stVoChnAttr.stRect.u32Height)/2;
			stVoChnAttr.stRect.u32Height = 32;
		}

		if(VochnInfo[index].VdecChn != DetVLoss_VdecChn)//�������续��ʱ������ʾ����ת��
		{
			stSrcRect.s32X		 = stVoChnAttr.stRect.s32X;
			stSrcRect.s32Y		 = stVoChnAttr.stRect.s32Y;
			stSrcRect.u32Width	 = stVoChnAttr.stRect.u32Width;
			stSrcRect.u32Height  = stVoChnAttr.stRect.u32Height;

			stDestRect = PRV_ReSetVoRect(OutPutMode, VochnInfo[index].VideoInfo.width, VochnInfo[index].VideoInfo.height, stSrcRect);
		
			stVoChnAttr.stRect.s32X 	 = stDestRect.s32X		& (~0x01);
			stVoChnAttr.stRect.s32Y 	 = stDestRect.s32Y		& (~0x01);
			stVoChnAttr.stRect.u32Width  = stDestRect.u32Width	& (~0x01);
			stVoChnAttr.stRect.u32Height = stDestRect.u32Height & (~0x01);
		}
		else
		{
			stVoChnAttr.stRect.s32X &= (~0x01);
			stVoChnAttr.stRect.s32Y &= (~0x01);
			stVoChnAttr.stRect.u32Width &= (~0x01);
			stVoChnAttr.stRect.u32Height &= (~0x01);
		}
#endif
	}
	CHECK_RET(HI_MPI_VO_SetChnAttr(VoDev, VoChn, &stVoChnAttr));
	//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "VochnInfo[index].VdecChn: %d, i: %d,X: %d, Y: %d, W: %d, H: %d\n", VochnInfo[index].VdecChn, i, stVoChnAttr.stRect.s32X, stVoChnAttr.stRect.s32Y, stVoChnAttr.stRect.u32Width, stVoChnAttr.stRect.u32Height);

	//���豸"��ʾ����"ʱ�������治��Ҫ��"������"ͼƬ����ͨ��
	if(s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_CTRL && IsDispInPic == 1 && VoDev == s_VoDevCtrlDflt)
		return HI_SUCCESS;
#if defined(SN9234H1)
	CHECK(HI_MPI_VO_SetChnFrameRate(VoDev, VoChn, 30));
	(HI_MPI_VDEC_BindOutput(/*DetVLoss_VdecChn*/VochnInfo[index].VdecChn, VoDev, VoChn));
#else
	CHECK(HI_MPI_VO_SetChnFrameRate(VoDev, VoChn, 25));
	if(VochnInfo[index].IsBindVdec[DHD0] != 0/* && VochnInfo[index].IsBindVdec[DSD0] != 0*/)
	{
		PRV_VPSS_ResetWH(VoChn,VochnInfo[index].VdecChn,704,576);
		CHECK(PRV_VDEC_BindVpss(VochnInfo[index].VdecChn, VoChn));
	}
#endif	
	VochnInfo[index].IsBindVdec[VoDev] = 0;
	if(VochnInfo[index].VdecChn == DetVLoss_VdecChn)
		sem_post(&sem_SendNoVideoPic);				
	return HI_SUCCESS;
			
}

//��Ƭ����ʾ��ͨ�������ͨ����
HI_S32 PRV_BindVoChnInMaster(VO_DEV VoDev, VO_CHN VoChn, int u32Index, PRV_PREVIEW_MODE_E enPreviewMode)
{
	if(PRV_STAT_PB == s_astVoDevStatDflt[VoDev].enPreviewStat || PRV_STAT_PIC == s_astVoDevStatDflt[VoDev].enPreviewStat)
		RET_FAILURE("In PB or Pic Stat!");
	int i = 0, u32ChnNum = 0, w = 0, h = 0;
	int index = PRV_GetVoChnIndex(VoChn);
	if(index < 0)
		RET_FAILURE("------ERR: Invalid Index!");
#if defined(Hi3531)||defined(Hi3535)	
	if (VoDev < 0 || VoDev > DHD0/*VoDev >= PRV_VO_MAX_DEV*/)
	{
		RET_FAILURE("Invalid Parameter: VoDev or u32Index");
	}
	if(VoDev == DHD1)
		VoDev = DSD0;
#endif	
	VO_CHN_ATTR_S stVoChnAttr;
	RECT_S *pstLayout = NULL;
	HI_U32 Max_num = 0;
	RECT_S stSrcRect, stDestRect;
	
	w = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width;
	h = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height;
	
	//��ȡ��ǰԤ��ģʽ�����ͨ����
	CHECK_RET(PRV_Get_Max_chnnum(s_astVoDevStatDflt[VoDev].enPreviewMode, &Max_num));

	//���л���ʾ����
	if(s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_CTRL && VoChn == s_astVoDevStatDflt[VoDev].s32CtrlChn)
	{
		if(s_astVoDevStatDflt[VoDev].enCtrlFlag == PRV_CTRL_REGION_SEL && IsDispInPic == 1//"��ʾ����"���л�ʱ��������IPC
			&& VoDev == s_VoDevCtrlDflt)//���豸����Ҫ��ʾ���л������豸����Ҫ
		{
			CHECK_RET(HI_MPI_VO_GetChnAttr(VoDev, VoChn, &stVoChnAttr));
			stVoChnAttr.stRect.s32X 	 = s_astVoDevStatDflt[VoDev].Pip_rect.s32X * w/s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width;
			stVoChnAttr.stRect.s32Y 	 = s_astVoDevStatDflt[VoDev].Pip_rect.s32Y * h/s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height;
			stVoChnAttr.stRect.u32Width  = 3 + s_astVoDevStatDflt[VoDev].Pip_rect.u32Width * w/s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width;
			stVoChnAttr.stRect.u32Height = 4 + s_astVoDevStatDflt[VoDev].Pip_rect.u32Height * h/s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height;
#if defined(SN9234H1)
			//stVoChnAttr.u32Priority = 4;
			#if 0
			stSrcRect.s32X		 = stVoChnAttr.stRect.s32X;
			stSrcRect.s32Y		 = stVoChnAttr.stRect.s32Y;
			stSrcRect.u32Width	 = stVoChnAttr.stRect.u32Width;
			stSrcRect.u32Height  = stVoChnAttr.stRect.u32Height;
			
			stDestRect = PRV_ReSetVoRect(OutPutMode, VochnInfo[index].VideoInfo.width, VochnInfo[index].VideoInfo.height, stSrcRect);
			stVoChnAttr.stRect.s32X 	 = stDestRect.s32X 		& (~0x01);
			stVoChnAttr.stRect.s32Y 	 = stDestRect.s32Y 		& (~0x01);
			stVoChnAttr.stRect.u32Width  = stDestRect.u32Width  & (~0x01);
			stVoChnAttr.stRect.u32Height = stDestRect.u32Height & (~0x01);
			//printf("line: %d------stSrcRect.s32X: %d, stSrcRect.s32Y: %d, stSrcRect.u32Width: %d, stSrcRect.u32Height: %d\n", __LINE__, stSrcRect.s32X, stSrcRect.s32Y, stSrcRect.u32Width, stSrcRect.u32Height); 
			//printf("line: %d------stDestRect.s32X: %d, stDestRect.s32Y: %d, stDestRect.u32Width: %d, stDestRect.u32Height: %d\n", __LINE__, stDestRect.s32X, stDestRect.s32Y, stDestRect.u32Width, stDestRect.u32Height);
			#endif
			CHECK_RET(HI_MPI_VO_SetChnAttr(VoDev, PRV_CTRL_VOCHN, &stVoChnAttr));
			
			CHECK(HI_MPI_VO_SetChnFrameRate(VoDev, PRV_CTRL_VOCHN, 30));
			CHECK(HI_MPI_VDEC_BindOutput(VochnInfo[index].VdecChn, VoDev, PRV_CTRL_VOCHN));
#else
			stVoChnAttr.u32Priority = 1;
	
			stVoChnAttr.stRect.s32X      &= (~0x1);
			stVoChnAttr.stRect.s32Y      &= (~0x1);
			stVoChnAttr.stRect.u32Width  &= (~0x1);
			stVoChnAttr.stRect.u32Height &= (~0x1);
#if defined(Hi3535)
			CHECK_RET(HI_MPI_VO_SetChnAttr(PIP, PRV_CTRL_VOCHN, &stVoChnAttr));
			CHECK(HI_MPI_VO_SetChnFrameRate(PIP, PRV_CTRL_VOCHN, 25));
#else
			CHECK_RET(HI_MPI_VO_SetChnAttr(VoDev, PRV_CTRL_VOCHN, &stVoChnAttr));
			CHECK(HI_MPI_VO_SetChnFrameRate(VoDev, PRV_CTRL_VOCHN, 25));
#endif
			PRV_VPSS_ResetWH(PRV_CTRL_VOCHN,VochnInfo[index].VdecChn,VochnInfo[index].VideoInfo.width,VochnInfo[index].VideoInfo.height);
			CHECK(PRV_VDEC_BindVpss(VochnInfo[index].VdecChn, PRV_CTRL_VOCHN));
#endif			
		}
		else if(s_astVoDevStatDflt[s_VoDevCtrlDflt].enCtrlFlag == PRV_CTRL_ZOOM_IN && VoDev == s_VoDevCtrlDflt)//���ӷŴ�ʱ�����ӵ�IPC�Ͽ�������С����������ʾ"������ͼƬ"
		{
			CHECK_RET(HI_MPI_VO_GetChnAttr(VoDev, VoChn, &stVoChnAttr));
			stVoChnAttr.stRect.s32X      = w*3/4;
			stVoChnAttr.stRect.s32Y      = h*3/4;
			stVoChnAttr.stRect.u32Width  = w*1/4;
			stVoChnAttr.stRect.u32Height = h*1/4;

			stSrcRect.s32X		 = stVoChnAttr.stRect.s32X;
			stSrcRect.s32Y		 = stVoChnAttr.stRect.s32Y;
			stSrcRect.u32Width	 = stVoChnAttr.stRect.u32Width;
			stSrcRect.u32Height  = stVoChnAttr.stRect.u32Height;

			stDestRect = PRV_ReSetVoRect(OutPutMode, VochnInfo[index].VideoInfo.width, VochnInfo[index].VideoInfo.height, stSrcRect);

			stVoChnAttr.stRect.s32X 	 = stDestRect.s32X		& (~0x01);
			stVoChnAttr.stRect.s32Y 	 = stDestRect.s32Y		& (~0x01);
			stVoChnAttr.stRect.u32Width  = stDestRect.u32Width	& (~0x01);
			stVoChnAttr.stRect.u32Height = stDestRect.u32Height & (~0x01);
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "x: %d, y: %d, w: %d, h: %d\n", stVoChnAttr.stRect.s32X, stVoChnAttr.stRect.s32Y, stVoChnAttr.stRect.u32Width, stVoChnAttr.stRect.u32Height);
#if defined(Hi3535)
			CHECK_RET(HI_MPI_VO_SetChnAttr(PIP, PRV_CTRL_VOCHN, &stVoChnAttr));
			CHECK(HI_MPI_VO_SetChnFrameRate(PIP, PRV_CTRL_VOCHN, 30));
#else
			CHECK_RET(HI_MPI_VO_SetChnAttr(VoDev, PRV_CTRL_VOCHN, &stVoChnAttr));
			CHECK(HI_MPI_VO_SetChnFrameRate(VoDev, PRV_CTRL_VOCHN, 30));
#endif
#if defined(SN9234H1)
			CHECK(HI_MPI_VDEC_BindOutput(VochnInfo[index].VdecChn, VoDev, PRV_CTRL_VOCHN));
#else
			PRV_VPSS_ResetWH(PRV_CTRL_VOCHN,VochnInfo[index].VdecChn,VochnInfo[index].VideoInfo.width,VochnInfo[index].VideoInfo.height);
			CHECK(PRV_VDEC_BindVpss(VochnInfo[index].VdecChn, PRV_CTRL_VOCHN));
#endif
		}
	}
	
	if((s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_CTRL && VoChn == s_astVoDevStatDflt[VoDev].s32CtrlChn)
		|| (s_astVoDevStatDflt[VoDev].bIsAlarm && VoChn == s_astVoDevStatDflt[VoDev].s32AlarmChn))	
	{
		u32ChnNum = 1;
		pstLayout = s_astPreviewLayout1;
		i = 0;
	}
	else if(s_astVoDevStatDflt[VoDev].bIsSingle )
	{
		if((enPreviewMode != SingleScene) && s_astVoDevStatDflt[VoDev].s32DoubleIndex == 0)
			enPreviewMode = SingleScene;

		if(VoChn == s_astVoDevStatDflt[VoDev].as32ChnOrder[enPreviewMode][s_astVoDevStatDflt[VoDev].s32SingleIndex])
		{
			u32ChnNum = 1;
			pstLayout = s_astPreviewLayout1;
			i = 0;
		}		
		if(PRV_CurDecodeMode == PassiveDecode || ScmGetListCtlState() == 1)
		{
			u32ChnNum = 1;
			pstLayout = s_astPreviewLayout1;
			i = 0;
		}
	}
	else if(LayoutToSingleChn == VoChn)
	{
		u32ChnNum = 1;
		pstLayout = s_astPreviewLayout1;
		i = 0;
	}	
	else if(s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_NORM
		|| pstLayout == NULL)
	{
	    switch(enPreviewMode)
		{
			case SingleScene:
				u32ChnNum = 1;
				pstLayout = s_astPreviewLayout1;
				break;
			case TwoScene:
				u32ChnNum = 2;
				pstLayout = s_astPreviewLayout2;
				break;
			case ThreeScene:
				u32ChnNum = 3;
				pstLayout = s_astPreviewLayout3;
				break;
			case FourScene:
			case LinkFourScene:
				u32ChnNum = 4;
				pstLayout = s_astPreviewLayout4;
				break;
			case FiveScene:
				u32ChnNum = 5;
				pstLayout = s_astPreviewLayout5;
				break;
			case SixScene:
				u32ChnNum = 6;
				pstLayout = s_astPreviewLayout6;
				break;
			case SevenScene:
				u32ChnNum = 7;
				pstLayout = s_astPreviewLayout7;
				break;
			case EightScene:
				u32ChnNum = 8;
				pstLayout = s_astPreviewLayout8;
				break;
			case NineScene:
			case LinkNineScene:
				u32ChnNum = 9;
				pstLayout = s_astPreviewLayout9;
				break;
			case SixteenScene:
				u32ChnNum = 16;
				pstLayout = s_astPreviewLayout16;
				break;
			default:
				RET_FAILURE("Invalid Parameter: enPreviewMode");
		}

		for(i = 0; i < u32ChnNum && u32Index+i < Max_num; i++)
		{
			if(VoChn == s_astVoDevStatDflt[VoDev].as32ChnOrder[enPreviewMode][(u32Index+i) % Max_num])
				break;
		}
		if(PRV_CurDecodeMode == PassiveDecode || ScmGetListCtlState() == 1)
			i = VoChn;
		
	}

    if((enPreviewMode == NineScene || enPreviewMode == LinkNineScene || enPreviewMode == ThreeScene || enPreviewMode == FiveScene
		|| enPreviewMode == SevenScene)&& s_astVoDevStatDflt[VoDev].bIsAlarm!=1 && s_astVoDevStatDflt[VoDev].bIsSingle!=1)
    {
	//���9����Ԥ��ʱ�������л���֮����ڷ�϶������
		while(w%6 != 0)
			w++;
		while(h%6 != 0)
			h++;
	}
	//printf("VoChn: %d, i: %d, u32ChnNum: %d\n", VoChn, i, u32ChnNum);
	//CHECK(HI_MPI_VO_ClearChnBuffer(VoDev, VoChn, HI_TRUE));
	CHECK_RET(HI_MPI_VO_GetChnAttr(VoDev, VoChn, &stVoChnAttr));
	if(pstLayout != NULL && i < u32ChnNum)
	{
		stVoChnAttr.stRect.s32X 	 = (w * pstLayout[i].s32X) / PRV_PREVIEW_LAYOUT_DIV;
		stVoChnAttr.stRect.s32Y 	 = (h * pstLayout[i].s32Y) / PRV_PREVIEW_LAYOUT_DIV;
		stVoChnAttr.stRect.u32Width  = (w * pstLayout[i].u32Width) / PRV_PREVIEW_LAYOUT_DIV;
		stVoChnAttr.stRect.u32Height = (h * pstLayout[i].u32Height) / PRV_PREVIEW_LAYOUT_DIV;
		if(enPreviewMode == NineScene || enPreviewMode == LinkNineScene)
		{ 
			if((i + 1) % 3 == 0)//���һ��
				stVoChnAttr.stRect.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width - stVoChnAttr.stRect.s32X;
			if(i > 5 && i < 9)//���һ��
				stVoChnAttr.stRect.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height - stVoChnAttr.stRect.s32Y;
		}
		if( enPreviewMode == ThreeScene )
		{ 
			if( i == 2)//���һ��
				stVoChnAttr.stRect.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width - stVoChnAttr.stRect.s32X;
			//���һ��
				stVoChnAttr.stRect.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height - stVoChnAttr.stRect.s32Y;
		}
		if( enPreviewMode == FiveScene )
		{ 
			if( i > 1 )//���һ��
				stVoChnAttr.stRect.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width - stVoChnAttr.stRect.s32X;
			if(i==0 || i==1 || i==4)//���һ��
				stVoChnAttr.stRect.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height - stVoChnAttr.stRect.s32Y;
		}
		if(enPreviewMode == SevenScene)
		{ 
			if(i==2 || i==4 || i==6)//���һ��
				stVoChnAttr.stRect.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width - stVoChnAttr.stRect.s32X;
			if(i==0 || i==5 || i==6)//���һ��
				stVoChnAttr.stRect.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height - stVoChnAttr.stRect.s32Y;
		}
		stSrcRect.s32X		 = stVoChnAttr.stRect.s32X;
		stSrcRect.s32Y		 = stVoChnAttr.stRect.s32Y;
		stSrcRect.u32Width	 = stVoChnAttr.stRect.u32Width;
		stSrcRect.u32Height  = stVoChnAttr.stRect.u32Height;
		
		stDestRect = PRV_ReSetVoRect(OutPutMode, VochnInfo[index].VideoInfo.width, VochnInfo[index].VideoInfo.height, stSrcRect);
		stVoChnAttr.stRect.s32X 	 = stDestRect.s32X 		& (~0x01);
		stVoChnAttr.stRect.s32Y 	 = stDestRect.s32Y 		& (~0x01);
		stVoChnAttr.stRect.u32Width  = stDestRect.u32Width  & (~0x01);
		stVoChnAttr.stRect.u32Height = stDestRect.u32Height & (~0x01);
		
		//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "-------stVoChnAttr.stRect.s32X:%d, stVoChnAttr.stRect.s32Y:%d, stVoChnAttr.stRect.u32Width: %d, stVoChnAttr.stRect.u32Height: %d\n",
		//									stVoChnAttr.stRect.s32X, stVoChnAttr.stRect.s32Y, stVoChnAttr.stRect.u32Width, stVoChnAttr.stRect.u32Height);
	}
	CHECK_RET(HI_MPI_VO_SetChnAttr(VoDev, VoChn, &stVoChnAttr));
#if defined(SN9234H1)
	CHECK(HI_MPI_VO_SetChnFrameRate(VoDev, VoChn, 30));
	CHECK(HI_MPI_VDEC_BindOutput(VochnInfo[index].VdecChn, VoDev, VoChn));
#else
	CHECK(HI_MPI_VO_SetChnFrameRate(VoDev, VoChn, 25));
	//vpssֻҪ��һ��
	if(VochnInfo[index].IsBindVdec[DHD0] != 1/* && VochnInfo[index].IsBindVdec[DSD0] != 1*/)
	{
		PRV_VPSS_ResetWH(VoChn,VochnInfo[index].VdecChn,VochnInfo[index].VideoInfo.width,VochnInfo[index].VideoInfo.height);
		CHECK(PRV_VDEC_BindVpss(VochnInfo[index].VdecChn, VoChn));
	}
#endif	
	VochnInfo[index].IsBindVdec[VoDev] = 1;
	return HI_SUCCESS;
}
/*************************************************
Function: //PRV_BindVoChnInSlave
Description: //  ��VO���ͨ����VI(0,0)��ʾ��Ƭ���������Ƶ
Calls: 
Called By: //
Input://VoDev����豸
	//VoChn���ͨ����
	//u32Index���ͨ��VoChn��������
	//enPreviewModeԤ��ģʽ
Output: // ��
Return: //�ĵ�������
Others: //��PRV_PreviewVoDevInMode�ӿ��з������������ͨ���������Ӻ󣬴�Ƭ�ɹ���������ͨ�����غ�
	    //����Ҫ����PRV_PreviewVoDevInModeˢ������ͨ����ֻ����ô˽ӿ�ˢ�������ӵ�ͨ��
************************************************************************/

HI_S32 PRV_BindVoChnInSlave(VO_DEV VoDev, VO_CHN VoChn, int u32Index, PRV_PREVIEW_MODE_E enPreviewMode)
{
	if(PRV_STAT_PB == s_astVoDevStatDflt[VoDev].enPreviewStat || PRV_STAT_PIC == s_astVoDevStatDflt[VoDev].enPreviewStat)
		RET_FAILURE("In PB or Pic Stat!");
	
	int i = 0, u32ChnNum = 0, w = 0, h = 0;
	int index = PRV_GetVoChnIndex(VoChn);
	if(index < 0)
		RET_FAILURE("------ERR: Invalid Index!");
	VO_ZOOM_ATTR_S stVoZoomAttr;
	VO_CHN_ATTR_S stVoChnAttr;
	RECT_S *pstLayout = NULL;
	RECT_S stSrcRect,stDestRect;
	HI_U32 Max_num = 0;
	
	w = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width;
	h = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height;
	
	if(s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_CTRL && VoChn == s_astVoDevStatDflt[VoDev].s32CtrlChn)
	{
		//���л���ʾ����
		if(s_astVoDevStatDflt[VoDev].enCtrlFlag == PRV_CTRL_REGION_SEL && IsDispInPic == 1//"��ʾ����"���л�ʱ��������IPC
			&& VoDev == s_VoDevCtrlDflt)//���豸����Ҫ��ʾ���л������豸����Ҫ
		{
			CHECK_RET(HI_MPI_VO_GetChnAttr(VoDev, VoChn, &stVoChnAttr));
			stVoChnAttr.stRect.s32X 	 = s_astVoDevStatDflt[VoDev].Pip_rect.s32X * w/s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width;
			stVoChnAttr.stRect.s32Y 	 = s_astVoDevStatDflt[VoDev].Pip_rect.s32Y * h/s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height;
			stVoChnAttr.stRect.u32Width  = 3 + s_astVoDevStatDflt[VoDev].Pip_rect.u32Width * w/s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width;
			stVoChnAttr.stRect.u32Height = 4 + s_astVoDevStatDflt[VoDev].Pip_rect.u32Height * h/s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height;
			//stVoChnAttr.u32Priority = 4;
	
			stVoChnAttr.stRect.s32X      &= (~0x1);
			stVoChnAttr.stRect.s32Y      &= (~0x1);
			stVoChnAttr.stRect.u32Width  &= (~0x1);
			stVoChnAttr.stRect.u32Height &= (~0x1);
			
			CHECK_RET(HI_MPI_VO_SetChnAttr(VoDev, PRV_CTRL_VOCHN, &stVoChnAttr));
		}
		//���ӷŴ�����
		else if(s_astVoDevStatDflt[s_VoDevCtrlDflt].enCtrlFlag == PRV_CTRL_ZOOM_IN && VoDev == s_VoDevCtrlDflt)//���ӷŴ�ʱ�����ӵ�IPC�Ͽ�������С����������ʾ"������ͼƬ"
		{
			CHECK_RET(HI_MPI_VO_GetChnAttr(VoDev, VoChn, &stVoChnAttr));
			stVoChnAttr.stRect.s32X      = w*3/4;
			stVoChnAttr.stRect.s32Y      = h*3/4;
			stVoChnAttr.stRect.u32Width  = w*1/4;
			stVoChnAttr.stRect.u32Height = h*1/4;

			stSrcRect.s32X		 = stVoChnAttr.stRect.s32X;
			stSrcRect.s32Y		 = stVoChnAttr.stRect.s32Y;
			stSrcRect.u32Width	 = stVoChnAttr.stRect.u32Width;
			stSrcRect.u32Height  = stVoChnAttr.stRect.u32Height;
	
			stDestRect = PRV_ReSetVoRect(OutPutMode, VochnInfo[index].VideoInfo.width, VochnInfo[index].VideoInfo.height, stSrcRect);

			stVoChnAttr.stRect.s32X 	 = stDestRect.s32X		& (~0x01);
			stVoChnAttr.stRect.s32Y 	 = stDestRect.s32Y		& (~0x01);
			stVoChnAttr.stRect.u32Width  = stDestRect.u32Width	& (~0x01);
			stVoChnAttr.stRect.u32Height = stDestRect.u32Height & (~0x01);

			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "x: %d, y: %d, w: %d, h: %d\n", stVoChnAttr.stRect.s32X, stVoChnAttr.stRect.s32Y, stVoChnAttr.stRect.u32Width, stVoChnAttr.stRect.u32Height);
			CHECK_RET(HI_MPI_VO_SetChnAttr(VoDev, PRV_CTRL_VOCHN, &stVoChnAttr));
		}
	}
	
	//��ȡ��ǰԤ��ģʽ�����ͨ����
	CHECK_RET(PRV_Get_Max_chnnum(s_astVoDevStatDflt[VoDev].enPreviewMode, &Max_num));
	if((s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_CTRL && VoChn == s_astVoDevStatDflt[VoDev].s32CtrlChn)
		|| (s_astVoDevStatDflt[VoDev].bIsAlarm && VoChn == s_astVoDevStatDflt[VoDev].s32AlarmChn))
	{
		i = 0;
		u32ChnNum = 1;
		pstLayout = s_astPreviewLayout1;

	}
	else if(s_astVoDevStatDflt[VoDev].bIsSingle )
	{
		if((enPreviewMode != SingleScene) && s_astVoDevStatDflt[VoDev].s32DoubleIndex == 0)
			enPreviewMode = SingleScene;

		if(VoChn == s_astVoDevStatDflt[VoDev].as32ChnOrder[enPreviewMode][s_astVoDevStatDflt[VoDev].s32SingleIndex])
		{
			u32ChnNum = 1;
			pstLayout = s_astPreviewLayout1;
			i = 0;
		}		
		if(PRV_CurDecodeMode == PassiveDecode || ScmGetListCtlState() == 1)
		{
			u32ChnNum = 1;
			pstLayout = s_astPreviewLayout1;
			i = 0;
		}
	}
	else if(LayoutToSingleChn == VoChn)
	{
		u32ChnNum = 1;
		pstLayout = s_astPreviewLayout1;
		i = 0;
	}
	else if(s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_NORM
		|| pstLayout == NULL)
	{	
	    switch(enPreviewMode)
		{
			case SingleScene:
				u32ChnNum = 1;
				pstLayout = s_astPreviewLayout1;
				break;
			case TwoScene:
				u32ChnNum = 2;
				pstLayout = s_astPreviewLayout2;
				break;
			case ThreeScene:
				u32ChnNum = 3;
				pstLayout = s_astPreviewLayout3;
				break;
			case FourScene:
			case LinkFourScene:
				u32ChnNum = 4;
				pstLayout = s_astPreviewLayout4;
				break;
			case FiveScene:
				u32ChnNum = 5;
				pstLayout = s_astPreviewLayout5;
				break;
			case SixScene:
				u32ChnNum = 6;
				pstLayout = s_astPreviewLayout6;
				break;
			case SevenScene:
				u32ChnNum = 7;
				pstLayout = s_astPreviewLayout7;
				break;
			case EightScene:
				u32ChnNum = 8;
				pstLayout = s_astPreviewLayout8;
				break;
			case NineScene:
			case LinkNineScene:
				u32ChnNum = 9;
				pstLayout = s_astPreviewLayout9;
				break;
			case SixteenScene:
				u32ChnNum = 16;
				pstLayout = s_astPreviewLayout16;
				break;
			default:
				RET_FAILURE("Invalid Parameter: enPreviewMode");
		}

		for(i = 0; i < u32ChnNum && u32Index+i < Max_num; i++)
		{
			if(VoChn == s_astVoDevStatDflt[VoDev].as32ChnOrder[enPreviewMode][(u32Index+i) % Max_num])
				break;
		}
		if(PRV_CurDecodeMode == PassiveDecode || ScmGetListCtlState() == 1)
			i = VoChn;
	}
	//CHECK(HI_MPI_VO_ClearChnBuffer(VoDev, VoChn, HI_TRUE));
	if((enPreviewMode == NineScene || enPreviewMode == LinkNineScene || enPreviewMode == ThreeScene || enPreviewMode == FiveScene
		|| enPreviewMode == SevenScene) && s_astVoDevStatDflt[VoDev].bIsAlarm!=1 && s_astVoDevStatDflt[VoDev].bIsSingle!=1)
	{
	//���9����Ԥ��ʱ�������л���֮����ڷ�϶������
		while(w%6 != 0)
			w++;
		while(h%6 != 0)
			h++;
	}
	CHECK_RET(HI_MPI_VO_GetChnAttr(VoDev, VoChn, &stVoChnAttr));
	if(pstLayout != NULL && i < u32ChnNum)
	{
		stVoChnAttr.stRect.s32X 	 = (w * pstLayout[i].s32X) / PRV_PREVIEW_LAYOUT_DIV;
		stVoChnAttr.stRect.s32Y 	 = (h * pstLayout[i].s32Y) / PRV_PREVIEW_LAYOUT_DIV;
		stVoChnAttr.stRect.u32Width  = (w * pstLayout[i].u32Width) / PRV_PREVIEW_LAYOUT_DIV;
		stVoChnAttr.stRect.u32Height = (h * pstLayout[i].u32Height) / PRV_PREVIEW_LAYOUT_DIV;
		if(enPreviewMode == NineScene || enPreviewMode == LinkNineScene)
		{ 
			if((i + 1) % 3 == 0)//���һ��
				stVoChnAttr.stRect.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width - stVoChnAttr.stRect.s32X;
			if(i > 5 && i < 9)//���һ��
				stVoChnAttr.stRect.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height - stVoChnAttr.stRect.s32Y;
		}

	    if( enPreviewMode == ThreeScene )
		{ 
		    if( i == 2)//���һ��
			    stVoChnAttr.stRect.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width - stVoChnAttr.stRect.s32X;
					//���һ��
				stVoChnAttr.stRect.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height - stVoChnAttr.stRect.s32Y;
		}
		if( enPreviewMode == FiveScene )
		{ 
		    if( i > 1 )//���һ��
			    stVoChnAttr.stRect.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width - stVoChnAttr.stRect.s32X;
			if(i==0 || i==1 || i==4)//���һ��
			    stVoChnAttr.stRect.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height - stVoChnAttr.stRect.s32Y;
		}
		if(enPreviewMode == SevenScene)
		{ 
		    if(i==2 || i==4 || i==6)//���һ��
			    stVoChnAttr.stRect.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width - stVoChnAttr.stRect.s32X;
			if(i==0 || i==5 || i==6)//���һ��
			    stVoChnAttr.stRect.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height - stVoChnAttr.stRect.s32Y;
		}
		
		stSrcRect.s32X		 = stVoChnAttr.stRect.s32X;
		stSrcRect.s32Y		 = stVoChnAttr.stRect.s32Y;
		stSrcRect.u32Width	 = stVoChnAttr.stRect.u32Width;
		stSrcRect.u32Height  = stVoChnAttr.stRect.u32Height;
		
		stDestRect = PRV_ReSetVoRect(OutPutMode, VochnInfo[index].VideoInfo.width, VochnInfo[index].VideoInfo.height, stSrcRect);
		stVoChnAttr.stRect.s32X 	 = stDestRect.s32X 		& (~0x01);
		stVoChnAttr.stRect.s32Y 	 = stDestRect.s32Y 		& (~0x01);
		stVoChnAttr.stRect.u32Width  = stDestRect.u32Width  & (~0x01);
		stVoChnAttr.stRect.u32Height = stDestRect.u32Height & (~0x01);
		
		//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "-------stVoChnAttr.stRect.s32X:%d, stVoChnAttr.stRect.s32Y:%d, stVoChnAttr.stRect.u32Width: %d, stVoChnAttr.stRect.u32Height: %d\n",
		//									stVoChnAttr.stRect.s32X, stVoChnAttr.stRect.s32Y, stVoChnAttr.stRect.u32Width, stVoChnAttr.stRect.u32Height);
	}
	CHECK_RET(HI_MPI_VO_SetChnAttr(VoDev, VoChn, &stVoChnAttr));
	
	//if(VochnInfo[index].IsBindVdec[VoDev] == -1)
	if(pstLayout != NULL && i < u32ChnNum)
	{
		#if 0
		if(s_astVoDevStatDflt[VoDev].bIsSingle || s_astVoDevStatDflt[VoDev].enPreviewStat != PRV_STAT_NORM)
		{	
			w = PRV_SINGLE_SCREEN_W;
			h = PRV_SINGLE_SCREEN_H;
		}
		else
		
		{
			w = PRV_BT1120_SIZE_W;
			h = PRV_BT1120_SIZE_H;
		}
		#endif
		stSrcRect.s32X		= (w * pstLayout[i].s32X) / PRV_PREVIEW_LAYOUT_DIV;
		stSrcRect.s32Y		= (h * pstLayout[i].s32Y) / PRV_PREVIEW_LAYOUT_DIV;
		stSrcRect.u32Width	= (w * pstLayout[i].u32Width) / PRV_PREVIEW_LAYOUT_DIV;
		stSrcRect.u32Height = (h * pstLayout[i].u32Height) / PRV_PREVIEW_LAYOUT_DIV;
		
		stDestRect = PRV_ReSetVoRect(OutPutMode, VochnInfo[index].VideoInfo.width, VochnInfo[index].VideoInfo.height, stSrcRect);
		
		stVoZoomAttr.stZoomRect.s32X		= stDestRect.s32X	   & (~0x01);
		stVoZoomAttr.stZoomRect.s32Y		= stDestRect.s32Y	   & (~0x01);
		stVoZoomAttr.stZoomRect.u32Width	= stDestRect.u32Width  & (~0x01);
		stVoZoomAttr.stZoomRect.u32Height	= stDestRect.u32Height & (~0x01);
		
		//stVoZoomAttr.enField = VIDEO_FIELD_INTERLACED;
#if defined(SN9234H1)		
		stVoZoomAttr.enField = VIDEO_FIELD_FRAME;
#endif	
		//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "-------x:%d, y:%d, width: %d, height: %d\n", stVoZoomAttr.stZoomRect.s32X, stVoZoomAttr.stZoomRect.s32Y,stVoZoomAttr.stZoomRect.u32Width, stVoZoomAttr.stZoomRect.u32Height);
		CHECK_RET(HI_MPI_VO_SetZoomInWindow(VoDev, VoChn, &stVoZoomAttr));
	}
	
	CHECK(HI_MPI_VO_SetChnFrameRate(VoDev, VoChn, 30));
#if defined(SN9234H1)	
	CHECK(HI_MPI_VI_BindOutput(PRV_HD_DEV, 0, VoDev, VoChn));
#endif
	//if(s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_NORM)
	{
		VochnInfo[index].IsBindVdec[VoDev] = 1;
	}
	if(s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_CTRL && VoDev == s_VoDevCtrlDflt && VoChn == s_astVoDevStatDflt[VoDev].s32CtrlChn
		&& (IsDispInPic == 1 || s_astVoDevStatDflt[s_VoDevCtrlDflt].enCtrlFlag == PRV_CTRL_ZOOM_IN ))//���ӷŴ���л�
	{
		CHECK(HI_MPI_VO_SetChnFrameRate(VoDev, PRV_CTRL_VOCHN, 30));
#if defined(SN9234H1)		
		CHECK(HI_MPI_VI_BindOutput(PRV_HD_DEV, 0, VoDev, PRV_CTRL_VOCHN));
#endif
	}
		
			
	return HI_SUCCESS;
}

/*************************************************
Function: //PRV_PreviewVoDevSingle
Description: ����ָ��VO״̬��ʾVO�豸�ϵ���Ƶ��ĵ����档
Calls: 
Called By: //
Input: //VoDev:�豸��
		enPreviewMode:��Ҫ��ʾԤ��ģʽ
		u32Index:�໭��ͨ��������
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/

STATIC HI_S32 PRV_PreviewVoDevSingle(VO_DEV VoDev, HI_U32 u32chn)
{	
	//sem_post(&sem_SendNoVideoPic);
    HI_U32 u32Width = 0, u32Height = 0, u32ChnNum = 0;
	HI_S32 i = 0, index = 0;
	VO_CHN VoChn = 0;
	VO_CHN_ATTR_S stVoChnAttr;
#if defined(SN9234H1)	
	VO_ZOOM_ATTR_S stVoZoomAttr;
#else
	int s32Ret = 0;
#endif
	RECT_S *pstLayout = NULL;
	unsigned char order_buf[PRV_VO_CHN_NUM];
	VoChn = u32chn;
	RECT_S stSrcRect, stDestRect;
	
	/*ȷ�������ĺϷ���*/
#if defined(SN9234H1)
	if(VoDev == SPOT_VO_DEV || VoDev == AD)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}

	if ( (VoDev < 0 || VoDev >= PRV_VO_MAX_DEV))
	{
		RET_FAILURE("Invalid Parameter: VoDev or u32Index");
	}


#else	

	if (VoDev < 0 || VoDev > DHD0/*VoDev >= PRV_VO_MAX_DEV*/)
	{
		RET_FAILURE("Invalid Parameter: VoDev or u32Index");
	}
	if(VoDev == DHD1)
		VoDev = DSD0;
#endif	
	if (VoChn < 0 || VoChn >= g_Max_Vo_Num)//PRV_VO_CHN_NUM)
	{
		RET_FAILURE("Invalid Parameter: VoChn ");
	}
	
	index = PRV_GetVoChnIndex(VoChn);
	if(index < 0)
		RET_FAILURE("Valid index!!");
	TRACE(SCI_TRACE_NORMAL, MOD_PRV, "#############PRV_PreviewVoDevSingle s_s32NPFlagDflt = %d######################\n",s_s32NPFlagDflt);

	//printf("#############PRV_PreviewVoDevSingle s_s32NPFlagDflt = %d ,stImageSize h  =%d######################\n",s_s32NPFlagDflt,u32Height);

	u32ChnNum = 1;
	pstLayout = s_astPreviewLayout1;
#if defined(SN9234H1)
	//PRV_HideAllVoChn(VoDev);
	PRV_ViUnBindAllVoChn(VoDev);
	PRV_VdecUnBindAllVoChn1(VoDev);
	//PRV_ClearAllVoChnBuf(VoDev);
	PRV_ResetVideoLayer(VoDev);
#else
	PRV_HideAllVoChn(VoDev);
	PRV_VdecUnBindAllVpss(VoDev);
	//PRV_ClearAllVoChnBuf(VoDev);
	for(i = 0; i < PRV_VO_CHN_NUM; i++)
	{
#if defined(Hi3535)
		PRV_VO_UnBindVpss(VoDev, i, i, VPSS_BSTR_CHN);
#else
		PRV_VO_UnBindVpss(VoDev, i, i, VPSS_PRE0_CHN);
#endif	
	}
	PRV_ResetVideoLayer(VoDev);

	for(i = 0; i < PRV_VO_CHN_NUM;i++)
	{
		if(VoDev == DHD0)
		{
#if defined(Hi3535)
			s32Ret = PRV_VO_BindVpss(VoDev, i, i,VPSS_BSTR_CHN);
		    if (HI_SUCCESS != s32Ret)
		    {
		        TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_VO_BindVpss VPSS_PRE0_CHN failed!\n");
		        return -1;
		    }
#else
			s32Ret = PRV_VO_BindVpss(VoDev, i, i,VPSS_PRE0_CHN);
		    if (HI_SUCCESS != s32Ret)
		    {
		        TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_VO_BindVpss VPSS_PRE0_CHN failed!\n");
		        return -1;
		    }
#endif
		}
		#if 0
		else
		{
			s32Ret = PRV_VO_BindVpss(VoDev, i, i,VPSS_BYPASS_CHN);
		    if (HI_SUCCESS != s32Ret)
		    {
		        TRACE(SCI_TRACE_NORMAL, MOD_PRV,"PRV_VO_BindVpss VPSS_BYPASS_CHN failed!\n");
		        return -1;
		    }
		}
		#endif
	}
#endif	
	u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width;
	u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height;
	PRV_HideAllVoChn(VoDev);
	i = 0;
    {
		if(PRV_CurDecodeMode == PassiveDecode || ScmGetListCtlState() == 1)
			VoChn = u32chn;
		
		order_buf[i] = VoChn;
		//���������£�û��"������"ѡ��
		if(PRV_CurDecodeMode == PassiveDecode && VochnInfo[index].VdecChn == DetVLoss_VdecChn)
		{
			VochnInfo[index].VdecChn = NoConfig_VdecChn;	
		}
		//��������ʱ����ʹ��Ӧͨ��δ���ã�Ҳ�����ӣ�����Ҫ�޶�Ϊ"δ����"
		if(PRV_CurDecodeMode == SwitchDecode && s_astVoDevStatDflt[VoDev].bIsAlarm != HI_TRUE)
		{
			if(ScmGetListCtlState() == 0 && SCM_ChnConfigState(VoChn) == 0 && PRV_GetDoubleIndex() == 0)//�ǵ�λ�����£�ͨ��δ����ʱ����
			{
				VochnInfo[index].VdecChn = NoConfig_VdecChn;	
			}
		}
		/* �ж�VoChn�Ƿ���Ч */
		

		CHECK_RET(HI_MPI_VO_GetChnAttr(VoDev, VoChn, &stVoChnAttr));
    	stVoChnAttr.stRect.s32X		 = (u32Width * pstLayout[i].s32X) / PRV_PREVIEW_LAYOUT_DIV;
    	stVoChnAttr.stRect.s32Y		 = (u32Height * pstLayout[i].s32Y) / PRV_PREVIEW_LAYOUT_DIV;
		stVoChnAttr.stRect.u32Width	 = (u32Width * pstLayout[i].u32Width) / PRV_PREVIEW_LAYOUT_DIV;
		stVoChnAttr.stRect.u32Height = (u32Height * pstLayout[i].u32Height) / PRV_PREVIEW_LAYOUT_DIV;

		//������������ô˽ӿ�:����������������������Լ����ӷŴ�
		//�����������ʱ������Ĭ���豸�ĵ������"������"�ᱻ���ǣ������������Լ��󶨽���ͨ����ʾЧ�����ã�
		//								�ڶ��豸ֻ��ʾ�����棬��Ҫ����
		//"������"ʱ���ӷŴ����
		//����ֻ�б�������ʱ��Ҫ������ʾ�����Լ��󶨽���ͨ��
#if defined(SN9234H1)
		if((VochnInfo[index].VdecChn == DetVLoss_VdecChn || VochnInfo[index].VdecChn == NoConfig_VdecChn) 
			&& (s_astVoDevStatDflt[VoDev].bIsAlarm == HI_TRUE || VoDev == (s_VoDevCtrlDflt == HD ? s_VoSecondDev : HD)))
#else
		if((VochnInfo[index].VdecChn == DetVLoss_VdecChn || VochnInfo[index].VdecChn == NoConfig_VdecChn) 
			&& (s_astVoDevStatDflt[VoDev].bIsAlarm == HI_TRUE || VoDev == (s_VoDevCtrlDflt == DHD0 ? s_VoSecondDev : DHD0)))
#endif			
		{
			//stVoChnAttr.stRect.s32X 	 = stVoChnAttr.stRect.s32X + (stVoChnAttr.stRect.u32Width - (NOVIDEO_IMAGWIDTH * pstLayout[i].u32Width / PRV_PREVIEW_LAYOUT_DIV)) / 2;
			//stVoChnAttr.stRect.s32Y 	 = stVoChnAttr.stRect.s32Y + (stVoChnAttr.stRect.u32Height - (NOVIDEO_IMAGHEIGHT * pstLayout[i].u32Height / PRV_PREVIEW_LAYOUT_DIV))/ 2;
			//stVoChnAttr.stRect.u32Width  = (NOVIDEO_IMAGWIDTH * pstLayout[i].u32Width) / PRV_PREVIEW_LAYOUT_DIV;
			//stVoChnAttr.stRect.u32Height = (NOVIDEO_IMAGHEIGHT * pstLayout[i].u32Height) / PRV_PREVIEW_LAYOUT_DIV;
			
		}

		if(VochnInfo[index].VdecChn >= 0 && VochnInfo[index].VdecChn != DetVLoss_VdecChn && VochnInfo[index].VdecChn != NoConfig_VdecChn)
		{
			stSrcRect.s32X		 = stVoChnAttr.stRect.s32X;
			stSrcRect.s32Y		 = stVoChnAttr.stRect.s32Y;
			stSrcRect.u32Width	 = stVoChnAttr.stRect.u32Width;
			stSrcRect.u32Height  = stVoChnAttr.stRect.u32Height;
			
			stDestRect = PRV_ReSetVoRect(OutPutMode, VochnInfo[index].VideoInfo.width, VochnInfo[index].VideoInfo.height, stSrcRect);
			stVoChnAttr.stRect.s32X 	 = stDestRect.s32X;
			stVoChnAttr.stRect.s32Y 	 = stDestRect.s32Y;
			stVoChnAttr.stRect.u32Width  = stDestRect.u32Width;
			stVoChnAttr.stRect.u32Height = stDestRect.u32Height;

		}
		stVoChnAttr.stRect.s32X 	 &= (~0x01);
		stVoChnAttr.stRect.s32Y 	 &= (~0x01);
		stVoChnAttr.stRect.u32Width  &= (~0x01);
		stVoChnAttr.stRect.u32Height &= (~0x01);
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "i: %d,X: %d, Y: %d, W: %d, H: %d\n", i, stVoChnAttr.stRect.s32X, stVoChnAttr.stRect.s32Y, stVoChnAttr.stRect.u32Width, stVoChnAttr.stRect.u32Height);
		CHECK_RET(HI_MPI_VO_SetChnAttr(VoDev, VoChn, &stVoChnAttr));
#if defined(SN9234H1)
		if(VoChn < (LOCALVEDIONUM < PRV_CHAN_NUM ? LOCALVEDIONUM : PRV_CHAN_NUM))
		{
			int videv = 0;
			if(VoChn >= PRV_VI_CHN_NUM)
			{
			//���ͨ��Ϊ5��8����ô��Ӧ�ɼ��豸2
				videv = PRV_656_DEV;
			}
			else
			{
				videv = PRV_656_DEV_1;
			}

			CHECK_RET(HI_MPI_VI_BindOutput(videv, VoChn%PRV_VI_CHN_NUM, VoDev, VoChn));

			//printf("###########VoChn = %d ,VoDev = %d, videv = %d,vicha = %d ######################\n",VoChn,VoDev,videv,VoChn%PRV_VI_CHN_NUM);
		}
		else 
#endif
		{	
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "VochnInfo[index].VdecChn: %d, VochnInfo[index].VoChn: %d, VochnInfo[index].SlaveId: %d\n", VochnInfo[index].VdecChn, VochnInfo[index].VoChn, VochnInfo[index].SlaveId);
#if defined(SN9234H1)
			if((VochnInfo[index].VdecChn == DetVLoss_VdecChn || VochnInfo[index].VdecChn == NoConfig_VdecChn)
				&& (s_astVoDevStatDflt[VoDev].bIsAlarm == HI_TRUE|| VoDev == (s_VoDevCtrlDflt == HD ? s_VoSecondDev : HD))
				&& VochnInfo[index].IsBindVdec[VoDev] == -1)
#else			
			if((VochnInfo[index].VdecChn == DetVLoss_VdecChn || VochnInfo[index].VdecChn == NoConfig_VdecChn)
				&& (s_astVoDevStatDflt[VoDev].bIsAlarm == HI_TRUE|| VoDev == (s_VoDevCtrlDflt == DHD0 ? s_VoSecondDev : DHD0))
				&& VochnInfo[index].IsBindVdec[VoDev] == -1)
#endif				
			{
#if defined(SN9234H1)
				CHECK(HI_MPI_VDEC_BindOutput(VochnInfo[index].VdecChn, VoDev, VochnInfo[index].VoChn));
#else
				PRV_VPSS_ResetWH(VochnInfo[index].VoChn,VochnInfo[index].VdecChn,704,576);
				CHECK(PRV_VDEC_BindVpss(VochnInfo[index].VdecChn, VochnInfo[index].VoChn));
#endif
				VochnInfo[index].IsBindVdec[VoDev] =  0;

			}
			//֮ǰ�Ѿ��󶨺ý���ͨ�����ڴ���Ҫ���
			if((VochnInfo[index].VdecChn == DetVLoss_VdecChn || VochnInfo[index].VdecChn == NoConfig_VdecChn)
				&& s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_CTRL
				&& VoDev == s_VoDevCtrlDflt
				&& VochnInfo[index].IsBindVdec[VoDev] == 0)
			{
#if defined(SN9234H1)
				(HI_MPI_VDEC_UnbindOutputChn(VochnInfo[index].VdecChn, VoDev, VochnInfo[index].VoChn));
#else
				CHECK(PRV_VDEC_UnBindVpss(VochnInfo[index].VdecChn, VochnInfo[index].VoChn));
#endif
				VochnInfo[index].IsBindVdec[VoDev] = -1;
			}
			//CHECK(HI_MPI_VO_ClearChnBuffer(VoDev, VochnInfo[index].VoChn, HI_TRUE));
			if((VochnInfo[index].VdecChn != DetVLoss_VdecChn && VochnInfo[index].VdecChn != NoConfig_VdecChn)
				&&VochnInfo[index].SlaveId == PRV_MASTER
				&& VochnInfo[index].IsBindVdec[VoDev] == -1)
			{	
#if defined(SN9234H1)
				(HI_MPI_VDEC_BindOutput(VochnInfo[index].VdecChn, VoDev, VochnInfo[index].VoChn));
#else
				PRV_VPSS_ResetWH(VochnInfo[index].VoChn,VochnInfo[index].VdecChn,704,576);
				CHECK(PRV_VDEC_BindVpss(VochnInfo[index].VdecChn, VochnInfo[index].VoChn));
#endif
				VochnInfo[index].IsBindVdec[VoDev] = 1;
			}
#if defined(SN9234H1)
			//��Ƭ�󶨵������豸0��
			//else if(VoChn >= PRV_CHAN_NUM)
			else if(VochnInfo[index].SlaveId > PRV_MASTER && VochnInfo[index].IsBindVdec[VoDev] == -1)
			{
				int w = 0, h = 0;
#if 0
#if defined(SN8604M) || defined(SN8608M) || defined(SN8608M_LE) || defined(SN8616M_LE)|| defined(SN9234H1)
				w = PRV_BT1120_SIZE_W;
				h = PRV_BT1120_SIZE_H;
#else
				w = PRV_SINGLE_SCREEN_W;
				h = PRV_SINGLE_SCREEN_H;
#endif
#endif
				w = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width;
				h = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height;
				#if 1
				stSrcRect.s32X		= (w * pstLayout[i].s32X) / PRV_PREVIEW_LAYOUT_DIV;
				stSrcRect.s32Y		= (h * pstLayout[i].s32Y) / PRV_PREVIEW_LAYOUT_DIV;
				stSrcRect.u32Width	= (w * pstLayout[i].u32Width) / PRV_PREVIEW_LAYOUT_DIV;
				stSrcRect.u32Height = (h * pstLayout[i].u32Height) / PRV_PREVIEW_LAYOUT_DIV;
				stDestRect = PRV_ReSetVoRect(OutPutMode, VochnInfo[index].VideoInfo.width, VochnInfo[index].VideoInfo.height, stSrcRect);
				stVoZoomAttr.stZoomRect.s32X		= stDestRect.s32X	   & (~0x01);
				stVoZoomAttr.stZoomRect.s32Y		= stDestRect.s32Y	   & (~0x01);
				stVoZoomAttr.stZoomRect.u32Width	= stDestRect.u32Width  & (~0x01);
				stVoZoomAttr.stZoomRect.u32Height	= stDestRect.u32Height & (~0x01);
				#else
				stVoZoomAttr.stZoomRect.s32X		= ((w * pstLayout[i].s32X) / PRV_PREVIEW_LAYOUT_DIV) & (~0x01);
				stVoZoomAttr.stZoomRect.s32Y		= ((h * pstLayout[i].s32Y) / PRV_PREVIEW_LAYOUT_DIV) & (~0x01);
				stVoZoomAttr.stZoomRect.u32Width	= ((w * pstLayout[i].u32Width) / PRV_PREVIEW_LAYOUT_DIV) & (~0x01);
				stVoZoomAttr.stZoomRect.u32Height	= ((h * pstLayout[i].u32Height) / PRV_PREVIEW_LAYOUT_DIV) & (~0x01);
				#endif
				stVoZoomAttr.enField = VIDEO_FIELD_FRAME;
		
				CHECK_RET(HI_MPI_VO_SetZoomInWindow(VoDev, VoChn, &stVoZoomAttr));

				CHECK(HI_MPI_VI_BindOutput(PRV_HD_DEV, 0, VoDev, VoChn));

				VochnInfo[index].IsBindVdec[VoDev] = 1;								
			}
#endif
			
		}
		//VochnInfo[i].bIsStopGetVideoData = 0;
#if defined(Hi3535)
		CHECK(HI_MPI_VO_SetChnFrameRate(VoDev, VoChn, 25));
		CHECK_RET(HI_MPI_VO_ShowChn(VoDev,VoChn));
		CHECK_RET(HI_MPI_VO_EnableChn(VoDev,VoChn));
#else
		CHECK(HI_MPI_VO_SetChnFrameRate(VoDev, VoChn, 30));
		CHECK_RET(HI_MPI_VO_ChnShow(VoDev,VoChn));
		CHECK_RET(HI_MPI_VO_EnableChn(VoDev,VoChn));
#endif
   }
	//CHECK_RET(HI_MPI_VO_SetAttrEnd(VoDev));
#if defined(SN9234H1)
	PRV_VdecBindAllVoChn(VoDev);
	sem_post(&sem_SendNoVideoPic);
#else	
	//PRV_VdecBindAllVpss(VoDev);
	//sem_post(&sem_SendNoVideoPic);
#endif
	OSD_Get_Preview_param(VoDev,
		s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width,
		s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height,
		u32ChnNum,SingleScene,order_buf);

#if 0 /*2010-9-17 ��̬����VIͨ��ͼ���С*/
	PRV_SetViChnAttrByPreviewMode();
#endif
	RET_SUCCESS("");
}
/*************************************************
Function: //PRV_PreviewVoDevInMode
Description: ����ָ��VO״̬��ʾVO�豸�ϵ���Ƶ��Ļ����Ų���
Calls: 
Called By: //
Input: //VoDev:�豸��
		enPreviewMode:��Ҫ��ʾԤ��ģʽ
		u32Index:�໭��ͨ��������
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_PreviewVoDevInMode(VO_DEV VoDev, PRV_PREVIEW_MODE_E enPreviewMode, HI_U32 u32Index, VO_CHN *pOrder)
{
	//sem_post(&sem_SendNoVideoPic);
    HI_U32 u32Width = 0, u32Height = 0, u32ChnNum = 0, Max_num = 0;
	//HI_U32 oldX = 0, oldY = 0, oldWidth = 0, oldHeight = 0, tmpX = 0, tmpY = 0, tmpWidth = 0, tmpHeight = 0;
	HI_S32 i = 0, index = 0;
	VO_CHN VoChn;
	VO_CHN_ATTR_S stVoChnAttr;
#if defined(SN9234H1)
	VO_ZOOM_ATTR_S stVoZoomAttr;
#else
	HI_S32 s32Ret = 0;
#endif
	RECT_S *pstLayout = NULL;
	RECT_S stSrcRect,stDestRect;
	unsigned char order_buf[PRV_VO_CHN_NUM];

#if defined(SN9234H1)
	//��ȡ��ǰԤ��ģʽ�����ͨ����
	CHECK_RET(PRV_Get_Max_chnnum(s_astVoDevStatDflt[HD].enPreviewMode, &Max_num));
	TRACE(SCI_TRACE_NORMAL,MOD_PRV,"-------------PRV_PreviewVoDevInMode s_s32NPFlagDflt = %d ,Max_num =%d ,u32Index =%d ,enPreviewMode= %d\n",s_s32NPFlagDflt,Max_num,u32Index,enPreviewMode);
	//printf("-------------PRV_PreviewVoDevInMode s_s32NPFlagDflt = %d ,Max_num =%d ,u32Index =%d ,enPreviewMode= %d\n",s_s32NPFlagDflt,Max_num,u32Index,enPreviewMode);

	/*ȷ�������ĺϷ���*/
	if(VoDev == SPOT_VO_DEV || VoDev == AD)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}

	if ( (VoDev < 0 || VoDev >= PRV_VO_MAX_DEV))
		//|| (u32Index < 0 || u32Index >= Max_num))
	{
		RET_FAILURE("Invalid Parameter: VoDev or u32Index");
	}
#else
	//��ȡ��ǰԤ��ģʽ�����ͨ����
	CHECK_RET(PRV_Get_Max_chnnum(s_astVoDevStatDflt[DHD0].enPreviewMode, &Max_num));
	TRACE(SCI_TRACE_NORMAL,MOD_PRV,"-------------PRV_PreviewVoDevInMode VoDev: %d, s_s32NPFlagDflt = %d ,Max_num =%d ,u32Index =%d ,enPreviewMode= %d\n", VoDev, s_s32NPFlagDflt,Max_num,u32Index,enPreviewMode);
	//printf("-------------PRV_PreviewVoDevInMode s_s32NPFlagDflt = %d ,Max_num =%d ,u32Index =%d ,enPreviewMode= %d\n",s_s32NPFlagDflt,Max_num,u32Index,enPreviewMode);

	/*ȷ�������ĺϷ���*/

	if (VoDev < 0 || VoDev > DHD0/*VoDev >= PRV_VO_MAX_DEV*/)
		//|| (u32Index < 0 || u32Index >= Max_num))
	{
		RET_FAILURE("Invalid Parameter: VoDev or u32Index");
	}
	if(VoDev == DHD1)
		VoDev = DSD0;
#endif
    switch(enPreviewMode)
	{
		case SingleScene:
			u32ChnNum = 1;
			pstLayout = s_astPreviewLayout1;
			break;
		case TwoScene:
			u32ChnNum = 2;
			pstLayout = s_astPreviewLayout2;
			break;
		case ThreeScene:
			u32ChnNum = 3;
			pstLayout = s_astPreviewLayout3;
			break;
		case FourScene:
		case LinkFourScene:
			u32ChnNum = 4;
			pstLayout = s_astPreviewLayout4;
			break;
		case FiveScene:
			u32ChnNum = 5;
			pstLayout = s_astPreviewLayout5;
			break;
		case SixScene:
			u32ChnNum = 6;
			pstLayout = s_astPreviewLayout6;
			break;
		case SevenScene:
			u32ChnNum = 7;
			pstLayout = s_astPreviewLayout7;
			break;
		case EightScene:
			u32ChnNum = 8;
			pstLayout = s_astPreviewLayout8;
			break;
		case NineScene:
		case LinkNineScene:
			u32ChnNum = 9;
			pstLayout = s_astPreviewLayout9;
			break;
		case SixteenScene:
			u32ChnNum = 16;
			pstLayout = s_astPreviewLayout16;
			break;
		default:
			RET_FAILURE("Invalid Parameter: enPreviewMode");
	}
	PRV_HideAllVoChn(VoDev);
#if defined(SN9234H1)
	PRV_ViUnBindAllVoChn(VoDev);
	PRV_VdecUnBindAllVoChn1(VoDev);
	PRV_ResetVideoLayer(VoDev);
#else
	PRV_VdecUnBindAllVpss(VoDev);
	for(i = 0; i < PRV_VO_CHN_NUM; i++)
	{
#if defined(Hi3535)
		PRV_VO_UnBindVpss(VoDev, i, i, VPSS_BSTR_CHN);
#else
		PRV_VO_UnBindVpss(VoDev, i, i, VPSS_PRE0_CHN);
#endif			
	}
	PRV_ClearAllVoChnBuf(VoDev);
	PRV_ResetVideoLayer(VoDev);

	for(i = 0;i < PRV_VO_CHN_NUM; i++)
	{
		if(VoDev == DHD0)
		{			
			HI_MPI_VO_DisableChn(VoDev, i);
#if defined(Hi3535)
			s32Ret = PRV_VO_BindVpss(VoDev, i, i,VPSS_BSTR_CHN);
		    if (HI_SUCCESS != s32Ret)
		    {
		        TRACE(SCI_TRACE_NORMAL, MOD_PRV,"PRV_VO_BindVpss VPSS_PRE0_CHN failed!\n");
		        return -1;
		    }
#else
			s32Ret = PRV_VO_BindVpss(VoDev, i, i,VPSS_PRE0_CHN);
		    if (HI_SUCCESS != s32Ret)
		    {
		        TRACE(SCI_TRACE_NORMAL, MOD_PRV,"PRV_VO_BindVpss VPSS_PRE0_CHN failed!\n");
		        return -1;
		    }
#endif
		}
	}
#endif	
	//PRV_ClearAllVoChnBuf(VoDev);
	u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width;
	u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height;
	
	if(enPreviewMode == NineScene || enPreviewMode == LinkNineScene || enPreviewMode == ThreeScene || enPreviewMode == FiveScene
		|| enPreviewMode == SevenScene)
	{
	//���9����Ԥ��ʱ�������л���֮����ڷ�϶������
		while(u32Width%6 != 0)
			u32Width++;
		while(u32Height%6 != 0)
			u32Height++;
	}
	TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----u32Index:%d-----u32Width: %d, u32Height: %d\n", u32Index, u32Width, u32Height);
	PRV_HideAllVoChn(VoDev);
	//for(i = 0; i < Max_num; i++)
    for (i = 0; i < u32ChnNum && u32Index + i < Max_num; i++)
    {
		//��λ���ƺͱ����������£�����ͨ���Ƿ����أ�ͨ��˳����Σ�����ͨ������ʾ
		if(PRV_CurDecodeMode == PassiveDecode)
		{
			VoChn = i + u32Index;
		}
		else if(enPreviewMode == SingleScene && LayoutToSingleChn >= 0)
		{
			VoChn = LayoutToSingleChn;
		}
		else
		{
			VoChn = pOrder[(u32Index + i) % Max_num];	
		}
		
		order_buf[i] = VoChn;
		//printf("i: %d, u32ChnNum: %d, PRV_CurDecodeMode: %d, Vochn: %d\n", i, u32ChnNum, PRV_CurDecodeMode, VoChn);
		/* �ж�VoChn�Ƿ���Ч */
		if (VoChn < 0 || VoChn >= g_Max_Vo_Num)
		{
			continue;//break;//
		}
		index = PRV_GetVoChnIndex(VoChn);
		//���������£�û��"δ����"ѡ��
		if(PRV_CurDecodeMode == PassiveDecode && VochnInfo[index].VdecChn == DetVLoss_VdecChn && IsTest == 0)
		{
			VochnInfo[index].VdecChn = NoConfig_VdecChn;	
		}
		if(PRV_CurDecodeMode == SwitchDecode && IsTest == 0)//��������û�д�������ͨ��:NoConfig_VdecChn
		{
			if(ScmGetListCtlState() == 0 && SCM_ChnConfigState(VoChn) == 0  && PRV_GetDoubleIndex() == 0)//�ǵ�λ�����£�ͨ��δ����ʱ����
			{
				VochnInfo[index].VdecChn = NoConfig_VdecChn;
			}
			
		}
		{
			CHECK_RET(HI_MPI_VO_GetChnAttr(VoDev, VoChn, &stVoChnAttr));

	        stVoChnAttr.stRect.s32X		 = (u32Width * pstLayout[i].s32X) / PRV_PREVIEW_LAYOUT_DIV;
	        stVoChnAttr.stRect.s32Y		 = (u32Height * pstLayout[i].s32Y) / PRV_PREVIEW_LAYOUT_DIV;
			stVoChnAttr.stRect.u32Width	 = (u32Width * pstLayout[i].u32Width) / PRV_PREVIEW_LAYOUT_DIV;
			stVoChnAttr.stRect.u32Height = (u32Height * pstLayout[i].u32Height) / PRV_PREVIEW_LAYOUT_DIV;

			if(enPreviewMode == NineScene || enPreviewMode == LinkNineScene)
			{ 
				if((i + 1) % 3 == 0)//���һ��
					stVoChnAttr.stRect.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width - stVoChnAttr.stRect.s32X;
				if(i > 5 && i < 9)//���һ��
					stVoChnAttr.stRect.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height - stVoChnAttr.stRect.s32Y;
			}
			
			if( enPreviewMode == ThreeScene )
			{ 
			    if( i == 2)//���һ��
				     stVoChnAttr.stRect.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width - stVoChnAttr.stRect.s32X;
						//���һ��
					 stVoChnAttr.stRect.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height - stVoChnAttr.stRect.s32Y;
			}
			if( enPreviewMode == FiveScene )
			{ 
			    if( i > 1 )//���һ��
				     stVoChnAttr.stRect.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width - stVoChnAttr.stRect.s32X;
				if(i==0 || i==1 || i==4)//���һ��
				     stVoChnAttr.stRect.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height - stVoChnAttr.stRect.s32Y;
			}
			if(enPreviewMode == SevenScene)
			{ 
			    if(i==2 || i==4 || i==6)//���һ��
				     stVoChnAttr.stRect.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width - stVoChnAttr.stRect.s32X;
				if(i==0 || i==5 || i==6)//���һ��
				     stVoChnAttr.stRect.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height - stVoChnAttr.stRect.s32Y;
			}
			//"��������Ƶ"��"δ����"ͼƬ��СΪ352*80�����ֻ����ָ�������н�ȡһ������ʾ����
			if(IsTest == 0//��������ʱ������ͬһ������ͨ����������Ҫ������ʾ����
				&& (VochnInfo[index].VdecChn == DetVLoss_VdecChn || VochnInfo[index].VdecChn == NoConfig_VdecChn))
			{				
		        //stVoChnAttr.stRect.s32X		 = stVoChnAttr.stRect.s32X + (stVoChnAttr.stRect.u32Width - (NOVIDEO_IMAGWIDTH * pstLayout[i].u32Width / PRV_PREVIEW_LAYOUT_DIV)) / 2;
		        //stVoChnAttr.stRect.s32Y		 = stVoChnAttr.stRect.s32Y + (stVoChnAttr.stRect.u32Height - (NOVIDEO_IMAGHEIGHT * pstLayout[i].u32Height / PRV_PREVIEW_LAYOUT_DIV))/ 2;
				//stVoChnAttr.stRect.u32Width	 = (NOVIDEO_IMAGWIDTH * pstLayout[i].u32Width) / PRV_PREVIEW_LAYOUT_DIV;
				//stVoChnAttr.stRect.u32Height = (NOVIDEO_IMAGHEIGHT * pstLayout[i].u32Height) / PRV_PREVIEW_LAYOUT_DIV;
#if defined(Hi3531)||defined(Hi3535)
				if(stVoChnAttr.stRect.u32Height < 32)
				{
					stVoChnAttr.stRect.s32Y = stVoChnAttr.stRect.s32Y - (32 - stVoChnAttr.stRect.u32Height)/2;
					stVoChnAttr.stRect.u32Height = 32;
				}
#endif				
			}
			else
			{
				stSrcRect.s32X		 = stVoChnAttr.stRect.s32X;
				stSrcRect.s32Y		 = stVoChnAttr.stRect.s32Y;
				stSrcRect.u32Width	 = stVoChnAttr.stRect.u32Width;
				stSrcRect.u32Height  = stVoChnAttr.stRect.u32Height;				
				stDestRect = PRV_ReSetVoRect(OutPutMode, VochnInfo[index].VideoInfo.width, VochnInfo[index].VideoInfo.height, stSrcRect);
				stVoChnAttr.stRect.s32X 	 = stDestRect.s32X;
				stVoChnAttr.stRect.s32Y 	 = stDestRect.s32Y;
				stVoChnAttr.stRect.u32Width  = stDestRect.u32Width;
				stVoChnAttr.stRect.u32Height = stDestRect.u32Height;
			}
			
			stVoChnAttr.stRect.s32X 	 &= (~0x01);
			stVoChnAttr.stRect.s32Y 	 &= (~0x01);
			stVoChnAttr.stRect.u32Width  &= (~0x01);
			stVoChnAttr.stRect.u32Height &= (~0x01);
			//HostSendHostToSlaveStream(0,0,0,0);
			//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "i: %d,X: %d, Y: %d, W: %d, H: %d\n", i, stVoChnAttr.stRect.s32X, stVoChnAttr.stRect.s32Y, stVoChnAttr.stRect.u32Width, stVoChnAttr.stRect.u32Height);
			//printf("i: %d,X: %d, Y: %d, W: %d, H: %d, stVoChnAttr.u32Priority: %d\n", i, stVoChnAttr.stRect.s32X, stVoChnAttr.stRect.s32Y, stVoChnAttr.stRect.u32Width, stVoChnAttr.stRect.u32Height, stVoChnAttr.u32Priority);

			CHECK_RET(HI_MPI_VO_SetChnAttr(VoDev, VoChn, &stVoChnAttr));

			//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "-------VoChn: %d, x:%d, y:%d, width: %d, height: %d\n", VoChn, stVoChnAttr.stRect.s32X, stVoChnAttr.stRect.s32Y, stVoChnAttr.stRect.u32Width, stVoChnAttr.stRect.u32Height);
#if !defined(Hi3535)
			if((enPreviewMode == EightScene && 0 == i))
			{
				//8����ĵ�һ��������both��ʾ
				CHECK_RET(HI_MPI_VO_SetChnField(VoDev, VoChn, VO_FIELD_BOTH));
			}
			else if(enPreviewMode < EightScene)
			{
				CHECK_RET(HI_MPI_VO_SetChnField(VoDev, VoChn, VO_FIELD_BOTH));
			}
			else
			{
				CHECK_RET(HI_MPI_VO_SetChnField(VoDev, VoChn, VO_FIELD_BOTTOM));
			}
#endif
		}
#if defined(SN9234H1)
		if(VoChn < (LOCALVEDIONUM < PRV_CHAN_NUM ? LOCALVEDIONUM : PRV_CHAN_NUM))
		{
			int videv = 0;
			if(VoChn >= PRV_VI_CHN_NUM)
			{
			//���ͨ��Ϊ5��8����ô��Ӧ�ɼ��豸2
				videv = PRV_656_DEV;
			}
			else
			{
				videv = PRV_656_DEV_1;
			}
			CHECK_RET(HI_MPI_VI_BindOutput(videv, VoChn%PRV_VI_CHN_NUM, VoDev, VoChn));

			//printf("###########VoChn = %d ,VoDev = %d, videv = %d,vichn = %d ######################\n",VoChn,VoDev,videv,VoChn%PRV_VI_CHN_NUM);
		}
		else 
#endif
		{			
			//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "-----index: %d---VoDev: %d----Bind---SlaveId: %d, IsBindVdec: %d, VochnInfo[index].VdecChn: %d, VoChn: %d\n",index, VoDev, VochnInfo[index].SlaveId, VochnInfo[index].IsBindVdec[VoDev], VochnInfo[index].VdecChn, VoChn);
			if(VochnInfo[index].VdecChn >= 0 && -1 == VochnInfo[index].IsBindVdec[VoDev])
			{
				if(VochnInfo[index].SlaveId == PRV_MASTER)
				{
#if defined(SN9234H1)
					CHECK(HI_MPI_VDEC_BindOutput(VochnInfo[index].VdecChn, VoDev, VoChn));
#else
					PRV_VDEC_UnBindVpss(VochnInfo[index].VdecChn, VoChn);
					PRV_VPSS_ResetWH(VoChn,VochnInfo[index].VdecChn,VochnInfo[index].VideoInfo.width,VochnInfo[index].VideoInfo.height);
					CHECK(PRV_VDEC_BindVpss(VochnInfo[index].VdecChn, VoChn));
#endif					
					VochnInfo[index].IsBindVdec[VoDev] = ((VochnInfo[index].VdecChn == DetVLoss_VdecChn || VochnInfo[index].VdecChn == NoConfig_VdecChn) ? 0 : 1);
				}
#if defined(SN9234H1)
				else if(VochnInfo[index].SlaveId > PRV_MASTER/* && VochnInfo[index].IsHaveVdec*/)//��Ƭ�����ý���ͨ���Ѿ���ʼ��������
				{//��Ƭ�󶨵������豸0��
					int w = 0, h = 0;

					w = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width;
					h = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height;
					stSrcRect.s32X 	    = (w * pstLayout[i].s32X) / PRV_PREVIEW_LAYOUT_DIV;
					stSrcRect.s32Y 	    = (h * pstLayout[i].s32Y) / PRV_PREVIEW_LAYOUT_DIV;
					stSrcRect.u32Width  = (w * pstLayout[i].u32Width) / PRV_PREVIEW_LAYOUT_DIV;
					stSrcRect.u32Height = (h * pstLayout[i].u32Height) / PRV_PREVIEW_LAYOUT_DIV;

					stDestRect = PRV_ReSetVoRect(OutPutMode, VochnInfo[index].VideoInfo.width, VochnInfo[index].VideoInfo.height, stSrcRect);
					
					stVoZoomAttr.stZoomRect.s32X		= stDestRect.s32X      & (~0x01);
					stVoZoomAttr.stZoomRect.s32Y		= stDestRect.s32Y      & (~0x01);
					stVoZoomAttr.stZoomRect.u32Width	= stDestRect.u32Width  & (~0x01);
					stVoZoomAttr.stZoomRect.u32Height	= stDestRect.u32Height & (~0x01);
					//stVoZoomAttr.enField = VIDEO_FIELD_INTERLACED;
					stVoZoomAttr.enField = VIDEO_FIELD_FRAME;
					
					//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "-------x:%d, y:%d, width: %d, height: %d\n", stVoZoomAttr.stZoomRect.s32X, stVoZoomAttr.stZoomRect.s32Y,stVoZoomAttr.stZoomRect.u32Width, stVoZoomAttr.stZoomRect.u32Height);
					//printf("-------x:%d, y:%d, width: %d, height: %d\n", stVoZoomAttr.stZoomRect.s32X, stVoZoomAttr.stZoomRect.s32Y,stVoZoomAttr.stZoomRect.u32Width, stVoZoomAttr.stZoomRect.u32Height);
					CHECK_RET(HI_MPI_VO_SetZoomInWindow(VoDev, VoChn, &stVoZoomAttr));
					CHECK(HI_MPI_VI_BindOutput(PRV_HD_DEV, 0, VoDev, VoChn));
					VochnInfo[index].IsBindVdec[VoDev] = 1;
				}
#endif
			}			
		}
#if defined(Hi3535)
		CHECK_RET(HI_MPI_VO_ShowChn(VoDev, VoChn));
#else		
		CHECK_RET(HI_MPI_VO_ChnShow(VoDev, VoChn));
#endif
		CHECK_RET(HI_MPI_VO_EnableChn(VoDev, VoChn));		
	}
#if defined(SN9234H1)
	PRV_VdecBindAllVoChn(VoDev);
#else
	//PRV_VdecBindAllVpss(VoDev);
#endif
	sem_post(&sem_SendNoVideoPic);
	OSD_Get_Preview_param(VoDev,
		s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width,
		s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height,
		i, enPreviewMode,order_buf);

#if 0 /*2010-9-17 ��̬����VIͨ��ͼ���С*/
	PRV_SetViChnAttrByPreviewMode();
#endif

	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_RefreshVoDevScreen
Description: ����VO״̬ˢ��VO�豸����ʾ
Calls: 
Called By: //
Input: //VoDev:�豸��
		Is_Double: �Ƿ�˫����ʾ
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_RefreshVoDevScreen(VO_DEV VoDev, HI_U32 Is_Double, VO_CHN *pOrder)
{
	VO_DEV VoDev2 = VoDev;
#if defined(SN9234H1)
	sem_post(&sem_SendNoVideoPic);
again:
	if(VoDev != HD)
#else
	Is_Double = DISP_NOT_DOUBLE_DISP;
again:
	if(VoDev != DHD0)
#endif

	{
		RET_SUCCESS("");
	}	
	//1���ж�״̬��enPreviewStat
	switch (s_astVoDevStatDflt[VoDev].enPreviewStat)
	{
		case PRV_STAT_NORM:
			{				
				//2���Ƿ񱨾�bIsAlarm
				if (s_astVoDevStatDflt[VoDev].bIsAlarm)
				{
					//�е�������s32AlarmChn
					//HI_S32 s32Index;
					//CHECK_RET(PRV_Chn2Index(VoDev, s_astVoDevStatDflt[VoDev].s32AlarmChn, &s32Index,pOrder));
					PRV_PreviewVoDevSingle(VoDev, s_astVoDevStatDflt[VoDev].s32AlarmChn);
				}
				else if (s_astVoDevStatDflt[VoDev].bIsSingle)
				{
					//�е�������s32SingleIndex
					if(PRV_CurDecodeMode == PassiveDecode)
					{
						PRV_PreviewVoDevInMode(VoDev, SingleScene, DoubleToSingleIndex, pOrder);
					}
					else
					{
						PRV_PreviewVoDevInMode(VoDev, SingleScene, s_astVoDevStatDflt[VoDev].s32SingleIndex, pOrder);
					}
				}
				else
				{
					//�е��໭��s32PreviewIndex
					PRV_PreviewVoDevInMode(VoDev, s_astVoDevStatDflt[VoDev].enPreviewMode, s_astVoDevStatDflt[VoDev].s32PreviewIndex, pOrder);
				}
			}
			break;
		case PRV_STAT_PB:
		case PRV_STAT_PIC:	
			{
				//���VoDev������VoChn
//				int i = 0;
#if defined(SN9234H1)
				PRV_ViUnBindAllVoChn(VoDev);				
				PRV_VdecUnBindAllVoChn1(VoDev);
				PRV_ClearAllVoChnBuf(VoDev);
				PRV_DisableAllVoChn(VoDev);
				CHECK_RET(HI_MPI_VO_DisableVideoLayer(VoDev));
				s_astVoDevStatDflt[VoDev].stVideoLayerAttr.s32PiPChn = VO_DEFAULT_CHN;
				CHECK_RET(HI_MPI_VO_SetVideoLayerAttr(VoDev, &s_astVoDevStatDflt[VoDev].stVideoLayerAttr));
				CHECK_RET(HI_MPI_VO_EnableVideoLayer(VoDev));
#else
				//PRV_ViUnBindAllVoChn(VoDev);	
				PRV_HideAllVoChn(VoDev);
				PRV_VdecUnBindAllVpss(VoDev);
				PRV_VoUnBindAllVpss(VoDev);
				PRV_ClearAllVoChnBuf(VoDev);
				PRV_DisableAllVoChn(VoDev);
				//CHECK_RET(HI_MPI_VO_DisableVideoLayer(VoDev));
				PRV_DisableVideoLayer(VoDev);
				//s_astVoDevStatDflt[VoDev].stVideoLayerAttr.s32PiPChn = VO_DEFAULT_CHN;
				CHECK_RET(HI_MPI_VO_SetVideoLayerAttr(VoDev, &s_astVoDevStatDflt[VoDev].stVideoLayerAttr));
				CHECK_RET(HI_MPI_VO_EnableVideoLayer(VoDev));
				CHECK_RET(PRV_EnablePipLayer(VoDev));
				CHECK_RET(HI_MPI_VO_SetPlayToleration (VoDev, 200));
#endif			
				PRV_SetPreviewVoDevInMode(1);
			}
			break;
		case PRV_STAT_CTRL:
			{
				//3���жϿ���״̬����
				switch (s_astVoDevStatDflt[VoDev].enCtrlFlag)
				{
					case PRV_CTRL_REGION_SEL:
					case PRV_CTRL_ZOOM_IN:
					case PRV_CTRL_PTZ:
						{
							//�л���������s32CtrlChn
						//	HI_S32 s32Index,ret=0;


							//CHECK_RET(PRV_Chn2Index(VoDev, s_astVoDevStatDflt[VoDev].s32CtrlChn, &s32Index));
						//	ret = PRV_Chn2Index(VoDev, s_astVoDevStatDflt[VoDev].s32CtrlChn,&s32Index,pOrder);
							//printf("2222222222222s_astVoDevStatDflt[VoDev].s32CtrlCh = %d ,s32Index =%d2222222222222\n",s_astVoDevStatDflt[VoDev].s32CtrlChn,s32Index);
							//if(ret == HI_FAILURE)
							{//�����ǰͨ�������أ�����������ʱ������Ҫ��ʾ
								PRV_PreviewVoDevSingle(VoDev, s_astVoDevStatDflt[VoDev].s32CtrlChn);
							}
							//else
							{
							//	PRV_PreviewVoDevInMode(VoDev, SingleScene, s32Index,pOrder);
							}
						}
						break;
					default:
						RET_FAILURE("Invalid Parameter: enCtrlFlag");
				}
			}
			break;
		default:
			RET_FAILURE("Invalid Parameter: enPreviewStat");
	}
#if defined(SN9234H1)
	if(VoDev == HD)
	{
		PRV_Check_LinkageGroup(VoDev,s_astVoDevStatDflt[VoDev].enPreviewMode);
	}
#else
	if(VoDev == DHD0)
	{
		PRV_Check_LinkageGroup(VoDev,s_astVoDevStatDflt[VoDev].enPreviewMode);
	}
#endif	
//	printf("s_State_Info.bIsOsd_Init=%d,s_State_Info.bIsRe_Init=%d\n",s_State_Info.bIsOsd_Init,s_State_Info.bIsRe_Init);
	if((s_State_Info.bIsOsd_Init && s_State_Info.bIsRe_Init) && (PRV_STAT_PB != s_astVoDevStatDflt[VoDev].enPreviewStat && PRV_STAT_PIC != s_astVoDevStatDflt[VoDev].enPreviewStat))
	{
#if defined(SN9234H1)
		if(VoDev == s_VoSecondDev)
		{
			PRV_Check_LinkageGroup(AD,s_astVoDevStatDflt[VoDev].enPreviewMode);
			Prv_Disp_OSD(AD);
		}
#else
		if(VoDev == s_VoSecondDev || VoDev == DHD1)
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "VoDev == s_VoSecondDev\n");
			PRV_Check_LinkageGroup(DSD0,s_astVoDevStatDflt[VoDev].enPreviewMode);
			Prv_Disp_OSD(DSD0);
		}
#endif		
		else
		{
			Prv_Disp_OSD(VoDev);
		}
	}
#if 1 /*2010-9-19 ˫����*/
	//printf("##########s_VoSecondDev = %d######################\n",s_VoSecondDev);
	if(Is_Double == DISP_DOUBLE_DISP)
	{
		if (VoDev2 == VoDev)
		{
#if defined(SN9234H1)
			switch(VoDev)
			{
				case HD:
					{
						VO_PUB_ATTR_S stVoPubAttr = s_astVoDevStatDflt[s_VoSecondDev].stVoPubAttr;
						VO_VIDEO_LAYER_ATTR_S stVideoLayerAttr = s_astVoDevStatDflt[s_VoSecondDev].stVideoLayerAttr;

						s_astVoDevStatDflt[s_VoSecondDev] = s_astVoDevStatDflt[HD];
						s_astVoDevStatDflt[s_VoSecondDev].stVoPubAttr = stVoPubAttr;
						s_astVoDevStatDflt[s_VoSecondDev].stVideoLayerAttr = stVideoLayerAttr;

						VoDev = s_VoSecondDev;
						goto again;
					}
					break;
				//case s_VoSecondDev:
				case AD:
					{
						VO_PUB_ATTR_S stVoPubAttr = s_astVoDevStatDflt[HD].stVoPubAttr;
						VO_VIDEO_LAYER_ATTR_S stVideoLayerAttr = s_astVoDevStatDflt[HD].stVideoLayerAttr;
						
						s_astVoDevStatDflt[HD] = s_astVoDevStatDflt[AD];
						s_astVoDevStatDflt[HD].stVoPubAttr = stVoPubAttr;
						s_astVoDevStatDflt[HD].stVideoLayerAttr = stVideoLayerAttr;
						
						VoDev = HD;
						goto again;
					}
					break;
				case SD:
					{
						VO_PUB_ATTR_S stVoPubAttr = s_astVoDevStatDflt[HD].stVoPubAttr;
						VO_VIDEO_LAYER_ATTR_S stVideoLayerAttr = s_astVoDevStatDflt[HD].stVideoLayerAttr;
						
						s_astVoDevStatDflt[HD] = s_astVoDevStatDflt[SD];
						s_astVoDevStatDflt[HD].stVoPubAttr = stVoPubAttr;
						s_astVoDevStatDflt[HD].stVideoLayerAttr = stVideoLayerAttr;
						
						VoDev = HD;
						goto again;
					}
					break;
				default:
					break;
			}
#else
			switch(VoDev)
			{
				case DHD0:
					{
						VO_PUB_ATTR_S stVoPubAttr = s_astVoDevStatDflt[s_VoSecondDev].stVoPubAttr;
						VO_VIDEO_LAYER_ATTR_S stVideoLayerAttr = s_astVoDevStatDflt[s_VoSecondDev].stVideoLayerAttr;

						s_astVoDevStatDflt[s_VoSecondDev] = s_astVoDevStatDflt[DHD0];
						s_astVoDevStatDflt[s_VoSecondDev].stVoPubAttr = stVoPubAttr;
						s_astVoDevStatDflt[s_VoSecondDev].stVideoLayerAttr = stVideoLayerAttr;

						VoDev = s_VoSecondDev;
						goto again;
					}
					break;
				//case s_VoSecondDev:
				case DHD1:
				case DSD0:
					{
						VO_PUB_ATTR_S stVoPubAttr = s_astVoDevStatDflt[DHD0].stVoPubAttr;
						VO_VIDEO_LAYER_ATTR_S stVideoLayerAttr = s_astVoDevStatDflt[DHD0].stVideoLayerAttr;
						
						s_astVoDevStatDflt[DHD0] = s_astVoDevStatDflt[DSD0];
						s_astVoDevStatDflt[DHD0].stVoPubAttr = stVoPubAttr;
						s_astVoDevStatDflt[DHD0].stVideoLayerAttr = stVideoLayerAttr;
						
						VoDev = DHD0;
						goto again;
					}
					break;
				default:
					break;
			}
#endif			
		}
	}
#endif
	//������Ƶ
	PRV_PlayAudio(VoDev);	
	RET_SUCCESS("");
}
/*************************************************
Function: //PRV_ResetVo
Description: ����ָ����VO�豸���������ϵ���Ƶ�㼰ͨ����
Calls: 
Called By: //
Input: //VoDev:�豸��
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_ResetVo(VO_DEV VoDev)
{
//	int flag = 0;
	TRACE(SCI_TRACE_NORMAL, MOD_PRV,"##########s_VoDevCtrlDflt = %d,VoDev = %d################\n",s_VoDevCtrlDflt,VoDev);
	pthread_mutex_lock(&s_Reset_vo_mutex);
#if defined(Hi3531)||defined(Hi3535)	
	PRV_HideAllVoChn(VoDev);
	PRV_VoUnBindAllVpss(VoDev);
#endif	
	PRV_DisableAllVoChn(VoDev);
	usleep(200000);//��ʱ200���룬��ͨ����Դ�����ͷ�
	if(PRV_DisableVideoLayer(VoDev) == HI_FAILURE)
	{//������һ��
		PRV_DisableAllVoChn(VoDev);
		usleep(100000);//��ʱ100���룬��ͨ����Դ�����ͷ�
		PRV_DisableVideoLayer(VoDev);
	}
	PRV_DisableVoDev(VoDev);

	if(PRV_EnableVoDev(VoDev) == HI_FAILURE)
	{
		pthread_mutex_unlock(&s_Reset_vo_mutex);
		RET_FAILURE("PRV_EnableVoDev(VoDev)");;
	}
	if(PRV_EnableVideoLayer(VoDev) == HI_FAILURE)
	{
		pthread_mutex_unlock(&s_Reset_vo_mutex);
		RET_FAILURE("PRV_EnableVideoLayer(VoDev)");;
	}
	if(PRV_EnableAllVoChn(VoDev) == HI_FAILURE)
	{
		pthread_mutex_unlock(&s_Reset_vo_mutex);
		RET_FAILURE("PRV_EnableAllVoChn(VoDev)");;
	}
#if defined(SN9234H1)
	//PRV_BindAllVoChn(VoDev);
	//if(s_State_Info.bIsInit)	
	{
		PRV_RefreshVoDevScreen(VoDev, (SD == VoDev) ? DISP_NOT_DOUBLE_DISP : DISP_DOUBLE_DISP,s_astVoDevStatDflt[VoDev].as32ChnOrder[s_astVoDevStatDflt[VoDev].enPreviewMode]);
	}
#else
	PRV_VoBindAllVpss(VoDev);
	//if(s_State_Info.bIsInit)	
	{
		PRV_RefreshVoDevScreen(VoDev, (DHD0 == VoDev) ? DISP_NOT_DOUBLE_DISP : DISP_DOUBLE_DISP, s_astVoDevStatDflt[VoDev].as32ChnOrder[s_astVoDevStatDflt[VoDev].enPreviewMode]);
	}
#endif	
	pthread_mutex_unlock(&s_Reset_vo_mutex);
	
	RET_SUCCESS("");
}


/*************************************************
Function: //PRV_ResetVoDev
Description: ����ָ����VO�豸���������ϵ���Ƶ�㼰ͨ����
Calls: 
Called By: //
Input: //VoDev:�豸��
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_ResetVoDev(VO_DEV VoDev)
{
	//HI_S32 i = LOCALVEDIONUM;
#if defined(SN9234H1)
	PRV_ViUnBindAllVoChn(VoDev);
#endif
	//for(;i < DEV_CHANNEL_NUM; i++)
	//	PRV_VdecUnBindAllVoChn(VoDev, i);
#if defined(SN9234H1)
	PRV_VdecUnBindAllVoChn1(VoDev);
#else
	PRV_VdecUnBindAllVpss(VoDev);
#endif
//	PRV_VoUnBindAllVpss(VoDev);
	PRV_ResetVo(VoDev);
	
    RET_SUCCESS("");
}
/*************************************************
Function: //PRV_OpenVoFb
Description: ��ָ����VO�豸��GUI�����
Calls: 
Called By: //
Input: //VoDev:�豸��
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_OpenVoFb(VO_DEV VoDev)
{
	int h, w;
	char fb_name[16];
	
#if defined(Hi3531)||defined(Hi3535)	
	if (VoDev < 0 || VoDev > DHD0/*VoDev >= PRV_VO_MAX_DEV*/)
		RET_FAILURE("Invalid Parameter: VoDev");
#endif		
	pthread_mutex_lock(&s_Reset_vo_mutex);

	h = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height;
	w = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width;
	
	//printf("##########h = %d,w = %d,VoDev=%d################\n",h,w,VoDev);
#if defined(SN9234H1)
	if(VoDev == HD)
	{
		SN_SPRINTF(fb_name,16,"/dev/fb1");
	}
	else if(VoDev == s_VoSecondDev)
	{
		/*if(s_VoSecondDev == SD)
		{
			SN_SPRINTF(fb_name,16,"/dev/fb3");
		}
		else
		{*/
			SN_SPRINTF(fb_name,16,"/dev/fb4");
		//}
	}
	else
	{
		SN_SPRINTF(fb_name,16,"/dev/fb1");
	}
#else
#if defined(Hi3535)
	SN_SPRINTF(fb_name,16,"/dev/fb0");
#else
	SN_SPRINTF(fb_name,16,"/dev/fb4");
#endif
#endif
	MMIOpenFB(fb_name, w, h, 16);
		
	pthread_mutex_unlock(&s_Reset_vo_mutex);
	
    RET_SUCCESS("PRV_OpenVoFb");
}
/*************************************************
Function: //PRV_CloseVoFb
Description: �ر�ָ����VO�豸��GUI�����
Calls: 
Called By: //
Input: //VoDev:�豸��
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_CloseVoFb(VO_DEV VoDev)
{
#if defined(USE_UI_OSD)
	MMI_OsdReset();
#endif
	//printf("##########s_VoDevCtrlDflt = %d,VoDev = %d################\n",s_VoDevCtrlDflt,VoDev);
	pthread_mutex_lock(&s_Reset_vo_mutex);
	Get_Fb_param_exit();
	MMICloseFB();
	pthread_mutex_unlock(&s_Reset_vo_mutex);
	
    RET_SUCCESS("PRV_CloseVoFb");
}

#if (IS_DECODER_DEVTYPE == 1)

#else

/*************************************************
Function: //PRV_NextScreen
Description:��һ������һ��
Calls: 
Called By: //
Input: // VoDev: ����豸
   		s32Dir: 0-��һ����1-��һ��
  		 bIsManual: �Ƿ��ֶ���/��һ������
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_NextScreen(VO_DEV VoDev, HI_S32 s32Dir, HI_BOOL bIsAuto/*�Ƿ��Զ���һ����HI_TRUE:��, HI_FALSE:��*/)
{
	HI_U32 u32ChnNum;
	HI_U32 Max_num;
	unsigned char mode = s_astVoDevStatDflt[VoDev].enPreviewMode;	

	if(s_astVoDevStatDflt[VoDev].bIsSingle == HI_TRUE)
	{
		mode = SingleScene;
	}			
	//��ȡ��ǰԤ��ģʽ�����ͨ����
	CHECK_RET(PRV_Get_Max_chnnum(mode,&Max_num));

	/*ȷ�������ĺϷ���*/
#if defined(Hi3520)
	if(VoDev == SPOT_VO_DEV|| VoDev == AD)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}
#endif
	if( VoDev < 0 || VoDev >= PRV_VO_MAX_DEV )
	{
		RET_FAILURE("Invalid Parameter: VoDev");
	}

	if (PRV_STAT_NORM != s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat )
	{
		RET_FAILURE("NOT in preview stat!!!");
	}

	switch(mode)
	{
		case SingleScene:
			u32ChnNum = 1;
			break;
		case TwoScene:
			u32ChnNum = 2;
			break;
		case ThreeScene:
			u32ChnNum = 3;
			break;
		case FourScene:
		case LinkFourScene:
			u32ChnNum = 4;
			break;
		case FiveScene:
			u32ChnNum = 5;
			break;
		case SixScene:
			u32ChnNum = 6;
			break;
		case SevenScene:
			u32ChnNum = 7;
			break;
		case EightScene:
			u32ChnNum = 8;
			break;
		case NineScene:
		case LinkNineScene:
			u32ChnNum = 9;
			break;
		case SixteenScene:
			u32ChnNum = 16;
			break;
		default:
			RET_FAILURE("Invalid Preview Mode");
	}
	
	if (s_astVoDevStatDflt[VoDev].bIsSingle)
	{	/*������*/
		//������һ����Ҫ��ѯ�Ļ���������
		CHECK_RET(PRV_GetValidChnIdx(VoDev, s_astVoDevStatDflt[VoDev].s32SingleIndex, &s_astVoDevStatDflt[VoDev].s32SingleIndex, s32Dir,s_astVoDevStatDflt[VoDev].as32ChnOrder[mode],s_astVoDevStatDflt[VoDev].as32ChnpollOrder[mode]));	
		s_astVoDevStatDflt[VoDev].s32DoubleIndex = 0;
	}
	else
	{	/*�໭��*/
		HI_S32 s32Index;
		//�����ǰ����ģʽΪ�����ģʽ����ô��������ѯ
		
		if (PRV_VO_MAX_MOD == mode)
		{
			RET_SUCCESS("PRV_VO_MAX_MOD!!");
		}
		//�����ǰΪ8ͨ������ô8����Ҳ����Ҫ��ѯ
		if((g_Max_Vo_Num == 8) && (EightScene == mode))
		{
			RET_SUCCESS("PRV_VO_MAX_MOD!!");
		}
		do 
		{
			if (0 == s32Dir)//0-��һ��;
			{
				s32Index = (s_astVoDevStatDflt[VoDev].s32PreviewIndex+u32ChnNum >= Max_num ) ? 0 : (s_astVoDevStatDflt[VoDev].s32PreviewIndex+u32ChnNum);
			}
			else if (1 == s32Dir)//1-��һ��;
			{
				s32Index = (s_astVoDevStatDflt[VoDev].s32PreviewIndex < u32ChnNum) 
					? ((s_astVoDevStatDflt[VoDev].s32PreviewIndex==0)?((Max_num-1)/u32ChnNum)*u32ChnNum:0) 
					: (s_astVoDevStatDflt[VoDev].s32PreviewIndex-u32ChnNum);
			}
			else
			{
				RET_FAILURE("Invalid redirection value!!!");
			}
			
			s_astVoDevStatDflt[VoDev].s32PreviewIndex = s32Index;
#if 1
			/*��������*/
			if (s_astVoDevStatDflt[VoDev].as32ChnOrder[mode][s32Index] >= 0
				&& s_astVoDevStatDflt[VoDev].as32ChnOrder[mode][s32Index] < g_Max_Vo_Num)
			{
				//printf("############s_astVoDevStatDflt[VoDev].as32ChnOrder[mode][s32Index] = %d,,,s32Index = %d###############\n",s_astVoDevStatDflt[VoDev].as32ChnOrder[mode][s32Index],s32Index);
				if (bIsAuto)
				{
					//�����ǰ������Ҫ��ѯ
					if(s_astVoDevStatDflt[VoDev].as32ChnpollOrder[mode][s32Index])
					{
						break;
					}
				}else
				{
					break;
				}
			}
			//������һ����Ҫ��ѯ�Ļ���������
			CHECK_RET(PRV_GetValidChnIdx(VoDev, s_astVoDevStatDflt[VoDev].s32PreviewIndex, &s32Index, 0,s_astVoDevStatDflt[VoDev].as32ChnOrder[mode],s_astVoDevStatDflt[VoDev].as32ChnpollOrder[mode]));
			//�жϵ�ǰ�������Ƿ���ͨ���ŷ�Χ��
			if (s32Index > s_astVoDevStatDflt[VoDev].s32PreviewIndex
				&& s32Index < s_astVoDevStatDflt[VoDev].s32PreviewIndex+u32ChnNum)
			{
				break;
			}
#else
			break;
#endif
		} while (1);
		s_astVoDevStatDflt[VoDev].s32PreviewIndex %= Max_num;

		//s_astVoDevStatDflt[VoDev].s32PreviewIndex = s32Index%Max_num;
#if 0
		/*���һ��������*/
		if ((s_astVoDevStatDflt[VoDev].s32PreviewIndex+u32ChnNum) > Max_num)
		{
			s_astVoDevStatDflt[VoDev].s32PreviewIndex = Max_num - u32ChnNum;
		}
#endif
	}
	
	//printf("#############PRV_PreviewVoDevInMode idx = %d ,mode =%d ,Max_num =%d,uidx=%d,ISSINGLE= %d######################\n",
	//		s_astVoDevStatDflt[VoDev].s32SingleIndex,s_astVoDevStatDflt[VoDev].enPreviewMode,Max_num,s_astVoDevStatDflt[VoDev].s32SingleIndex,s_astVoDevStatDflt[VoDev].bIsSingle);
	//ˢ�»���
	if (!s_astVoDevStatDflt[VoDev].bIsAlarm)
	{//���ڷǱ���״̬��ˢ�»���
		PRV_RefreshVoDevScreen(VoDev,DISP_DOUBLE_DISP,s_astVoDevStatDflt[VoDev].as32ChnOrder[mode]);
	}
	RET_SUCCESS("Next/Prev Screen!");
}
#endif
int PRV_IsAllowZoomIn(int VoChn)
{
	if(VochnInfo[VoChn].VdecChn == DetVLoss_VdecChn || VochnInfo[VoChn].VdecChn == NoConfig_VdecChn)
	{
		return HI_FAILURE;
	}
	return HI_SUCCESS;
}
//ͨ�����ӷŴ�
STATIC HI_S32 PRV_ChnZoomIn(VO_CHN VoChn, HI_U32 u32Ratio, const Preview_Point *pstPoint)
{
#if defined(SN9234H1)
	VoChn = s_astVoDevStatDflt[s_VoDevCtrlDflt].s32CtrlChn;//��ʱ���Բ���VoChn
	int index = 0;
	index = PRV_GetVoChnIndex(VoChn);
	if(index < 0)
		RET_FAILURE("------ERR: Invalid Index!");
	if(VoChn < 0 || VoChn >= g_Max_Vo_Num
		|| u32Ratio < PRV_MIN_ZOOM_IN_RATIO || u32Ratio > PRV_MAX_ZOOM_IN_RATIO)
	{
		RET_FAILURE("Invalid Parameter: u32Ratio or VoChn");
	}
	
	if(NULL == pstPoint)
	{
		RET_FAILURE("NULL Pointer!!");
	}
	
	if (PRV_STAT_CTRL != s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat
		&& PRV_CTRL_ZOOM_IN != s_astVoDevStatDflt[s_VoDevCtrlDflt].enCtrlFlag)
	{
		RET_FAILURE("NOT in [zoom in ctrl] stat!!!");
	}
	
	if (VoChn != s_astVoDevStatDflt[s_VoDevCtrlDflt].s32CtrlChn)
	{
		RET_FAILURE("VoChn NOT match current VoChn!!!");
	}
	//printf("vochn=%d, u32ratio=%d, pstPoint->x= %d, pstPoint->y=%d===========================\n",VoChn,u32Ratio,pstPoint->x, pstPoint->y);

//Mϵ�в�Ʒ֧�ָ���(1080P/720P)�������ʾ��������ͨ������Ƭ���ź�Դ�ֱ���̫�󣬽������ź�
//Ҳ����VO_ZOOM_ATTR_S����ķ�Χ������HI_MPI_VO_SetZoomInWindow�᷵��ʧ�ܣ�
//����ڴ�Ƭ����Ϊ�ź�Դ�Ѿ������������������ţ�����Ƭ����ʾʱ����VO_ZOOM_ATTR_S��Χ��
//������֧�ָ�����ͺţ�������Ƭ��ʾ����ʱ����HI_MPI_VO_SetZoomInRatio���е��ӷŴ�

//����Ƭ����ʾ�ĸ���(����D1)ͨ������HI_MPI_VO_SetZoomInRatio���е��ӷŴ�
	if(PRV_MASTER == VochnInfo[index].SlaveId
		&& VochnInfo[index].VdecChn >= 0 )		
	{
		VO_ZOOM_RATIO_S	stZoomRatio;

		if (u32Ratio <= 1)
		{
			stZoomRatio.u32XRatio = 0;
			stZoomRatio.u32YRatio = 0;
			stZoomRatio.u32WRatio = 0;
			stZoomRatio.u32HRatio = 0;
		}
		else
		{
#if 0
			stZoomRatio.u32WRatio = 1000/u32Ratio;
			stZoomRatio.u32HRatio = 1000/u32Ratio;
			stZoomRatio.u32XRatio = ((pstPoint->x * 1000)/s_u32GuiWidthDflt + stZoomRatio.u32WRatio > 1000)
				? 1000 - stZoomRatio.u32WRatio
				: (pstPoint->x * 1000)/s_u32GuiWidthDflt;
			stZoomRatio.u32YRatio = ((pstPoint->y * 1000)/s_u32GuiHeightDflt + stZoomRatio.u32HRatio > 1000)
				? 1000 - stZoomRatio.u32HRatio
				: (pstPoint->y * 1000)/s_u32GuiHeightDflt;

#else /*��1��16���Ŵ�תΪ1��4���Ŵ�y = (x - 1)/5 + 1*/

			u32Ratio += 4;
				
			stZoomRatio.u32WRatio = 5000/u32Ratio;
			stZoomRatio.u32HRatio = 5000/u32Ratio;
			stZoomRatio.u32XRatio = ((pstPoint->x * 1000)/s_u32GuiWidthDflt + stZoomRatio.u32WRatio > 1000)
				? 1000 - stZoomRatio.u32WRatio
				: (pstPoint->x * 1000)/s_u32GuiWidthDflt;
			stZoomRatio.u32YRatio = ((pstPoint->y * 1000)/s_u32GuiHeightDflt + stZoomRatio.u32HRatio > 1000)
				? 1000 - stZoomRatio.u32HRatio
				: (pstPoint->y * 1000)/s_u32GuiHeightDflt;
#endif
		}
//	printf("==================stZoomRatio.u32XRatio = %d; stZoomRatio.u32YRatio = %d; stZoomRatio.u32WRatio =%d; stZoomRatio.u32HRatio = %d;\n",	
//		stZoomRatio.u32XRatio ,stZoomRatio.u32YRatio,stZoomRatio.u32WRatio,stZoomRatio.u32HRatio);
#if 0 /*2010-8-31 �Ż������ӷŴ����ķŴ�ʽ���зŴ�*/
		stZoomRatio.u32XRatio = (stZoomRatio.u32XRatio < stZoomRatio.u32WRatio/2)?0:stZoomRatio.u32XRatio - stZoomRatio.u32WRatio/2;
		stZoomRatio.u32YRatio = (stZoomRatio.u32YRatio < stZoomRatio.u32HRatio/2)?0:stZoomRatio.u32YRatio - stZoomRatio.u32HRatio/2;
#endif
#if 1
		CHECK_RET(HI_MPI_VO_SetZoomInRatio(s_VoDevCtrlDflt, VoChn, &stZoomRatio));
#else /*2010-9-19 ˫����*/
		if(s_State_Info.g_zoom_first_in == HI_FALSE)
		{
			//CHECK_RET(HI_MPI_VO_GetZoomInRatio(HD,VoChn,&s_astZoomAttrDflt[HD]));
			CHECK_RET(HI_MPI_VO_GetZoomInRatio(s_VoDevCtrlDflt,VoChn,&s_astZoomAttrDflt[s_VoDevCtrlDflt]));
			s_State_Info.g_zoom_first_in = HI_TRUE;
		}
		//CHECK_RET(HI_MPI_VO_SetZoomInRatio(HD, VoChn, &stZoomRatio));
		CHECK_RET(HI_MPI_VO_SetZoomInRatio(s_VoDevCtrlDflt, VoChn, &stZoomRatio));
#endif
	}
	else
	//Dϵ�еĲ�Ʒ��֧�ָ��壬���֧��D1(704*576),�����ô��ַ������ӷŴ�
	//Mϵ���ڴ�Ƭ��(SN8616M_LE)��ʾ��ͨ���Լ���Ƭ��ʾ��D1ͨ�����ô��ַ���
	{
		VO_ZOOM_ATTR_S stVoZoomAttr;
		int w = 0, h = 0, x = 0, y = 0;
		HI_U32 u32Width = 0, u32Height = 0;
#if defined(SN8604M) || defined(SN8608M) || defined(SN8608M_LE) || defined(SN8616M_LE)|| defined(SN9234H1)
		u32Width = PRV_BT1120_SIZE_W;
		u32Height = PRV_BT1120_SIZE_H;
#else
		u32Width = PRV_SINGLE_SCREEN_W;
		u32Height = PRV_SINGLE_SCREEN_H;
#endif
		u32Width = s_astVoDevStatDflt[s_VoDevCtrlDflt].stVideoLayerAttr.stImageSize.u32Width;
		u32Height = s_astVoDevStatDflt[s_VoDevCtrlDflt].stVideoLayerAttr.stImageSize.u32Height;
		x = u32Width * pstPoint->x / s_u32GuiWidthDflt; //��ӦD1��Ļ����X
		y = u32Height * pstPoint->y / s_u32GuiHeightDflt; //��ӦD1��Ļ����Y
		w = u32Width * 5/(u32Ratio+4);					//�Ŵ���ο���
		h = u32Height * 5/(u32Ratio+4);					//�Ŵ���ο�߶�
		stVoZoomAttr.stZoomRect.s32X		= (((x + w) > u32Width) ? (u32Width -w) : x) & (~0x01);;	//����xλ�ã�����D1���Ҫ�˲�,2���ض���
		stVoZoomAttr.stZoomRect.s32Y		= (((y + h) > u32Height) ? (u32Height -h) : y) & (~0x01);		//����yλ�ã�����D1�߶�Ҫ�˲�,2���ض���
		stVoZoomAttr.stZoomRect.u32Width	= w & (~0x01);
		stVoZoomAttr.stZoomRect.u32Height	= h & (~0x01);
		stVoZoomAttr.enField = VIDEO_FIELD_FRAME;		
		
		CHECK_RET(HI_MPI_VO_SetZoomInWindow(s_VoDevCtrlDflt, VoChn, &stVoZoomAttr));

	}
	
#else	


	//VoChn = s_astVoDevStatDflt[s_VoDevCtrlDflt].s32CtrlChn;//��ʱ���Բ���VoChn
	int index = 0;
	VoChn = 0;
	//index = PRV_GetVoChnIndex(VoChn);
	index = s_astVoDevStatDflt[s_VoDevCtrlDflt].s32CtrlChn;
	if(index < 0)
		RET_FAILURE("------ERR: Invalid Index!");
	if(VoChn < 0 || VoChn >= PRV_VO_CHN_NUM
		|| u32Ratio < PRV_MIN_ZOOM_IN_RATIO || u32Ratio > PRV_MAX_ZOOM_IN_RATIO)
	{
		RET_FAILURE("Invalid Parameter: u32Ratio or VoChn");
	}
	
	if(NULL == pstPoint)
	{
		RET_FAILURE("NULL Pointer!!");
	}
	
	if (PRV_STAT_CTRL != s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat
		&& PRV_CTRL_ZOOM_IN != s_astVoDevStatDflt[s_VoDevCtrlDflt].enCtrlFlag)
	{
		RET_FAILURE("NOT in [zoom in ctrl] stat!!!");
	}
//	if (VoChn != s_astVoDevStatDflt[s_VoDevCtrlDflt].s32CtrlChn)
	{
//		RET_FAILURE("VoChn NOT match current VoChn!!!");
	}
	//printf("vochn=%d, u32ratio=%f, pstPoint->x= %d, pstPoint->y=%d===========================\n",VoChn,u32Ratio,pstPoint->x, pstPoint->y);

	VoChn = s_astVoDevStatDflt[s_VoDevCtrlDflt].s32CtrlChn;
	VPSS_GRP VpssGrp = VoChn;
    VPSS_CROP_INFO_S stVpssCropInfo;
	HI_S32 w = 0, h = 0, x = 0, y = 0, s32X = 0, s32Y = 0;
	HI_U32 u32Width = 0, u32Height = 0, u32W = 0, u32H = 0;
	u32Width = s_astVoDevStatDflt[s_VoDevCtrlDflt].stVideoLayerAttr.stImageSize.u32Width;
	u32Height = s_astVoDevStatDflt[s_VoDevCtrlDflt].stVideoLayerAttr.stImageSize.u32Height;
	w = u32Width/sqrt(u32Ratio);
	h = u32Height/sqrt(u32Ratio);
    stVpssCropInfo.bEnable = HI_TRUE;

	VO_CHN_ATTR_S stVoChnAttr;	
	CHECK_RET(HI_MPI_VO_GetChnAttr(s_VoDevCtrlDflt, VoChn, &stVoChnAttr));
	s32X = stVoChnAttr.stRect.s32X;
	s32Y = stVoChnAttr.stRect.s32Y;
	u32W = stVoChnAttr.stRect.u32Width;
	u32H = stVoChnAttr.stRect.u32Height;
	TRACE(SCI_TRACE_NORMAL, MOD_PRV, "VoChn: %d, s32X=%d, s32Y=%d, u32W=%d, u32H=%d\n", VoChn, s32X, s32Y, u32W, u32H);
	
	#if 1
	//x = u32Width * pstPoint->x / s_u32GuiWidthDflt; //��ӦD1��Ļ����X
	//y = u32Height * pstPoint->y / s_u32GuiHeightDflt; //��ӦD1��Ļ����Y
	//w = u32Width * 5/(u32Ratio+4);					//�Ŵ���ο���
	//h = u32Height * 5/(u32Ratio+4); 				//�Ŵ���ο�߶�
	//x = pstPoint->x;
	//y = pstPoint->y;
	//w = u32Width/sqrt(u32Ratio);					//�Ŵ���ο���
	//h = u32Height/sqrt(u32Ratio); 				//�Ŵ���ο�߶�
	//����ģʽ�µ��ӷŴ�
	if(u32H == u32Height && u32W != u32Width)
	{
		if(pstPoint->x < s32X)
		{
			if(pstPoint->x + w <= s32X)
			{
				CHECK(PRV_VPSS_Stop(VpssGrp));
				CHECK(PRV_VPSS_Start(VpssGrp));
				return 0;
			}
			else if(pstPoint->x + w <= s32X + u32W)
			{
				x = 0;
				y = pstPoint->y * VochnInfo[index].VideoInfo.height/u32Height;
				w = (w - (s32X - pstPoint->x)) * VochnInfo[index].VideoInfo.width/u32W;
				h = VochnInfo[index].VideoInfo.height/sqrt(u32Ratio);
			}
			else
			{
				x = 0;
				y = pstPoint->y * VochnInfo[index].VideoInfo.height/u32Height;
				w = u32W * VochnInfo[index].VideoInfo.width/u32W;
				h = VochnInfo[index].VideoInfo.height/sqrt(u32Ratio);
			}
		}
		else if(pstPoint->x >= s32X && pstPoint->x <= s32X + u32W)
		{
			if(pstPoint->x + w <= s32X + u32W)
			{
				x = (pstPoint->x - s32X) * VochnInfo[index].VideoInfo.width/u32W;
				y = pstPoint->y * VochnInfo[index].VideoInfo.height/u32Height;
				w = VochnInfo[index].VideoInfo.width * w/u32W;
				h = VochnInfo[index].VideoInfo.height/sqrt(u32Ratio);

			}
			else
			{
				x = (pstPoint->x - s32X) * VochnInfo[index].VideoInfo.width/u32W;
				y = pstPoint->y * VochnInfo[index].VideoInfo.height/u32Height;
				w = (u32W - (pstPoint->x - s32X)) * VochnInfo[index].VideoInfo.width/u32W;
				h = VochnInfo[index].VideoInfo.height/sqrt(u32Ratio);
			}
		}
		else
		{
			CHECK(PRV_VPSS_Stop(VpssGrp));
			CHECK(PRV_VPSS_Start(VpssGrp));
			return 0;
		}
		
	}
	else if(u32W == u32Width && u32H != u32Height)
	{
		if(pstPoint->y < s32Y)
		{
			if(pstPoint->y + h <= s32Y)
			{
				CHECK(PRV_VPSS_Stop(VpssGrp));
				CHECK(PRV_VPSS_Start(VpssGrp));
				return 0;
			}
			else if(pstPoint->y + h <= s32Y + u32H)
			{
				x = pstPoint->x * VochnInfo[index].VideoInfo.width/u32Width;
				y = 0;
				w = VochnInfo[index].VideoInfo.width/sqrt(u32Ratio);
				h = (h - (s32Y - pstPoint->y)) * VochnInfo[index].VideoInfo.height/u32Height;
			}
			else
			{
				x = pstPoint->x * 1000 /u32Width;
				y = 0;
				w = VochnInfo[index].VideoInfo.width/sqrt(u32Ratio);
				h = u32H * VochnInfo[index].VideoInfo.height/u32H;
			}

		}
		else if(pstPoint->y >= s32Y && pstPoint->y <= s32Y + u32H)
		{
			if(pstPoint->y + h <= s32Y + u32H)
			{
				x = pstPoint->x * VochnInfo[index].VideoInfo.width/u32Width;
				y = (pstPoint->y - s32Y) * VochnInfo[index].VideoInfo.height/u32H;
				w = VochnInfo[index].VideoInfo.width * h/u32H;
				h = VochnInfo[index].VideoInfo.height/sqrt(u32Ratio);
			}
			else
			{
				x = pstPoint->x * VochnInfo[index].VideoInfo.width/u32Width;
				y = (pstPoint->y - s32Y) * VochnInfo[index].VideoInfo.height/u32H;
				w = VochnInfo[index].VideoInfo.width/sqrt(u32Ratio);
				h = (u32H - (pstPoint->y - s32Y)) * VochnInfo[index].VideoInfo.height/u32H;
			}
		}
		else
		{
			CHECK(PRV_VPSS_Stop(VpssGrp));
			CHECK(PRV_VPSS_Start(VpssGrp));
			return 0;
		}
	}
	else
	{
		x = pstPoint->x * VochnInfo[index].VideoInfo.width/u32Width;
		y = pstPoint->y * VochnInfo[index].VideoInfo.height/u32Height;
		w = VochnInfo[index].VideoInfo.width/sqrt(u32Ratio);
		h = VochnInfo[index].VideoInfo.height/sqrt(u32Ratio);
	}
	x = ((x + w) > VochnInfo[index].VideoInfo.width) ? (VochnInfo[index].VideoInfo.width - w) : x;
	y = ((y + h) > VochnInfo[index].VideoInfo.height) ? (VochnInfo[index].VideoInfo.height - h) : y;
	//printf("pstPoint->x: %d, pstPoint->y: %d\n", pstPoint->x, pstPoint->y);
	x = ((x + w) > VochnInfo[index].VideoInfo.width) ? (VochnInfo[index].VideoInfo.width - w) : x;
	y = ((y + h) > VochnInfo[index].VideoInfo.height) ? (VochnInfo[index].VideoInfo.height - h) : y;
	w = w >= 32 ? w : 32;
	h = h >= 32 ? h : 32;
	x = ALIGN_BACK(x, 4);//��ʼ��Ϊ4�������������Ϊ16��������
	y = ALIGN_BACK(y, 4);
	w = ALIGN_BACK(w, 16);
	h = ALIGN_BACK(h, 16);
	TRACE(SCI_TRACE_NORMAL, MOD_PRV, "11111111u32Ratio: %u, x: %d, y: %d, w: %d, h: %d===width: %d, height: %d\n", u32Ratio, x, y, w, h, VochnInfo[index].VideoInfo.width, VochnInfo[index].VideoInfo.height);
    stVpssCropInfo.enCropCoordinate = VPSS_CROP_ABS_COOR;
    stVpssCropInfo.stCropRect.s32X = x;
    stVpssCropInfo.stCropRect.s32Y = y;
    stVpssCropInfo.stCropRect.u32Width = w;
    stVpssCropInfo.stCropRect.u32Height = h;
	#else
	#if 0
	if(u32H == u32Height && u32W != u32Width)
	{
		if(pstPoint->x < s32X)
		{
			if(pstPoint->x + w <= s32X)
			{
				CHECK(PRV_VPSS_Stop(VpssGrp));
				CHECK(PRV_VPSS_Start(VpssGrp));
				return 0;
			}
			else if(pstPoint->x + w <= s32X + u32W)
			{
				x = s32X * u32W/u32Width;
				y = pstPoint->y;
				w = w - (s32X - pstPoint->x);
			}
			else
			{
				x = s32X;
				y = pstPoint->y;
				w = u32W;
			}
		}
		else if(pstPoint->x >= s32X && pstPoint->x <= s32X + u32W)
		{
			if(pstPoint->x + w <= s32X + u32W)
			{
				x = (pstPoint->x - s32X);
				y = pstPoint->y;
				w = u32W/sqrt(u32Ratio);
				h = u32H/sqrt(u32Ratio);
			}
			else
			{
				x = (pstPoint->x - s32X);
				y = pstPoint->y;
				w = u32W - (pstPoint->x - s32X);
			}
		}
		else
		{
			CHECK(PRV_VPSS_Stop(VpssGrp));
			CHECK(PRV_VPSS_Start(VpssGrp));
			return 0;
		}
	}
	else if(u32W == u32Width && u32H != u32Height)
	{
		if(pstPoint->y < s32Y)
		{
			if(pstPoint->y + h <= s32Y)
			{
				CHECK(PRV_VPSS_Stop(VpssGrp));
				CHECK(PRV_VPSS_Start(VpssGrp));
				return 0;
			}
			else if(pstPoint->y + h <= s32Y + u32H)
			{
				y = s32Y * 1000/u32Height;
				x = pstPoint->x * 1000 /u32Width;
				h = h - (s32Y - pstPoint->y);
			}
			else
			{
				y = s32Y * 1000/u32Height;
				x = pstPoint->x * 1000 /u32Width;
				h = u32H;
			}

		}
		else if(pstPoint->y >= s32Y && pstPoint->y <= s32Y + u32H)
		{
			if(pstPoint->y + h <= s32Y + u32H)
			{
				x = pstPoint->x * 1000/u32Width;
				y = pstPoint->y * 1000 /u32Height;
				w = u32W/sqrt(u32Ratio);
				h = u32H/sqrt(u32Ratio);
			}
			else
			{
				x = pstPoint->y * 1000/u32Width;
				y = pstPoint->y * 1000 /u32Height;
				h = u32H - (pstPoint->y - s32Y);
			}

		}
		else
		{
			CHECK(PRV_VPSS_Stop(VpssGrp));
			CHECK(PRV_VPSS_Start(VpssGrp));
			return 0;
		}
	}
	else
	#endif
	{
		x = pstPoint->y * 1000 /u32Width;
		y = pstPoint->y * 1000 /u32Height;
		w = u32Width/sqrt(u32Ratio);
		h = u32Height/sqrt(u32Ratio);
	}
	//printf("11111111u32Ratio: %f, x: %d, y: %d, w: %d, h: %d\n", u32Ratio, x, y, w, h);
	
	x = x > 999 ? 999 : x;
	y = y > 999 ? 999 : y;
	x = ALIGN_BACK(x, 4);//��ʼ��Ϊ4�������������Ϊ16��������
	y = ALIGN_BACK(y, 4);
	w = ALIGN_BACK(w, 16);
	h = ALIGN_BACK(h, 16);
    stVpssCropInfo.enCropCoordinate = VPSS_CROP_RITIO_COOR;
    stVpssCropInfo.stCropRect.s32X = x;
    stVpssCropInfo.stCropRect.s32Y = y;
    stVpssCropInfo.stCropRect.u32Width = w;
    stVpssCropInfo.stCropRect.u32Height = h;
	#endif
#if defined(Hi3535)
	CHECK_RET(HI_MPI_VPSS_SetGrpCrop(VpssGrp, &stVpssCropInfo));
	CHECK_RET(HI_MPI_VO_RefreshChn(VHD0, VoChn));
#else
    stVpssCropInfo.enCapSel = VPSS_CAPSEL_BOTH;
    CHECK_RET(HI_MPI_VPSS_SetCropCfg(VpssGrp, &stVpssCropInfo));
    CHECK_RET(HI_MPI_VO_ChnRefresh(0, VoChn));
#endif
#endif	
	RET_SUCCESS("Chn Zoom in!");
}

/*************************************************
Function: //PRV_AlarmChn
Description://��������
Calls: 
Called By: //
Input: // VoDev: ����豸
   		VoChn: ͨ����
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_AlarmChn(VO_DEV VoDev, VO_CHN VoChn)
{
//	HI_U32 u32Index;

#if defined(SN9234H1)
	if(VoDev == SPOT_VO_DEV|| VoDev == AD)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}

	if(VoDev < 0 || VoDev >= PRV_VO_MAX_DEV 
		|| VoChn < 0 || VoChn >= g_Max_Vo_Num)
	{
		RET_FAILURE("Invalid Parameter: VoChn or VoDev");
	}
	
#else

	if (VoDev < 0 || VoDev > DHD0/*VoDev >= PRV_VO_MAX_DEV*/
		|| VoChn < 0 || VoChn >= g_Max_Vo_Num)
	{
		RET_FAILURE("Invalid Parameter: VoChn or VoDev");
	}
	if(DHD1 == VoDev)
		VoDev = DSD0;
#endif	
	if (s_astVoDevStatDflt[VoDev].s32AlarmChn == VoChn
		&& s_astVoDevStatDflt[VoDev].bIsAlarm == HI_TRUE)
	{
		RET_SUCCESS("alarm chn already in display");/*�ظ������ı������治���ٴ�ˢ�£�*/
	}
	//CHECK_RET(PRV_Chn2Index(VoDev, VoChn, &u32Index,s_astVoDevStatDflt[VoDev].as32ChnOrder[s_astVoDevStatDflt[VoDev].enPreviewMode]));/*check if this chn has been hiden*/
	s_astVoDevStatDflt[VoDev].s32AlarmChn = VoChn;
	s_astVoDevStatDflt[VoDev].bIsAlarm = HI_TRUE;
	//s_astVoDevStatDflt[s_VoDevCtrlDflt].bIsDouble = 2;
	//printf("######PRV_AlarmChn   :s_astVoDevStatDflt[VoDev].enPreviewStat = %d###################\n",s_astVoDevStatDflt[VoDev].enPreviewStat);
	/*��Ҫʱ��ˢ���������*/
	
	if (PRV_STAT_NORM == s_astVoDevStatDflt[VoDev].enPreviewStat)
	{
		PRV_RefreshVoDevScreen(VoDev,DISP_DOUBLE_DISP,s_astVoDevStatDflt[VoDev].as32ChnOrder[SingleScene]);
	}
	
	RET_SUCCESS("Alarm Chn!");
}

/*************************************************
Function: //PRV_AlarmOff
Description://��������
Calls: 
Called By: //
Input: // VoDev: ����豸
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_AlarmOff(VO_DEV VoDev)
{
	if (s_astVoDevStatDflt[VoDev].bIsAlarm)
	{
		
		s_astVoDevStatDflt[VoDev].bIsAlarm = HI_FALSE;
		//s_astVoDevStatDflt[s_VoDevCtrlDflt].bIsDouble = 0;
		/*��Ҫʱ��ˢ���������*/
		if (PRV_STAT_NORM == s_astVoDevStatDflt[VoDev].enPreviewStat)
		{
#if defined(SN9234H1)
			PRV_RefreshVoDevScreen(VoDev,DISP_DOUBLE_DISP,s_astVoDevStatDflt[VoDev].as32ChnOrder[s_astVoDevStatDflt[VoDev].enPreviewMode]);
#else
			PRV_RefreshVoDevScreen(VoDev,DISP_NOT_DOUBLE_DISP,s_astVoDevStatDflt[VoDev].as32ChnOrder[s_astVoDevStatDflt[VoDev].enPreviewMode]);
#endif
			/*if(s_astVoDevStatDflt[VoDev].bIsDouble == 1)
			{
				s_astVoDevStatDflt[VoDev].bIsDouble ++;
			}*/
		}
	}
	
	RET_SUCCESS("Alarm Off!");
}

/*************************************************
Function: //PRV_SingleChn
Description://�л���������
Calls: 
Called By: //
Input: // VoDev: ����豸
		VoChn: ͨ����
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_SingleChn(VO_DEV VoDev, VO_CHN VoChn)
{
#if defined(SN9234H1)
	HI_S32 s32Index = 0;

	if(VoDev == SPOT_VO_DEV|| VoDev == AD)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}
	if( VoDev < 0 || VoDev >= PRV_VO_MAX_DEV
		|| VoChn < 0 || VoChn >= g_Max_Vo_Num)
	{
		RET_FAILURE("Invalid Parameter: VoDev or VoChn");
	}
	
#else
	HI_U32 s32Index = 0;
	if (VoDev < 0 || VoDev > DHD0/*VoDev >= PRV_VO_MAX_DEV*/
		|| VoChn < 0 || VoChn >= g_Max_Vo_Num)
	{
		RET_FAILURE("Invalid Parameter: VoDev or VoChn");
	}
	if(DHD1 == VoDev)
		VoDev = DSD0;
#endif	
	if (PRV_STAT_NORM != s_astVoDevStatDflt[VoDev].enPreviewStat)
	{
		if(PRV_CurDecodeMode == PassiveDecode)
		{
			s_astVoDevStatDflt[VoDev].enPreviewMode = SingleScene;
			RET_SUCCESS("");
		}
		RET_FAILURE("NOT in preview stat!!!");
	}	
		
#if (DEV_TYPE != DEV_SN_9234_H_1)
	/*��������*/
	if (HI_FAILURE == PRV_Chn2Index(VoDev, VoChn, &s32Index,s_astVoDevStatDflt[VoDev].as32ChnOrder[SingleScene]))
	{
		CHECK_RET(PRV_Chn2Index(VoDev, -VoChn-1, &s32Index,s_astVoDevStatDflt[VoDev].as32ChnOrder[SingleScene]));/*VoChn����(-1~-16)��ʾ���ص�ͨ��*/
	}
#endif
	s_astVoDevStatDflt[VoDev].enPreviewMode = SingleScene;
	s_astVoDevStatDflt[VoDev].bIsSingle = HI_FALSE;	
	s_astVoDevStatDflt[VoDev].bIsAlarm = HI_FALSE;//�л���ȡ������״̬
	s_astVoDevStatDflt[VoDev].s32SingleIndex = s32Index;
	s_astVoDevStatDflt[VoDev].s32DoubleIndex = 0;
	//printf("---------------s_astVoDevStatDflt[VoDev].s32SingleIndex: %d\n", s_astVoDevStatDflt[VoDev].s32SingleIndex);
	PRV_RefreshVoDevScreen(VoDev, DISP_DOUBLE_DISP, s_astVoDevStatDflt[VoDev].as32ChnOrder[SingleScene]);

	RET_SUCCESS("Set Single Chn!");
}

/*************************************************
Function: //PRV_MultiChn
Description://�л����໭��
Calls: 
Called By: //
Input: // VoDev: ����豸
		enPreviewMode: �໭��Ԥ��ģʽ
		s32Index: �໭�濪ʼ������
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_MultiChn(VO_DEV VoDev, PRV_PREVIEW_MODE_E enPreviewMode, HI_S32 s32Index)
{
#if defined(SN9234H1)
	if(VoDev == SPOT_VO_DEV|| VoDev == AD)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}
	if( VoDev < 0 || VoDev >= PRV_VO_MAX_DEV
		|| s32Index < 0 || s32Index >= SIXINDEX)
	{
		RET_FAILURE("Invalid Parameter: VoDev or s32Index");
	}
	
	LayoutToSingleChn = -1;

	if (SD == VoDev)
	{
	
		//PRV_RefreshVoDevScreen(VoDev, DISP_NOT_DOUBLE_DISP, s_astVoDevStatDflt[VoDev].as32ChnOrder[s_astVoDevStatDflt[VoDev].enPreviewMode]);
		RET_SUCCESS("spot out single chn!");
	}
#else
	if (VoDev < 0 || VoDev > DHD0/*VoDev >= PRV_VO_MAX_DEV*/
		|| s32Index < 0 || s32Index >= SIXINDEX)
	{
		RET_FAILURE("Invalid Parameter: VoDev or s32Index");
	}
	
	LayoutToSingleChn = -1;

	if (DHD1 == VoDev)
	{
		VoDev = DSD0;	
	}
	
#endif	

	if (PRV_STAT_NORM != s_astVoDevStatDflt[VoDev].enPreviewStat)
	{
		//�����������ͨ������ʱ��Ԥ��ģʽ�л���
		//�˳�ͨ�����ƺ󣬻�����ԭ��Ԥ��ģʽ��
		if(PRV_CurDecodeMode == PassiveDecode)
		{
			s_astVoDevStatDflt[VoDev].enPreviewMode = enPreviewMode;
			RET_SUCCESS("");			
		}
			
		RET_FAILURE("NOT in preview state!!!");
	}
	
	s_astVoDevStatDflt[VoDev].enPreviewMode = enPreviewMode;
	s_astVoDevStatDflt[VoDev].s32PreviewIndex = s32Index;
	s_astVoDevStatDflt[VoDev].bIsSingle = HI_FALSE;
	s_astVoDevStatDflt[VoDev].bIsAlarm = HI_FALSE;//�л���ȡ������״̬
	//printf("------s32Index: %d\n", s32Index);
	if(enPreviewMode == LinkFourScene||enPreviewMode==LinkNineScene)
	{
		LinkAgeGroup_ChnState LinkageGroup;
		int i = 0;
		SN_MEMSET(&LinkageGroup,0,sizeof(LinkAgeGroup_ChnState));
		Scm_GetLinkGroup(&LinkageGroup);
		if(enPreviewMode == LinkFourScene)
		{
			for(i = 0; i < 4; i++)
			{
				if(LinkageGroup.DevGroupChn[i].DevChn==0)
				{
					s_astVoDevStatDflt[VoDev].as32ChnOrder[enPreviewMode][i] = -1;
				}
				else
				{
					s_astVoDevStatDflt[VoDev].as32ChnOrder[enPreviewMode][i] = LinkageGroup.DevGroupChn[i].DevChn-1;
				}
			//	printf("enPreviewMode:%d,i:%d,%d\n",enPreviewMode,i,s_astVoDevStatDflt[VoDev].as32ChnOrder[enPreviewMode][i]);
			}
		}
		else
		{
			for(i = 0; i < 9; i++)
			{
				if(i<3)
				{
					s_astVoDevStatDflt[VoDev].as32ChnOrder[enPreviewMode][i] = -1;
					if(LinkageGroup.DevGroupChn[i].DevChn==0)
					{
						s_astVoDevStatDflt[VoDev].as32ChnOrder[enPreviewMode][i+3] = -1;
					}
					else
					{
						s_astVoDevStatDflt[VoDev].as32ChnOrder[enPreviewMode][i+3] = LinkageGroup.DevGroupChn[i].DevChn-1;
					}
				}
				if(i>5)
				{
					s_astVoDevStatDflt[VoDev].as32ChnOrder[enPreviewMode][i] = -1;
				}
				//printf("enPreviewMode:%d,i:%d,%d\n",enPreviewMode,i,s_astVoDevStatDflt[VoDev].as32ChnOrder[enPreviewMode][i]);
			}
		}
	}
	PRV_RefreshVoDevScreen(VoDev, DISP_DOUBLE_DISP, s_astVoDevStatDflt[VoDev].as32ChnOrder[s_astVoDevStatDflt[VoDev].enPreviewMode]);
	
	RET_SUCCESS("Set Multi Chn!");
}

/*************************************************
Function: //PRV_ZoomInPic
Description://���л�,���ӷŴ�״̬ʱר��
Calls: 
Called By: //
Input: // bIsShow:�Ƿ���ʾ
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
***********************************************************************/
STATIC HI_S32 PRV_ZoomInPic(HI_BOOL bIsShow)
{
	VO_DEV VoDev = s_VoDevCtrlDflt;
	VO_CHN VoChn = PRV_CTRL_VOCHN;
#if defined(SN9234H1)	
	VI_DEV ViDev = -1;
	VI_CHN ViChn = -1;
#endif
	VO_CHN_ATTR_S stVoChnAttr;
	int index = 0;
	RECT_S stSrcRect, stDestRect;

#if defined(SN9234H1)

 /*2010-9-19 ˫����*/
//VoDev = HD;
//again:
//��ȡs32CtrlChn��Ӧ��VI
	if(OldCtrlChn >= 0)
	{
		if(OldCtrlChn < (LOCALVEDIONUM < PRV_CHAN_NUM ? LOCALVEDIONUM : PRV_CHAN_NUM))
		{
			if(OldCtrlChn < PRV_VI_CHN_NUM)
			{
				ViDev = PRV_656_DEV_1;
			}
			else
			{
				ViDev = PRV_656_DEV;
			}
			ViChn = OldCtrlChn%PRV_VI_CHN_NUM;
		}
		//�ڴ�Ƭ
		else if(OldCtrlChn >= PRV_CHAN_NUM && OldCtrlChn < LOCALVEDIONUM)
		{
			ViDev = PRV_HD_DEV;
		}
	}
#endif

	if (s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat != PRV_STAT_CTRL
		|| s_astVoDevStatDflt[s_VoDevCtrlDflt].enCtrlFlag != PRV_CTRL_ZOOM_IN)
	{
		RET_FAILURE("Warning!!! not int zoom in stat now!!!");
	}

	//1.���VOͨ��
	if(OldCtrlChn >= 0)
	{	
#if defined(SN9234H1)
		if(ViDev != -1)//ģ����Ƶͨ��
			CHECK(HI_MPI_VI_UnBindOutput(ViDev, ViChn, VoDev, VoChn));
		else
		{
			index = PRV_GetVoChnIndex(OldCtrlChn);
			if(index < 0)
				RET_FAILURE("-----------Invalid Index!");
			if(VochnInfo[index].SlaveId > PRV_MASTER)
				(HI_MPI_VI_UnBindOutput(PRV_HD_DEV, 0, VoDev, VoChn));
			else
			{
				(HI_MPI_VDEC_UnbindOutputChn(VochnInfo[index].VdecChn, VoDev, VoChn));
				VochnInfo[index].IsBindVdec[VoDev] = -1;
			}
			
		}
#else
		index = PRV_GetVoChnIndex(OldCtrlChn);
		if(index < 0)
			RET_FAILURE("-----------Invalid Index!");
#if defined(Hi3535)
		CHECK(HI_MPI_VO_HideChn(VoDev, VoChn));
#else
		CHECK(HI_MPI_VO_ChnHide(VoDev, VoChn));
#endif
 		CHECK(PRV_VDEC_UnBindVpss(VochnInfo[index].VdecChn, VoChn));
		VochnInfo[index].IsBindVdec[VoDev] = -1;
#if defined(Hi3535)
		CHECK(PRV_VO_UnBindVpss(PIP,VoChn,VoChn,VPSS_BSTR_CHN));
#else
 		CHECK(PRV_VO_UnBindVpss(VoDev, VoChn, VoChn, VPSS_PRE0_CHN));
#endif
#endif		
	}
	//2.�ر�VOͨ��
#if defined(Hi3535)
	CHECK(HI_MPI_VO_DisableChn(PIP ,VoChn));
#else	
	CHECK(HI_MPI_VO_DisableChn(VoDev ,VoChn));
#endif

	if (bIsShow)
	{		
#if defined(SN9234H1)
		if(s_astVoDevStatDflt[VoDev].s32CtrlChn < (LOCALVEDIONUM < PRV_CHAN_NUM ? LOCALVEDIONUM : PRV_CHAN_NUM))
		{
			if(s_astVoDevStatDflt[VoDev].s32CtrlChn < PRV_VI_CHN_NUM)
			{
				ViDev = PRV_656_DEV_1;
			}
			else
			{
				ViDev = PRV_656_DEV;
			}
			ViChn = s_astVoDevStatDflt[VoDev].s32CtrlChn %PRV_VI_CHN_NUM;
		}
		else if(s_astVoDevStatDflt[VoDev].s32CtrlChn >= PRV_CHAN_NUM && s_astVoDevStatDflt[VoDev].s32CtrlChn  < LOCALVEDIONUM)
		{
			ViDev = PRV_HD_DEV;
		}

#endif
		//3.����VOͨ��		
		index = PRV_GetVoChnIndex(s_astVoDevStatDflt[VoDev].s32CtrlChn);
		if(index < 0)
			RET_FAILURE("------ERR: Invalid Index!");
#if defined(Hi3535)
		CHECK_RET(HI_MPI_VO_GetChnAttr(PIP, s_astVoDevStatDflt[VoDev].s32CtrlChn, &stVoChnAttr));
#else
		CHECK_RET(HI_MPI_VO_GetChnAttr(VoDev, s_astVoDevStatDflt[VoDev].s32CtrlChn, &stVoChnAttr));
#endif
		stVoChnAttr.stRect.s32X      = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width*3/4;
		stVoChnAttr.stRect.s32Y      = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height*3/4;
		stVoChnAttr.stRect.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height*1/4;
		stVoChnAttr.stRect.u32Width  = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width*1/4;
		stSrcRect.s32X		 = stVoChnAttr.stRect.s32X;
		stSrcRect.s32Y		 = stVoChnAttr.stRect.s32Y;
		stSrcRect.u32Width	 = stVoChnAttr.stRect.u32Width;
		stSrcRect.u32Height  = stVoChnAttr.stRect.u32Height;
		
		stDestRect = PRV_ReSetVoRect(OutPutMode, VochnInfo[index].VideoInfo.width, VochnInfo[index].VideoInfo.height, stSrcRect);
		stVoChnAttr.stRect.s32X 	 = stDestRect.s32X 		& (~0x01);
		stVoChnAttr.stRect.s32Y 	 = stDestRect.s32Y 		& (~0x01);
		stVoChnAttr.stRect.u32Width  = stDestRect.u32Width  & (~0x01);
		stVoChnAttr.stRect.u32Height = stDestRect.u32Height & (~0x01);
		//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "-----------w=%d, h=%d, d_w=%d, d_h=%d, x=%d, y=%d, s_w=%d, s_h=%d\n", s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width, s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height,
		//	s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width, s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height,
		//	stVoChnAttr.stRect.s32X ,stVoChnAttr.stRect.s32Y,stVoChnAttr.stRect.u32Width,stVoChnAttr.stRect.u32Height);
#if defined(Hi3535)
		stVoChnAttr.u32Priority = 1;
		CHECK_RET(HI_MPI_VO_SetChnAttr(PIP, VoChn, &stVoChnAttr));
		CHECK_RET(PRV_VO_BindVpss(PIP,VoChn,VoChn,VPSS_BSTR_CHN));
#elif defined(Hi3531)		
		stVoChnAttr.u32Priority = 1;
		CHECK_RET(HI_MPI_VO_SetChnAttr(VoDev, VoChn, &stVoChnAttr));
		CHECK_RET(PRV_VO_BindVpss(VoDev,VoChn,VoChn,VPSS_PRE0_CHN));
#else
		CHECK_RET(HI_MPI_VO_SetChnAttr(VoDev, VoChn, &stVoChnAttr));
#endif
		//stVoZoomAttr.stZoomRect = stVoChnAttr.stRect;
		//stVoZoomAttr.enField = VIDEO_FIELD_INTERLACED;
		//CHECK_RET(HI_MPI_VO_SetZoomInWindow(VoDev, VoChn, &stVoZoomAttr));

#if defined(SN9234H1)
		//4.��VOͨ��
		if(-1 == ViDev)
		{			
			if(VochnInfo[index].VdecChn >= 0)
			{			
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "-----index: %d---SlaveId: %d, VoDev: %d----Bind---VochnInfo[index].VdecChn: %d, VoChn: %d\n",index, VochnInfo[index].SlaveId, VoDev, VochnInfo[index].VdecChn, VoChn);
				if(VochnInfo[index].SlaveId == PRV_MASTER )
				{
					CHECK_RET(HI_MPI_VDEC_BindOutput(VochnInfo[index].VdecChn, VoDev, VoChn)); 
				}				
				else if(VochnInfo[index].SlaveId > PRV_MASTER)
				{
					ViDev = PRV_HD_DEV;
					ViChn = 0;
				}
			}
		}
		
#if defined(SN_SLAVE_ON)
		if(ViDev == PRV_HD_DEV)
		{			
			VO_ZOOM_ATTR_S stVoZoomAttr;
			HI_U32 u32Width = 0, u32Height = 0;
#if defined(SN8604M) || defined(SN8608M) || defined(SN8608M_LE) || defined(SN8616M_LE)|| defined(SN9234H1)
			//u32Width = s_astVoDevStatDflt[s_VoDevCtrlDflt].stVideoLayerAttr.stImageSize.u32Width;
			//u32Height = s_astVoDevStatDflt[s_VoDevCtrlDflt].stVideoLayerAttr.stImageSize.u32Height;
			u32Width = PRV_BT1120_SIZE_W;
			u32Width = PRV_BT1120_SIZE_H;
#else
			u32Width = PRV_SINGLE_SCREEN_W;
			u32Height = PRV_SINGLE_SCREEN_H;
#endif				
			u32Width = s_astVoDevStatDflt[s_VoDevCtrlDflt].stVideoLayerAttr.stImageSize.u32Width;
			u32Height = s_astVoDevStatDflt[s_VoDevCtrlDflt].stVideoLayerAttr.stImageSize.u32Height;
			stSrcRect.s32X		= 0;
			stSrcRect.s32Y		= 0;
			stSrcRect.u32Width	= u32Width;
			stSrcRect.u32Height = u32Height;
			
			stDestRect = PRV_ReSetVoRect(OutPutMode, VochnInfo[index].VideoInfo.width, VochnInfo[index].VideoInfo.height, stSrcRect);
			
			stVoZoomAttr.stZoomRect.s32X		= stDestRect.s32X	   & (~0x01);
			stVoZoomAttr.stZoomRect.s32Y		= stDestRect.s32Y	   & (~0x01);
			stVoZoomAttr.stZoomRect.u32Width	= stDestRect.u32Width  & (~0x01);
			stVoZoomAttr.stZoomRect.u32Height	= stDestRect.u32Height & (~0x01);

			//stVoZoomAttr.enField = VIDEO_FIELD_INTERLACED;
			stVoZoomAttr.enField = VIDEO_FIELD_FRAME;
#if defined(Hi3535)
			CHECK_RET(HI_MPI_VO_SetZoomInWindow(PIP, VoChn, &stVoZoomAttr));
#else
			CHECK_RET(HI_MPI_VO_SetZoomInWindow(VoDev, VoChn, &stVoZoomAttr));
#endif
		}
#endif
		if(-1 != ViDev)
		{
			//printf("--------Bind ViDev: %d, ViChn: %d\n", ViDev, ViChn);
			CHECK_RET(HI_MPI_VI_BindOutput(ViDev, ViChn, VoDev, VoChn));
		}

#else
		//4.��VOͨ��
		if(VochnInfo[index].VdecChn >= 0)
		{			
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "-----index: %d---SlaveId: %d, VoDev: %d----Bind---VochnInfo[index].VdecChn: %d, VoChn: %d\n",index, VochnInfo[index].SlaveId, VoDev, VochnInfo[index].VdecChn, VoChn);
			if(VochnInfo[index].SlaveId == PRV_MASTER )
			{
				PRV_VPSS_ResetWH(VoChn,VochnInfo[index].VdecChn,VochnInfo[index].VideoInfo.width,VochnInfo[index].VideoInfo.height);
				CHECK_RET(PRV_VDEC_BindVpss(VochnInfo[index].VdecChn, VoChn)); 
			}				
		}		
#endif
		VochnInfo[index].IsBindVdec[VoDev] = 1;
		//5.����VOͨ��
#if defined(Hi3535)
		CHECK_RET(HI_MPI_VO_EnableChn(PIP, VoChn));
#else		
		CHECK_RET(HI_MPI_VO_EnableChn(VoDev, VoChn));
#endif
		OldCtrlChn = s_astVoDevStatDflt[VoDev].s32CtrlChn;
		sem_post(&sem_SendNoVideoPic);		
	}
	else
		OldCtrlChn = -1;
	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_Disp_ParamInPic
Description://���л�,��Ƶ��ʾ��������ʱר��
Calls: 
Called By: //
Input: // bIsShow:�Ƿ���ʾ
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
***********************************************************************/
STATIC HI_S32 PRV_Disp_ParamInPic(HI_BOOL bIsShow)
{
	VO_DEV VoDev = s_VoDevCtrlDflt;
	VO_CHN VoChn = PRV_CTRL_VOCHN;
	VO_CHN_ATTR_S stVoChnAttr;
	int index = 0;
	
#if defined(SN9234H1)
	VI_DEV ViDev = -1;
	VI_CHN ViChn = -1;
	RECT_S stSrcRect, stDestRect;

/*2010-9-19 ˫����*/
//VoDev = HD;
//again:
	if(OldCtrlChn >= 0)
	{
		if(OldCtrlChn < (LOCALVEDIONUM < PRV_CHAN_NUM ? LOCALVEDIONUM : PRV_CHAN_NUM))
		{
			if(OldCtrlChn < PRV_VI_CHN_NUM)
			{
				ViDev = PRV_656_DEV_1;
			}
			else
			{
				ViDev = PRV_656_DEV;
			}
			ViChn = OldCtrlChn%PRV_VI_CHN_NUM;
		}
		//�ڴ�Ƭ
		else if(OldCtrlChn  >= PRV_CHAN_NUM && OldCtrlChn  < LOCALVEDIONUM)
		{
			ViDev = PRV_HD_DEV;
		}
		
	}

#endif

	if (s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat != PRV_STAT_CTRL)
		//|| s_astVoDevStatDflt[s_VoDevCtrlDflt].enCtrlFlag != PRV_CTRL_REGION_SEL)
	{
		RET_FAILURE("Warning!!! not int zoom in stat now!!!");
	}

	//1.�����PRV_CTRL_VOCHN�󶨵�VDEC��VIͨ��
	if(OldCtrlChn >= 0)
	{
#if defined(SN9234H1)
		if(ViDev != -1)//ģ����Ƶͨ��
		{
			CHECK(HI_MPI_VI_UnBindOutput(ViDev, ViChn, VoDev, VoChn));
		}
		else
		{
			index = PRV_GetVoChnIndex(OldCtrlChn);
			if(index < 0)
				RET_FAILURE("------ERR: Invalid Index!");
			if(VochnInfo[index].SlaveId > PRV_MASTER)
			{
				(HI_MPI_VI_UnBindOutput(PRV_HD_DEV, 0, VoDev, VoChn));
			}
			else
			{
				(HI_MPI_VDEC_UnbindOutputChn(VochnInfo[index].VdecChn, VoDev, VoChn));
				VochnInfo[index].IsBindVdec[VoDev] = -1;
			}
		}
#else
		index = PRV_GetVoChnIndex(OldCtrlChn);
		if(index < 0)
			RET_FAILURE("------ERR: Invalid Index!");
		CHECK(PRV_VDEC_UnBindVpss(VochnInfo[index].VdecChn, VoChn));
		VochnInfo[index].IsBindVdec[VoDev] = -1;
#endif		
	}
	
	//2.�ر�VOͨ��	
#if defined(Hi3535)
	CHECK(HI_MPI_VO_HideChn(PIP, VoChn));
	CHECK_RET(PRV_VO_UnBindVpss(PIP,VoChn,VoChn,VPSS_BSTR_CHN));
	CHECK(HI_MPI_VO_DisableChn(PIP ,VoChn));
#elif defined(Hi3531)
	CHECK(HI_MPI_VO_ChnHide(VoDev, VoChn));
	CHECK_RET(PRV_VO_UnBindVpss(VoDev, VoChn, VoChn, VPSS_PRE0_CHN));
	CHECK(HI_MPI_VO_DisableChn(VoDev, VoChn));
#else
	CHECK(HI_MPI_VO_DisableChn(VoDev, VoChn));
#endif

	if (bIsShow)
	{
#if defined(SN9234H1)
//		HI_S32 s32Index;
		//�жϵ�ǰ��ͨ���Ƿ����
		//PRV_Chn2Index(VoDev,s_astVoDevStatDflt[VoDev].s32CtrlChn,&s32Index);
		//3.����VOͨ��
		/*���ֱ����µķ�Χֵ
		800*600:  121\113\328\264
		1024*768: 173\151\256\206
		1280*1024: 210\185\205\154
		1366*768: 220\151\192\206
		1440*900: 227\171\182\176
		*/
		//��ȡ�µ�s32CtrlChn��Ӧ��VI
		if(s_astVoDevStatDflt[VoDev].s32CtrlChn < (LOCALVEDIONUM < PRV_CHAN_NUM ? LOCALVEDIONUM : PRV_CHAN_NUM))
		{
			if(s_astVoDevStatDflt[VoDev].s32CtrlChn < PRV_VI_CHN_NUM)
			{
				ViDev = PRV_656_DEV_1;
			}
			else
			{
				ViDev = PRV_656_DEV;
			}
			ViChn = s_astVoDevStatDflt[VoDev].s32CtrlChn %PRV_VI_CHN_NUM;
		}
		else if(s_astVoDevStatDflt[VoDev].s32CtrlChn >= PRV_CHAN_NUM && s_astVoDevStatDflt[VoDev].s32CtrlChn  < LOCALVEDIONUM)
		{
			ViDev = PRV_HD_DEV;
		}
#endif
		index = PRV_GetVoChnIndex(s_astVoDevStatDflt[VoDev].s32CtrlChn);
		if(index < 0)
			RET_FAILURE("------ERR: Invalid Index!");
//		HI_U32 u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width;
//		HI_U32 u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height;
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "-----------s32X=%d, s32Y=%d, u32Width=%d, u32Height=%d\n",
			s_astVoDevStatDflt[s_VoDevCtrlDflt].Pip_rect.s32X,s_astVoDevStatDflt[s_VoDevCtrlDflt].Pip_rect.s32Y,
			s_astVoDevStatDflt[s_VoDevCtrlDflt].Pip_rect.u32Width, s_astVoDevStatDflt[s_VoDevCtrlDflt].Pip_rect.u32Height);
#if defined(Hi3535)
		CHECK_RET(HI_MPI_VO_GetChnAttr(PIP, s_astVoDevStatDflt[VoDev].s32CtrlChn, &stVoChnAttr));
#else
		CHECK_RET(HI_MPI_VO_GetChnAttr(VoDev, s_astVoDevStatDflt[VoDev].s32CtrlChn, &stVoChnAttr));
#endif
		stVoChnAttr.stRect.s32X      = s_astVoDevStatDflt[s_VoDevCtrlDflt].Pip_rect.s32X * s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width/s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width;
		stVoChnAttr.stRect.s32Y      = s_astVoDevStatDflt[s_VoDevCtrlDflt].Pip_rect.s32Y * s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height/s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height;
		stVoChnAttr.stRect.u32Width  = 3 + s_astVoDevStatDflt[s_VoDevCtrlDflt].Pip_rect.u32Width * s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width/s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width;
		stVoChnAttr.stRect.u32Height = 4 + s_astVoDevStatDflt[s_VoDevCtrlDflt].Pip_rect.u32Height * s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height/s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height;
#if defined(SN9234H1)
		stVoChnAttr.u32Priority = 4;
#else
		stVoChnAttr.u32Priority = 1;
#endif
		if(VochnInfo[index].VdecChn == DetVLoss_VdecChn || VochnInfo[index].VdecChn == NoConfig_VdecChn)
		{
			//printf("VochnInfo[index].VdecChn == DetVLoss_VdecChn || VochnInfo[index].VdecChn == NoConfig_VdecCh\n");
			//stVoChnAttr.stRect.s32X 	 = stVoChnAttr.stRect.s32X + (stVoChnAttr.stRect.u32Width - (NOVIDEO_IMAGWIDTH * stVoChnAttr.stRect.u32Width / u32Width)) / 2;
			//stVoChnAttr.stRect.s32Y 	 = stVoChnAttr.stRect.s32Y + (stVoChnAttr.stRect.u32Height - (NOVIDEO_IMAGHEIGHT * stVoChnAttr.stRect.u32Height/ u32Height))/ 2;
			//stVoChnAttr.stRect.u32Width  = (NOVIDEO_IMAGWIDTH * stVoChnAttr.stRect.u32Width / u32Width);
			//stVoChnAttr.stRect.u32Height = (NOVIDEO_IMAGHEIGHT * stVoChnAttr.stRect.u32Height / u32Height);
			
		}
#if defined(Hi3531)||defined(Hi3535)		
		if(stVoChnAttr.stRect.u32Height < 32)
		{
			stVoChnAttr.stRect.s32Y = stVoChnAttr.stRect.s32Y - (32 - stVoChnAttr.stRect.u32Height)/2;
			stVoChnAttr.stRect.u32Height = 32;
		}
#endif		
		stVoChnAttr.stRect.s32X 	 &= (~0x1);
		stVoChnAttr.stRect.s32Y 	 &= (~0x1);
		stVoChnAttr.stRect.u32Width  &= (~0x1);
		stVoChnAttr.stRect.u32Height &= (~0x1);
		
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "-----------w=%d, h=%d, d_w=%d, d_h=%d, x=%d, y=%d, s_w=%d, s_h=%d\n", s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width, s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height,
			s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width, s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height,
			stVoChnAttr.stRect.s32X ,stVoChnAttr.stRect.s32Y,stVoChnAttr.stRect.u32Width,stVoChnAttr.stRect.u32Height);
#if defined(Hi3535)
		CHECK_RET(HI_MPI_VO_SetChnAttr(PIP, VoChn, &stVoChnAttr));
		CHECK_RET(PRV_VO_BindVpss(PIP, VoChn, VoChn, VPSS_BSTR_CHN));
#elif defined(Hi3531)
		CHECK_RET(HI_MPI_VO_SetChnAttr(VoDev, VoChn, &stVoChnAttr));
		CHECK_RET(PRV_VO_BindVpss(VoDev, VoChn, VoChn, VPSS_PRE0_CHN));
#else
		CHECK_RET(HI_MPI_VO_SetChnAttr(VoDev, VoChn, &stVoChnAttr));
#endif
		//stVoZoomAttr.stZoomRect = stVoChnAttr.stRect;
		//stVoZoomAttr.enField = VIDEO_FIELD_INTERLACED;
		//CHECK_RET(HI_MPI_VO_SetZoomInWindow(VoDev, VoChn, &stVoZoomAttr));
#if defined(SN9234H1)
		//4.��VOͨ��
		if(-1 == ViDev)
		{			
			if(VochnInfo[index].VdecChn >= 0 /*&& VochnInfo[index].IsBindVdec[VoDev] == -1*/)
			{				
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "-----index: %d---SlaveId: %d, VoDev: %d----Bind---VochnInfo[index].VdecChn: %d, VoChn: %d\n",index, VochnInfo[index].SlaveId, VoDev, VochnInfo[index].VdecChn, VoChn);
				if(VochnInfo[index].SlaveId == PRV_MASTER )
				{
					CHECK_RET(HI_MPI_VDEC_BindOutput(VochnInfo[index].VdecChn, VoDev, VoChn));
					if(VochnInfo[index].VdecChn == DetVLoss_VdecChn || VochnInfo[index].VdecChn == NoConfig_VdecChn )						
						sem_post(&sem_SendNoVideoPic);				
				}				
				else if(VochnInfo[index].SlaveId > PRV_MASTER)
				{
					ViDev = PRV_HD_DEV;
					ViChn = 0;
				}
			}
		}
		
#if defined(SN_SLAVE_ON)
		if(ViDev == PRV_HD_DEV)
		{			
			VO_ZOOM_ATTR_S stVoZoomAttr;
			int w = 0, h = 0;

			w = PRV_BT1120_SIZE_W;
			h = PRV_BT1120_SIZE_H;			
			w = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width;
			h = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height;

			stSrcRect.s32X		= 0;
			stSrcRect.s32Y		= 0;
			stSrcRect.u32Width	= w;
			stSrcRect.u32Height = h;
			
			stDestRect = PRV_ReSetVoRect(OutPutMode, VochnInfo[index].VideoInfo.width, VochnInfo[index].VideoInfo.height, stSrcRect);
			
			stVoZoomAttr.stZoomRect.s32X		= stDestRect.s32X	   & (~0x01);
			stVoZoomAttr.stZoomRect.s32Y		= stDestRect.s32Y	   & (~0x01);
			stVoZoomAttr.stZoomRect.u32Width	= stDestRect.u32Width  & (~0x01);
			stVoZoomAttr.stZoomRect.u32Height	= stDestRect.u32Height & (~0x01);

			stVoZoomAttr.enField = VIDEO_FIELD_FRAME;
			CHECK_RET(HI_MPI_VO_SetZoomInWindow(VoDev, VoChn, &stVoZoomAttr));
		}
#endif
		if(-1 != ViDev)
		{
			//printf("--------Bind ViDev: %d, ViChn: %d\n", ViDev, ViChn);
			CHECK_RET(HI_MPI_VI_BindOutput(ViDev, ViChn, VoDev, VoChn));
		}
#else
		//4.��VOͨ��
		if(VochnInfo[index].VdecChn >= 0 /*&& VochnInfo[index].IsBindVdec[VoDev] == -1*/)
		{				
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "-----index: %d---SlaveId: %d, VoDev: %d----Bind---VochnInfo[index].VdecChn: %d, VoChn: %d\n",index, VochnInfo[index].SlaveId, VoDev, VochnInfo[index].VdecChn, VoChn);
			if(VochnInfo[index].SlaveId == PRV_MASTER )
			{	
				if(VochnInfo[index].VdecChn == DetVLoss_VdecChn || VochnInfo[index].VdecChn == NoConfig_VdecChn )		
				{
#if defined(Hi3535)
					CHECK(HI_MPI_VO_HideChn(VoDev, VochnInfo[index].VoChn));
#else
					CHECK(HI_MPI_VO_ChnHide(VoDev, VochnInfo[index].VoChn));
#endif
					PRV_VDEC_UnBindVpss(VochnInfo[index].VdecChn, VochnInfo[index].VoChn);
					VochnInfo[index].IsBindVdec[VoDev] = -1;
				}				
				PRV_VPSS_ResetWH(VoChn,VochnInfo[index].VdecChn,VochnInfo[index].VideoInfo.width,VochnInfo[index].VideoInfo.height);
				CHECK_RET(PRV_VDEC_BindVpss(VochnInfo[index].VdecChn, VoChn));
				VochnInfo[index].IsBindVdec[VoDev] = 0;
				if(VochnInfo[index].VdecChn == DetVLoss_VdecChn || VochnInfo[index].VdecChn == NoConfig_VdecChn )						
					sem_post(&sem_SendNoVideoPic);				
			}				
		}
#endif		
		//5.����VOͨ��
#if defined(Hi3535)
		CHECK_RET(HI_MPI_VO_EnableChn(PIP, VoChn));
#else
		CHECK_RET(HI_MPI_VO_EnableChn(VoDev, VoChn));
#endif
		
		OldCtrlChn = s_astVoDevStatDflt[VoDev].s32CtrlChn;
		
	}
	else		
		OldCtrlChn = -1;
	
	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_EnterChnCtrl
Description://����ͨ������״̬
Calls: 
Called By: //
Input: // VoDev: ����豸
		VoChn: ͨ����
		s32Flag: ͨ��״̬
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_EnterChnCtrl(VO_DEV VoDev, VO_CHN VoChn, HI_S32 s32Flag)
{
	unsigned int is_double = DISP_DOUBLE_DISP;
#if defined(SN9234H1)
	if(VoDev == SPOT_VO_DEV|| VoDev == AD)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}
	if( VoDev < 0 || VoDev >= PRV_VO_MAX_DEV
		|| VoChn < 0 || VoChn >= g_Max_Vo_Num)
#else
	if(VoDev > DHD0)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}
	if (VoDev < 0 || VoDev > DHD0/*VoDev >= PRV_VO_MAX_DEV*/
		|| VoChn < 0 || VoChn >= g_Max_Vo_Num)
#endif		
	{
		RET_FAILURE("Invalid Parameter: VoDev or VoChn");
	}
	//printf("------------s_astVoDevStatDflt[VoDev].enPreviewStat: %d\n", s_astVoDevStatDflt[VoDev].enPreviewStat);
	if (PRV_STAT_NORM != s_astVoDevStatDflt[VoDev].enPreviewStat)
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV, "WARNING: NOT in preview stat currently: %d!!,operation continues.", s_astVoDevStatDflt[VoDev].enPreviewStat);
		//RET_FAILURE("NOT in preview stat!!!");
	}
	
	//printf("######PRV_EnterChnCtrl s32Flag = %d  ,s_astVoDevStatDflt[VoDev].enPreviewStat = %d,vochn:%d###################\n",s32Flag,s_astVoDevStatDflt[VoDev].enPreviewStat,VoChn);
	//sem_post(&sem_SendNoVideoPic);

	switch (s32Flag)
	{
		case 8://1-��������ѡ��,����OSD,OSDλ��ѡ�����OSDλ��ѡ��
 /*2010-9-2 ��������������ѡ��ʱ�ر�Ԥ��OSD*/
			if(s_State_Info.bIsOsd_Init && s_State_Info.bIsRe_Init)
			{
#if defined(SN9234H1)
				//Prv_OSD_Show(VoDev, HI_FALSE);
				Prv_OSD_Show(HD, HI_FALSE);
				Prv_OSD_Show(AD, HI_FALSE);
#else
				//Prv_OSD_Show(VoDev, HI_FALSE);
				Prv_OSD_Show(DHD0, HI_FALSE);
				//Prv_OSD_Show(DSD0, HI_FALSE);
#endif
			}

		case 1://1-��������ѡ����Ƶ�������õ�
			s_astVoDevStatDflt[VoDev].enCtrlFlag = PRV_CTRL_REGION_SEL;
			s_astVoDevStatDflt[VoDev].enPreviewStat = PRV_STAT_CTRL;
			s_astVoDevStatDflt[VoDev].s32CtrlChn = VoChn;
			//is_double = DISP_NOT_DOUBLE_DISP;
			break;
		case 2://2-���ӷŴ�
			s_astVoDevStatDflt[VoDev].enCtrlFlag = PRV_CTRL_ZOOM_IN;
			s_astVoDevStatDflt[VoDev].enPreviewStat = PRV_STAT_CTRL;
			s_astVoDevStatDflt[VoDev].s32CtrlChn = VoChn;
			//is_double = DISP_NOT_DOUBLE_DISP;
			break;
		case 3://3-��̨����
			s_astVoDevStatDflt[VoDev].enCtrlFlag = PRV_CTRL_PTZ;
			s_astVoDevStatDflt[VoDev].enPreviewStat = PRV_STAT_CTRL;
			s_astVoDevStatDflt[VoDev].s32CtrlChn = VoChn;
			break;
		case 4://4-�л����ط�
		case SLC_CTL_FLAG:
			s_astVoDevStatDflt[VoDev].enCtrlFlag = PRV_CTRL_PB;
			s_astVoDevStatDflt[VoDev].enPreviewStat = PRV_STAT_PB;
			
			//is_double = DISP_NOT_DOUBLE_DISP;
			break;
		case PIC_CTL_FLAG:	
			s_astVoDevStatDflt[VoDev].enCtrlFlag = PRV_CTRL_PB;
			s_astVoDevStatDflt[VoDev].enPreviewStat = PRV_STAT_PIC;
			//is_double = DISP_NOT_DOUBLE_DISP;
			break;	
		case 7:	//�ڸ�
#if 0 /*2010-9-2 ��������������ѡ��ʱ�ر�Ԥ��OSD*/
			if(s_State_Info.bIsOsd_Init)
			{
				Prv_OSD_Show(VoDev, HI_FALSE);
			}
#endif
			//�����ڸ�ʱ����Ҫ�ر��ڸ�����
			//OSD_Mask_disp_Ctl(VoChn,0);
			IsDispInPic = 0;
			PRV_Disp_ParamInPic(HI_FALSE);//�رջ��л�
			//s_astVoDevStatDflt[VoDev].enCtrlFlag = PRV_CTRL_REGION_SEL;
			//s_astVoDevStatDflt[VoDev].enPreviewStat = PRV_STAT_CTRL;
			//s_astVoDevStatDflt[VoDev].s32CtrlChn = VoChn;
			//is_double = DISP_NOT_DOUBLE_DISP;
			//break;
			RET_SUCCESS("Enter Chn Ctrl!");
		case 9://������Ƶ��ʾ�������ý���
			s_astVoDevStatDflt[VoDev].enCtrlFlag = PRV_CTRL_REGION_SEL;
			s_astVoDevStatDflt[VoDev].enPreviewStat = PRV_STAT_CTRL;
			s_astVoDevStatDflt[VoDev].s32CtrlChn = VoChn;
			IsDispInPic = 1;
#if defined(SN9234H1)			
			sem_post(&sem_SendNoVideoPic);
#endif			
			if(s_State_Info.bIsOsd_Init && s_State_Info.bIsRe_Init)
			{
				if(s_VoDevCtrlDflt == s_VoSecondDev)
				{
#if defined(SN9234H1)					
					Prv_OSD_Show(AD, HI_FALSE);
#endif					
				}
				else
				{
					Prv_OSD_Show(s_VoDevCtrlDflt, HI_FALSE);
				}
			}
			//9-������Ƶ�������ý��棬��ʾ���л�
			//PRV_Disp_ParamInPic(HI_TRUE);
			//is_double = DISP_NOT_DOUBLE_DISP;
			break;
			//RET_SUCCESS("Enter Chn Ctrl!");
		default:
			RET_FAILURE("Invalid CTRL FLAG!!!");
	}
	//printf("111111111######PRV_EnterChnCtrl s32Flag = %d  ,s_astVoDevStatDflt[VoDev].enPreviewStat = %d###################\n",s32Flag,s_astVoDevStatDflt[VoDev].enPreviewStat);
	PRV_RefreshVoDevScreen(VoDev, is_double, s_astVoDevStatDflt[VoDev].as32ChnOrder[s_astVoDevStatDflt[VoDev].enPreviewMode]);

	if (s32Flag == 2)//2-������ӷŴ���ʾ���л�
	{
		PRV_ZoomInPic(HI_TRUE);
	}
	else if(s32Flag == 9)
	{
		//9-������Ƶ�������ý��棬��ʾ���л�
		PRV_Disp_ParamInPic(HI_TRUE);
	}
	
	RET_SUCCESS("Enter Chn Ctrl!");
}

/*************************************************
Function: //PRV_ExitChnCtrl
Description://�˳�ͨ������״̬
Calls: 
Called By: //
Input: // s32Flag: ͨ��״̬
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_ExitChnCtrl(HI_S32 s32Flag)
{
	PRV_CTRL_FLAG_E enCtrlFlag=0;
	unsigned int is_double=DISP_DOUBLE_DISP;

	if (PRV_STAT_CTRL != s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat
		&& PRV_STAT_PB != s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat
		&& PRV_STAT_PIC	!= s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat)
	{
		RET_FAILURE("NOT in ctrl or pb stat or pic stat!!!");
	}
	//printf("######s32Flag = %d###################\n",s32Flag);
	switch (s32Flag)
	{
		case 8://1-����ѡ��,��Ԥ��OSD
#if 1 /*2010-9-2 �������˳�����ѡ��ʱ��Ԥ��OSD*/
			if(s_State_Info.bIsOsd_Init && s_State_Info.bIsRe_Init)
			{
#if defined(SN9234H1)
				//Prv_OSD_Show(s_VoDevCtrlDflt, HI_TRUE);
				Prv_OSD_Show(HD, HI_TRUE);
				Prv_OSD_Show(AD, HI_TRUE);
#else
				//Prv_OSD_Show(s_VoDevCtrlDflt, HI_TRUE);
				Prv_OSD_Show(DHD0, HI_TRUE);
				//Prv_OSD_Show(DSD0, HI_TRUE);
#endif				
			}
#endif
		case 1://1-����ѡ��
			enCtrlFlag = PRV_CTRL_REGION_SEL;
			is_double = DISP_NOT_DOUBLE_DISP;
			break;
		case 2://2-���ӷŴ�
			{
#if defined(SN9234H1)
				VO_ZOOM_RATIO_S stZoomRatio = {0};
#if defined(SN6108) || defined(SN8608D) || defined(SN8608M) || defined(SN6104) || defined(SN8604D) || defined(SN8604M)
				HI_MPI_VO_SetZoomInRatio(s_VoDevCtrlDflt, s_astVoDevStatDflt[s_VoDevCtrlDflt].s32CtrlChn, &stZoomRatio);
#else
				HI_MPI_VO_SetZoomInRatio(HD, s_astVoDevStatDflt[s_VoDevCtrlDflt].s32CtrlChn, &stZoomRatio);
				//HI_MPI_VO_SetZoomInRatio(AD, s_astVoDevStatDflt[s_VoDevCtrlDflt].s32CtrlChn, &stZoomRatio);
#endif
				s_State_Info.g_zoom_first_in = HI_FALSE;
#else
				int index = s_astVoDevStatDflt[s_VoDevCtrlDflt].s32CtrlChn;
				VPSS_GRP VpssGrp = VochnInfo[index].VoChn;
				//����VPSS GROP����������ӷŴ�ʱ���õ�����
				
#if defined(Hi3535)
				CHECK(HI_MPI_VO_HideChn(s_VoDevCtrlDflt, VochnInfo[index].VoChn));
				CHECK(PRV_VDEC_UnBindVpss(VochnInfo[index].VdecChn, VpssGrp));
				CHECK(PRV_VO_UnBindVpss(s_VoDevCtrlDflt, VochnInfo[index].VoChn, VpssGrp, VPSS_BSTR_CHN));
#else
				CHECK(HI_MPI_VO_ChnHide(s_VoDevCtrlDflt, VochnInfo[index].VoChn));
				CHECK(PRV_VDEC_UnBindVpss(VochnInfo[index].VdecChn, VpssGrp));
				CHECK(PRV_VO_UnBindVpss(s_VoDevCtrlDflt, VochnInfo[index].VoChn, VpssGrp, VPSS_PRE0_CHN));
#endif
				CHECK(PRV_VPSS_Stop(VpssGrp));
				CHECK(PRV_VPSS_Start(VpssGrp));
#if defined(Hi3535)
				CHECK(PRV_VO_BindVpss(s_VoDevCtrlDflt, VochnInfo[index].VoChn, VpssGrp, VPSS_BSTR_CHN));
#else
				CHECK(PRV_VO_BindVpss(s_VoDevCtrlDflt, VochnInfo[index].VoChn, VpssGrp, VPSS_PRE0_CHN));
#endif
				PRV_VPSS_ResetWH(VpssGrp,VochnInfo[index].VdecChn,VochnInfo[index].VideoInfo.width,VochnInfo[index].VideoInfo.height);
				CHECK(PRV_VDEC_BindVpss(VochnInfo[index].VdecChn, VpssGrp));
				s_State_Info.g_zoom_first_in = HI_FALSE;
#endif				
			}
			enCtrlFlag = PRV_CTRL_ZOOM_IN;
			break;
		case 3://3-��̨����
			enCtrlFlag = PRV_CTRL_PTZ;
			break;
		case 4://4-�л����ط�
		case SLC_CTL_FLAG:
		case PIC_CTL_FLAG:
			enCtrlFlag = PRV_CTRL_PB;
			//is_double = DISP_NOT_DOUBLE_DISP;
			break;
		case 7:
#if 0 /*2010-9-2 �������˳�����ѡ��ʱ��Ԥ��OSD*/
			if(s_State_Info.bIsOsd_Init)
			{
				Prv_OSD_Show(s_VoDevCtrlDflt, HI_TRUE);
			}
#endif
			//�˳��ڸ�ʱ����Ҫ�����ڸ�����
			//OSD_Mask_disp_Ctl(s_astVoDevStatDflt[s_VoDevCtrlDflt].s32CtrlChn,1);
			PRV_Disp_ParamInPic(HI_TRUE);	//�������л�
			//enCtrlFlag = PRV_CTRL_REGION_SEL;
			//is_double = DISP_NOT_DOUBLE_DISP;
			//break;
			RET_SUCCESS("Enter Chn Ctrl!");
		case 9://9-������Ƶ��ʾ�������ý���
			enCtrlFlag = PRV_CTRL_REGION_SEL;
			if(s_State_Info.bIsOsd_Init && s_State_Info.bIsRe_Init)
			{
				if(s_VoDevCtrlDflt == s_VoSecondDev)
				{
#if defined(SN9234H1)					
					Prv_OSD_Show(AD, HI_TRUE);
#endif					
				}
				else
				{
					Prv_OSD_Show(s_VoDevCtrlDflt, HI_TRUE);
				}
			}
			IsDispInPic = 0;
			PRV_Disp_ParamInPic(HI_FALSE);
			//s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat = PRV_STAT_NORM;
			//is_double = DISP_NOT_DOUBLE_DISP;
			break;
			//RET_SUCCESS("Exit Chn Ctrl!");
		default:
			RET_FAILURE("Invalid CTRL FLAG!!!");
	}

	if (s_astVoDevStatDflt[s_VoDevCtrlDflt].enCtrlFlag != enCtrlFlag)
	{
		RET_FAILURE("Ctrl Flag NOT match!");
	}
	if (s32Flag == 2)//2-�˳����ӷŴ�ȡ�����л�
	{
		PRV_ZoomInPic(HI_FALSE);
		//is_double = DISP_NOT_DOUBLE_DISP;
	}
	s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat = PRV_STAT_NORM;
#if defined(Hi3535)
	int i = 0;
	for(i=0;i<PRV_VO_CHN_NUM;i++)
	{
		PRV_VO_UnBindVpss(DHD0,i,i,VPSS_BSTR_CHN);
	}
#endif

	PRV_RefreshVoDevScreen(s_VoDevCtrlDflt,is_double,s_astVoDevStatDflt[s_VoDevCtrlDflt].as32ChnOrder[s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewMode]);
	//if(s32Flag == 7)
	//{//������ڸ������ܣ���ô״̬���ǿ���״̬�������л��޷��˳�
	//	s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat = PRV_STAT_CTRL;
	//}
	RET_SUCCESS("Enter Chn Ctrl!");

}

/*************************************************
Function: //PRV_DoubleClick
Description://���˫������״̬�л�
Calls: 
Called By: //
Input: // vochn: ͨ����
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_DoubleClick(const Preview_Point *pstPoint)
{
	HI_U32 u32Index;
	unsigned char mode =s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewMode;
	if (NULL == pstPoint)
	{
		RET_FAILURE("NULL pointer");
	}

	if (PRV_STAT_NORM != s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat)
	{
		RET_FAILURE("NOT in preview stat!!!");
	}
	LayoutToSingleChn = -1;
#if 0
	if (s_astVoDevStatDflt[s_VoDevCtrlDflt].bIsAlarm)
	{
		s_astVoDevStatDflt[s_VoDevCtrlDflt].bIsAlarm = HI_FALSE;
	}
	else if (s_astVoDevStatDflt[s_VoDevCtrlDflt].bIsSingle)
	{
		s_astVoDevStatDflt[s_VoDevCtrlDflt].bIsSingle = HI_FALSE;
	}
	else
	{
		CHECK_RET(PRV_Point2Index(pstPoint, &u32Index));
		s_astVoDevStatDflt[s_VoDevCtrlDflt].s32SingleIndex = u32Index;
		s_astVoDevStatDflt[s_VoDevCtrlDflt].bIsSingle = HI_TRUE;
	}
#else 
	/* 2010-8-23 �޸�Ԥ�����õĲ������˻ص�Ԥ�����浼�����˫��ʧЧ�� 
	ǰ��������
	����Ԥ�����ò˵����޸�ͨ���ͻ���˳�򣬣������������������ã��˻ص�Ԥ�����档˫��ͨ��1��
	BUG������
	��ʱʱ�������ֺ�ɫ�ף�˫���󣬲���ص��໭����ʾģʽ��
	������Ϣ��
	��Ԥ����������Ԥ��ģʽΪ�����棬����ʱ˫��������ͬ�����������⡣  */
	
	if (s_astVoDevStatDflt[s_VoDevCtrlDflt].bIsAlarm)
	{
		//RET_FAILURE("In Alarm stat!!!");
		s_astVoDevStatDflt[s_VoDevCtrlDflt].bIsAlarm = HI_FALSE;
		//������Ԥ����������ʱ��˫���ص�������
		if (SingleScene == mode)
		{
			s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewMode = PRV_VO_MAX_MOD;
			mode = PRV_VO_MAX_MOD;
		}
		s_astVoDevStatDflt[s_VoDevCtrlDflt].s32DoubleIndex = 0;
	}
	else if (s_astVoDevStatDflt[s_VoDevCtrlDflt].bIsSingle)
	{
		s_astVoDevStatDflt[s_VoDevCtrlDflt].bIsSingle = HI_FALSE;
#if(IS_DECODER_DEVTYPE == 0)
		//������Ԥ��ʱ��˫����Ч
		if (SingleScene == mode)
		{
			s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewMode = PRV_VO_MAX_MOD;
			s_astVoDevStatDflt[s_VoDevCtrlDflt].s32PreviewIndex = 0;
			mode = PRV_VO_MAX_MOD;
		}
#endif
		
		s_astVoDevStatDflt[s_VoDevCtrlDflt].s32DoubleIndex = 0;
		DoubleToSingleIndex = -1;
	}
	else if (SingleScene == mode)
	{
	//�������е�����˫����Ч
#if(IS_DECODER_DEVTYPE == 0)
		s_astVoDevStatDflt[s_VoDevCtrlDflt].s32PreviewIndex = 0;
		s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewMode = PRV_VO_MAX_MOD;
		mode = PRV_VO_MAX_MOD;
#endif
		s_astVoDevStatDflt[s_VoDevCtrlDflt].s32DoubleIndex = 0;
	}
	else
	{
		CHECK_RET(PRV_Point2Index(pstPoint, &u32Index, s_astVoDevStatDflt[s_VoDevCtrlDflt].as32ChnOrder[mode]));
		s_astVoDevStatDflt[s_VoDevCtrlDflt].s32SingleIndex = u32Index;
		s_astVoDevStatDflt[s_VoDevCtrlDflt].bIsSingle = HI_TRUE;
		s_astVoDevStatDflt[s_VoDevCtrlDflt].s32DoubleIndex = 1;
		DoubleToSingleIndex = u32Index;
		VochnInfo[u32Index].bIsDouble = 0;

		g_ChnPlayStateInfo stPlayStateInfo;
		g_PlayInfo	stPlayInfo;
		PRV_GetPlayInfo(&stPlayInfo);
		PRV_GetVoChnPlayStateInfo(stPlayInfo.InstantPbChn, &stPlayStateInfo);
		
		if(stPlayInfo.PlayBackState == PLAY_INSTANT && stPlayStateInfo.CurPlayState == DEC_STATE_NORMALPAUSE)
		{
#if defined(Hi3535)
			HI_MPI_VO_ResumeChn(0, stPlayInfo.InstantPbChn);
#else
			HI_MPI_VO_ChnResume(0, stPlayInfo.InstantPbChn);
#endif
			if(Achn == stPlayInfo.InstantPbChn)
			{
#if defined(SN9234H1)
				HI_MPI_AO_ResumeChn(0, AOCHN);
#else
				HI_MPI_AO_ResumeChn(4, AOCHN);
#endif
			}
			sem_post(&sem_VoPtsQuery);
		}
		
	}
#endif
	PRV_RefreshVoDevScreen(s_VoDevCtrlDflt, DISP_DOUBLE_DISP, s_astVoDevStatDflt[s_VoDevCtrlDflt].as32ChnOrder[mode]);
	PRV_PlayAudio(s_VoDevCtrlDflt);
	RET_SUCCESS("Double Click");
}

/*************************************************
Function: //PRV_GetVoPrvMode
Description://��ȡָ������豸�ϵ�Ԥ��ģʽ��
Calls: 
Called By: //
Input: // VoDev: �豸��
Output: // pePreviewMode:Ԥ��ģʽ
Return: //�ɹ�����HI_SUCCESS
		ʧ�ܷ���HI_FAILURE
Others: // ����˵��
************************************************************************/
HI_S32 PRV_GetVoPrvMode(VO_DEV VoDev, PRV_PREVIEW_MODE_E *pePreviewMode)
{
	if ( NULL == pePreviewMode)
	{
		RET_FAILURE("NULL Poniter parameter!");
	}
#if defined(SN9234H1)
	if(VoDev == SPOT_VO_DEV|| VoDev == AD)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}

	if (VoDev<0 || VoDev>=PRV_VO_MAX_DEV)
	{
		RET_FAILURE("bad parameter: VoDev!!!");
	}
#else
	if(VoDev > DHD0)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}
	if (VoDev < 0 || VoDev > DHD0/*VoDev >= PRV_VO_MAX_DEV*/)
	{
		RET_FAILURE("bad parameter: VoDev!!!");
	}
#endif
	switch(s_astVoDevStatDflt[VoDev].enPreviewStat)
	{
		case PRV_STAT_NORM:
			{
				if (s_astVoDevStatDflt[VoDev].bIsAlarm || s_astVoDevStatDflt[VoDev].bIsSingle)
				{
					*pePreviewMode = SingleScene;
				}
				else
				{
					*pePreviewMode = s_astVoDevStatDflt[VoDev].enPreviewMode;
				}
			}
			break;
		case PRV_STAT_CTRL:
			*pePreviewMode = SingleScene;
			break;
		case PRV_STAT_PB:
			RET_FAILURE("current in playback mode now!");
		case PRV_STAT_PIC:
			break;
		default :
			RET_FAILURE("I'm in PRV_STAT_???... (��ϡ�)~@ ..");
	}
	RET_SUCCESS("");
}


int PRV_GetPlaybackState(UINT8 *PlaybackState)
{
	g_ChnPlayStateInfo stPlayStateInfo;
	g_PlayInfo	stPlayInfo;
	PRV_GetPlayInfo(&stPlayInfo);

	PRV_GetVoChnPlayStateInfo(0, &stPlayStateInfo);

	//printf("333333333333stPlayStateInfo.CurPlayState=%d\n", stPlayStateInfo.CurPlayState);
	//printf("444444444444stPlayInfo.PlayBackState=%d\n", stPlayInfo.PlayBackState);

	if(stPlayInfo.PlayBackState >= PLAY_INSTANT)
	{
		switch(stPlayStateInfo.CurPlayState)
		{
			case DEC_STATE_NORMAL:
				*PlaybackState = PLAYBACK_STATE_NORMAL;
				break;
			case DEC_STATE_NORMALPAUSE:
				*PlaybackState = PLAYBACK_STATE_NORMALPAUSE;
				break;
			case DEC_STATE_STOP:
				*PlaybackState = PLAYBACK_STATE_STOP;
				break;
			case DEC_STATE_EXIT:
				*PlaybackState = PLAYBACK_STATE_STOP;
				break;
			default:
				*PlaybackState = PLAYBACK_STATE_STOP;
				break;
		}
		
	}
	else
	{
		*PlaybackState = PLAYBACK_STATE_EXIT;
	}

	return OK;
}

int PRV_GetPrvStat()
{
	return s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat;
}

/*************************************************
Function: //PRV_GetPrvMode
Description:// ��ȡ��ǰGUI���������豸��Ԥ��ģʽ��
Calls: 
Called By: //
Input: //
Output: // pePreviewMode:Ԥ��ģʽ
Return: //�ɹ�����HI_SUCCESS
		ʧ�ܷ���HI_FAILURE
Others: // ����˵��
************************************************************************/
int PRV_GetPrvMode(enum PreviewMode_enum *pePreviewMode)
{
	return PRV_GetVoPrvMode(s_VoDevCtrlDflt, pePreviewMode);
}

void PRV_GetPrvMode_EX(enum PreviewMode_enum *pePreviewMode)
{
	*pePreviewMode = s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewMode;
}

/************************************************************************/
/* ��ȡVO�豸���뻭�����ʾ��С��λ�á�
                                                                     */
/************************************************************************/
HI_S32 PRV_GetVoDspRect(VO_DEV VoDev, PRV_RECT_S *pstDspRect)
{
	VO_DEV vodevtmp = VoDev;
	//printf("11111111VoDev = %d,PRV_VO_DEV_NUM=%d1111111111111111111\n",VoDev,PRV_VO_DEV_NUM);
	if (NULL == pstDspRect)
	{
		RET_FAILURE("NULL pointer!!!!");
	}
#if defined(SN9234H1)	
	if(VoDev == SPOT_VO_DEV|| VoDev == AD)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}
	if (VoDev<0 || VoDev>=PRV_VO_MAX_DEV)
	{
		RET_FAILURE("bad parameter: VoDev!!!");
	}
	vodevtmp = (VoDev == HD)?HD:s_VoSecondDev;
#else
	if(VoDev > DHD0)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}
	if (VoDev < 0 || VoDev > DHD0/*VoDev >= PRV_VO_MAX_DEV*/)
	{
		RET_FAILURE("bad parameter: VoDev!!!");
	}
	vodevtmp = (VoDev == DHD0) ? DHD0 : s_VoSecondDev;
#endif	
	pstDspRect->s32X = s_astVoDevStatDflt[vodevtmp].stVideoLayerAttr.stDispRect.s32X;
	pstDspRect->s32Y = s_astVoDevStatDflt[vodevtmp].stVideoLayerAttr.stDispRect.s32Y;
	pstDspRect->u32Height = s_astVoDevStatDflt[vodevtmp].stVideoLayerAttr.stDispRect.u32Height;
	pstDspRect->u32Width = s_astVoDevStatDflt[vodevtmp].stVideoLayerAttr.stDispRect.u32Width;
	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_GetVoDevImgSize
Description://  ��ȡVOͨ���Ĵ�Сλ�ã���ֵ�������VO�豸��iMAGE��С����
			ĩ����ʾ״̬��VOͨ������HI_FAILURE��
Calls: 
Called By: //
Input: //VoDev:�豸��
		VoChn:ͨ����
		pstRect:���淵�ص�ͨ��λ��
Output: // pstSize: ����ͼ���С
Return: //�ɹ�����HI_SUCCESS
		ʧ�ܷ���HI_FAILURE
Others: // ����˵��
************************************************************************/

HI_S32 PRV_GetVoChnRect(VO_DEV VoDev, VO_CHN VoChn, RECT_S *pstRect)
{
//	int chn, index, i, chnnum;
//	int num[16] = {1,2,4,6,8,9,16};
	VO_CHN_ATTR_S stVoChnAttr;
	HI_S32 index = 0;
//	HI_U32 Max_num;
//	PRV_PREVIEW_MODE_E mode = s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewMode;
	if (NULL == pstRect)
	{
		RET_FAILURE("NULL pointer!");
	}
#if defined(SN9234H1)
	if(VoDev == SPOT_VO_DEV|| VoDev == AD)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}
	if (VoChn<0 || VoChn>=g_Max_Vo_Num || VoDev<0 || VoDev>=PRV_VO_MAX_DEV)
	{
		RET_FAILURE("bad parameter: VoDev or VoChn");
	}
#else
	if(VoDev > DHD0)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}
	if (VoChn<0 || VoChn>=g_Max_Vo_Num ||VoDev < 0 || VoDev > DHD0/*VoDev >= PRV_VO_MAX_DEV*/)
	{
		RET_FAILURE("bad parameter: VoDev or VoChn");
	}
#endif

	if(s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_PB
		|| s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_PIC)
	{
		RET_FAILURE("current in pb or pic stat!");
	}
	
	index = PRV_GetVoChnIndex(VoChn);
	if(index < 0)
		RET_FAILURE("------ERR: Invalid Index!");

#if defined(SN9234H1)
	if(s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_NORM
		&& DEV_SPOT_NUM > 0 && SPOT_VO_DEV == VoDev)
	{
		VO_VIDEO_LAYER_ATTR_S stLayerAttr;
		CHECK_RET(HI_MPI_VO_GetVideoLayerAttr(VoDev, &stLayerAttr));
		pstRect->u32Height = stLayerAttr.stImageSize.u32Height;
		pstRect->u32Width = stLayerAttr.stImageSize.u32Width;
		pstRect->s32X = 0;
		pstRect->s32Y = 0;
		RET_SUCCESS("");

	}
#endif
	
	#if 0
	if(VochnInfo[index].VdecChn != DetVLoss_VdecChn && VochnInfo[index].VdecChn != NoConfig_VdecChn)
	{
		switch (s_astVoDevStatDflt[VoDev].enPreviewStat)
		{
			case PRV_STAT_NORM:
				{
	//#ifdef SN_SLAVE_ON
	                if (DEV_SPOT_NUM > 0 && SPOT_VO_DEV == VoDev) /*SPOT: PCIV !!!*/
	                {
	                    VO_VIDEO_LAYER_ATTR_S stLayerAttr;
	                    CHECK_RET(HI_MPI_VO_GetVideoLayerAttr(VoDev, &stLayerAttr));
	                    pstRect->u32Height = stLayerAttr.stImageSize.u32Height;
	                    pstRect->u32Width = stLayerAttr.stImageSize.u32Width;
	                    pstRect->s32X = 0;
	                    pstRect->s32Y = 0;
	                    RET_SUCCESS("");
	                }
	//#endif                
					if (s_astVoDevStatDflt[VoDev].bIsAlarm && VoChn == s_astVoDevStatDflt[VoDev].s32AlarmChn)
					{
						if(VochnInfo[index].VdecChn >= 0)
						{
							CHECK_RET(HI_MPI_VO_GetChnAttr(VoDev, VoChn, &stVoChnAttr));
							*pstRect = stVoChnAttr.stRect;
							RET_SUCCESS("");
						}
					}
					else if(s_astVoDevStatDflt[VoDev].bIsSingle)
					{
						if(mode !=SingleScene && s_astVoDevStatDflt[VoDev].s32DoubleIndex == 0 )
							mode = SingleScene;
						if(PRV_CurDecodeMode == PassiveDecode || VoChn == s_astVoDevStatDflt[VoDev].as32ChnOrder[mode][s_astVoDevStatDflt[VoDev].s32SingleIndex])
						{
							CHECK_RET(HI_MPI_VO_GetChnAttr(VoDev, VoChn, &stVoChnAttr));
							*pstRect = stVoChnAttr.stRect;
							RET_SUCCESS("");
						}
					}
					
					else
					{					
						CHECK_RET(HI_MPI_VO_GetChnAttr(VoDev, VoChn, &stVoChnAttr));
						*pstRect = stVoChnAttr.stRect;
						RET_SUCCESS("");
					
					}
					
				}
				break;
			case PRV_STAT_CTRL:
				if (VoChn == s_astVoDevStatDflt[VoDev].s32CtrlChn)
				{			
					CHECK_RET(HI_MPI_VO_GetChnAttr(VoDev, VoChn, &stVoChnAttr));
					*pstRect = stVoChnAttr.stRect;
					RET_SUCCESS("");
				}
				else
				{
					RET_FAILURE("not the ctrl chn!");
				}
				break;
			case PRV_STAT_PB:
				RET_FAILURE("current in pb stat!");
				break;
			case PRV_STAT_PIC:
				RET_FAILURE("current in pic stat!");
				break;
			default:
				RET_FAILURE("unknown preview stat!");
		}
	}
	//�������õ�"��������Ƶ"��"δ����"ͼƬ����ʾ����Ϊָ�������һ���֣�������Ҫ
	//��ȡָ�������С�����Ƿ�ͼƬ�������С��
	else
	#endif
	{
		HI_S32  i = 0, u32Index = 0;	
		HI_U32 u32ChnNum = 0, u32Width = 0, u32Height = 0;
		RECT_S *pstLayout = NULL;
		PRV_PREVIEW_MODE_E	enPreviewMode;
		HI_U32 Max_num;
		enPreviewMode = s_astVoDevStatDflt[VoDev].enPreviewMode;	
#if defined(SN9234H1)
		u32Index = s_astVoDevStatDflt[s_VoDevCtrlDflt].bIsSingle ? s_astVoDevStatDflt[s_VoDevCtrlDflt].s32SingleIndex : s_astVoDevStatDflt[HD].s32PreviewIndex;
#else
		u32Index = s_astVoDevStatDflt[s_VoDevCtrlDflt].bIsSingle ? s_astVoDevStatDflt[s_VoDevCtrlDflt].s32SingleIndex : s_astVoDevStatDflt[DHD0].s32PreviewIndex;
#endif
		//��ȡ��ǰԤ��ģʽ�����ͨ����
		CHECK_RET(PRV_Get_Max_chnnum(s_astVoDevStatDflt[VoDev].enPreviewMode, &Max_num));
		
		if((s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_CTRL && VoChn == s_astVoDevStatDflt[VoDev].s32CtrlChn)
			|| (s_astVoDevStatDflt[VoDev].bIsAlarm && VoChn == s_astVoDevStatDflt[VoDev].s32AlarmChn))
		{
			u32ChnNum = 1;
			pstLayout = s_astPreviewLayout1;
			i = 0;
		}
		else if(s_astVoDevStatDflt[VoDev].bIsSingle )
		{
			if(enPreviewMode != SingleScene && s_astVoDevStatDflt[VoDev].s32DoubleIndex == 0)
				enPreviewMode = SingleScene;
			//printf("enPreviewMode: %d, VoChn: %d, s_astVoDevStatDflt[VoDev].as32ChnOrder[enPreviewMode][s_astVoDevStatDflt[VoDev].s32SingleIndex]: %d\n", enPreviewMode, VoChn, s_astVoDevStatDflt[VoDev].as32ChnOrder[enPreviewMode][s_astVoDevStatDflt[VoDev].s32SingleIndex]);
			if(VoChn == s_astVoDevStatDflt[VoDev].as32ChnOrder[enPreviewMode][s_astVoDevStatDflt[VoDev].s32SingleIndex])
			{
				u32ChnNum = 1;
				pstLayout = s_astPreviewLayout1;
				i = 0;
			}		
		}
		else if(LayoutToSingleChn != -1)
		{
			u32ChnNum = 1;
			pstLayout = s_astPreviewLayout1;
			i = 0;
		}
		else if(s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_NORM
				|| pstLayout == NULL)
		{
			switch(enPreviewMode)
			{
				case SingleScene:
					u32ChnNum = 1;
					pstLayout = s_astPreviewLayout1;
					break;
				case TwoScene:
					u32ChnNum = 2;
					pstLayout = s_astPreviewLayout2;
					break;
				case ThreeScene:
					u32ChnNum = 3;
					pstLayout = s_astPreviewLayout3;
					break;
				case FourScene:
				case LinkFourScene:
					u32ChnNum = 4;
					pstLayout = s_astPreviewLayout4;
					break;
				case FiveScene:
					u32ChnNum = 5;
					pstLayout = s_astPreviewLayout5;
					break;
				case SixScene:
					u32ChnNum = 6;
					pstLayout = s_astPreviewLayout6;
					break;
				case SevenScene:
					u32ChnNum = 7;
					pstLayout = s_astPreviewLayout7;
					break;
				case EightScene:
					u32ChnNum = 8;
					pstLayout = s_astPreviewLayout8;
					break;
				case NineScene:
				case LinkNineScene:
					u32ChnNum = 9;
					pstLayout = s_astPreviewLayout9;
					break;
				case SixteenScene:
					u32ChnNum = 16;
					pstLayout = s_astPreviewLayout16;
					break;
				default:
					RET_FAILURE("Invalid Parameter: enPreviewMode");
			}
			if(PRV_CurDecodeMode == PassiveDecode || ScmGetListCtlState() == 1)
			{
				i = VoChn;
			}
			else
			{
				for(i = 0; i < u32ChnNum && u32Index+i < Max_num; i++)
				{
					if(VoChn == s_astVoDevStatDflt[VoDev].as32ChnOrder[enPreviewMode][(i + u32Index)%Max_num])
						break;
				}
			}
		
		}
		u32Width  = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width;
		u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height;
		if((enPreviewMode == NineScene || enPreviewMode == LinkNineScene || enPreviewMode == ThreeScene || enPreviewMode == FiveScene
		|| enPreviewMode == SevenScene) && s_astVoDevStatDflt[VoDev].bIsAlarm!=1 && s_astVoDevStatDflt[VoDev].bIsSingle!=1)
		{
		//���9����Ԥ��ʱ�������л���֮����ڷ�϶������
			while(u32Width%6 != 0)
				u32Width++;
			while(u32Height%6 != 0)
				u32Height++;
		}
		if(pstLayout != NULL && i < u32ChnNum)
		{
			stVoChnAttr.stRect.s32X 	 = (u32Width  * pstLayout[i].s32X) / PRV_PREVIEW_LAYOUT_DIV;
			stVoChnAttr.stRect.s32Y 	 = (u32Height * pstLayout[i].s32Y) / PRV_PREVIEW_LAYOUT_DIV;
			stVoChnAttr.stRect.u32Width  = (u32Width  * pstLayout[i].u32Width) / PRV_PREVIEW_LAYOUT_DIV;
			stVoChnAttr.stRect.u32Height = (u32Height * pstLayout[i].u32Height) / PRV_PREVIEW_LAYOUT_DIV;
			if(enPreviewMode == NineScene || enPreviewMode == LinkNineScene)
			{ 
				if((i + 1) % 3 == 0)//���һ��
					stVoChnAttr.stRect.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width - stVoChnAttr.stRect.s32X;
				if(i > 5 && i < 9)//���һ��
					stVoChnAttr.stRect.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height - stVoChnAttr.stRect.s32Y;
			}
		   if( enPreviewMode == ThreeScene )
		   { 
			   if( i == 2)//���һ��
				   stVoChnAttr.stRect.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width - stVoChnAttr.stRect.s32X;
			//���һ��
			       stVoChnAttr.stRect.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height - stVoChnAttr.stRect.s32Y;
		   }
		   if( enPreviewMode == FiveScene )
		  { 
			 if( i > 1 )//���һ��
				   stVoChnAttr.stRect.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width - stVoChnAttr.stRect.s32X;
			 if(i==0 || i==1 || i==4)//���һ��
				   stVoChnAttr.stRect.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height - stVoChnAttr.stRect.s32Y;
		  }
		  if(enPreviewMode == SevenScene)
		  { 
			if(i==2 || i==4 || i==6)//���һ��
				stVoChnAttr.stRect.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width - stVoChnAttr.stRect.s32X;
			if(i==0 || i==5 || i==6)//���һ��
				stVoChnAttr.stRect.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height - stVoChnAttr.stRect.s32Y;
		 }
			stVoChnAttr.stRect.s32X 	 &= (~0x01);
			stVoChnAttr.stRect.s32Y 	 &= (~0x01);
			stVoChnAttr.stRect.u32Width  &= (~0x01);
			stVoChnAttr.stRect.u32Height &= (~0x01);
		}
		else
		{
			CHECK_RET(HI_MPI_VO_GetChnAttr(VoDev, VoChn, &stVoChnAttr));
		}
		
		*pstRect = stVoChnAttr.stRect;
		RET_SUCCESS("");
	}
	RET_FAILURE("????");
}
/*************************************************
Function: //PRV_GetVoChnRect_Forxy
Description://  ��ȡVOͨ���Ĵ�Сλ�ã���ֵ�������VO�豸����ʾ��С����
			ĩ����ʾ״̬��VOͨ������HI_FAILURE��
Calls: 
Called By: //
Input: //VoDev:�豸��
		VoChn:ͨ����
		pstRect:���淵�ص�ͨ��λ��
Output: // pstSize: ����ͼ���С
Return: //�ɹ�����HI_SUCCESS
		ʧ�ܷ���HI_FAILURE
Others: // ����˵��
************************************************************************/
static HI_S32 PRV_GetVoChnRect_Forxy(VO_DEV VoDev, VO_CHN VoChn, RECT_S *pstRect)
{
	VO_CHN_ATTR_S stVoChnAttr;
	HI_S32 index = 0;
	if (NULL == pstRect)
	{
		RET_FAILURE("NULL pointer!");
	}
#if defined(SN9234H1)
	if(VoDev == SPOT_VO_DEV|| VoDev == AD)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}
	if (VoChn<0 || VoChn>=PRV_VO_CHN_NUM || VoDev<0 || VoDev>=PRV_VO_MAX_DEV)
	{
		RET_FAILURE("bad parameter: VoDev or VoChn");
	}
#else
	if(VoDev != DHD0)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}
	if (VoChn<0 || VoChn>=PRV_VO_CHN_NUM || VoDev < 0 || VoDev > DHD0/*VoDev >= PRV_VO_MAX_DEV*/)
	{
		RET_FAILURE("bad parameter: VoDev or VoChn");
	}
#endif	
	index = PRV_GetVoChnIndex(VoChn);
	if(index < 0)
		RET_FAILURE("------ERR: Invalid Index!");
	#if 0
	if(VochnInfo[index].VdecChn != DetVLoss_VdecChn && VochnInfo[index].VdecChn != NoConfig_VdecChn)
	{
		CHECK_RET(HI_MPI_VO_GetChnAttr(VoDev, VoChn, &stVoChnAttr));
		*pstRect = stVoChnAttr.stRect;
	}
	else
	#endif
	{
		HI_S32  i = 0, u32Index = 0;	
		HI_U32 u32ChnNum = 0, u32Width = 0, u32Height = 0;
		RECT_S *pstLayout = NULL;
		PRV_PREVIEW_MODE_E	enPreviewMode;
		HI_U32 Max_num;
		enPreviewMode = s_astVoDevStatDflt[VoDev].enPreviewMode;	
#if defined(SN9234H1)
		u32Index = s_astVoDevStatDflt[s_VoDevCtrlDflt].bIsSingle ? s_astVoDevStatDflt[s_VoDevCtrlDflt].s32SingleIndex : s_astVoDevStatDflt[HD].s32PreviewIndex;
#else
		u32Index = s_astVoDevStatDflt[s_VoDevCtrlDflt].bIsSingle ? s_astVoDevStatDflt[s_VoDevCtrlDflt].s32SingleIndex : s_astVoDevStatDflt[DHD0].s32PreviewIndex;
#endif
		//��ȡ��ǰԤ��ģʽ�����ͨ����
		CHECK_RET(PRV_Get_Max_chnnum(s_astVoDevStatDflt[VoDev].enPreviewMode, &Max_num));
		
		if((s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_CTRL && VoChn == s_astVoDevStatDflt[VoDev].s32CtrlChn)
			|| (s_astVoDevStatDflt[VoDev].bIsAlarm && VoChn == s_astVoDevStatDflt[VoDev].s32AlarmChn))
		{
			u32ChnNum = 1;
			pstLayout = s_astPreviewLayout1;
			i = 0;
		}

		else if(s_astVoDevStatDflt[VoDev].bIsSingle)
		{
			if((enPreviewMode != SingleScene) && s_astVoDevStatDflt[VoDev].s32DoubleIndex == 0 )
				enPreviewMode = SingleScene;
			if(VoChn == s_astVoDevStatDflt[VoDev].as32ChnOrder[enPreviewMode][s_astVoDevStatDflt[VoDev].s32SingleIndex])
			{
				u32ChnNum = 1;
				pstLayout = s_astPreviewLayout1;
				i = 0;				
			}
			if(PRV_CurDecodeMode == PassiveDecode || ScmGetListCtlState() == 1)
			{
				u32ChnNum = 1;
				pstLayout = s_astPreviewLayout1;
				i = 0;
			}
		}
		else if(LayoutToSingleChn == VoChn)
		{
			u32ChnNum = 1;
			pstLayout = s_astPreviewLayout1;
			i = 0;

		}
		else if(s_astVoDevStatDflt[VoDev].enPreviewStat == PRV_STAT_NORM
			|| pstLayout == NULL)
		{
			switch(enPreviewMode)
			{
				case SingleScene:
					u32ChnNum = 1;
					pstLayout = s_astPreviewLayout1;
					break;
				case TwoScene:
					u32ChnNum = 2;
					pstLayout = s_astPreviewLayout2;
					break;
				case ThreeScene:
					u32ChnNum = 3;
					pstLayout = s_astPreviewLayout3;
					break;
				case FourScene:
				case LinkFourScene:
					u32ChnNum = 4;
					pstLayout = s_astPreviewLayout4;
					break;
				case FiveScene:
					u32ChnNum = 5;
					pstLayout = s_astPreviewLayout5;
					break;
				case SixScene:
					u32ChnNum = 6;
					pstLayout = s_astPreviewLayout6;
					break;
				case SevenScene:
					u32ChnNum = 7;
					pstLayout = s_astPreviewLayout7;
					break;
				case EightScene:
					u32ChnNum = 8;
					pstLayout = s_astPreviewLayout8;
					break;
				case NineScene:
				case LinkNineScene:
					u32ChnNum = 9;
					pstLayout = s_astPreviewLayout9;
					break;
				case SixteenScene:
					u32ChnNum = 16;
					pstLayout = s_astPreviewLayout16;
					break;
				default:
					RET_FAILURE("Invalid Parameter: enPreviewMode");
			}
			for(i = 0; i < u32ChnNum && u32Index+i < Max_num; i++)
			{
				if(VoChn == s_astVoDevStatDflt[VoDev].as32ChnOrder[enPreviewMode][(i + u32Index)%Max_num])
					break;
			}
			if(PRV_CurDecodeMode == PassiveDecode || ScmGetListCtlState() == 1)
			{
				i = VoChn;
			}

		}
		u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width;
		u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height;
		if((enPreviewMode == NineScene || enPreviewMode == LinkNineScene || enPreviewMode == ThreeScene || enPreviewMode == FiveScene
		|| enPreviewMode == SevenScene) && s_astVoDevStatDflt[VoDev].bIsAlarm!=1 && s_astVoDevStatDflt[VoDev].bIsSingle!=1)
		{
		//���9����Ԥ��ʱ�������л���֮����ڷ�϶������
			while(u32Width%6 != 0)
				u32Width++;
			while(u32Height%6 != 0)
				u32Height++;
		}
		if(pstLayout != NULL && i < u32ChnNum)
		{
			stVoChnAttr.stRect.s32X 	 = (u32Width * pstLayout[i].s32X) / PRV_PREVIEW_LAYOUT_DIV;
			stVoChnAttr.stRect.s32Y 	 = (u32Height * pstLayout[i].s32Y) / PRV_PREVIEW_LAYOUT_DIV;
			stVoChnAttr.stRect.u32Width  = (u32Width * pstLayout[i].u32Width) / PRV_PREVIEW_LAYOUT_DIV;
			stVoChnAttr.stRect.u32Height = (u32Height * pstLayout[i].u32Height) / PRV_PREVIEW_LAYOUT_DIV;
			if(enPreviewMode == NineScene || enPreviewMode == LinkNineScene)
			{ 
				if((i + 1) % 3 == 0)//���һ��
					stVoChnAttr.stRect.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width - stVoChnAttr.stRect.s32X;
				if(i > 5 && i < 9)//���һ��
					stVoChnAttr.stRect.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height - stVoChnAttr.stRect.s32Y;
			}
			
			if( enPreviewMode == ThreeScene )
			{ 
			    if( i == 2)//���һ��
				     stVoChnAttr.stRect.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width - stVoChnAttr.stRect.s32X;
						//���һ��
					 stVoChnAttr.stRect.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height - stVoChnAttr.stRect.s32Y;
			}
			if( enPreviewMode == FiveScene )
			{ 
			    if( i > 1 )//���һ��
				      stVoChnAttr.stRect.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width - stVoChnAttr.stRect.s32X;
				if(i==0 || i==1 || i==4)//���һ��
				      stVoChnAttr.stRect.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height - stVoChnAttr.stRect.s32Y;
			}
			if(enPreviewMode == SevenScene)
			{ 
			    if(i==2 || i==4 || i==6)//���һ��
				      stVoChnAttr.stRect.u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width - stVoChnAttr.stRect.s32X;
				if(i==0 || i==5 || i==6)//���һ��
				      stVoChnAttr.stRect.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height - stVoChnAttr.stRect.s32Y;
			}
			stVoChnAttr.stRect.s32X &= (~0x01);
			stVoChnAttr.stRect.s32Y &= (~0x01);
			stVoChnAttr.stRect.u32Width &= (~0x01);
			stVoChnAttr.stRect.u32Height &= (~0x01);
		}
		else
		{
			CHECK_RET(HI_MPI_VO_GetChnAttr(VoDev, VoChn, &stVoChnAttr));
		}
		*pstRect = stVoChnAttr.stRect;

	}
	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_GetVoDevDispSize
Description://  ��ȡVO�豸��ʾ��С��
Calls: 
Called By: //
Input: //VoDev:�豸��
Output: // pstSize: ������ʾ��С
Return: //�ɹ�����HI_SUCCESS
		ʧ�ܷ���HI_FAILURE
Others: // ����˵��
************************************************************************/
HI_S32 PRV_GetVoDevDispSize(VO_DEV VoDev, SIZE_S *pstSize)
{
	if (NULL == pstSize)
	{
		RET_FAILURE("NULL pointer!");
	}
#if defined(SN9234H1)
	if(VoDev == SPOT_VO_DEV|| VoDev == AD)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}
	if (VoDev<0 || VoDev>=PRV_VO_MAX_DEV)
	{
		RET_FAILURE("bad parameter: VoDev");
	}
#else
	if(VoDev > DHD0)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}
	if (VoDev < 0 || VoDev > DHD0/*VoDev >= PRV_VO_MAX_DEV*/)
	{
		RET_FAILURE("bad parameter: VoDev");
	}
#endif

	pstSize->u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height;
	pstSize->u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width;
	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_GetVoDevImgSize
Description://  ��ȡVO�豸ͼ���С��
Calls: 
Called By: //
Input: //VoDev:�豸��
Output: // pstSize: ����ͼ���С
Return: //�ɹ�����HI_SUCCESS
		ʧ�ܷ���HI_FAILURE
Others: // ����˵��
************************************************************************/
HI_S32 PRV_GetVoDevImgSize(VO_DEV VoDev, SIZE_S *pstSize)
{
	if (NULL == pstSize)
	{
		RET_FAILURE("NULL pointer!");
	}
#if defined(SN9234H1)
	if(VoDev == SPOT_VO_DEV|| VoDev == AD)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}
	if (VoDev<0 || VoDev>=PRV_VO_MAX_DEV)
	{
		RET_FAILURE("bad parameter: VoDev");
	}
#else
	if(VoDev > DHD0)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}
	if (VoDev < 0 || VoDev > DHD0/*VoDev >= PRV_VO_MAX_DEV*/)
	{
		RET_FAILURE("bad parameter: VoDev");
	}
#endif
	pstSize->u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height;
	pstSize->u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width;
	RET_SUCCESS("");
}
/*************************************************
Function: //PRV_GetVoChnDispRect
Description://  ��ȡVOͨ���Ĵ�Сλ�ã���ֵ�������VO�豸�ķֱ��ʴ�С����
			ĩ����ʾ״̬��VOͨ������HI_FAILURE��
Calls: 
Called By: //
Input: //VoDev:�豸��
		VoChn:ͨ����
		pstRect:���淵�ص�ͨ��λ��
Output: // pstSize: ����ͼ���С
Return: //�ɹ�����HI_SUCCESS
		ʧ�ܷ���HI_FAILURE
Others: // ����˵��
************************************************************************/

HI_S32 PRV_GetVoChnDispRect(VO_DEV VoDev, VO_CHN VoChn, RECT_S *pstRect)
{
	RECT_S rect;
	SIZE_S disp_size,img_size;
	
	CHECK_RET(PRV_GetVoChnRect(VoDev,VoChn,&rect));
	CHECK_RET(PRV_GetVoDevDispSize(VoDev,&disp_size));
	CHECK_RET(PRV_GetVoDevImgSize(VoDev,&img_size));
	
	pstRect->s32X = rect.s32X*disp_size.u32Width/img_size.u32Width;
	pstRect->s32Y = rect.s32Y*disp_size.u32Height/img_size.u32Height;
	pstRect->u32Height= rect.u32Height*disp_size.u32Height/img_size.u32Height;
	pstRect->u32Width= rect.u32Width*disp_size.u32Width/img_size.u32Width;
	
	RET_SUCCESS("");
}
/*************************************************
Function: //PRV_GetVoChnDispRect
Description://  ��ȡVOͨ���Ĵ�Сλ�ã���ֵ�������VO�豸�ķֱ��ʴ�С����
			ĩ����ʾ״̬��VOͨ������HI_FAILURE��
Calls: 
Called By: //
Input: //VoDev:�豸��
		VoChn:ͨ����
		pstRect:���淵�ص�ͨ��λ��
Output: // pstSize: ����ͼ���С
Return: //�ɹ�����HI_SUCCESS
		ʧ�ܷ���HI_FAILURE
Others: // ����˵��
************************************************************************/

HI_S32 PRV_GetVoChnDispRect_Forxy(VO_DEV VoDev, VO_CHN VoChn, RECT_S *pstRect)
{
	RECT_S rect;
	SIZE_S disp_size,img_size;	
	
	CHECK_RET(PRV_GetVoChnRect_Forxy(VoDev,VoChn,&rect));
	CHECK_RET(PRV_GetVoDevDispSize(VoDev,&disp_size));
	CHECK_RET(PRV_GetVoDevImgSize(VoDev,&img_size));
	
	pstRect->s32X = rect.s32X*disp_size.u32Width/img_size.u32Width;
	pstRect->s32Y = rect.s32Y*disp_size.u32Height/img_size.u32Height;
	pstRect->u32Height= rect.u32Height*disp_size.u32Height/img_size.u32Height;
	pstRect->u32Width= rect.u32Width*disp_size.u32Width/img_size.u32Width;
	RET_SUCCESS("");
}

/************************************************************************/
/* ����طš�
                                                                     */
/************************************************************************/
STATIC HI_S32 PRV_EnterPB(VO_DEV dev, HI_S32 s32Flag)
{
	int flag = 0,ret = 0;
	int i = 0;
	//AUDIO_DEV AoDev = 0;
	//AO_CHN AoChn = 0;
#if defined(SN9234H1)
	if(dev == SPOT_VO_DEV|| dev == AD)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}
	if (dev < 0 || dev >= PRV_VO_MAX_DEV)
	{
		RET_FAILURE("invalid dev!!");
	}
#else
	if(dev > DHD0)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}
	if (dev < 0 || dev > DHD0/*VoDev >= PRV_VO_MAX_DEV*/)
	{
		RET_FAILURE("invalid dev!!");
	}
#endif
	if (PRV_STAT_NORM != s_astVoDevStatDflt[dev].enPreviewStat)
	{
		RET_FAILURE("current stat already in ctrl or pb !!");
	}

	ret = PRV_TkOrPb(&flag);
	if(ret == HI_FAILURE)
	{
		RET_FAILURE("PONIT null!!");
	}
	if(s32Flag == PIC_CTL_FLAG)
	{
		s_astVoDevStatDflt[dev].enCtrlFlag = PRV_CTRL_PB;
		s_astVoDevStatDflt[dev].enPreviewStat = PRV_STAT_PIC;
	}
	else
	{
		if(flag != PRV_STATE)
		{//�����ǰ���ǿ���״̬����ô����ʧ��
			RET_FAILURE("current stat already in pb or voa stat!!");
		}
		s_astVoDevStatDflt[dev].enCtrlFlag = PRV_CTRL_PB;
		s_astVoDevStatDflt[dev].enPreviewStat = PRV_STAT_PB;

		PRV_TkPbSwitch(1);
		//PRV_DisableAudioPreview();		
		//HI_MPI_AO_UnBindAdec(AoDev, AoChn, DecAdec);
	}
	//PRV_VdecUnBindAllVoChn1(dev);


	for(i = LOCALVEDIONUM; i < DEV_CHANNEL_NUM; i++)
	{
#if defined(SN9234H1)
		if(VochnInfo[i].SlaveId == 0 && VochnInfo[i].VdecChn >= 0 && VochnInfo[i].VdecChn != DetVLoss_VdecChn)
		{
			if(HI_SUCCESS == PRV_WaitDestroyVdecChn(VochnInfo[i].VdecChn))	
			{
				PRV_VoChnStateInit(VochnInfo[i].CurChnIndex);
				PRV_PtsInfoInit(VochnInfo[i].CurChnIndex);		
			}
			PRV_InitVochnInfo(i);	
		}		
#else
		if(VochnInfo[i].SlaveId == 0 && VochnInfo[i].VdecChn >= 0
			&& VochnInfo[i].VdecChn != DetVLoss_VdecChn && VochnInfo[i].VdecChn != NoConfig_VdecChn)
		{
			if(HI_SUCCESS == PRV_WaitDestroyVdecChn(VochnInfo[i].VdecChn))	
			{
				VochnInfo[i].IsHaveVdec = 0;	
			}
			PRV_InitVochnInfo(i);
		}
#endif		
		BufferSet(i + PRV_VIDEOBUFFER, MAX_ARRAY_NODE); 			
		BufferSet(i + PRV_AUDIOBUFFER, MAX_ARRAY_NODE);
		PRV_VoChnStateInit(i);
		
	}
#if defined(Hi3531)||defined(Hi3535)	
	if(PRV_CurIndex > 0)
	{
		for(i = 0; i < PRV_CurIndex; i++)
		{
			NTRANS_FreeMediaData(PRV_OldVideoData[i]);
			PRV_OldVideoData[i] = NULL;
		}
		PRV_CurIndex = 0;
		PRV_SendDataLen = 0;
	}
#endif
	PRV_RefreshVoDevScreen(dev, DISP_DOUBLE_DISP, s_astVoDevStatDflt[dev].as32ChnOrder[s_astVoDevStatDflt[dev].enPreviewMode]);
	
	if(s_State_Info.bIsOsd_Init && s_State_Info.bIsRe_Init)
	{
#if defined(SN9234H1)
		Prv_OSD_Show(HD, HI_FALSE);
		Prv_OSD_Show(AD, HI_FALSE);
#else /*2010-9-19 ˫����*/
		Prv_OSD_Show(DHD0, HI_FALSE);
		//Prv_OSD_Show(DSD0, HI_FALSE);
#endif
	}
	PRV_DisableDigChnAudio();
	CurCap = 0;
	CurMasterCap = 0;
	CurSlaveCap = 0;
	RET_SUCCESS("");
}

/************************************************************************/
/* �˳��طš�
                                                                     */
/************************************************************************/
STATIC HI_S32 PRV_ExitPB(VO_DEV dev,HI_S32 s32Flag)
{
	int i = 0;
#if defined(Hi3531)
	AUDIO_DEV AoDev = 4;
#else
	AUDIO_DEV AoDev = 0;
#endif
#if defined(SN9234H1)
	if(dev == SPOT_VO_DEV|| dev == AD)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}
	if (dev<0 || dev>=PRV_VO_MAX_DEV)
	{
		RET_FAILURE("invalid dev!!");
	}
#else
	if(dev > DHD0)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}
	if (dev < 0 || dev > DHD0/*VoDev >= PRV_VO_MAX_DEV*/)
	{
		RET_FAILURE("invalid dev!!");
	}
#endif
	if (PRV_CTRL_PB != s_astVoDevStatDflt[dev].enCtrlFlag
		|| (PRV_STAT_PB != s_astVoDevStatDflt[dev].enPreviewStat
		&& PRV_STAT_PIC != s_astVoDevStatDflt[dev].enPreviewStat))
	{
		RET_FAILURE("Ctrl Flag NOT PB or Preview stat not in PB and not in pic stat!");
	}
#if defined(SN9234H1)
	s_astVoDevStatDflt[dev].enPreviewStat = PRV_STAT_NORM;

	PRV_EnableAllVoChn(HD);
	PRV_HideAllVoChn(HD);
	//PRV_BindAllVoChn(HD);

	//PRV_EnableAllVoChn(s_VoSecondDev);
	//PRV_HideAllVoChn(s_VoSecondDev);
	//PRV_BindAllVoChn(AD);
#else
	for(i = LOCALVEDIONUM; i < DEV_CHANNEL_NUM; i++)
	{
		if(VochnInfo[i].IsConnect == 1 && //δ�Ͽ��豸���ӡ��ڻط��ڼ䣬�п��ܶϿ�����
			VochnInfo[i].SlaveId == PRV_MASTER && 
			VochnInfo[i].VdecChn >= 0
			&& VochnInfo[i].VdecChn != DetVLoss_VdecChn && VochnInfo[i].VdecChn != NoConfig_VdecChn)
		{		
			if(HI_SUCCESS == PRV_CreateVdecChn(VochnInfo[i].VideoInfo.vdoType, VochnInfo[i].VideoInfo.height, VochnInfo[i].VideoInfo.width, VochnInfo[i].u32RefFrameNum, VochnInfo[i].VdecChn))
			{
				VochnInfo[i].IsHaveVdec = 1;
			}
			else
			{				
				TRACE(SCI_TRACE_HIGH, MOD_PRV,"-------line: %d, Create Vdec: %d fail!!!\n", __LINE__, VochnInfo[i].VdecChn);
			}
		}
	}
	s_astVoDevStatDflt[dev].enPreviewStat = PRV_STAT_NORM;

	PRV_EnableAllVoChn(DHD0);
	PRV_HideAllVoChn(DHD0);
	//PRV_BindAllVoChn(HD);
	CHECK_RET(HI_MPI_VO_SetPlayToleration (DHD0, 200));
#if defined(Hi3531)
	PRV_EnableAllVoChn(s_VoSecondDev);
	PRV_HideAllVoChn(s_VoSecondDev);
#endif
	//PRV_BindAllVoChn(AD);
#endif
	if(s32Flag != PIC_CTL_FLAG)
	{
		PRV_TkPbSwitch(0);
		//PRV_EnableAudioPreview();
	}

	AIO_ATTR_S stmp;
#if defined(SN9234H1)	
	CHECK(HI_MPI_AO_GetPubAttr(AoDev, &stmp));
#else
	CHECK(HI_MPI_AO_GetPubAttr(AoDev, &stmp));
#endif
	PtNumPerFrm = stmp.u32PtNumPerFrm;
	for(i = 0; i < DEV_CHANNEL_NUM; i++)
	{
		PRV_PBStateInfoInit(i);
		PRV_PtsInfoInit(i);
		PRV_InitVochnInfo(i);
		PRV_VoChnStateInit(i);
	}
	if(PRV_CurIndex > 0)
	{
		for(i = 0; i < PRV_CurIndex; i++)
		{
			NTRANS_FreeMediaData(PRV_OldVideoData[i]);
			PRV_OldVideoData[i] = NULL;
		}
		PRV_CurIndex = 0;
		PRV_SendDataLen = 0;
	}
	
	PRV_RefreshVoDevScreen(dev, DISP_DOUBLE_DISP,s_astVoDevStatDflt[dev].as32ChnOrder[s_astVoDevStatDflt[dev].enPreviewMode]);


	if(s_State_Info.bIsOsd_Init && s_State_Info.bIsRe_Init)
	{
#if defined(SN9234H1)		
		Prv_OSD_Show(HD,HI_TRUE);
		Prv_OSD_Show(AD,HI_TRUE);
#else
		Prv_OSD_Show(DHD0,HI_TRUE);
#endif
	}
	PRV_PBPlayInfoInit();
	PRV_EnableDigChnAudio();
	CurCap = 0;
	CurMasterCap = 0;
	CurSlaveCap = 0;
	sem_post(&sem_PrvGetData); 		
	//sem_post(&sem_PrvSendData); 	

	RET_SUCCESS("");
}

#if defined(SN_SLAVE_ON)
/*************************************************
Function: //PRV_TimeOut_repeat
Description: //��ʱ��ʱ���������ó���
Calls: 
Called By: //
Input: // timer:��ʱ��ʱ�䣬�ط����Ϊԭʱ��+2
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
static int PRV_TimeOut_repeat(int timer)
{
	switch(s_State_Info.TimeoutCnt)
	{
		case 0:
		{//������Ƭ��ʱ��ʱ��Ϊ100ms
			if(s_State_Info.f_timer_handle == -1)
			{
				TimeInfo timer_info;
				SN_MEMSET(&timer_info, 0, sizeof(TimeInfo));
				timer_info.type = 1;//��ʱ����������Ϊ��Ϣ����
				timer_info.info.MESSAGE = MSG_ID_PRV_DISPLAY_TIMEOUT_IND;//��Ϣ����ֵΪMSG_ID_PRV_DISPLAY_TIMEOUT_IND
				
				s_State_Info.f_timer_handle = TimerAdd(MOD_PRV, timer, timer_info,timer, 0);
			}else
			{	//���ö�ʱ��
				TimerReset(s_State_Info.f_timer_handle,timer);
				TimerResume(s_State_Info.f_timer_handle,0);
			}
			s_State_Info.bIsTimerState = HI_TRUE;
			s_State_Info.TimerType = PRV_INIT;
		}
			break;
		case 1:
		{//�������ö�ʱ����ʱ��Ϊ200ms
			TimerReset(s_State_Info.f_timer_handle,timer+2);
			TimerResume(s_State_Info.f_timer_handle,0);
			s_State_Info.bIsTimerState = HI_TRUE;
			s_State_Info.TimerType = PRV_INIT;
		}
			break;
		case 2:
		{//�������ö�ʱ����ʱ��Ϊ300ms
			TimerReset(s_State_Info.f_timer_handle,timer+4);
			TimerResume(s_State_Info.f_timer_handle,0);
			s_State_Info.bIsTimerState = HI_TRUE;
			s_State_Info.TimerType = PRV_INIT;
		}
			break;
		case 3:
		{	//3�γ�ʱ����ô������Ƭ
			s_State_Info.bslave_IsInit = HI_TRUE;//�����Ƭ��Ϣ�������ˣ���ô��λ��Ƭ��ʼ����־λ
			s_State_Info.TimeoutCnt = HI_FALSE;//������λ��ʱ��־λ
			s_State_Info.bIsSlaveConfig = HI_FALSE;	//������λ���ñ�־λ
			s_State_Info.bIsReply = 0;			//�ظ�״̬�˳�
			s_State_Info.g_slave_OK = 0;	//���ûظ���ֵ��־λ
		}	
			return-1;
		default:
			return -1;
	}
	return 0;
}

/*************************************************
Function: //Prv_Set_Slave_chn_param
Description: //��ʼ����Ƭ����
Calls: 
Called By: //
Input: //
Output: // 
Return: //
Others: // ����˵��
************************************************************************/
static int Prv_Set_Slave_chn_param(void)
{
//	HI_S32 i=0;
#if defined(SN9234H1)
	s_slaveVoStat.bIsSingle = s_astVoDevStatDflt[HD].bIsSingle;
	s_slaveVoStat.enPreviewMode = s_astVoDevStatDflt[HD].enPreviewMode;
	s_slaveVoStat.s32PreviewIndex = s_astVoDevStatDflt[HD].s32PreviewIndex;
	s_slaveVoStat.s32SingleIndex = s_astVoDevStatDflt[HD].s32SingleIndex;
#else
	s_slaveVoStat.bIsSingle = s_astVoDevStatDflt[DHD0].bIsSingle;
	s_slaveVoStat.enPreviewMode = s_astVoDevStatDflt[DHD0].enPreviewMode;
	s_slaveVoStat.s32PreviewIndex = s_astVoDevStatDflt[DHD0].s32PreviewIndex;
	s_slaveVoStat.s32SingleIndex = s_astVoDevStatDflt[DHD0].s32SingleIndex;
#endif	
	s_slaveVoStat.enVideoNorm = s_s32NPFlagDflt;
	return 0;
}
#endif

/*************************************************
Function: //PRV_Init_TimeOut
Description: //��ʼ����ʱ����
Calls: 
Called By: //
Input: // is_first:�Ƿ���ε��ã�0��ʾ���ε��ã�1��ʾ�ٴε���
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
#if defined(SN_SLAVE_ON)
static int PRV_Slave_Init(unsigned char is_first)
{
	int i,j;
	Prv_Slave_Init_Req slave_req;
	//���ʹ�Ƭ��ʼ����Ϣ
	slave_req.DecodeMode= PRV_CurDecodeMode;	
	slave_req.VoOutPutMode = OutPutMode;
	slave_req.enPreviewMode= s_slaveVoStat.enPreviewMode;
	slave_req.bIsSingle= s_slaveVoStat.bIsSingle;
	slave_req.s32PreviewIndex= s_slaveVoStat.s32PreviewIndex;
	slave_req.s32SingleIndex= s_slaveVoStat.s32SingleIndex;
	slave_req.enVideoNorm = s_slaveVoStat.enVideoNorm;
	SN_MEMCPY(slave_req.slave_OSD_off_flag,PRV_CHAN_NUM, s_slaveVoStat.slave_OSD_off_flag,PRV_CHAN_NUM,PRV_CHAN_NUM);
	//printf("############slave_req.enPreviewMode = %d###########################\n",slave_req.enPreviewMode);
	for(i=0;i<NINEINDEX;i++)
	{
		slave_req.as32ChnOrder[SingleScene][i] = s_astVoDevStatDflt[HD].as32ChnOrder[SingleScene][i];
		slave_req.as32ChnpollOrder[SingleScene][i] = s_astVoDevStatDflt[HD].as32ChnpollOrder[SingleScene][i];
		slave_req.as32ChnOrder[ThreeScene][i] = s_astVoDevStatDflt[HD].as32ChnOrder[ThreeScene][i];
		slave_req.as32ChnpollOrder[ThreeScene][i] = s_astVoDevStatDflt[HD].as32ChnpollOrder[ThreeScene][i];
		slave_req.as32ChnOrder[FourScene][i] = s_astVoDevStatDflt[HD].as32ChnOrder[FourScene][i];
		slave_req.as32ChnpollOrder[FourScene][i] = s_astVoDevStatDflt[HD].as32ChnpollOrder[FourScene][i];
		slave_req.as32ChnOrder[FiveScene][i] = s_astVoDevStatDflt[HD].as32ChnOrder[FiveScene][i];
		slave_req.as32ChnpollOrder[FiveScene][i] = s_astVoDevStatDflt[HD].as32ChnpollOrder[FiveScene][i];
		slave_req.as32ChnOrder[SixScene][i] = s_astVoDevStatDflt[HD].as32ChnOrder[SixScene][i];
		slave_req.as32ChnpollOrder[SixScene][i] = s_astVoDevStatDflt[HD].as32ChnpollOrder[SixScene][i];
		slave_req.as32ChnOrder[SevenScene][i] = s_astVoDevStatDflt[HD].as32ChnOrder[SevenScene][i];
		slave_req.as32ChnpollOrder[SevenScene][i] = s_astVoDevStatDflt[HD].as32ChnpollOrder[SevenScene][i];
		slave_req.as32ChnOrder[EightScene][i] = s_astVoDevStatDflt[HD].as32ChnOrder[EightScene][i];
		slave_req.as32ChnpollOrder[EightScene][i] = s_astVoDevStatDflt[HD].as32ChnpollOrder[EightScene][i];
		slave_req.as32ChnOrder[NineScene][i] = s_astVoDevStatDflt[HD].as32ChnOrder[NineScene][i];
		slave_req.as32ChnpollOrder[NineScene][i] = s_astVoDevStatDflt[HD].as32ChnpollOrder[NineScene][i];
		slave_req.as32ChnOrder[SixteenScene][i] = s_astVoDevStatDflt[HD].as32ChnOrder[SixteenScene][i];
		slave_req.as32ChnpollOrder[SixteenScene][i] = s_astVoDevStatDflt[HD].as32ChnpollOrder[SixteenScene][i];
		
		if(i<PRV_CHAN_NUM)
		{
			for(j=0;j<REC_OSD_GROUP;j++)
			{
				slave_req.slave_Chn_Bmp_h[j][i] = s_slaveVoStat.slave_BmpData_name_w[j][i];
				slave_req.slave_Chn_Bmp_w[j][i] = s_slaveVoStat.slave_BmpData_name_h[j][i];
				slave_req.slave_Chn_Bmp_DSize[j][i] = s_slaveVoStat.slave_BmpData_name_size[j][i];	
				slave_req.f_rec_srceen_h[j][i] = s_slaveVoStat.f_rec_srceen_h[j][i];
				slave_req.f_rec_srceen_w[j][i] = s_slaveVoStat.f_rec_srceen_w[j][i];
			}
		}
	}
	for(i=0;i<THREEINDEX;i++)
	{
        slave_req.as32ChnOrder[ThreeScene][i] = s_astVoDevStatDflt[HD].as32ChnOrder[ThreeScene][i];
		slave_req.as32ChnpollOrder[ThreeScene][i] = s_astVoDevStatDflt[HD].as32ChnpollOrder[ThreeScene][i];
	}
	for(i=0;i<FIVEINDEX;i++)
	{
        slave_req.as32ChnOrder[FiveScene][i] = s_astVoDevStatDflt[HD].as32ChnOrder[FiveScene][i];
		slave_req.as32ChnpollOrder[FiveScene][i] = s_astVoDevStatDflt[HD].as32ChnpollOrder[FiveScene][i];
	}
	for(i=0;i<SEVENINDEX;i++)
	{
        slave_req.as32ChnOrder[SevenScene][i] = s_astVoDevStatDflt[HD].as32ChnOrder[SevenScene][i];
		slave_req.as32ChnpollOrder[SevenScene][i] = s_astVoDevStatDflt[HD].as32ChnpollOrder[SevenScene][i];
	}
	
	SN_MEMCPY(slave_req.cover_info,PRV_CHAN_NUM*sizeof(PRM_AREA_HIDE), s_slaveVoStat.cover_info,PRV_CHAN_NUM*sizeof(PRM_AREA_HIDE),PRV_CHAN_NUM*sizeof(PRM_AREA_HIDE));
	SN_MEMCPY(slave_req.slave_OSD_Rect,PRV_CHAN_NUM*REGION_NUM*sizeof(Preview_Point),s_slaveVoStat.slave_OSD_Rect,PRV_CHAN_NUM*REGION_NUM*sizeof(Preview_Point),PRV_CHAN_NUM*REGION_NUM*sizeof(Preview_Point));

	Slave_Get_Time_InitInfo(slave_req.slave_osd_time,MAX_TIME_STR_LEN);//��ȡʱ��ͼƬ��ʼ����Ϣ
	SN_SendMccMessageEx(PRV_SLAVE_1,SUPER_USER_ID, MOD_PRV, MOD_PRV, 0, 0, MSG_ID_PRV_MCC_SLAVE_INIT_REQ, &slave_req, sizeof(Prv_Slave_Init_Req));
	s_State_Info.bIsSlaveConfig = HI_TRUE;	//��λ���ñ�־λ

	return 0;
}
#endif
static int PRV_Init_TimeOut(unsigned char is_first)
{
	
#if defined(SN_SLAVE_ON)	
	int ret=0;
	

	if(s_State_Info.bslave_IsInit == HI_FALSE)
	{
	//�����Ƭû�г�ʼ�������ͳ�ʼ����Ϣ
		if(is_first == 0)
		{ 
			//����ǳ��γ�ʼ������ô��Ҫ��ȡ������Ϣ
			Prv_Set_Slave_chn_param();
		}
		//printf("###############s  PRV_Init_TimeOut  !!!! s_State_Info.TimeoutCnt = %d##########################\n",s_State_Info.TimeoutCnt);
		ret = PRV_TimeOut_repeat(5);
		if(ret == -1)
		{//������Ƭ
			
		}
		else
		{
			//���ʹ�Ƭ��ʼ����Ϣ
			PRV_Slave_Init(is_first);
		}
	}
#endif	
	return 0;
}


/*************************************************
Function: //PRV_MSG_Mcc_Init_Rsp
Description: // �յ���Ƭ���س�ʼ�� ��Ϣ����
Calls: 
Called By: //
Input: // msg_req :�յ��Ĵ�Ƭ��Ϣ�ṹ��
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_MSG_Mcc_Init_Rsp(const SN_MSG *msg_req)
{
	Prv_Slave_Init_Rsp *rsp = (Prv_Slave_Init_Rsp *)msg_req->para;
				
	//�����Ƭ���أ���ô��λ��Ƭ�ķ��ر�־λ
	s_State_Info.bIsReply |= 1<<(rsp->slaveid-1);
	if(rsp->result == SN_RET_OK)
	{
		s_State_Info.g_slave_OK = 1<<(rsp->slaveid-1);
	}
	else
	{//�����Ƭ���ش�����Ϣ����ô������
		PRV_Init_TimeOut(1);
	}
	//�������ֵ��1 ����1����оƬ��������ֵ����ô��ʾ���д�Ƭ��������
	if((s_State_Info.bIsReply+1) == 1<<(PRV_CHIP_NUM-1))	
	{
		s_State_Info.bslave_IsInit = HI_TRUE;//�����Ƭ��Ϣ�������ˣ���ô��λ��Ƭ��ʼ����־λ
		s_State_Info.TimeoutCnt = HI_FALSE;//������λ��ʱ��־λ
		s_State_Info.bIsSlaveConfig = HI_FALSE;	//������λ���ñ�־λ
		TimerPause(s_State_Info.f_timer_handle); //��ͣ��ʱ��
		s_State_Info.bIsReply = 0;			//�ظ�״̬�˳�
		s_State_Info.g_slave_OK = 0;
	}

	RET_SUCCESS("");
}

#if defined(SN9234H1)
/************************************************************************/
/* ����ͨ����ʾ������ʱ����ͨ��˳�������С�
                                                                     */
/************************************************************************/
STATIC HI_VOID PRV_SortChnOrder(VO_DEV VoDev)
{
#if 1
	/*��������*/
	return;
#else
	/*˳��������*/
	int i, j;
#if defined(SN8608D_LE) || defined(SN8608M_LE) || defined(SN8616D_LE) ||defined(SN8616M_LE) || defined(SN9234H1)
	if(VoDev == SPOT_VO_DEV)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}
#endif

	if (VoDev<0 || VoDev>=PRV_VO_MAX_DEV)
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV, TEXT_COLOR_RED("%s:%d: bad devid: VoDev=%d\n"), __FUNCTION__, __LINE__, VoDev);
		return;
	}

	for (i=0, j=0; i<g_Max_Vo_Num; i++)
	{
		if (s_astVoDevStatDflt[VoDev].as32ChnOrder[i] < 0
			|| s_astVoDevStatDflt[VoDev].as32ChnOrder[i] >= g_Max_Vo_Num)
		{
			continue;
		}
		else
		{
			s_astVoDevStatDflt[VoDev].as32ChnOrder[j++] = s_astVoDevStatDflt[VoDev].as32ChnOrder[i];
		}
	}
	while (j<g_Max_Vo_Num)
	{
		s_astVoDevStatDflt[VoDev].as32ChnOrder[j++] = -1;
	}
#endif
}



HI_S32 PRV_RefreshSpotOsd(int chan)
{

	unsigned char order_buf[PRV_VO_CHN_NUM];
	order_buf[0] = chan;
	OSD_Get_Preview_param(SPOT_VO_DEV,
							s_astVoDevStatDflt[SPOT_VO_DEV].stVideoLayerAttr.stDispRect.u32Width,
							s_astVoDevStatDflt[SPOT_VO_DEV].stVideoLayerAttr.stDispRect.u32Height,
							1,SingleScene,order_buf);
	if((s_State_Info.bIsOsd_Init && s_State_Info.bIsRe_Init))
	{
		if(SPOT_VO_DEV == s_VoSecondDev)
		{
			Prv_Disp_OSD(AD);
		}
		else
		{
			Prv_Disp_OSD(SPOT_VO_DEV);
		}
	}
	return HI_SUCCESS;
}

HI_S32 PRV_HostStopPciv(PCIV_CHN PcivChn, int EventId)
{
	HI_S32  i;
	HI_U32 u32Count;
	PCIV_CHN  ch = PcivChn;
	PCIV_ATTR_S  stPciAttr;
	PCIV_BIND_OBJ_S astBindObj[PCIV_MAX_BINDOBJ];

	TRACE(SCI_TRACE_NORMAL, MOD_PRV,  "PRV_HostStopPciv: chan=%d\n", PcivChn);
	//printf("PRV_HostStopPciv: chan=%d\n", PcivChn);

	/* 1, stop */
	CHECK_RET(HI_MPI_PCIV_Stop(PcivChn));

	/* 2, unbind */
	CHECK_RET(HI_MPI_PCIV_EnumBindObj(PcivChn, astBindObj, &u32Count));
	for (i=0; i<u32Count; i++)
	{
		CHECK_RET(HI_MPI_PCIV_UnBind(PcivChn, &astBindObj[i]));
	}

	/* 3, free */
	CHECK_RET(HI_MPI_PCIV_GetAttr(PcivChn, &stPciAttr));
	CHECK_RET(HI_MPI_PCIV_Free(stPciAttr.u32Count, stPciAttr.u32PhyAddr));

	/* 4, destroy */
	CHECK_RET(HI_MPI_PCIV_Destroy(PcivChn));

	SN_SendMccMessageEx(PRV_SLAVE_1, SUPER_USER_ID, MOD_PRV, MOD_PRV, 0, 0, EventId, &ch, sizeof(PCIV_CHN));
    	return HI_SUCCESS;
}




/*************************************************
Function: //PRV_InitSpotVo
Description: ��ʼ��SPOT�ڵ� VO��
Calls: 
Called By: //
Input: //
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_InitSpotVo(void)
{
	HI_U32 u32Width, u32Height;
	VO_CHN_ATTR_S stVoChnAttr;
	RECT_S *pstLayout = NULL;
	VO_DEV VoDev = SPOT_VO_DEV;


	u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width;
	u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height;
	pstLayout = s_astPreviewLayout1;

	PRV_HideAllVoChn(VoDev);
	PRV_ViUnBindAllVoChn(VoDev);
	PRV_VdecUnBindAllVoChn1(VoDev);

	CHECK_RET(HI_MPI_VO_GetChnAttr(VoDev, SPOT_VO_CHAN, &stVoChnAttr));

	//stVoChnAttr.u32Priority = PRV_DFLT_CHN_PRIORITY + 1;
	stVoChnAttr.stRect.s32X		= (u32Width * pstLayout->s32X) / PRV_PREVIEW_LAYOUT_DIV;
	stVoChnAttr.stRect.s32Y		= (u32Height * pstLayout->s32Y) / PRV_PREVIEW_LAYOUT_DIV;
	stVoChnAttr.stRect.u32Width	= (u32Width * pstLayout->u32Width) / PRV_PREVIEW_LAYOUT_DIV;
	stVoChnAttr.stRect.u32Height	= (u32Height * pstLayout->u32Height) / PRV_PREVIEW_LAYOUT_DIV;

	stVoChnAttr.stRect.s32X /= 2;
	stVoChnAttr.stRect.s32Y /= 2;
	stVoChnAttr.stRect.u32Width /= 2;
	stVoChnAttr.stRect.u32Height /= 2;
	stVoChnAttr.stRect.s32X *= 2;
	stVoChnAttr.stRect.s32Y *= 2;
	stVoChnAttr.stRect.u32Width *= 2;
	stVoChnAttr.stRect.u32Height *= 2;

	CHECK_RET(HI_MPI_VO_SetChnAttr(VoDev, SPOT_VO_CHAN, &stVoChnAttr));
	CHECK_RET(HI_MPI_VO_ChnShow(VoDev,SPOT_VO_CHAN));


	RET_SUCCESS("");
}


/*************************************************
Function: //PRV_PrevInitSpotVo
Description: ����ָ��VO״̬��ʾVO�豸�ϵ���Ƶ��Ļ����Ų���
Calls: 
Called By: //
Input: //VoDev:�豸��
		enPreviewMode:��Ҫ��ʾԤ��ģʽ
		u32Index:�໭��ͨ��������
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_PrevInitSpotVo( HI_U32 u32Index)
{
	HI_U32 u32Width, u32Height;
	VO_CHN VoChn;
	VO_CHN_ATTR_S stVoChnAttr;
	RECT_S *pstLayout = NULL;
	VO_DEV VoDev = SPOT_VO_DEV;
	int index = 0;
	RECT_S stSrcRect, stDestRect;

	/*ȷ�������ĺϷ���*/

	if ( (VoDev < 0 || VoDev >= PRV_VO_MAX_DEV))
	{
		RET_FAILURE("Invalid Parameter: VoDev or u32Index");
	}

	//TRACE(SCI_TRACE_NORMAL, MOD_PRV,  "PRV_PrevInitSpotVo = %d\n", u32Index);
	//printf("PRV_PrevInitSpotVo = %d\n", u32Index);
	u32Width = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width;
	u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height;
	pstLayout = s_astPreviewLayout1;

	PRV_HideAllVoChn(VoDev);
	PRV_ViUnBindAllVoChn(VoDev);
	//PRV_VdecUnBindAllVoChn(VoDev);
	
	VoChn = u32Index;
	index = PRV_GetVoChnIndex(VoChn);

	CHECK_RET(HI_MPI_VO_GetChnAttr(VoDev, SPOT_VO_CHAN, &stVoChnAttr));

	//stVoChnAttr.u32Priority = PRV_DFLT_CHN_PRIORITY + 1;
	stVoChnAttr.stRect.s32X		= (u32Width * pstLayout->s32X) / PRV_PREVIEW_LAYOUT_DIV;
	stVoChnAttr.stRect.s32Y		= (u32Height * pstLayout->s32Y) / PRV_PREVIEW_LAYOUT_DIV;
	stVoChnAttr.stRect.u32Width	= (u32Width * pstLayout->u32Width) / PRV_PREVIEW_LAYOUT_DIV;
	stVoChnAttr.stRect.u32Height	= (u32Height * pstLayout->u32Height) / PRV_PREVIEW_LAYOUT_DIV;

	stSrcRect.s32X		 = stVoChnAttr.stRect.s32X;
	stSrcRect.s32Y		 = stVoChnAttr.stRect.s32Y;
	stSrcRect.u32Width	 = stVoChnAttr.stRect.u32Width;
	stSrcRect.u32Height  = stVoChnAttr.stRect.u32Height;	
	stDestRect = PRV_ReSetVoRect(OutPutMode, VochnInfo[index].VideoInfo.width, VochnInfo[index].VideoInfo.height, stSrcRect);
	stVoChnAttr.stRect.s32X 	 = stDestRect.s32X 		& (~0x01);
	stVoChnAttr.stRect.s32Y 	 = stDestRect.s32Y 		& (~0x01);
	stVoChnAttr.stRect.u32Width  = stDestRect.u32Width  & (~0x01);
	stVoChnAttr.stRect.u32Height = stDestRect.u32Height & (~0x01);


	CHECK_RET(HI_MPI_VO_SetChnAttr(VoDev, SPOT_VO_CHAN, &stVoChnAttr));

	//if(ViChn < PRV_CHAN_NUM)
	if(VoChn < LOCALVEDIONUM)
	{
		int videv = 0;
		if(VoChn >= PRV_VI_CHN_NUM)
		{
		//���ͨ��Ϊ5��8����ô��Ӧ�ɼ��豸2
			videv = PRV_656_DEV;
		}
		else
		{
			videv = PRV_656_DEV_1;
		}
		CHECK_RET(HI_MPI_VI_BindOutput(videv, VoChn%PRV_VI_CHN_NUM, VoDev, SPOT_VO_CHAN));
	}
	else if(VoChn >= LOCALVEDIONUM || VoChn < PRV_CHAN_NUM)
	{
		int index = PRV_GetVoChnIndex(VoChn);
		if(index < 0)
			return HI_FAILURE;
		if(VochnInfo[index].VdecChn >= 0 /*&& VochnInfo[index].IsBindVdec[VoDev] == 0*/)
		{
			if(VochnInfo[index].SlaveId == 0 )
			{
				//printf("-----index: %d---VoDev: %d----Bind---VochnInfo[index].VdecChn: %d, VoChn: %d\n",index, VoDev, VochnInfo[index].VdecChn, SPOT_VO_CHAN);
				if(HI_SUCCESS != HI_MPI_VDEC_BindOutput(VochnInfo[index].VdecChn, VoDev, SPOT_VO_CHAN))
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "------------Vdec Bind Vo fail\n");
				VochnInfo[index].IsBindVdec[VoDev] = (VochnInfo[index].VdecChn == DetVLoss_VdecChn || VochnInfo[index].VdecChn == NoConfig_VdecChn) ? 0 : 1;
			
			}
		}
	}	
	CHECK_RET(HI_MPI_VO_ChnShow(VoDev, SPOT_VO_CHAN));

	PRV_RefreshSpotOsd(VoChn);

	RET_SUCCESS("");
}


HI_S32 PRV_start_pciv(PCIV_CHN PcivChn)
{
	VO_DEV VoDev = SPOT_VO_DEV;    
	PCIV_ATTR_S stPcivAttr;      
	PCIV_BIND_OBJ_S stPcivBindObj;    

	CurrertPciv = PcivChn;
	TRACE(SCI_TRACE_NORMAL, MOD_PRV,  "PRV_start_pciv: chan=%d\n", PcivChn);

	stPcivAttr.stPicAttr.u32Width = 704;
	//stPcivAttr.stPicAttr.u32Height = (s_s32NPFlagDflt ==0) ? PRV_IMAGE_SIZE_H_P : PRV_IMAGE_SIZE_H_N;
	
	stPcivAttr.stPicAttr.u32Height = 576;
	stPcivAttr.stPicAttr.u32Stride[2] = stPcivAttr.stPicAttr.u32Stride[1] = stPcivAttr.stPicAttr.u32Stride[0] = 704;
	//stPcivAttr.stPicAttr.u32Field  = VIDEO_FIELD_INTERLACED;	
	
	stPcivAttr.stPicAttr.u32Field  = VIDEO_FIELD_FRAME;	
	stPcivAttr.stPicAttr.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;

	/* config target pci id and pciv chn*/
	stPcivAttr.s32BufChip = 0;
	stPcivAttr.stRemoteObj.s32ChipId = PRV_SLAVE_1;
	stPcivAttr.stRemoteObj.pcivChn = PcivChn;

	/* 1) malloc phyaddr for receive pic (come from video buffer) */
	stPcivAttr.u32Count = 4;
	stPcivAttr.u32BlkSize = stPcivAttr.stPicAttr.u32Stride[0]*stPcivAttr.stPicAttr.u32Height*2;
	CHECK_RET(HI_MPI_PCIV_Malloc(stPcivAttr.u32BlkSize, stPcivAttr.u32Count, stPcivAttr.u32PhyAddr));

	/* 2) create pciv chn */
	CHECK_RET(HI_MPI_PCIV_Create(PcivChn, &stPcivAttr));

	/* 3) pciv chn bind vo chn (for display pic in host)*/
	stPcivBindObj.enType = PCIV_BIND_VO;
	stPcivBindObj.unAttachObj.voDevice.voDev = VoDev;
	stPcivBindObj.unAttachObj.voDevice.voChn = SPOT_VO_CHAN;
	CHECK_RET(HI_MPI_PCIV_Bind(PcivChn, &stPcivBindObj));

	/* 4) start pciv chn (now vo will display pic from slave chip) */
	CHECK_RET(HI_MPI_PCIV_Start(PcivChn));


	TRACE(SCI_TRACE_NORMAL, MOD_PRV,  "pciv chn%d start ok, remote(%d,%d);then send msg to slave chip !\n", PcivChn, stPcivAttr.stRemoteObj.s32ChipId,stPcivAttr.stRemoteObj.pcivChn);
	//printf("pciv chn: %d start ok, remote(%d,%d);then send msg to slave chip !\n", PcivChn, stPcivAttr.stRemoteObj.s32ChipId,stPcivAttr.stRemoteObj.pcivChn);


	SN_SendMccMessageEx(PRV_SLAVE_1, SUPER_USER_ID, MOD_PRV, MOD_PRV, 0, 0, MSG_ID_PRV_MCC_SPOT_PREVIEW_START_REQ, &stPcivAttr, sizeof(PCIV_ATTR_S));
	return HI_SUCCESS;
}


#endif


HI_S32 PRV_SetVoPreview(const SN_MSG *msg_req)
{
	HI_S32 i = 0, j = 0;
	UINT8 index = 255;
	PRM_PREVIEW_CFG_EX	stVoPreview;
	PRM_PREVIEW_CFG_EX_EXTEND stVoPreview_extend;
	if (ERROR == GetParameter(PRM_ID_PREVIEW_CFG_EX, NULL, &stVoPreview, sizeof(PRM_PREVIEW_CFG_EX), 0, SUPER_USER_ID, NULL))
	{
		TRACE(SCI_TRACE_NORMAL, MOD_SCM, "PRV_MSG_LayoutCtrl---PRM_ID_PREVIEW_CFG_EX Error!\n");
		return HI_FAILURE;
	}
	if (ERROR == GetParameter(PRM_ID_PREVIEW_CFG_EX_EXTEND, NULL, &stVoPreview_extend, sizeof(PRM_PREVIEW_CFG_EX_EXTEND), 0, SUPER_USER_ID, NULL))
	{
		TRACE(SCI_TRACE_NORMAL, MOD_SCM, "PRV_MSG_LayoutCtrl---PRM_ID_PREVIEW_CFG_EXTEND Error!\n");
		return HI_FAILURE;
	}
	for(i = 0; i < PRV_VO_MAX_DEV; i++)
	{
#if defined(SN9234H1)
		if(i != HD)
#else
		if(i > DHD0)
#endif			
			continue;

		if(ScmGetIsListCtrlStat() == 1)
		{
			s_astVoDevStatDflt[i].as32ChnOrder[SingleScene][0] = UCHAR2INIT32(j);
			for(j = 0; j < CHANNEL_NUM; j++)
			{
                if(j < 3)
				{
					s_astVoDevStatDflt[i].as32ChnOrder[ThreeScene][j] = UCHAR2INIT32(j);
				}
				if(j < 5)
				{
					s_astVoDevStatDflt[i].as32ChnOrder[FiveScene][j] = UCHAR2INIT32(j);
				}
				if(j < 7)
				{
					s_astVoDevStatDflt[i].as32ChnOrder[SevenScene][j] = UCHAR2INIT32(j);
				}
				
				if(j < 4)
				{
					s_astVoDevStatDflt[i].as32ChnOrder[FourScene][j] = UCHAR2INIT32(j);
					s_astVoDevStatDflt[i].as32ChnOrder[NineScene][j] = UCHAR2INIT32(j);
					s_astVoDevStatDflt[i].as32ChnOrder[SixteenScene][j] = UCHAR2INIT32(j);
				}
				else if(j < 9)
				{
					s_astVoDevStatDflt[i].as32ChnOrder[NineScene][j] = UCHAR2INIT32(j);
					s_astVoDevStatDflt[i].as32ChnOrder[SixteenScene][j] = UCHAR2INIT32(j);
				}
				else
				{
					s_astVoDevStatDflt[i].as32ChnOrder[SixteenScene][j] = UCHAR2INIT32(j);
				}
			}
			//4��Ԥ��ģʽ�¶�Ӧ����Ƶͨ��
			for(j = 0; j < 4; j++)
			{
				index = stVoPreview.AudioChn[j];
				//������Ƶͨ����
				if(j == 0)
				{
					if(index == 0)
						s_astVoDevStatDflt[i].AudioChn[j] = UCHAR2INIT32(index);
					else
						s_astVoDevStatDflt[i].AudioChn[j] = -1;
				}
				else if(j == 1)
				{
					if(index < 4)
						s_astVoDevStatDflt[i].AudioChn[j] = UCHAR2INIT32(index);
					else
						s_astVoDevStatDflt[i].AudioChn[j] = -1;
				}
				else if(j == 2)
				{
					if(index < 9)
						s_astVoDevStatDflt[i].AudioChn[j] = UCHAR2INIT32(index);
					else
						s_astVoDevStatDflt[i].AudioChn[j] = -1;
				}
				else
				{
					if(index < 16)
						s_astVoDevStatDflt[i].AudioChn[j] = UCHAR2INIT32(index);
					else
						s_astVoDevStatDflt[i].AudioChn[j] = -1;
				}
			}
			for(j = 0; j < 3; j++)
			{
				index = stVoPreview_extend.AudioChn[j];
				//������Ƶͨ����
				if(j == 0)
				{
					if(index < 3)
						s_astVoDevStatDflt[i].AudioChn[j+4] = UCHAR2INIT32(index);
					else
						s_astVoDevStatDflt[i].AudioChn[j+4] = -1;
				}
				else if(j == 1)
				{
					if(index < 5)
						s_astVoDevStatDflt[i].AudioChn[j+4] = UCHAR2INIT32(index);
					else
						s_astVoDevStatDflt[i].AudioChn[j+4] = -1;
				}
				else if(j == 2)
				{
					if(index < 7)
						s_astVoDevStatDflt[i].AudioChn[j+4] = UCHAR2INIT32(index);
					else
						s_astVoDevStatDflt[i].AudioChn[j+4] = -1;
				}
			}
            
		}
		else
		{
			s_astVoDevStatDflt[i].as32ChnOrder[SingleScene][0] = UCHAR2INIT32(stVoPreview.SingleOrder);
			for(j = 0; j < CHANNEL_NUM; j++)
			{
                if(j < 3)
				{
					s_astVoDevStatDflt[i].as32ChnOrder[ThreeScene][j] = UCHAR2INIT32(stVoPreview_extend.ThreeOrder[j]);
				} 
				if(j < 5)
				{
					s_astVoDevStatDflt[i].as32ChnOrder[FiveScene][j] = UCHAR2INIT32(stVoPreview_extend.FiveOrder[j]);
				} 
				if(j < 7)
				{
					s_astVoDevStatDflt[i].as32ChnOrder[SevenScene][j] = UCHAR2INIT32(stVoPreview_extend.SevenOrder[j]);
				} 
				if(j < 4)
				{
					s_astVoDevStatDflt[i].as32ChnOrder[FourScene][j] = UCHAR2INIT32(stVoPreview.FourOrder[j]);
					s_astVoDevStatDflt[i].as32ChnOrder[NineScene][j] = UCHAR2INIT32(stVoPreview.NineOrder[j]);
					s_astVoDevStatDflt[i].as32ChnOrder[SixteenScene][j] = UCHAR2INIT32(stVoPreview.SixteenOrder[j]);
				}
				else if(j < 9)
				{
					s_astVoDevStatDflt[i].as32ChnOrder[NineScene][j] = UCHAR2INIT32(stVoPreview.NineOrder[j]);
					s_astVoDevStatDflt[i].as32ChnOrder[SixteenScene][j] = UCHAR2INIT32(stVoPreview.SixteenOrder[j]);
				}
				else
				{
					s_astVoDevStatDflt[i].as32ChnOrder[SixteenScene][j] = UCHAR2INIT32(stVoPreview.SixteenOrder[j]);
				}
			}
			
			//4��Ԥ��ģʽ�¶�Ӧ����Ƶͨ��
			for(j = 0; j < 4; j++)
			{
				index = stVoPreview.AudioChn[j];
				//������Ƶͨ����
				if(j == 0)
				{
					if(index == 0)
						s_astVoDevStatDflt[i].AudioChn[j] = UCHAR2INIT32(stVoPreview.SingleOrder);
					else
						s_astVoDevStatDflt[i].AudioChn[j] = -1;
				}
				else if(j == 1)
				{
					if(index < 4)
						s_astVoDevStatDflt[i].AudioChn[j] = UCHAR2INIT32(stVoPreview.FourOrder[index]);
					else
						s_astVoDevStatDflt[i].AudioChn[j] = -1;
				}
				else if(j == 2)
				{
					if(index < 9)
						s_astVoDevStatDflt[i].AudioChn[j] = UCHAR2INIT32(stVoPreview.NineOrder[index]);
					else
						s_astVoDevStatDflt[i].AudioChn[j] = -1;
				}
				else
				{
					if(index < 16)
						s_astVoDevStatDflt[i].AudioChn[j] = UCHAR2INIT32(stVoPreview.SixteenOrder[index]);
					else
						s_astVoDevStatDflt[i].AudioChn[j] = -1;
				}
			}
			for(j = 0; j < 3; j++)
			{
				index = stVoPreview_extend.AudioChn[j];
				//������Ƶͨ����
				if(j == 0)
				{
					if(index <3)
						s_astVoDevStatDflt[i].AudioChn[4+j] = UCHAR2INIT32(stVoPreview_extend.ThreeOrder[index]);
					else
						s_astVoDevStatDflt[i].AudioChn[4+j] = -1;
				}
				else if(j == 1)
				{
					if(index < 5)
						s_astVoDevStatDflt[i].AudioChn[4+j] = UCHAR2INIT32(stVoPreview_extend.FiveOrder[index]);
					else
						s_astVoDevStatDflt[i].AudioChn[4+j] = -1;
				}
				else if(j == 2)
				{
					if(index < 7)
						s_astVoDevStatDflt[i].AudioChn[4+j] = UCHAR2INIT32(stVoPreview_extend.SevenOrder[index]);
					else
						s_astVoDevStatDflt[i].AudioChn[4+j] = -1;
				}
				
			}
		}
	}
#if defined(SN_SLAVE_ON)
	//������Ϣ����Ƭ
	{
		Prv_Slave_Set_vo_preview_Req_EX	slave_req;
		int ret=0;
		
		slave_req.preview_info = stVoPreview;
		slave_req.preview_info_exn=stVoPreview_extend;
#if defined(SN9234H1)		
		slave_req.dev = HD;
#else
		slave_req.dev = DHD0;
#endif
		
		ret = SN_SendMccMessageEx(PRV_SLAVE_1,msg_req->user, msg_req->source, MOD_PRV, msg_req->xid, msg_req->thread, MSG_ID_PRV_MCC_SET_VO_PREVIEW_REQ, &slave_req, sizeof(Prv_Slave_Set_vo_preview_Req_EX));
	}	
#endif
	return HI_SUCCESS;
}

void PRV_GetVoChnVideoInfo(unsigned char Chn, unsigned int *Width, unsigned int *Height)
{
	if(Chn > DEV_CHANNEL_NUM)
		return;
	*Width = VochnInfo[Chn].VideoInfo.width;	
	*Height = VochnInfo[Chn].VideoInfo.height;
}

void PRV_GetDecodeState(int VoChn, PRV_DecodeState *DecodeState)
{
#if !defined(USE_UI_OSD)
	pthread_mutex_lock(&send_data_mutex);
#endif
	if(VoChn > DEV_CHANNEL_NUM)
	{
		pthread_mutex_unlock(&send_data_mutex);
		return;
	}
	if(VochnInfo[VoChn].VdecChn >= 0
		&& VochnInfo[VoChn].VdecChn != DetVLoss_VdecChn 
		&& VochnInfo[VoChn].VdecChn != NoConfig_VdecChn)
		DecodeState->ConnectState = 1;
	else
		DecodeState->ConnectState = 0;
	
	DecodeState->Height = VochnInfo[VoChn].VideoInfo.height;
	DecodeState->Width = VochnInfo[VoChn].VideoInfo.width;
		
	DecodeState->DecodeVideoStreamFrames = VoChnState.VideoDataCount[VoChn];
	if(Achn == VoChn)
		DecodeState->DecodeAudioStreamFrames = VoChnState.SendAudioDataCount[VoChn];	
	else
		DecodeState->DecodeAudioStreamFrames = 0;
#if !defined(USE_UI_OSD)
	pthread_mutex_unlock(&send_data_mutex);
#endif

}

void PRV_UpdateDecodeStrategy(UINT8 DecodeStrategy)
{	
	int i = 0;
	pthread_mutex_lock(&send_data_mutex);
	g_PrvType = DecodeStrategy;
	for(i = 0; i < DEV_CHANNEL_NUM; i++)
		VochnInfo[i].PrvType = DecodeStrategy;
	pthread_mutex_unlock(&send_data_mutex);
}
/************************************************************************/
/* ��Ӧ����Ϣ�Ĵ�������
   PRV_MSG_???();
                                                                  */
/************************************************************************/
#if (IS_DECODER_DEVTYPE == 1)

#else
/*************************************************
Function: //PRV_MSG_ScreenCtrl
Description: // ��Ƭ��һ��/��һ���л���Ϣ ����
Calls: 
Called By: //
Input: // msg_req:�յ�����Ϣ�ṹ��
		rsp : �ظ���GUI����Ϣ
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
static HI_S32 PRV_MSG_ScreenCtrl(const SN_MSG *msg_req, Msg_id_prv_Rsp *rsp)
{
	Screen_ctrl_Req *param = (Screen_ctrl_Req *)msg_req->para;
	unsigned char vodev=param->dev;
	//��Ƭ�ظ�GUI��Ϣ��Ϣ
	rsp->chn = 0;
	rsp->dev = param->dev;
	rsp->flag = 0;
	rsp->result = -1;
	if (NULL == param || (NULL == rsp))
	{
		RET_FAILURE("NULL pointer!!!");
	}
#if defined(Hi3520)
	if(param->dev == HD || param->dev == AD)
#else
	if(param->dev == DHD0)
#endif
	//if(param->dev == HD )
	{
		//VGA ��CVBS����ѯ
		vodev = (param->dev == HD) ? HD : s_VoSecondDev;
		//printf("################PRV_MSG_ScreenCtrl  param->dev = %d  ,vodev =%d#################\n", param->dev,vodev);
#if defined(SN_SLAVE_ON)	
		{
			Prv_Slave_Screen_ctrl_Req slave_req;
			//�ѵ�ǰ����Ϣ��Ϣ������Ƭ����Ƭ��Ҫ����
			//printf("################PRV_MSG_ScreenCtrl  s32Dir = %d#################\n", param->dir);
			slave_req.dev = rsp->dev;
			slave_req.dir = param->dir;
            SN_MEMCPY(slave_req.reserve, sizeof(slave_req.reserve), param->reserve, sizeof(param->reserve), sizeof(slave_req.reserve));
			//������һ���������Ƭ
			SN_SendMccMessageEx(PRV_SLAVE_1,msg_req->user, msg_req->source, MOD_PRV, msg_req->xid, msg_req->thread, MSG_ID_PRV_MCC_SCREEN_CTRL_REQ, &slave_req, sizeof(Prv_Slave_Screen_ctrl_Req));

			TimerReset(s_State_Info.f_timer_handle, 15);
			TimerResume(s_State_Info.f_timer_handle, 0);
			
			PRV_Msg_Cpy(msg_req);		//���浱ǰ��Ϣ���Ա������Ϣ�ط�

			return SN_SLAVE_MSG;
		}
#else
		CHECK_RET(PRV_NextScreen(vodev, param->dir, param->reserve[0]));
		rsp->result = 0;
#endif
	}
	else
	{
		RET_SUCCESS("");
#if defined (SN6108HE) || (SN6108LE) || (SN6116HE) || (SN6116LE) || (SN8608D_LE) || (SN8608M_LE) || (SN8616D_LE) || (SN8616M_LE)|| defined(SN9234H1)
		{
			//SPOT����ѯ
			int index1 = 0, index2 = 0;
			int bCurrentMaster = 0, bNextMaster = 0;
			int oldchan = s_astVoDevStatDflt[vodev].s32SingleIndex, next;
			vodev = SPOT_VO_DEV;
			s_astVoDevStatDflt[vodev].bIsSingle = HI_TRUE;//ǿ��Ϊ������
			oldchan = s_astVoDevStatDflt[vodev].as32ChnOrder[SingleScene][s_astVoDevStatDflt[vodev].s32SingleIndex];
			if(oldchan < LOCALVEDIONUM)
			{
				bCurrentMaster = (oldchan >= PRV_CHAN_NUM) ? HI_FALSE : HI_TRUE;
			}
			else
			{
				index1 = PRV_GetVoChnIndex(oldchan);
				if(index1 < 0)
					RET_FAILURE("------ERR: Invalid Index!");
				bCurrentMaster = (VochnInfo[index1].SlaveId > 0) ? HI_FALSE : HI_TRUE;
			}
			CHECK_RET(PRV_GetValidChnIdx(vodev, s_astVoDevStatDflt[vodev].s32SingleIndex, &s_astVoDevStatDflt[vodev].s32SingleIndex, 
										param->dir, s_astVoDevStatDflt[vodev].as32ChnOrder[SingleScene], s_astVoDevStatDflt[vodev].as32ChnpollOrder[SingleScene]));
			
			next = s_astVoDevStatDflt[vodev].as32ChnOrder[SingleScene][s_astVoDevStatDflt[vodev].s32SingleIndex];
			if(next < LOCALVEDIONUM)
			{
				bCurrentMaster = (next >= PRV_CHAN_NUM) ? HI_FALSE : HI_TRUE;
			}
			else
			{
				index2 = PRV_GetVoChnIndex(next);
				if(index2 < 0)
					RET_FAILURE("------ERR: Invalid Index!");
				bNextMaster = (VochnInfo[index2].SlaveId > 0) ? HI_FALSE : HI_TRUE;
			}
			
			CHECK_RET(HI_MPI_VO_ClearChnBuffer(SPOT_VO_DEV, SPOT_VO_CHAN, 1)); /* ���VO���� */
			//printf("SD : current =%d, next=%d, bCurrentMaster=%d, bNextMaster=%d\n", oldchan, next, bCurrentMaster, bNextMaster);
			if( bCurrentMaster == HI_TRUE && bNextMaster == HI_TRUE)
			{
				//������Ƭ������ͨ������
				//printf("SD : bCurrentMaster =%d, bNextMaster=%d, line=%d\n", bCurrentMaster, bNextMaster, __LINE__);
				//PRV_RefreshVoDevScreen(vodev,DISP_NOT_DOUBLE_DISP,s_astVoDevStatDflt[vodev].as32ChnOrder[SingleScene]);
				if(VochnInfo[index1].IsBindVdec[SPOT_VO_DEV] != -1)
				{
					CHECK(HI_MPI_VDEC_UnbindOutputChn(VochnInfo[index1].VdecChn, SPOT_VO_DEV, SPOT_VO_CHAN));				
					VochnInfo[index1].IsBindVdec[SPOT_VO_DEV] = -1;
				}
				PRV_PrevInitSpotVo(next);
				//printf("\n");
			}
			else if( bCurrentMaster == HI_TRUE && bNextMaster == HI_FALSE)
			{
				//��ǰ����Ƭ����һ��ͨ���ڴ�Ƭ������ͨ���л���PCIͨ��
				//printf("SD : bCurrentMaster =%d, bNextMaster=%d, line=%d\n", bCurrentMaster, bNextMaster, __LINE__);
				//int videv=(oldchan >= PRV_VI_CHN_NUM) ? PRV_656_DEV : PRV_656_DEV_1;
				//CHECK(HI_MPI_VI_UnBindOutput(videv, oldchan%PRV_VI_CHN_NUM, SPOT_VO_DEV, SPOT_VO_CHAN));				
				if(VochnInfo[index1].IsBindVdec[SPOT_VO_DEV] != -1)
				{
					CHECK(HI_MPI_VDEC_UnbindOutputChn(VochnInfo[index1].VdecChn, SPOT_VO_DEV, SPOT_VO_CHAN));				
					VochnInfo[index1].IsBindVdec[SPOT_VO_DEV] = -1;

				}
				PRV_start_pciv(next);
				PRV_RefreshSpotOsd(next);
				//printf("\n");
			}
			else if( bCurrentMaster == HI_FALSE && bNextMaster == HI_TRUE)
			{
				
				//��ǰ�ڴ�Ƭ����һ��ͨ������Ƭ��֪ͨ��Ƭ���
				//TRACE(SCI_TRACE_NORMAL, MOD_PRV,  "SD : bCurrentMaster =%d, bNextMaster=%d, line=%d\n", bCurrentMaster, bNextMaster, __LINE__);

				PRV_HostStopPciv(CurrertPciv, MSG_ID_PRV_MCC_SPOT_PREVIEW_STOP_REQ);
				PRV_PrevInitSpotVo(next);
				//printf("\n");
			}
			else if( bCurrentMaster == HI_FALSE && bNextMaster == HI_FALSE)
			{
				Prv_Next_Pciv NextPciv;
				NextPciv.CurrentChan = oldchan;
				NextPciv.NextChan = next;
				#if 0
				//���ڴ�Ƭ��֪ͨ��Ƭ�л�PCIͨ��
				SN_SendMccMessageEx(PRV_SLAVE_1, SUPER_USER_ID, MOD_PRV, MOD_PRV, 0, 0, 
						MSG_ID_PRV_MCC_SPOT_PREVIEW_NEXT_REQ, &NextPciv, sizeof(Prv_Next_Pciv));
				#else
				PRV_HostStopPciv(CurrertPciv, MSG_ID_PRV_MCC_SPOT_PREVIEW_NEXT_REQ);
				//PRV_start_pciv(s_astVoDevStatDflt[vodev].s32SingleIndex);
				
				//PRV_RefreshSpotOsd(s_astVoDevStatDflt[vodev].s32SingleIndex);
				#endif
				//printf("\n");
			
			}
		}
#endif
	}
	RET_SUCCESS("");
}
/*************************************************
Function: //PRV_MSG_Mcc_ScreenCtrl_Rsp
Description: // �յ���Ƭ���غ���һ��/��һ���л� ��Ϣ����
Calls: 
Called By: //
Input: // slave_rsp :�յ��Ĵ�Ƭ��Ϣ�ṹ��
		rsp : �ظ���GUI����Ϣ
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_MSG_Mcc_ScreenCtrl_Rsp(const Prv_Slave_Screen_ctrl_Rsp *slave_rsp,Msg_id_prv_Rsp *rsp)
{	
	if (NULL == rsp || NULL == slave_rsp)
	{
		RET_FAILURE("NULL pointer!!!");
	}
	//��Ƭ�ظ�GUI��Ϣ��Ϣ
	rsp->chn = 0;
	rsp->dev = slave_rsp->dev;
	rsp->flag = 0;
	rsp->result = -1;
	 
	//�����Ƭ���أ���ô��λ��Ƭ�ķ��ر�־λ
	s_State_Info.bIsReply |= 1<<(slave_rsp->slaveid-1);
	
	if(slave_rsp->result == SN_RET_OK)
	{	//�����Ƭ������ȷֵ
		s_State_Info.g_slave_OK = 1<<(slave_rsp->slaveid-1);
	}
	//�������ֵ��1 ����1����оƬ��������ֵ����ô��ʾ���д�Ƭ��������
	if((s_State_Info.bIsReply+1) == 1<<(PRV_CHIP_NUM-1))	
	{
		TimerPause(s_State_Info.f_timer_handle); //��ͣ��ʱ��
		s_State_Info.bIsReply = 0;//�ظ�״̬�˳�
		s_State_Info.bIsSlaveConfig = HI_FALSE;	//������λ���ñ�־λ
		if((s_State_Info.g_slave_OK+1) == 1<<(PRV_CHIP_NUM-1))
		{//������ص���Ϣ��û�д�����Ϣ
			CHECK_RET(PRV_NextScreen(slave_rsp->dev, slave_rsp->dir, slave_rsp->reserve[0]));
			rsp->result = 0;
		}		
		s_State_Info.g_slave_OK = 0;	//���ûظ���ֵ��־λ
	}
	RET_SUCCESS("");
}
#endif
/*************************************************
Function: //PRV_MSG_LayoutCtrl
Description: // ��Ƭ�л����沼����Ϣ����
Calls: 
Called By: //
Input: // slave_rsp :�յ�����Ϣ�ṹ��
		rsp : �ظ���GUI����Ϣ
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_MSG_LayoutCtrl(const SN_MSG *msg_req , Msg_id_prv_Rsp *rsp)
{
	//printf("%s Line %d ---------> here\n",__func__,__LINE__);
	HI_S32 s32Index = 0, vochn = 0;
	Layout_crtl_Req *param = (Layout_crtl_Req *)msg_req->para;
	unsigned char vodev = param->dev;
	
	if (NULL == param || NULL == rsp)
	{
		RET_FAILURE("NULL pointer!!!");
	}
	
	//printf("##########ch=%d,dev=%d,mode=%d,flag=%d#############################\n",
									//param->chn,param->dev,param->mode,param->flag);
	rsp->chn = param->chn;	//����ʾ�ڼ�������
	rsp->dev = param->dev;
	rsp->flag = 0;
	rsp->result = -1;
#if defined(SN9234H1)
	vodev = (param->dev == HD) ? HD : s_VoSecondDev;
#else
	vodev = (param->dev == DHD0) ? DHD0 : s_VoSecondDev;
#endif

	if(PRV_CurDecodeMode == param->flag  && 
		s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewMode == param->mode && 
		s_astVoDevStatDflt[s_VoDevCtrlDflt].bIsAlarm == 0 &&
		param->reserve != 1 && param->reserve != 2\
		&&param->mode != LinkFourScene && param->mode != LinkNineScene)
	{
		PRV_CurDecodeMode = param->flag;		
		//printf("%s Line %d ---------> PRV_CurDecodeMode:%d\n",__func__,__LINE__,PRV_CurDecodeMode);
		RET_SUCCESS("");
	}
	PRV_CurDecodeMode = param->flag;
	
	//printf("%s Line %d ---------> PRV_CurDecodeMode:%d\n",__func__,__LINE__,PRV_CurDecodeMode);
	Achn = -1;
	IsChoosePlayAudio = 0;
	PreAudioChn = -1;
	CurAudioChn = -1;
	PRV_SetVoPreview(msg_req);

#if 0 /*֧��GUI������ͨ����ʼ��ʾ�໭�棡*/
	CHECK_RET(PRV_Chn2Index(param->dev, param->chn, &s32Index));
#else
	s32Index = param->chn;
#endif

#if 0
	if(param->flag == 1)
	{
		CHECK_RET(PRV_MultiChn(param->dev, param->mode, s32Index));
	}
	else
	{
		if (param->mode == SingleScene)
		{
			CHECK_RET(PRV_SingleChn(param->dev, param->chn));
		} 
		else
		{
			CHECK_RET(PRV_MultiChn(param->dev, param->mode, s32Index));
		}
	}
#else
	/*2010-8-24 ����Ԥ��Ϊ�� ����Ϊ���������˫�������ɻص�֮ǰ�Ķ໭�档������ͣ���ڵ������ˡ�*/
/*
	if(param->flag == 1)
	{ //�˵�����ʱ
		if (param->mode == SingleScene)
		{
			CHECK_RET(PRV_SingleChn(param->dev, s_astVoDevStatDflt[param->dev].as32ChnOrder[0]));
		} 
		else
		{
			CHECK_RET(PRV_MultiChn(param->dev, param->mode, s32Index));
		}
	}
	else
*/
#if 0
	{ //�Ҽ�ѡ��ʱ
		if (param->mode == SingleScene)
		{
			CHECK_RET(PRV_SingleChn(param->dev, param->chn));
		} 
		else
		{
			CHECK_RET(PRV_MultiChn(param->dev, param->mode, s32Index));
		}
	}
#else
#if defined(SN_SLAVE_ON)
	//������Ϣ����Ƭ
	{
		if(param->mode == LinkFourScene || param->mode == LinkNineScene)
		{
			LinkAgeGroup_ChnState LinkageGroup;
			SN_MEMSET(&LinkageGroup,0,sizeof(LinkAgeGroup_ChnState));
			Scm_GetLinkGroup(&LinkageGroup);
			SN_SendMccMessageEx(PRV_SLAVE_1,msg_req->user, msg_req->source, MOD_PRV, msg_req->xid, msg_req->thread, MSG_ID_PRV_MCC_LINKAGEGROUP_CFG_REQ, &LinkageGroup, sizeof(LinkAgeGroup_ChnState));
		}
		Prv_Slave_Layout_crtl_Req slave_req;
		SN_MEMSET(&slave_req, 0, sizeof(Prv_Slave_Layout_crtl_Req));
		int ret=0;
		slave_req.chn = param->chn;
		slave_req.enPreviewMode = param->mode;
		slave_req.dev = rsp->dev;
		slave_req.flag = param->flag;
		slave_req.reserve[0] = param->reserve;
		slave_req.reserve[1] = ScmGetListCtlState();
		s_slaveVoStat.enPreviewMode = param->mode;
		s_slaveVoStat.s32SingleIndex = param->chn;
		s_slaveVoStat.s32PreviewIndex = param->chn;
		//printf("----------Send Message: MSG_ID_PRV_MCC_LAYOUT_CTRL_REQ\n");
		ret = SN_SendMccMessageEx(PRV_SLAVE_1, msg_req->user, msg_req->source, MOD_PRV, msg_req->xid, msg_req->thread, MSG_ID_PRV_MCC_LAYOUT_CTRL_REQ, &slave_req, sizeof(Prv_Slave_Layout_crtl_Req));
		//������ʱ��
		TimerReset(s_State_Info.f_timer_handle, 15);
		TimerResume(s_State_Info.f_timer_handle, 0);
		PRV_Msg_Cpy(msg_req);
		return SN_SLAVE_MSG;
	}		
#endif

	if (param->mode== SingleScene)
	{
#if defined(SN9234H1)	
			vochn = param->chn;
#else
		if(param->reserve == 1)//���̲����л���������
			LayoutToSingleChn = param->chn;
		else
			LayoutToSingleChn = -1;
#endif		
		CHECK_RET(PRV_SingleChn(vodev, vochn));
	} 
	else
	{
		CHECK_RET(PRV_MultiChn(vodev, param->mode, s32Index));
	}
	rsp->result = 0;
#endif
#endif
	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_MSG_MCC_LayoutCtrl
Description: // �յ���Ƭ���غ�//�л����沼����Ϣ����
Calls: 
Called By: //
Input: // msg_req :�յ��Ĵ�Ƭ��Ϣ�ṹ��
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_MSG_MCC_LayoutCtrl_Rsp(const Prv_Slave_Layout_crtl_Rsp *slave_rsp,Msg_id_prv_Rsp *rsp)
{

	if (NULL == rsp || NULL == slave_rsp)
	{
		RET_FAILURE("NULL pointer!!!");
	}
	rsp->chn = slave_rsp->chn;
	rsp->dev = slave_rsp->dev;
	rsp->flag = 0;
	rsp->result = -1;
	
	//�����Ƭ���أ���ô��λ��Ƭ�ķ��ر�־λ
	s_State_Info.bIsReply |= 1<<(slave_rsp->slaveid-1);
	if(slave_rsp->result == SN_RET_OK)
	{	
		s_State_Info.g_slave_OK = 1<<(slave_rsp->slaveid-1);
	}
	//�������ֵ��1 ����1����оƬ��������ֵ����ô��ʾ���д�Ƭ��������
	if((s_State_Info.bIsReply+1) == 1<<(PRV_CHIP_NUM-1))	
	{
		s_State_Info.TimeoutCnt = HI_FALSE;//������λ��ʱ��־λ
		TimerPause(s_State_Info.f_timer_handle); //��ͣ��ʱ��
		s_State_Info.bIsReply = 0;//�ظ�״̬�˳�
		s_State_Info.bIsSlaveConfig = HI_FALSE;	//������λ���ñ�־λ
		
		if((s_State_Info.g_slave_OK+1) == 1<<(PRV_CHIP_NUM-1))
		{//������ص���Ϣ��û�д�����Ϣ
			//��Ƭ�л����洦��
			if (slave_rsp->enPreviewMode== SingleScene)
			{
				if(slave_rsp->reserve[0] == 1)//���̲����л���������
					LayoutToSingleChn = slave_rsp->chn;
				else
					LayoutToSingleChn = -1;
				CHECK_RET(PRV_SingleChn(slave_rsp->dev, slave_rsp->chn));	
			} 
			else
			{
				CHECK_RET(PRV_MultiChn(slave_rsp->dev, slave_rsp->enPreviewMode, slave_rsp->chn));
			}
			rsp->result = 0;	
		}
		s_State_Info.g_slave_OK = 0;	//���ûظ���ֵ��־λ
		if(s_State_Info.Prv_msg_Cur)
		{
			SN_FREE(s_State_Info.Prv_msg_Cur);//�ͷ�����ɵ���Ϣ�ṹ��ָ��
		}
		
	}
	RET_SUCCESS("");
}

//GUI������л�
STATIC HI_S32 PRV_MSG_OutputChange(const Output_Change_Req *param, Msg_id_prv_Rsp *rsp)
{
	unsigned char vodev = param->dev;
	if (NULL == param || NULL == rsp)
	{
		RET_FAILURE("NULL pointer!!!");
	}
	
	rsp->chn = 0;
	rsp->dev = param->dev;
	rsp->flag = 0;
	rsp->result = -1;

#if defined(Hi3520)
	if(param->dev == SPOT_VO_DEV|| param->dev == AD)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}

	if (param->dev >= PRV_VO_MAX_DEV)
	{
		RET_FAILURE("Invalid VoDev Id!");
	}
	if(s_VoSecondDev == AD)
	{
		if(param->dev == SD)
		{
			RET_FAILURE("Invalid VoDev Id!");
		}
	}
	vodev = (param->dev == HD) ? HD:s_VoSecondDev;
	
#else
	if(param->dev > DHD0)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}

	if (param->dev > DHD0)
	//if (param->dev >= PRV_VO_MAX_DEV)
	{
		RET_FAILURE("Invalid VoDev Id!");
	}
#if defined(Hi3531)
	if(s_VoSecondDev == DSD0)
	{
		if(param->dev == DSD1)
		{
			RET_FAILURE("Invalid VoDev Id!");
		}
	}
#endif
	vodev = (param->dev == DHD0) ? DHD0 : s_VoSecondDev;
#endif	
	
	if (s_VoDevCtrlDflt != vodev)
	{
		//�л�GUI����豸��ֱ���
		s_VoDevCtrlDflt = vodev;
		s_u32GuiWidthDflt = s_astVoDevStatDflt[s_VoDevCtrlDflt].stVideoLayerAttr.stDispRect.u32Width;
		s_u32GuiHeightDflt = s_astVoDevStatDflt[s_VoDevCtrlDflt].stVideoLayerAttr.stDispRect.u32Height;
		MMICloseFB(s_devfb[G4],s_u32GuiWidthDflt,s_u32GuiHeightDflt - s_VoDevCtrlDflt, 16);
		PRV_BindGuiVo(s_VoDevCtrlDflt);
		MMIOpenFB(s_devfb[G4],s_u32GuiWidthDflt,s_u32GuiHeightDflt - s_VoDevCtrlDflt, 16);
	}
	PRINT_YELLOW("This message is NOT recommended for changing GUI!");
	rsp->result = 0;
	RET_SUCCESS("");
}

//����ͼ��/¼��ͼ����ʾ
STATIC HI_S32 PRV_MSG_ChnIconCtrl(const Chn_icon_ctrl_Req *param, Msg_id_prv_Rsp *rsp)
{
	HI_S32 s32Ret=-1;
	if (NULL == param || NULL == rsp)
	{
		RET_FAILURE("NULL pointer!!!");
	}
	
	rsp->chn = param->chn;
	rsp->dev = 0;
	rsp->flag = 0;
	rsp->result = -1;
#if 0
	
	//TODO
	if(param->icon == 1)
	{
		s32Ret = OSD_Ctl(param->chn,param->bshow,OSD_ALARM_TYPE);
		if(s32Ret != HI_SUCCESS)
		{
			TRACE(SCI_TRACE_HIGH, MOD_PRV,"PRV_MSG_ChnIconCtrl OSD_Ctl faild 0x%x!\n",s32Ret);
			rsp->result = -1;
			RET_FAILURE("");
		}
	}
	else if(param->icon == 2)
	{
		s32Ret = OSD_Ctl(param->chn,param->bshow,OSD_REC_TYPE);
		if(s32Ret != HI_SUCCESS)
		{
			TRACE(SCI_TRACE_HIGH, MOD_PRV,"PRV_MSG_ChnIconCtrl OSD_Ctl faild 0x%x!\n",s32Ret);
			rsp->result = -1;
			RET_FAILURE("");
		}
	}
#else
	s32Ret = OSD_Ctl(param->chn,param->bshow,param->icon);
	//TRACE(SCI_TRACE_NORMAL, MOD_PRV,"PRV_MSG_ChnIconCtrl OSD_Ctl param->icon 0x%x, param->bshow=%d, param->chn = %d!\n",param->icon, param->bshow, param->chn);

	if(s32Ret != HI_SUCCESS)
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV,"PRV_MSG_ChnIconCtrl OSD_Ctl faild 0x%x!\n",s32Ret);
		rsp->result = -1;
		RET_FAILURE("");
	}
#endif	
	rsp->result = 0;
	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_MSG_EnterChnCtrl
Description: ��Ƭ����ͨ�����ƴ�����Ϣ����
Calls: 
Called By: //
Input: // msg_req :�յ�����Ϣ�ṹ��
		rsp : �ظ���GUI����Ϣ
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_MSG_EnterChnCtrl(const SN_MSG *msg_req, Msg_id_prv_Rsp *rsp)
{
	VO_CHN VoChn = 0;
	HI_U32 s32Index= 0;

	Enter_chn_ctrl_Req *param = (Enter_chn_ctrl_Req *)msg_req->para;
	
	if (NULL == param || NULL == rsp)
	{
		RET_FAILURE("NULL pointer!!!");
	}
	//printf("---------------flag: %d,param->chn: %d\n", param->flag, param->chn);
	rsp->chn = param->chn;
	rsp->dev = 0;
	rsp->flag = param->flag;
	rsp->result = -1;
	TRACE(SCI_TRACE_NORMAL, MOD_PRV, "######PRV_MSG_EnterChnCtrl s32Flag = %d s_astVoDevStatDflt[VoDev].enPreviewStat = %d, param->chn: %d###################\n",param->flag,s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat, param->chn);
	switch (param->flag)
	{
		case 1://��Ϣ��Դ��1-��������ѡ��2-���ӷŴ�3-��̨���ƣ�4-�л����طţ�5-����������6-���˫��;7-�ڸ�����;8-����������Ʋ�����OSD,9-������Ƶ��ʾ�������ý���
		case 8:
		case 3:
		case 5:
		case 7:
			VoChn = param->chn;//1�� ��Ϣ��ԴΪ1(��������ѡ��)��5(��������)ʱ������ָ����Ч��ͨ����chn��
			break;
		case 9:
			//���滭�л�����ʼλ��
			s_astVoDevStatDflt[s_VoDevCtrlDflt].Pip_rect.s32X = param->Pip_x;
			s_astVoDevStatDflt[s_VoDevCtrlDflt].Pip_rect.s32Y= param->Pip_y;
			s_astVoDevStatDflt[s_VoDevCtrlDflt].Pip_rect.u32Width= param->Pip_w;
			s_astVoDevStatDflt[s_VoDevCtrlDflt].Pip_rect.u32Height= param->Pip_h;
			VoChn = param->chn;//1�� ��Ϣ��ԴΪ1(��������ѡ��)��5(��������)ʱ������ָ����Ч��ͨ����chn��
			break;
		case 2:
		case 6:	
		{
			//printf("param->flag: %d, param->mouse_pos.x: %d, param->mouse_pos.x: %d\n", param->flag, param->mouse_pos.x, param->mouse_pos.y);
			{
				if(s_astVoDevStatDflt[s_VoDevCtrlDflt].bIsAlarm)
				{
					VoChn = s_astVoDevStatDflt[s_VoDevCtrlDflt].s32AlarmChn;
				}
				else
				{
					CHECK_RET(PRV_Point2Index(&param->mouse_pos, &s32Index,s_astVoDevStatDflt[s_VoDevCtrlDflt].as32ChnOrder[s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewMode]));//2�� ��Ϣ��ԴΪ2(���ӷŴ�)��3(��̨����)��4(�л����ط�)��6(���˫��)ʱ������ָ����Ч�����λ��mouse_pos��
					VoChn = s_astVoDevStatDflt[s_VoDevCtrlDflt].as32ChnOrder[s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewMode][s32Index];
					if(PRV_CurDecodeMode == PassiveDecode || ScmGetListCtlState() == 1)
					{
						VoChn = s32Index;
						break;
					}
					if(LayoutToSingleChn >= 0)
						VoChn = LayoutToSingleChn;
				}
			}
			//printf("----------VoChn: %d, mouse_pos X: %d, Y: %d\n", VoChn, param->mouse_pos.x, param->mouse_pos.y);
		}	
			break;
		case 4:
		case SLC_CTL_FLAG:
		case PIC_CTL_FLAG:
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Master Enter PB Control, param->chn = %d!\n", param->chn);
			VoChn = param->chn;
			break;
		default:
			RET_FAILURE("param->flag out off range");
	}
	//������ӷŴ���ʾ��������ͨ��ֹͣ��ѯ
	if(param->flag == 2 || param->flag == 9)//���ӷŴ�ͽ�����ʾ����������⣬����/�˳�������ͬһ����Ϣ
	{
		ScmChnCtrlReq(1);
		//int EnterChnCtl = 1;
		//SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_SCM, 0, 0, MSG_ID_SCM_CHNCTRL_IND, &EnterChnCtl, sizeof(EnterChnCtl));						
	}
#if defined(SN_SLAVE_ON)	
	{
		Prv_Slave_Enter_chn_ctrl_Req slave_req;
		//����ͨ��������Ϣ����Ƭ
		slave_req.chn = VoChn;
		slave_req.flag = param->flag;
		slave_req.mouse_pos = param->mouse_pos;
		SN_SendMccMessageEx(PRV_SLAVE_1, msg_req->user, msg_req->source, MOD_PRV, msg_req->xid, msg_req->thread, MSG_ID_PRV_MCC_ENTER_CHN_CTRL_REQ, &slave_req, sizeof(slave_req));
		//SN_SendMccMessageEx(VochnInfo[index].SlaveId,msg_req->user, MOD_PRV, MOD_DEC, 0, 0,  MSG_ID_DEC_ZOOM_REQ, &slave_req, sizeof(slave_req));

		//������ʱ��
		TimerReset(s_State_Info.f_timer_handle, 15);
		TimerResume(s_State_Info.f_timer_handle, 0);
		
		PRV_Msg_Cpy(msg_req);		//���浱ǰ��Ϣ���Ա������Ϣ�ط�
		return SN_SLAVE_MSG;
	}
#endif

	switch(param->flag)
	{
		case 1:
		case 8:
		case 2:
		case 3:
		case 7:		
		case 9:	
			CHECK_RET(PRV_EnterChnCtrl(s_VoDevCtrlDflt, VoChn, param->flag));
			break;
		case 4:
		case SLC_CTL_FLAG:
		case PIC_CTL_FLAG:	
			CHECK_RET(PRV_EnterPB(s_VoDevCtrlDflt,param->flag));
			break;
		case 5:
			//printf(TEXT_COLOR_PURPLE("Alarm!!! dev:%d, chn:%d\n"),s_VoDevAlarmDflt,VoChn);
			CHECK_RET(PRV_AlarmChn(s_VoDevAlarmDflt, VoChn));
			//��ɺ���Ҫ֪ͨGUI�л������
			SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_MMI, msg_req->xid, msg_req->thread, 
							MSG_ID_PRV_SCREEN_CTRL_IND, NULL, 0);
			break;
		case 6:
			CHECK_RET(PRV_DoubleClick(&param->mouse_pos));
			break;
		default:
			RET_FAILURE("unknown param->flag");
	}

	rsp->result = 0;
	RET_SUCCESS("");

}

/*************************************************
Function: //PRV_MSG_MCC_EnterChnCtrl_Rsp
Description: �յ���Ƭ����ͨ��������Ϣ�������
Calls: 
Called By: //
Input: // msg_req :��Ƭ��Ϣ�ṹ��
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_MSG_MCC_EnterChnCtrl_Rsp(const Prv_Slave_Enter_chn_ctrl_Rsp *param,Msg_id_prv_Rsp *rsp)
{
	VO_CHN VoChn = 0;
	HI_S32 s32Ret = 0;
	if (NULL == param || NULL == rsp)
	{
		RET_FAILURE("NULL pointer!!!");
	}
	rsp->chn = param->chn;
	rsp->dev = 0;
	rsp->flag = param->flag;
	rsp->result = -1; 		
	//�����Ƭ���أ���ô��λ��Ƭ�ķ��ر�־λ
	s_State_Info.bIsReply |= 1<<(param->slaveid-1);
	
	if(param->result == SN_RET_OK)
	{	//�����Ƭ������ȷֵ
		s_State_Info.g_slave_OK = 1<<(param->slaveid-1);
	}
	//�������ֵ��1 ����1����оƬ��������ֵ����ô��ʾ���д�Ƭ��������
	if((s_State_Info.bIsReply+1) == 1<<(PRV_CHIP_NUM-1))	
	{
		if((s_State_Info.g_slave_OK+1) == 1<<(PRV_CHIP_NUM-1))
		{//������ص���Ϣ��û�д�����Ϣ
			VoChn = param->chn;
			switch(param->flag)
			{
				case 1:
				case 8:
				case 2:
				case 3: 
				case 7:	
				case 9:
				{
					//��״̬λ��λ�Ƶ�ǰ�棬
					//��ֹ��Ϣ�Ѿ����أ�����Ϊ��Ӧ����Ľӿ������������´���ʱ���������������Ϣ��ʱ
					s_State_Info.TimeoutCnt = HI_FALSE;//������λ��ʱ��־λ
					TimerPause(s_State_Info.f_timer_handle); //��ͣ��ʱ��
					s_State_Info.bIsReply = 0;//�ظ�״̬�˳�
					s_State_Info.bIsSlaveConfig = HI_FALSE;	//������λ���ñ�־λ
					s_State_Info.g_slave_OK = 0;	//���ûظ���ֵ��־λ
					CHECK_RET(PRV_EnterChnCtrl(s_VoDevCtrlDflt, VoChn, param->flag));
				}
					break;
				case 4:
				case SLC_CTL_FLAG:
				case PIC_CTL_FLAG:
					{
						s_State_Info.TimeoutCnt = HI_FALSE;//������λ��ʱ��־λ
						TimerPause(s_State_Info.f_timer_handle); //��ͣ��ʱ��
						s_State_Info.bIsReply = 0;//�ظ�״̬�˳�
						s_State_Info.bIsSlaveConfig = HI_FALSE; //������λ���ñ�־λ
						s_State_Info.g_slave_OK = 0;	//���ûظ���ֵ��־λ
						
						s32Ret = HostCleanAllHostToSlaveStream(MasterToSlaveChnId, 0, 1);
						if (s32Ret != HI_SUCCESS)
						{
							TRACE(SCI_TRACE_NORMAL, MOD_PRV, "slave %d HostCleanAllHostToSlaveStream error", MasterToSlaveChnId);
						}
#if defined(Hi3531)||defined(Hi3535)				
						s32Ret = HostStopHostToSlaveStream(MasterToSlaveChnId);
						if (s32Ret != HI_SUCCESS)
						{
							TRACE(SCI_TRACE_NORMAL, MOD_PRV, "slave %d HostStopHostToSlaveStream error", MasterToSlaveChnId);
						}
						else
						{
							MasterToSlaveChnId = 0;
						}	
#endif						
					}
					CHECK_RET(PRV_EnterPB(s_VoDevCtrlDflt, param->flag));
					break;
				case 5:
					{
						s_State_Info.TimeoutCnt = HI_FALSE;//������λ��ʱ��־λ
						TimerPause(s_State_Info.f_timer_handle); //��ͣ��ʱ��
						s_State_Info.bIsReply = 0;//�ظ�״̬�˳�
						s_State_Info.bIsSlaveConfig = HI_FALSE;	//������λ���ñ�־λ
						s_State_Info.g_slave_OK = 0;	//���ûظ���ֵ��־λ
						//printf(TEXT_COLOR_PURPLE("Alarm!!! dev:%d, chn:%d\n"),s_VoDevAlarmDflt,VoChn);
						CHECK_RET(PRV_AlarmChn(s_VoDevAlarmDflt, VoChn));
						//��ɺ���Ҫ֪ͨGUI�л������
						SN_SendMessageEx(s_State_Info.Prv_msg_Cur->user, MOD_PRV, MOD_MMI, s_State_Info.Prv_msg_Cur->xid, s_State_Info.Prv_msg_Cur->thread, 
								MSG_ID_PRV_SCREEN_CTRL_IND, NULL, 0);
					}
					break;
				case 6:
					{
						s_State_Info.TimeoutCnt = HI_FALSE;//������λ��ʱ��־λ
						TimerPause(s_State_Info.f_timer_handle); //��ͣ��ʱ��
						s_State_Info.bIsReply = 0;//�ظ�״̬�˳�
						s_State_Info.bIsSlaveConfig = HI_FALSE;	//������λ���ñ�־λ
						s_State_Info.g_slave_OK = 0;	//���ûظ���ֵ��־λ
						CHECK_RET(PRV_DoubleClick(&param->mouse_pos));
					}
					break;
				default:
					RET_FAILURE("unknown param->flag");
			}
			rsp->result = 0;
		}

		if(s_State_Info.Prv_msg_Cur)
		{
			SN_FREE(s_State_Info.Prv_msg_Cur);//�ͷ�����ɵ���Ϣ�ṹ��ָ��
		}
	}
	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_MSG_ExitChnCtrl
Description: //��Ƭ�˳�ͨ��������Ϣ����
Calls: 
Called By: //
Input: // msg_req :�յ�����Ϣ�ṹ��
		rsp : �ظ���GUI����Ϣ
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_MSG_ExitChnCtrl(const SN_MSG *msg_req, Msg_id_prv_Rsp *rsp)
{
	Exit_chn_ctrl_Req *param = (Exit_chn_ctrl_Req *)msg_req->para;
	
	if ((NULL == param) || (NULL == rsp))
	{
		RET_FAILURE("NULL pointer!!!");
	}
	
	rsp->chn = 0;
	rsp->dev = 0;
	rsp->flag = param->flag;
	rsp->result = -1;
#if defined(SN_SLAVE_ON)	
	{
		Prv_Slave_Exit_chn_ctrl_Req slave_req;
		//����ͨ��������Ϣ����Ƭ
		slave_req.flag = param->flag;
		SN_SendMccMessageEx(PRV_SLAVE_1, msg_req->user, msg_req->source, MOD_PRV, msg_req->xid, msg_req->thread, MSG_ID_PRV_MCC_EXIT_CHN_CTRL_REQ, &slave_req, sizeof(slave_req));
		//������ʱ��
		TimerReset(s_State_Info.f_timer_handle, 15);
		TimerResume(s_State_Info.f_timer_handle, 0);
		PRV_Msg_Cpy(msg_req);		//���浱ǰ��Ϣ���Ա������Ϣ�ط�
		return SN_SLAVE_MSG;
	}
#endif
	switch (param->flag)
	{
	case 1://��Ϣ��Դ��1-��������ѡ��2-���ӷŴ�3-��̨���ƣ�4-�л����طţ�5-����������6-���˫��;7-�ڸ�����;8-����������Ʋ�����OSD,9-������Ƶ��ʾ�������ý���
	case 8:
	case 2:
	case 3:
	case 7:	
	case 9:	
			CHECK_RET(PRV_ExitChnCtrl(param->flag));
			break;
	case 4:
		case SLC_CTL_FLAG:
		case PIC_CTL_FLAG:
			//printf("--------Master Exit PB\n");
			CHECK_RET(PRV_ExitPB(s_VoDevCtrlDflt,param->flag));
			break;
	case 5:
			CHECK_RET(PRV_AlarmOff(s_VoDevAlarmDflt));
			//��ɺ���Ҫ֪ͨGUI�л������
			SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_MMI, msg_req->xid, msg_req->thread, 
								MSG_ID_PRV_SCREEN_CTRL_IND, NULL, 0);
			break;
	case 6:
			RET_FAILURE("double click should not use this message!!!");
		default:
			RET_FAILURE("param->flag out off range");
	}
	//�˳����ӷŴ���ʾ��������ͨ����ʼ��ѯ
	if(param->flag == 2 || param->flag == 9)
	{
		ScmChnCtrlReq(0);
		//int ExitChnCtl = 0;
		//SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_SCM, 0, 0, MSG_ID_SCM_CHNCTRL_IND, &ExitChnCtl, sizeof(ExitChnCtl));	
	}
	rsp->result = 0;
	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_MSG_MCC_ExitChnCtrl_Rsp
Description: //��Ƭ���غ��˳�ͨ��������Ϣ����
Calls: 
Called By: //
Input: // msg_req :�յ��Ĵ�Ƭ��Ϣ�ṹ��
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 PRV_MSG_MCC_ExitChnCtrl_Rsp(const Prv_Slave_Exit_chn_ctrl_Rsp *slave_rsp, Msg_id_prv_Rsp *rsp)
{
//	 int i = 0;
	 if (NULL == rsp || NULL == slave_rsp)
	 {
		 RET_FAILURE("NULL pointer!!!");
	 }
	 
	 rsp->chn = 0;
	 rsp->dev = 0;
	 rsp->flag = slave_rsp->flag;
	 rsp->result = -1;
	 
	//�����Ƭ���أ���ô��λ��Ƭ�ķ��ر�־λ
	s_State_Info.bIsReply |= 1<<(slave_rsp->slaveid-1);
	
	if(slave_rsp->result == SN_RET_OK)
	{	//�����Ƭ������ȷֵ
		s_State_Info.g_slave_OK = 1<<(slave_rsp->slaveid-1);
	}
	//�������ֵ��1 ����1����оƬ��������ֵ����ô��ʾ���д�Ƭ��������
	if((s_State_Info.bIsReply+1) == 1<<(PRV_CHIP_NUM-1))	
	{
		if((s_State_Info.g_slave_OK+1) == 1<<(PRV_CHIP_NUM-1))
		{//������ص���Ϣ��û�д�����Ϣ
			switch (slave_rsp->flag)
			{
			case 1://��Ϣ��Դ��1-��������ѡ��2-���ӷŴ�3-��̨���ƣ�4-�л����طţ�5-����������6-���˫��;7-�ڸ�����;8-����������Ʋ�����OSD,9-������Ƶ��ʾ�������ý���
			case 8:
			case 2:
			case 3:
			case 7:	
			case 9:	
				{
					s_State_Info.TimeoutCnt = HI_FALSE;//������λ��ʱ��־λ
					TimerPause(s_State_Info.f_timer_handle); //��ͣ��ʱ��
					s_State_Info.bIsReply = 0;//�ظ�״̬�˳�
					s_State_Info.bIsSlaveConfig = HI_FALSE;	//������λ���ñ�־λ
					s_State_Info.g_slave_OK = 0;	//���ûظ���ֵ��־λ
					CHECK_RET(PRV_ExitChnCtrl(slave_rsp->flag));
				}
					break;
			case 4:
				case SLC_CTL_FLAG:
				case PIC_CTL_FLAG:
				{
					s_State_Info.TimeoutCnt = HI_FALSE;//������λ��ʱ��־λ
					TimerPause(s_State_Info.f_timer_handle); //��ͣ��ʱ��
					s_State_Info.bIsReply = 0;//�ظ�״̬�˳�
					s_State_Info.bIsSlaveConfig = HI_FALSE;	//������λ���ñ�־λ
					s_State_Info.g_slave_OK = 0;	//���ûظ���ֵ��־λ
#if defined(Hi3531)||defined(Hi3535)					
					PRV_InitHostToSlaveStream();
#endif
						
					CHECK_RET(PRV_ExitPB(s_VoDevCtrlDflt,slave_rsp->flag));
				}
					break;
			case 5:
				{
					s_State_Info.TimeoutCnt = HI_FALSE;//������λ��ʱ��־λ
					TimerPause(s_State_Info.f_timer_handle); //��ͣ��ʱ��
					s_State_Info.bIsReply = 0;//�ظ�״̬�˳�
					s_State_Info.bIsSlaveConfig = HI_FALSE;	//������λ���ñ�־λ
					s_State_Info.g_slave_OK = 0;	//���ûظ���ֵ��־λ
					CHECK_RET(PRV_AlarmOff(s_VoDevAlarmDflt));
				}
					break;
			case 6:
					RET_FAILURE("double click should not use this message!!!");
				default:
					RET_FAILURE("param->flag out off range");
			}
			rsp->result = 0;
		}

		if(s_State_Info.Prv_msg_Cur)
		{
			SN_FREE(s_State_Info.Prv_msg_Cur);//�ͷ�����ɵ���Ϣ�ṹ��ָ��
		}
		//�˳����ӷŴ���ʾ��������ͨ����ʼ��ѯ
		if(slave_rsp->flag == 2 || slave_rsp->flag == 9)
		{
			int ExitChnCtl = 0;
			SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_SCM, 0, 0, MSG_ID_SCM_CHNCTRL_IND, &ExitChnCtl, sizeof(ExitChnCtl));	
		}
	}
	RET_SUCCESS("");
}

/**********************************************************************
* �������ƣ�   MPRV_OSD_SetGuiAlpha
* ����������   UI�ڽ�������ѡ��ʱ��Ҫ�������͸��
* ���������   unsigned char alpha ͸���� 0-255
* ���������   ��
* �� �� ֵ��    MMI_ERR_OK -�ɹ�������-ʧ��
* ��    ��:  ��
***********************************************************************/
int PRV_SetGuiAlpha (int b_isnew, int Transparency)
{
	int ret = ERROR;
	UINT8 alpha = 0;
	PRM_GENERAL_CFG_ADV  stGenPrm;

	if(b_isnew)
	{
		alpha = 0xFF - Transparency * 25;
	}
	else
	{
		if (PARAM_OK != GetParameter (PRM_ID_GENERAL_CFG_ADV, NULL, &stGenPrm, sizeof (stGenPrm), 1, SUPER_USER_ID, NULL))
		{
			return ret;
		}

		alpha = 0xFF - stGenPrm.MenuTransparency * 25;
	}
	
#if(DEV_TYPE == DEV_SN_9234_H_1)
	ret = PRV_OSD_SetGuiAlpha (alpha);
#elif defined(Hi3531)||defined(Hi3535)	
	ret = FBSetAlphaKey (alpha, TRUE);
#endif	
	
	return ret;
}

/*************************************************
Function: //PRV_MSG_SetGeneralCfg
Description: ͨ��������Ϣ�������
Calls: 
Called By: //
Input: // // msg_req:ͨ��������Ϣ
			rsp:����GUI��RSP
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: //��N\P��ʽ�ı��ʱ����Ҫ֪ͨ��Ƭ����ʱ�������Ĳ������ȵ���Ƭ���غ���
************************************************************************/
STATIC HI_S32 PRV_MSG_SetGeneralCfg(const SN_MSG *msg_req,Set_general_cfg_Rsp *rsp)
{
	VO_INTF_SYNC_E enIntfSync,old_enIntfSync;		/*VGA�ֱ���*/
	VO_DEV VoDev;					/*GUI����豸*/
	HI_S32 s32NPFlag=0,old_s32NPFlag=0;				/*NP��ʽ*/
	Set_general_cfg_Req *param = (Set_general_cfg_Req *)msg_req->para;
	int Changeflag=0,osd_flag=0,ret=0;			//�仯��־λ����0λ��ʾN\P��ʽ�仯����1λ��ʾ�ֱ��ʱ仯����2λ��ʾ����豸�仯
	
	//char gui_flag = 0;

	if (NULL == param || NULL == rsp)
	{
		RET_FAILURE("NULL pointer!!!");
	}
	rsp->result = -1;
	rsp->general_info = param->general_info;

	/*NP��ʽ*/
	s32NPFlag = (0 == param->general_info.CVBSOutputType) ? 1 : 0;
	
	old_s32NPFlag = s_s32NPFlagDflt;
#if defined(SN9234H1)
	old_enIntfSync = s_astVoDevStatDflt[HD].stVoPubAttr.enIntfSync;
#else
	old_enIntfSync = s_astVoDevStatDflt[DHD0].stVoPubAttr.enIntfSync;
#endif	
	TRACE(SCI_TRACE_NORMAL, MOD_PRV, "%d  , %d  ,%d ,  %d,  %d\n", param->general_info.CVBSOutputType,param->general_info.Language,param->general_info.OutputSel,
						param->general_info.OutPutType,param->general_info.VGAResolution);

	/*VGA�ֱ���*/
	
	switch(param->general_info.VGAResolution)
	{
		case VGA_1080P:
#if defined(Hi3531)||defined(Hi3535)			
			enIntfSync = VO_OUTPUT_1080P60;
			break;
#endif
			
			
		case VGA_720P:
#if defined(Hi3531)||defined(Hi3535)			
			enIntfSync = VO_OUTPUT_720P60;
			break;
#endif
			
			
		case VGA_1024X768:
#if defined(SN9234H1)
			enIntfSync = VO_OUTPUT_720P60;
#else
			enIntfSync = VO_OUTPUT_1024x768_60;
#endif
			break;
		
		case VGA_1280X1024:
#if defined(SN9234H1)
			enIntfSync = VO_OUTPUT_720P60;
#else
			enIntfSync = VO_OUTPUT_1280x1024_60;
#endif
			break;
		case VGA_800X600:
			enIntfSync = VO_OUTPUT_800x600_60;
			break;
		case VGA_1366x768:
			enIntfSync = VO_OUTPUT_1366x768_60;
			break;
		case VGA_1440x900:
			enIntfSync = VO_OUTPUT_1440x900_60;
			break;
		default:
			RET_FAILURE("Unsupport VGA Resolution");
	}
	TRACE(SCI_TRACE_NORMAL, MOD_PRV, "=============enIntfSync: %d\n", enIntfSync);
#if defined(SN_SLAVE_ON)
		if(s_State_Info.bslave_IsInit == HI_TRUE)
		{
		//��Ƭ�Ѿ���ʼ��
#if defined(SN6104) || defined(SN8604D) || defined(SN8604M) || defined(SN6108) || defined(SN8608D) || defined(SN8608M)
			if ((enIntfSync != s_astVoDevStatDflt[DHD0].stVoPubAttr.enIntfSync) || (s32NPFlag != s_s32NPFlagDflt))
#else
			if ((param->general_info.VGAResolution != s_s32VGAResolution) || (s32NPFlag != s_s32NPFlagDflt))
#endif
			{
			//����ֱ��ʸı䡣��ô��Ҫ֪ͨ��Ƭ,���������ȴ�Ƭ���غ����
			
				Prv_Slave_Set_general_cfg_Req slave_req;
				//printf("###########PRV_MSG_SetGeneralCfg Send msg !######################\n");
				//�ѵ�ǰ����Ϣ��Ϣ������Ƭ����Ƭ��Ҫ����
				slave_req.general_info = param->general_info;
				//��Ҫ����¼��ģ�鷵�ص�����
				slave_req.vam_result = param->reserve[0];
				//������һ���������Ƭ
				SN_SendMccMessageEx(PRV_SLAVE_1,msg_req->user, msg_req->source, MOD_PRV, msg_req->xid, msg_req->thread, MSG_ID_PRV_MCC_SET_GENERAL_CFG_REQ, &slave_req, sizeof(slave_req));
				//������ʱ��
				TimerReset(s_State_Info.f_timer_handle,15); 
				TimerResume(s_State_Info.f_timer_handle,0);
				PRV_Msg_Cpy(msg_req);	//���浱ǰ��Ϣ����
				return SN_SLAVE_MSG;
			}
		}
#endif

	/*GUI����豸*/
	switch(param->general_info.OutputSel)
	{
#if defined(SN9234H1)
		case 0:
			VoDev = HD;
			break;
		case 1:
			VoDev = s_VoSecondDev;
			break;
		case 2:
			VoDev = SD;
			RET_FAILURE("SD is not supported for Gui!");
		default :
			RET_FAILURE("general_info.OutputSel is out off range!");
#else
		case 0:
			VoDev = DHD0;
			break;
		case 1:
			VoDev = s_VoSecondDev;
			break;
		case 2:
			VoDev = DSD0;
		default :
			RET_FAILURE("general_info.OutputSel is out off range!");
#endif			
	}
    pthread_mutex_lock(&s_osd_mutex);

//step1:
	//step1: CVBS�����ʽ
	if (s32NPFlag != s_s32NPFlagDflt)
	{
		//Changeflag |= 1<<0; 
	}
#if defined(SN9234H1)
	if( (enIntfSync != s_astVoDevStatDflt[HD].stVoPubAttr.enIntfSync) || (param->general_info.VGAResolution != s_s32VGAResolution))
	{
		Changeflag |= 1<<1; 
		s_s32VGAResolution = param->general_info.VGAResolution;
	}
#else
	if( (enIntfSync != s_astVoDevStatDflt[DHD0].stVoPubAttr.enIntfSync) || (param->general_info.VGAResolution != s_s32VGAResolution))
	{
		Changeflag |= 1<<1; 
		s_s32VGAResolution = param->general_info.VGAResolution;
	}
#endif
	if (s_VoDevCtrlDflt != VoDev)
	{
		Changeflag |= 1<<2; 
	}
#if defined(SN_SLAVE_ON)

	if(s_State_Info.bslave_IsInit == HI_FALSE)
		Changeflag = 2;
#endif
	TRACE(SCI_TRACE_NORMAL, MOD_PRV, "==========Changeflag: %d\n", Changeflag);
	switch(Changeflag)
	{
		case 1://ֻ��N\P��ʽ�仯
		case 3://N\P��ʽ�仯���ֱ��ʱ仯
		case 5://N\P��ʽ�仯������豸�仯
		case 7://N\P��ʽ�仯���ֱ��ʱ仯������豸�仯
		{
			s_s32NPFlagDflt = s32NPFlag;
			s_State_Info.bIsRe_Init = 0;
			if((s_State_Info.bIsOsd_Init))
			{
#if defined(SN9234H1)
				Prv_OSD_Close_fb(HD);
				Prv_OSD_Close_fb(AD);		
				osd_flag = 1;
#else
				Prv_OSD_Close_fb(DHD0);
				//Prv_OSD_Close_fb(DSD0);
				osd_flag = 1;
#endif				
			}
			PRV_CloseVoFb(s_VoDevCtrlDflt);

#if defined(SN9234H1)
			//���ͷ����а󶨹�ϵ
			PRV_ViUnBindAllVoChn(VoDev);
			PRV_VdecUnBindAllVoChn1(VoDev);
			
			PRV_ViUnBindAllVoChn(s_VoSecondDev);
			PRV_VdecUnBindAllVoChn1(s_VoSecondDev);

			//VI���³�ʼ��
			ret = PRV_ViInit();
			if(ret == HI_FAILURE)
			{//������ش���,�ָ��ɵ����ã�����ʧ��
				goto Falied;
			}
			//VO���³�ʼ��
			s_astVoDevStatDflt[HD].stVoPubAttr.enIntfSync = enIntfSync;
			ret = PRV_ResetVo(HD);
			if(ret == HI_FAILURE)
			{//������ش���,�ָ��ɵ����ã�����ʧ��
				goto Falied;
			}
			ret = PRV_ResetVo(s_VoSecondDev);
			if(ret == HI_FAILURE)
			{//������ش���,�ָ��ɵ����ã�����ʧ��
				goto Falied;
			}

#else
			//���ͷ����а󶨹�ϵ
			//PRV_ViUnBindAllVoChn(VoDev);
			//PRV_VdecUnBindAllVoChn(VoDev);
			
		//	PRV_ViUnBindAllVoChn(s_VoSecondDev);
			//PRV_VdecUnBindAllVoChn(s_VoSecondDev);
			//VI���³�ʼ��
			//ret = PRV_ViInit();
			if(ret == HI_FAILURE)
			{//������ش���,�ָ��ɵ����ã�����ʧ��
				goto Falied;
			}
			//VO���³�ʼ��
			s_astVoDevStatDflt[DHD0].stVoPubAttr.enIntfSync = enIntfSync;
			ret = PRV_ResetVo(DHD0);
			if(ret == HI_FAILURE)
			{//������ش���,�ָ��ɵ����ã�����ʧ��
				goto Falied;
			}
			//ret = PRV_ResetVo(s_VoSecondDev);
			//if(ret == HI_FAILURE)
			{//������ش���,�ָ��ɵ����ã�����ʧ��
			//	goto Falied;
			}
#endif			
			s_VoDevCtrlDflt = VoDev;			
			PRV_OpenVoFb(s_VoDevCtrlDflt);
			
			if(osd_flag)
			{
				OSD_Set_Rec_Range_NP(s_s32NPFlagDflt);
#if defined(SN9234H1)
				Prv_OSD_Open_fb( HD);
				Prv_OSD_Open_fb( AD);
#else
				Prv_OSD_Open_fb( DHD0);
				//Prv_OSD_Open_fb( DSD0);
#endif				
			}
			s_State_Info.bIsRe_Init = 1;
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "step1: N/P mode changed! N-0,P-1: %d", s_s32NPFlagDflt);
			break;
Falied:		

#if defined(SN9234H1)	
			s_s32NPFlagDflt = old_s32NPFlag;
			PRV_ViInit();
			s_s32NPFlagDflt = old_enIntfSync;
			PRV_ResetVoDev(HD);
			PRV_ResetVoDev(s_VoSecondDev);
			PRV_OpenVoFb(s_VoDevCtrlDflt);
			
			if(osd_flag)
			{
				OSD_Set_Rec_Range_NP(s_s32NPFlagDflt);
				Prv_OSD_Open_fb( HD);
				Prv_OSD_Open_fb( AD);
			}
#else
			s_s32NPFlagDflt = old_s32NPFlag;
////			PRV_ViInit();
			s_s32NPFlagDflt = old_enIntfSync;
			PRV_ResetVoDev(DHD0);
			//PRV_ResetVoDev(s_VoSecondDev);
			PRV_OpenVoFb(s_VoDevCtrlDflt);
			
			if(osd_flag)
			{
				OSD_Set_Rec_Range_NP(s_s32NPFlagDflt);
				Prv_OSD_Open_fb( DHD0);
				//Prv_OSD_Open_fb( DSD0);
			}
#endif
			
			s_State_Info.bIsRe_Init = 1;
            pthread_mutex_unlock(&s_osd_mutex);
			RET_FAILURE("PRV_Vireset or PRV_Voreset falied!\n");
		}
		case 2://ֻ�зֱ��ʱ仯
		{
#if defined(SN9234H1)
			s_astVoDevStatDflt[HD].stVoPubAttr.enIntfSync = enIntfSync;
			
			//printf("##########s_astVoDevStatDflt[HD]   s_VoDevCtrlDflt = %d,VoDev = %d################\n",s_VoDevCtrlDflt,VoDev);
			s_State_Info.bIsRe_Init = 0;
			if((s_State_Info.bIsOsd_Init))
			{
				Prv_OSD_Close_fb(HD);
				osd_flag = 1;
			}
			if(s_VoDevCtrlDflt == HD)
			{//�����ǰ����豸��HD����ô��Ҫ�ر�GUI
				PRV_CloseVoFb(s_VoDevCtrlDflt);
			}
			
			ret = PRV_ResetVoDev(HD);
			if(ret == HI_FAILURE)
			{//������ش���,�ָ��ɵ����ã�����ʧ��
				s_s32NPFlagDflt = old_enIntfSync;
				PRV_ResetVoDev(HD);
			}
			
			if(s_VoDevCtrlDflt == HD)
			{//�����ǰ����豸��HD����ô��Ҫ����GUI
				PRV_OpenVoFb(s_VoDevCtrlDflt);
			}
			if(osd_flag)
			{
				OSD_Set_Rec_Range_NP(s_s32NPFlagDflt);
				Prv_OSD_Open_fb( HD);
			}
			s_State_Info.bIsRe_Init = 1;
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "step2: VGA resolution changed! %dx%d", s_astVoDevStatDflt[HD].stVideoLayerAttr.stDispRect.u32Width, s_astVoDevStatDflt[HD].stVideoLayerAttr.stDispRect.u32Height);
#else
			s_astVoDevStatDflt[DHD0].stVoPubAttr.enIntfSync = enIntfSync;
			
			//printf("##########s_astVoDevStatDflt[DHD0]   s_VoDevCtrlDflt = %d,VoDev = %d################\n",s_VoDevCtrlDflt,VoDev);
			s_State_Info.bIsRe_Init = 0;
			if((s_State_Info.bIsOsd_Init))
			{
				Prv_OSD_Close_fb(DHD0);
				osd_flag = 1;
			}
			if(s_VoDevCtrlDflt == DHD0)
			{//�����ǰ����豸��HD����ô��Ҫ�ر�GUI
				PRV_CloseVoFb(s_VoDevCtrlDflt);
			}
			
			ret = PRV_ResetVoDev(DHD0);
			if(ret == HI_FAILURE)
			{//������ش���,�ָ��ɵ����ã�����ʧ��
				s_s32NPFlagDflt = old_enIntfSync;
				PRV_ResetVoDev(DHD0);
			}
			
			if(s_VoDevCtrlDflt == DHD0)
			{//�����ǰ����豸��HD����ô��Ҫ����GUI
				PRV_OpenVoFb(s_VoDevCtrlDflt);
			}
			if(osd_flag)
			{
				OSD_Set_Rec_Range_NP(s_s32NPFlagDflt);
				Prv_OSD_Open_fb( DHD0);
			}
			s_State_Info.bIsRe_Init = 1;
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "step2: VGA resolution changed! %dx%d", s_astVoDevStatDflt[DHD0].stVideoLayerAttr.stDispRect.u32Width, s_astVoDevStatDflt[DHD0].stVideoLayerAttr.stDispRect.u32Height);
#endif
			if(ret == HI_FAILURE)
			{
                pthread_mutex_unlock(&s_osd_mutex);
				RET_FAILURE("PRV_Voreset falied!\n");
			}
		}
			break;
		case 4://ֻ������豸�仯
		{
			s_State_Info.bIsRe_Init = 0;
			PRV_CloseVoFb(s_VoDevCtrlDflt);
			s_VoDevCtrlDflt = VoDev;
			PRV_OpenVoFb(VoDev);
			s_State_Info.bIsRe_Init = 1;
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "do nothing!! step4: Default GUI output dev changed! HD-0,AD-1,SD-2: %d", s_VoDevCtrlDflt);
		}
			break;
		case 6://�ֱ��ʱ仯������豸�仯
		{
			s_astVoDevStatDflt[HD].stVoPubAttr.enIntfSync = enIntfSync;
			s_State_Info.bIsRe_Init = 0;
			if((s_State_Info.bIsOsd_Init))
			{
				Prv_OSD_Close_fb(HD);
				osd_flag = 1;
			}
			PRV_CloseVoFb(s_VoDevCtrlDflt);
			ret = PRV_ResetVoDev(HD);
			if(ret == HI_FAILURE)
			{//������ش���,�ָ��ɵ����ã�����ʧ��
				s_s32NPFlagDflt = old_enIntfSync;
				PRV_ResetVoDev(HD);
				goto Err;
			}
			s_VoDevCtrlDflt = VoDev;
Err:			
			PRV_OpenVoFb(s_VoDevCtrlDflt);		
			if(osd_flag)
			{
				OSD_Set_Rec_Range_NP(s_s32NPFlagDflt);
				Prv_OSD_Open_fb( HD);
			}
			s_State_Info.bIsRe_Init = 1;
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "step2: VGA resolution changed! %dx%d\n", s_astVoDevStatDflt[HD].stVideoLayerAttr.stDispRect.u32Width, s_astVoDevStatDflt[HD].stVideoLayerAttr.stDispRect.u32Height);
			if(ret == HI_FAILURE)
			{
                pthread_mutex_unlock(&s_osd_mutex);
				RET_FAILURE("PRV_VoReset falied!\n");
			}
		}
			break;
		default:
			break;	
 	}
	//OSD_G1_open();//��ʾ���е�һ��GUI��
	////2010-9-30
	if(Changeflag & (1<<0))
	{
		VIDEO_FRAME_INFO_S *pstVFrame;
		pstVFrame = (0==s_s32NPFlagDflt)?&s_stUserFrameInfo_P:&s_stUserFrameInfo_N;
#if defined(SN9234H1)		
		CHECK(HI_MPI_VI_SetUserPic(0, 0, pstVFrame));
#endif	
		Loss_State_Pre = 0;
	}
//step2:
	//step2: ��ʾ���ֱ���  ��Ӧenum VGAResolution_enum{VGA_1024X768,VGA_1280_960,VGA_1280X1024}
//step3:
	//step3: ����ȥ��NoShake   0-������   1-����
	Prv_Set_Flicker(param->general_info.NoShake);
//step4:
	//step4: ����豸ѡ�� 0-DHD0/VGA   1-CVBS1 2- CVBS2
	s_u32GuiWidthDflt = s_astVoDevStatDflt[s_VoDevCtrlDflt].stVideoLayerAttr.stDispRect.u32Width;
	s_u32GuiHeightDflt = s_astVoDevStatDflt[s_VoDevCtrlDflt].stVideoLayerAttr.stDispRect.u32Height;
//step5:
	//step5: ������ʾ��ʽDataDisplayForm  0Ϊ YYYY-MM-DD   1Ϊ MM-DD-YYYY   2ΪDD-MM-YYYY   3ΪYYYY��MM��DD��   4ΪMM��DD��YYYY��
	//TODO
	if(param->general_info.DataDisplayForm != s_OSD_Time_type)
	{
#if defined(SN_SLAVE_ON)

		Prv_Slave_Set_general_cfg_Req slave_req;
		//�ѵ�ǰ����Ϣ��Ϣ������Ƭ����Ƭ��Ҫ����
		slave_req.general_info = param->general_info;
		//����ʱ���ʽ�޸�֪ͨ����Ƭ
		SN_SendMccMessageEx(PRV_SLAVE_1,msg_req->user, msg_req->source, MOD_PRV, msg_req->xid, msg_req->thread, MSG_ID_PRV_MCC_TIME_TYPE_CHANGE_IND, &slave_req, sizeof(slave_req));
#endif
		s_OSD_Time_type = param->general_info.DataDisplayForm;
	}
//step6:
	//step6: ʱ������TimeZoneSlt TimeZoneSlt_enum
	//TODO
//step7:
	//step7: HDVGA���ѡ��OutPutType 0-DHD0   1-VGA
	//TODO
//finished:
	//��ӱ����˿��޸ģ���ΪĿǰ�����˿���ͬ�Եģ�
#if defined(SN9234H1)
	s_VoDevAlarmDflt = s_VoDevCtrlDflt;//
	{//n\p��ʽ�л�������ֵ���޸ģ�����������Ҫ�ظ����ݿ����ֵ
		int i=0,ret=0;
		PRM_DISPLAY_CFG_CHAN disp_info;
		for(i=0;i<g_Max_Vo_Num;i++)
		{
			//ͼ������
			ret= GetParameter(PRM_ID_DISPLAY_CFG,NULL,&disp_info,sizeof(disp_info),i+1,SUPER_USER_ID,NULL);
			if(ret!= PARAM_OK)
			{
				TRACE(SCI_TRACE_HIGH, MOD_PRV,"get Preview_VideoParam param error!");
			}	
			ret = Preview_SetVideoParam(i,&disp_info);
			if(ret != HI_SUCCESS)
			{
				TRACE(SCI_TRACE_HIGH, MOD_PRV,"Preview_initVideoParam error1!");
			}
		}
	}
#endif
	rsp->result = 0;
    pthread_mutex_unlock(&s_osd_mutex);
	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_MSG_MCC_SetGeneralCfg
Description: ͨ��������Ϣ�������
Calls: 
Called By: //
Input: // // param:ͨ������Ԥ��ģ��ṹ��
			rsp:����GUI��RSP
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: //
***********************************************************************/
STATIC HI_S32 PRV_MSG_MCC_SetGeneralCfg(const Prv_Slave_Set_general_cfg_Rsp *param)
{
	VO_INTF_SYNC_E enIntfSync;		/*VGA�ֱ���*/
	VO_DEV VoDev;					/*GUI����豸*/
	HI_S32 s32NPFlag=0;				/*NP��ʽ*/
	int Changeflag=0,osd_flag=0;			//�仯��־λ����0λ��ʾN\P��ʽ�仯����1λ��ʾ�ֱ��ʱ仯����2λ��ʾ����豸�仯
	//printf("###########PRV_MSG_MCC_SetGeneralCfg Get msg !######################\n");

	if (NULL == param)
	{
		RET_FAILURE("NULL pointer!!!");
	}
	
	/*NP��ʽ*/
	s32NPFlag = (0 == param->general_info.CVBSOutputType) ? 1 : 0;
	/*VGA�ֱ���*/
	switch(param->general_info.VGAResolution)
	{
	case VGA_1080P:
	case VGA_720P:
	case VGA_1024X768:	
		enIntfSync = VO_OUTPUT_720P60;
		break;
	case VGA_1280X1024:	
		enIntfSync = VO_OUTPUT_720P60;
		break;
	case VGA_800X600:
		enIntfSync = VO_OUTPUT_800x600_60;
		break;
	case VGA_1366x768:
		enIntfSync = VO_OUTPUT_1366x768_60;
		break;
	case VGA_1440x900:
		enIntfSync = VO_OUTPUT_1440x900_60;
		break;
	default:
		RET_FAILURE("Unsupport VGA Resolution");
	}

	/*GUI����豸*/
	switch(param->general_info.OutputSel)
	{
	case 0:
		VoDev = HD;
		break;
	case 1:
		VoDev = s_VoSecondDev;
		break;
	
#if defined(SN9234H1)
		case 2:	
		VoDev = SD;
#elif defined(Hi3531)
		case 2:
		VoDev = DSD1;
#endif
		RET_FAILURE("SD is not supported for Gui!");
	default :
		RET_FAILURE("general_info.OutputSel is out off range!");
	}
    pthread_mutex_lock(&s_osd_mutex);

//step1:
	//step1: CVBS�����ʽ
	if (s32NPFlag != s_s32NPFlagDflt)
	{
		//Changeflag |= 1<<0; 
	}
#if defined(SN9234H1)
	if( (enIntfSync != s_astVoDevStatDflt[HD].stVoPubAttr.enIntfSync) ||( param->general_info.VGAResolution != s_s32VGAResolution))
	{
		Changeflag |= 1<<1; 
		s_s32VGAResolution = param->general_info.VGAResolution;
	}
#else
	if( (enIntfSync != s_astVoDevStatDflt[DHD0].stVoPubAttr.enIntfSync) ||( param->general_info.VGAResolution != s_s32VGAResolution))
	{
		Changeflag |= 1<<1; 
		s_s32VGAResolution = param->general_info.VGAResolution;
	}
#endif

	if (s_VoDevCtrlDflt != VoDev)
	{
		Changeflag |= 1<<2; 
	}
	switch(Changeflag)
	{
		case 1://ֻ��N\P��ʽ�仯
		case 3://N\P��ʽ�仯���ֱ��ʱ仯
		case 5://N\P��ʽ�仯������豸�仯
		case 7://N\P��ʽ�仯���ֱ��ʱ仯������豸�仯
		{
			s_s32NPFlagDflt = s32NPFlag;
			s_State_Info.bIsRe_Init = 0;
			if((s_State_Info.bIsOsd_Init))
			{
#if defined(SN9234H1)
				Prv_OSD_Close_fb(HD);
				Prv_OSD_Close_fb(AD);
				Prv_OSD_Close_fb(SD);
#else
				Prv_OSD_Close_fb(DHD0);
				//Prv_OSD_Close_fb(DSD0);
#endif				
				osd_flag = 1;
			}
			PRV_CloseVoFb(s_VoDevCtrlDflt);
			
			//���ͷ����а󶨹�ϵ
#if defined(SN9234H1)
			PRV_ViUnBindAllVoChn(VoDev);
			PRV_VdecUnBindAllVoChn1(VoDev);
			PRV_ViUnBindAllVoChn(s_VoSecondDev);
			PRV_VdecUnBindAllVoChn1(s_VoSecondDev);
			//VI
			PRV_ViInit();
			//VO
			s_astVoDevStatDflt[HD].stVoPubAttr.enIntfSync = enIntfSync;
			PRV_ResetVoDev(HD);
#else
		//	PRV_ViUnBindAllVoChn(VoDev);
			PRV_VdecUnBindAllVpss(VoDev);
		//	PRV_ViUnBindAllVoChn(s_VoSecondDev);
		//	PRV_VdecUnBindAllVoChn1(s_VoSecondDev);
			//VI
			PRV_ViInit();
			//VO
			s_astVoDevStatDflt[DHD0].stVoPubAttr.enIntfSync = enIntfSync;
			PRV_ResetVoDev(DHD0);
#endif			
			PRV_ResetVoDev(s_VoSecondDev);
	
			PRV_OpenVoFb(VoDev);
			s_VoDevCtrlDflt = VoDev;
			if(osd_flag)
			{
				OSD_Set_Rec_Range_NP(s_s32NPFlagDflt);
#if defined(SN9234H1)
				Prv_OSD_Open_fb( HD);
				Prv_OSD_Open_fb( AD);
#else
				Prv_OSD_Open_fb( DHD0);
				//Prv_OSD_Open_fb( DSD0);
#endif				
			}
			s_State_Info.bIsRe_Init = 1;
			TRACE(SCI_TRACE_HIGH, MOD_PRV, "step1: N/P mode changed! N-0,P-1: %d", s_s32NPFlagDflt);
		}
			break;
		case 2://ֻ�зֱ��ʱ仯
		{
#if defined(SN9234H1)
			s_astVoDevStatDflt[HD].stVoPubAttr.enIntfSync = enIntfSync;
			
			//printf("##########s_astVoDevStatDflt[HD]   s_VoDevCtrlDflt = %d,VoDev = %d################\n",s_VoDevCtrlDflt,VoDev);
			s_State_Info.bIsRe_Init = 0;
			if((s_State_Info.bIsOsd_Init))
			{
				Prv_OSD_Close_fb(HD);
				osd_flag = 1;
			}
			if(s_VoDevCtrlDflt == HD)
			{//�����ǰ����豸��HD����ô��Ҫ�ر�GUI
				PRV_CloseVoFb(s_VoDevCtrlDflt);
			}
			PRV_ResetVoDev(HD);
			if(s_VoDevCtrlDflt == HD)
			{//�����ǰ����豸��HD����ô��Ҫ����GUI
				PRV_OpenVoFb(s_VoDevCtrlDflt);
			}
			if(osd_flag)
			{
				OSD_Set_Rec_Range_NP(s_s32NPFlagDflt);
				Prv_OSD_Open_fb(HD);
			}
			s_State_Info.bIsRe_Init = 1;
			TRACE(SCI_TRACE_HIGH, MOD_PRV, "step2: VGA resolution changed! %dx%d", s_astVoDevStatDflt[HD].stVideoLayerAttr.stDispRect.u32Width, s_astVoDevStatDflt[HD].stVideoLayerAttr.stDispRect.u32Height);
#else
			s_astVoDevStatDflt[DHD0].stVoPubAttr.enIntfSync = enIntfSync;
			
			//printf("##########s_astVoDevStatDflt[DHD0]   s_VoDevCtrlDflt = %d,VoDev = %d################\n",s_VoDevCtrlDflt,VoDev);
			s_State_Info.bIsRe_Init = 0;
			if((s_State_Info.bIsOsd_Init))
			{
				Prv_OSD_Close_fb(DHD0);
				osd_flag = 1;
			}
			if(s_VoDevCtrlDflt == DHD0)
			{//�����ǰ����豸��HD����ô��Ҫ�ر�GUI
				PRV_CloseVoFb(s_VoDevCtrlDflt);
			}
			PRV_ResetVoDev(DHD0);
			if(s_VoDevCtrlDflt == DHD0)
			{//�����ǰ����豸��HD����ô��Ҫ����GUI
				PRV_OpenVoFb(s_VoDevCtrlDflt);
			}
			if(osd_flag)
			{
				OSD_Set_Rec_Range_NP(s_s32NPFlagDflt);
				Prv_OSD_Open_fb(DHD0);
			}
			s_State_Info.bIsRe_Init = 1;
			TRACE(SCI_TRACE_HIGH, MOD_PRV, "step2: VGA resolution changed! %dx%d", s_astVoDevStatDflt[DHD0].stVideoLayerAttr.stDispRect.u32Width, s_astVoDevStatDflt[DHD0].stVideoLayerAttr.stDispRect.u32Height);
#endif
		}
			break;
		case 4://ֻ������豸�仯
		{
			s_State_Info.bIsRe_Init = 0;
			PRV_CloseVoFb(s_VoDevCtrlDflt);
			PRV_OpenVoFb(VoDev);
			s_State_Info.bIsRe_Init = 1;
			s_VoDevCtrlDflt = VoDev;
			TRACE(SCI_TRACE_HIGH, MOD_PRV, "do nothing!! step4: Default GUI output dev changed! DHD0-0,AD-1,SD-2: %d", s_VoDevCtrlDflt);
		}
			break;
		case 6://�ֱ��ʱ仯������豸�仯
		{
#if defined(SN9234H1)
			s_astVoDevStatDflt[HD].stVoPubAttr.enIntfSync = enIntfSync;
			s_State_Info.bIsRe_Init = 0;
			if((s_State_Info.bIsOsd_Init))
			{
				Prv_OSD_Close_fb(HD);
				osd_flag = 1;
			}
			PRV_CloseVoFb(s_VoDevCtrlDflt);
			PRV_ResetVoDev(HD);
			PRV_OpenVoFb(VoDev);
			s_VoDevCtrlDflt = VoDev;
			if(osd_flag)
			{
				OSD_Set_Rec_Range_NP(s_s32NPFlagDflt);
				Prv_OSD_Open_fb(HD);
			}
			s_State_Info.bIsRe_Init = 1;
			TRACE(SCI_TRACE_HIGH, MOD_PRV, "step2: VGA resolution changed! %dx%d", s_astVoDevStatDflt[HD].stVideoLayerAttr.stDispRect.u32Width, s_astVoDevStatDflt[HD].stVideoLayerAttr.stDispRect.u32Height);
#else
			s_astVoDevStatDflt[DHD0].stVoPubAttr.enIntfSync = enIntfSync;
			s_State_Info.bIsRe_Init = 0;
			if((s_State_Info.bIsOsd_Init))
			{
				Prv_OSD_Close_fb(DHD0);
				osd_flag = 1;
			}
			PRV_CloseVoFb(s_VoDevCtrlDflt);
			PRV_ResetVoDev(DHD0);
			PRV_OpenVoFb(VoDev);
			s_VoDevCtrlDflt = VoDev;
			if(osd_flag)
			{
				OSD_Set_Rec_Range_NP(s_s32NPFlagDflt);
				Prv_OSD_Open_fb(DHD0);
			}
			s_State_Info.bIsRe_Init = 1;
			TRACE(SCI_TRACE_HIGH, MOD_PRV, "step2: VGA resolution changed! %dx%d", s_astVoDevStatDflt[DHD0].stVideoLayerAttr.stDispRect.u32Width, s_astVoDevStatDflt[DHD0].stVideoLayerAttr.stDispRect.u32Height);
#endif
		}
			break;
		default:
			break;	
 	}
	////2010-9-30
	if(Changeflag & (1<<0))
	{
		VIDEO_FRAME_INFO_S *pstVFrame;
		pstVFrame = (0==s_s32NPFlagDflt)?&s_stUserFrameInfo_P:&s_stUserFrameInfo_N;
#if defined(SN9234H1)
		CHECK(HI_MPI_VI_SetUserPic(0, 0, pstVFrame));
#endif	
		Loss_State_Pre = 0;
	}
//step2:
	//step2: ��ʾ���ֱ���  ��Ӧenum VGAResolution_enum{VGA_1024X768,VGA_1280_960,VGA_1280X1024}
//step3:
	//step3: ����ȥ��NoShake   0-������   1-����
	Prv_Set_Flicker(param->general_info.NoShake);
//step4:
	//step4: ����豸ѡ�� 0-DHD0/VGA   1-CVBS1 2- CVBS2
	s_u32GuiWidthDflt = s_astVoDevStatDflt[s_VoDevCtrlDflt].stVideoLayerAttr.stDispRect.u32Width;
	s_u32GuiHeightDflt = s_astVoDevStatDflt[s_VoDevCtrlDflt].stVideoLayerAttr.stDispRect.u32Height;
//step5:
	//step5: ������ʾ��ʽDataDisplayForm  0Ϊ YYYY-MM-DD   1Ϊ MM-DD-YYYY   2ΪDD-MM-YYYY   3ΪYYYY��MM��DD��   4ΪMM��DD��YYYY��
	//TODO
	if(param->general_info.DataDisplayForm != s_OSD_Time_type)
	{
		s_OSD_Time_type = param->general_info.DataDisplayForm;
	}
    pthread_mutex_unlock(&s_osd_mutex);

	RET_SUCCESS("");
}
/*************************************************
Function: //PRV_MSG_MCC_SetGeneralCfg_Rsp
Description: ��Ƭ���غ�ͨ��������Ϣ�������
Calls: 
Called By: //
Input: // // msg_req:ͨ��������Ϣ
			rsp:����GUI��RSP
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: //
***********************************************************************/
STATIC HI_S32 PRV_MSG_MCC_SetGeneralCfg_Rsp(const Prv_Slave_Set_general_cfg_Rsp *slave_rsp,Set_general_cfg_Rsp *rsp)
{
	
	if (NULL == slave_rsp || NULL == rsp)
	{
		RET_FAILURE("NULL pointer!!!");
	}
	rsp->result = -1;
	rsp->general_info = slave_rsp->general_info;
	
	//�����Ƭ���أ���ô��λ��Ƭ�ķ��ر�־λ
	s_State_Info.bIsReply |= 1<<(slave_rsp->slaveid-1);
	
	if(slave_rsp->result == SN_RET_OK)
	{	//�����Ƭ������ȷֵ
		s_State_Info.g_slave_OK = 1<<(slave_rsp->slaveid-1);
	}
	//�������ֵ��1 ����1����оƬ��������ֵ����ô��ʾ���д�Ƭ��������
	if((s_State_Info.bIsReply+1) == 1<<(PRV_CHIP_NUM-1))	
	{
		s_State_Info.TimeoutCnt = HI_FALSE;//������λ��ʱ��־λ
		TimerPause(s_State_Info.f_timer_handle); //��ͣ��ʱ��
		s_State_Info.bIsReply = 0;//�ظ�״̬�˳�
		s_State_Info.bIsSlaveConfig = HI_FALSE;	//������λ���ñ�־λ
		if((s_State_Info.g_slave_OK+1) == 1<<(PRV_CHIP_NUM-1))
		{//������ص���Ϣ��û�д�����Ϣ����ô�˴���ͨ��������Ϣ
			PRV_MSG_MCC_SetGeneralCfg(slave_rsp);//��Ƭ���뻭��ģʽ����
			rsp->result = 0;
		}
		s_State_Info.g_slave_OK = 0;	//���ûظ���ֵ��־λ
		if(s_State_Info.Prv_msg_Cur)
		{
			SN_FREE(s_State_Info.Prv_msg_Cur);//�ͷ�����ɵ���Ϣ�ṹ��ָ��
		}
	}
	PRV_SetGuiAlpha(0, 0);
	
	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_MSG_SetChnCfg
Description: //��Ƭͨ��������Ϣ����
//��λ�����£�����ô˽ӿ�����ͨ������OSD
Calls: 
Called By: //
Input: // // msg_req:ͨ��������Ϣ
			rsp : �ظ���GUI����Ϣ
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: //
***********************************************************************/
int PRV_MSG_SetChnCfg(Set_chn_cfg_Req *param, Set_chn_cfg_Rsp *rsp)
{
	HI_S32 s32Ret = -1;
	//Set_chn_cfg_Req *param = (Set_chn_cfg_Req *)msg_req->para;
	if (NULL == param || NULL == rsp)
	{
		RET_FAILURE("NULL pointer!!!");
	}
	rsp->chn = param->chn;
	rsp->result = -1;
	rsp->chn_info = param->chn_info;
	//printf("11111vparam->chn_info.ChannelName = %s  ,param->chn_info.ChanNameDsp=%d,param->chn_info.ChanDataDsp=%d\n",param->chn_info.ChannelName,param->chn_info.ChanNameDsp,param->chn_info.ChanDataDsp);
	//TODO
	if(param->reserve[0] != 1)//ͨ����������
	{
		if(param->reserve[1] != 1 && ScmGetListCtlState() == 1)//��λ����������ͨ������OSD����ʱ��Ч
		{
			rsp->result = 0;
			ScmAllChnSetOsd(EnterListCtl);
			return HI_SUCCESS;
		}
		
		if(param->chn == 0)
		{//���ͨ����Ϊ0����ʾȫ��ͨ������
			int i = 0;
			for(i = 0; i < g_Max_Vo_Num; i++)
			{
			//ͨ����������
				//if(param->chn_info.ChanNameDsp)
				{//
					s32Ret = OSD_Set_Ch(i, param->chn_info.ChannelName);	//��ʾͨ������
					if(s32Ret != HI_SUCCESS)
					{
						TRACE(SCI_TRACE_HIGH, MOD_PRV,"PRV_MSG_SetChnCfg OSD_Set_Ch faild 0x%x!\n",s32Ret);
						rsp->chn = i;
						rsp->result = -1;
						RET_FAILURE("");
					}
						
				}
				s32Ret = OSD_Ctl(i, param->chn_info.ChanNameDsp,OSD_NAME_TYPE);
				if(s32Ret != HI_SUCCESS)
				{
					TRACE(SCI_TRACE_HIGH, MOD_PRV,"PRV_MSG_SetChnCfg OSD_Ctl name faild 0x%x!\n",s32Ret);
					rsp->chn = i;
					rsp->result = -1;
					RET_FAILURE("");
				}
				#if 0
				//������ʾ
				s32Ret = OSD_Ctl(i,param->chn_info.ChanDataDsp,OSD_TIME_TYPE);
				if(s32Ret != HI_SUCCESS)
				{
					TRACE(SCI_TRACE_HIGH, MOD_PRV,"PRV_MSG_SetChnCfg OSD_Ctl time faild 0x%x!\n",s32Ret);
					rsp->chn = i;
					rsp->result = -1;
					RET_FAILURE("");
				}
				#endif
			}
#if 0
#if defined(SN_SLAVE_ON)		
			//������Ϣ����Ƭ
			{
				Prv_Slave_Set_chn_cfg_Req slave_req;
				slave_req.chn_info = param->chn_info;
				slave_req.chn = param->chn;
				
				SN_SendMccMessageEx(PRV_SLAVE_1,msg_req->user, msg_req->source, MOD_PRV, msg_req->xid, msg_req->thread, MSG_ID_PRV_MCC_SET_CHN_CFG_REQ, &slave_req, sizeof(slave_req));
				//������ʱ��
				TimerReset(s_State_Info.f_timer_handle,10);
				TimerResume(s_State_Info.f_timer_handle,0);
				PRV_Msg_Cpy(msg_req);		//���浱ǰ��Ϣ���Ա������Ϣ�ط�
				s32Ret = SN_SLAVE_MSG;
			}
#endif
#endif
		}
		else
		{
			//ͨ����������
			//if(param->chn_info.ChanNameDsp)
			{//
				s32Ret = OSD_Set_Ch(param->chn - 1, param->chn_info.ChannelName);	//��ʾͨ������
				if(s32Ret != HI_SUCCESS)
				{
					TRACE(SCI_TRACE_HIGH, MOD_PRV,"PRV_MSG_SetChnCfg OSD_Set_Ch faild 0x%x!\n",s32Ret);
					rsp->result = -1;
					RET_FAILURE("");
				}
			}
			s32Ret = OSD_Ctl(param->chn-1, param->chn_info.ChanNameDsp, OSD_NAME_TYPE);
			if(s32Ret != HI_SUCCESS)
			{
				TRACE(SCI_TRACE_HIGH, MOD_PRV,"PRV_MSG_SetChnCfg OSD_Ctl faild 0x%x!\n",s32Ret);
				rsp->result = -1;
				RET_FAILURE("");
			}
			#if 0
			//������ʾ
			s32Ret = OSD_Ctl(param->chn-1,param->chn_info.ChanDataDsp,OSD_TIME_TYPE);
			if(s32Ret != HI_SUCCESS)
			{
				TRACE(SCI_TRACE_HIGH, MOD_PRV,"PRV_MSG_SetChnCfg OSD_Ctl time faild 0x%x!\n",s32Ret);
				rsp->result = -1;
				RET_FAILURE("");
			}
			#endif
#if 0
#if defined(SN_SLAVE_ON)		
			if(param->chn > PRV_CHAN_NUM)//���ͨ���Ŵ�����Ƭ���ͨ����
			{
				//������Ϣ����Ƭ
				Prv_Slave_Set_chn_cfg_Req slave_req;
				
				slave_req.chn_info = param->chn_info;
				slave_req.chn = param->chn;
				
				SN_SendMccMessageEx(PRV_SLAVE_1,msg_req->user, msg_req->source, MOD_PRV, msg_req->xid, msg_req->thread, MSG_ID_PRV_MCC_SET_CHN_CFG_REQ, &slave_req, sizeof(slave_req));
				//������ʱ��
				TimerReset(s_State_Info.f_timer_handle,10);
				TimerResume(s_State_Info.f_timer_handle,0);
				PRV_Msg_Cpy(msg_req);		//���浱ǰ��Ϣ���Ա������Ϣ�ط�
				s32Ret = SN_SLAVE_MSG;
			}
#endif	
#endif
		}
	}
	else//������ʾ
	{
		//printf("param->chn_info.ChanDataDsp: %d\n", param->chn_info.ChanDataDsp);
		s32Ret = OSD_Ctl(0, param->chn_info.ChanDataDsp, OSD_TIME_TYPE);
		if(s32Ret != HI_SUCCESS)
		{
			TRACE(SCI_TRACE_HIGH, MOD_PRV,"PRV_MSG_SetChnCfg OSD_Ctl time faild 0x%x!\n",s32Ret);
			rsp->result = -1;
			RET_FAILURE("");
		}		
	}
	rsp->result = 0;
	return s32Ret;
}
/*************************************************
Function: //PRV_MSG_MCC_SetChnCfg_Rsp
Description: //��Ƭͨ��������Ϣ����
Calls: 
Called By: //
Input: // // msg_req:ͨ��������Ϣ
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: //
***********************************************************************/
STATIC HI_S32 PRV_MSG_MCC_SetChnCfg_Rsp(const Prv_Slave_Set_chn_cfg_Rsp *slave_rsp,Set_chn_cfg_Rsp *rsp)
{
	
	if (NULL == slave_rsp || NULL == rsp)
	{
		RET_FAILURE("NULL pointer!!!");
	}
	rsp->chn = slave_rsp->chn;
	rsp->result = -1;
	rsp->chn_info = slave_rsp->chn_info;
	
	//�����Ƭ���أ���ô��λ��Ƭ�ķ��ر�־λ
	s_State_Info.bIsReply |= 1<<(slave_rsp->slaveid-1);
	
	if(slave_rsp->result == SN_RET_OK)
	{	//�����Ƭ������ȷֵ
		s_State_Info.g_slave_OK = 1<<(slave_rsp->slaveid-1);
	}
	//�������ֵ��1 ����1����оƬ��������ֵ����ô��ʾ���д�Ƭ��������
	if((s_State_Info.bIsReply+1) == 1<<(PRV_CHIP_NUM-1))	
	{
		if((s_State_Info.g_slave_OK+1) == 1<<(PRV_CHIP_NUM-1))
		{//������ص���Ϣ��û�д�����Ϣ
			rsp->result = 0;
		}
		s_State_Info.TimeoutCnt = HI_FALSE;//������λ��ʱ��־λ
		TimerPause(s_State_Info.f_timer_handle); //��ͣ��ʱ��
		s_State_Info.bIsReply = 0;//�ظ�״̬�˳�
		s_State_Info.bIsSlaveConfig = HI_FALSE;	//������λ���ñ�־λ
		s_State_Info.g_slave_OK = 0;	//���ûظ���ֵ��־λ
		if(s_State_Info.Prv_msg_Cur)
		{
			SN_FREE(s_State_Info.Prv_msg_Cur);//�ͷ�����ɵ���Ϣ�ṹ��ָ��
		}
		
	}
	
	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_MSG_SetChnCover
Description: //��Ƭͨ���ڸ�������Ϣ����
Calls: 
Called By: //
Input: // // param:ͨ���ڸ�������Ϣ�ṹ��
			rsp: �ظ���Ϣ�ṹ��
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: //
***********************************************************************/
STATIC HI_S32 PRV_MSG_SetChnCover(const SN_MSG *msg_req, Set_chn_cover_Rsp *rsp)
{
	int i=0,ret=-1;
	Set_chn_cover_Req *param = (Set_chn_cover_Req *)msg_req->para;
	
	if (NULL == param || NULL == rsp)
	{
		RET_FAILURE("NULL pointer!!!");
	}
	rsp->chn = param->chn;
	rsp->result = -1;
	rsp->cover_info = param->cover_info;
	
	//TODO
	if(param->chn == 0)
	{//���ͨ����Ϊ0����ô��ʾ����ͨ������
		for(i=0;i<PRV_CHAN_NUM;i++)
		{
			ret = OSD_Mask_update(i,param->cover_info.AreaHideScape,MAX_HIDE_AREA_NUM);
			if(ret != HI_SUCCESS)
			{
				rsp->chn = i;
				rsp->result = -1;
				RET_FAILURE("");
			}
			ret = OSD_Mask_Ctl(i,param->cover_info.EnableAreaHide);
			if(ret != HI_SUCCESS)
			{
				rsp->chn = i;
				rsp->result = -1;
				RET_FAILURE("");
			}
		}
#if defined(SN_SLAVE_ON)		
		{//������Ϣ����Ƭ
			Prv_Slave_Set_chn_cover_Req slave_req;
			slave_req.cover_info = param->cover_info;
			slave_req.chn = param->chn;
			
			SN_SendMccMessageEx(PRV_SLAVE_1,msg_req->user, msg_req->source, MOD_PRV, msg_req->xid, msg_req->thread, MSG_ID_PRV_MCC_SET_CHN_COVER_REQ, &slave_req, sizeof(slave_req));
			//������ʱ��
			TimerReset(s_State_Info.f_timer_handle,10);
			TimerResume(s_State_Info.f_timer_handle,0);
			PRV_Msg_Cpy(msg_req);		//���浱ǰ��Ϣ���Ա������Ϣ�ط�
			ret = SN_SLAVE_MSG;
			return ret;
		}
#endif
	}
	else
	{
#if defined(SN_SLAVE_ON)	
		if(param->chn > PRV_CHAN_NUM)
		{//���ͨ����������Ƭ���ͨ��������ô������Ϣ����Ƭ
			//������Ϣ����Ƭ
			Prv_Slave_Set_chn_cover_Req slave_req;
			slave_req.cover_info = param->cover_info;
			slave_req.chn = param->chn;
			
			SN_SendMccMessageEx(PRV_SLAVE_1,msg_req->user, msg_req->source, MOD_PRV, msg_req->xid, msg_req->thread, MSG_ID_PRV_MCC_SET_CHN_COVER_REQ, &slave_req, sizeof(slave_req));
			//������ʱ��
			TimerReset(s_State_Info.f_timer_handle,10);
			TimerResume(s_State_Info.f_timer_handle,0);
			PRV_Msg_Cpy(msg_req);		//���浱ǰ��Ϣ���Ա������Ϣ�ط�
			ret = SN_SLAVE_MSG;
			return ret;
		}
#endif		
		//ͨ��������Ƭ��Ҫ������Ϣ����Ƭ
		ret = OSD_Mask_update(param->chn-1,param->cover_info.AreaHideScape,MAX_HIDE_AREA_NUM);
		if(ret != HI_SUCCESS)
		{
			rsp->result = -1;
			RET_FAILURE("");
		}
		ret = OSD_Mask_Ctl(param->chn-1,param->cover_info.EnableAreaHide);
		if(ret != HI_SUCCESS)
		{
			rsp->result = -1;
			RET_FAILURE("");
		}
	}
	rsp->result = 0;
	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_MSG_MCC_SetChnCover_Rsp
Description: //��Ƭͨ���ڸ�������Ϣ����
Calls: 
Called By: //
Input: // // param:ͨ���ڸ�������Ϣ�ṹ��
			rsp: �ظ���Ϣ�ṹ��
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: //
***********************************************************************/
STATIC HI_S32 PRV_MSG_MCC_SetChnCover_Rsp(const Prv_Slave_Set_chn_cover_Rsp *slave_rsp,Set_chn_cover_Rsp *rsp)
{
	if (NULL == slave_rsp || NULL == rsp)
	{
		RET_FAILURE("NULL pointer!!!");
	}
	rsp->chn = slave_rsp->chn;
	rsp->result = -1;
	rsp->cover_info = slave_rsp->cover_info;
	
	//TODO
	//�����Ƭ���أ���ô��λ��Ƭ�ķ��ر�־λ
	s_State_Info.bIsReply |= 1<<(slave_rsp->slaveid-1);
	
	if(slave_rsp->result == SN_RET_OK)
	{	//�����Ƭ������ȷֵ
		s_State_Info.g_slave_OK = 1<<(slave_rsp->slaveid-1);
	}
	//�������ֵ��1 ����1����оƬ��������ֵ����ô��ʾ���д�Ƭ��������
	if((s_State_Info.bIsReply+1) == 1<<(PRV_CHIP_NUM-1))	
	{
		if((s_State_Info.g_slave_OK+1) == 1<<(PRV_CHIP_NUM-1))
		{//������ص���Ϣ��û�д�����Ϣ
			rsp->result = 0;
		}
		s_State_Info.TimeoutCnt = HI_FALSE;//������λ��ʱ��־λ
		TimerPause(s_State_Info.f_timer_handle); //��ͣ��ʱ��
		s_State_Info.bIsReply = 0;//�ظ�״̬�˳�
		s_State_Info.bIsSlaveConfig = HI_FALSE;	//������λ���ñ�־λ
		s_State_Info.g_slave_OK = 0;	//���ûظ���ֵ��־λ
		if(s_State_Info.Prv_msg_Cur)
		{
			SN_FREE(s_State_Info.Prv_msg_Cur);//�ͷ�����ɵ���Ϣ�ṹ��ָ��
		}
	}
	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_MSG_SetDisplay_Time
Description: //ͨ��Ч��ʵʱ��ʾ��Ϣ����
Calls: 
Called By: //
Input: // // param:ͨ��Ч��ʵʱ��ʾ������Ϣ�ṹ��
			rsp: �ظ���Ϣ�ṹ��
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: //
***********************************************************************/
STATIC HI_S32 PRV_MSG_SetDisplay_Time(const Chn_disp_change_Req *param, Msg_id_prv_Rsp *rsp)
{
	HI_S32 s32Ret=-1;
	if (NULL == param || NULL == rsp)
	{
		RET_FAILURE("NULL pointer!!!");
	}
	rsp->chn = param->chn;
	rsp->result = -1;
	//TODO
	switch(param->index)
	{
		case 0:
			//�Աȶ�
			s32Ret = Preview_SetVideo_Cont(param->chn,param->value);
			if(s32Ret != HI_SUCCESS)
			{
				rsp->result = -1;
				RET_FAILURE("Preview_SetVideo_Cont error!");
			}
			break;
		case 1:
			//����
			s32Ret = Preview_SetVideo_Brt(param->chn,param->value);
			if(s32Ret != HI_SUCCESS)
			{
				rsp->result = -1;
				RET_FAILURE("Preview_SetVideo_Brt error!");
			}
			break;
		case 2:
			//ɫ��
			s32Ret = Preview_SetVideo_Hue(param->chn,param->value);
			if(s32Ret != HI_SUCCESS)
			{
				rsp->result = -1;
				RET_FAILURE("Preview_SetVideo_Hue error!");
			}
			break;
		case 3:
			//���Ͷ�
			s32Ret = Preview_SetVideo_Sat(param->chn,param->value);
			if(s32Ret != HI_SUCCESS)
			{
				rsp->result = -1;
				RET_FAILURE("Preview_SetVideo_Sat error!");
			}
			break;
		case 4:
			//x������λ��
			s32Ret = Preview_SetVideo_x(param->chn,param->value);
			if(s32Ret != HI_SUCCESS)
			{
				rsp->result = -1;
				RET_FAILURE("Preview_SetVideo_x error!");
			}
			break;
		case 5:
			//y������λ��
			s32Ret = Preview_SetVideo_y(param->chn,param->value);
			if(s32Ret != HI_SUCCESS)
			{
				rsp->result = -1;
				RET_FAILURE("Preview_SetVideo_y error!");
			}
			break;
		default:
			rsp->result = -1;
			RET_FAILURE("the param is not surpport!");
			break;
	}
	rsp->result = 0;
	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_MSG_SetChnDisplay
Description: //ͨ����ʾЧ��������Ϣ����
Calls: 
Called By: //
Input: // // param:ͨ����ʾЧ������������Ϣ�ṹ��
			rsp: �ظ���Ϣ�ṹ��
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: //
***********************************************************************/
STATIC HI_S32 PRV_MSG_SetChnDisplay(const Set_chn_display_Req *param, Set_chn_display_Rsp *rsp)
{
	HI_S32 s32Ret=-1;
	if (NULL == param || NULL == rsp)
	{
		RET_FAILURE("NULL pointer!!!");
	}
	rsp->chn = param->chn;
	rsp->result = -1;
	rsp->display_info = param->display_info;
	
	if (param->chn > g_Max_Vo_Num)
	{
		RET_FAILURE("param->chn out off range:0~16");
	}
	//TODO
	if(param->chn == 0)
	{//���ͨ����Ϊ0����ʾȫ��ͨ������
		int i=0;
		for(i=0;i<g_Max_Vo_Num;i++)
		{
			s32Ret = Preview_SetVideoParam(i,(PRM_DISPLAY_CFG_CHAN *)&param->display_info);
			if(s32Ret != HI_SUCCESS)
			{
				rsp->chn = i;
				rsp->result = -1;
				RET_FAILURE("Preview_SetVideoParam error!");
			}
		}
	}
	else
	{
		s32Ret = Preview_SetVideoParam(param->chn-1,(PRM_DISPLAY_CFG_CHAN *)&param->display_info);
		if(s32Ret != HI_SUCCESS)
		{
			rsp->result = -1;
			RET_FAILURE("Preview_SetVideoParam error!");
		}
	}
	rsp->result = 0;
	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_MSG_SetChnDisplay
Description: //��Ƭͨ��OSD������Ϣ����
Calls: 
Called By: //
Input: // // msg_req:ͨ��OSD����������Ϣ�ṹ��
			rsp: �ظ���Ϣ�ṹ��
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: //
***********************************************************************/
STATIC HI_S32 PRV_MSG_SetChnOsd(const SN_MSG *msg_req, Set_chn_osd_Rsp *rsp)
{
	HI_S32 s32Ret=-1;
	Set_chn_osd_Req *param = (Set_chn_osd_Req*)msg_req->para;
	if (NULL == param || NULL == rsp)
	{
		RET_FAILURE("NULL pointer!!!");
	}
	rsp->chn = param->chn;
	rsp->result = -1;
	rsp->osd_info = param->osd_info;
	
	//TODO
	if(param->chn == 0)
	{//���ͨ����Ϊ0����ʾȫ��ͨ������
		int i = 0;
		for(i = 0;i < g_Max_Vo_Num; i++)
		{
			/*//ʱ��λ��
			s32Ret = OSD_Set_Time_xy(i,param->osd_info.ChannelTimePosition_x,param->osd_info.ChannelTimePosition_y);
			if(s32Ret != HI_SUCCESS)
			{
				rsp->chn = i;
				rsp->result = -1;
				RET_FAILURE("");
			}
			//ͨ������λ��
			s32Ret = OSD_Set_CH_xy(i,param->osd_info.ChannelNamePosition_x,param->osd_info.ChannelNamePosition_y);
			if(s32Ret != HI_SUCCESS)
			{
				rsp->chn = i;
				rsp->result = -1;
				RET_FAILURE("");
			}*/
			s32Ret = OSD_Set_xy(i,param->osd_info.ChannelNamePosition_x,param->osd_info.ChannelNamePosition_y,param->osd_info.ChannelTimePosition_x,param->osd_info.ChannelTimePosition_y);
			if(s32Ret != HI_SUCCESS)
			{
				rsp->chn = i;
				rsp->result = -1;
				RET_FAILURE("");
			}
		}
#if defined(SN_SLAVE_ON)		
		{//������Ϣ����Ƭ
			Prv_Slave_Set_chn_osd_Req slave_req;
			slave_req.osd_info = param->osd_info;
			slave_req.chn = param->chn;
			
			SN_SendMccMessageEx(PRV_SLAVE_1,msg_req->user, msg_req->source, MOD_PRV, msg_req->xid, msg_req->thread, MSG_ID_PRV_MCC_SET_CHN_OSD_REQ, &slave_req, sizeof(slave_req));
			//������ʱ��
			TimerReset(s_State_Info.f_timer_handle,10);
			TimerResume(s_State_Info.f_timer_handle,0);
			PRV_Msg_Cpy(msg_req);		//���浱ǰ��Ϣ���Ա������Ϣ�ط�
			s32Ret = SN_SLAVE_MSG;
			return s32Ret;
		}
#endif
	}
	else
	{
		//ʱ��λ��
	/*	s32Ret = OSD_Set_Time_xy(param->chn-1,param->osd_info.ChannelTimePosition_x,param->osd_info.ChannelTimePosition_y);
		if(s32Ret != HI_SUCCESS)
		{
			rsp->result = -1;
			RET_FAILURE("");
		}
		//ͨ������λ��
		s32Ret = OSD_Set_CH_xy(param->chn-1,param->osd_info.ChannelNamePosition_x,param->osd_info.ChannelNamePosition_y);
		if(s32Ret != HI_SUCCESS)
		{
			rsp->result = -1;
			RET_FAILURE("");
		}*/
		s32Ret = OSD_Set_xy(param->chn-1,param->osd_info.ChannelNamePosition_x,param->osd_info.ChannelNamePosition_y,param->osd_info.ChannelTimePosition_x,param->osd_info.ChannelTimePosition_y);
		if(s32Ret != HI_SUCCESS)
		{
			rsp->result = -1;
			RET_FAILURE("");
		}
#if defined(SN_SLAVE_ON)		
		if(param->chn > PRV_CHAN_NUM)
		{//���ͨ�����ڴ�Ƭ�ϣ�������Ϣ����Ƭ
			Prv_Slave_Set_chn_osd_Req slave_req;
			slave_req.osd_info= param->osd_info;
			slave_req.chn = param->chn;
			
			SN_SendMccMessageEx(PRV_SLAVE_1,msg_req->user, msg_req->source, MOD_PRV, msg_req->xid, msg_req->thread, MSG_ID_PRV_MCC_SET_CHN_OSD_REQ, &slave_req, sizeof(slave_req));
			//������ʱ��
			TimerReset(s_State_Info.f_timer_handle,10);
			TimerResume(s_State_Info.f_timer_handle,0);
			PRV_Msg_Cpy(msg_req);		//���浱ǰ��Ϣ���Ա������Ϣ�ط�
			s32Ret = SN_SLAVE_MSG;
			return s32Ret;
		}
#endif	
		//rsp->chn = param->chn-1;
	}
	rsp->result = 0;
	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_MSG_MCC_SetChnOsd_Rsp
Description: //��Ƭ����ͨ��OSD������Ϣ����
Calls: 
Called By: //
Input: // // msg_req:ͨ��OSD����������Ϣ�ṹ��
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: //
***********************************************************************/
STATIC HI_S32 PRV_MSG_MCC_SetChnOsd_Rsp(const Prv_Slave_Set_chn_osd_Rsp *slave_rsp, Set_chn_osd_Rsp *rsp)
{
	
	if (NULL == slave_rsp || NULL == rsp)
	{
		RET_FAILURE("NULL pointer!!!");
	}
	rsp->chn = slave_rsp->chn;
	rsp->result = -1;
	rsp->osd_info = slave_rsp->osd_info;
	
	//TODO
	//TODO
	//�����Ƭ���أ���ô��λ��Ƭ�ķ��ر�־λ
	s_State_Info.bIsReply |= 1<<(slave_rsp->slaveid-1);
	if(slave_rsp->result == SN_RET_OK)
	{	//�����Ƭ������ȷֵ
		s_State_Info.g_slave_OK = 1<<(slave_rsp->slaveid-1);
	}
	//�������ֵ��1 ����1����оƬ��������ֵ����ô��ʾ���д�Ƭ��������
	if((s_State_Info.bIsReply+1) == 1<<(PRV_CHIP_NUM-1))	
	{
		if((s_State_Info.g_slave_OK+1) == 1<<(PRV_CHIP_NUM-1))
		{//������ص���Ϣ��û�д�����Ϣ
			rsp->result = 0;
		}
		s_State_Info.TimeoutCnt = HI_FALSE;//������λ��ʱ��־λ
		TimerPause(s_State_Info.f_timer_handle); //��ͣ��ʱ��
		s_State_Info.bIsReply = 0;//�ظ�״̬�˳�
		s_State_Info.bIsSlaveConfig = HI_FALSE;	//������λ���ñ�־λ
		s_State_Info.g_slave_OK = 0;	//���ûظ���ֵ��־λ
		if(s_State_Info.Prv_msg_Cur)
		{
			SN_FREE(s_State_Info.Prv_msg_Cur);//�ͷ�����ɵ���Ϣ�ṹ��ָ��
		}
	}
	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_MSG_SetVoPreview
Description: //����豸Ԥ������
Calls: 
Called By: //
Input: // // param:Ԥ���������������Ϣ�ṹ��
			rsp: �ظ���Ϣ�ṹ��
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: //
***********************************************************************/
STATIC HI_S32 PRV_MSG_SetVoPreview(const SN_MSG *msg_req, Set_vo_preview_Rsp *rsp)
{
	HI_S32 i=0;
	unsigned char vodev = 0,vodev1=0;
	Set_vo_preview_Req *param = (Set_vo_preview_Req*)msg_req->para;
	
	if (NULL == param || NULL == rsp)
	{
		RET_FAILURE("NULL pointer!!!");
	}
	
	rsp->dev = param->dev;
	rsp->result = -1;
	rsp->preview_info = param->preview_info;
	rsp->preview_info_exp=param->preview_info_exp;
	vodev = param->dev;
	
#if defined(SN9234H1)

	vodev1 = vodev;
again:	
	if(vodev == SPOT_VO_DEV|| vodev == AD)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}
	if (vodev >= PRV_VO_MAX_DEV)
		
#else

	if(vodev == DHD1)
		vodev = DSD0;
	vodev1 = vodev;
again:	
	if (vodev > DHD0)
		
#endif		
	{
		RET_FAILURE("param->dev out off range: 0,2");
	}	
	s_astVoDevStatDflt[vodev].enPreviewMode = param->preview_info.PreviewMode;
	/*����Ԥ��ͨ��˳��*/
	for (i = 0; i < g_Max_Vo_Num; i++)
	{	
		//����Ԥ��������˳��
		s_astVoDevStatDflt[vodev].as32ChnOrder[SingleScene][i] = UCHAR2INIT32(param->preview_info.SingleOrder[i]);
		//����Ԥ��4����˳��
		s_astVoDevStatDflt[vodev].as32ChnOrder[FourScene][i] = UCHAR2INIT32( param->preview_info.FourOrder[i]);
		//����Ԥ��8����˳��
		s_astVoDevStatDflt[vodev].as32ChnOrder[EightScene][i] = UCHAR2INIT32( param->preview_info.EightOrder[i]);
		//����Ԥ��16����˳��
		s_astVoDevStatDflt[vodev].as32ChnOrder[SixteenScene][i] = UCHAR2INIT32( param->preview_info.SixteenOrder[i]);
		//���õ�������ѯͨ��˳��
		s_astVoDevStatDflt[vodev].as32ChnpollOrder[SingleScene][i] = UCHAR2INIT32( param->preview_info.SingleSel[i]);
		//����4������ѯͨ��˳��
		s_astVoDevStatDflt[vodev].as32ChnpollOrder[FourScene][i] = UCHAR2INIT32( param->preview_info.FourSel[i/4]);
		//����8������ѯͨ��˳��
		s_astVoDevStatDflt[vodev].as32ChnpollOrder[EightScene][i] = UCHAR2INIT32( param->preview_info.EightSel[i/8]);

	}
	for (i = 0; i < THREEINDEX; i++)
	{	
		//����Ԥ��3����˳��
		s_astVoDevStatDflt[vodev].as32ChnOrder[ThreeScene][i] = UCHAR2INIT32(param->preview_info_exp.ThreeOrder[i]);
		//����3������ѯͨ��˳��
		s_astVoDevStatDflt[vodev].as32ChnpollOrder[ThreeScene][i] = UCHAR2INIT32( param->preview_info_exp.ThreeSel[i/3]);
	}
	for (i = 0; i < FIVEINDEX; i++)
	{	
		 //����Ԥ��5����˳��
		s_astVoDevStatDflt[vodev].as32ChnOrder[FiveScene][i] = UCHAR2INIT32(param->preview_info_exp.FiveOrder[i]);
		 //����5������ѯͨ��˳��
		s_astVoDevStatDflt[vodev].as32ChnpollOrder[FiveScene][i] = UCHAR2INIT32( param->preview_info_exp.FiveSel[i/5]);
	}
	for (i = 0; i < SEVENINDEX; i++)
	{	
		 //����Ԥ��7����˳��
		s_astVoDevStatDflt[vodev].as32ChnOrder[SevenScene][i] = UCHAR2INIT32(param->preview_info_exp.SevenOrder[i]);
         //����7������ѯͨ��˳��
		s_astVoDevStatDflt[vodev].as32ChnpollOrder[SevenScene][i] = UCHAR2INIT32( param->preview_info_exp.SevenSel[i/7]);
	}
	for (i = 0; i < SIXINDEX; i++)
	{	
		//����Ԥ��6����˳��
		s_astVoDevStatDflt[vodev].as32ChnOrder[SixScene][i] = UCHAR2INIT32( param->preview_info.SixOrder[i]);
		//����6������ѯͨ��˳��
		s_astVoDevStatDflt[vodev].as32ChnpollOrder[SixScene][i] = UCHAR2INIT32( param->preview_info.Sixel[i/6]);
		//printf("#############s_astVoDevStatDflt[param->dev].as32ChnOrder[SixScene][i] = %d##############\n",s_astVoDevStatDflt[vodev].as32ChnOrder[SixScene][i]);
		//printf("#############s_astVoDevStatDflt[param->dev].as32ChnpollOrder[SixScene][i] = %d##############\n",s_astVoDevStatDflt[vodev].as32ChnpollOrder[SixScene][i]);
	}
	for (i = 0; i < NINEINDEX; i++)
	{	
		//����Ԥ��9����˳��
		s_astVoDevStatDflt[vodev].as32ChnOrder[NineScene][i] = UCHAR2INIT32( param->preview_info.NineOrder[i]);
		//����9������ѯͨ��˳��
		s_astVoDevStatDflt[vodev].as32ChnpollOrder[NineScene][i] = UCHAR2INIT32( param->preview_info.NineSel[i/9]);	
		//printf("#############s_astVoDevStatDflt[param->dev].as32ChnOrder[NineScene][i] = %d##############\n",s_astVoDevStatDflt[vodev].as32ChnOrder[NineScene][i]);
		//printf("#############s_astVoDevStatDflt[param->dev].as32ChnpollOrder[NineScene][i] = %d##############\n",s_astVoDevStatDflt[vodev].as32ChnpollOrder[NineScene][i]);
	}

#if defined(SN9234H1)
/*2010-11-4 ͨ����ʾ������*/
	PRV_SortChnOrder(param->dev);
	if(vodev1 ==  vodev && vodev != SPOT_VO_DEV)
	{
		vodev =  (param->dev == HD)? s_VoSecondDev: HD;
		goto again;
	}	
#else
	/*�����豸��Ԥ��ģʽ*/
	if(vodev1 == vodev)
	{
		vodev = (vodev == DHD0) ? DSD0 : DHD0;
		goto again;
	}	
#endif

#if defined(SN_SLAVE_ON)
	//������Ϣ����Ƭ
	{
		Prv_Slave_Set_vo_preview_Req slave_req;
		int ret=0;
		
		slave_req.preview_info = param->preview_info;
		slave_req.preview_info_exp = param->preview_info_exp;
		slave_req.dev = rsp->dev;
		
		ret = SN_SendMccMessageEx(PRV_SLAVE_1,msg_req->user, msg_req->source, MOD_PRV, msg_req->xid, msg_req->thread, MSG_ID_PRV_MCC_SET_VO_PREVIEW_REQ, &slave_req, sizeof(Prv_Slave_Set_vo_preview_Req));
	}		
#endif
	rsp->result = 0;
	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_MSG_SetVoPreview_Adv
Description: //����豸Ԥ������
Calls: 
Called By: //
Input: // // param:Ԥ������߼�����������Ϣ�ṹ��
			rsp: �ظ���Ϣ�ṹ��
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: //
***********************************************************************/
STATIC HI_S32 PRV_MSG_SetVoPreview_Adv(const Set_vo_preview_Adv_Req *param, Set_vo_preview_Adv_Rsp *rsp)
{
	VO_DEV VoDev;

	if (NULL == param || NULL == rsp)
	{
		RET_FAILURE("NULL pointer!!!");
	}
	rsp->dev = param->dev;
	rsp->result = -1;
	rsp->preview_adv_info = param->preview_adv_info;
	
	/*���ñ��������˿�*/
	switch(param->preview_adv_info.AlarmHandlePort)/*���������˿ڶ�Ӧ0-DHD0/VGA, 2-DSD0/CVBS2*/
	{
#if defined(SN9234H1)
		case 0:
			VoDev = HD;
			break;
		case 1:
			VoDev = s_VoSecondDev;
			break;
		case 2:
			VoDev = SD;
			break;
#else
		case 0:
			VoDev = DHD0;
			break;
		case 1:
			VoDev = DSD0;
			break;
		case 2:
			VoDev = DSD0;
			break;
#endif			
		default:
			RET_FAILURE("param->preview_info.AlarmHandlePort out off range: 0~2");
	}
#if defined(SN9234H1)
	if (VoDev != s_VoDevAlarmDflt && (param->dev == HD || param->dev == s_VoSecondDev))/*Ŀǰֻ��ȡHD�ڣ���SD���еı��������˿����ý������ԣ���¼�еı��������˿�������Ϊ��Ч�ı��������˿�*/
#else
	if (VoDev != s_VoDevAlarmDflt && (param->dev == DHD0 || param->dev == s_VoSecondDev))
#endif		
	{
		if (s_astVoDevStatDflt[s_VoDevAlarmDflt].bIsAlarm)
		{
			PRV_AlarmOff(s_VoDevAlarmDflt);
			PRV_AlarmChn(VoDev, s_astVoDevStatDflt[s_VoDevAlarmDflt].s32AlarmChn);
		}
		s_VoDevAlarmDflt = VoDev;
	}
	/*����Ԥ����Ƶ*/
	PRV_Set_AudioPreview_Enable(param->preview_adv_info.AudioPreview);	
	IsAudioOpen = param->preview_adv_info.AudioPreview[0];
	if(!IsAudioOpen)//�ر���Ƶ�ܿ��غ�����״̬
	{
		CurAudioChn = -1;
		PreAudioChn = -1;
		CurAudioPlayStat = 0;
		IsChoosePlayAudio = 0;
		Achn = -1;
	}
	else
	{		
		PRV_PlayAudio(VoDev);
	}
	rsp->result = 0;
	RET_SUCCESS("");
}
/*************************************************
Function: //PRV_MSG_SetPreview_Audio
Description: //����豸Ԥ����Ƶӳ������
Calls: 
Called By: //
Input: // // param:Ԥ������߼�����������Ϣ�ṹ��
			rsp: �ظ���Ϣ�ṹ��
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: //
***********************************************************************/
STATIC HI_S32 PRV_MSG_SetPreview_Audio(const Set_preview_Audio_Req *param, Set_preview_Audio_Rsp *rsp)
{
	//int i=0,j=0;
	//VO_CHN chn;
	
	if (NULL == param || NULL == rsp)
	{
		RET_FAILURE("NULL pointer!!!");
	}
	rsp->result = -1;
	rsp->preview_audio_info = param->preview_audio_info;
	//������Ƶ���ù�ϵ
	//PRV_Set_AudioMap(param->preview_audio_info.AudioMap);
	CHECK(PRV_PlayAudio(s_VoDevCtrlDflt));
	rsp->result = 0;
	RET_SUCCESS("");

}

//��ȡGUI���������
STATIC HI_S32 PRV_MSG_GetGuiVo(Get_gui_vo_Rsp *rsp)
{
	if (NULL == rsp)
	{
		RET_FAILURE("NULL pointer!!!");
	}

	rsp->dev = s_VoDevCtrlDflt;
	
	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_GetGuiVo
Description: //��ȡGUI���������
Calls: 
Called By: //
Input: // // pdev_id:�����豸ID
Output: // 
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: //�ط�ģ��ʹ��
***********************************************************************/
int PRV_GetGuiVo(int *pdev_id )
{
	if (NULL == pdev_id)
	{
		RET_FAILURE("NULL pointer!!!");
	}

	*pdev_id = s_VoDevCtrlDflt;
	
	RET_SUCCESS("");
}


/*************************************************
Function: //PRV_MSG_ChnZoomIn
Description: //ͨ�����ӷŴ���Ϣ����
Calls: 
Called By: //
Input: // // msg_req:ͨ��OSD����������Ϣ�ṹ��
		rsp : �ظ���GUI����Ϣ
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: //
***********************************************************************/
STATIC HI_S32 PRV_MSG_ChnZoomIn(const SN_MSG *msg_req, Msg_id_prv_Rsp *rsp)
{
	Chn_zoom_in_Req *param = (Chn_zoom_in_Req *)msg_req->para;
	
	rsp->chn = param->chn;
	rsp->dev = 0;//param->dev;
	rsp->flag = 0;
	rsp->result = -1;
	
	if (NULL == param || NULL == rsp)
	{
		RET_FAILURE("NULL pointer!!!");
	}
	//CHECK_RET(PRV_ChnZoomIn(param->dev,param->chn, param->ratio, &param->point));
	CHECK_RET(PRV_ChnZoomIn(param->chn, param->ratio, &param->point));

	rsp->result = 0;
	RET_SUCCESS("");
}

HI_S32 PRV_GetVoChnAttr_ForPB(VO_DEV VoDev, VO_CHN VoChn, RECT_S *pstRect)
{
	HI_S32 div = 0, s32ChnCnt = 0;
	//VO_CHN_ATTR_S stChnAttr;
	//SIZE_S disp_size,img_size;	
	g_PlayInfo stPlayInfo;
	int u32Width = 1040, u32Height = 571;
	int u32Width_s = 0,u32Height_s = 0;
	PlayBack_GetPlaySize((HI_U32 *)&u32Width, (HI_U32 *)&u32Height);

	PRV_GetPlayInfo(&stPlayInfo);
	s32ChnCnt = stPlayInfo.ImagCount;
    u32Width_s = u32Width;
	u32Height_s = u32Height;
    if(s32ChnCnt == 9)
    {
		while(u32Width%6 != 0)
			u32Width++;
		while(u32Height%6 != 0)
			u32Height++;
	}

	div = sqrt(s32ChnCnt);		/* ����ÿ��ͨ���Ŀ�Ⱥ͸߶� */
	if(stPlayInfo.bISDB==1 || stPlayInfo.IsZoom==1)
	{
        pstRect->s32X = 0;
		pstRect->s32Y = 0;
		pstRect->u32Height = u32Height;
		pstRect->u32Width = u32Width;
		RET_SUCCESS("");
	}
	if(div == 1)
	{
		pstRect->s32X = 0;
		pstRect->s32Y = 0;
		pstRect->u32Height = u32Height;
		pstRect->u32Width = u32Width;
		RET_SUCCESS("");
	}
	else if(div == 2)
	{
		switch(VoChn)
		{
			case 0:
				pstRect->s32X = 0;
				pstRect->s32Y = 0;
				pstRect->u32Height = u32Height/2;
				pstRect->u32Width = u32Width/2;
				break;
			case 1:
				pstRect->s32X = u32Width/2;
				pstRect->s32Y = 0;
				pstRect->u32Height = u32Height/2;
				pstRect->u32Width = u32Width/2;
				break;
			case 2:
				pstRect->s32X = 0;
				pstRect->s32Y = u32Height/2;
				pstRect->u32Height = u32Height/2;
				pstRect->u32Width = u32Width/2;
				break;
			case 3:
				pstRect->s32X = u32Width/2;
				pstRect->s32Y = u32Height/2;
				pstRect->u32Height = u32Height/2;
				pstRect->u32Width = u32Width/2;
				break;
		}
	}
	else if(div == 3)
	{
		switch(VoChn)
		{
			case 0:
				pstRect->s32X = 0;
				pstRect->s32Y = 0;
				pstRect->u32Height = u32Height/3;
				pstRect->u32Width = u32Width/3;
				break;
			case 1:
				pstRect->s32X = u32Width/3;
				pstRect->s32Y = 0;
				pstRect->u32Height = u32Height/3;
				pstRect->u32Width = u32Width/3;
				break;
			case 2:
				pstRect->s32X = u32Width*2/3;
				pstRect->s32Y = 0;
				pstRect->u32Height = u32Height/3;
				pstRect->u32Width = u32Width/3;
				break;
			case 3:
				pstRect->s32X = 0;
				pstRect->s32Y = u32Height/3;
				pstRect->u32Height = u32Height/3;
				pstRect->u32Width = u32Width/3;
				break;
			case 4:
				pstRect->s32X = u32Width/3;
				pstRect->s32Y = u32Height/3;
				pstRect->u32Height = u32Height/3;
				pstRect->u32Width = u32Width/3;
				break;
			case 5:
				pstRect->s32X = u32Width*2/3;
				pstRect->s32Y = u32Height/3;
				pstRect->u32Height = u32Height/3;
				pstRect->u32Width = u32Width/3;
				break;
			case 6:
				pstRect->s32X = 0;
				pstRect->s32Y = u32Height*2/3;
				pstRect->u32Height = u32Height/3;
				pstRect->u32Width = u32Width/3;
				break;
			case 7:
				pstRect->s32X = u32Width/3;
				pstRect->s32Y = u32Height*2/3;
				pstRect->u32Height = u32Height/3;
				pstRect->u32Width = u32Width/3;
				break;
			case 8:
				pstRect->s32X = u32Width*2/3;
				pstRect->s32Y = u32Height*2/3;
				pstRect->u32Height = u32Height/3;
				pstRect->u32Width = u32Width/3;
				break;
		}
        if(s32ChnCnt == 9)
        {
		   if((VoChn + 1) % 3 == 0)//���һ��
			  pstRect->u32Width = u32Width_s- pstRect->s32X;
		   if(VoChn> 5 && VoChn< 9)//���һ��
		      pstRect->u32Height = u32Height_s- pstRect->s32Y;
        }
	}

#if 0	
	s32Ret = HI_MPI_VO_GetChnAttr(VoDev, VoChn, &stChnAttr);
	printf("pstRect.s32X=%d, s32Y=%d, u32Width=%d, u32Height=%d\n", stChnAttr.stRect.s32X, stChnAttr.stRect.s32Y, stChnAttr.stRect.u32Width, stChnAttr.stRect.u32Height);

	CHECK_RET(PRV_GetVoDevDispSize(VoDev,&disp_size));
	printf("disp_size.u32Width=%d, u32Height=%d\n", disp_size.u32Width, disp_size.u32Height);

	CHECK_RET(PRV_GetVoDevImgSize(VoDev,&img_size));
	printf("img_size.u32Width=%d, u32Height=%d\n", img_size.u32Width, img_size.u32Height);
	
	pstRect->s32X = stChnAttr.stRect.s32X*disp_size.u32Width/img_size.u32Width;
	pstRect->s32Y = stChnAttr.stRect.s32Y*disp_size.u32Height/img_size.u32Height;
	pstRect->u32Height= stChnAttr.stRect.u32Height*disp_size.u32Height/img_size.u32Height;
	pstRect->u32Width= stChnAttr.stRect.u32Width*disp_size.u32Width/img_size.u32Width;
#endif	

	RET_SUCCESS("");
}

int PRV_GetVoChn_ForPB(Preview_Point stPoint)
{
	HI_S32 i = 0, s32ChnCnt = 0, div = 0;
	g_PlayInfo stPlayInfo;
	RECT_S *pstLayout = NULL;
	int u32Width = 1040, u32Height = 571;

	PlayBack_GetPlaySize((HI_U32 *)&u32Width, (HI_U32 *)&u32Height);
	
	PRV_GetPlayInfo(&stPlayInfo);
	s32ChnCnt = stPlayInfo.ImagCount;

	div = sqrt(s32ChnCnt);		/* ����ÿ��ͨ���Ŀ�Ⱥ͸߶� */
	
	if(div == 1)
	{
		pstLayout = s_astPreviewLayout1;
	}
	else if(div == 2)
	{
		pstLayout = s_astPreviewLayout4;
	}
	else
	{
		pstLayout = s_astPreviewLayout9;
	}
	
	//���ҷ��ϵ�ǰλ�õ�ͨ������ID
	if(stPlayInfo.bISDB==1)
	{
        return stPlayInfo.DBClickChn;
	}
	for (i = 0; i<s32ChnCnt; i++)
	{
		if (
			(pstLayout[i].s32X * u32Width) / (PRV_PREVIEW_LAYOUT_DIV) <= stPoint.x
			&& (pstLayout[i].s32Y * u32Height) / PRV_PREVIEW_LAYOUT_DIV <= stPoint.y
			&& ((pstLayout[i].s32X + pstLayout[i].u32Width) * u32Width) / PRV_PREVIEW_LAYOUT_DIV >= stPoint.x
			&& ((pstLayout[i].s32Y + pstLayout[i].u32Height) * u32Height) / PRV_PREVIEW_LAYOUT_DIV >= stPoint.y
			)
		{
			return i;
		}
	}
	

	return -1;
	
}
/*************************************************
Function: //PRV_MSG_GetChnByXY
Description: //��ȡ����λ�����ڵ�ͨ����
Calls: 
Called By: //
Input: // // param:��ȡ����λ��������Ϣ�ṹ��
			rsp: �ظ���Ϣ�ṹ��
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: //
***********************************************************************/
int PRV_MSG_GetChnByXY(const Get_chn_by_xy_Req *param, Get_chn_by_xy_Rsp *rsp)
{
	Preview_Point stPoint;
	HI_U32 u32Index = 0, i = 0,u32ChnNum = 0;
	VO_CHN VoChn = 0;
	RECT_S rect;
	RECT_S pstRect;
	//HI_U32 Max_num;
	unsigned char mode =s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewMode;
	if (NULL == param || NULL == rsp)
	{
		RET_FAILURE("NULL pointer!!!");
	}
	rsp->x = param->x;
	rsp->y = param->y;
	rsp->w = 0;
	rsp->h = 0;
	rsp->chn = 0;
#if defined(SN9234H1)
	rsp->dev = (s_VoDevCtrlDflt == HD)? HD : AD;
#else
	rsp->dev = (s_VoDevCtrlDflt == DHD0)? DHD0 : DSD0;
#endif
	rsp->result = -1;
	stPoint.x = param->x;
	stPoint.y = param->y;
	if(s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat != PRV_STAT_NORM)
	{
		if(s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat == PRV_STAT_PB)
		{
			g_ChnPlayStateInfo stPlayStateInfo;
      
			VoChn = PRV_GetVoChn_ForPB(stPoint);
			PRV_GetVoChnPlayStateInfo(VoChn, &stPlayStateInfo);
			if(PlayBack_QueryPbStat(VoChn)==HI_FAILURE)
	        {
	            rsp->result = -1;
                return HI_FAILURE;
		    }
			if(VoChn < 0)
			{
				RET_FAILURE("VoChn=-1, error");
			}

			PRV_GetVoChnAttr_ForPB(DHD0, VoChn, &pstRect);
			
			rsp->chn = VoChn;
			rsp->x = pstRect.s32X;
			rsp->y = pstRect.s32Y;
			rsp->w = pstRect.u32Width;
			rsp->h = pstRect.u32Height;
			
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "VoChn=%d, pstRect.s32X=%d, s32Y=%d, u32Width=%d, u32Height=%d\n", VoChn, pstRect.s32X, pstRect.s32Y, pstRect.u32Width, pstRect.u32Height);

			rsp->result = 0;
			RET_SUCCESS("");
		}
		else
		{
			RET_FAILURE("not In PB or PIC or norm, Don't Get Rect By XY");
		}

	}
	
	if(s_astVoDevStatDflt[s_VoDevCtrlDflt].bIsAlarm)
	{//����״̬
		VoChn = s_astVoDevStatDflt[s_VoDevCtrlDflt].s32AlarmChn;
	}
	else if(param->reserve[0] > 0)//ֱ�Ӵ�ͨ����
	{
		if(param->reserve[0] <= DEV_CHANNEL_NUM)
		{
			VoChn = param->reserve[0] - 1;
			if(PRV_VoChnIsInCurLayOut(s_VoDevCtrlDflt, VoChn) == HI_FAILURE)
			{
				RET_FAILURE("VoChn No In Current Layout!!!");			
			}
		}
		else
			RET_FAILURE("Invalid Parameter: VoChn!!!");			
	}
	else
	{
		CHECK_RET(PRV_Point2Index(&stPoint, &u32Index, s_astVoDevStatDflt[s_VoDevCtrlDflt].as32ChnOrder[mode]));
		if(s_astVoDevStatDflt[s_VoDevCtrlDflt].bIsSingle && (mode != SingleScene) && (s_astVoDevStatDflt[s_VoDevCtrlDflt].s32DoubleIndex == 0))
		{
			mode = SingleScene;
		}
		VoChn = s_astVoDevStatDflt[s_VoDevCtrlDflt].as32ChnOrder[mode][u32Index];
		
#if defined(Hi3531)||defined(Hi3535)
		if(PRV_CurDecodeMode == PassiveDecode || ScmGetListCtlState() == 1)
		{
			VoChn = u32Index;
		}
#endif
	}
	if (VoChn < 0 || VoChn >= g_Max_Vo_Num)
	{//�����ǰ��ͨ��Ϊ����ͨ������ô��Ҫ����ѡ�񽹵�λ��
		switch(mode)
		{
			case SingleScene:
				u32ChnNum = 1;
				break;
			case TwoScene:
				u32ChnNum = 2;
				break;
			case ThreeScene:
				u32ChnNum = 3;
				break;
			case FourScene:
			case LinkFourScene:
				u32ChnNum = 4;
				break;
			case FiveScene:
				u32ChnNum = 5;
				break;
			case SixScene:
				u32ChnNum = 6;
				break;
			case SevenScene:
				u32ChnNum = 7;
				break;
			case EightScene:
				u32ChnNum = 8;
				break;
			case NineScene:
			case LinkNineScene:
				u32ChnNum = 9;
				break;
			case SixteenScene:
				u32ChnNum = 16;
				break;
			default:
				RET_FAILURE("Invalid Parameter: enPreviewMode");
		}
		for(i = 0; i < u32ChnNum; i++)
		{//�����ǰͨ�����أ�������һ��ͨ����ֱ��û��ͨ�����أ���ô���ش���
			if((u32Index+i) >= ((u32Index/u32ChnNum+1)*u32ChnNum))
			{
				u32Index = (u32Index/u32ChnNum)*u32ChnNum - i;
			}
			VoChn = s_astVoDevStatDflt[s_VoDevCtrlDflt].as32ChnOrder[mode][u32Index+i];

			if (VoChn < 0 || VoChn >= g_Max_Vo_Num)
			{
				continue;
			}
			else
			{
				break;
			}
		}
		if(i >= u32ChnNum)
		{//���i����ֵ�������ֵ��˵��û���ҵ�����ô���ش���
			RET_FAILURE("chn id hiden!!");
		}
	}
	CHECK_RET(PRV_GetVoChnDispRect_Forxy(s_VoDevCtrlDflt,VoChn, &rect));
	if(LayoutToSingleChn >= 0)
	{
		VoChn = LayoutToSingleChn;
		rect.s32X = 0;
		rect.s32Y = 0;
		rect.u32Height = s_astVoDevStatDflt[s_VoDevCtrlDflt].stVideoLayerAttr.stDispRect.u32Height;
		rect.u32Width = s_astVoDevStatDflt[s_VoDevCtrlDflt].stVideoLayerAttr.stDispRect.u32Width;			
	}
	TRACE(SCI_TRACE_NORMAL, MOD_PRV, "--------enPreviewStat: %d, enPreviewMode: %d, VoChn: %d, X: %d, Y: %d, Width: %d, Height: %d\n",
		s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat, s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewMode, VoChn, rect.s32X, rect.s32Y, rect.u32Width, rect.u32Height);
	rsp->chn = VoChn;
	rsp->x = rect.s32X;
	rsp->y = rect.s32Y;
	rsp->w = rect.u32Width;
	rsp->h = rect.u32Height;
	rsp->result = 0;
	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_MSG_GetPrvMode
Description: //��ȡGUI��������豸��Ԥ��ģʽ
Calls: 
Called By: //
Input: // // rsp: �ظ���Ϣ�ṹ��
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: //
***********************************************************************/
STATIC HI_S32 PRV_MSG_GetPrvMode(Get_prv_mode_Rsp *rsp)
{
	if (NULL == rsp)
	{
		RET_FAILURE("NULL pointer!!!");
	}

	rsp->result = -1;

	CHECK_RET(PRV_GetPrvMode(&rsp->prv_mode));

	rsp->result = 0;
	RET_SUCCESS("");
}

/*************************************************
Function: //PRV_MSG_GetRec_Resolution
Description: //��ȡGUI��������豸��Ԥ��ģʽ
Calls: 
Called By: //
Input: // // req: ¼��ֱ����޸�֪ͨ�ṹ��
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: //
***********************************************************************/
STATIC HI_S32 PRV_MSG_GetRec_Resolution(const SN_MSG *msg_req)
{
	Prv_Rec_Osd_Resolution_Ind *req = (Prv_Rec_Osd_Resolution_Ind*)msg_req->para;
	if (NULL == req)
	{
		RET_FAILURE("NULL pointer!!!");
	}
    OSD_Get_Rec_Range_Ch(req->rec_group,req->chn,req->w,req->h);;
	if(req->chn >= PRV_CHAN_NUM)
	{//���ͨ�����ڴ�Ƭ������Ϣ����Ƭ
		SN_SendMccMessageEx(PRV_SLAVE_1,msg_req->user, msg_req->source, MOD_PRV, msg_req->xid, msg_req->thread, MSG_ID_PRV_GET_REC_OSDRES_IND, (void*)msg_req->para, msg_req->size);		
	}
	RET_SUCCESS("");
}

/********************************************************
������:PRV_WaitDestroyVdecChn
��     ��:����ָ������ͨ��
��     ��:[in]VdecChn  ָ������ͨ��?
����ֵ:  0�ɹ�
		    -1ʧ��

*********************************************************/
HI_S32 PRV_WaitDestroyVdecChn(HI_S32 VdecChn)
{
#if defined(SN9234H1)
	HI_S32 i = 0, index = 0, s32Ret = 0;
	if(VdecChn < 0/* || VdecChn == DetVLoss_VdecChn*/)
#else
	HI_S32 index = 0, s32Ret = 0;
	if(VdecChn < 0 || VdecChn == DetVLoss_VdecChn)
#endif		
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV,"-------Invalid VdecChn = %d\n ", VdecChn);  
		return HI_FAILURE;
	}
	
	index = PRV_GetVoChnIndex(VdecChn);
	if((VdecChn != DetVLoss_VdecChn && VdecChn != NoConfig_VdecChn) && index < 0)
	{
		RET_FAILURE("------ERR: index");
	}
	CHECK(HI_MPI_VDEC_StopRecvStream(VdecChn));//����ͨ��ǰ��ֹͣ��������
	
	if(VdecChn != DetVLoss_VdecChn && VdecChn != NoConfig_VdecChn)//JPEG��֧��reset����
		CHECK(HI_MPI_VDEC_ResetChn(VdecChn)); //��������λ 
	
	if((VdecChn != DetVLoss_VdecChn && VdecChn != NoConfig_VdecChn)
		&& s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat != PRV_STAT_PB
		&& s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat != PRV_STAT_PIC)
	{
		s32Ret = PRV_VoChnIsInCurLayOut(s_VoDevCtrlDflt, VochnInfo[index].VoChn);
	}
	
	if(VdecChn != DetVLoss_VdecChn && VdecChn != NoConfig_VdecChn)
	{		
#if defined(SN9234H1)
		for(i = 0; i < PRV_VO_DEV_NUM; i++)
		{			
			if(i == AD || i == SPOT_VO_DEV)
				continue;
			//if(VochnInfo[index].IsBindVdec[i] == 1)
			{
				if(i == SPOT_VO_DEV)
				{
					if(VochnInfo[index].VoChn == s_astVoDevStatDflt[SPOT_VO_DEV].as32ChnOrder[SingleScene][s_astVoDevStatDflt[SPOT_VO_DEV].s32SingleIndex])
					{
						(HI_MPI_VDEC_UnbindOutputChn(VdecChn, SPOT_VO_DEV, SPOT_VO_CHAN));/* ���ͨ�� */
						CHECK(HI_MPI_VO_ClearChnBuffer(SPOT_VO_DEV, SPOT_VO_CHAN, 1)); /* ���VO���� */
					}
						
				}
				else 
				{
					(HI_MPI_VDEC_UnbindOutputChn(VdecChn, i, VdecChn));/* ���ͨ�� */
					CHECK(HI_MPI_VO_ClearChnBuffer(i, VochnInfo[index].VoChn, 1)); /* ���VO���� */
					//ֻ���ڿ���״̬����Ҫ�Կ���ͨ��������Ӧ����
					if(s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat == PRV_STAT_CTRL
						&& (s_astVoDevStatDflt[s_VoDevCtrlDflt].enCtrlFlag == PRV_CTRL_REGION_SEL || s_astVoDevStatDflt[HD].enCtrlFlag == PRV_CTRL_ZOOM_IN)
						&& s32Ret == HI_SUCCESS && i == s_VoDevCtrlDflt)
					{
						(HI_MPI_VDEC_UnbindOutputChn(VdecChn, i, PRV_CTRL_VOCHN));							
						CHECK(HI_MPI_VO_ClearChnBuffer(i, PRV_CTRL_VOCHN, HI_TRUE)); /* ���VO���� */
						
					}
				}
				
				VochnInfo[index].IsBindVdec[i] = -1;
			}
			
		}
#else
		CHECK(PRV_VDEC_UnBindVpss(VdecChn, VdecChn));// ���ͨ�� 
		CHECK(HI_MPI_VO_ClearChnBuffer(DHD0, VochnInfo[index].VoChn, 1)); 
		//CHECK(HI_MPI_VO_ClearChnBuffer(DSD0, VochnInfo[index].VoChn, 1)); 
		//ֻ���ڿ���״̬����Ҫ�Կ���ͨ��������Ӧ����
		if(s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat == PRV_STAT_CTRL
			&& (s_astVoDevStatDflt[s_VoDevCtrlDflt].enCtrlFlag == PRV_CTRL_REGION_SEL || s_astVoDevStatDflt[DHD0].enCtrlFlag == PRV_CTRL_ZOOM_IN)
			&& s32Ret == HI_SUCCESS)
		{
			CHECK(PRV_VDEC_UnBindVpss(VdecChn, PRV_CTRL_VOCHN));							
			CHECK(HI_MPI_VO_ClearChnBuffer(DHD0, PRV_CTRL_VOCHN, HI_TRUE)); 
			//CHECK(HI_MPI_VO_ClearChnBuffer(DSD0, PRV_CTRL_VOCHN, HI_TRUE)); 
			
		}				
		VochnInfo[index].IsBindVdec[DHD0] = -1;								
		//VochnInfo[index].IsBindVdec[DSD0] = -1;								
#endif		
		CHECK_RET(HI_MPI_VDEC_DestroyChn(VdecChn)); // ������Ƶͨ��
		VochnInfo[VdecChn].IsHaveVdec = 0;
	}

	return HI_SUCCESS;
}

/********************************************************
������:PRV_MasterChnReChooseSlave
��     ��:ԭ����ƬԤ����ͨ������ѡ��Ŵ�ƬԤ��
��     ��:[in]index:VochnInfo���±�
����ֵ:  

*********************************************************/

HI_VOID PRV_MasterChnReChooseSlave(int index)
{
	PRV_MccCreateVdecReq SlaveCreateVdecReq;
	//���ҵ��Ϻ�������ͨ���Ŵ�Ƭ
	PRV_WaitDestroyVdecChn(VochnInfo[index].VdecChn);
	VochnInfo[index].SlaveId = PRV_SLAVE_1;
	CurSlaveCap += VochnInfo[index].VdecCap;
	CurSlaveChnCount++;
	SlaveCreateVdecReq.s32StreamChnIDs = MasterToSlaveChnId;
	SlaveCreateVdecReq.EncType = VochnInfo[index].VideoInfo.vdoType;
	SlaveCreateVdecReq.chn = VochnInfo[index].CurChnIndex;
	SlaveCreateVdecReq.VoChn = VochnInfo[index].VoChn;
	SlaveCreateVdecReq.VdecChn = VochnInfo[index].VoChn;
	SlaveCreateVdecReq.VdecCap = VochnInfo[index].VdecCap;	
	SlaveCreateVdecReq.height = VochnInfo[index].VideoInfo.height;
	SlaveCreateVdecReq.width = VochnInfo[index].VideoInfo.width;		
	SlaveCreateVdecReq.SlaveId = PRV_SLAVE_1;
	TRACE(SCI_TRACE_NORMAL, MOD_PRV, "============Master Chn: %d---VdecCap: %d ReChoose Slave---\n", SlaveCreateVdecReq.VdecChn, VochnInfo[index].VdecCap);
	VochnInfo[index].MccCreateingVdec = 1;
#if defined(Hi3531)||defined(Hi3535)	
	MccCreateingVdecCount++;
#endif
	SN_SendMccMessageEx(SlaveCreateVdecReq.SlaveId, SUPER_USER_ID, MOD_PRV, MOD_PRV, 0, 0, MSG_ID_PRV_MCC_CREATEVDEC_REQ, &SlaveCreateVdecReq, sizeof(PRV_MccCreateVdecReq));					
	CurMasterCap -= VochnInfo[index].VdecCap;

}

/********************************************************
������:PRV_FindMasterChnReChooseSlave
��     ��:����Ƭ������Ҫ�ķŴ�Ƭ�����ͨ��
��     ��:[in]ExtraCap:��Ƭ��Ҫ�ó�������
		[in]index:��ǰ�����ͨ���±�(�ֱ��ʱ���ͨ��)
		[in]TmpIndex:��Ҫѡ��ķŴ�Ƭ�����ͨ����
����ֵ:  

*********************************************************/

HI_VOID PRV_FindMasterChnReChooseSlave(int ExtraCap, int index, int TmpIndex[])
{
	int i = 0, j = 0;
	if(ExtraCap == 1)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ExtraCap == 1,Need Find One Master Chn!\n");			
		//������Ƭ������Ϊ2��ͨ��
		for(i = 0; i < PRV_VO_CHN_NUM; i++)
		{
			if(VochnInfo[i].SlaveId == PRV_MASTER && VochnInfo[i].VdecCap == 1 && i != index)
			{
				TmpIndex[0] = i;
				break;	
			}
		}
	}
	//ExtraCap=2ʱ����Ҫ����Ƭ��1����2��ͨ���Ŵ�Ƭ����������Ϊ1����һ������Ϊ2
	else if(ExtraCap == 2)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ExtraCap == 2,Need Find Two or Three Master Chn!\n");			
		for(i = 0; i < PRV_VO_CHN_NUM; i++)
		{
			if(VochnInfo[i].SlaveId == PRV_MASTER && VochnInfo[i].VdecCap == 2 && i != index)
			{
				TmpIndex[0] = i;
				return;
			}
		}
		//��Ƭû������Ϊ2��ͨ������Ҫ��2��ͨ���Ŵ�Ƭ
		if(i == PRV_VO_CHN_NUM)
		{
			j = 0;
			for(i = 0; i < PRV_VO_CHN_NUM; i++)
			{
				//���Ҷ�����������Ϊ1��ͨ��
				if(VochnInfo[i].SlaveId == PRV_MASTER
					&& VochnInfo[i].VdecCap == 1
					&& i != index)
				{
					TmpIndex[j] = i;
					j++;
				}
				//�ҵ�2������Ϊ1��ͨ��
				if(TmpIndex[1] != -1)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ExtraCap == 3, Allready Find Three Master Chn: %d-%d-%d!\n", VochnInfo[TmpIndex[0]].VoChn, VochnInfo[TmpIndex[1]].VoChn, VochnInfo[TmpIndex[2]].VoChn);		
					break;
				}
			}

		}

	}
	//ExtraCap=3ʱ����Ҫ����Ƭ��2��ͨ���Ŵ�Ƭ��һ������Ϊ1��һ������Ϊ2
	//����Ҫ����Ƭ��3��ͨ���Ŵ�Ƭ����3��ͨ�����ܾ�Ϊ1
	else if(ExtraCap == 3)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ExtraCap == 3,Need Find Two or Three Master Chn!\n");			
		//������Ƭ������Ϊ2��ͨ��
		for(i = 0; i < PRV_VO_CHN_NUM; i++)
		{
			if(VochnInfo[i].SlaveId == PRV_MASTER && VochnInfo[i].VdecCap == 2 && i != index)
				break;
	
		}
		//��Ƭ������Ϊ2��ͨ������ֻ��Ҫ��2��ͨ���Ŵ�Ƭ
		if(i != PRV_VO_CHN_NUM)
		{
			int tempVdecCap1 = 0, tempVdecCap2 = 0;
			for(i = 0; i < PRV_VO_CHN_NUM; i++)
			{
				//���ҵ�һ����������(1��2)��ͨ��
				if(TmpIndex[0] == -1)
				{
					if(VochnInfo[i].SlaveId == PRV_MASTER
						&& VochnInfo[i].VdecCap < ExtraCap
						&& VochnInfo[i].IsHaveVdec
						&& i != index)
					{
						TmpIndex[0] = i;
						tempVdecCap1 = VochnInfo[i].VdecCap;
						TRACE(SCI_TRACE_NORMAL, MOD_PRV, "One: Find Master Chn: %d---Cap: %d!\n", VochnInfo[TmpIndex[0]].VoChn, VochnInfo[TmpIndex[0]].VdecCap);		
					}
				}
				//���ҵڶ����������ܵ�ͨ��
				else
				{
					if(VochnInfo[i].SlaveId == PRV_MASTER
						&& VochnInfo[i].VdecCap == (ExtraCap - tempVdecCap1)						
						&& VochnInfo[i].IsHaveVdec
						&& i != index && i != TmpIndex[0])
					{
						TmpIndex[1] = i;
						tempVdecCap2 = VochnInfo[i].VdecCap;
						TRACE(SCI_TRACE_NORMAL, MOD_PRV, "TWO: Find Master Chn: %d---Cap: %d!\n", VochnInfo[TmpIndex[1]].VoChn, VochnInfo[TmpIndex[1]].VdecCap);		
					}
				}
				if(TmpIndex[0] != -1 && TmpIndex[1] != -1)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ExtraCap == 3, Allready Find Two Master Chn: %d-%d!\n", VochnInfo[TmpIndex[0]].VoChn, VochnInfo[TmpIndex[1]].VoChn);		
					break;
				}
			}
		}
		//��Ƭû������Ϊ2��ͨ������Ҫ��3��ͨ���Ŵ�Ƭ
		else
		{		
			j = 0;
			for(i = 0; i < PRV_VO_CHN_NUM; i++)
			{
				//����������������Ϊ1��ͨ��
				if(VochnInfo[i].SlaveId == PRV_MASTER
					&& VochnInfo[i].VdecCap == 1
					&& i != index)
				{
					TmpIndex[j] = i;
					j++;
				}
				//�ҵ�3������Ϊ1��ͨ��
				if(TmpIndex[2] != -1)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ExtraCap == 3, Allready Find Three Master Chn: %d-%d-%d!\n", VochnInfo[TmpIndex[0]].VoChn, VochnInfo[TmpIndex[1]].VoChn, VochnInfo[TmpIndex[2]].VoChn);		
					break;
				}
			}
	
		}
	
	}
	//ExtraCap=4ʱ����Ҫ����Ƭ��1��ͨ���Ŵ�Ƭ������Ϊ4
	//����Ҫ����Ƭ��2��ͨ���Ŵ�Ƭ�����ܾ�Ϊ2
	//����Ҫ����Ƭ��3��ͨ���Ŵ�Ƭ������Ϊ1��1��2
	//����Ҫ����Ƭ��4��ͨ���Ŵ�Ƭ�����ܾ�Ϊ1
	else if(ExtraCap == 4)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ExtraCap == 4,Need Find one or Two or Three or Four Master Chn!\n");			
		//������Ƭ������Ϊ4��ͨ��
		for(i = 0; i < PRV_VO_CHN_NUM; i++)
		{
			if(VochnInfo[i].SlaveId == PRV_MASTER && VochnInfo[i].VdecCap == 4 && i != index)
			{	
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ExtraCap == 4, Allready Find One Master Chn: %d!\n", VochnInfo[i].VoChn);
				TmpIndex[0] = i;
				return;
			}	
		}
		int count =0;
		//������Ƭ������Ϊ2��ͨ��
		for(i = 0; i < PRV_VO_CHN_NUM; i++)
		{
			if(VochnInfo[i].SlaveId == PRV_MASTER && VochnInfo[i].VdecCap == 2 && i != index)
			{
				count++;
			}	
		}
		//û���ҵ�����Ϊ2��ͨ������Ҫ����4������Ϊ1��ͨ���Ŵ�Ƭ
		j = 0;
		if(count == 0)
		{
			for(i = 0; i < PRV_VO_CHN_NUM; i++)
			{
				//�����ĸ���������Ϊ1��ͨ��
				if(VochnInfo[i].SlaveId == PRV_MASTER
					&& VochnInfo[i].VdecCap == 1
					&& i != index)
				{
					TmpIndex[j] = i;
					j++;
				}
				//�ҵ�3������Ϊ1��ͨ��
				if(TmpIndex[3] != -1)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ExtraCap == 4, Allready Find Four Master Chn: %d-%d-%d-%d!\n",
						VochnInfo[TmpIndex[0]].VoChn, VochnInfo[TmpIndex[1]].VoChn, VochnInfo[TmpIndex[2]].VoChn, VochnInfo[TmpIndex[3]].VoChn);		

					break;
				}

			}
		}
		//�ҵ�1������Ϊ2��ͨ������Ҫ����3��ͨ���Ŵ�Ƭ
		else if(count == 1)
		{
			for(i = 0; i < PRV_VO_CHN_NUM; i++)
			{
				if(VochnInfo[i].SlaveId == PRV_MASTER && VochnInfo[i].VdecCap == 2 && i != index)
				{
					TmpIndex[j] = i;
					j++;
					break;
				}	
			}
			for(i = 0; i < PRV_VO_CHN_NUM; i++)
			{
				if(VochnInfo[i].SlaveId == PRV_MASTER && VochnInfo[i].VdecCap == 1 && i != index)
				{
					TmpIndex[j] = i;
					j++;
				}
				if(TmpIndex[2] != -1)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ExtraCap == 4, Allready Find Three Master Chn: %d-%d-%d!\n", VochnInfo[TmpIndex[0]].VoChn, VochnInfo[TmpIndex[1]].VoChn, VochnInfo[TmpIndex[2]].VoChn);		
					break;
				}
				
			}
		}
		//�ҵ�2����������Ϊ2��ͨ������Ҫ����2��ͨ���Ŵ�Ƭ
		else if(count >= 2)
		{
			for(i = 0; i < PRV_VO_CHN_NUM; i++)
			{
				if(VochnInfo[i].SlaveId == PRV_MASTER && VochnInfo[i].VdecCap == 2 && i != index)
				{
					TmpIndex[j] = i;
					j++;
				}
				if(TmpIndex[2] != -1)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ExtraCap == 4, Allready Find Two Master Chn: %d-%d-%d!\n", VochnInfo[TmpIndex[0]].VoChn, VochnInfo[TmpIndex[1]].VoChn);		
					break;
				}
				
			}
		}

	}
	else
	{
		for(i = 0; i < PRV_VO_CHN_NUM; i++)
		{
			if(VochnInfo[i].SlaveId == PRV_MASTER && i != index)
			{
				TmpIndex[j] = i;
				j++;
				if(j >= 8)
					break;
			}			
		}

	}
}

int PRV_FindMaster_Min(int ExtraCap, int index, int TmpIndex[])
{
	int i = 0, j = 0, b_sing = 1, b_most = 1;
	int SingChn = 0, SingCap = 0;
	int MostChn[PRV_VO_CHN_NUM] = {0}, MostCap = 0;

	
	for(i = 0; i < PRV_VO_CHN_NUM; i++)
	{
		if(VochnInfo[i].VdecCap > 0 && i != index)
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "VochnInfo[%d].SlaveId=%d, VdecCap=%d, ExtraCap=%d\n", i, VochnInfo[i].SlaveId, VochnInfo[i].VdecCap, ExtraCap);
		}
		
		if(b_sing && VochnInfo[i].SlaveId == PRV_MASTER && i != index)
		{
			if(VochnInfo[i].VdecCap >= ExtraCap)
			{
				b_sing = 0;
				SingChn = i;
				SingCap	= VochnInfo[i].VdecCap;
			}
		}
		else if(VochnInfo[i].SlaveId == PRV_MASTER && i != index)
		{
			if((VochnInfo[i].VdecCap >= ExtraCap) && (abs(SingCap - ExtraCap) > abs(VochnInfo[i].VdecCap - ExtraCap)))
			{
				SingChn = i;
				SingCap	= VochnInfo[i].VdecCap;
			}
		}

		if(b_most && VochnInfo[i].SlaveId == PRV_MASTER  && i != index)
		{
			if(VochnInfo[i].VdecCap <= ExtraCap)
			{
				b_most = 0;
				MostCap = VochnInfo[i].VdecCap;
				MostChn[i] = VochnInfo[i].VdecCap;
			}
		}
		else if(VochnInfo[i].SlaveId == PRV_MASTER && i != index)
		{
			if(MostCap > ExtraCap)
			{
				int temCap=0, needCap=0, nowcap=0, tempchn=0;
				nowcap = VochnInfo[i].VdecCap;
				needCap = MostCap - ExtraCap;
				for(j=0; j<i; j++)
				{
					if(MostChn[j] > 0 && MostChn[j] > nowcap && MostChn[j] <= (nowcap + needCap))
					{
						if(temCap == 0)
						{
							tempchn = j;
							temCap = MostChn[j];
						}
						else
						{
							if(temCap < MostChn[j])
							{
								tempchn = j;
								temCap = MostChn[j];
							}
						}
					}
				}
				if(temCap > 0)
				{
					MostCap -= (MostChn[tempchn] - nowcap);
					MostChn[tempchn] = 0;
					MostChn[i] = nowcap;
				}
			}
			else
			{
				MostCap += VochnInfo[i].VdecCap;
				MostChn[i] = VochnInfo[i].VdecCap;
			}
		}
		
	}

	if(SingCap > 0 || MostCap > 0)
	{
		if((SingCap > 0 && SingCap < MostCap) || (SingCap > 0 && MostCap <= 0))
		{
			if(SingCap + CurSlaveCap > TOTALCAPPCHIP || SingCap < ExtraCap)
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line:%d SingCap=%d, MostCap=%d, CurSlaveCap=%d, total=%d, SingCap is over total\n", __LINE__, SingCap, MostCap, CurSlaveCap, TOTALCAPPCHIP);
				return -1;
			}
			else
			{
				TmpIndex[0] = SingChn;
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "SingCap=%d, MostCap=%d, CurSlaveCap=%d, total=%d, SingChn=%d\n", SingCap, MostCap, CurSlaveCap, TOTALCAPPCHIP, SingChn);
			}
			
		}
		else if(MostCap > 0)
		{
			if(MostCap + CurSlaveCap > TOTALCAPPCHIP || MostCap < ExtraCap)
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line:%d SingCap=%d, MostCap=%d, CurSlaveCap=%d, total=%d, MostCap is over total\n", __LINE__, SingCap, MostCap, CurSlaveCap, TOTALCAPPCHIP);
				return -1;
			}
			else
			{
				i = 0, j = 0;
				for(i=0; i<PRV_VO_CHN_NUM; i++)
				{
					if(MostChn[i] > 0)
					{
						TRACE(SCI_TRACE_NORMAL, MOD_PRV, "MostChn[%d]=%d\n", i, MostChn[i]);
						TmpIndex[j] = i;
						j++;
					}
				}
			}
			
		}
		else
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "SingCap=%d, MostCap=%d, CurSlaveCap=%d, total=%d\n", SingCap, MostCap, CurSlaveCap, TOTALCAPPCHIP);
			return -1;
		}

		return OK;
	}

	return -1;
}


/********************************************************
������:PRV_FindMasterChnReChooseSlave
��     ��:����Ƭ������Ҫ�ķŴ�Ƭ�����ͨ��
��     ��:[in]ExtraCap:��Ƭ��Ҫ�ó�������
		[in]index:��ǰ�����ͨ���±�(�ֱ��ʱ���ͨ��)
		[in]TmpIndex:��Ҫѡ��ķŴ�Ƭ�����ͨ����
����ֵ:  

*********************************************************/

HI_VOID PRV_FindMasterChnReChooseSlave_EX(int ExtraCap, int index, int TmpIndex[])
{
	int i = 0, j = 0;

	if(ExtraCap == QCIF)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ExtraCap == 1,Need Find One Master Chn!\n");			
		//������Ƭ������Ϊ1��ͨ��
		for(i = 0; i < PRV_VO_CHN_NUM; i++)
		{
			if(VochnInfo[i].SlaveId == PRV_MASTER && VochnInfo[i].VdecCap == QCIF && i != index)
			{
				TmpIndex[0] = i;
				break;	
			}
		}
	}
	else if(ExtraCap == CIF)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ExtraCap == 1,Need Find One Master Chn!\n");			
		//������Ƭ������Ϊ1��ͨ��
		for(i = 0; i < PRV_VO_CHN_NUM; i++)
		{
			if(VochnInfo[i].SlaveId == PRV_MASTER && VochnInfo[i].VdecCap == CIF && i != index)
			{
				TmpIndex[0] = i;
				break;	
			}
		}
	}
	else if(ExtraCap == D1)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ExtraCap == 1,Need Find One Master Chn!\n");			
		//������Ƭ������Ϊ1��ͨ��
		for(i = 0; i < PRV_VO_CHN_NUM; i++)
		{
			if(VochnInfo[i].SlaveId == PRV_MASTER && VochnInfo[i].VdecCap == D1 && i != index)
			{
				TmpIndex[0] = i;
				break;	
			}
		}
	}
	//ExtraCap=2ʱ����Ҫ����Ƭ��1����2��ͨ���Ŵ�Ƭ����������Ϊ1����һ������Ϊ2
	else if(ExtraCap == HIGH_SEVEN)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ExtraCap == 2,Need Find Two or Three Master Chn!\n");			
		for(i = 0; i < PRV_VO_CHN_NUM; i++)
		{
			if(VochnInfo[i].SlaveId == PRV_MASTER && VochnInfo[i].VdecCap == HIGH_SEVEN && i != index)
			{
				TmpIndex[0] = i;
				return;
			}
		}
		//��Ƭû������Ϊ2��ͨ������Ҫ��2��ͨ���Ŵ�Ƭ
		if(i == PRV_VO_CHN_NUM)
		{
			j = 0;
			for(i = 0; i < PRV_VO_CHN_NUM; i++)
			{
				//���Ҷ�����������Ϊ1��ͨ��
				if(VochnInfo[i].SlaveId == PRV_MASTER
					&& VochnInfo[i].VdecCap == D1
					&& i != index)
				{
					TmpIndex[j] = i;
					j++;
				}
				//�ҵ�2������Ϊ1��ͨ��
				if(TmpIndex[1] != -1)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ExtraCap == 3, Allready Find Three Master Chn: %d-%d-%d!\n", VochnInfo[TmpIndex[0]].VoChn, VochnInfo[TmpIndex[1]].VoChn, VochnInfo[TmpIndex[2]].VoChn);		
					break;
				}
			}

		}

	}
	//ExtraCap=3ʱ����Ҫ����Ƭ��2��ͨ���Ŵ�Ƭ��һ������Ϊ1��һ������Ϊ2
	//����Ҫ����Ƭ��3��ͨ���Ŵ�Ƭ����3��ͨ�����ܾ�Ϊ1
	else if(ExtraCap == (MAX_3D1))
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ExtraCap == 3,Need Find Two or Three Master Chn!\n");			
		//������Ƭ������Ϊ2��ͨ��
		for(i = 0; i < PRV_VO_CHN_NUM; i++)
		{
			if(VochnInfo[i].SlaveId == PRV_MASTER && VochnInfo[i].VdecCap == HIGH_SEVEN && i != index)
				break;
	
		}
		//��Ƭ������Ϊ2��ͨ������ֻ��Ҫ��2��ͨ���Ŵ�Ƭ
		if(i != PRV_VO_CHN_NUM)
		{
			int tempVdecCap1 = 0, tempVdecCap2 = 0;
			for(i = 0; i < PRV_VO_CHN_NUM; i++)
			{
				//���ҵ�һ����������(1��2)��ͨ��
				if(TmpIndex[0] == -1)
				{
					if(VochnInfo[i].SlaveId == PRV_MASTER
						&& VochnInfo[i].VdecCap < ExtraCap
						&& VochnInfo[i].IsHaveVdec
						&& i != index)
					{
						TmpIndex[0] = i;
						tempVdecCap1 = VochnInfo[i].VdecCap;
						TRACE(SCI_TRACE_NORMAL, MOD_PRV, "One: Find Master Chn: %d---Cap: %d!\n", VochnInfo[TmpIndex[0]].VoChn, VochnInfo[TmpIndex[0]].VdecCap);		
					}
				}
				//���ҵڶ����������ܵ�ͨ��
				else
				{
					if(VochnInfo[i].SlaveId == PRV_MASTER
						&& VochnInfo[i].VdecCap == (ExtraCap - tempVdecCap1)						
						&& VochnInfo[i].IsHaveVdec
						&& i != index && i != TmpIndex[0])
					{
						TmpIndex[1] = i;
						tempVdecCap2 = VochnInfo[i].VdecCap;
						TRACE(SCI_TRACE_NORMAL, MOD_PRV, "TWO: Find Master Chn: %d---Cap: %d!\n", VochnInfo[TmpIndex[1]].VoChn, VochnInfo[TmpIndex[1]].VdecCap);		
					}
				}
				if(TmpIndex[0] != -1 && TmpIndex[1] != -1)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ExtraCap == 3, Allready Find Two Master Chn: %d-%d!\n", VochnInfo[TmpIndex[0]].VoChn, VochnInfo[TmpIndex[1]].VoChn);		
					break;
				}
			}
		}
		//��Ƭû������Ϊ2��ͨ������Ҫ��3��ͨ���Ŵ�Ƭ
		else
		{		
			j = 0;
			for(i = 0; i < PRV_VO_CHN_NUM; i++)
			{
				//����������������Ϊ1��ͨ��
				if(VochnInfo[i].SlaveId == PRV_MASTER
					&& VochnInfo[i].VdecCap == D1
					&& i != index)
				{
					TmpIndex[j] = i;
					j++;
				}
				//�ҵ�3������Ϊ1��ͨ��
				if(TmpIndex[2] != -1)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ExtraCap == 3, Allready Find Three Master Chn: %d-%d-%d!\n", VochnInfo[TmpIndex[0]].VoChn, VochnInfo[TmpIndex[1]].VoChn, VochnInfo[TmpIndex[2]].VoChn);		
					break;
				}
			}
	
		}
	
	}
	//ExtraCap=4ʱ����Ҫ����Ƭ��1��ͨ���Ŵ�Ƭ������Ϊ4
	//����Ҫ����Ƭ��2��ͨ���Ŵ�Ƭ�����ܾ�Ϊ2
	//����Ҫ����Ƭ��3��ͨ���Ŵ�Ƭ������Ϊ1��1��2
	//����Ҫ����Ƭ��4��ͨ���Ŵ�Ƭ�����ܾ�Ϊ1
	else if(ExtraCap == HIGH_TEN)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ExtraCap == 4,Need Find one or Two or Three or Four Master Chn!\n");			
		//������Ƭ������Ϊ4��ͨ��
		for(i = 0; i < PRV_VO_CHN_NUM; i++)
		{
			if(VochnInfo[i].SlaveId == PRV_MASTER && VochnInfo[i].VdecCap == HIGH_TEN && i != index)
			{	
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ExtraCap == 4, Allready Find One Master Chn: %d!\n", VochnInfo[i].VoChn);
				TmpIndex[0] = i;
				return;
			}	
		}
		int count =0;
		//������Ƭ������Ϊ2��ͨ��
		for(i = 0; i < PRV_VO_CHN_NUM; i++)
		{
			if(VochnInfo[i].SlaveId == PRV_MASTER && VochnInfo[i].VdecCap == HIGH_SEVEN && i != index)
			{
				count++;
			}	
		}
		//û���ҵ�����Ϊ2��ͨ������Ҫ����4������Ϊ1��ͨ���Ŵ�Ƭ
		j = 0;
		if(count == 0)
		{
			for(i = 0; i < PRV_VO_CHN_NUM; i++)
			{
				//�����ĸ���������Ϊ1��ͨ��
				if(VochnInfo[i].SlaveId == PRV_MASTER
					&& VochnInfo[i].VdecCap == D1
					&& i != index)
				{
					TmpIndex[j] = i;
					j++;
				}
				//�ҵ�3������Ϊ1��ͨ��
				if(TmpIndex[3] != -1)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ExtraCap == 4, Allready Find Four Master Chn: %d-%d-%d-%d!\n",
						VochnInfo[TmpIndex[0]].VoChn, VochnInfo[TmpIndex[1]].VoChn, VochnInfo[TmpIndex[2]].VoChn, VochnInfo[TmpIndex[3]].VoChn);		

					break;
				}

			}
		}
		//�ҵ�1������Ϊ2��ͨ������Ҫ����3��ͨ���Ŵ�Ƭ
		else if(count == 1)
		{
			for(i = 0; i < PRV_VO_CHN_NUM; i++)
			{
				if(VochnInfo[i].SlaveId == PRV_MASTER && VochnInfo[i].VdecCap == HIGH_SEVEN && i != index)
				{
					TmpIndex[j] = i;
					j++;
					break;
				}	
			}
			for(i = 0; i < PRV_VO_CHN_NUM; i++)
			{
				if(VochnInfo[i].SlaveId == PRV_MASTER && VochnInfo[i].VdecCap == D1 && i != index)
				{
					TmpIndex[j] = i;
					j++;
				}
				if(TmpIndex[2] != -1)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ExtraCap == 4, Allready Find Three Master Chn: %d-%d-%d!\n", VochnInfo[TmpIndex[0]].VoChn, VochnInfo[TmpIndex[1]].VoChn, VochnInfo[TmpIndex[2]].VoChn);		
					break;
				}
				
			}
		}
		//�ҵ�2����������Ϊ2��ͨ������Ҫ����2��ͨ���Ŵ�Ƭ
		else if(count >= 2)
		{
			for(i = 0; i < PRV_VO_CHN_NUM; i++)
			{
				if(VochnInfo[i].SlaveId == PRV_MASTER && VochnInfo[i].VdecCap == HIGH_SEVEN && i != index)
				{
					TmpIndex[j] = i;
					j++;
				}
				if(TmpIndex[2] != -1)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ExtraCap == 4, Allready Find Two Master Chn: %d-%d-%d!\n", VochnInfo[TmpIndex[0]].VoChn, VochnInfo[TmpIndex[1]].VoChn);		
					break;
				}
				
			}
		}

	}
	else
	{
		for(i = 0; i < PRV_VO_CHN_NUM; i++)
		{
			if(VochnInfo[i].SlaveId == PRV_MASTER && i != index)
			{
				TmpIndex[j] = i;
				j++;
				if(j >= 8)
					break;
			}			
		}

	}
}

/********************************************************
������:PRV_ReCreateVdecChn
��     ��:���´�������ͨ����
		��Ƭ����ͨ������������Ϣ�����ı�ʱ(�ֱ���)�����ô˽ӿ�
��     ��:[in]VdecChn  ָ������ͨ��
		    [in]EncType  �����Ľ�������������
		    [in]new_height  �ֱ��ʸ�(��)
		    [in]new_width   �ֱ��ʿ�(��)
		    [in]NewVdeCap  �ý�����ռ�õ�����(��)
����ֵ:  HI_SUCCESS�ɹ�
		    HI_FAILUREʧ��

*********************************************************/

HI_S32 PRV_ReCreateVdecChn(HI_S32 chn, HI_S32 EncType, HI_S32 new_height, HI_S32 new_width, HI_U32 u32RefFrameNum, HI_S32 NewVdeCap)
{
	int index = chn + LOCALVEDIONUM, PreChnIndex = 0;	
	PRV_MccCreateVdecReq SlaveCreateVdecReq;
	if(HI_SUCCESS == PRV_WaitDestroyVdecChn(chn))//�����ٽ���ͨ��
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "chn: %d, CurMasterCap: %d, OldVdeCap: %d, NewVdecCap: %d\n", chn, CurMasterCap, VochnInfo[index].VdecCap, NewVdeCap);	
		CurCap -= VochnInfo[index].VdecCap;
		CurMasterCap -= VochnInfo[index].VdecCap;			
		
		VochnInfo[index].IsHaveVdec = 0;	
		VochnInfo[index].VdecChn = NoConfig_VdecChn;
		//VochnInfo[index].VdecCap = 0;
		PreChnIndex = VochnInfo[index].CurChnIndex;
		//VochnInfo[index].CurChnIndex = -1;
		
	}
	else
	{
		//VochnInfo[index].bIsWaitIFrame = 1;
		VochnInfo[index].bIsWaitGetIFrame = 1;
		
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_WaitDestroyVdecChn fail---chn: %d\n", chn);	
		return HI_FAILURE;
	}
	//�����ܹ�����Ƭ���ܲ������鿴��Ƭ�Ƿ��㹻
	if((CurMasterCap + NewVdeCap) > TOTALCAPPCHIP)
	{
	
#if defined(Hi3531)||defined(Hi3535)//3531��������������Ƭ
		CurSlaveCap = TOTALCAPPCHIP;
#endif
		//CurIPCCount--;
		//�����Ƭ�����㹻���ܣ�����ѡ���Ƭ����
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "CurSlaveCap---%d, NewVdeCap: %d\n", CurSlaveCap, NewVdeCap);
		if(!VochnInfo[index].bIsPBStat && (CurSlaveCap + NewVdeCap) <= TOTALCAPPCHIP)
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "============ReChoose Slave, Vdec---%d\n", chn);
			VochnInfo[chn].SlaveId = PRV_SLAVE_1;
			VochnInfo[chn].CurChnIndex = PreChnIndex;
			VochnInfo[chn].VoChn = chn;
			VochnInfo[chn].VdecChn = VochnInfo[chn].VoChn;
			VochnInfo[chn].IsConnect = 1;
			VochnInfo[chn].VdecCap = NewVdeCap;
			CurSlaveCap += NewVdeCap;
			CurSlaveChnCount++;
			//��Ƭ��������ͨ����Ϣ
			//SlaveCreateVdec->SlaveId = VochnInfo[chn].SlaveId;	
			SlaveCreateVdecReq.s32StreamChnIDs = MasterToSlaveChnId;
			SlaveCreateVdecReq.EncType = VochnInfo[chn].VideoInfo.vdoType;
			SlaveCreateVdecReq.chn = VochnInfo[chn].CurChnIndex;
			SlaveCreateVdecReq.VoChn = VochnInfo[chn].VoChn;
			SlaveCreateVdecReq.VdecChn = VochnInfo[chn].VoChn;
			SlaveCreateVdecReq.VdecCap = VochnInfo[chn].VdecCap;	
			SlaveCreateVdecReq.height = VochnInfo[chn].VideoInfo.height;
			SlaveCreateVdecReq.width = VochnInfo[chn].VideoInfo.width;		
			SlaveCreateVdecReq.SlaveId = PRV_SLAVE_1;
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "============Send: MSG_ID_PRV_MCC_CREATEVDEC_REQ---%d\n", SlaveCreateVdecReq.VdecChn);
			VochnInfo[index].MccCreateingVdec = 1;
#if defined(Hi3531)||defined(Hi3535)			
			MccCreateingVdecCount++;
#endif
			SN_SendMccMessageEx(SlaveCreateVdecReq.SlaveId, SUPER_USER_ID, MOD_PRV, MOD_PRV, 0, 0, MSG_ID_PRV_MCC_CREATEVDEC_REQ, &SlaveCreateVdecReq, sizeof(PRV_MccCreateVdecReq));					
			return 1;

		}
		//��Ƭ����Ƭ���ܾ��������鿴�ܿ��������Ƿ��㹻��
		//����㹻�����·�������Ƭ
		else if((CurMasterCap + CurSlaveCap + NewVdeCap) <= DEV_CHIP_NUM * TOTALCAPPCHIP)
		{
			int tempVdecCap = 0, i = 0, ret = 0;
			deluser_used DelUserReq;
			DelUserReq.channel = chn;
			int TmpIndex[8] = {-1, -1, -1, -1, -1, -1, -1, -1,};
			SCM_Link_Rsp Rsp;
			SN_MEMSET(&Rsp, 0, sizeof(Rsp));
			
			Rsp.channel = VochnInfo[index].VoChn;
			Rsp.endtype = LINK_OVERFLOW;
			//��ͨ������Ƭ��Ҫ���������
			tempVdecCap = NewVdeCap - (TOTALCAPPCHIP - CurMasterCap);

			//tempVdecCap = PRV_ComPare(tempVdecCap);
			
			//����Ƭ���ҵ�����������ܵ�ͨ��(1����2����3����4�������)�ķŴ�Ƭ
			if(tempVdecCap >= TOTALCAPPCHIP)
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "chn: %d, No Vaild Cap: %d\n", chn, VochnInfo[chn].VdecCap);
				SN_SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_NTRANS, 0, 0, MSG_ID_NTRANS_DELUSER_REQ, &DelUserReq, sizeof(deluser_used));
				goto DiscardChn;
			}
			
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "%s line:%d CurMasterCap=%d, CurSlaveCap=%d, tempVdecCap=%d, total=%d, Need Find Master Chn!\n", __FUNCTION__, __LINE__, CurMasterCap, CurSlaveCap, tempVdecCap, TOTALCAPPCHIP);			

			ret = PRV_FindMaster_Min(tempVdecCap, chn, TmpIndex);
			if(ret < 0)
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Master: =======tempVdecCap: %d, No Find Master Chn ReChoose Slave---\n", tempVdecCap);
				SN_SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_NTRANS, 0, 0, MSG_ID_NTRANS_DELUSER_REQ, &DelUserReq, sizeof(deluser_used));
				goto DiscardChn;
			}

			for(i = 0; i < 8; i++)
			{
				//������Ƭ�ҵ���ͨ���ķŴ�Ƭ
				if(TmpIndex[i] > -1)
					PRV_MasterChnReChooseSlave(TmpIndex[i]);
				else
					break;
					
			}
			
#if 0			
			PRV_FindMasterChnReChooseSlave(tempVdecCap, chn, TmpIndex);
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Master: tempVdecCap: %d <= 4,Need Find Master Chn!\n", tempVdecCap);			
			for(i = 0; i < 8; i++)
			{
				//������Ƭ�ҵ���ͨ���ķŴ�Ƭ
				if(TmpIndex[i] > -1)
					PRV_MasterChnReChooseSlave(TmpIndex[i]);
				else
					break;
					
			}
			if(i == 0)
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Master: =======tempVdecCap: %d, No Find Master Chn ReChoose Slave---\n", tempVdecCap);
				SN_SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_NTRANS, 0, 0, MSG_ID_NTRANS_DELUSER_REQ, &DelUserReq, sizeof(deluser_used));
				goto DiscardChn;
			}					
#endif			
		}
		//�����ܲ������ݲ����ͨ�����ݣ����յ���ͨ�����ݺ󣬶�������"��������Ƶ"
		else
		{
DiscardChn:
			if(VochnInfo[index].bIsPBStat)
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_ReCreateVdecChn fail----chn: %d, CurMasterCap: %d, NewVdeCap: %d\n", chn, CurMasterCap, NewVdeCap);	
				Prv_Chn_ChnPBOverFlow_Ind ChnPbOver;
				SN_MEMSET(&ChnPbOver, 0, sizeof(Prv_Chn_ChnPBOverFlow_Ind));
				ChnPbOver.Chn = chn;
				ChnPbOver.NewWidth = new_width;
				ChnPbOver.NewHeight = new_height;							
				SN_SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_FWK, 0, 0, MSG_ID_PRV_CHNPBOVERFLOW_IND, &ChnPbOver, sizeof(Prv_Chn_ChnPBOverFlow_Ind));					
				PRV_VoChnStateInit(PreChnIndex);
				
				PRM_ID_TIME tmpQueryStartTime = PtsInfo[chn].QueryStartTime; 
				PRM_ID_TIME tmpQueryFinalTime = PtsInfo[chn].QueryFinalTime;
				PRV_PtsInfoInit(PreChnIndex);		
				PRV_InitVochnInfo(chn);
				VochnInfo[index].bIsWaitGetIFrame = 1;
				VochnInfo[index].bIsPBStat = 1;
				PtsInfo[chn].QueryStartTime = tmpQueryStartTime;
				PtsInfo[chn].QueryFinalTime = tmpQueryFinalTime;
				return HI_FAILURE;
			}	
			PRV_VoChnStateInit(PreChnIndex);			
			PRV_PtsInfoInit(PreChnIndex);		
			PRV_InitVochnInfo(chn);
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_ReCreateVdecChn++++++++++++++Discard---chn: %d, CurMasterCap: %d, NewVdeCap: %d\n", chn, CurMasterCap, NewVdeCap);	
			/*
			SCM_Link_Rsp rsp;
			SN_MEMSET(&rsp, 0, sizeof(rsp));
			rsp.channel = chn;
			rsp.endtype = LINK_OVERFLOW;
			SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_FWK, 0, 0, MSG_ID_NTRANS_ONCEOVER_IND, &rsp, sizeof(rsp));
			*/
			if(HI_SUCCESS == PRV_VoChnIsInCurLayOut(s_VoDevCtrlDflt, VochnInfo[chn].VoChn))
			{
#if defined(SN9234H1)
				CHECK(HI_MPI_VO_DisableChn(HD, VochnInfo[chn].VoChn));			
			//	CHECK(HI_MPI_VO_DisableChn(s_VoSecondDev, VochnInfo[chn].VoChn));
				PRV_VLossVdecBindVoChn(HD, VochnInfo[index].VoChn,	s_astVoDevStatDflt[HD].s32PreviewIndex, s_astVoDevStatDflt[HD].enPreviewMode);
			//	PRV_VLossVdecBindVoChn(s_VoSecondDev, VochnInfo[chn].VoChn, s_astVoDevStatDflt[s_VoSecondDev].s32PreviewIndex, s_astVoDevStatDflt[s_VoSecondDev].enPreviewMode);
				CHECK(HI_MPI_VO_EnableChn(HD, VochnInfo[chn].VoChn));			
			//	CHECK(HI_MPI_VO_EnableChn(s_VoSecondDev, VochnInfo[chn].VoChn));
#else
				CHECK(HI_MPI_VO_DisableChn(DHD0, VochnInfo[chn].VoChn));			
				//CHECK(HI_MPI_VO_DisableChn(DSD0, VochnInfo[chn].VoChn));
				PRV_VLossVdecBindVoChn(DHD0, VochnInfo[index].VoChn,	s_astVoDevStatDflt[DHD0].s32PreviewIndex, s_astVoDevStatDflt[DHD0].enPreviewMode);
				//PRV_VLossVdecBindVoChn(DSD0, VochnInfo[chn].VoChn, s_astVoDevStatDflt[s_VoSecondDev].s32PreviewIndex, s_astVoDevStatDflt[s_VoSecondDev].enPreviewMode);
				CHECK(HI_MPI_VO_EnableChn(DHD0, VochnInfo[chn].VoChn));			
				//CHECK(HI_MPI_VO_EnableChn(DSD0, VochnInfo[chn].VoChn));
#endif		
			//	sem_post(&sem_SendNoVideoPic);
				
			//	if(!IsChoosePlayAudio)
					PRV_PlayAudio(s_VoDevCtrlDflt);				
			}
			else
			{
#if defined(SN9234H1)
				(HI_MPI_VDEC_BindOutput(NoConfig_VdecChn, HD, VochnInfo[index].VoChn));	
				VochnInfo[index].IsBindVdec[HD] = 0;				
		//		CHECK(HI_MPI_VDEC_BindOutput(DetVLoss_VdecChn, s_VoSecondDev, VochnInfo[chn].VoChn));	
		//		VochnInfo[chn].IsBindVdec[s_VoSecondDev] = 0;				
#else				
				PRV_VPSS_ResetWH(VochnInfo[index].VoChn,NoConfig_VdecChn,704,576);
				PRV_VDEC_BindVpss(NoConfig_VdecChn, VochnInfo[index].VoChn);
				VochnInfo[index].IsBindVdec[DHD0] = 0;	
				//VochnInfo[index].IsBindVdec[DSD0] = 0;	
#endif				
			}
			//VochnInfo[index].IsDiscard = 1;
			//VochnInfo[index].bIsWaitIFrame = 1;
			VochnInfo[index].bIsWaitGetIFrame = 1;
			return HI_FAILURE;
		}
	}
	if(HI_SUCCESS == PRV_CreateVdecChn(EncType, new_height, new_width, u32RefFrameNum, chn))
	{
		VochnInfo[index].VideoInfo.height = new_height;
		VochnInfo[index].VideoInfo.width = new_width;
		VochnInfo[index].VdecCap = NewVdeCap;			
		VochnInfo[index].CurChnIndex = PreChnIndex;
		VochnInfo[index].VdecChn = chn;
		VochnInfo[index].IsHaveVdec = 1;	
		
		CurCap += VochnInfo[index].VdecCap;
		CurMasterCap += VochnInfo[index].VdecCap;
#if defined(SN9234H1)
		PRV_BindVoChnInMaster(HD, VochnInfo[index].VoChn, s_astVoDevStatDflt[HD].s32PreviewIndex, s_astVoDevStatDflt[HD].enPreviewMode);			
#else
		PRV_BindVoChnInMaster(DHD0, VochnInfo[index].VoChn, s_astVoDevStatDflt[DHD0].s32PreviewIndex, s_astVoDevStatDflt[DHD0].enPreviewMode);	
#endif
		PRV_PlayAudio(s_VoDevCtrlDflt);
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Recreate Vdec: %d Ok---CurMasterCap: %d, NewVdecCap: %d!\n", chn, CurMasterCap, NewVdeCap);		
	}
	else
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ReCreate Vdec: fail!\n", chn);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}


/********************************************************
������:PRV_MCC_RecreateVdecRsp
��     ��:�����Ƭ���´�������ͨ��������Ϣ
��     ��:[in]msg_rsp  ��Ƭ���´�������ͨ�����ص���Ϣ
����ֵ:  HI_SUCCESS�ɹ�
		    HI_FAILUREʧ��

*********************************************************/
HI_S32 PRV_MCC_RecreateVdecRsp(const SN_MSG * msg_rsp)
{	
	PRV_MccReCreateVdecRsp *SlaveReCreateRsp = (PRV_MccReCreateVdecRsp *)(msg_rsp->para);
	HI_S32 index = -1, chn = -1;
	index = PRV_GetVoChnIndex(SlaveReCreateRsp->VoChn);
	if(index < 0)
		RET_FAILURE("Invalid Channel");
	chn = VochnInfo[index].CurChnIndex;
	TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_MCC_RecreateVdecRsp, Vdec: %d, SlaveReCreateRsp->VdecCap: %d\n", SlaveReCreateRsp->VdecChn, SlaveReCreateRsp->VdecCap);
	
	VochnInfo[index].MccReCreateingVdec = 0;
#if defined(Hi3531)||defined(Hi3535)	
	MccReCreateingVdecCount--;
#endif
	if(0 != SlaveReCreateRsp->Result)
	{
		if (-1 == SlaveReCreateRsp->Result)	//��Ƭ û�н������´�������
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Slave Not ReCreate\n");
			//VochnInfo[index].bIsWaitIFrame = 1;
			VochnInfo[chn].bIsWaitGetIFrame = 1;
			CurCap = CurCap + VochnInfo[index].VdecCap - SlaveReCreateRsp->VdecCap;	
			CurSlaveCap = CurSlaveCap + VochnInfo[index].VdecCap - SlaveReCreateRsp->VdecCap;;
			return HI_FAILURE;
		}
		else if(-2 == SlaveReCreateRsp->Result)//��Ƭ���ٽ���ͨ��ʧ��
		{	
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Slave Destroy vdec fail\n");
			//VochnInfo[index].bIsWaitIFrame = 1;
			VochnInfo[index].bIsWaitGetIFrame = 1;
			CurCap = CurCap + VochnInfo[index].VdecCap - SlaveReCreateRsp->VdecCap;	
			CurSlaveCap = CurSlaveCap + VochnInfo[index].VdecCap - SlaveReCreateRsp->VdecCap;;
			return HI_FAILURE;
		}
		else if(-3 == SlaveReCreateRsp->Result)//��Ƭ���ٽ���ͨ���ɹ������´�������ͨ��ʧ��(��Ƭ���ܲ���)
		{
#if defined(SN9234H1)
			(HI_MPI_VI_UnBindOutput(PRV_HD_DEV, 0, HD, VochnInfo[index].VoChn));					
			CHECK(HI_MPI_VO_ClearChnBuffer(HD, VochnInfo[index].VoChn, 1));
			VochnInfo[index].IsBindVdec[HD] = -1;
#else
			//(HI_MPI_VI_UnBindOutput(PRV_HD_DEV, 0, DHD0, VochnInfo[index].VoChn));					
			CHECK(HI_MPI_VO_ClearChnBuffer(DHD0, VochnInfo[index].VoChn, 1));
			VochnInfo[index].IsBindVdec[DHD0] = -1;
#endif			
			//BufferSet(chn + PRV_VIDEOBUFFER, MAX_ARRAY_NODE);			
			//BufferSet(chn + PRV_AUDIOBUFFER, MAX_ARRAY_NODE);			
			//BufferClear(chn + PRV_VIDEOBUFFER);
			//BufferClear(chn + PRV_AUDIOBUFFER);
			CurCap -= SlaveReCreateRsp->VdecCap;	
			CurSlaveCap -= SlaveReCreateRsp->VdecCap;
			CurSlaveChnCount--;
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Slave Destroy vdec OK, Create Vdec: %d fail!,CurMasterCap: %d, CurSlaveCap: %d, SlaveReCreateRsp->VdecCap: %d\n", SlaveReCreateRsp->VdecChn, CurMasterCap, CurSlaveCap, SlaveReCreateRsp->VdecCap);
			//�鿴��Ƭ�����Ƿ��㹻������㹻������Ƭ����
			if((CurMasterCap + SlaveReCreateRsp->VdecCap) <= TOTALCAPPCHIP)
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "=============ReChoose master chip!\n");			
				CurCap = CurCap + SlaveReCreateRsp->VdecCap; 
				VochnInfo[index].CurChnIndex = chn;
				VochnInfo[index].SlaveId = PRV_MASTER;
				VochnInfo[index].VoChn = SlaveReCreateRsp->VoChn; 
				VochnInfo[index].VdecCap = SlaveReCreateRsp->VdecCap;
				VochnInfo[index].IsConnect = 1;
				VochnInfo[index].VdecChn = SlaveReCreateRsp->VdecChn;	
				VochnInfo[index].IsChooseSlaveId = 1;
				PRV_CreateVdecChn_EX(index);
				//CurIPCCount++;
			}
			//�鿴�������Ƿ��㹻
			//����������㹻��������Ƭ��ѡ����ʵ�ͨ�����ķŴ�Ƭ���룻
			//Ȼ�󽫵�ǰͨ���ķ���Ƭ����
			else if((CurMasterCap + CurSlaveCap + SlaveReCreateRsp->VdecCap) <= 2 * TOTALCAPPCHIP)
			{
				int tempVdecCap, i = 0, ret = 0;
				int TmpIndex[8] = {-1};
				SCM_Link_Rsp Rsp;
				SN_MEMSET(&Rsp, 0, sizeof(Rsp));
				
				Rsp.channel = VochnInfo[index].VoChn;
				Rsp.endtype = LINK_OVERFLOW;
				//����ͨ������Ƭ��Ҫ���������
				tempVdecCap = SlaveReCreateRsp->VdecCap - (TOTALCAPPCHIP - CurMasterCap);

				//tempVdecCap = PRV_ComPare(tempVdecCap);

				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "%s line:%d CurMasterCap=%d, CurSlaveCap=%d, tempVdecCap=%d, total=%d, Need Find Master Chn!\n", __FUNCTION__, __LINE__, CurMasterCap, CurSlaveCap, tempVdecCap, TOTALCAPPCHIP);			

				ret = PRV_FindMaster_Min(tempVdecCap, index, TmpIndex);
				if(ret < 0)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "%s, line:%d tempVdecCap=%d, No Find Master Chn ReChoose Slave---\n", __FUNCTION__, __LINE__, tempVdecCap);
					goto DiscardChn;
				}

				for(i = 0; i < 8; i++)
				{
					//������Ƭ�ҵ���ͨ���ķŴ�Ƭ
					if(TmpIndex[i] >= 0)
						PRV_MasterChnReChooseSlave(TmpIndex[i]);
					else
						break;
						
				}

				CurCap += SlaveReCreateRsp->VdecCap; 
#if 0				
				//����Ƭ���ҵ�����������ܵ�ͨ��(1����2��)�ķŴ�Ƭ
				if(tempVdecCap == MAX_3D1 || tempVdecCap == HIGH_TEN)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Slave: tempVdecCap: %d >= 3,Need Find Two Master Chn!\n", tempVdecCap);			
					int TmpIndex[4] = {-1, -1, -1, -1};
					PRV_FindMasterChnReChooseSlave(tempVdecCap, index, TmpIndex);
					for(i = 0; i < 4; i++)
					{
						//������Ƭ�ҵ���ͨ���ķŴ�Ƭ
						if(TmpIndex[i] > -1)
							PRV_MasterChnReChooseSlave(TmpIndex[i]);
						else
							break;
							
					}
					if(i == 0)
					{
						TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Slave: =======tempVdecCap: %d >= 3, No Find Master Chn ReChoose Slave---\n", tempVdecCap);
						goto DiscardChn;
					}
					else
					{
						CurCap += SlaveReCreateRsp->VdecCap; 
					}
				}
				//tempVdecCap=1��2ʱ��ֻ��Ҫ��һ��ͨ��
				else if (tempVdecCap <= D1 || tempVdecCap == HIGH_SEVEN)
				{
					for(i = 0; i < PRV_VO_CHN_NUM; i++)
					{
						if(VochnInfo[i].SlaveId == PRV_MASTER && VochnInfo[i].VdecCap == tempVdecCap && i != index)
						{
							break;
						}
					}
					if(i != PRV_VO_CHN_NUM)
					{
						CurCap += SlaveReCreateRsp->VdecCap; 
						PRV_MasterChnReChooseSlave(i);
					}
					else
					{
						TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Slave: =======tempVdecCap < 3, No Find Master Chn ReChoose Slave---\n");
						goto DiscardChn;
					}
					
				}
				else
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Slave: =======tempVdecCap > 4, No Find Master Chn ReChoose Slave---\n");
					goto DiscardChn;

				}
#endif				
				//����ǰͨ������Ƭ
				if(HI_SUCCESS == PRV_CreateVdecChn(VochnInfo[index].VideoInfo.vdoType, SlaveReCreateRsp->height, SlaveReCreateRsp->width, VochnInfo[index].u32RefFrameNum, SlaveReCreateRsp->VdecChn))
				{
					VochnInfo[index].VideoInfo.height = SlaveReCreateRsp->height;
					VochnInfo[index].VideoInfo.width = SlaveReCreateRsp->width;
					VochnInfo[index].VdecCap = SlaveReCreateRsp->VdecCap;			
					VochnInfo[index].CurChnIndex = chn;
					VochnInfo[index].VdecChn = SlaveReCreateRsp->VdecChn;
					VochnInfo[index].IsHaveVdec = 1;	
					VochnInfo[index].SlaveId = PRV_MASTER;
					
					CurMasterCap += VochnInfo[index].VdecCap;
					if(HI_SUCCESS == PRV_VoChnIsInCurLayOut(s_VoDevCtrlDflt, VochnInfo[index].VoChn))
					{
#if defined(SN9234H1)
						if(s_astVoDevStatDflt[HD].enPreviewStat == PRV_STAT_CTRL)
						{
							if(VochnInfo[index].VoChn == s_astVoDevStatDflt[HD].s32CtrlChn)
							{
								CHECK(HI_MPI_VI_UnBindOutput(PRV_HD_DEV, 0, HD, PRV_CTRL_VOCHN));					
								CHECK(HI_MPI_VO_ClearChnBuffer(HD, PRV_CTRL_VOCHN, 1));
							}
						}
						//����disable Vo���������"������"ͼƬ��ʾ�쳣���������ͨ���´�����Ƭ��ʾ��Ҳ���쳣
						CHECK(HI_MPI_VO_DisableChn(HD, VochnInfo[chn].VoChn));			
						PRV_BindVoChnInMaster(HD, VochnInfo[index].VoChn, s_astVoDevStatDflt[HD].s32PreviewIndex, s_astVoDevStatDflt[HD].enPreviewMode);			
						CHECK(HI_MPI_VO_EnableChn(HD, VochnInfo[chn].VoChn));			
#else
						if(s_astVoDevStatDflt[DHD0].enPreviewStat == PRV_STAT_CTRL)
						{
							if(VochnInfo[index].VoChn == s_astVoDevStatDflt[DHD0].s32CtrlChn)
							{
								//CHECK(HI_MPI_VI_UnBindOutput(PRV_HD_DEV, 0, DHD0, PRV_CTRL_VOCHN));					
								CHECK(HI_MPI_VO_ClearChnBuffer(DHD0, PRV_CTRL_VOCHN, 1));
							}
						}
						//����disable Vo���������"������"ͼƬ��ʾ�쳣���������ͨ���´�����Ƭ��ʾ��Ҳ���쳣
						CHECK(HI_MPI_VO_DisableChn(DHD0, VochnInfo[chn].VoChn));			
						PRV_BindVoChnInMaster(DHD0, VochnInfo[index].VoChn, s_astVoDevStatDflt[DHD0].s32PreviewIndex, s_astVoDevStatDflt[DHD0].enPreviewMode);			
						CHECK(HI_MPI_VO_EnableChn(DHD0, VochnInfo[chn].VoChn));	
#endif						
						PRV_PlayAudio(s_VoDevCtrlDflt);
						TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ReChoose Master---Vdec: %d Ok---CurMasterCap: %d, NewVdecCap: %d!\n", chn, CurMasterCap, VochnInfo[index].VdecCap);
					}			
					else
					{
#if defined(SN9234H1)
						(HI_MPI_VDEC_BindOutput(VochnInfo[index].VdecChn, HD, VochnInfo[index].VoChn)); 	
						VochnInfo[index].IsBindVdec[HD] = 0;				
#else
						PRV_VPSS_ResetWH(VochnInfo[index].VoChn,VochnInfo[index].VdecChn,VochnInfo[index].VideoInfo.width,VochnInfo[index].VideoInfo.height);
						PRV_VDEC_BindVpss(VochnInfo[index].VdecChn, VochnInfo[index].VoChn);
						//(HI_MPI_VDEC_BindOutput(VochnInfo[index].VdecChn, DHD0, VochnInfo[index].VoChn)); 	
						VochnInfo[index].IsBindVdec[DHD0] = 0;	
#endif
			//			CHECK(HI_MPI_VDEC_BindOutput(DetVLoss_VdecChn, s_VoSecondDev, VochnInfo[index].VoChn)); 	
			//			VochnInfo[index].IsBindVdec[s_VoSecondDev] = 0;				
					}
					
				}
				else
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line: %d---ReCreate Vdec: fail!\n", __LINE__, chn);
					return HI_FAILURE;				
				}
			}
			else
			{
DiscardChn:
				PRV_InitVochnInfo(VochnInfo[index].VoChn);
				PRV_VoChnStateInit(chn);			
				PRV_PtsInfoInit(chn);		
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_MCC_RecreateVdecRsp++++++++++++++Discard---chn: %d, CurMasterCap: %d, NewVdeCap: %d\n", chn, CurMasterCap, SlaveReCreateRsp->VdecCap);	
				/*
				SCM_Link_Rsp rsp;
				SN_MEMSET(&rsp, 0, sizeof(rsp));
				rsp.channel = chn;
				rsp.endtype = LINK_OVERFLOW;
				SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_FWK, 0, 0, MSG_ID_NTRANS_ONCEOVER_IND, &rsp, sizeof(rsp));
				*/
				//VochnInfo[index].IsDiscard = 1;
				
				//VochnInfo[index].bIsWaitIFrame = 1;
				VochnInfo[index].bIsWaitGetIFrame = 1;
				if(HI_SUCCESS == PRV_VoChnIsInCurLayOut(s_VoDevCtrlDflt, VochnInfo[index].VoChn))
				{
#if defined(SN9234H1)
					if(s_astVoDevStatDflt[HD].enPreviewStat == PRV_STAT_CTRL)
					{
						if(VochnInfo[index].VoChn == s_astVoDevStatDflt[HD].s32CtrlChn)
						{
							CHECK(HI_MPI_VI_UnBindOutput(PRV_HD_DEV, 0, HD, PRV_CTRL_VOCHN));					
							CHECK(HI_MPI_VO_ClearChnBuffer(HD, PRV_CTRL_VOCHN, 1));
						}
					}
					//����disable Vo���������"������"ͼƬ��ʾ�쳣���������ͨ���´�����Ƭ��ʾ��Ҳ���쳣
					CHECK(HI_MPI_VO_DisableChn(HD, VochnInfo[index].VoChn));			
				//	CHECK(HI_MPI_VO_DisableChn(s_VoSecondDev, VochnInfo[index].VoChn));
					PRV_VLossVdecBindVoChn(HD, VochnInfo[index].VoChn, s_astVoDevStatDflt[HD].s32PreviewIndex, s_astVoDevStatDflt[HD].enPreviewMode);
				//	PRV_VLossVdecBindVoChn(s_VoSecondDev, VochnInfo[index].VoChn, s_astVoDevStatDflt[s_VoSecondDev].s32PreviewIndex, s_astVoDevStatDflt[s_VoSecondDev].enPreviewMode);
					CHECK(HI_MPI_VO_EnableChn(HD, VochnInfo[index].VoChn));
				//	CHECK(HI_MPI_VO_EnableChn(s_VoSecondDev, VochnInfo[index].VoChn));

					//sem_post(&sem_SendNoVideoPic);
					
#else
					if(s_astVoDevStatDflt[DHD0].enPreviewStat == PRV_STAT_CTRL)
					{
						if(VochnInfo[index].VoChn == s_astVoDevStatDflt[DHD0].s32CtrlChn)
						{
							//CHECK(HI_MPI_VI_UnBindOutput(PRV_HD_DEV, 0, DHD0, PRV_CTRL_VOCHN));					
							CHECK(HI_MPI_VO_ClearChnBuffer(DHD0, PRV_CTRL_VOCHN, 1));
						}
					}
					//����disable Vo���������"������"ͼƬ��ʾ�쳣���������ͨ���´�����Ƭ��ʾ��Ҳ���쳣
					CHECK(HI_MPI_VO_DisableChn(DHD0, VochnInfo[index].VoChn));			
				//	CHECK(HI_MPI_VO_DisableChn(s_VoSecondDev, VochnInfo[index].VoChn));
					PRV_VLossVdecBindVoChn(DHD0, VochnInfo[index].VoChn, s_astVoDevStatDflt[DHD0].s32PreviewIndex, s_astVoDevStatDflt[DHD0].enPreviewMode);
				//	PRV_VLossVdecBindVoChn(s_VoSecondDev, VochnInfo[index].VoChn, s_astVoDevStatDflt[s_VoSecondDev].s32PreviewIndex, s_astVoDevStatDflt[s_VoSecondDev].enPreviewMode);
					CHECK(HI_MPI_VO_EnableChn(DHD0, VochnInfo[index].VoChn));
				//	CHECK(HI_MPI_VO_EnableChn(s_VoSecondDev, VochnInfo[index].VoChn));

					//sem_post(&sem_SendNoVideoPic);
#endif					
					//if(!IsChoosePlayAudio)
						PRV_PlayAudio(s_VoDevCtrlDflt);
				}			
				else
				{
#if defined(SN9234H1)
					(HI_MPI_VDEC_BindOutput(DetVLoss_VdecChn, HD, VochnInfo[index].VoChn)); 	
					VochnInfo[index].IsBindVdec[HD] = 0;				
#else
					
					PRV_VPSS_ResetWH(VochnInfo[index].VoChn,NoConfig_VdecChn,704,576);
					PRV_VDEC_BindVpss(NoConfig_VdecChn, VochnInfo[index].VoChn);
					//(HI_MPI_VDEC_BindOutput(DetVLoss_VdecChn, DHD0, VochnInfo[index].VoChn)); 	
					VochnInfo[index].IsBindVdec[DHD0] = 0;
#endif					
		//			CHECK(HI_MPI_VDEC_BindOutput(DetVLoss_VdecChn, s_VoSecondDev, VochnInfo[index].VoChn)); 	
		//			VochnInfo[index].IsBindVdec[s_VoSecondDev] = 0;				
				}
			}
		}
		
	}
	else//���´����ɹ�
	{		
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "=============Slave ReCreate VdecChn: %d OK, CurSlaveCap: %d, OldVdecCap: %d, NewVdecCap: %d, SlaveId: %d!\n", VochnInfo[index].VdecChn, CurSlaveCap, VochnInfo[index].VdecCap, SlaveReCreateRsp->VdecCap, VochnInfo[index].SlaveId);			
		VochnInfo[index].VdecCap = SlaveReCreateRsp->VdecCap;
		VochnInfo[index].VideoInfo.height = SlaveReCreateRsp->height;
		VochnInfo[index].VideoInfo.width = SlaveReCreateRsp->width;
		VochnInfo[index].VdecChn = SlaveReCreateRsp->VdecChn;
		VochnInfo[index].IsHaveVdec = 1;
		if(HI_SUCCESS == PRV_VoChnIsInCurLayOut(s_VoDevCtrlDflt, VochnInfo[index].VoChn))
		{
#if defined(SN9234H1)
			if(s_astVoDevStatDflt[HD].enPreviewStat == PRV_STAT_CTRL)
			{
				if(VochnInfo[index].VoChn == s_astVoDevStatDflt[HD].s32CtrlChn)
				{
					CHECK(HI_MPI_VI_UnBindOutput(PRV_HD_DEV, 0, HD, PRV_CTRL_VOCHN));					
					CHECK(HI_MPI_VO_ClearChnBuffer(HD, PRV_CTRL_VOCHN, 1));
				}
			}
			HI_MPI_VI_UnBindOutput(PRV_HD_DEV, 0, s_VoDevCtrlDflt, VochnInfo[index].VoChn);					
			CHECK(HI_MPI_VO_ClearChnBuffer(s_VoDevCtrlDflt, VochnInfo[index].VoChn, 1));
			
			CHECK(HI_MPI_VO_DisableChn(s_VoDevCtrlDflt, VochnInfo[index].VoChn));			
			PRV_BindVoChnInSlave(s_VoDevCtrlDflt, VochnInfo[index].VoChn, s_astVoDevStatDflt[HD].s32PreviewIndex, s_astVoDevStatDflt[HD].enPreviewMode);				
#else
			if(s_astVoDevStatDflt[DHD0].enPreviewStat == PRV_STAT_CTRL)
			{
				if(VochnInfo[index].VoChn == s_astVoDevStatDflt[DHD0].s32CtrlChn)
				{
					//CHECK(HI_MPI_VI_UnBindOutput(PRV_HD_DEV, 0, DHD0, PRV_CTRL_VOCHN));					
					CHECK(HI_MPI_VO_ClearChnBuffer(DHD0, PRV_CTRL_VOCHN, 1));
				}
			}
			//HI_MPI_VI_UnBindOutput(PRV_HD_DEV, 0, s_VoDevCtrlDflt, VochnInfo[index].VoChn);					
			CHECK(HI_MPI_VO_ClearChnBuffer(s_VoDevCtrlDflt, VochnInfo[index].VoChn, 1));
			
			CHECK(HI_MPI_VO_DisableChn(s_VoDevCtrlDflt, VochnInfo[index].VoChn));			
			PRV_BindVoChnInSlave(s_VoDevCtrlDflt, VochnInfo[index].VoChn, s_astVoDevStatDflt[DHD0].s32PreviewIndex, s_astVoDevStatDflt[DHD0].enPreviewMode);	
#endif			
			CHECK(HI_MPI_VO_EnableChn(s_VoDevCtrlDflt, VochnInfo[index].VoChn));
			PRV_PlayAudio(s_VoDevCtrlDflt);
		}			

	}
	RET_SUCCESS("");
}

/********************************************************
������:PRV_MSG_DestroyAllVdec
��     ��:�������н���ͨ����Ԥ��ģʽ�л�ʱ����
��     ��:flag:---0:�������н���ͨ���󣬲���Ҫ��"��������Ƶ"
				1:�������н���ͨ������Ҫ��"��������Ƶ"
����ֵ:  ��

*********************************************************/

HI_VOID PRV_MSG_DestroyAllVdec(int flag)
{
	int i = 0;
	//��������Ƭ�����н���ͨ������֪ͨ��Ƭ�������н���ͨ��
	for(i = LOCALVEDIONUM; i < CHANNEL_NUM; i++)
	{
		if(VochnInfo[i].SlaveId == PRV_MASTER)
		{
			if(VochnInfo[i].IsHaveVdec)
			{
				PRV_WaitDestroyVdecChn(VochnInfo[i].VdecChn);
				BufferSet(VochnInfo[i].VoChn + PRV_VIDEOBUFFER, MAX_ARRAY_NODE);						
				BufferSet(VochnInfo[i].VoChn + PRV_AUDIOBUFFER, MAX_ARRAY_NODE);
				CurCap -= VochnInfo[i].VdecCap;
				CurMasterCap -= VochnInfo[i].VdecCap;			
				PRV_VoChnStateInit(VochnInfo[i].CurChnIndex);
				//CurIPCCount--;			
				PRV_PtsInfoInit(VochnInfo[i].CurChnIndex);		
				PRV_InitVochnInfo(i);	
			}
			else
			{
#if defined(SN9234H1)
				HI_MPI_VDEC_UnbindOutputChn(VochnInfo[i].VdecChn, HD, VochnInfo[i].VoChn);/* ���ͨ�� */
				//CHECK(HI_MPI_VO_ClearChnBuffer(HD, VochnInfo[i].VoChn, 1)); /* ���VO���� */
#else			
				PRV_VDEC_UnBindVpss(NoConfig_VdecChn, VochnInfo[i].VoChn);
				//CHECK(HI_MPI_VO_ClearChnBuffer(DHD0, VochnInfo[i].VoChn, 1)); /* ���VO���� */
#endif				
				VochnInfo[i].IsBindVdec[DHD0] = -1;
			}
			if(flag == 0||flag == LayOut_KeyBoard||flag == ParamUpdate_Switch)//�����л�
				VochnInfo[i].VdecChn = NoConfig_VdecChn;	
			else if(flag == 1)//��������������ʧ��
				VochnInfo[i].VdecChn = DetVLoss_VdecChn;	
			else//����Ͽ�
			{
				if(SCM_ChnConfigState(VochnInfo[i].VoChn) == 0)
					VochnInfo[i].VdecChn = NoConfig_VdecChn;	
				else
					VochnInfo[i].VdecChn = DetVLoss_VdecChn;

			}
			VochnInfo[i].VdecChn = NoConfig_VdecChn;
#if defined(SN9234H1)
			CHECK(HI_MPI_VO_DisableChn(HD, VochnInfo[i].VoChn));			
			if(HI_SUCCESS == PRV_VoChnIsInCurLayOut(s_VoDevCtrlDflt, VochnInfo[i].VoChn))
			{
			//	CHECK(HI_MPI_VO_DisableChn(s_VoSecondDev, VochnInfo[chn].VoChn));
				PRV_VLossVdecBindVoChn(HD, VochnInfo[i].VoChn,  s_astVoDevStatDflt[HD].s32PreviewIndex, s_astVoDevStatDflt[HD].enPreviewMode);
			//	PRV_VLossVdecBindVoChn(s_VoSecondDev, VochnInfo[chn].VoChn, s_astVoDevStatDflt[s_VoSecondDev].s32PreviewIndex, s_astVoDevStatDflt[s_VoSecondDev].enPreviewMode);
			//	CHECK(HI_MPI_VO_EnableChn(s_VoSecondDev, VochnInfo[chn].VoChn));

			//	sem_post(&sem_SendNoVideoPic);
				
			//	if(!IsChoosePlayAudio)
			//		PRV_PlayAudio(s_VoDevCtrlDflt);
				CHECK(HI_MPI_VO_EnableChn(HD, VochnInfo[i].VoChn)); 		
					
			}
			else
			{	
				(HI_MPI_VDEC_BindOutput(VochnInfo[i].VdecChn, HD, VochnInfo[i].VoChn)); 	
				VochnInfo[i].IsBindVdec[HD] = 0;				
		//		CHECK(HI_MPI_VDEC_BindOutput(DetVLoss_VdecChn, s_VoSecondDev, VochnInfo[chn].VoChn)); 	
		//		VochnInfo[chn].IsBindVdec[s_VoSecondDev] = 0;				
			}		

#else			
			
			CHECK(HI_MPI_VO_DisableChn(DHD0, VochnInfo[i].VoChn));			
			//CHECK(HI_MPI_VO_DisableChn(DSD0, VochnInfo[i].VoChn));			
			if(HI_SUCCESS == PRV_VoChnIsInCurLayOut(s_VoDevCtrlDflt, VochnInfo[i].VoChn))
			{
				PRV_VLossVdecBindVoChn(DHD0, VochnInfo[i].VoChn,  s_astVoDevStatDflt[DHD0].s32PreviewIndex, s_astVoDevStatDflt[DHD0].enPreviewMode);
			//	PRV_VLossVdecBindVoChn(DSD0, VochnInfo[i].VoChn, s_astVoDevStatDflt[DSD0].s32PreviewIndex, s_astVoDevStatDflt[DSD0].enPreviewMode);
				CHECK(HI_MPI_VO_EnableChn(DHD0, VochnInfo[i].VoChn));
			//	CHECK(HI_MPI_VO_EnableChn(DSD0, VochnInfo[i].VoChn)); 		

			//	sem_post(&sem_SendNoVideoPic);
				
			//	if(!IsChoosePlayAudio)
			//		PRV_PlayAudio(s_VoDevCtrlDflt);
					
			}
			else
			{	
				(PRV_VDEC_BindVpss(VochnInfo[i].VdecChn, VochnInfo[i].VoChn)); 	
				VochnInfo[i].IsBindVdec[DHD0] = 0;				
			//	VochnInfo[i].IsBindVdec[DSD0] = 0;				
			}
#endif					
		}
	}
#if defined(SN_SLAVE_ON)	
	HI_S32 s32Ret = 0;
	BufferSet(0, MAX_ARRAY_NODE);						
	
	s32Ret = HostCleanAllHostToSlaveStream(MasterToSlaveChnId, 0, 1);
	if (s32Ret != HI_SUCCESS)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "slave %d HostCleanAllHostToSlaveStream error");
	}	
	SN_SendMccMessageEx(PRV_SLAVE_1, SUPER_USER_ID, MOD_PRV, MOD_PRV, 0, 0, MSG_ID_PRV_MCC_DESALLVDEC_REQ, &flag, sizeof(int));
#endif	
}


/********************************************************
������:PRV_MSG_MCC_DesAllVdecRsp
��     ��:��Ƭ�������н���ͨ��������Ϣ����
��     ��:��
����ֵ:  ��

*********************************************************/
HI_VOID PRV_MSG_MCC_DesAllVdecRsp(int flag)
{
	int i = 0;
	for(i = LOCALVEDIONUM; i < CHANNEL_NUM; i++)
	{
		if(VochnInfo[i].IsHaveVdec && VochnInfo[i].SlaveId > PRV_MASTER)
		{
#if defined(SN9234H1)
			(HI_MPI_VI_UnBindOutput(PRV_HD_DEV, 0, HD, VochnInfo[i].VoChn));					
			CHECK(HI_MPI_VO_ClearChnBuffer(HD, VochnInfo[i].VoChn, 1));
			VochnInfo[i].IsBindVdec[HD] = -1;

#else		
			//(HI_MPI_VI_UnBindOutput(PRV_HD_DEV, 0, DHD0, VochnInfo[i].VoChn));					
			CHECK(HI_MPI_VO_ClearChnBuffer(DHD0, VochnInfo[i].VoChn, 1));
			VochnInfo[i].IsBindVdec[DHD0] = -1;
#endif			
			BufferSet(VochnInfo[i].VoChn + PRV_VIDEOBUFFER, MAX_ARRAY_NODE);						
			BufferSet(VochnInfo[i].VoChn + PRV_AUDIOBUFFER, MAX_ARRAY_NODE);
			CurCap -= VochnInfo[i].VdecCap;
			CurSlaveCap -= VochnInfo[i].VdecCap;			
			CurSlaveChnCount--;
			PRV_VoChnStateInit(VochnInfo[i].CurChnIndex);
			//CurIPCCount--;			
			PRV_PtsInfoInit(VochnInfo[i].CurChnIndex);		
			PRV_InitVochnInfo(i);
			if(flag == 0)//�����л�
				VochnInfo[i].VdecChn = NoConfig_VdecChn;	
			else if(flag == 1)//��������������ʧ��
				VochnInfo[i].VdecChn = DetVLoss_VdecChn;	
			else//����Ͽ�
			{
				if(SCM_ChnConfigState(VochnInfo[i].VoChn) == 0)
					VochnInfo[i].VdecChn = NoConfig_VdecChn;	
				else
					VochnInfo[i].VdecChn = DetVLoss_VdecChn;

			}
			VochnInfo[i].VdecChn = NoConfig_VdecChn;
			if(HI_SUCCESS == PRV_VoChnIsInCurLayOut(s_VoDevCtrlDflt, VochnInfo[i].VoChn))
			{
#if defined(SN9234H1)
				if(s_astVoDevStatDflt[HD].enPreviewStat == PRV_STAT_CTRL)
				{
					if(VochnInfo[i].VoChn == s_astVoDevStatDflt[HD].s32CtrlChn)
					{
						(HI_MPI_VI_UnBindOutput(PRV_HD_DEV, 0, HD, PRV_CTRL_VOCHN));					
						(HI_MPI_VO_ClearChnBuffer(HD, PRV_CTRL_VOCHN, 1));
					}
				}
				//����disable Vo���������"������"ͼƬ��ʾ�쳣���������ͨ���´�����Ƭ��ʾ��Ҳ���쳣
				CHECK(HI_MPI_VO_DisableChn(HD, VochnInfo[i].VoChn));			
			//	CHECK(HI_MPI_VO_DisableChn(s_VoSecondDev, VochnInfo[index].VoChn));
				PRV_VLossVdecBindVoChn(HD, VochnInfo[i].VoChn, s_astVoDevStatDflt[HD].s32PreviewIndex, s_astVoDevStatDflt[HD].enPreviewMode);
			//	PRV_VLossVdecBindVoChn(s_VoSecondDev, VochnInfo[index].VoChn, s_astVoDevStatDflt[s_VoSecondDev].s32PreviewIndex, s_astVoDevStatDflt[s_VoSecondDev].enPreviewMode);
				CHECK(HI_MPI_VO_EnableChn(HD, VochnInfo[i].VoChn));
			//	CHECK(HI_MPI_VO_EnableChn(s_VoSecondDev, VochnInfo[index].VoChn));

#else			
				if(s_astVoDevStatDflt[DHD0].enPreviewStat == PRV_STAT_CTRL)
				{
					if(VochnInfo[i].VoChn == s_astVoDevStatDflt[DHD0].s32CtrlChn)
					{
						//(HI_MPI_VI_UnBindOutput(PRV_HD_DEV, 0, DHD0, PRV_CTRL_VOCHN));					
						(HI_MPI_VO_ClearChnBuffer(DHD0, PRV_CTRL_VOCHN, 1));
					}
				}
				//����disable Vo���������"������"ͼƬ��ʾ�쳣���������ͨ���´�����Ƭ��ʾ��Ҳ���쳣
				CHECK(HI_MPI_VO_DisableChn(DHD0, VochnInfo[i].VoChn));			
			//	CHECK(HI_MPI_VO_DisableChn(s_VoSecondDev, VochnInfo[index].VoChn));
				PRV_VLossVdecBindVoChn(DHD0, VochnInfo[i].VoChn, s_astVoDevStatDflt[DHD0].s32PreviewIndex, s_astVoDevStatDflt[DHD0].enPreviewMode);
			//	PRV_VLossVdecBindVoChn(s_VoSecondDev, VochnInfo[index].VoChn, s_astVoDevStatDflt[s_VoSecondDev].s32PreviewIndex, s_astVoDevStatDflt[s_VoSecondDev].enPreviewMode);
				CHECK(HI_MPI_VO_EnableChn(DHD0, VochnInfo[i].VoChn));
			//	CHECK(HI_MPI_VO_EnableChn(s_VoSecondDev, VochnInfo[index].VoChn));
#endif
				//sem_post(&sem_SendNoVideoPic);
				
				//if(!IsChoosePlayAudio)
				//	PRV_PlayAudio(s_VoDevCtrlDflt);
			}			
			else
			{
#if defined(SN9234H1)
				(HI_MPI_VDEC_BindOutput(VochnInfo[i].VdecChn, HD, VochnInfo[i].VoChn)); 	
				VochnInfo[i].IsBindVdec[HD] = 0;				
#else			
				PRV_VDEC_BindVpss(VochnInfo[i].VdecChn, VochnInfo[i].VoChn);
				VochnInfo[i].IsBindVdec[DHD0] = 0;	
#endif							
	//			CHECK(HI_MPI_VDEC_BindOutput(DetVLoss_VdecChn, s_VoSecondDev, VochnInfo[index].VoChn)); 	
	//			VochnInfo[index].IsBindVdec[s_VoSecondDev] = 0;				
			}			
		}
	}
#if defined(SN_SLAVE_ON)	

	HI_S32 s32Ret = 0;
	BufferSet(0, MAX_ARRAY_NODE);						
	
	s32Ret = HostCleanAllHostToSlaveStream(MasterToSlaveChnId, 0, 1);
	if (s32Ret != HI_SUCCESS)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "slave %d HostCleanAllHostToSlaveStream error");
	}	
#endif	
	if(PRV_CurIndex > 0)
	{
		for(i = 0; i < PRV_CurIndex; i++)
		{
			NTRANS_FreeMediaData(PRV_OldVideoData[i]);
			PRV_OldVideoData[i] = NULL;
		}
		PRV_CurIndex = 0;
		PRV_SendDataLen = 0;
	}
	
}

/********************************************************
������:PRV_MSG_CtrlVdec
��     ��:����ָ��ͨ��������
��     ��:[in]msg_req  ��Ϣ
����ֵ:  ��

*********************************************************/

HI_VOID	PRV_MSG_CtrlVdec(const SN_MSG *msg_req)
{
	int chn = 0, flag = -1, index = -1, OldVdecChn = -1;
	SCM_CtrVdecInd	*CtrlVdec = (SCM_CtrVdecInd *)msg_req->para;
	chn = CtrlVdec->chn;
	flag = CtrlVdec->flag;
	index = chn + LOCALVEDIONUM;
	TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Receive message: MSG_ID_PRV_CTRVDEC_IND---chn: %d, flag=%d\n", chn, flag);				

	if(chn < 0 || chn >= DEV_CHANNEL_NUM)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "%s,line: %d---------invalid channel: %d!!\n", __FUNCTION__, __LINE__, chn);
		if(PRV_GetDoubleIndex())//˫��״̬���뵥����
		{
			if(chn == DEV_CHANNEL_NUM)
			{
				chn = PRV_GetDoubleToSingleIndex();
				if(VochnInfo[chn].bIsDouble)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "%s===chn: %d stop===\n", __FUNCTION__, chn);
					VochnInfo[chn].bIsDouble = 0;
				}
			}
		}

		return;
	}

	if(s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat != PRV_STAT_NORM
		|| VochnInfo[index].bIsPBStat)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "In PB or PIC, Don't PRV_MSG_CtrlVdec"); 
		return;
	}
	
	if(PRV_CurDecodeMode == SwitchDecode || ScmGetListCtlState() == 1)//���������£���Ҫ���ٽ���ͨ��
	{
		//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "flag: %d, IsHaveVdec: %d, SlaveId: %d\n", flag, VochnInfo[index].IsHaveVdec, VochnInfo[index].SlaveId);				
		if(VochnInfo[index].SlaveId == PRV_MASTER)
		{
			//if(/*VochnInfo[chn + LOCALVEDIONUM].IsHaveVdec || */VochnInfo[chn + LOCALVEDIONUM].VdecChn != DetVLoss_VdecChn)
			if(VochnInfo[index].bIsPBStat != 1)
			{
				OldVdecChn = VochnInfo[index].VdecChn;
				if(VochnInfo[chn + LOCALVEDIONUM].IsHaveVdec)
				{
					PRV_WaitDestroyVdecChn(VochnInfo[index].VdecChn);
					BufferSet(VochnInfo[index].VoChn + PRV_VIDEOBUFFER, MAX_ARRAY_NODE);						
					BufferSet(VochnInfo[index].VoChn + PRV_AUDIOBUFFER, MAX_ARRAY_NODE);
					CurCap -= VochnInfo[index].VdecCap;
					CurMasterCap -= VochnInfo[index].VdecCap;			
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line:%d chn: %d, CurMasterCap: %d, VochnInfo[chn + LOCALVEDIONUM].VdecCap: %d\n", __LINE__, chn, CurMasterCap, VochnInfo[index].VdecCap); 
					PRV_VoChnStateInit(VochnInfo[index].CurChnIndex);
					PRV_PtsInfoInit(VochnInfo[index].CurChnIndex);
					PRV_InitVochnInfo(index);	
				}
				//CurIPCCount--;	
				PRV_InitVochnInfo(index);				
			//	CHECK(HI_MPI_VO_ClearChnBuffer(DSD0, VochnInfo[index].VoChn, 1)); 
				//if(ScmGetListCtlState() == 1)//��λ����������ʧ��ʱ����"��������Ƶ"
				//	VochnInfo[index].VdecChn = DetVLoss_VdecChn;
				//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "VochnInfo[index].VdecChn: %d\n", VochnInfo[index].VdecChn);				
#if defined(SN9234H1)
				VochnInfo[index].IsBindVdec[HD] = -1;
#else			
				VochnInfo[index].IsBindVdec[DHD0] = -1;	
#endif		
				//����ͨ����Ȼ��"��������Ƶ"ʱ����ˢ��ͨ������		
				if(OldVdecChn == DetVLoss_VdecChn && VochnInfo[index].VdecChn == DetVLoss_VdecChn)
					return;
				if(OldVdecChn == DetVLoss_VdecChn || OldVdecChn == NoConfig_VdecChn)
#if defined(SN9234H1)
				{
					CHECK(HI_MPI_VDEC_UnbindOutputChn(OldVdecChn, HD, VochnInfo[index].VoChn));
				}
				//CHECK(HI_MPI_VO_ClearChnBuffer(HD, VochnInfo[index].VoChn, 1)); 
#else				
				{				
					PRV_VDEC_UnBindVpss(OldVdecChn,VochnInfo[index].VoChn);	
				}			
				//CHECK(HI_MPI_VO_ClearChnBuffer(DHD0, VochnInfo[index].VoChn, 1)); // ���VO���� 
#endif				
				VochnInfo[index].IsBindVdec[DHD0] = -1;
				if(HI_SUCCESS == PRV_VoChnIsInCurLayOut(s_VoDevCtrlDflt, VochnInfo[index].VoChn))
				{
#if defined(SN9234H1)
					CHECK(HI_MPI_VO_DisableChn(HD, VochnInfo[index].VoChn));			
				//	CHECK(HI_MPI_VO_DisableChn(s_VoSecondDev, VochnInfo[chn].VoChn));
					PRV_VLossVdecBindVoChn(HD, VochnInfo[index].VoChn, s_astVoDevStatDflt[HD].s32PreviewIndex, s_astVoDevStatDflt[HD].enPreviewMode);
				//	PRV_VLossVdecBindVoChn(s_VoSecondDev, VochnInfo[chn].VoChn, s_astVoDevStatDflt[s_VoSecondDev].s32PreviewIndex, s_astVoDevStatDflt[s_VoSecondDev].enPreviewMode);
					CHECK(HI_MPI_VO_EnableChn(HD, VochnInfo[index].VoChn));			

#else				
					PRV_InitVochnInfo(index);	
					//if(ScmGetListCtlState() == 1)//��λ����������ʧ��ʱ����"��������Ƶ"
					//	VochnInfo[index].VdecChn = DetVLoss_VdecChn;
					CHECK(HI_MPI_VO_DisableChn(DHD0, VochnInfo[index].VoChn));			
					//CHECK(HI_MPI_VO_DisableChn(DSD0, VochnInfo[chn].VoChn));
					PRV_VLossVdecBindVoChn(DHD0, VochnInfo[index].VoChn, s_astVoDevStatDflt[DHD0].s32PreviewIndex, s_astVoDevStatDflt[DHD0].enPreviewMode);
					//PRV_VLossVdecBindVoChn(DSD0, VochnInfo[chn].VoChn, s_astVoDevStatDflt[DSD0].s32PreviewIndex, s_astVoDevStatDflt[DSD0].enPreviewMode);
					CHECK(HI_MPI_VO_EnableChn(DHD0, VochnInfo[index].VoChn));			
					//CHECK(HI_MPI_VO_EnableChn(DSD0, VochnInfo[index].VoChn));	
#endif							
				}
				else// if(VochnInfo[index].IsBindVdec[DHD0] < 0)
				{
#if defined(SN9234H1)
					(HI_MPI_VDEC_BindOutput(VochnInfo[index].VdecChn, HD, VochnInfo[index].VoChn));	
					VochnInfo[index].IsBindVdec[HD] = 0;				
		//			CHECK(HI_MPI_VDEC_BindOutput(DetVLoss_VdecChn, s_VoSecondDev, VochnInfo[index].VoChn)); 	
		//			VochnInfo[index].IsBindVdec[s_VoSecondDev] = 0; 			
#else				
					PRV_VPSS_ResetWH(VochnInfo[index].VoChn,VochnInfo[index].VdecChn,VochnInfo[index].VideoInfo.width,VochnInfo[index].VideoInfo.height);
					PRV_VDEC_BindVpss(VochnInfo[index].VdecChn,VochnInfo[index].VoChn);
					VochnInfo[index].IsBindVdec[DHD0] = 0;				
					//VochnInfo[index].IsBindVdec[DSD0] = 0;	
#endif								
				}			
			}
			
		}
		else if(VochnInfo[index].SlaveId > PRV_MASTER && VochnInfo[chn + LOCALVEDIONUM].IsHaveVdec)
		{
			PRV_MccDestroyVdecReq DestroyVdecReq;
			DestroyVdecReq.VdecChn = VochnInfo[index].VdecChn;
			
			SN_SendMccMessageEx(PRV_SLAVE_1, SUPER_USER_ID, MOD_PRV, MOD_PRV, 0, 0, MSG_ID_PRV_MCC_DESVDEC_REQ, &DestroyVdecReq, sizeof(PRV_MccDestroyVdecReq));

		}
	}
	else//����������ʱ�������ٽ���ͨ������Ԥ����ͨ��������
	{
		
	}	
	
}


/********************************************************
������:PRV_CheckIsCreateVdec
��     ��:�ж��Ƿ��Ѿ�����ָ��ͨ���Ľ�������
��     ��:[in]index  ��Ƶ���ͨ����ͨ����Ϣ�±�
����ֵ:  HI_SUCCESS�ɹ�
		    HI_FAILUREʧ��

*********************************************************/

HI_S32 PRV_CheckIsCreateVdec(int index)
{
	if(index < 0)
		return HI_FAILURE;

	if(VochnInfo[index].VdecChn >= 0
		&& VochnInfo[index].VdecChn != DetVLoss_VdecChn && VochnInfo[index].VdecChn != NoConfig_VdecChn
		&& VochnInfo[index].SlaveId == PRV_MASTER)//�Ѵ�������ͨ��
	{
		CHECK_RET(PRV_WaitDestroyVdecChn(VochnInfo[index].VdecChn));				
	}
	
	return HI_SUCCESS;

}

/********************************************************
������:PRV_ReCreateAllVdec
��     ��:���´�����Ƭ���н�������
�����㹻����Ϊ�ڴ治��(�����ڴ���Ƭ)���´���������ʧ��ʱ����
��     ��:��
����ֵ:  ��

*********************************************************/

HI_VOID PRV_ReCreateAllVdec()
{
	HI_S32 i = 0;
	//���������н�����
	for(i = LOCALVEDIONUM; i < DEV_CHANNEL_NUM; i++)
	{
		if(VochnInfo[i].IsHaveVdec
			&& VochnInfo[i].SlaveId == PRV_MASTER
			&& VochnInfo[i].VdecChn >= 0
			&& VochnInfo[i].VdecChn != DetVLoss_VdecChn && VochnInfo[i].VdecChn != NoConfig_VdecChn)
		{
			CHECK(HI_MPI_VDEC_StopRecvStream(VochnInfo[i].VdecChn));//����ͨ��ǰ��ֹͣ��������
			CHECK(HI_MPI_VDEC_ResetChn(VochnInfo[i].VdecChn)); //��������λ 	
#if defined(SN9234H1)
			CHECK(HI_MPI_VDEC_UnbindOutput(VochnInfo[i].VdecChn));// ���ͨ�� 
#else					
			CHECK(PRV_VDEC_UnBindVpss(VochnInfo[i].VdecChn,VochnInfo[i].VdecChn));// ���ͨ�� 
#endif		
			VochnInfo[i].IsBindVdec[DHD0] = -1;
			HI_MPI_VO_ClearChnBuffer(i, VochnInfo[i].VoChn, 1); // ���VO���� 
			HI_MPI_VDEC_DestroyChn(VochnInfo[i].VdecChn); // ������Ƶͨ�� 
			
			BufferSet(VochnInfo[i].VoChn + PRV_VIDEOBUFFER, MAX_ARRAY_NODE);						
			BufferSet(VochnInfo[i].VoChn + PRV_AUDIOBUFFER, MAX_ARRAY_NODE);
		}
	}
	//�ؽ��Ѿ����ڵ����н�����	
	for(i = LOCALVEDIONUM; i < DEV_CHANNEL_NUM; i++)
	{
		if(VochnInfo[i].IsHaveVdec
			&& VochnInfo[i].SlaveId == PRV_MASTER
			&& VochnInfo[i].VdecChn >= 0
			&& VochnInfo[i].VdecChn != DetVLoss_VdecChn && VochnInfo[i].VdecChn != NoConfig_VdecChn)
		{	
			CHECK(PRV_CreateVdecChn(VochnInfo[i].VideoInfo.vdoType, VochnInfo[i].VideoInfo.height, VochnInfo[i].VideoInfo.width, VochnInfo[i].u32RefFrameNum, VochnInfo[i].VoChn));
#if defined(SN9234H1)
			PRV_BindVoChnInMaster(HD, VochnInfo[i].VoChn, s_astVoDevStatDflt[HD].s32PreviewIndex, s_astVoDevStatDflt[HD].enPreviewMode);			
#else			
			PRV_BindVoChnInMaster(DHD0, VochnInfo[i].VoChn, s_astVoDevStatDflt[DHD0].s32PreviewIndex, s_astVoDevStatDflt[DHD0].enPreviewMode);			
			//PRV_BindVoChnInMaster(DSD0, VochnInfo[i].VoChn, s_astVoDevStatDflt[DSD0].s32PreviewIndex, s_astVoDevStatDflt[DSD0].enPreviewMode);
#endif						
			//VochnInfo[i].bIsWaitIFrame = 1;
			VochnInfo[i].bIsWaitGetIFrame = 1;
		}
	}
}

/*************************************************
Function: PRV_MSG_NTRANSLinkReq
Description: ��������ͨ������VO������������ն�Ӧͨ�����ݱ�ʶ��ʵ��Ԥ��
Calls: 
Called By: 
Input:  msg_req��������ṹ�壬�������ӵ�ͨ����
Output: ����ϵͳ����Ĵ�����
Return: ��ϸ�ο��ĵ��Ĵ�����
Others: 
***********************************************************************/
STATIC HI_S32 PRV_MSG_NTRANSLinkReq(const SN_MSG *msg_req)
{
	HI_S32 chn = 0, s32Ret = 0;
	if(msg_req->msgId == MSG_ID_NTRANS_ADDUSER_RSP)
	{					
		NTRANS_ENDOFCOM_RSP *AddUser_Rsp = (NTRANS_ENDOFCOM_RSP*)msg_req->para;
		if(AddUser_Rsp->endtype != LINK_ALL_SUCCESS)
		{
			//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ADDUSER_RSP fail!!!!chn: %d, endtype: %d\n", AddUser_Rsp->channel, AddUser_Rsp->endtype);
			return HI_FAILURE;			
		}
		chn = AddUser_Rsp->channel;
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "-------------Receive MSG: MSG_ID_NTRANS_ADDUSER_RSP---%d\n", chn);
	}
	else if(msg_req->msgId == MSG_ID_NTRANS_BEGINLINK_IND)
	{				
		beginlink_rsp *BeginLinkReq = (beginlink_rsp *)msg_req->para;
		chn = BeginLinkReq->channel;
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "-------------Receive MSG: MSG_ID_NTRANS_BEGINLINK_IND---%d\n", chn);
	}
	
	if(chn < 0 || chn >= MAX_IPC_CHNNUM)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Invalid Channel: %d, line: %d\n", chn, __LINE__);
		return HI_FAILURE;		
	}
	
	deluser_used DelUserReq;
	DelUserReq.channel = chn;
	//��ǰ��Ƭ���ڴ�����ͨ��
	if(SlaveState.SlaveCreatingVdec[chn + LOCALVEDIONUM])
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Slave Is Creating Vdec: %d\n", chn);
		//SN_SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_NTRANS, 0, 0, MSG_ID_NTRANS_DELUSER_REQ, &DelUserReq, sizeof(deluser_used));			
		return HI_FAILURE;
	}

	//��ǰͨ���ڴ�Ƭ�����ٽ���ͨ��������
	if(SlaveState.SlaveIsDesingVdec[chn + LOCALVEDIONUM])
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV,"Slave Is Destroying vdec: %d\n", chn);
		//����Ͽ�����
		SN_SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_NTRANS, 0, 0, MSG_ID_NTRANS_DELUSER_REQ, &DelUserReq, sizeof(deluser_used));			
		return HI_FAILURE;
	}
	
	
	PRV_MccCreateVdecReq SlaveCreateVdec;		
	RTSP_C_SDPInfo RTSP_SDP;				
	SN_MEMSET(&SlaveCreateVdec, 0, sizeof(SlaveCreateVdec));
	SN_MEMSET(&RTSP_SDP, 0, sizeof(RTSP_C_SDPInfo));
	
	if(RTSP_C_getParam(chn, &RTSP_SDP) != 0)
	{		
		RET_FAILURE("RTSP_C_getParam!!!");
	}
	
	if(RTSP_SDP.vdoType == SN_UNKOWN_VDOTYPE )
	{			
		RET_FAILURE("Video Type!!!");
	}
	
	TRACE(SCI_TRACE_NORMAL, MOD_PRV,"---------chn: %d, RTSP_SDP.high:%d, RTSP_SDP.width:%d, RTSP_SDP.vdoType: %d\n", chn, RTSP_SDP.high, RTSP_SDP.width, RTSP_SDP.vdoType);

	TRACE(SCI_TRACE_NORMAL, MOD_PRV,"-----------adoType: %d, samrate: %d, soundchannel: %d\n", RTSP_SDP.adoType, RTSP_SDP.samrate, RTSP_SDP.soundchannel);
	if(RTSP_SDP.high > 4096 || RTSP_SDP.width > 4096
		|| RTSP_SDP.high <= 0 || RTSP_SDP.width <= 0)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV,"Not Support Res---height: %d, width: %d\n", RTSP_SDP.high, RTSP_SDP.width);
		//����Ͽ�����
		SN_SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_NTRANS, 0, 0, MSG_ID_NTRANS_DELUSER_REQ, &DelUserReq, sizeof(deluser_used));			
		return SN_SLAVE_MSG;
	}
#if 0
	//�����ͺ��£�֧�ֵ����ֱ���
#if defined(SN8604D) || defined(SN8608D) ||defined(SN8608D_LE) ||defined(SN8616D_LE)
	if(RTSP_SDP.width * RTSP_SDP.high > D1_PIXEL)
#elif defined(SN8604M)|| defined(SN8608M) ||defined(SN8608M_LE) ||defined(SN8616M_LE) || defined(SN9234H1)
	if(RTSP_SDP.width * RTSP_SDP.high > _1080P_PIXEL)
#endif
	{
		RET_FAILURE("Not Supported Resolution!");
	}
#endif
#if 1
	if(RTSP_SDP.adoType != 0)//���û��Ƶ��û��Ҫ��ȡ����Ƶ��׼ֵ��ͬ��
	{
		NTRANS_getFirstMediaPts(chn, &(PtsInfo[chn].FirstVideoPts), &(PtsInfo[chn].FirstAudioPts));
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------chn: %d, FistVideopts: %llu, FirstAudiopts: %llu\n", chn, PtsInfo[chn].FirstVideoPts, PtsInfo[chn].FirstAudioPts);
		
	}
#endif

	chn = chn + LOCALVEDIONUM;				
	CHECK(PRV_CheckIsCreateVdec(chn));
		
	if(!VochnInfo[chn].IsHaveVdec)
	{
		PRV_SetVochnInfo(&(VochnInfo[chn]), &RTSP_SDP);
		CHECK_RET(PRV_ChooseSlaveId(chn, &SlaveCreateVdec));
	
		if(SlaveCreateVdec.SlaveId > PRV_MASTER)
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Send MSG_ID_PRV_MCC_CREATEVDEC_REQ!\n");								
			SlaveState.SlaveCreatingVdec[chn] = 1;
			s_State_Info.bIsSlaveConfig = HI_TRUE;
			SN_SendMccMessageEx(SlaveCreateVdec.SlaveId, SUPER_USER_ID, MOD_PRV, MOD_PRV, 0, 0, MSG_ID_PRV_MCC_CREATEVDEC_REQ, &SlaveCreateVdec, sizeof(PRV_MccCreateVdecReq)); 					
			return HI_SUCCESS;
		}			
		else
		{
			if(PRV_STAT_PB == s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat || PRV_STAT_PIC == s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat)
			{	
				CurMasterCap += VochnInfo[chn].VdecCap;
				RET_FAILURE("In PB or Pic Stat!");
			}
			else
			{
				s32Ret = PRV_CreateVdecChn(VochnInfo[chn].VideoInfo.vdoType, VochnInfo[chn].VideoInfo.height, VochnInfo[chn].VideoInfo.width, VochnInfo[chn].u32RefFrameNum, VochnInfo[chn].VoChn);

				if(s32Ret == HI_ERR_VDEC_NOMEM)
				{
					PRV_ReCreateAllVdec();
					//�ٴδ����˽���ͨ��
					s32Ret = PRV_CreateVdecChn(VochnInfo[chn].VideoInfo.vdoType, VochnInfo[chn].VideoInfo.height, VochnInfo[chn].VideoInfo.width, VochnInfo[chn].u32RefFrameNum, VochnInfo[chn].VoChn);
				}

				if(s32Ret == HI_SUCCESS)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "-------Master-----Create Vdec: %d OK!\n", VochnInfo[chn].VdecChn);					

					//�ж������ӵ�ͨ���Ƿ��ڵ�ǰԤ��������
					//�ڵ�ǰ�����У���VO������������ʾ
					//if(HI_SUCCESS == PRV_VoChnIsInCurLayOut(s_VoDevCtrlDflt, VochnInfo[chn].VoChn))//ֻ���HD�жϣ�s_VoSecondDev��HDһ�£�ͬ��
					{	
						if(s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat == PRV_STAT_CTRL
							&&(s_astVoDevStatDflt[s_VoDevCtrlDflt].enCtrlFlag == PRV_CTRL_REGION_SEL || s_astVoDevStatDflt[s_VoDevCtrlDflt].enCtrlFlag == PRV_CTRL_ZOOM_IN)
							&& VochnInfo[chn].VoChn == s_astVoDevStatDflt[s_VoDevCtrlDflt].s32CtrlChn)
#if defined(SN9234H1)
						{
							CHECK(HI_MPI_VDEC_UnbindOutputChn(NoConfig_VdecChn, s_VoDevCtrlDflt, PRV_CTRL_VOCHN));
							//CHECK_RET(HI_MPI_VDEC_BindOutput(VochnInfo[chn].VdecChn, s_VoDevCtrlDflt, PRV_CTRL_VOCHN)); 
						}
						//else
						{
							//CHECK(HI_MPI_VO_ClearChnBuffer(s_VoDevCtrlDflt, VochnInfo[chn].VoChn, 1));
							//CHECK(HI_MPI_VO_ClearChnBuffer(s_VoSecondDev, VochnInfo[chn].VoChn, 1));	
							//if(VochnInfo[chn].IsBindVdec[s_VoDevCtrlDflt] != -1)
							{
								(HI_MPI_VDEC_UnbindOutputChn(NoConfig_VdecChn, HD, VochnInfo[chn].VoChn));
								VochnInfo[chn].IsBindVdec[HD] = -1;
							//	CHECK(HI_MPI_VDEC_UnbindOutputChn(DetVLoss_VdecChn, s_VoSecondDev, VochnInfo[chn].VoChn));
							//	VochnInfo[chn].IsBindVdec[s_VoSecondDev] = -1;
							}
							PRV_BindVoChnInMaster(HD, VochnInfo[chn].VoChn, s_astVoDevStatDflt[HD].s32PreviewIndex, s_astVoDevStatDflt[HD].enPreviewMode); 			
							//PRV_BindVoChnInMaster(s_VoSecondDev, VochnInfo[chn].VoChn, s_astVoDevStatDflt[s_VoSecondDev].s32PreviewIndex, s_astVoDevStatDflt[s_VoSecondDev].enPreviewMode);
							
							//VochnInfo[chn].bIsStopGetVideoData = 0;

						}
#else
						{
							PRV_VDEC_UnBindVpss(NoConfig_VdecChn,PRV_CTRL_VOCHN);							
							//CHECK_RET(HI_MPI_VDEC_BindOutput(VochnInfo[chn].VdecChn, s_VoDevCtrlDflt, PRV_CTRL_VOCHN)); 
						}
						//CHECK(HI_MPI_VO_ClearChnBuffer(DHD0, VochnInfo[chn].VoChn, 1));
						//CHECK(HI_MPI_VO_ClearChnBuffer(DSD0, VochnInfo[chn].VoChn, 1));	
						PRV_VDEC_UnBindVpss(NoConfig_VdecChn,VochnInfo[chn].VoChn);
						VochnInfo[chn].IsBindVdec[DHD0] = -1;
						//VochnInfo[chn].IsBindVdec[DSD0] = -1;
						PRV_BindVoChnInMaster(DHD0, VochnInfo[chn].VoChn, s_astVoDevStatDflt[DHD0].s32PreviewIndex, s_astVoDevStatDflt[DHD0].enPreviewMode); 			
						//PRV_BindVoChnInMaster(DSD0, VochnInfo[chn].VoChn, s_astVoDevStatDflt[DSD0].s32PreviewIndex, s_astVoDevStatDflt[DSD0].enPreviewMode); 			
#endif
					}

#if defined(SN9234H1)
					//����Spot�ڣ��ж�SPOT�ڵ�ǰ��VOͨ���Ƿ���������ӵ�ͨ����
					if(DEV_SPOT_NUM > 0)
					{
						//����ǣ�VO��Ҫ�Ƚ����DetVLoss_VdecChn(30)
						if(VochnInfo[chn].VoChn == s_astVoDevStatDflt[SPOT_VO_DEV].as32ChnOrder[SingleScene][s_astVoDevStatDflt[SPOT_VO_DEV].s32SingleIndex])
						{				
							//if(VochnInfo[chn].IsBindVdec[SD] == 0)
							{
								(HI_MPI_VDEC_UnbindOutputChn(NoConfig_VdecChn, SPOT_VO_DEV, SPOT_VO_CHAN));						
								VochnInfo[chn].IsBindVdec[SD] = -1;
							}
							CHECK(HI_MPI_VDEC_BindOutput(VochnInfo[chn].VdecChn, SPOT_VO_DEV, SPOT_VO_CHAN));			
							VochnInfo[chn].IsBindVdec[SD] = 1;
						}
					}
#endif
					CurMasterCap += VochnInfo[chn].VdecCap;
					VochnInfo[chn].IsHaveVdec = 1;
					
					//if(!IsChoosePlayAudio)
						PRV_PlayAudio(s_VoDevCtrlDflt);
					
				}
				else
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Create Vdec: %d fail!!!\n", VochnInfo[chn].VdecChn);
					CurCap -= VochnInfo[chn].VdecCap;
					
					SN_SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_NTRANS, 0, 0, MSG_ID_NTRANS_DELUSER_REQ, &DelUserReq, sizeof(deluser_used));			
					PRV_InitVochnInfo(chn);
#if defined(SN9234H1)
					VochnInfo[chn].IsBindVdec[HD] = 0;
#else					
					VochnInfo[chn].IsBindVdec[DHD0] = 0;
#endif					
					return HI_FAILURE;
				}				
			}
		}
	}
	else
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Channel: %d allready Create Vdec!!!\n", chn);
		return HI_FAILURE;
	}

	TRACE(SCI_TRACE_NORMAL, MOD_PRV, "-------Master-----chn: %d, VdecCap: %d, CurMasterCap: %d, Begin receive data!!!\n\n", 
									VochnInfo[chn].CurChnIndex, VochnInfo[chn].VdecCap, CurMasterCap);

	return HI_SUCCESS;
}
/********************************************************
������:PRV_CreateVdecChn_EX
��     ��:��������ͨ����������(Decoder)��ʹ��
��     ��:[in]chnָ��ͨ����
����ֵ:  ��

*********************************************************/

HI_S32 PRV_CreateVdecChn_EX(HI_S32 chn)
{
	HI_S32 s32Ret = 0;
	s32Ret = PRV_CreateVdecChn(VochnInfo[chn].VideoInfo.vdoType, VochnInfo[chn].VideoInfo.height, VochnInfo[chn].VideoInfo.width, VochnInfo[chn].u32RefFrameNum, VochnInfo[chn].VoChn);

	//��Ϊ�ڴ治�����´�������ͨ��ʧ�ܣ�
	//�ؽ���Ƭ����ͨ���ٴ�����ͨ��(��ʹ�ڴ����·���)
	if(s32Ret == HI_ERR_VDEC_NOMEM)
	{
		PRV_ReCreateAllVdec();
		s32Ret = PRV_CreateVdecChn(VochnInfo[chn].VideoInfo.vdoType, VochnInfo[chn].VideoInfo.height, VochnInfo[chn].VideoInfo.width, VochnInfo[chn].u32RefFrameNum, VochnInfo[chn].VoChn);
	}
	
	if(s32Ret == HI_SUCCESS)
	{	
#if defined(SN9234H1)
		(HI_MPI_VDEC_UnbindOutputChn(VochnInfo[chn].VdecChn, HD, VochnInfo[chn].VoChn));
		CHECK(HI_MPI_VO_ClearChnBuffer(s_VoDevCtrlDflt, VochnInfo[chn].VoChn, 1));
		VochnInfo[chn].IsBindVdec[HD] = -1;
		//�ж������ӵ�ͨ���Ƿ��ڵ�ǰԤ��������
		//�ڵ�ǰ�����У���VO������������ʾ
		if(HI_SUCCESS == PRV_VoChnIsInCurLayOut(s_VoDevCtrlDflt, VochnInfo[chn].VoChn))//ֻ���HD�жϣ�s_VoSecondDev��HDһ�£�ͬ��
		{	
			if(s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat == PRV_STAT_CTRL
				&&(s_astVoDevStatDflt[s_VoDevCtrlDflt].enCtrlFlag == PRV_CTRL_REGION_SEL || s_astVoDevStatDflt[s_VoDevCtrlDflt].enCtrlFlag == PRV_CTRL_ZOOM_IN)
				&& VochnInfo[chn].VoChn == s_astVoDevStatDflt[s_VoDevCtrlDflt].s32CtrlChn)
			{
				(HI_MPI_VDEC_UnbindOutputChn(VochnInfo[chn].VdecChn, s_VoDevCtrlDflt, PRV_CTRL_VOCHN));
				CHECK(HI_MPI_VO_ClearChnBuffer(s_VoDevCtrlDflt, VochnInfo[chn].VoChn, 1));
			}
			//else
			{

				VochnInfo[chn].VdecChn = VochnInfo[chn].VoChn;	
				CHECK(HI_MPI_VO_DisableChn(HD, VochnInfo[chn].VoChn));			
				PRV_BindVoChnInMaster(HD, VochnInfo[chn].VoChn, s_astVoDevStatDflt[HD].s32PreviewIndex, s_astVoDevStatDflt[HD].enPreviewMode);			
				CHECK(HI_MPI_VO_EnableChn(HD, VochnInfo[chn].VoChn));			
				//PRV_BindVoChnInMaster(s_VoSecondDev, VochnInfo[chn].VoChn, s_astVoDevStatDflt[s_VoSecondDev].s32PreviewIndex, s_astVoDevStatDflt[s_VoSecondDev].enPreviewMode);				
				//VochnInfo[chn].bIsStopGetVideoData = 0;
			}
		}
		else
		{
			VochnInfo[chn].VdecChn = VochnInfo[chn].VoChn;	
			(HI_MPI_VDEC_BindOutput(VochnInfo[chn].VdecChn, HD, VochnInfo[chn].VoChn));	
			VochnInfo[chn].IsBindVdec[HD] = 1;				
	//		CHECK(HI_MPI_VDEC_BindOutput(DetVLoss_VdecChn, s_VoSecondDev, VochnInfo[chn].VoChn));	
	//		VochnInfo[chn].IsBindVdec[s_VoSecondDev] = 0;				
		}
		CurMasterCap += VochnInfo[chn].VdecCap;
		VochnInfo[chn].IsHaveVdec = 1;
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Choose Master Chip---Create Vdec: %d OK!,CurMasterCap: %d, VochnInfo[chn].VdecCap: %d, totalcap: %d \n",
			VochnInfo[chn].VdecChn, CurMasterCap, VochnInfo[chn].VdecCap, TOTALCAPPCHIP);	
		PRV_PlayAudio(s_VoDevCtrlDflt);
#else	
	
		PRV_VDEC_UnBindVpss(VochnInfo[chn].VdecChn, VochnInfo[chn].VoChn);
		//(HI_MPI_VDEC_UnbindOutputChn(VochnInfo[chn].VdecChn, DHD0, VochnInfo[chn].VoChn));
		CHECK(HI_MPI_VO_ClearChnBuffer(DHD0, VochnInfo[chn].VoChn, 1));
		//CHECK(HI_MPI_VO_ClearChnBuffer(DSD0, VochnInfo[chn].VoChn, 1));
		VochnInfo[chn].IsBindVdec[DHD0] = -1;
		//VochnInfo[chn].IsBindVdec[DSD0] = -1;
		//�ж������ӵ�ͨ���Ƿ��ڵ�ǰԤ��������
		//�ڵ�ǰ�����У�vdec��vpss
		if(HI_SUCCESS == PRV_VoChnIsInCurLayOut(DHD0, VochnInfo[chn].VoChn))//ֻ���HD�жϣ�s_VoSecondDev��HDһ�£�ͬ��
		{	
			if(s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat == PRV_STAT_CTRL
				&&(s_astVoDevStatDflt[s_VoDevCtrlDflt].enCtrlFlag == PRV_CTRL_REGION_SEL || s_astVoDevStatDflt[s_VoDevCtrlDflt].enCtrlFlag == PRV_CTRL_ZOOM_IN)
				&& VochnInfo[chn].VoChn == s_astVoDevStatDflt[s_VoDevCtrlDflt].s32CtrlChn)
			{
			
				PRV_VDEC_UnBindVpss(VochnInfo[chn].VdecChn, PRV_CTRL_VOCHN);
				CHECK(HI_MPI_VO_ClearChnBuffer(DHD0, VochnInfo[chn].VoChn, 1));
				//CHECK(HI_MPI_VO_ClearChnBuffer(DSD0, VochnInfo[chn].VoChn, 1));
			}

			VochnInfo[chn].VdecChn = VochnInfo[chn].VoChn;	
			CHECK(HI_MPI_VO_DisableChn(DHD0, VochnInfo[chn].VoChn));			
			//CHECK(HI_MPI_VO_DisableChn(DSD0, VochnInfo[chn].VoChn));			
			PRV_BindVoChnInMaster(DHD0, VochnInfo[chn].VoChn, s_astVoDevStatDflt[DHD0].s32PreviewIndex, s_astVoDevStatDflt[DHD0].enPreviewMode);			
			//PRV_BindVoChnInMaster(DSD0, VochnInfo[chn].VoChn, s_astVoDevStatDflt[DSD0].s32PreviewIndex, s_astVoDevStatDflt[DSD0].enPreviewMode);			
			CHECK(HI_MPI_VO_EnableChn(DHD0, VochnInfo[chn].VoChn));			
			//CHECK(HI_MPI_VO_EnableChn(DSD0, VochnInfo[chn].VoChn));			
			
		}
		else
		{
			VochnInfo[chn].VdecChn = VochnInfo[chn].VoChn;	
			PRV_VPSS_ResetWH(VochnInfo[chn].VoChn,VochnInfo[chn].VdecChn,VochnInfo[chn].VideoInfo.width,VochnInfo[chn].VideoInfo.height);
			PRV_VDEC_BindVpss(VochnInfo[chn].VdecChn,VochnInfo[chn].VoChn);
			VochnInfo[chn].IsBindVdec[DHD0] = 1;				
			//VochnInfo[chn].IsBindVdec[DSD0] = 1;				
		}
		CurMasterCap += VochnInfo[chn].VdecCap;
		VochnInfo[chn].IsHaveVdec = 1;
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Choose Master Chip---Create Vdec: %d OK!,CurMasterCap: %d, VochnInfo[chn].VdecCap: %d, totalcap: %d\n",
			VochnInfo[chn].VdecChn, CurMasterCap, VochnInfo[chn].VdecCap, TOTALCAPPCHIP);					
		PRV_PlayAudio(s_VoDevCtrlDflt);
#endif
	}	
	else
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Create Vdec: %d fail!!!\n", VochnInfo[chn].VoChn);
		CurCap -= VochnInfo[chn].VdecCap;
		
		PRV_InitVochnInfo(chn);
		//VochnInfo[chn].bIsWaitIFrame = 1;
		VochnInfo[chn].bIsWaitGetIFrame = 1;
		
		return HI_FAILURE;
	}
	return HI_SUCCESS;
}
/*************************************************
Function: PRV_MSG_MCC_CreateVdecRsp
Description: ��Ƭ��������ͨ�����غ󣬸����Ƿ񴴽��ɹ������ö�Ӧ��״̬
Called By: 
Input:  msg_req��������ṹ�壬���������Ľ���ͨ����
Output: ����ϵͳ����Ĵ�����
Return: ��ϸ�ο��ĵ��Ĵ�����
Others: 
***********************************************************************/
STATIC HI_S32 PRV_MSG_MCC_CreateVdecRsp(const SN_MSG *msg_rsp)
{
	PRV_MccCreateVdecRsp *Rsp = (PRV_MccCreateVdecRsp *)msg_rsp->para;
	HI_S32 index = PRV_GetVoChnIndex(Rsp->VoChn);
	TRACE(SCI_TRACE_NORMAL, MOD_PRV, "MCC Create index: %d, Vdec: %d, Rsp->Result: %d\n", index, Rsp->VdecChn, Rsp->Result);
	if(index < 0)
		RET_FAILURE("------ERR: Invalid Index!");

	VochnInfo[index].MccCreateingVdec = 0;
#if defined(Hi3531)||defined(Hi3535)
	MccCreateingVdecCount--;
#endif
	if(Rsp->Result < 0)
	{
		CurCap -= VochnInfo[index].VdecCap;
		CurSlaveCap -= VochnInfo[index].VdecCap;
		CurSlaveChnCount--;
		
		PRV_InitVochnInfo(Rsp->VoChn);
		//VochnInfo[index].bIsWaitIFrame = 1;
		VochnInfo[index].bIsWaitGetIFrame = 1;

#if defined(SN9234H1)
		VochnInfo[index].IsBindVdec[HD] = 0;
#else
		VochnInfo[index].IsBindVdec[DHD0] = 0;
#endif
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ERR: MCC Create fail!  Rsp->Result: %d\n", Rsp->Result);
		return HI_FAILURE;
	}

	//�˳��طš�ͼƬ���ʱ����Ƭ�����´�������طš�ͼƬ���ʱ���ٵĽ���ͨ��
	//����֮ǰ����طš�ͼƬ���ʱ���󲿷�״̬δ�䣬�����˳��طš�ͼƬ���״̬ʱ��
	//��Ƭ��������ͨ���������ش���Ϣ����Ƭֻ��Ҫ�����������ݱ�ʶ	
	//��ƬExitPB()�ӿڷ��ش˷�֧
	else if(1 == Rsp->Result)
	{
		VochnInfo[index].IsHaveVdec = 1;
		
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Exiting PB, Vdec: %d!\n", VochnInfo[index].VdecChn);
		RET_SUCCESS("");
	}
	//����Ԥ��״̬���Ҵ����ɹ�
	else//(0 == Rsp->Result)
	{
 		//�ط�״̬�£���Ƭ��δ��������ͨ����������Ϣ
		if(PRV_STAT_PB == s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat || PRV_STAT_PIC == s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat)
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Warning: In PB, Slave Create Vdec: %d later!\n", Rsp->VdecChn);
						
			VochnInfo[index].IsHaveVdec = 0;
			VochnInfo[index].VdecChn = Rsp->VdecChn;
			VochnInfo[index].SlaveId = Rsp->SlaveId;		
			VochnInfo[index].IsConnect = 1;
			
			RET_SUCCESS();			
		}
		else
		{
			int i = 0;
#if defined(SN9234H1)
			//�Ƚ��VO
			for(i = 0; i < PRV_VO_DEV_NUM; i++)
			{
				if(i == SPOT_VO_DEV || i == AD)
					continue;
				//if(VochnInfo[index].IsBindVdec[i] != -1)
				{
					(HI_MPI_VDEC_UnbindOutputChn(VochnInfo[index].VdecChn, i, VochnInfo[index].VoChn));
					VochnInfo[index].IsBindVdec[i] = -1;
				}
			}
#else			
			//�Ƚ��VO
			for(i = 0; i < PRV_VO_MAX_DEV; i++)
			{
				if(i > DHD0)
					continue;
				//if(VochnInfo[index].IsBindVdec[i] != -1)
				{
					PRV_VDEC_UnBindVpss(VochnInfo[index].VdecChn,VochnInfo[index].VoChn);
					//(HI_MPI_VDEC_UnbindOutputChn(VochnInfo[index].VdecChn, i, VochnInfo[index].VoChn));
					VochnInfo[index].IsBindVdec[i] = -1;
				}
			}
#endif			
			VochnInfo[index].SlaveId = Rsp->SlaveId;
			VochnInfo[index].VoChn = Rsp->VoChn;
			VochnInfo[index].IsHaveVdec = 1;
			VochnInfo[index].IsConnect = 1;
			//PRV_RefreshVoDevScreen(DHD0, DISP_DOUBLE_DISP, s_astVoDevStatDflt[DHD0].as32ChnOrder[s_astVoDevStatDflt[DHD0].enPreviewMode]);
		}			
		
		//��Ӧ���ͨ���Ƿ��ڵ�ǰ���沼����
		if(HI_SUCCESS == PRV_VoChnIsInCurLayOut(s_VoDevCtrlDflt, VochnInfo[index].VoChn))
		{
			//ͨ����VI(0��0)����ʾ��Ƭ������
			if(s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat == PRV_STAT_CTRL
				&&(s_astVoDevStatDflt[s_VoDevCtrlDflt].enCtrlFlag == PRV_CTRL_REGION_SEL || s_astVoDevStatDflt[s_VoDevCtrlDflt].enCtrlFlag == PRV_CTRL_ZOOM_IN)
				&& VochnInfo[index].VoChn == s_astVoDevStatDflt[s_VoDevCtrlDflt].s32CtrlChn)
			{
#if defined(SN9234H1)
				HI_MPI_VDEC_UnbindOutputChn(VochnInfo[index].VdecChn, s_VoDevCtrlDflt, PRV_CTRL_VOCHN);
#else			
				PRV_VDEC_UnBindVpss(VochnInfo[index].VdecChn,PRV_CTRL_VOCHN);
#endif				
				CHECK(HI_MPI_VO_ClearChnBuffer(s_VoDevCtrlDflt, PRV_CTRL_VOCHN, 1));
			}
			//else
			{
				VochnInfo[index].VdecChn = Rsp->VdecChn;
				CHECK(HI_MPI_VO_DisableChn(s_VoDevCtrlDflt, VochnInfo[index].VoChn));
#if defined(SN9234H1)
				PRV_BindVoChnInSlave(s_VoDevCtrlDflt, VochnInfo[index].VoChn, s_astVoDevStatDflt[HD].s32PreviewIndex, s_astVoDevStatDflt[HD].enPreviewMode);				
#else							
				PRV_BindVoChnInSlave(s_VoDevCtrlDflt, VochnInfo[index].VoChn, s_astVoDevStatDflt[DHD0].s32PreviewIndex, s_astVoDevStatDflt[DHD0].enPreviewMode);
#endif								
				CHECK(HI_MPI_VO_EnableChn(s_VoDevCtrlDflt, VochnInfo[index].VoChn));			
				//PRV_BindVoChnInSlave(s_VoSecondDev, VochnInfo[index].VoChn, s_astVoDevStatDflt[s_VoSecondDev].s32PreviewIndex, s_astVoDevStatDflt[s_VoSecondDev].enPreviewMode);
			}
			#if 0
			//������Ƶ��ʧ����ͼ��
			if(IsOSDAlarmOn[index - LOCALVEDIONUM])
			{
				CHECK_RET(OSD_Ctl(VochnInfo[index].VoChn, 0, OSD_ALARM_TYPE));						
				IsOSDAlarmOn[index - LOCALVEDIONUM] = 0;
			}
			#endif
			//if(!IsChoosePlayAudio)
				PRV_PlayAudio(s_VoDevCtrlDflt);
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "---------Slave-----chn: %d, CurSlaveCap: %d, VochnInfo[index].VdecCap: %d\n", VochnInfo[index].CurChnIndex, CurSlaveCap, VochnInfo[index].VdecCap);
			
		}
		else
		{
			VochnInfo[index].VdecChn = Rsp->VdecChn;
		}
#if defined(SN9234H1)
		//SPOTֹͣ��ѯʱ��SPOT�ĵ�ǰ�����ͨ������仯(�������絽����Ƶ����֮)��
		//���Ҵ�ͨ��������Ƶʱ�ڴ�Ƭ��(��������Ƶʱ������Ƭ)
		if(DEV_SPOT_NUM > 0)
		{
			UINT8 SpotPollDelay;
			if (PARAM_OK != GetParameter(PRM_ID_PREVIEW_CFG, "ChanSwitchDelay", &SpotPollDelay, sizeof(SpotPollDelay), 1 + SD, SUPER_USER_ID, NULL))
			{			
				RET_FAILURE("ERR: get parameter ChanSwitchDelay fail!");
			}
			VO_CHN SpotVoChn = s_astVoDevStatDflt[SPOT_VO_DEV].as32ChnOrder[SingleScene][s_astVoDevStatDflt[SPOT_VO_DEV].s32SingleIndex];
			if(VochnInfo[index].VoChn == SpotVoChn)//Spot��ǰ���ͨ������仯
			{				
				CHECK(HI_MPI_VO_ClearChnBuffer(SPOT_VO_DEV, SPOT_VO_CHAN, 1)); /* ���VO���� */
				//if(VochnInfo[index].IsBindVdec[SPOT_VO_DEV] == 0)
				{
					(HI_MPI_VDEC_UnbindOutputChn(NoConfig_VdecChn, SPOT_VO_DEV, SPOT_VO_CHAN));
					VochnInfo[index].IsBindVdec[SPOT_VO_DEV] = -1;
				}
				//Spot��ǰ���ڲ��л�״̬����Ҫ����pciv������Ƶ���ݣ�
				//����ǿ���״̬����pciv�Ѿ�����
				if(SpotPollDelay == UnSwitch )
					PRV_start_pciv(VochnInfo[index].VoChn);
				//PRV_RefreshSpotOsd(VoChn);
			}
		}
#endif		
	}
	RET_SUCCESS("");
}

/*************************************************
Function: PRV_MSG_MCC_DesVdecRsp
Description: ��Ƭ���ٽ���ͨ�����غ󣬸����Ƿ����ٳɹ������ö�Ӧ��״̬
Called By: 
Input:  msg_req��������ṹ�壬�������ٵĽ���ͨ����
Output: ����ϵͳ����Ĵ�����
Return: ��ϸ�ο��ĵ��Ĵ�����
Others: 
***********************************************************************/
STATIC HI_S32 PRV_MSG_MCC_DesVdecRsp(const SN_MSG *msg_rsp)
{
	PRV_MccDestroyVdecRsp *DestroyVdecRsp = (PRV_MccDestroyVdecRsp *)msg_rsp->para;
	HI_S32 index = 0;
	index = PRV_GetVoChnIndex(DestroyVdecRsp->VdecChn);
	if(index < 0)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "------ERR: Invalid Index: %d, VdecChn: %d!\n", index, DestroyVdecRsp->VdecChn);
		return HI_FAILURE;
	}
#if (IS_DECODER_DEVTYPE == 0)	

	SlaveState.SlaveIsDesingVdec[index] = 0;
	s_State_Info.bIsSlaveConfig = HI_FALSE;
#endif
	//��Ƭû�д�����Ӧ�Ľ���ͨ��
	if(-1 == DestroyVdecRsp->Result)
	{
		VochnInfo[index].IsHaveVdec = 0;
		VochnInfo[index].IsConnect = 0;
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ERR: MCC destroy Vdec: %d fail!\n", DestroyVdecRsp->VdecChn);
		return HI_FAILURE;
	}
#if defined(Hi3531)||defined(Hi3535)	
	//����طš�ͼƬ���״̬ʱ����ƬҲ��Ҫ���ٽ���ͨ���������ش���Ϣ��
	//��Ƭֻ��Ҫ�رս������ݱ�ʶ,����״̬���䡣
	//�˳��ط�ʱҪ��������״̬���´�������ͨ��
	//��ƬEnterPB()�ӿ��з��ش�״̬
	else if(1 == DestroyVdecRsp->Result)
	{	
		VochnInfo[index].IsHaveVdec = 0;
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Enter into PB, VdecChn: %d!\n", DestroyVdecRsp->VdecChn);
		RET_SUCCESS("");
	}	
#endif	
	//��Ƭ���ٽ���ͨ���ɹ�
	else//(0 == DestroyVdecRsp->Result)
	{
		//���ڻط�״̬�¶Ͽ�����ʱ����Ϊ����ط�״̬ǰ�Ѿ����ٽ���ͨ�����ڴ�ֻ��������һЩ״̬
		//��ʱ����Ҫ��VO������VO�ѱ��ط�ռ��
#if defined(SN9234H1)
		if(PRV_STAT_PB == s_astVoDevStatDflt[HD].enPreviewStat || PRV_STAT_PIC == s_astVoDevStatDflt[HD].enPreviewStat)
#else		
		if(PRV_STAT_PB == s_astVoDevStatDflt[DHD0].enPreviewStat || PRV_STAT_PIC == s_astVoDevStatDflt[DHD0].enPreviewStat)
#endif		
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Warning: In PB, Vdec: %d allready destroy!\n", DestroyVdecRsp->VdecChn);
			CurCap -= VochnInfo[index].VdecCap;
			CurSlaveCap -= VochnInfo[index].VdecCap;	
			CurSlaveChnCount--;
			PRV_VoChnStateInit(VochnInfo[index].CurChnIndex);
			PRV_PtsInfoInit(VochnInfo[index].CurChnIndex);		
			PRV_InitVochnInfo(VochnInfo[index].VoChn);
			RET_SUCCESS();			
		}
		else
		{			
#if defined(SN9234H1)		
			CHECK(HI_MPI_VI_UnBindOutput(PRV_HD_DEV, 0, HD, VochnInfo[index].VoChn));					
			CHECK(HI_MPI_VO_ClearChnBuffer(HD, VochnInfo[index].VoChn, 1));
			//CHECK(HI_MPI_VO_DisableChn(i, VochnInfo[index].VoChn));
			VochnInfo[index].IsBindVdec[HD] = -1;
#else		
			//CHECK(HI_MPI_VI_UnBindOutput(PRV_HD_DEV, 0, DHD0, VochnInfo[index].VoChn));					
			CHECK(HI_MPI_VO_ClearChnBuffer(DHD0, VochnInfo[index].VoChn, 1));
			//CHECK(HI_MPI_VO_DisableChn(i, VochnInfo[index].VoChn));
			VochnInfo[index].IsBindVdec[DHD0] = -1;
#endif
			BufferSet(VochnInfo[index].VoChn + PRV_VIDEOBUFFER, MAX_ARRAY_NODE);			
			BufferSet(VochnInfo[index].VoChn + PRV_AUDIOBUFFER, MAX_ARRAY_NODE);			
			//BufferClear(chn + PRV_VIDEOBUFFER);
			//BufferClear(chn + PRV_AUDIOBUFFER);
			CurCap -= VochnInfo[index].VdecCap;	
			CurSlaveCap -= VochnInfo[index].VdecCap;
			CurSlaveChnCount--;
			PRV_VoChnStateInit(VochnInfo[index].CurChnIndex);
			PRV_PtsInfoInit(VochnInfo[index].CurChnIndex);					
			PRV_InitVochnInfo(VochnInfo[index].VoChn);
			//if(ScmGetListCtlState() == 1)//��λ����������ʧ��ʱ����"��������Ƶ"
			//	VochnInfo[index].VdecChn = DetVLoss_VdecChn;
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "slave destroy vdec: %d OK, CurSlaveCap: %d\n", DestroyVdecRsp->VdecChn, CurSlaveCap);
			if(HI_SUCCESS == PRV_VoChnIsInCurLayOut(s_VoDevCtrlDflt, VochnInfo[index].VoChn))
			{
#if defined(SN9234H1)
				if(s_astVoDevStatDflt[HD].enPreviewStat == PRV_STAT_CTRL)
				{
					if(VochnInfo[index].VoChn == s_astVoDevStatDflt[HD].s32CtrlChn)
					{
						CHECK(HI_MPI_VI_UnBindOutput(PRV_HD_DEV, 0, HD, PRV_CTRL_VOCHN));					
						CHECK(HI_MPI_VO_ClearChnBuffer(HD, PRV_CTRL_VOCHN, 1));
					}
				}
				//����disable Vo���������"������"ͼƬ��ʾ�쳣���������ͨ���´�����Ƭ��ʾ��Ҳ���쳣
				CHECK(HI_MPI_VO_DisableChn(HD, VochnInfo[index].VoChn));			
			//	CHECK(HI_MPI_VO_DisableChn(s_VoSecondDev, VochnInfo[index].VoChn));
				PRV_VLossVdecBindVoChn(HD, VochnInfo[index].VoChn, s_astVoDevStatDflt[HD].s32PreviewIndex, s_astVoDevStatDflt[HD].enPreviewMode);
			//	PRV_VLossVdecBindVoChn(s_VoSecondDev, VochnInfo[index].VoChn, s_astVoDevStatDflt[s_VoSecondDev].s32PreviewIndex, s_astVoDevStatDflt[s_VoSecondDev].enPreviewMode);
				CHECK(HI_MPI_VO_EnableChn(HD, VochnInfo[index].VoChn));
			//	CHECK(HI_MPI_VO_EnableChn(s_VoSecondDev, VochnInfo[index].VoChn));
#else			
				if(s_astVoDevStatDflt[DHD0].enPreviewStat == PRV_STAT_CTRL)
				{
					if(VochnInfo[index].VoChn == s_astVoDevStatDflt[DHD0].s32CtrlChn)
					{
						//CHECK(HI_MPI_VI_UnBindOutput(PRV_HD_DEV, 0, DHD0, PRV_CTRL_VOCHN));					
						CHECK(HI_MPI_VO_ClearChnBuffer(DHD0, PRV_CTRL_VOCHN, 1));
					}
				}
				//����disable Vo���������"������"ͼƬ��ʾ�쳣���������ͨ���´�����Ƭ��ʾ��Ҳ���쳣
				CHECK(HI_MPI_VO_DisableChn(DHD0, VochnInfo[index].VoChn));			
			//	CHECK(HI_MPI_VO_DisableChn(s_VoSecondDev, VochnInfo[index].VoChn));
				PRV_VLossVdecBindVoChn(DHD0, VochnInfo[index].VoChn, s_astVoDevStatDflt[DHD0].s32PreviewIndex, s_astVoDevStatDflt[DHD0].enPreviewMode);
			//	PRV_VLossVdecBindVoChn(s_VoSecondDev, VochnInfo[index].VoChn, s_astVoDevStatDflt[s_VoSecondDev].s32PreviewIndex, s_astVoDevStatDflt[s_VoSecondDev].enPreviewMode);
				CHECK(HI_MPI_VO_EnableChn(DHD0, VochnInfo[index].VoChn));
			//	CHECK(HI_MPI_VO_EnableChn(s_VoSecondDev, VochnInfo[index].VoChn));
#endif
				//sem_post(&sem_SendNoVideoPic);
				
				//if(!IsChoosePlayAudio)
					PRV_PlayAudio(s_VoDevCtrlDflt);
			}			
			else
			{
#if defined(SN9234H1)
				CHECK(HI_MPI_VDEC_BindOutput(VochnInfo[index].VdecChn, HD, VochnInfo[index].VoChn)); 	
				VochnInfo[index].IsBindVdec[HD] = 0;				
#else			
				PRV_VDEC_BindVpss(VochnInfo[index].VdecChn, VochnInfo[index].VoChn);
				//CHECK(HI_MPI_VDEC_BindOutput(VochnInfo[index].VdecChn, DHD0, VochnInfo[index].VoChn)); 	
				VochnInfo[index].IsBindVdec[DHD0] = 0;	
#endif							
	//			CHECK(HI_MPI_VDEC_BindOutput(VochnInfo[index].VdecChn, s_VoSecondDev, VochnInfo[index].VoChn)); 	
	//			VochnInfo[index].IsBindVdec[s_VoSecondDev] = 0;				
			}

#if defined(SN9234H1)
			if(DEV_SPOT_NUM > 0)
			{
				UINT8 SpotPollDelay;
				if (PARAM_OK != GetParameter(PRM_ID_PREVIEW_CFG, "ChanSwitchDelay", &SpotPollDelay, sizeof(SpotPollDelay), 1 + SD, SUPER_USER_ID, NULL))
				{
					RET_FAILURE("ERR: get parameter ChanSwitchDelay fail!");
				}
				VO_CHN SpotVoChn = s_astVoDevStatDflt[SPOT_VO_DEV].as32ChnOrder[SingleScene][s_astVoDevStatDflt[SPOT_VO_DEV].s32SingleIndex];
				if(VochnInfo[index].VoChn == SpotVoChn)
				{
					CHECK(HI_MPI_VO_ClearChnBuffer(SPOT_VO_DEV, SPOT_VO_CHAN, 1)); //���VO���� 
					if(SpotPollDelay == UnSwitch )
						PRV_HostStopPciv(CurrertPciv, MSG_ID_PRV_MCC_SPOT_PREVIEW_STOP_REQ);
					PRV_PrevInitSpotVo(VochnInfo[index].VoChn);
				}
			}
#endif
		}		
	}
	RET_SUCCESS("");
}
/*************************************************
Function: PRV_MSG_OverLinkReq
Description: �Ͽ�����
Called By: 
Input:  msg_req��������ṹ�壬�����Ͽ����ӵ�ͨ����
Output: ����ϵͳ����Ĵ�����
Return: ��ϸ�ο��ĵ��Ĵ�����
Others: 
***********************************************************************/
STATIC HI_S32 PRV_MSG_OverLinkReq(const SN_MSG *msg_req)
{
	HI_S32 i = 0, chn = 0, index = 0, s32Ret = 0;
	if(msg_req->msgId == MSG_ID_NTRANS_DELUSER_RSP)
	{					
		deluser_used *DelUser_Rsp = (deluser_used*)msg_req->para;
		chn = DelUser_Rsp->channel;
		index = chn + LOCALVEDIONUM;
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "-------------Receive MSG: MSG_ID_NTRANS_DELUSER_RSP---%d\n", DelUser_Rsp->channel);
	}
	else if(msg_req->msgId == MSG_ID_NTRANS_ONCEOVER_IND)
	{				
		NTRANS_ENDOFCOM_RSP *DisconNetReq = (NTRANS_ENDOFCOM_RSP *)msg_req->para;
		chn = DisconNetReq->channel;
		index = chn + LOCALVEDIONUM;
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "-------------Receive MSG: MSG_ID_NTRANS_ONCEOVER_IND---%d\n", DisconNetReq->channel);
	}	
	if(chn < 0 || chn >= MAX_IPC_CHNNUM)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Invalid Channel: %d, line: %d\n", chn, __LINE__);
		return HI_FAILURE;		
	}
	if(VochnInfo[index].VdecChn == DetVLoss_VdecChn
		|| VochnInfo[index].VdecChn == NoConfig_VdecChn)
	{
		RET_FAILURE("Error: Vdec for Picture---30 or 31");
	}
	
	//��ǰͨ���Ѿ���������ͨ��
	if(SlaveState.SlaveIsDesingVdec[index] == 1)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Slave Is Destroying Vdec: %d\n", VochnInfo[index].VdecChn);
		return HI_FAILURE;
	}
		
	//�Ͽ����ӵ�ͨ�����ڴ�Ƭ�����Ľ���ͨ����������Ϣ֪ͨ��Ƭ���ٽ���ͨ��
	if(VochnInfo[index].SlaveId > PRV_MASTER && VochnInfo[index].VdecChn >= 0)
	{
		PRV_GetVoChnIndex(VochnInfo[index].VoChn);
		PRV_MccDestroyVdecReq DestroyVdecReq;
		DestroyVdecReq.VdecChn = VochnInfo[index].VdecChn;
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------Send Msg to Slave, Destroy Vdec: %d\n", DestroyVdecReq.VdecChn);

		SlaveState.SlaveIsDesingVdec[index] = 1;
		s_State_Info.bIsSlaveConfig = HI_TRUE;
		SN_SendMccMessageEx(PRV_SLAVE_1, SUPER_USER_ID, MOD_PRV, MOD_PRV, 0, 0, MSG_ID_PRV_MCC_DESVDEC_REQ, &DestroyVdecReq, sizeof(PRV_MccDestroyVdecReq));
		for(i = 0; i < PRV_CurIndex; i++)
		{
			if(PRV_OldVdec[i] == VochnInfo[index].VdecChn)
			{
				NTRANS_FreeMediaData(PRV_OldVideoData[i]);
				PRV_OldVideoData[i] = NULL;
			}
		}
		
		return SN_SLAVE_MSG;
	}
	//��Ƭ�����Ľ���ͨ��
	else
	{		
		//��ǰ���ڻط�״̬����Ϊ����ط�ǰ�Ѿ������˽���ͨ�����ڴ�ֻ��Ҫ����һЩ״̬
		if(PRV_STAT_PB == s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat || PRV_STAT_PIC == s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat)
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Warning: In PB!!!Already destroy vdec: %d\n", VochnInfo[chn].VdecChn);
			CurCap -= VochnInfo[index].VdecCap;
			CurMasterCap -= VochnInfo[index].VdecCap; 				
			PRV_VoChnStateInit(chn);
			PRV_PtsInfoInit(VochnInfo[index].CurChnIndex);		
			PRV_InitVochnInfo(index);
			RET_SUCCESS("");			
		}
		//����Ԥ��״̬
		else
		{	
			s32Ret = PRV_WaitDestroyVdecChn(VochnInfo[index].VdecChn);
			if(s32Ret != HI_SUCCESS)//���ٽ���ͨ��ʧ�ܣ���������
			{
				VochnInfo[index].IsHaveVdec = 0;	
				VochnInfo[index].IsConnect = 0;
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Destroy Vdec: %d fail!", VochnInfo[index].VdecChn);
				return HI_FAILURE;
			}

			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Master destroy vdec: %d Ok!!\n", VochnInfo[index].VdecChn);
			
			//CHECK(HI_MPI_VO_DisableChn(DHD0, VochnInfo[chn].VoChn));			
			//CHECK(HI_MPI_VO_DisableChn(s_VoSecondDev, VochnInfo[chn].VoChn));
			BufferSet(VochnInfo[index].VoChn + PRV_VIDEOBUFFER, MAX_ARRAY_NODE);						
			BufferSet(VochnInfo[index].VoChn + PRV_AUDIOBUFFER, MAX_ARRAY_NODE);
			CurCap -= VochnInfo[index].VdecCap;
			CurMasterCap -= VochnInfo[index].VdecCap;			
			PRV_VoChnStateInit(chn);
			//CurIPCCount--;			
			PRV_PtsInfoInit(VochnInfo[index].CurChnIndex);		
			PRV_InitVochnInfo(index);
			//VochnInfo[chn].CurChnIndex = -1;
#if 1
			//���ڵ�ǰ���沼���У���VO������Ƶ�źŽ���ͨ��DetVLoss_VdecChn(30)
			if(HI_SUCCESS == PRV_VoChnIsInCurLayOut(s_VoDevCtrlDflt, VochnInfo[index].VoChn))
			{
#if defined(SN9234H1)
				CHECK(HI_MPI_VO_DisableChn(HD, VochnInfo[chn].VoChn));			
			//	CHECK(HI_MPI_VO_DisableChn(s_VoSecondDev, VochnInfo[chn].VoChn));
				PRV_VLossVdecBindVoChn(HD, VochnInfo[index].VoChn,  s_astVoDevStatDflt[HD].s32PreviewIndex, s_astVoDevStatDflt[HD].enPreviewMode);
			//	PRV_VLossVdecBindVoChn(s_VoSecondDev, VochnInfo[chn].VoChn, s_astVoDevStatDflt[s_VoSecondDev].s32PreviewIndex, s_astVoDevStatDflt[s_VoSecondDev].enPreviewMode);
				CHECK(HI_MPI_VO_EnableChn(HD, VochnInfo[chn].VoChn));			
			//	CHECK(HI_MPI_VO_EnableChn(s_VoSecondDev, VochnInfo[chn].VoChn));
#else			
			
				CHECK(HI_MPI_VO_DisableChn(DHD0, VochnInfo[chn].VoChn));			
			//	CHECK(HI_MPI_VO_DisableChn(s_VoSecondDev, VochnInfo[chn].VoChn));
				PRV_VLossVdecBindVoChn(DHD0, VochnInfo[index].VoChn,  s_astVoDevStatDflt[DHD0].s32PreviewIndex, s_astVoDevStatDflt[DHD0].enPreviewMode);
			//	PRV_VLossVdecBindVoChn(s_VoSecondDev, VochnInfo[chn].VoChn, s_astVoDevStatDflt[s_VoSecondDev].s32PreviewIndex, s_astVoDevStatDflt[s_VoSecondDev].enPreviewMode);
				CHECK(HI_MPI_VO_EnableChn(DHD0, VochnInfo[chn].VoChn));			
			//	CHECK(HI_MPI_VO_EnableChn(s_VoSecondDev, VochnInfo[chn].VoChn));
#endif
			//	sem_post(&sem_SendNoVideoPic);
				
			//	if(!IsChoosePlayAudio)
					PRV_PlayAudio(s_VoDevCtrlDflt);
					
			}
			else
			{
#if defined(SN9234H1)
				CHECK(HI_MPI_VDEC_BindOutput(VochnInfo[index].VdecChn, HD, VochnInfo[index].VoChn)); 	
				VochnInfo[index].IsBindVdec[HD] = 0;
#else			
				PRV_VDEC_BindVpss(VochnInfo[index].VdecChn, VochnInfo[index].VoChn);
				//CHECK(HI_MPI_VDEC_BindOutput(VochnInfo[index].VdecChn, DHD0, VochnInfo[index].VoChn)); 	
				VochnInfo[index].IsBindVdec[DHD0] = 0;
#endif								
		//		CHECK(HI_MPI_VDEC_BindOutput(VochnInfo[index].VdecChn, s_VoSecondDev, VochnInfo[chn].VoChn)); 	
		//		VochnInfo[chn].IsBindVdec[s_VoSecondDev] = 0;				
			}
#endif						

#if defined(SN9234H1)
			if(DEV_SPOT_NUM > 0)
			{
				//SPOT���ж�
				if(VochnInfo[index].VoChn == s_astVoDevStatDflt[SPOT_VO_DEV].as32ChnOrder[SingleScene][s_astVoDevStatDflt[SPOT_VO_DEV].s32SingleIndex])
				{			
					CHECK(HI_MPI_VO_ClearChnBuffer(SPOT_VO_DEV, SPOT_VO_CHAN, 1)); 
					//if(VochnInfo[index].IsBindVdec[SD] != 0)
					{
						CHECK(HI_MPI_VDEC_BindOutput(VochnInfo[index].VdecChn, SPOT_VO_DEV, SPOT_VO_CHAN));				
						VochnInfo[index].IsBindVdec[SD] = 0;	
					}
				}
			}
#endif
			//PRV_RefreshVoDevScreen(DHD0, DISP_DOUBLE_DISP, s_astVoDevStatDflt[DHD0].as32ChnOrder[s_astVoDevStatDflt[DHD0].enPreviewMode]);
		}
	}

	RET_SUCCESS("");
}
#if 0
/*************************************************
Function: PRV_MSG_ReCreateVdecIND
Description: ���´�������ͨ����
			���յ���I֡�ֱ��ʸı�ʱ�����Ͽ����ӵ�����£���Ҫ�������´�������ͨ��
Called By: 
Input:  msg_req��������ṹ�壬�������´����Ľ���ͨ����
Output: ����ϵͳ����Ĵ�����
Return: ��ϸ�ο��ĵ��Ĵ�����
Others: 
***********************************************************************/

HI_S32 PRV_MSG_ReCreateVdecIND(const SN_MSG *msg_req)
{
	TRACE(SCI_TRACE_NORMAL, MOD_PRV, "-------------Receive MSG: MSG_ID_PRV_RECREATEVDEC_IND\n");
	PRV_ReCreateVdecIND *ReCreateReq = (PRV_ReCreateVdecIND *)msg_req->para;
	HI_S32 index = PRV_GetVoChnIndex(ReCreateReq->VoChn);
	if(index < 0)
		RET_FAILURE("ERR: Invalid Channel");
	
	HI_S32 PreChnIndex = -1, IsReCreateOK = 1, VdecChn = -1, tmpCap = 0;
//	HI_S32 tmpCap = PRV_CompareToCif(ReCreateReq->height, ReCreateReq->width);
	TRACE(SCI_TRACE_HIGH, MOD_PRV, "New Height: %dNew Width: %d\n", ReCreateReq->height, ReCreateReq->width);			

	tmpCap = (ReCreateReq->height * ReCreateReq->width);

#if defined (SN_SLAVE_ON)	
	if(VochnInfo[index].SlaveId > 0)
	{
		PRV_MccReCreateVdecReq SlaveReCreateReq;
		SlaveReCreateReq.SlaveId = VochnInfo[index].SlaveId;
		SlaveReCreateReq.VoChn = ReCreateReq->VoChn;
		SlaveReCreateReq.VdecChn = ReCreateReq->VdecChn;
		SlaveReCreateReq.height = ReCreateReq->height;
		SlaveReCreateReq.width = ReCreateReq->width;
		SlaveReCreateReq.VdecCap = tmpCap;
		SN_SendMccMessageEx(SlaveReCreateReq.SlaveId, SUPER_USER_ID, MOD_PRV, MOD_PRV, 0, 0, MSG_ID_PRV_MCC_RECREATEVDEC_REQ, &SlaveReCreateReq, sizeof(PRV_MccReCreateVdecReq)); 					
	}
	else
#endif
	{
		if(HI_SUCCESS == PRV_WaitDestroyVdecChn(ReCreateReq->VdecChn))//�����ٽ���ͨ��
		{
			BufferSet(index + PRV_VIDEOBUFFER, MAX_ARRAY_NODE);						
			BufferSet(index + PRV_AUDIOBUFFER, MAX_ARRAY_NODE);
			//BufferClear(chn + PRV_VIDEOBUFFER);					
			//BufferClear(chn + PRV_AUDIOBUFFER);					
			CurCap -= VochnInfo[index].VdecCap;
			CurMasterCap -= VochnInfo[index].VdecCap;			
			
			VochnInfo[index].IsHaveVdec = 0;	
			VochnInfo[index].VdecChn = DetVLoss_VdecChn;
#if (DEV_TYPE == DEV_SN_9234_H4_1 || DEV_TYPE == DEV_SN_9234_H_V1_6 || DEV_TYPE == DEV_SN_9234_H_V1_8)			
			PtsInfo[VochnInfo[index].CurChnIndex].NewFramePts = 0;
#endif
			PreChnIndex = VochnInfo[index].CurChnIndex;
			VochnInfo[index].CurChnIndex = -1;
			
		}
		else
		{
			IsReCreateOK = 0;
			VochnInfo[index].IsHaveVdec = 0;//����ʧ�ܣ���������
			goto BindVdec;

		}
		//���ܱ����Ҫ�������ܼ��
		if(tmpCap > VochnInfo[index].VdecCap)
		{
			//��������
			if((CurCap - VochnInfo[index].VdecCap + tmpCap) > (TOTALCAPPCHIP * DEV_CHIP_NUM - LOCALVEDIONUM * D1) //�ֱ��ʱ仯���������ܳ���
				|| (VochnInfo[index].SlaveId == 0 && (CurMasterCap - VochnInfo[index].VdecCap + tmpCap) > (TOTALCAPPCHIP - LOCALVEDIONUM * D1)))//�ֱ��ʱ仯���µ�Ƭ���ܳ���
			{	
				//����Ͽ�����
				deluser_used DelUserReq;
				DelUserReq.channel = ReCreateReq->VoChn;
				SN_SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_NTRANS, 0, 0, MSG_ID_NTRANS_DELUSER_REQ, &DelUserReq, sizeof(deluser_used));			
				TRACE(SCI_TRACE_HIGH, MOD_PRV,"----------The current capbility is beyond the TOTALCAP, discard the newest channel---%d\n",  index);			
				IsReCreateOK = 0;
				goto BindVdec;
				//PRV_RefreshVoDevScreen(DHD0, DISP_DOUBLE_DISP, s_astVoDevStatDflt[DHD0].as32ChnOrder[s_astVoDevStatDflt[DHD0].enPreviewMode]);
				//return HI_FAILURE;
			}
		}
		//�ֱ��ʺϷ����ж�
		if(ReCreateReq->height <= 0 || ReCreateReq->width <=0
			|| ReCreateReq->height > 4096 || ReCreateReq->width > 4096)
		{
			TRACE(SCI_TRACE_HIGH, MOD_PRV, "InValid Height or Width: %d, %d\n", ReCreateReq->height, ReCreateReq->width);			
			IsReCreateOK = 0;
			goto BindVdec;
			//return HI_FAILURE;			
		}
		
		if(ReCreateReq->height <= 16)
		{
			ReCreateReq->height = 64;
		}
		if(ReCreateReq->width <=16)
		{
			ReCreateReq->width = 64;
		}
		
		if(HI_SUCCESS == PRV_CreateVdecChn(VochnInfo[index].VideoInfo.vdoType, ReCreateReq->height, ReCreateReq->width, ReCreateReq->VdecChn))
		{
			VochnInfo[index].VideoInfo.height = ReCreateReq->height;
			VochnInfo[index].VideoInfo.width = ReCreateReq->width;
			VochnInfo[index].VdecCap = ReCreateReq->height * ReCreateReq->width;			
			VochnInfo[index].CurChnIndex = PreChnIndex;
			VochnInfo[index].VdecChn = ReCreateReq->VdecChn;
			VochnInfo[index].IsHaveVdec = 1;	
			
			CurCap += VochnInfo[index].VdecCap;
			CurMasterCap += VochnInfo[index].VdecCap;
		}
		else
		{
			IsReCreateOK = 0;//�ؽ�����ͨ��ʧ�ܱ�ʶ
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ReCreate Vdec: fail!\n", ReCreateReq->VoChn);

		}
BindVdec:	
		//if(HI_SUCCESS == PRV_VoChnIsInCurLayOut(s_VoDevCtrlDflt, VochnInfo[index].VoChn))
		{	
			if(!IsReCreateOK)
				VdecChn = DetVLoss_VdecChn;
			else
				VdecChn = VochnInfo[index].VdecChn;
			if(s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat == PRV_STAT_CTRL
				&&(s_astVoDevStatDflt[s_VoDevCtrlDflt].enCtrlFlag == PRV_CTRL_REGION_SEL || s_astVoDevStatDflt[s_VoDevCtrlDflt].enCtrlFlag == PRV_CTRL_ZOOM_IN))
			{
				CHECK(HI_MPI_VDEC_BindOutput(VdecChn, s_VoDevCtrlDflt, PRV_CTRL_VOCHN)); 
			}
			else
			{
#if defined(SN9234H1)
				CHECK_RET(HI_MPI_VDEC_BindOutput(VdecChn, HD, VochnInfo[index].VoChn));
				VochnInfo[index].IsBindVdec[HD] = IsReCreateOK ? 1 : 0;	
#else			
				CHECK_RET(HI_MPI_VDEC_BindOutput(VdecChn, DHD0, VochnInfo[index].VoChn));
				VochnInfo[index].IsBindVdec[DHD0] = IsReCreateOK ? 1 : 0;
#endif					
			//	CHECK_RET(HI_MPI_VDEC_BindOutput(VdecChn, s_VoSecondDev, VochnInfo[index].VoChn));
			//	VochnInfo[index].IsBindVdec[s_VoSecondDev] = IsReCreateOK ? 1 : 0;

			}
			
			if(!IsChoosePlayAudio)
				PRV_PlayAudio(s_VoDevCtrlDflt);
		}	

		//PRV_RefreshVoDevScreen(DHD0, DISP_DOUBLE_DISP, s_astVoDevStatDflt[DHD0].as32ChnOrder[s_astVoDevStatDflt[DHD0].enPreviewMode]);
	}
	return HI_SUCCESS;
}
#endif


/*************************************************
Function: PRV_MSG_ReCreateAdecIND
Description: ���´�����Ƶ����ͨ����
Called By: 
Input:  msg_req��������ṹ�壬�������´�������Ƶͨ����
Output: ����ϵͳ����Ĵ�����
Return: ��ϸ�ο��ĵ��Ĵ�����
Others: 
***********************************************************************/
HI_S32 PRV_MSG_ReCreateAdecIND(const SN_MSG * msg_req)
{
	TRACE(SCI_TRACE_NORMAL, MOD_PRV, "-------------Receive MSG: MSG_ID_PRV_RECREATEADEC_IND\n");
	PRV_ReCreateAdecIND *ReCreateAdec = (PRV_ReCreateAdecIND *)msg_req->para;
	int chn = ReCreateAdec->chn;
	IsCreatingAdec = 1;
	CHECK_RET(PRV_StopAdec());
	IsCreateAdec = 0;

	if(ReCreateAdec->NewPtNumPerFrm == 160 || ReCreateAdec->NewPtNumPerFrm == 320)
	{
		VochnInfo[chn].AudioInfo.PtNumPerFrm = ReCreateAdec->NewPtNumPerFrm;
	}
	else
	{
		VochnInfo[chn].AudioInfo.PtNumPerFrm = 320;
	}
	
	CHECK_RET(PRV_StartAdecAo(VochnInfo[chn]));
	IsCreateAdec = 1;
#if defined(Hi3531)||defined(Hi3535)	
	IsAdecBindAo = 1;
#endif	
	Achn = chn;
	IsCreatingAdec = 0;
	CurPlayAudioInfo = VochnInfo[chn].AudioInfo;
	RET_SUCCESS("");
}
/************************************************************************/
/*                       END OF PRV_MSG_???()
                                               */
/************************************************************************/
static int get_chn_param_init(void)
{
	HI_S32 s32Ret = -1, i = 0, j = 0;
	UINT8	index = 255;
	PRM_PREVIEW_CFG_EX preview_info;
	PRM_PREVIEW_CFG_EX_EXTEND preview_info_exn;
	PRM_PREVIEW_ADV_CFG preview_Adv_info;
	PRM_Decode	stDecode;
	if (PARAM_OK != GetParameter(PRM_ID_PREVIEW_CFG_EX, NULL, &preview_info, sizeof(preview_info), 0, SUPER_USER_ID, NULL))
	{
		RET_FAILURE("get parameter PRM_PREVIEW_CFG_EX fail!");
	}
	if (PARAM_OK != GetParameter(PRM_ID_PREVIEW_CFG_EX_EXTEND, NULL, &preview_info_exn, sizeof(preview_info_exn), 0, SUPER_USER_ID, NULL))
	{
		RET_FAILURE("get parameter PRM_PREVIEW_CFG_EX_EXTN fail!");
	}

	if (PARAM_OK != GetParameter(PRM_ID_DECODEMODE_CFG, NULL, &stDecode, sizeof(stDecode), 0, SUPER_USER_ID, NULL))
	{
		RET_FAILURE("get parameter PRM_ID_DECODEMODE_CFG fail!");
	}
	
	PRV_CurDecodeMode = stDecode.DecodeMode;
	g_PrvType = stDecode.reserve0;
	for(i = 0; i < DEV_CHANNEL_NUM; i++)
		VochnInfo[i].PrvType = stDecode.reserve0;
	//Ԥ������
	for(i = 0; i < PRV_VO_MAX_DEV; i++)
	{//�����е�ͨ���Ƚ���һ�γ�ʼ��
		for(j = 0; j < SEVENINDEX; j++)
		{
			if(j < 7)
			{
				s_astVoDevStatDflt[i].AudioChn[j] = -1;
			}
			s_astVoDevStatDflt[i].as32ChnOrder[SingleScene][j] = -1;
			s_astVoDevStatDflt[i].as32ChnOrder[TwoScene][j] = -1;
			s_astVoDevStatDflt[i].as32ChnOrder[ThreeScene][j] = -1;
			s_astVoDevStatDflt[i].as32ChnOrder[FourScene][j] = -1;
			s_astVoDevStatDflt[i].as32ChnOrder[LinkFourScene][j] = -1;
				s_astVoDevStatDflt[i].as32ChnOrder[FiveScene][j] = -1;
			s_astVoDevStatDflt[i].as32ChnOrder[SixScene][j] = -1;
			s_astVoDevStatDflt[i].as32ChnOrder[SevenScene][j] = -1;
			s_astVoDevStatDflt[i].as32ChnOrder[EightScene][j] = -1;
			s_astVoDevStatDflt[i].as32ChnOrder[NineScene][j] = -1;
			s_astVoDevStatDflt[i].as32ChnOrder[LinkNineScene][j] = -1;
			s_astVoDevStatDflt[i].as32ChnOrder[SixteenScene][j] = -1;
			s_astVoDevStatDflt[i].as32ChnpollOrder[SingleScene][j] = -1;
			s_astVoDevStatDflt[i].as32ChnpollOrder[TwoScene][j] = -1;
			s_astVoDevStatDflt[i].as32ChnpollOrder[ThreeScene][j] = -1;
			s_astVoDevStatDflt[i].as32ChnpollOrder[FourScene][j] = -1;
			s_astVoDevStatDflt[i].as32ChnpollOrder[FiveScene][j] = -1;
			s_astVoDevStatDflt[i].as32ChnpollOrder[SixScene][j] = -1;
			s_astVoDevStatDflt[i].as32ChnpollOrder[SevenScene][j] = -1;
			s_astVoDevStatDflt[i].as32ChnpollOrder[EightScene][j] = -1;
			s_astVoDevStatDflt[i].as32ChnpollOrder[NineScene][j] = -1;
			s_astVoDevStatDflt[i].as32ChnpollOrder[SixteenScene][j] = -1;
		}
		/*����Ԥ��ͨ��˳��*/
		s_astVoDevStatDflt[i].as32ChnOrder[SingleScene][0] = UCHAR2INIT32(preview_info.SingleOrder);
		for(j = 0; j < CHANNEL_NUM; j++)
		{
            if(j < 3)
			{
				s_astVoDevStatDflt[i].as32ChnOrder[ThreeScene][j] = UCHAR2INIT32(preview_info_exn.ThreeOrder[j]);
			}
			if(j < 5)
			{
				s_astVoDevStatDflt[i].as32ChnOrder[FiveScene][j] = UCHAR2INIT32(preview_info_exn.FiveOrder[j]);
			}
			if(j < 7)
			{
				s_astVoDevStatDflt[i].as32ChnOrder[SevenScene][j] = UCHAR2INIT32(preview_info_exn.SevenOrder[j]);
			}
			if(j < 4)
			{
				s_astVoDevStatDflt[i].as32ChnOrder[FourScene][j] = UCHAR2INIT32(preview_info.FourOrder[j]);
				s_astVoDevStatDflt[i].as32ChnOrder[NineScene][j] = UCHAR2INIT32(preview_info.NineOrder[j]);
				s_astVoDevStatDflt[i].as32ChnOrder[SixteenScene][j] = UCHAR2INIT32(preview_info.SixteenOrder[j]);
			}
			else if(j < 9)
			{
				s_astVoDevStatDflt[i].as32ChnOrder[NineScene][j] = UCHAR2INIT32(preview_info.NineOrder[j]);
				s_astVoDevStatDflt[i].as32ChnOrder[SixteenScene][j] = UCHAR2INIT32(preview_info.SixteenOrder[j]);
			}
			else
			{
				s_astVoDevStatDflt[i].as32ChnOrder[SixteenScene][j] = UCHAR2INIT32(preview_info.SixteenOrder[j]);

			}
		}

		for(j = 0; j < 4; j++)
		{
			index = preview_info.AudioChn[j];
			//������Ƶͨ����
			if(j == 0 && index == 0)
				s_astVoDevStatDflt[i].AudioChn[j] = UCHAR2INIT32(preview_info.SingleOrder);
			else if(j == 1 && index < 4)
				s_astVoDevStatDflt[i].AudioChn[j] = UCHAR2INIT32(preview_info.FourOrder[index]);
			else if(j == 2 && index < 9)
				s_astVoDevStatDflt[i].AudioChn[j] = UCHAR2INIT32(preview_info.NineOrder[index]);
			else if(j == 3 && index < 16)
				s_astVoDevStatDflt[i].AudioChn[j] = UCHAR2INIT32(preview_info.SixteenOrder[index]);
		}
		for(j = 0; j < 3; j++)
		{
			index = preview_info_exn.AudioChn[j];
			//������Ƶͨ����
			if(j == 0 && index < 3)
				s_astVoDevStatDflt[i].AudioChn[4+j] = UCHAR2INIT32(preview_info_exn.ThreeOrder[index]);
			else if(j == 1 && index < 5)
				s_astVoDevStatDflt[i].AudioChn[4+j] = UCHAR2INIT32(preview_info_exn.FiveOrder[index]);
			else if(j == 2 && index < 7)
				s_astVoDevStatDflt[i].AudioChn[4+j] = UCHAR2INIT32(preview_info_exn.SevenOrder[index]);
			
		}

		/*�����豸��Ԥ��ģʽ*/
		s_astVoDevStatDflt[i].enPreviewMode = preview_info.PreviewMode;
		if(s_astVoDevStatDflt[i].enPreviewMode == SingleScene)
		{
			s_astVoDevStatDflt[i].bIsSingle = HI_TRUE;
			s_astVoDevStatDflt[i].s32SingleIndex = 0;			
		}
	}
	if (preview_info.reserve[1] > IntelligentMode){
		preview_info.reserve[1] = IntelligentMode;
	}
	OutPutMode = preview_info.reserve[1];
	if (PARAM_OK != GetParameter(PRM_ID_PREVIEW_ADV_CFG,NULL,&preview_Adv_info,sizeof(preview_Adv_info),1,SUPER_USER_ID,NULL))
	{
		RET_FAILURE("get parameter PRM_PREVIEW_CFG fail!");
	}
	/*���ñ��������˿�*/
	switch(preview_Adv_info.AlarmHandlePort)/*���������˿ڶ�Ӧ0-DHD0/VGA 1-CVBS1, 2-CVBS2*/
	{
#if defined(SN9234H1)	
		case 0:
			s_VoDevAlarmDflt = HD;
			break;
		case 1:
			s_VoDevAlarmDflt = s_VoSecondDev;
			break;
		case 2:
			s_VoDevAlarmDflt = SD;
			break;
#else
		case 0:
			s_VoDevAlarmDflt = DHD0;
			break;
		case 1:
			s_VoDevAlarmDflt = s_VoSecondDev;
			break;
		case 2:
			s_VoDevAlarmDflt = DSD0;
			break;
#endif			
		default:
			RET_FAILURE("param->preview_info.AlarmHandlePort out off range: 0~2");
	}

	/*����Ԥ����Ƶ*/
	PRV_Set_AudioPreview_Enable(preview_Adv_info.AudioPreview);
	IsAudioOpen = preview_Adv_info.AudioPreview[0];
	
#if !defined(Hi3535)
	PRM_DISPLAY_CFG_CHAN disp_info;
	for(i=0;i<g_Max_Vo_Num;i++)
	{
		//ͼ������
		s32Ret= GetParameter(PRM_ID_DISPLAY_CFG,NULL,&disp_info,sizeof(disp_info),i+1,SUPER_USER_ID,NULL);
		if(s32Ret!= PARAM_OK)
		{
			RET_FAILURE("get Preview_VideoParam param error!");
		}	
		s32Ret = Preview_SetVideoParam(i,&disp_info);
		if(s32Ret != HI_SUCCESS)
		{
			RET_FAILURE("Preview_initVideoParam error!");
		}
	}
#endif
	GetParameter (PRM_ID_LINKAGE_GROUP_CFG, NULL, g_PrmLinkAge_Cfg,sizeof (g_PrmLinkAge_Cfg), 0, SUPER_USER_ID, NULL);
	return s32Ret;
}


static void exit_mpp_sys(void)
{
	int i = 0;
	TRACE(SCI_TRACE_NORMAL, MOD_VAM, "exit_mpp_sys\n");
	s_State_Info.bIsRe_Init = 0;
#if defined(SN9234H1)
	int chan = 0;
	Prv_OSD_Close_fb(HD);
	Prv_OSD_Close_fb(AD);
	Prv_OSD_Close_fb(SD);
	if(DEV_SPOT_NUM > 0)
	{
		chan = s_astVoDevStatDflt[SPOT_VO_DEV].as32ChnOrder[SingleScene][s_astVoDevStatDflt[SPOT_VO_DEV].s32SingleIndex];
		if(chan > PRV_CHAN_NUM)
		{
			PRV_HostStopPciv(CurrertPciv, MSG_ID_PRV_MCC_SPOT_PREVIEW_STOP_REQ);
		}	
	}
#else	
	Prv_OSD_Close_fb(DHD0);
	//Prv_OSD_Close_fb(DSD0);
#endif
	//PRV_DestroyAllVdecChn();
	//PRV_DisableAllVoChn(DHD0);	
	//CHECK(HI_MPI_VO_DisableVideoLayer(DHD0));
	//PRV_DisableVoDev(DHD0);
	//PRV_DisableAllVoChn(s_VoSecondDev);		
	//CHECK(HI_MPI_VO_DisableVideoLayer(s_VoSecondDev));
	//PRV_DisableVoDev(s_VoSecondDev);

	PRV_DisableAudioPreview();
	PRV_SysExit();
	for(i = LOCALVEDIONUM; i < DEV_CHANNEL_NUM; i++)
		VochnInfo[i].IsHaveVdec = 0;
	s_bIsSysInit = HI_FALSE;
	PRINT_RED("MPP System (MOD_PRV) exit!\n");
}


/************************************************************************/
/* PRVģ��OSDʱ�������̺߳�����
                                                                     */
/************************************************************************/
char *weekday[] = {"������", "����һ", "���ڶ�", "������", "������", "������", "������"};
char *weekday_en[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

sem_t OSD_time_Sem;
unsigned int osdtime;
unsigned int getosdtime()
{
	return osdtime;
}
STATIC void *SendNvrNoVideoPicThread(void *parg)
{
#if defined(SN9234H1)
	char PicBuff[VLOSSPICBUFFSIZE] = {0};
	int dataLen = 0, s32Ret = 0;	

	dataLen = PRV_ReadNvrNoVideoPic(NVR_NOVIDEO_FILE, PicBuff);	
	if(dataLen <= 0)
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV, "--------Read NoVideoFile: %s---fail!\n", NVR_NOVIDEO_FILE);
		return (void*)(-1);
	}

	VDEC_STREAM_S stVstream; 
	stVstream.pu8Addr = PicBuff;
	stVstream.u32Len = dataLen;	
	stVstream.u64PTS = 0;

	char PicBuff_1[VLOSSPICBUFFSIZE] = {0};
	int dataLen_1 = 0;	

	dataLen_1 = PRV_ReadNvrNoVideoPic(DVS_NOCONFIG_FILE, PicBuff_1);	
	if(dataLen_1 <= 0)
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV, "--------Read NoVideoFile: %s---fail!\n", NVR_NOVIDEO_FILE);
		return (void*)(-1);
	}

	VDEC_STREAM_S stVstream_1; 
	stVstream_1.pu8Addr = PicBuff_1;
	stVstream_1.u32Len = dataLen_1;	
	stVstream_1.u64PTS = 0;	
	
#else

	unsigned char PicBuff1[VLOSSPICBUFFSIZE] = {0}, PicBuff2[VLOSSPICBUFFSIZE] = {0};
	HI_U32 dataLen1 = 0, dataLen2 = 0;	
	int s32Ret = 0;	

	dataLen1 = PRV_ReadNvrNoVideoPic(NVR_NOVIDEO_FILE_1, PicBuff1);	
	if(dataLen1 <= 0)
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV, "--------Read NoVideoFile: %s---fail!\n", NVR_NOVIDEO_FILE_1);
		return (void*)(-1);
	}

	VDEC_STREAM_S stVstream1; 
	stVstream1.pu8Addr = PicBuff1;
	stVstream1.u32Len = dataLen1;	
	stVstream1.u64PTS = 0;
	
	VDEC_STREAM_S stVstream2;
	dataLen2 = PRV_ReadNvrNoVideoPic(NVR_NOVIDEO_FILE_2, PicBuff2);
	if(dataLen2 <= 0)
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV, "--------Read NoVideoFile: %s---fail!\n", NVR_NOVIDEO_FILE_2);
		return (void*)(-1);
	}
	stVstream2.pu8Addr = PicBuff2;
	stVstream2.u32Len = dataLen2;	
	stVstream2.u64PTS = 0;
#endif
	int count = 0;
	//printf("--------------dataLen1: %d, dataLen2: %d\n", dataLen1, dataLen2);
	//if(MAX_IPC_CHNNUM > 0)
	//	PRV_NvrNoVideoDet();
	while(1)
	{
		//���ݻᱣ����VO����������������һֱ�������ݡ�ֻ���ڻ����л�����Ͽ�ͨ������ʱ���ٷ�������
		sem_wait(&sem_SendNoVideoPic);
	
		//pthread_mutex_lock(&send_data_mutex);
		//�豸����ʱ���˳�ϵͳ��ֹͣ����ͼƬ
		if(!s_bIsSysInit)
			continue;
		//��ǰ���ڻط�״̬������VO�ɻط�ģ��ռ�ã�������ͼƬ
		if(s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat == PRV_STAT_PB || s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat == PRV_STAT_PIC)
			continue;
		
		//if(MAX_IPC_CHNNUM > 0)
		//	PRV_NVRChnVLossDet();
		#if 0
		int s32Ret = 0, index = 0;
		VO_CHN SpotVoChn = 0;
		s32Ret = PRV_VLossInCurLayOut();
		//��ǰ���沼������������ͨ��������Ƶ����ʱ����Ҫ������������ƵͼƬ
		if(0 == DEV_SPOT_NUM && HI_FAILURE == s32Ret)
		{
			continue;	
		}
		else//NVR��ʱ��֧��SPOT�ڣ�Ϊ�Ժ����
		{//�������SPOT�ڣ���Ҫ�жϵ�ǰSPOT�ڶ�Ӧ�����ͨ���Ƿ�����Ƶ
			SpotVoChn = s_astVoDevStatDflt[SPOT_VO_DEV].as32ChnOrder[SingleScene][s_astVoDevStatDflt[SPOT_VO_DEV].s32SingleIndex];
			index = PRV_GetVoChnIndex(SpotVoChn);
			if(index < 0)
				continue;

			if(index >= 0 && HI_FAILURE == s32Ret
				&& VochnInfo[index].VdecChn != DetVLoss_VdecChn)//��ǰSPOT���ͨ��Ϊ����Ƶ�ź�
				continue;
		}
		#endif
		//PRV_ReleaseVdecData(DetVLoss_VdecChn);
		count = 0;
		while(count < 3)
		{
		//if(s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewStat == PRV_STAT_NORM)
#if defined(SN9234H1)
			s32Ret= HI_MPI_VDEC_SendStream(DetVLoss_VdecChn, &stVstream, HI_IO_BLOCK);
#else		
			s32Ret= HI_MPI_VDEC_SendStream(DetVLoss_VdecChn, &stVstream1, HI_IO_BLOCK);
#endif			
			if(s32Ret != HI_SUCCESS) //������Ƶ������
			{
				TRACE(SCI_TRACE_HIGH, MOD_PRV, "===========HI_MPI_VDEC_SendStream fail---0x%x!\n", s32Ret);
				//CHECK(HI_MPI_VDEC_SendStream(NoConfig_VdecChn, &stVstream_1, HI_IO_BLOCK)); //������Ƶ������
				//CHECK(HI_MPI_VDEC_StopRecvStream(DetVLoss_VdecChn));/*����ͨ��ǰ��ֹͣ��������*/
				//CHECK(HI_MPI_VDEC_DestroyChn(DetVLoss_VdecChn)); /* ������Ƶͨ�� */			
				//PRV_CreateVdecChn(JPEGENC, NOVIDEO_VDECHEIGHT, NOVIDEO_VDECWIDTH, DetVLoss_VdecChn);//��������ͨ�����"��������Ƶ"ͼƬ
			}
			usleep(40 * 1000);//��֤���͵�2֡���ݼ��40ms����
			count++;
		}
		//pthread_mutex_unlock(&send_data_mutex);
		
		//usleep(100 * 1000);//��֤���͵�2֡���ݼ��40ms����
		
	}
	
	return NULL;
}

STATIC void *Set_OSD_TimeProc(void *parg)
{
	HI_S32 s32Ret;
	HI_U32 	VLoss_Cnt=0;//,OSD_Re_Cnt=0;
	time_t rawtime;
	struct tm newtime;
	char m_strTime[MAX_BMP_STR_LEN], m_strQTime[MAX_BMP_STR_LEN];
	Log_pid(__FUNCTION__);
	struct timeval tv;

	Loss_State_Pre = Loss_State_Cur;
	s_State_Info.bIsRe_Init = 1;
	//sleep(5);
    while (1)
    {
    	//sem_wait(&OSD_time_Sem);
    	
    	tv.tv_sec = 0;
		tv.tv_usec = 500 * 1000;
		select(0, NULL, NULL, NULL, &tv);//�ȴ�500ms
    	
		pthread_mutex_lock(&s_osd_mutex);

		//	printf("osd change: bIsRe_Init=%d-->%d, bIsOsd_Init =%d-->%d ", bIsRe_Init, s_State_Info.bIsRe_Init, bIsOsd_Init, s_State_Info.bIsOsd_Init);
    	if(s_State_Info.bIsRe_Init && s_State_Info.bIsOsd_Init)
    	{
	    	time(&rawtime);
			osdtime = rawtime;
			localtime_r(&rawtime, &newtime);
			switch(s_OSD_Time_type)
			{
				case 0:
					if (MMI_GetLangID() == Chinese)
						SN_SPRINTF(m_strTime, sizeof(m_strTime), "%04d��%02d��%02d�� %s %02d:%02d:%02d",newtime.tm_year + 1900,newtime.tm_mon + 1,newtime.tm_mday,weekday[newtime.tm_wday],newtime.tm_hour,newtime.tm_min,newtime.tm_sec);
					else
						SN_SPRINTF(m_strTime, sizeof(m_strTime), "%04d-%02d-%02d  %s %02d:%02d:%02d",newtime.tm_year + 1900,newtime.tm_mon + 1,newtime.tm_mday,weekday_en[newtime.tm_wday],newtime.tm_hour,newtime.tm_min,newtime.tm_sec);
					SN_SPRINTF(m_strQTime, sizeof(m_strQTime), "%04d-%02d-%02d %02d:%02d:%02d",newtime.tm_year + 1900,newtime.tm_mon + 1,newtime.tm_mday,newtime.tm_hour,newtime.tm_min,newtime.tm_sec);				
					break;
				case 1:
					if (MMI_GetLangID() == Chinese)
						SN_SPRINTF(m_strTime, sizeof(m_strTime), "%02d��%02d��%04d�� %s %02d:%02d:%02d",newtime.tm_mon + 1,newtime.tm_mday,newtime.tm_year + 1900,weekday[newtime.tm_wday],newtime.tm_hour,newtime.tm_min,newtime.tm_sec);
					else
						SN_SPRINTF(m_strTime, sizeof(m_strTime), "%02d-%02d-%04d  %s %02d:%02d:%02d",newtime.tm_mon + 1,newtime.tm_mday,newtime.tm_year + 1900,weekday_en[newtime.tm_wday],newtime.tm_hour,newtime.tm_min,newtime.tm_sec);
					SN_SPRINTF(m_strQTime, sizeof(m_strQTime), "%02d-%02d-%04d %02d:%02d:%02d",newtime.tm_mon + 1,newtime.tm_mday,newtime.tm_year + 1900,newtime.tm_hour,newtime.tm_min,newtime.tm_sec);				
					break;
				case 2:
					if (MMI_GetLangID() == Chinese)
						SN_SPRINTF(m_strTime, sizeof(m_strTime), "%02d��%02d��%04d�� %s %02d:%02d:%02d",newtime.tm_mday,newtime.tm_mon + 1,newtime.tm_year + 1900,weekday[newtime.tm_wday],newtime.tm_hour,newtime.tm_min,newtime.tm_sec);
					else
						SN_SPRINTF(m_strTime, sizeof(m_strTime), "%02d-%02d-%04d  %s %02d:%02d:%02d",newtime.tm_mday,newtime.tm_mon + 1,newtime.tm_year + 1900,weekday_en[newtime.tm_wday],newtime.tm_hour,newtime.tm_min,newtime.tm_sec);
					SN_SPRINTF(m_strQTime, sizeof(m_strQTime), "%02d-%02d-%04d %02d:%02d:%02d",newtime.tm_mday,newtime.tm_mon + 1,newtime.tm_year + 1900,newtime.tm_hour,newtime.tm_min,newtime.tm_sec);				
					break;	
			}
			//����OSDʱ��
			s32Ret = OSD_Set_Time(m_strTime, m_strQTime);
			if(s32Ret != HI_SUCCESS)
			{
				TRACE(SCI_TRACE_HIGH, MOD_PRV,"Set_OSD_TimeProc OSD_Set_Time faild 0x%x!\n",s32Ret);
			}
    	}
		/*else if(!s_State_Info.bIsOsd_Init)
		{
			if(s_State_Info.bIsInit)
			{//Ԥ���Ѿ���ʼ��
				if(!(OSD_Re_Cnt % 30))
				{
					s32Ret = OSD_init(s_OSD_Time_type);
					if(s32Ret == 0)
					{
						s_State_Info.bIsOsd_Init = 1;
						OSD_Re_Cnt = 0;
					}
				}
				OSD_Re_Cnt ++;
			}
		}*/
		
		if(VLoss_Cnt %2 == 0)
		{
			//������Ƶ��ʧ��������
			if(LOCALVEDIONUM > 0)
			{
				s32Ret = PRV_VLossDet();
				if(s32Ret != HI_SUCCESS)
				{
					TRACE(SCI_TRACE_HIGH, MOD_PRV,"Set_OSD_TimeProc PRV_VLossDet faild 0x%x!\n",s32Ret);
				}
			}
			//����Ƶ�źŵ�����ͨ����Ҫ��ʾ"����Ƶ�ź�"����ͼƬ
			//if(MAX_IPC_CHNNUM > 0)
			//	PRV_NVRChnVLossDet();

			VLoss_Cnt++;
		}
		else
		{
			VLoss_Cnt = 0;
		}

		pthread_mutex_unlock(&s_osd_mutex);
	}
		
    return NULL;
}
#if defined(SN9234H1)
/************************************************************************/
/* �������������е�����Ƶ�źż�⹦�ܡ�
                                                                     */
/************************************************************************/

STATIC void* PRV_VLossDetProc()
{
	//printf("------Local VLossDetProc\n");
    int i = 0,is_lost = 0;
	Log_pid(__FUNCTION__);

	//printf("------------------Begin Local Video Loss Detect!!!\n");
	for (;;)
	{
		for (i = 0; i < LOCALVEDIONUM; i++)
		{
			is_lost = Preview_GetAVstate(i);
			//printf("------------is_lost: %d\n", is_lost);
			if(is_lost == -1)
			{
				fprintf(stderr, "%s: Preview_GetAVstate error!!!", __FUNCTION__);
				return NULL;
			}

			if (is_lost)
			{
#if defined(SN6116HE)||defined(SN6116LE) || defined(SN6108HE)  || defined(SN6108LE) || defined(SN8608D_LE) || defined(SN8608M_LE) || defined(SN8616D_LE) || defined(SN8616M_LE)|| defined(SN9234H1)
	        	//printf("@####################i = %d##################\n",i);
				if(i< PRV_CHAN_NUM)
	        	{//���Ϊǰ8��ͨ������ô����Ƭ��������Ƶ�źţ�����Ǻ�8·����Ҫ������Ϣ����Ƭ���ô�Ƭ��������Ƶ���ɺ�Ŷ
					if(i>= PRV_VI_CHN_NUM)
					{	//���Ϊͨ��5��8����ô���������豸2						
		            	CHECK(HI_MPI_VI_EnableUserPic(PRV_656_DEV, i%PRV_VI_CHN_NUM));
					}
					else
					{//���Ϊͨ��1��4����ô���������豸3						
						CHECK(HI_MPI_VI_EnableUserPic(PRV_656_DEV_1, i%PRV_VI_CHN_NUM));
					}
	        	}
#if defined(SN_SLAVE_ON)				
				else	
				//���Ϊͨ��9��16����ô������Ϣ����Ƭ��
				//���ȴ���Ƭ������Ϣ
				{
					Prv_Slave_Vloss_Ind slave_req;
					slave_req.chn = i;
					slave_req.state = HI_TRUE;
					//printf("@####################loss i = %d##################\n",i);
					SN_SendMccMessageEx(PRV_SLAVE_1,SUPER_USER_ID, MOD_PRV, MOD_PRV, 0, 0, MSG_ID_PRV_MCC_VLOSS_IND, &slave_req, sizeof(Prv_Slave_Vloss_Ind));		
				}
#endif
#else
				CHECK(HI_MPI_VI_EnableUserPic(i/PRV_VI_CHN_NUM, i%PRV_VI_CHN_NUM));
#endif
	        }
			else
			{
#if defined(SN6116HE)||defined(SN6116LE) || defined(SN6108HE)  || defined(SN6108LE) ||defined(SN8608D) || defined(SN8608M) || defined(SN8608D_LE) || defined(SN8608M_LE) || defined(SN8616D_LE) || defined(SN8616M_LE)|| defined(SN9234H1)
	        	//printf("@####################i = %d##################\n",i);
				if(i< PRV_CHAN_NUM)
	        	{//���Ϊǰ8��ͨ������ô����Ƭȡ������Ƶ�źţ�����Ǻ�8·����Ҫ������Ϣ����Ƭ���ô�Ƭȡ������Ƶ���ɺ�Ŷ
					if(i>= PRV_VI_CHN_NUM)
					{
		            	CHECK(HI_MPI_VI_DisableUserPic(PRV_656_DEV, i%PRV_VI_CHN_NUM));
					}
					else
					{
						CHECK(HI_MPI_VI_DisableUserPic(PRV_656_DEV_1, i%PRV_VI_CHN_NUM));
					}
				}
#if defined(SN_SLAVE_ON)				
				else	
				//���Ϊͨ��9��16����ô������Ϣ����Ƭ��
				//���ȴ���Ƭ������Ϣ
				{
					Prv_Slave_Vloss_Ind slave_req;
					slave_req.chn = i;
					slave_req.state = HI_FALSE;
					//printf("@####################unloss i = %d##################\n",i);
					SN_SendMccMessageEx(PRV_SLAVE_1,0xFFFFFFFF, MOD_PRV, MOD_PRV, 0, 0, MSG_ID_PRV_MCC_VLOSS_IND, &slave_req, sizeof(Prv_Slave_Vloss_Ind));		
				}
#endif				
#else
				CHECK(HI_MPI_VI_DisableUserPic(i/PRV_VI_CHN_NUM, i%PRV_VI_CHN_NUM));
#endif
			}      
		}	
		
		usleep(1000*800);
	}	

	return NULL;	
}
#endif
/*************************************************
Function: //PRV_TimeOut_Proc
Description: //��ʱ�������
Calls: 
Called By: //
Input: // 
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
static int PRV_TimeOut_Proc(HI_VOID)
{
	int ret=0;
	switch(s_State_Info.TimerType)
	{//�жϵ�ǰ��ʱ����
		case PRV_INIT://��ʼ����ʱ
			ret = PRV_Init_TimeOut(1);
			break;
		case PRV_LAY_OUT://�����л���ʱ
			//ret = PRV_LayOut_TimeOut();
			break;
		default:
			break;
	}
	return ret;
}

int PRV_SetPreviewVoDevInMode(int s32ChnCount)
{
	int i = 0;
	g_PlayInfo PlayState;
	
	SN_MEMSET(&PlayState, 0, sizeof(g_PlayInfo));
	//PlayState.FullScreenId = 0;
	PlayState.ImagCount = s32ChnCount;
	PlayState.IsSingle = (s32ChnCount == 1) ? 1 : 0;
	PlayState.PlayBackState = PLAY_ENTER;
	PlayState.IsPlaySound = 1;
	MMI_GetReplaySize(&PlayState.SubWidth, &PlayState.SubHeight);	

	int u32Width, u32Height;
	PlayBack_GetPlaySize((HI_U32 *)&u32Width, (HI_U32 *)&u32Height);
	
	PRV_SetPlayInfo(&PlayState);
	
	for(i = 0; i < CHANNEL_NUM; i++)
		VochnInfo[i].bIsPBStat = 1;

	PlayBack_StartVo();

	return 0;
}


/*************************************************
Function: //PRV_PreviewVoDevSingle
Description: �ط�ʱ�໭����ʾ��
Calls: 
Called By: //
Input: //VoDev:�豸��
		u32chn:�ط���ʾ��������
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
HI_S32 Playback_VoDevMul(VO_DEV VoDev, HI_U32 s32ChnCnt)
{	
		HI_U32 u32Width = 0, u32Height = 0,i=0,div=0,u32Width_s=0,u32Hight_s=0,width,height;
		g_PlayInfo stPlayInfo;
		HI_S32 s32Ret = 0;
		VO_VIDEO_LAYER_ATTR_S pstLayerAttr;
#if defined(SN9234H1)	
		VO_ZOOM_ATTR_S stVoZoomAttr;
#endif
		VO_CHN_ATTR_S stChnAttr;
	    RECT_S stSrcRect;
#if defined(SN9234H1)
		if(VoDev == SPOT_VO_DEV || VoDev == AD)
		{
			RET_FAILURE("Not Support Dev: SD!!");
		}
		
		if ( (VoDev < 0 || VoDev >= PRV_VO_MAX_DEV))
		{
			RET_FAILURE("Invalid Parameter: VoDev or u32Index");
		}
#else	
		
		if (VoDev < 0 || VoDev > DHD0/*VoDev >= PRV_VO_MAX_DEV*/)
		{
			RET_FAILURE("Invalid Parameter: VoDev or u32Index");
		}
		if(VoDev == DHD1)
			VoDev = DSD0;
#endif	

		if (s32ChnCnt<0)//PRV_VO_CHN_NUM)
		{
			RET_FAILURE("Invalid Parameter: VoChn ");
		}

		
		PRV_GetPlayInfo(&stPlayInfo);
		s32Ret = HI_MPI_VO_GetVideoLayerAttr(VoDev,&pstLayerAttr);
		u32Width = pstLayerAttr.stImageSize.u32Width;
		u32Height = pstLayerAttr.stImageSize.u32Height;
		PlayBack_GetPlaySize(&u32Width,&u32Height);
		u32Width_s = u32Width;
		u32Hight_s = u32Height;
	    if(s32ChnCnt==9)
	   	{
             while(u32Width%6 != 0)
			     u32Width++;
		     while(u32Height%6 != 0)
			     u32Height++;
		}
		 for(i=0;i<s32ChnCnt;i++)
		{
          #if defined(Hi3535)
			  HI_MPI_VO_ResumeChn(VoDev, i);
			  HI_MPI_VO_HideChn(VoDev,i);
          #else
			  HI_MPI_VO_ChnResume(VoDev,i);
			  HI_MPI_VO_ChnHide(VoDev,i);
          #endif
		}
		 div = sqrt(s32ChnCnt);		/* ����ÿ��ͨ���Ŀ�Ⱥ͸߶� */

	     width = (HI_S32)(u32Width/div);//ÿ�����ͨ���Ŀ��
	     height = (HI_S32)(u32Height/div);
		for (i = 0; i < s32ChnCnt; i++)
		{
			s32Ret = HI_MPI_VO_GetChnAttr(VoDev, i, &stChnAttr);
			stChnAttr.stRect.s32X = width*(i%div);/* ����������ʾʱͨ���Ŵ�С������������ */
			stChnAttr.stRect.s32Y = height*(i/div);
			stChnAttr.stRect.u32Width= width;
			stChnAttr.stRect.u32Height = height;
		    stSrcRect.s32X	= stChnAttr.stRect.s32X;
		    stSrcRect.s32Y	= stChnAttr.stRect.s32Y;
		    stSrcRect.u32Width	 = stChnAttr.stRect.u32Width;
		    stSrcRect.u32Height  = stChnAttr.stRect.u32Height;	
			if(s32ChnCnt==9)
            { 
			   if((i + 1) % 3 == 0)//���һ��
				stSrcRect.u32Width = u32Width_s- stSrcRect.s32X;
			   if(i > 5 && i < 9)//���һ��
				stSrcRect.u32Height = u32Hight_s- stSrcRect.s32Y;
		     }
		    stChnAttr.stRect = PRV_ReSetVoRect(OutPutMode, VochnInfo[i].VideoInfo.width, VochnInfo[i].VideoInfo.height, stSrcRect);
            
			stChnAttr.stRect.s32X 		&= (~0x01);
		    stChnAttr.stRect.s32Y		&= (~0x01);
		    stChnAttr.stRect.u32Width   &= (~0x01);
		    stChnAttr.stRect.u32Height  &= (~0x01);
		    s32Ret = HI_MPI_VO_SetChnAttr(VoDev, i, &stChnAttr);
		    if (s32Ret != HI_SUCCESS)
		    {
			   TRACE(SCI_TRACE_NORMAL, MOD_PRV,"VoDevID:%d--In set channel %d attr failed with %x! ",VoDev, i, s32Ret);
		    }
				  
#if !defined(Hi3535)
		    HI_MPI_VO_SetChnField(VoDev,i, VO_FIELD_BOTH);
#endif
		
#if defined(SN9234H1)
			if(VochnInfo[i].SlaveId > PRV_MASTER)
			{
			   stVoZoomAttr.stZoomRect=stChnAttr.stRect;
				stVoZoomAttr.enField = VIDEO_FIELD_FRAME;
				HI_MPI_VO_SetZoomInWindow(VoDev, i, &stVoZoomAttr);//genggai
			
			}
				//s32Ret = HI_MPI_VO_SetChnAttr(VoDev, i, &stChnAttr);
#endif
			HI_MPI_VO_EnableChn(VoDev, i);
           #if defined(Hi3535)
				HI_MPI_VO_ShowChn(VoDev, i);
           #else
				HI_MPI_VO_ChnShow(VoDev, i);
           #endif
	   }
	
		 RET_SUCCESS("");
	
}



/*************************************************
Function: //PRV_PreviewVoDevSingle
Description: �ط�ʱ��������ʾ��
Calls: 
Called By: //
Input: //VoDev:�豸��
		u32chn:�ط���ʾ��������
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/

HI_S32 Playback_VoDevSingle(VO_DEV VoDev, HI_U32 u32chn)
{	
    HI_U32 u32Width = 0, u32Height = 0,i=0;
	g_PlayInfo stPlayInfo;
	VO_CHN VoChn = 0;
	HI_S32 s32Ret = 0;
	VO_VIDEO_LAYER_ATTR_S pstLayerAttr;
#if defined(SN9234H1)	
	VO_ZOOM_ATTR_S stVoZoomAttr;
#endif
	VO_CHN_ATTR_S stChnAttr;
     RECT_S stSrcRect;
	VoChn=u32chn;
#if defined(SN9234H1)
	if(VoDev == SPOT_VO_DEV || VoDev == AD)
	{
		RET_FAILURE("Not Support Dev: SD!!");
	}
	
	if ( (VoDev < 0 || VoDev >= PRV_VO_MAX_DEV))
	{
		RET_FAILURE("Invalid Parameter: VoDev or u32Index");
	}
#else	
	
	if (VoDev < 0 || VoDev > DHD0/*VoDev >= PRV_VO_MAX_DEV*/)
	{
		RET_FAILURE("Invalid Parameter: VoDev or u32Index");
	}
	if(VoDev == DHD1)
		VoDev = DSD0;
#endif	
	if (VoChn < 0 || VoChn >= 4)//PRV_VO_CHN_NUM)
	{
		RET_FAILURE("Invalid Parameter: VoChn ");
	}

	PRV_GetPlayInfo(&stPlayInfo);
	s32Ret = HI_MPI_VO_GetVideoLayerAttr(VoDev,&pstLayerAttr);
	u32Width = pstLayerAttr.stImageSize.u32Width;
	u32Height = pstLayerAttr.stImageSize.u32Height;
	if(stPlayInfo.FullScreenId==1||stPlayInfo.IsZoom==1)
	{   
	}
	else
	{
         u32Width = u32Width - stPlayInfo.SubWidth;
	     u32Height = u32Height - stPlayInfo.SubHeight;
	}
     PlayBack_GetPlaySize(&u32Width,&u32Height);
     for(i=0;i<stPlayInfo.ImagCount;i++)
    {
          #if defined(Hi3535)
    	  HI_MPI_VO_ResumeChn(VoDev, i);
		  HI_MPI_VO_HideChn(VoDev,i);
          #else
		  HI_MPI_VO_ChnResume(VoDev,i);
		  HI_MPI_VO_ChnHide(VoDev,i);
          #endif
    }

   s32Ret = HI_MPI_VO_GetChnAttr(VoDev, VoChn, &stChnAttr);
		
   stChnAttr.stRect.s32X = 0;
   stChnAttr.stRect.s32Y =0;
   stChnAttr.stRect.u32Width= u32Width;
   stChnAttr.stRect.u32Height = u32Height;
   stSrcRect.s32X	= stChnAttr.stRect.s32X;
   stSrcRect.s32Y	= stChnAttr.stRect.s32Y;
	stSrcRect.u32Width	 = stChnAttr.stRect.u32Width;
	stSrcRect.u32Height  = stChnAttr.stRect.u32Height;				
	stChnAttr.stRect = PRV_ReSetVoRect(OutPutMode, VochnInfo[VoChn].VideoInfo.width, VochnInfo[VoChn].VideoInfo.height, stSrcRect);
	stChnAttr.stRect.s32X 		&= (~0x01);
	stChnAttr.stRect.s32Y		&= (~0x01);
	stChnAttr.stRect.u32Width   &= (~0x01);
	stChnAttr.stRect.u32Height  &= (~0x01);
	s32Ret = HI_MPI_VO_SetChnAttr(VoDev, VoChn, &stChnAttr);
	if (s32Ret != HI_SUCCESS)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV,"VoDevID:%d--In set channel %d attr failed with %x! ",VoDev, i, s32Ret);
	}
   s32Ret = HI_MPI_VO_SetChnAttr(VoDev, VoChn, &stChnAttr);
   if (s32Ret != HI_SUCCESS)
  {
	  TRACE(SCI_TRACE_NORMAL, MOD_PRV,"VoDevID:%d--In set channel %d attr failed with %x! ",VoDev, i, s32Ret);
  }
#if !defined(Hi3535)
	HI_MPI_VO_SetChnField(VoDev,VoChn, VO_FIELD_BOTH);
#endif
#if defined(SN9234H1)
       if(VochnInfo[VoChn].SlaveId > PRV_MASTER)
       	{
		stVoZoomAttr.stZoomRect=stChnAttr.stRect;
        stVoZoomAttr.enField = VIDEO_FIELD_FRAME;
		HI_MPI_VO_SetZoomInWindow(VoDev, VoChn, &stVoZoomAttr);
       	}
#endif
		HI_MPI_VO_EnableChn(VoDev, VoChn);
#if defined(Hi3535)
		HI_MPI_VO_ShowChn(VoDev, VoChn);
#else
		HI_MPI_VO_ChnShow(VoDev, VoChn);
#endif

     RET_SUCCESS("");

}


/*************************************************
Function: //PRV_ZoomInPic
Description://�طŻ��л�,���ӷŴ�״̬ʱר��
Calls: 
Called By: //
Input: // bIsShow:�Ƿ���ʾ
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
***********************************************************************/
	
	HI_S32 Playback_ZoomInPic(HI_BOOL bIsShow)
	{
	
		VO_DEV VoDev = s_VoDevCtrlDflt;
		VO_CHN VoChn = PRV_CTRL_VOCHN;
#if defined(SN9234H1)	
		VI_DEV ViDev = -1;
		VI_CHN ViChn = -1;
#endif
		VO_CHN_ATTR_S stVoChnAttr;
		int index = 0;
		RECT_S stSrcRect, stDestRect;
	    g_PlayInfo stPlayInfo;
        PRV_GetPlayInfo(&stPlayInfo);
        if(stPlayInfo.IsZoom==0)
        {
            RET_FAILURE("Warning!!! not int zoom in stat now!!!");
		}
#if defined(SN9234H1)
	
	 /*2010-9-19 ˫����*/
	//VoDev = HD;
	//again:
	//��ȡs32CtrlChn��Ӧ��VI
		if(OldCtrlChn >= 0)
		{
			if(OldCtrlChn < (LOCALVEDIONUM < PRV_CHAN_NUM ? LOCALVEDIONUM : PRV_CHAN_NUM))
			{
				if(OldCtrlChn < PRV_VI_CHN_NUM)
				{
					ViDev = PRV_656_DEV_1;
				}
				else
				{
					ViDev = PRV_656_DEV;
				}
				ViChn = OldCtrlChn%PRV_VI_CHN_NUM;
			}
			//�ڴ�Ƭ
			else if(OldCtrlChn >= PRV_CHAN_NUM && OldCtrlChn < LOCALVEDIONUM)
			{
				ViDev = PRV_HD_DEV;
			}
		}
#endif
		//1.���VOͨ��
		if(OldCtrlChn >= 0)
		{	
#if defined(SN9234H1)
			if(ViDev != -1)//ģ����Ƶͨ��
				CHECK(HI_MPI_VI_UnBindOutput(ViDev, ViChn, VoDev, VoChn));
			else
			{
				index = PRV_GetVoChnIndex(OldCtrlChn);
				if(index < 0)
					RET_FAILURE("-----------Invalid Index!");
				if(VochnInfo[index].SlaveId > PRV_MASTER)
					(HI_MPI_VI_UnBindOutput(PRV_HD_DEV, 0, VoDev, VoChn));
				else
				{
					(HI_MPI_VDEC_UnbindOutputChn(VochnInfo[index].VdecChn, VoDev, VoChn));
					VochnInfo[index].IsBindVdec[VoDev] = -1;
				}
				
			}
#else
			index = PRV_GetVoChnIndex(stPlayInfo.ZoomChn);
			if(index < 0)
				RET_FAILURE("-----------Invalid Index!");
#if defined(Hi3535)
			CHECK(HI_MPI_VO_HideChn(VoDev, VoChn));
#else
			CHECK(HI_MPI_VO_ChnHide(VoDev, VoChn));
#endif
			CHECK(PRV_VDEC_UnBindVpss(VochnInfo[index].VdecChn, VoChn));
			VochnInfo[index].IsBindVdec[VoDev] = -1;
#if defined(Hi3535)
			CHECK(PRV_VO_UnBindVpss(PIP,VoChn,VoChn,VPSS_BSTR_CHN));
#else
			CHECK(PRV_VO_UnBindVpss(VoDev, VoChn, VoChn, VPSS_PRE0_CHN));
#endif
#endif		
		}
		//2.�ر�VOͨ��
#if defined(Hi3535)
		CHECK(HI_MPI_VO_DisableChn(PIP ,VoChn));
#else	
		CHECK(HI_MPI_VO_DisableChn(VoDev ,VoChn));
#endif
	
		if (bIsShow)
		{		
#if defined(SN9234H1)
             HI_S32 Temp_Chn = (HI_S32)stPlayInfo.ZoomChn;
			if(Temp_Chn< (LOCALVEDIONUM < PRV_CHAN_NUM ? LOCALVEDIONUM : PRV_CHAN_NUM))
			{
				if(Temp_Chn < PRV_VI_CHN_NUM)
				{
					ViDev = PRV_656_DEV_1;
				}
				else
				{
					ViDev = PRV_656_DEV;
				}
				ViChn = stPlayInfo.ZoomChn %PRV_VI_CHN_NUM;
			}
			else if(Temp_Chn >= PRV_CHAN_NUM && Temp_Chn  < LOCALVEDIONUM)
			{
				ViDev = PRV_HD_DEV;
			}
	
#endif
			//3.����VOͨ��		
			index = PRV_GetVoChnIndex(stPlayInfo.ZoomChn);
			if(index < 0)
				RET_FAILURE("------ERR: Invalid Index!");
#if defined(Hi3535)
			CHECK_RET(HI_MPI_VO_GetChnAttr(PIP,stPlayInfo.ZoomChn, &stVoChnAttr));
#else
			CHECK_RET(HI_MPI_VO_GetChnAttr(VoDev, stPlayInfo.ZoomChn, &stVoChnAttr));
#endif
			stVoChnAttr.stRect.s32X 	 = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width*3/4;
			stVoChnAttr.stRect.s32Y 	 = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height*3/4;
			stVoChnAttr.stRect.u32Height = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height*1/4;
			stVoChnAttr.stRect.u32Width  = s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width*1/4;
			stSrcRect.s32X		 = stVoChnAttr.stRect.s32X;
			stSrcRect.s32Y		 = stVoChnAttr.stRect.s32Y;
			stSrcRect.u32Width	 = stVoChnAttr.stRect.u32Width;
			stSrcRect.u32Height  = stVoChnAttr.stRect.u32Height;
			
			stDestRect = PRV_ReSetVoRect(OutPutMode, VochnInfo[index].VideoInfo.width, VochnInfo[index].VideoInfo.height, stSrcRect);
			stVoChnAttr.stRect.s32X 	 = stDestRect.s32X		& (~0x01);
			stVoChnAttr.stRect.s32Y 	 = stDestRect.s32Y		& (~0x01);
			stVoChnAttr.stRect.u32Width  = stDestRect.u32Width	& (~0x01);
			stVoChnAttr.stRect.u32Height = stDestRect.u32Height & (~0x01);
			//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "-----------w=%d, h=%d, d_w=%d, d_h=%d, x=%d, y=%d, s_w=%d, s_h=%d\n", s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Width, s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stDispRect.u32Height,
			//	s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Width, s_astVoDevStatDflt[VoDev].stVideoLayerAttr.stImageSize.u32Height,
			//	stVoChnAttr.stRect.s32X ,stVoChnAttr.stRect.s32Y,stVoChnAttr.stRect.u32Width,stVoChnAttr.stRect.u32Height);
#if defined(Hi3535)
			stVoChnAttr.u32Priority = 1;
			CHECK_RET(HI_MPI_VO_SetChnAttr(PIP, VoChn, &stVoChnAttr));
			CHECK_RET(PRV_VO_BindVpss(PIP,VoChn,VoChn,VPSS_BSTR_CHN));
#elif defined(Hi3531)		
			stVoChnAttr.u32Priority = 1;
			CHECK_RET(HI_MPI_VO_SetChnAttr(VoDev, VoChn, &stVoChnAttr));
			CHECK_RET(PRV_VO_BindVpss(VoDev,VoChn,VoChn,VPSS_PRE0_CHN));
#else
			CHECK_RET(HI_MPI_VO_SetChnAttr(VoDev, VoChn, &stVoChnAttr));
#endif
			//stVoZoomAttr.stZoomRect = stVoChnAttr.stRect;
			//stVoZoomAttr.enField = VIDEO_FIELD_INTERLACED;
			//CHECK_RET(HI_MPI_VO_SetZoomInWindow(VoDev, VoChn, &stVoZoomAttr));
	
#if defined(SN9234H1)
			//4.��VOͨ��
			if(-1 == ViDev)
			{			
				if(VochnInfo[index].VdecChn >= 0)
				{			
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "-----index: %d---SlaveId: %d, VoDev: %d----Bind---VochnInfo[index].VdecChn: %d, VoChn: %d\n",index, VochnInfo[index].SlaveId, VoDev, VochnInfo[index].VdecChn, VoChn);
					if(VochnInfo[index].SlaveId == PRV_MASTER )
					{
						CHECK_RET(HI_MPI_VDEC_BindOutput(VochnInfo[index].VdecChn, VoDev, VoChn)); 
					}				
					else if(VochnInfo[index].SlaveId > PRV_MASTER)
					{
						ViDev = PRV_HD_DEV;
						ViChn = 0;
					}
				}
			}
			
#if defined(SN_SLAVE_ON)
			if(ViDev == PRV_HD_DEV)
			{			
				VO_ZOOM_ATTR_S stVoZoomAttr;
				HI_U32 u32Width = 0, u32Height = 0;
#if defined(SN8604M) || defined(SN8608M) || defined(SN8608M_LE) || defined(SN8616M_LE)|| defined(SN9234H1)
				//u32Width = s_astVoDevStatDflt[s_VoDevCtrlDflt].stVideoLayerAttr.stImageSize.u32Width;
				//u32Height = s_astVoDevStatDflt[s_VoDevCtrlDflt].stVideoLayerAttr.stImageSize.u32Height;
				u32Width = PRV_BT1120_SIZE_W;
				u32Width = PRV_BT1120_SIZE_H;
#else
				u32Width = PRV_SINGLE_SCREEN_W;
				u32Height = PRV_SINGLE_SCREEN_H;
#endif				
				u32Width = s_astVoDevStatDflt[s_VoDevCtrlDflt].stVideoLayerAttr.stImageSize.u32Width;
				u32Height = s_astVoDevStatDflt[s_VoDevCtrlDflt].stVideoLayerAttr.stImageSize.u32Height;
				stSrcRect.s32X		= 0;
				stSrcRect.s32Y		= 0;
				stSrcRect.u32Width	= u32Width;
				stSrcRect.u32Height = u32Height;
				
				stDestRect = PRV_ReSetVoRect(OutPutMode, VochnInfo[index].VideoInfo.width, VochnInfo[index].VideoInfo.height, stSrcRect);
				
				stVoZoomAttr.stZoomRect.s32X		= stDestRect.s32X	   & (~0x01);
				stVoZoomAttr.stZoomRect.s32Y		= stDestRect.s32Y	   & (~0x01);
				stVoZoomAttr.stZoomRect.u32Width	= stDestRect.u32Width  & (~0x01);
				stVoZoomAttr.stZoomRect.u32Height	= stDestRect.u32Height & (~0x01);
	
				//stVoZoomAttr.enField = VIDEO_FIELD_INTERLACED;
				stVoZoomAttr.enField = VIDEO_FIELD_FRAME;
#if defined(Hi3535)
				CHECK_RET(HI_MPI_VO_SetZoomInWindow(PIP, VoChn, &stVoZoomAttr));
#else
				CHECK_RET(HI_MPI_VO_SetZoomInWindow(VoDev, VoChn, &stVoZoomAttr));
#endif
			}
#endif
			if(-1 != ViDev)
			{
				//printf("--------Bind ViDev: %d, ViChn: %d\n", ViDev, ViChn);
				CHECK_RET(HI_MPI_VI_BindOutput(ViDev, ViChn, VoDev, VoChn));
			}
	
#else
			//4.��VOͨ��
			if(VochnInfo[index].VdecChn >= 0)
			{			
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "-----index: %d---SlaveId: %d, VoDev: %d----Bind---VochnInfo[index].VdecChn: %d, VoChn: %d\n",index, VochnInfo[index].SlaveId, VoDev, VochnInfo[index].VdecChn, VoChn);
				if(VochnInfo[index].SlaveId == PRV_MASTER )
				{
					CHECK_RET(PRV_VDEC_BindVpss(VochnInfo[index].VdecChn, VoChn)); 
				}				
			}		
#endif
			VochnInfo[index].IsBindVdec[VoDev] = 1;
			//5.����VOͨ��
#if defined(Hi3535)
			CHECK_RET(HI_MPI_VO_EnableChn(PIP, VoChn));
#else		
			CHECK_RET(HI_MPI_VO_EnableChn(VoDev, VoChn));
#endif
			OldCtrlChn = stPlayInfo.ZoomChn;
			sem_post(&sem_SendNoVideoPic);		
		}
		else
			OldCtrlChn = -1;
		RET_SUCCESS("");
	}



/*************************************************
Function: //PRV_RefreshVoDevScreen
Description: �طŸ���VO״̬ˢ��VO�豸����ʾ
Calls: 
Called By: //
Input: //VoDev:�豸��
		Is_Double: �Ƿ�˫����ʾ
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 Playback_RefreshVoDevScreen(VO_DEV VoDev, HI_U32 Is_Double)
	{
		VO_DEV VoDev2 = VoDev;
		g_PlayInfo stPlayInfo;
#if defined(SN9234H1)
	sem_post(&sem_SendNoVideoPic);
	again:
		if(VoDev != HD)
#else
		Is_Double = DISP_NOT_DOUBLE_DISP;
	again:
		if(VoDev != DHD0)
#endif
		{
			RET_SUCCESS("");
		}	
      PRV_GetPlayInfo(&stPlayInfo);
  if(stPlayInfo.PlayBackState > PLAY_INSTANT)
  {
        if(stPlayInfo.IsZoom==1)
       	{
            Playback_VoDevSingle(VoDev, stPlayInfo.ZoomChn);
		}
        else if(stPlayInfo.IsSingle)
        {
            Playback_VoDevSingle(VoDev,stPlayInfo.ZoomChn);  
		}
		else if(stPlayInfo.bISDB)
		{
            Playback_VoDevSingle(VoDev,stPlayInfo.DBClickChn);
		}
		else
		{
            Playback_VoDevMul(VoDev,stPlayInfo.ImagCount);
		}
 /*2010-9-19 ˫����*/
		//printf("##########s_VoSecondDev = %d######################\n",s_VoSecondDev);
		if(Is_Double == DISP_DOUBLE_DISP)
		{
			if (VoDev2 == VoDev)
			{
#if defined(SN9234H1)
				switch(VoDev)
				{
					case HD:
						{
							VO_PUB_ATTR_S stVoPubAttr = s_astVoDevStatDflt[s_VoSecondDev].stVoPubAttr;
							VO_VIDEO_LAYER_ATTR_S stVideoLayerAttr = s_astVoDevStatDflt[s_VoSecondDev].stVideoLayerAttr;
	
							s_astVoDevStatDflt[s_VoSecondDev] = s_astVoDevStatDflt[HD];
							s_astVoDevStatDflt[s_VoSecondDev].stVoPubAttr = stVoPubAttr;
							s_astVoDevStatDflt[s_VoSecondDev].stVideoLayerAttr = stVideoLayerAttr;
	
							VoDev = s_VoSecondDev;
							goto again;
						}
						break;
					//case s_VoSecondDev:
					case AD:
						{
							VO_PUB_ATTR_S stVoPubAttr = s_astVoDevStatDflt[HD].stVoPubAttr;
							VO_VIDEO_LAYER_ATTR_S stVideoLayerAttr = s_astVoDevStatDflt[HD].stVideoLayerAttr;
							
							s_astVoDevStatDflt[HD] = s_astVoDevStatDflt[AD];
							s_astVoDevStatDflt[HD].stVoPubAttr = stVoPubAttr;
							s_astVoDevStatDflt[HD].stVideoLayerAttr = stVideoLayerAttr;
							
							VoDev = HD;
							goto again;
						}
						break;
					case SD:
						{
							VO_PUB_ATTR_S stVoPubAttr = s_astVoDevStatDflt[HD].stVoPubAttr;
							VO_VIDEO_LAYER_ATTR_S stVideoLayerAttr = s_astVoDevStatDflt[HD].stVideoLayerAttr;
							
							s_astVoDevStatDflt[HD] = s_astVoDevStatDflt[SD];
							s_astVoDevStatDflt[HD].stVoPubAttr = stVoPubAttr;
							s_astVoDevStatDflt[HD].stVideoLayerAttr = stVideoLayerAttr;
							
							VoDev = HD;
							goto again;
						}
						break;
					default:
						break;
				}
#else
				switch(VoDev)
				{
					case DHD0:
						{
							VO_PUB_ATTR_S stVoPubAttr = s_astVoDevStatDflt[s_VoSecondDev].stVoPubAttr;
							VO_VIDEO_LAYER_ATTR_S stVideoLayerAttr = s_astVoDevStatDflt[s_VoSecondDev].stVideoLayerAttr;
	
							s_astVoDevStatDflt[s_VoSecondDev] = s_astVoDevStatDflt[DHD0];
							s_astVoDevStatDflt[s_VoSecondDev].stVoPubAttr = stVoPubAttr;
							s_astVoDevStatDflt[s_VoSecondDev].stVideoLayerAttr = stVideoLayerAttr;
	
							VoDev = s_VoSecondDev;
							goto again;
						}
						break;
					//case s_VoSecondDev:
					case DHD1:
					case DSD0:
						{
							VO_PUB_ATTR_S stVoPubAttr = s_astVoDevStatDflt[DHD0].stVoPubAttr;
							VO_VIDEO_LAYER_ATTR_S stVideoLayerAttr = s_astVoDevStatDflt[DHD0].stVideoLayerAttr;
							
							s_astVoDevStatDflt[DHD0] = s_astVoDevStatDflt[DSD0];
							s_astVoDevStatDflt[DHD0].stVoPubAttr = stVoPubAttr;
							s_astVoDevStatDflt[DHD0].stVideoLayerAttr = stVideoLayerAttr;
							
							VoDev = DHD0;
							goto again;
						}
						break;
					default:
						break;
				}
#endif			
			}
		}

		//������Ƶ
		PRV_PlayAudio(VoDev);
  }
	RET_SUCCESS("");
}

STATIC HI_S32 Playback_ChnZoomIn(VO_CHN VoChn, HI_U32 u32Ratio, const Preview_Point *pstPoint)
	{
       g_PlayInfo stPlayInfo;
	   PRV_GetPlayInfo(&stPlayInfo);
#if defined(SN9234H1)
		VoChn = stPlayInfo.ZoomChn;//��ʱ���Բ���VoChn
		int index = 0;
		index = VoChn;
		if(index < 0)
			RET_FAILURE("------ERR: Invalid Index!");
		if(VoChn < 0 || VoChn >= g_Max_Vo_Num
			|| u32Ratio < PRV_MIN_ZOOM_IN_RATIO || u32Ratio > PRV_MAX_ZOOM_IN_RATIO)
		{
			RET_FAILURE("Invalid Parameter: u32Ratio or VoChn");
		}
		
		if(NULL == pstPoint)
		{
			RET_FAILURE("NULL Pointer!!");
		}
		
		if (stPlayInfo.IsZoom==0)
		{
			RET_FAILURE("NOT in [zoom in ctrl] stat!!!");
		}
		
		
		//printf("vochn=%d, u32ratio=%d, pstPoint->x= %d, pstPoint->y=%d===========================\n",VoChn,u32Ratio,pstPoint->x, pstPoint->y);
	
	//Mϵ�в�Ʒ֧�ָ���(1080P/720P)�������ʾ��������ͨ������Ƭ���ź�Դ�ֱ���̫�󣬽������ź�
	//Ҳ����VO_ZOOM_ATTR_S����ķ�Χ������HI_MPI_VO_SetZoomInWindow�᷵��ʧ�ܣ�
	//����ڴ�Ƭ����Ϊ�ź�Դ�Ѿ������������������ţ�����Ƭ����ʾʱ����VO_ZOOM_ATTR_S��Χ��
	//������֧�ָ�����ͺţ�������Ƭ��ʾ����ʱ����HI_MPI_VO_SetZoomInRatio���е��ӷŴ�
	
	//����Ƭ����ʾ�ĸ���(����D1)ͨ������HI_MPI_VO_SetZoomInRatio���е��ӷŴ�
		if(PRV_MASTER == VochnInfo[index].SlaveId
			&& VochnInfo[index].VdecChn >= 0 )		
		{
			VO_ZOOM_RATIO_S stZoomRatio;
	
			if (u32Ratio <= 1)
			{
				stZoomRatio.u32XRatio = 0;
				stZoomRatio.u32YRatio = 0;
				stZoomRatio.u32WRatio = 0;
				stZoomRatio.u32HRatio = 0;
			}
			else
			{
#if 0
				stZoomRatio.u32WRatio = 1000/u32Ratio;
				stZoomRatio.u32HRatio = 1000/u32Ratio;
				stZoomRatio.u32XRatio = ((pstPoint->x * 1000)/s_u32GuiWidthDflt + stZoomRatio.u32WRatio > 1000)
					? 1000 - stZoomRatio.u32WRatio
					: (pstPoint->x * 1000)/s_u32GuiWidthDflt;
				stZoomRatio.u32YRatio = ((pstPoint->y * 1000)/s_u32GuiHeightDflt + stZoomRatio.u32HRatio > 1000)
					? 1000 - stZoomRatio.u32HRatio
					: (pstPoint->y * 1000)/s_u32GuiHeightDflt;
	
#else /*��1��16���Ŵ�תΪ1��4���Ŵ�y = (x - 1)/5 + 1*/
	
				u32Ratio += 4;
					
				stZoomRatio.u32WRatio = 5000/u32Ratio;
				stZoomRatio.u32HRatio = 5000/u32Ratio;
				stZoomRatio.u32XRatio = ((pstPoint->x * 1000)/s_u32GuiWidthDflt + stZoomRatio.u32WRatio > 1000)
					? 1000 - stZoomRatio.u32WRatio
					: (pstPoint->x * 1000)/s_u32GuiWidthDflt;
				stZoomRatio.u32YRatio = ((pstPoint->y * 1000)/s_u32GuiHeightDflt + stZoomRatio.u32HRatio > 1000)
					? 1000 - stZoomRatio.u32HRatio
					: (pstPoint->y * 1000)/s_u32GuiHeightDflt;
#endif
			}
	//	printf("==================stZoomRatio.u32XRatio = %d; stZoomRatio.u32YRatio = %d; stZoomRatio.u32WRatio =%d; stZoomRatio.u32HRatio = %d;\n",	
	//		stZoomRatio.u32XRatio ,stZoomRatio.u32YRatio,stZoomRatio.u32WRatio,stZoomRatio.u32HRatio);
#if 0 /*2010-8-31 �Ż������ӷŴ����ķŴ�ʽ���зŴ�*/
			stZoomRatio.u32XRatio = (stZoomRatio.u32XRatio < stZoomRatio.u32WRatio/2)?0:stZoomRatio.u32XRatio - stZoomRatio.u32WRatio/2;
			stZoomRatio.u32YRatio = (stZoomRatio.u32YRatio < stZoomRatio.u32HRatio/2)?0:stZoomRatio.u32YRatio - stZoomRatio.u32HRatio/2;
#endif
#if 1
			CHECK_RET(HI_MPI_VO_SetZoomInRatio(s_VoDevCtrlDflt, VoChn, &stZoomRatio));
#else /*2010-9-19 ˫����*/
			if(s_State_Info.g_zoom_first_in == HI_FALSE)
			{
				//CHECK_RET(HI_MPI_VO_GetZoomInRatio(HD,VoChn,&s_astZoomAttrDflt[HD]));
				CHECK_RET(HI_MPI_VO_GetZoomInRatio(s_VoDevCtrlDflt,VoChn,&s_astZoomAttrDflt[s_VoDevCtrlDflt]));
				s_State_Info.g_zoom_first_in = HI_TRUE;
			}
			//CHECK_RET(HI_MPI_VO_SetZoomInRatio(HD, VoChn, &stZoomRatio));
			CHECK_RET(HI_MPI_VO_SetZoomInRatio(s_VoDevCtrlDflt, VoChn, &stZoomRatio));
#endif
		}
		else
		//Dϵ�еĲ�Ʒ��֧�ָ��壬���֧��D1(704*576),�����ô��ַ������ӷŴ�
		//Mϵ���ڴ�Ƭ��(SN8616M_LE)��ʾ��ͨ���Լ���Ƭ��ʾ��D1ͨ�����ô��ַ���
		{
			VO_ZOOM_ATTR_S stVoZoomAttr;
			int w = 0, h = 0, x = 0, y = 0;
			HI_U32 u32Width = 0, u32Height = 0;
#if defined(SN8604M) || defined(SN8608M) || defined(SN8608M_LE) || defined(SN8616M_LE)|| defined(SN9234H1)
			u32Width = PRV_BT1120_SIZE_W;
			u32Height = PRV_BT1120_SIZE_H;
#else
			u32Width = PRV_SINGLE_SCREEN_W;
			u32Height = PRV_SINGLE_SCREEN_H;
#endif

			u32Width = s_astVoDevStatDflt[s_VoDevCtrlDflt].stVideoLayerAttr.stImageSize.u32Width;
			u32Height = s_astVoDevStatDflt[s_VoDevCtrlDflt].stVideoLayerAttr.stImageSize.u32Height;
			x = u32Width * pstPoint->x / s_u32GuiWidthDflt; //��ӦD1��Ļ����X
			y = u32Height * pstPoint->y / s_u32GuiHeightDflt; //��ӦD1��Ļ����Y
			w = u32Width * 5/(u32Ratio+4);					//�Ŵ���ο���
			h = u32Height * 5/(u32Ratio+4); 				//�Ŵ���ο�߶�
			stVoZoomAttr.stZoomRect.s32X		= (((x + w) > u32Width) ? (u32Width -w) : x) & (~0x01);;	//����xλ�ã�����D1���Ҫ�˲�,2���ض���
			stVoZoomAttr.stZoomRect.s32Y		= (((y + h) > u32Height) ? (u32Height -h) : y) & (~0x01);		//����yλ�ã�����D1�߶�Ҫ�˲�,2���ض���
			stVoZoomAttr.stZoomRect.u32Width	= w & (~0x01);
			stVoZoomAttr.stZoomRect.u32Height	= h & (~0x01);
            stVoZoomAttr.enField = VIDEO_FIELD_FRAME;	
       
			CHECK_RET(HI_MPI_VO_SetZoomInWindow(s_VoDevCtrlDflt, VoChn, &stVoZoomAttr));
	
		}
		
#else	
	
	
		//VoChn = s_astVoDevStatDflt[s_VoDevCtrlDflt].s32CtrlChn;//��ʱ���Բ���VoChn
		int index = 0;
		VoChn = stPlayInfo.ZoomChn;
		//index = PRV_GetVoChnIndex(VoChn);
		index = stPlayInfo.ZoomChn;
		if(index < 0)
			RET_FAILURE("------ERR: Invalid Index!");
		if(VoChn < 0 || VoChn >= PRV_VO_CHN_NUM
			|| u32Ratio < PRV_MIN_ZOOM_IN_RATIO || u32Ratio > PRV_MAX_ZOOM_IN_RATIO)
		{
			RET_FAILURE("Invalid Parameter: u32Ratio or VoChn");
		}
		
		if(NULL == pstPoint)
		{
			RET_FAILURE("NULL Pointer!!");
		}
		
		if (stPlayInfo.IsZoom==0)
		{
			RET_FAILURE("NOT in [zoom in ctrl] stat!!!");
		}
	//	if (VoChn != s_astVoDevStatDflt[s_VoDevCtrlDflt].s32CtrlChn)
		{
	//		RET_FAILURE("VoChn NOT match current VoChn!!!");
		}
		//printf("vochn=%d, u32ratio=%f, pstPoint->x= %d, pstPoint->y=%d===========================\n",VoChn,u32Ratio,pstPoint->x, pstPoint->y);
	
		VoChn = stPlayInfo.ZoomChn;
		VPSS_GRP VpssGrp = VoChn;
		VPSS_CROP_INFO_S stVpssCropInfo;
		HI_S32 w = 0, h = 0, x = 0, y = 0, s32X = 0, s32Y = 0;
		HI_U32 u32Width = 0, u32Height = 0, u32W = 0, u32H = 0;
		u32Width = s_astVoDevStatDflt[s_VoDevCtrlDflt].stVideoLayerAttr.stImageSize.u32Width;
		u32Height = s_astVoDevStatDflt[s_VoDevCtrlDflt].stVideoLayerAttr.stImageSize.u32Height;
		w = u32Width/sqrt(u32Ratio);
		h = u32Height/sqrt(u32Ratio);
		stVpssCropInfo.bEnable = HI_TRUE;
	
		VO_CHN_ATTR_S stVoChnAttr;	
		CHECK_RET(HI_MPI_VO_GetChnAttr(s_VoDevCtrlDflt, VoChn, &stVoChnAttr));
		s32X = stVoChnAttr.stRect.s32X;
		s32Y = stVoChnAttr.stRect.s32Y;
		u32W = stVoChnAttr.stRect.u32Width;
		u32H = stVoChnAttr.stRect.u32Height;
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "VoChn: %d, s32X=%d, s32Y=%d, u32W=%d, u32H=%d\n", VoChn, s32X, s32Y, u32W, u32H);
		
	#if 1
		//x = u32Width * pstPoint->x / s_u32GuiWidthDflt; //��ӦD1��Ļ����X
		//y = u32Height * pstPoint->y / s_u32GuiHeightDflt; //��ӦD1��Ļ����Y
		//w = u32Width * 5/(u32Ratio+4);					//�Ŵ���ο���
		//h = u32Height * 5/(u32Ratio+4);				//�Ŵ���ο�߶�
		//x = pstPoint->x;
		//y = pstPoint->y;
		//w = u32Width/sqrt(u32Ratio);					//�Ŵ���ο���
		//h = u32Height/sqrt(u32Ratio); 				//�Ŵ���ο�߶�
		//����ģʽ�µ��ӷŴ�
		if(u32H == u32Height && u32W != u32Width)
		{
			if(pstPoint->x < s32X)
			{
				if(pstPoint->x + w <= s32X)
				{
					CHECK(PRV_VPSS_Stop(VpssGrp));
					CHECK(PRV_VPSS_Start(VpssGrp));
					return 0;
				}
				else if(pstPoint->x + w <= s32X + u32W)
				{
					x = 0;
					y = pstPoint->y * VochnInfo[index].VideoInfo.height/u32Height;
					w = (w - (s32X - pstPoint->x)) * VochnInfo[index].VideoInfo.width/u32W;
					h = VochnInfo[index].VideoInfo.height/sqrt(u32Ratio);
				}
				else
				{
					x = 0;
					y = pstPoint->y * VochnInfo[index].VideoInfo.height/u32Height;
					w = u32W * VochnInfo[index].VideoInfo.width/u32W;
					h = VochnInfo[index].VideoInfo.height/sqrt(u32Ratio);
				}
			}
			else if(pstPoint->x >= s32X && pstPoint->x <= s32X + u32W)
			{
				if(pstPoint->x + w <= s32X + u32W)
				{
					x = (pstPoint->x - s32X) * VochnInfo[index].VideoInfo.width/u32W;
					y = pstPoint->y * VochnInfo[index].VideoInfo.height/u32Height;
					w = VochnInfo[index].VideoInfo.width * w/u32W;
					h = VochnInfo[index].VideoInfo.height/sqrt(u32Ratio);
	
				}
				else
				{
					x = (pstPoint->x - s32X) * VochnInfo[index].VideoInfo.width/u32W;
					y = pstPoint->y * VochnInfo[index].VideoInfo.height/u32Height;
					w = (u32W - (pstPoint->x - s32X)) * VochnInfo[index].VideoInfo.width/u32W;
					h = VochnInfo[index].VideoInfo.height/sqrt(u32Ratio);
				}
			}
			else
			{
				CHECK(PRV_VPSS_Stop(VpssGrp));
				CHECK(PRV_VPSS_Start(VpssGrp));
				return 0;
			}
			
		}
		else if(u32W == u32Width && u32H != u32Height)
		{
			if(pstPoint->y < s32Y)
			{
				if(pstPoint->y + h <= s32Y)
				{
					CHECK(PRV_VPSS_Stop(VpssGrp));
					CHECK(PRV_VPSS_Start(VpssGrp));
					return 0;
				}
				else if(pstPoint->y + h <= s32Y + u32H)
				{
					x = pstPoint->x * VochnInfo[index].VideoInfo.width/u32Width;
					y = 0;
					w = VochnInfo[index].VideoInfo.width/sqrt(u32Ratio);
					h = (h - (s32Y - pstPoint->y)) * VochnInfo[index].VideoInfo.height/u32Height;
				}
				else
				{
					x = pstPoint->x * 1000 /u32Width;
					y = 0;
					w = VochnInfo[index].VideoInfo.width/sqrt(u32Ratio);
					h = u32H * VochnInfo[index].VideoInfo.height/u32H;
				}
	
			}
			else if(pstPoint->y >= s32Y && pstPoint->y <= s32Y + u32H)
			{
				if(pstPoint->y + h <= s32Y + u32H)
				{
					x = pstPoint->x * VochnInfo[index].VideoInfo.width/u32Width;
					y = (pstPoint->y - s32Y) * VochnInfo[index].VideoInfo.height/u32H;
					w = VochnInfo[index].VideoInfo.width * h/u32H;
					h = VochnInfo[index].VideoInfo.height/sqrt(u32Ratio);
				}
				else
				{
					x = pstPoint->x * VochnInfo[index].VideoInfo.width/u32Width;
					y = (pstPoint->y - s32Y) * VochnInfo[index].VideoInfo.height/u32H;
					w = VochnInfo[index].VideoInfo.width/sqrt(u32Ratio);
					h = (u32H - (pstPoint->y - s32Y)) * VochnInfo[index].VideoInfo.height/u32H;
				}
			}
			else
			{
				CHECK(PRV_VPSS_Stop(VpssGrp));
				CHECK(PRV_VPSS_Start(VpssGrp));
				return 0;
			}
		}
		else
		{
			x = pstPoint->x * VochnInfo[index].VideoInfo.width/u32Width;
			y = pstPoint->y * VochnInfo[index].VideoInfo.height/u32Height;
			w = VochnInfo[index].VideoInfo.width/sqrt(u32Ratio);
			h = VochnInfo[index].VideoInfo.height/sqrt(u32Ratio);
		}
		x = ((x + w) > VochnInfo[index].VideoInfo.width) ? (VochnInfo[index].VideoInfo.width - w) : x;
		y = ((y + h) > VochnInfo[index].VideoInfo.height) ? (VochnInfo[index].VideoInfo.height - h) : y;
		//printf("pstPoint->x: %d, pstPoint->y: %d\n", pstPoint->x, pstPoint->y);
		x = ((x + w) > VochnInfo[index].VideoInfo.width) ? (VochnInfo[index].VideoInfo.width - w) : x;
		y = ((y + h) > VochnInfo[index].VideoInfo.height) ? (VochnInfo[index].VideoInfo.height - h) : y;
		w = w >= 32 ? w : 32;
		h = h >= 32 ? h : 32;
		x = ALIGN_BACK(x, 4);//��ʼ��Ϊ4�������������Ϊ16��������
		y = ALIGN_BACK(y, 4);
		w = ALIGN_BACK(w, 16);
		h = ALIGN_BACK(h, 16);
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "11111111u32Ratio: %u, x: %d, y: %d, w: %d, h: %d===width: %d, height: %d\n", u32Ratio, x, y, w, h, VochnInfo[index].VideoInfo.width, VochnInfo[index].VideoInfo.height);
		stVpssCropInfo.enCropCoordinate = VPSS_CROP_ABS_COOR;
		stVpssCropInfo.stCropRect.s32X = x;
		stVpssCropInfo.stCropRect.s32Y = y;
		stVpssCropInfo.stCropRect.u32Width = w;
		stVpssCropInfo.stCropRect.u32Height = h;
	#else
	#if 0
		if(u32H == u32Height && u32W != u32Width)
		{
			if(pstPoint->x < s32X)
			{
				if(pstPoint->x + w <= s32X)
				{
					CHECK(PRV_VPSS_Stop(VpssGrp));
					CHECK(PRV_VPSS_Start(VpssGrp));
					return 0;
				}
				else if(pstPoint->x + w <= s32X + u32W)
				{
					x = s32X * u32W/u32Width;
					y = pstPoint->y;
					w = w - (s32X - pstPoint->x);
				}
				else
				{
					x = s32X;
					y = pstPoint->y;
					w = u32W;
				}
			}
			else if(pstPoint->x >= s32X && pstPoint->x <= s32X + u32W)
			{
				if(pstPoint->x + w <= s32X + u32W)
				{
					x = (pstPoint->x - s32X);
					y = pstPoint->y;
					w = u32W/sqrt(u32Ratio);
					h = u32H/sqrt(u32Ratio);
				}
				else
				{
					x = (pstPoint->x - s32X);
					y = pstPoint->y;
					w = u32W - (pstPoint->x - s32X);
				}
			}
			else
			{
				CHECK(PRV_VPSS_Stop(VpssGrp));
				CHECK(PRV_VPSS_Start(VpssGrp));
				return 0;
			}
		}
		else if(u32W == u32Width && u32H != u32Height)
		{
			if(pstPoint->y < s32Y)
			{
				if(pstPoint->y + h <= s32Y)
				{
					CHECK(PRV_VPSS_Stop(VpssGrp));
					CHECK(PRV_VPSS_Start(VpssGrp));
					return 0;
				}
				else if(pstPoint->y + h <= s32Y + u32H)
				{
					y = s32Y * 1000/u32Height;
					x = pstPoint->x * 1000 /u32Width;
					h = h - (s32Y - pstPoint->y);
				}
				else
				{
					y = s32Y * 1000/u32Height;
					x = pstPoint->x * 1000 /u32Width;
					h = u32H;
				}
	
			}
			else if(pstPoint->y >= s32Y && pstPoint->y <= s32Y + u32H)
			{
				if(pstPoint->y + h <= s32Y + u32H)
				{
					x = pstPoint->x * 1000/u32Width;
					y = pstPoint->y * 1000 /u32Height;
					w = u32W/sqrt(u32Ratio);
					h = u32H/sqrt(u32Ratio);
				}
				else
				{
					x = pstPoint->y * 1000/u32Width;
					y = pstPoint->y * 1000 /u32Height;
					h = u32H - (pstPoint->y - s32Y);
				}
	
			}
			else
			{
				CHECK(PRV_VPSS_Stop(VpssGrp));
				CHECK(PRV_VPSS_Start(VpssGrp));
				return 0;
			}
		}
		else
	#endif
		{
			x = pstPoint->y * 1000 /u32Width;
			y = pstPoint->y * 1000 /u32Height;
			w = u32Width/sqrt(u32Ratio);
			h = u32Height/sqrt(u32Ratio);
		}
		//printf("11111111u32Ratio: %f, x: %d, y: %d, w: %d, h: %d\n", u32Ratio, x, y, w, h);
		
		x = x > 999 ? 999 : x;
		y = y > 999 ? 999 : y;
		x = ALIGN_BACK(x, 4);//��ʼ��Ϊ4�������������Ϊ16��������
		y = ALIGN_BACK(y, 4);
		w = ALIGN_BACK(w, 16);
		h = ALIGN_BACK(h, 16);
		stVpssCropInfo.enCropCoordinate = VPSS_CROP_RITIO_COOR;
		stVpssCropInfo.stCropRect.s32X = x;
		stVpssCropInfo.stCropRect.s32Y = y;
		stVpssCropInfo.stCropRect.u32Width = w;
		stVpssCropInfo.stCropRect.u32Height = h;
	#endif
#if defined(Hi3535)
		CHECK_RET(HI_MPI_VPSS_SetGrpCrop(VpssGrp, &stVpssCropInfo));
		CHECK_RET(HI_MPI_VO_RefreshChn(VHD0, VoChn));
#else
		stVpssCropInfo.enCapSel = VPSS_CAPSEL_BOTH;
		CHECK_RET(HI_MPI_VPSS_SetCropCfg(VpssGrp, &stVpssCropInfo));
		CHECK_RET(HI_MPI_VO_ChnRefresh(0, VoChn));
#endif
#endif	
		RET_SUCCESS("Chn Zoom in!");
	}


/*************************************************
Function: //PRV_ExitChnCtrl
Description://�˳�ͨ������״̬
Calls: 
Called By: //
Input: // s32Flag: ͨ��״̬
Output: // ����ϵͳ����Ĵ�����
Return: //��ϸ�ο��ĵ��Ĵ�����
Others: // ����˵��
************************************************************************/
STATIC HI_S32 Playback_ExitZoomChn(SN_MSG *msg_req)
{
        ZoomRsp Rsp;
		Rsp.result=0;
#if defined(SN9234H1)
     PRV_MccPBCtlReq Mcc_req;
#endif
	//	unsigned int is_double=DISP_DOUBLE_DISP;
        g_PlayInfo stPlayInfo;
	    PRV_GetPlayInfo(&stPlayInfo);
		if (stPlayInfo.IsZoom==0)
		{
			RET_FAILURE("NOT in ctrl or pb stat or pic stat!!!");
		}
		#if defined(SN9234H1)
				VO_ZOOM_RATIO_S stZoomRatio = {0};
#if defined(SN6108) || defined(SN8608D) || defined(SN8608M) || defined(SN6104) || defined(SN8604D) || defined(SN8604M)
				HI_MPI_VO_SetZoomInRatio(s_VoDevCtrlDflt, s_astVoDevStatDflt[s_VoDevCtrlDflt].s32CtrlChn, &stZoomRatio);
#else
				HI_MPI_VO_SetZoomInRatio(HD, stPlayInfo.ZoomChn, &stZoomRatio);
				//HI_MPI_VO_SetZoomInRatio(AD, s_astVoDevStatDflt[s_VoDevCtrlDflt].s32CtrlChn, &stZoomRatio);
#endif
				s_State_Info.g_zoom_first_in = HI_FALSE;
#else
				int index = stPlayInfo.ZoomChn;
				VPSS_GRP VpssGrp = VochnInfo[index].VoChn;
				//����VPSS GROP����������ӷŴ�ʱ���õ�����
				
#if defined(Hi3535)
				CHECK(HI_MPI_VO_HideChn(s_VoDevCtrlDflt, VochnInfo[index].VoChn));
				CHECK(PRV_VDEC_UnBindVpss(VochnInfo[index].VdecChn, VpssGrp));
				CHECK(PRV_VO_UnBindVpss(s_VoDevCtrlDflt, VochnInfo[index].VoChn, VpssGrp, VPSS_BSTR_CHN));
#else
				CHECK(HI_MPI_VO_ChnHide(s_VoDevCtrlDflt, VochnInfo[index].VoChn));
				CHECK(PRV_VDEC_UnBindVpss(VochnInfo[index].VdecChn, VpssGrp));
				CHECK(PRV_VO_UnBindVpss(s_VoDevCtrlDflt, VochnInfo[index].VoChn, VpssGrp, VPSS_PRE0_CHN));
#endif
				CHECK(PRV_VPSS_Stop(VpssGrp));
				CHECK(PRV_VPSS_Start(VpssGrp));
#if defined(Hi3535)
				CHECK(PRV_VO_BindVpss(s_VoDevCtrlDflt, VochnInfo[index].VoChn, VpssGrp, VPSS_BSTR_CHN));
#else
				CHECK(PRV_VO_BindVpss(s_VoDevCtrlDflt, VochnInfo[index].VoChn, VpssGrp, VPSS_PRE0_CHN));
#endif
                PRV_VPSS_ResetWH(VpssGrp,VochnInfo[index].VdecChn,VochnInfo[index].VideoInfo.width,VochnInfo[index].VideoInfo.height);
                CHECK(PRV_VDEC_BindVpss(VochnInfo[index].VdecChn, VpssGrp));
				s_State_Info.g_zoom_first_in = HI_FALSE;
#endif				
			Playback_ZoomInPic(HI_FALSE);
		   stPlayInfo.IsZoom= 0;
		   PRV_SetPlayInfo(&stPlayInfo);
#if defined(SN9234H1)
       if(stPlayInfo.IsSingle)
		   Mcc_req.Single=1;
	   else
	       Mcc_req.Single=stPlayInfo.bISDB;
	   Mcc_req.FullScreenId=stPlayInfo.FullScreenId;
	   if(stPlayInfo.IsSingle)
		   Mcc_req.VoChn=0;
	   else
	       Mcc_req.VoChn=stPlayInfo.DBClickChn;
	   Mcc_req.ImagCount=stPlayInfo.ImagCount;
	   Mcc_req.flag=2;
	   SN_SendMccMessageEx(PRV_SLAVE_1, SUPER_USER_ID, MOD_PRV, MOD_PRV, msg_req->xid, msg_req->thread, MSG_ID_PRV_MCC_PBZOOM_REQ, &Mcc_req, sizeof(PRV_MccPBCtlReq));
#endif
#if !defined(SN9234H1)
       Playback_RefreshVoDevScreen(s_VoDevCtrlDflt,DISP_DOUBLE_DISP);
#endif
	   SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_MMI, msg_req->xid, msg_req->thread, 
							MSG_ID_PRV_EXIT_ZOOMIN_RSP,&Rsp,sizeof(ZoomRsp));
	   RET_SUCCESS("Enter Chn Ctrl!");
	
}



STATIC HI_S32 Playback_MSG_ChnZoomIn(const SN_MSG *msg_req)
{
	Chn_zoom_in_Req *param = (Chn_zoom_in_Req *)msg_req->para;
	
	g_PlayInfo stPlayInfo;
	PRV_GetPlayInfo(&stPlayInfo);
	
	param->chn=stPlayInfo.ZoomChn;
	
	if (NULL == param )
	{
		RET_FAILURE("NULL pointer!!!");
	}
	param->chn=stPlayInfo.ZoomChn;
	//CHECK_RET(PRV_ChnZoomIn(param->dev,param->chn, param->ratio, &param->point));
	CHECK_RET(Playback_ChnZoomIn(param->chn, param->ratio, &param->point));

	RET_SUCCESS("");
}
HI_S32 Playback_ZoomChn(SN_MSG *msg_req)
{
#if defined(SN9234H1)
      PRV_MccPBCtlReq Mcc_req;
#endif
	  Preview_Point stPoint;
	  g_PlayInfo stPlayInfo;
	  PlayBack_Set_ZoomInChn_Req *Req;
	  PlayBack_Set_ZoomInChn_Rsp Rsp;
	 // unsigned int is_double = DISP_DOUBLE_DISP;
	   PRV_GetPlayInfo(&stPlayInfo);
	  Req=(PlayBack_Set_ZoomInChn_Req *)msg_req->para;
	  stPoint.x=Req->x;
	  stPoint.y=Req->y;
	  stPlayInfo.ZoomChn = PRV_GetVoChn_ForPB(stPoint);
	  if(stPlayInfo.IsSingle)
	  	   stPlayInfo.ZoomChn=0;
      PRV_SetPlayInfo(&stPlayInfo);
#if defined(SN9234H1)
	     Mcc_req.flag=1;
		 Mcc_req.Single=1;
		 Mcc_req.FullScreenId=1;
		 Mcc_req.VoChn=stPlayInfo.ZoomChn;
		 Mcc_req.ImagCount=stPlayInfo.ImagCount;
		 SN_SendMccMessageEx(PRV_SLAVE_1, SUPER_USER_ID, MOD_PRV, MOD_PRV, msg_req->xid, msg_req->thread, MSG_ID_PRV_MCC_PBZOOM_REQ, &Mcc_req, sizeof(PRV_MccPBCtlReq));
#endif
#if !defined(SN9234H1)
	  Playback_RefreshVoDevScreen(s_VoDevCtrlDflt, DISP_DOUBLE_DISP);
	  Playback_ZoomInPic(HI_TRUE);
	  pthread_mutex_unlock(&send_data_mutex);
	  sem_post(&sem_PBGetData);
#endif
	  Rsp.result=0;
	 SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_MMI, msg_req->xid, msg_req->thread, 
							MSG_ID_PRV_SET_ZOOMINCHN_RSP,&Rsp,sizeof(PlayBack_Set_ZoomInChn_Rsp));
	 return HI_SUCCESS;
}
#if defined(SN9234H1)
HI_S32 Playback_MCC_ZoomRSP(SN_MSG *msg_rsp)
{
	 PRV_MccPBCtlRsp *Mcc_rsp=(PRV_MccPBCtlRsp *)msg_rsp->para;
	 g_PlayInfo stPlayInfo;
	 PRV_GetPlayInfo(&stPlayInfo);
	 switch(Mcc_rsp->flag)
	 {
        case 0:
			 if(stPlayInfo.IsSingle)
	         {
                Playback_VoDevSingle(s_VoDevCtrlDflt, 0);
	         }
	         else if(stPlayInfo.bISDB)
	         {
                Playback_VoDevSingle(s_VoDevCtrlDflt, stPlayInfo.DBClickChn);
	         }
	         else Playback_VoDevMul(s_VoDevCtrlDflt, stPlayInfo.ImagCount);
			 break;
		case 1:
			 Playback_RefreshVoDevScreen(s_VoDevCtrlDflt, DISP_DOUBLE_DISP);
	         Playback_ZoomInPic(HI_TRUE);
	         pthread_mutex_unlock(&send_data_mutex);
	         sem_post(&sem_PBGetData);
			break;
		case 2:
			Playback_RefreshVoDevScreen(s_VoDevCtrlDflt,DISP_DOUBLE_DISP);
			break;
		default:
			break;
	 }
	 return HI_SUCCESS;
}

HI_S32 Playback_MCC_DBRSP(SN_MSG *msg_rsp)
{
    HI_S32 s32Ret = 0;
	g_PlayInfo stPlayInfo;
	PRV_GetPlayInfo(&stPlayInfo);
    if(stPlayInfo.bISDB==0)
    {
          s32Ret = Playback_VoDevMul(s_VoDevCtrlDflt,stPlayInfo.ImagCount);
		  if (s32Ret != HI_SUCCESS)
	      {
		      return HI_FAILURE;
	      }
	}
	else
	{
          s32Ret = Playback_VoDevSingle(s_VoDevCtrlDflt,stPlayInfo.DBClickChn);
		  if (s32Ret != HI_SUCCESS)
	      {
		      return HI_FAILURE;
	      }
	}
	return HI_SUCCESS;

}
HI_S32 Playback_MCC_FullScrRSP(SN_MSG *msg_rsp)
{
    g_PlayInfo stPlayInfo;
	HI_S32 s32Ret = 0;
	PRV_GetPlayInfo(&stPlayInfo);
	if(stPlayInfo.IsSingle == 1)
    {
           s32Ret = Playback_VoDevSingle(s_VoDevCtrlDflt,stPlayInfo.DBClickChn);
		   if (s32Ret != HI_SUCCESS)
		   {
			   return HI_FAILURE;
		   }
    }
	else if(stPlayInfo.bISDB==0)
	{
            s32Ret = Playback_VoDevMul(s_VoDevCtrlDflt,stPlayInfo.ImagCount);
			 if (s32Ret != HI_SUCCESS)
			 {
				 return HI_FAILURE;
			 }
	}
	else
	{
            s32Ret = Playback_VoDevSingle(s_VoDevCtrlDflt,stPlayInfo.DBClickChn);
			 if (s32Ret != HI_SUCCESS)
			 {
				 return HI_FAILURE;
			 }
	}
	return HI_SUCCESS;

}
#endif

HI_S32 Playback_ZoomEnter(SN_MSG *msg_req)
{
     g_PlayInfo stPlayInfo;
     #if defined(SN9234H1)
    PRV_MccPBCtlReq Mcc_req;
     #endif
	 ZoomRsp Rsp;
     PRV_GetPlayInfo(&stPlayInfo);
     stPlayInfo.IsZoom=1;
	 
	 PRV_SetPlayInfo(&stPlayInfo);
#if !defined(SN9234H1)
	 if(stPlayInfo.IsSingle)
	 {
          Playback_VoDevSingle(s_VoDevCtrlDflt, 0);
	 }
	 else if(stPlayInfo.bISDB)
	 {
          Playback_VoDevSingle(s_VoDevCtrlDflt, stPlayInfo.DBClickChn);
	 }
	 else Playback_VoDevMul(s_VoDevCtrlDflt, stPlayInfo.ImagCount);
#endif	 
	
#if defined(SN9234H1)
		 if(stPlayInfo.IsSingle)
			 Mcc_req.Single=1;
		 else
			Mcc_req.Single=stPlayInfo.bISDB;
		 Mcc_req.FullScreenId=1;
		 if(stPlayInfo.IsSingle)
			 Mcc_req.VoChn=0;
		 else
			Mcc_req.VoChn=stPlayInfo.DBClickChn;
		 Mcc_req.ImagCount=stPlayInfo.ImagCount;
		 Mcc_req.flag=0;
		 SN_SendMccMessageEx(PRV_SLAVE_1, SUPER_USER_ID, MOD_PRV, MOD_PRV, msg_req->xid, msg_req->thread, MSG_ID_PRV_MCC_PBZOOM_REQ, &Mcc_req, sizeof(PRV_MccPBCtlReq));
#endif
	 Rsp.result=0;
	 SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_MMI, msg_req->xid, msg_req->thread, 
								 MSG_ID_PRV_ENTER_ZOOMIN_RSP,&Rsp,sizeof(ZoomRsp));

	 return HI_SUCCESS;
	 
}

HI_S32 Playback_FullScr(SN_MSG *msg_req)
{
#if defined(SN9234H1)
    PRV_MccPBCtlReq Mcc_req;
#endif
	PlaybackFullScreenReq *FullScreenreq=(PlaybackFullScreenReq *)msg_req->para;
	PlaybackFullScreenRsp Rsp;
	HI_S32 s32Ret = 0, GuiVoDev = 0, s32ChnCount = 0;
	HI_U32 u32Width = 0, u32Height = 0;
	g_PlayInfo stPlayInfo;
	VO_VIDEO_LAYER_ATTR_S pstLayerAttr;
	UINT8 IsSingle;
	UINT8 IsDB;
	UINT8 DB_Vo;
    PRV_GetPlayInfo(&stPlayInfo);
	IsSingle=stPlayInfo.IsSingle;
	IsDB=stPlayInfo.bISDB;
	DB_Vo=stPlayInfo.DBClickChn;

    PRV_GetGuiVo(&GuiVoDev);
    s32Ret = HI_MPI_VO_GetVideoLayerAttr(GuiVoDev, &pstLayerAttr);
	s32ChnCount = stPlayInfo.ImagCount;
	u32Width = pstLayerAttr.stImageSize.u32Width;
	u32Height = pstLayerAttr.stImageSize.u32Height;
	
	if(FullScreenreq->FullScreenId==1)
	{
            stPlayInfo.FullScreenId=1;
			Rsp.FullScreenId=1;
			SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_MMI, msg_req->xid, msg_req->thread, 
							MSG_ID_PRV_FULLSCREEN_RSP,&Rsp,sizeof(PlaybackFullScreenRsp));
			
	}
	else
	{
            stPlayInfo.FullScreenId=0;
			Rsp.FullScreenId=0;
			SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_MMI, msg_req->xid, msg_req->thread, 
							MSG_ID_PRV_FULLSCREEN_RSP,&Rsp,sizeof(PlaybackFullScreenRsp));
			u32Width = u32Width - stPlayInfo.SubWidth;
	        u32Height = u32Height - stPlayInfo.SubHeight;
	}
	PB_Full_id=stPlayInfo.FullScreenId;
	PRV_SetPlayInfo(&stPlayInfo);
#if defined(SN9234H1)
    if(stPlayInfo.IsSingle)
		Mcc_req.Single=1;
	else
	   Mcc_req.Single=stPlayInfo.bISDB;
	Mcc_req.FullScreenId=stPlayInfo.FullScreenId;
	if(stPlayInfo.IsSingle)
		Mcc_req.VoChn=0;
	else
	   Mcc_req.VoChn=stPlayInfo.DBClickChn;
	Mcc_req.ImagCount=stPlayInfo.ImagCount;
	Mcc_req.flag=0;
	 SN_SendMccMessageEx(PRV_SLAVE_1, SUPER_USER_ID, MOD_PRV, MOD_PRV, msg_req->xid, msg_req->thread, MSG_ID_PRV_MCC_PBFULLSCREEN_REQ, &Mcc_req, sizeof(PRV_MccPBCtlReq));

#endif

#if !defined(SN9234H1)
	if(IsSingle==1)
    {
           s32Ret = Playback_VoDevSingle(GuiVoDev,DB_Vo);
		   if (s32Ret != HI_SUCCESS)
		   {
			   return HI_FAILURE;
		   }
    }
	else if(IsDB==0)
	{
            s32Ret = Playback_VoDevMul(GuiVoDev,s32ChnCount);
			 if (s32Ret != HI_SUCCESS)
			 {
				 return HI_FAILURE;
			 }
	}
	else
	{
            s32Ret = Playback_VoDevSingle(GuiVoDev,DB_Vo);
			 if (s32Ret != HI_SUCCESS)
			 {
				 return HI_FAILURE;
			 }
	}
#endif
    return HI_SUCCESS;
	
}
HI_S32 PlayBack_QueryPbStat(HI_U32 Vochn)
{
      g_ChnPlayStateInfo stPlayStateInfo;
      PRV_GetVoChnPlayStateInfo(Vochn, &stPlayStateInfo);
	  if( stPlayStateInfo.SynState != SYN_NOPLAY&&stPlayStateInfo.SynState != SYN_OVER&&
	  	stPlayStateInfo.CurPlayState != DEC_STATE_STOP && stPlayStateInfo.CurPlayState != DEC_STATE_EXIT&&
	  	VochnInfo[Vochn].VdecChn<30&&VochnInfo[Vochn].VdecChn>=0)
	    {
            return HI_SUCCESS;
		}
	  else return HI_FAILURE;
}



/**********************************************************************************************************************************
������:Playback_MsgZoomReq
����:  �����Ŵ�ͨ������
�������: HI_S32 *ReSetType   ��λ����
		  FileInfo *PlaybackFileInfo �ط���Ϣ�ṹ��ָ��
		  SN_MSG *vam_msg_dec    ��Ϣ�ṹ��ָ��
		  ST_FMG_QUERY_FILE_RSP *st_file  �ļ��б�ָ��
		  VIDEO_FRAME_INFO_S *stUserFrameInfo ��¼��ͼƬ����ָ��
���:    ��
***********************************************************************************************************************************/
HI_S32 Playback_DB(SN_MSG *msg_req)
{
	ZoomReq *Req=(ZoomReq *)msg_req->para;
	g_PlayInfo stPlayInfo;
	HI_U32 curVO,mode;
    ZoomRsp Rsp;
	HI_S32 s32Ret = 0, GuiVoDev = 0, s32ChnCount = 0;
	VO_VIDEO_LAYER_ATTR_S pstLayerAttr;
#if defined(SN9234H1)

    PRV_MccPBCtlReq Mcc_req;
#endif
	curVO=Req->ImagID;

    PRV_GetPlayInfo(&stPlayInfo);
	
    PRV_GetGuiVo(&GuiVoDev);
	s32Ret = HI_MPI_VO_GetVideoLayerAttr(GuiVoDev, &pstLayerAttr);
	s32ChnCount = stPlayInfo.ImagCount;
if(stPlayInfo.IsSingle==0)
{
	if(!stPlayInfo.bISDB)//�ǵ�ͨ���Ŵ󲥷�
	{   
        if(PlayBack_QueryPbStat(curVO)!=HI_SUCCESS)
	    {
             Rsp.result=-1;
	         SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_MMI, msg_req->xid, msg_req->thread, 
							MSG_ID_PRV_PBDBCLICK_RSP,&Rsp,sizeof(ZoomRsp));
			 return HI_FAILURE;

		}
		Rsp.result=0;
	    SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_MMI, msg_req->xid, msg_req->thread, 
							MSG_ID_PRV_PBDBCLICK_RSP,&Rsp,sizeof(ZoomRsp));
        mode=stPlayInfo.bISDB;
		stPlayInfo.bISDB=1;
		stPlayInfo.DBClickChn=curVO;
		
	}
    else //��ͨ���Ŵ󲥷�
    {
        mode=stPlayInfo.bISDB;
		Rsp.result=1;
		stPlayInfo.bISDB=0;
	    SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_MMI, msg_req->xid, msg_req->thread, 
							MSG_ID_PRV_PBDBCLICK_RSP,&Rsp,sizeof(ZoomRsp));
				
	}

	PRV_SetPlayInfo(&stPlayInfo);
#if defined(SN9234H1)
	Mcc_req.Single=stPlayInfo.bISDB;
	Mcc_req.FullScreenId=stPlayInfo.FullScreenId;
	Mcc_req.VoChn=stPlayInfo.DBClickChn;
	Mcc_req.ImagCount=stPlayInfo.ImagCount;
	Mcc_req.flag=0;
	SN_SendMccMessageEx(PRV_SLAVE_1, SUPER_USER_ID, MOD_PRV, MOD_PRV, msg_req->xid, msg_req->thread, MSG_ID_PRV_MCC_PBDBCLICK_REQ, &Mcc_req, sizeof(PRV_MccPBCtlReq));
#endif
#if !defined(SN9234H1)
    if(stPlayInfo.bISDB==0)
    {
          s32Ret = Playback_VoDevMul(GuiVoDev,s32ChnCount);
		  if (s32Ret != HI_SUCCESS)
	      {
		      return HI_FAILURE;
	      }
	}
	else
	{
          s32Ret = Playback_VoDevSingle(GuiVoDev,curVO);
		  if (s32Ret != HI_SUCCESS)
	      {
		      return HI_FAILURE;
	      }
	}
#endif 

}
 
	return HI_SUCCESS;
	
}

STATIC HI_S32 PRV_MSG_SetGroupNameCfg(const SN_MSG *msg_req)
{
	GetParameter (PRM_ID_LINKAGE_GROUP_CFG, NULL, g_PrmLinkAge_Cfg,sizeof (g_PrmLinkAge_Cfg), 0, SUPER_USER_ID, NULL);
#if defined(SN9234H1)
	PRV_Check_LinkageGroup(HD,s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewMode);
	OSD_Update_GroupName();
	Prv_Disp_OSD(HD);
#else
	PRV_Check_LinkageGroup(DHD0,s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewMode);
	OSD_Update_GroupName();
	Prv_Disp_OSD(DHD0);
#endif
	return 0;
}

STATIC HI_S32 PRV_MSG_SerialNoChange_Ind(const SN_MSG *msg_req)
{
	int ret = 0;
#if defined(SN9234H1)
	ret = PRV_Check_LinkageGroup(HD,s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewMode);
	if(ret == 1)
	{
		Prv_Disp_OSD(HD);
	}
#else
	ret = PRV_Check_LinkageGroup(DHD0,s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewMode);
	if(ret == 1)
	{
		Prv_Disp_OSD(DHD0);
	}
#endif
	return 0;
}
/*****************************************************************************************************************************/



/************************************************************************/
/* PRVģ����Ϣ�̺߳�����
                                                                     */
/************************************************************************/
STATIC HI_VOID *PRV_ParseMsgProc(HI_VOID *param)
{

	SN_MSG *msg_req = NULL;//,*msg_req1=NULL;
	

	int queue, ret;
	Log_pid(__FUNCTION__);

	queue = CreatQueque(MOD_PRV);
	if (queue <= 0)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_PRV: CreateQueue Failed: queue = %d", queue);
		return NULL;
	}
	//PRV_Init_TimeOut(0);
	for (;;)
	{
		msg_req = SN_GetMessage(queue, MSG_GET_WAIT_ROREVER, &ret);
		if (ret < 0)
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_PRV: SN_GetMessage Failed: %#x", ret);
			//sleep(1000);
			continue;
		}
		if (NULL == msg_req)
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_PRV: SN_GetMessage return Null Pointer!");
			//sleep(1000);
			continue;
		}
		//TRACE(SCI_TRACE_NORMAL, MOD_PRV,"######get msg msg_req->msgId = %x, src:%d ##############\n",msg_req->msgId,msg_req->source);
		pthread_mutex_lock(&send_data_mutex);
		switch(msg_req->msgId)
		{
#if 1
			case MSG_ID_PRV_DISPLAY_TIMEOUT_IND://��Ƭ��Ϣ��ʱ��ʱ��Ϣ
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_PRV_DISPLAY_TIMEOUT_IND\n", __LINE__);
				//printf("###############MSG_ID_PRV_DISPLAY_TIMEOUT_IND !!!!msg_req->msgId = %x##########################\n",msg_req->msgId);
				s_State_Info.TimeoutCnt++;
				s_State_Info.bIsTimerState = HI_FALSE;
				PRV_TimeOut_Proc();
				break;
			}
#endif
			case MSG_ID_PRV_GET_REC_OSDRES_IND://��ȡ��ǰ¼��ֱ���
			{				
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Receive Message: MSG_ID_PRV_GET_REC_OSDRES_IND\n");
				PRV_MSG_GetRec_Resolution(msg_req);
			}
				break;
			case MSG_ID_PRV_LAYOUT_CTRL_REQ:
			{
				//printf("%s Line %d ---------> msg_req->source: %d\n",__func__,__LINE__, msg_req->source);
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "msg_req->source: %d, Receive Message: MSG_ID_PRV_LAYOUT_CTRL_REQ, SlaveConfig: %d\n", msg_req->source, s_State_Info.bIsSlaveConfig);
				if(s_State_Info.bIsSlaveConfig == HI_FALSE)
				{				
					//printf("%s Line %d ---------> here\n",__func__,__LINE__);
					Msg_id_prv_Rsp rsp;
					ret = PRV_MSG_LayoutCtrl(msg_req,&rsp);
					if(ret != SN_SLAVE_MSG)
					{
						SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_FWK, msg_req->xid, msg_req->thread, 
							MSG_ID_PRV_LAYOUT_CTRL_RSP, &rsp, sizeof(rsp));
					
					}else
					{
						s_State_Info.bIsSlaveConfig = HI_TRUE;
					}
				}	
#if defined(Hi3531)||defined(Hi3535)
				SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_MMI, msg_req->xid, msg_req->thread, MSG_ID_PRV_SCREEN_CTRL_IND, NULL, 0);
#endif
			}
				break;				
			case MSG_ID_PRV_SCREEN_CTRL_REQ:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_PRV_SCREEN_CTRL_REQ, s_State_Info.bIsSlaveConfig: %d\n", __LINE__, s_State_Info.bIsSlaveConfig);


#if (IS_DECODER_DEVTYPE == 1)
				SN_SendMessageEx(msg_req->user, msg_req->source, MOD_SCM, msg_req->xid, msg_req->thread, MSG_ID_PRV_SCREEN_CTRL_REQ, msg_req->para, msg_req->size);
#else				
				Screen_ctrl_Req *param = (Screen_ctrl_Req *)msg_req->para;				
				if(s_State_Info.bIsSlaveConfig == HI_FALSE || param->dev == SPOT_VO_DEV)
				{
					Msg_id_prv_Rsp rsp;
					ret = PRV_MSG_ScreenCtrl(msg_req,&rsp);
					if(ret != SN_SLAVE_MSG)
					{
						//SPOT���л�����Ҫ֪ͨUI
						
						/*SN_SendMessageEx(msg_req->user, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, 
							MSG_ID_PRV_SCREEN_CTRL_RSP, &rsp, sizeof(rsp));*/
						if(param->dev != SPOT_VO_DEV)
						{
							SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_MMI, msg_req->xid, msg_req->thread, 
								MSG_ID_PRV_SCREEN_CTRL_IND, NULL, 0);
						}
						
					}else
					{
						s_State_Info.bIsSlaveConfig = HI_TRUE;
					}
				}
#endif
				break;
			}	
				
			case MSG_ID_PRV_ENTER_CHN_CTRL_REQ:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_PRV_ENTER_CHN_CTRL_REQ, SlaveConfig: %d\n", __LINE__, s_State_Info.bIsSlaveConfig);
				if(s_State_Info.bIsSlaveConfig == HI_FALSE)
				{
					Msg_id_prv_Rsp rsp;
					ret = PRV_MSG_EnterChnCtrl(msg_req, &rsp);
					if(ret != SN_SLAVE_MSG)
					{
						SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_FWK, msg_req->xid, msg_req->thread, 
							MSG_ID_PRV_ENTER_CHN_CTRL_RSP, &rsp, sizeof(rsp));
					}
					else
					{
						s_State_Info.bIsSlaveConfig = HI_TRUE;
					}
					
				}
#if defined(Hi3531)||defined(Hi3535)				
				pthread_mutex_unlock(&send_data_mutex);
				usleep(10000);
				pthread_mutex_lock(&send_data_mutex);
				
				g_ChnPlayStateInfo stPlayStateInfo;
				g_PlayInfo	stPlayInfo;
				PRV_GetPlayInfo(&stPlayInfo);
				PRV_GetVoChnPlayStateInfo(stPlayInfo.InstantPbChn, &stPlayStateInfo);
				if(stPlayInfo.PlayBackState == PLAY_INSTANT && stPlayStateInfo.CurPlayState == DEC_STATE_NORMALPAUSE)
				{
#if defined(Hi3535)
					HI_MPI_VO_PauseChn(0, stPlayInfo.InstantPbChn);
#else
					HI_MPI_VO_ChnPause(0, stPlayInfo.InstantPbChn);
#endif
					if(Achn == stPlayInfo.InstantPbChn)
					{
#if defined(Hi3531)
						HI_MPI_AO_PauseChn(4, AOCHN);
#else
						HI_MPI_AO_PauseChn(0, AOCHN);
#endif
						HI_MPI_ADEC_ClearChnBuf(DecAdec);
					}
				}
#endif				
			}
				break;
			case MSG_ID_PRV_EXIT_CHN_CTRL_REQ:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_PRV_EXIT_CHN_CTRL_REQ\n", __LINE__);
				Exit_chn_ctrl_Req *param = (Exit_chn_ctrl_Req *)msg_req->para;
				//Ԥ��״̬�£��յ��˳��طſ�����Ϣ������
#if defined(SN9234H1)
				if(s_astVoDevStatDflt[HD].enPreviewStat == PRV_STAT_NORM
					&& (param->flag == 4 || param->flag == SLC_CTL_FLAG || param->flag == PIC_CTL_FLAG))
#else				
				if(s_astVoDevStatDflt[DHD0].enPreviewStat == PRV_STAT_NORM
					&& (param->flag == 4 || param->flag == SLC_CTL_FLAG || param->flag == PIC_CTL_FLAG))
#endif					
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Current Stat: PRV_STAT_NORM, Discard MSG---param->flag: %d\n", param->flag);					
					break;
				}
				//if(s_State_Info.bIsSlaveConfig == HI_FALSE)
				{
					Msg_id_prv_Rsp rsp;
					ret = PRV_MSG_ExitChnCtrl(msg_req,&rsp);
				
					if(ret != SN_SLAVE_MSG)
					{
						if(rsp.flag == 4)
						{
							Exit_chn_ctrl_Req ExitPb;
							SN_MEMSET(&ExitPb, 0, sizeof(Exit_chn_ctrl_Req));
							ExitPb.flag = 4;
							SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_SCM, msg_req->xid, msg_req->thread, MSG_ID_PRV_EXIT_CHN_CTRL_REQ, &ExitPb, sizeof(Exit_chn_ctrl_Req));
						}
						else
						{
							SN_SendMessageEx(msg_req->user, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, 
								MSG_ID_PRV_EXIT_CHN_CTRL_RSP, &rsp, sizeof(rsp));
						}
						if(s_astVoDevStatDflt[s_VoDevCtrlDflt].bIsAlarm)
						{
							SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_MMI, msg_req->xid, msg_req->thread, 
										MSG_ID_PRV_SCREEN_CTRL_IND, NULL, 0);
						}
					}
					else
					{
						s_State_Info.bIsSlaveConfig = HI_TRUE;
					}
				}
			}
				break;
			case MSG_ID_PRV_SET_VO_PREVIEW_REQ:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_PRV_SET_VO_PREVIEW_REQ\n", __LINE__);
				Set_vo_preview_Rsp rsp;
				PRV_MSG_SetVoPreview(msg_req, &rsp);
				SN_SendMessageEx(msg_req->user, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, 
					MSG_ID_PRV_SET_VO_PREVIEW_RSP, &rsp, sizeof(rsp));
			}
				break;
			case MSG_ID_PRV_SET_GENERAL_CFG_REQ:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, The message is :MSG_ID_PRV_SET_GENERAL_CFG_REQ, bIsInit=%d\n", __LINE__, s_State_Info.bIsInit);
#if 1
				if(s_State_Info.bIsInit == 0)
				{
					Set_general_cfg_Rsp rsp;
					PRV_MSG_SetGeneralCfg(msg_req, &rsp);
#if defined(SN9234H1)
					PRV_ResetVoDev(HD);
#else					
					PRV_ResetVoDev(DHD0);
#endif					
					//PRV_ResetVoDev(s_VoSecondDev);
					PRV_SetGuiAlpha(0, 0);
					SN_SendMessageEx(msg_req->user, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, 
						MSG_ID_PRV_SET_GENERAL_CFG_RSP, &rsp, sizeof(rsp));
					Fb_clear_step2();
					ret = OSD_init(s_OSD_Time_type);
					if(ret == 0)
					{
						s_State_Info.bIsOsd_Init = 1;
					}	
					if(s_State_Info.bIsVam_rsp == 0)
					{
						SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_VAM, msg_req->xid, msg_req->thread, 
							MSG_ID_PRV_OSD_GETRECT_RSP, NULL, 0);
					}
#if defined (SN6116HE) || defined (SN6116LE) || defined (SN6108HE)	 || defined(SN6108LE)	 || defined(SN8608D_LE) || defined(SN8608M_LE) || defined(SN8616D_LE) || defined(SN8616M_LE)|| defined(SN9234H1)
					PRV_Init_TimeOut(0);
#endif
					s_State_Info.bIsInit = 1;	//Ԥ���Ѿ���ʼ��
					
				}
				else
				{
#if 1				
					Set_general_cfg_Rsp rsp;
					//ͨ������ǰ�ѱ�־λ����
					s_State_Info.bIsNpfinish = 0;

					//SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_VAM, msg_req->xid, msg_req->thread, 
					//		MSG_ID_PRV_SET_GENERAL_CFG_REQ, msg_req->para,msg_req->size);
					
					ret = PRV_MSG_SetGeneralCfg(msg_req, &rsp);
					if(ret == SN_SLAVE_MSG)
					{//�����Ҫ������Ϣ����Ƭ
						s_State_Info.bIsSlaveConfig = HI_TRUE;	//��λ���ñ�־λ
						break;
					}
					PRV_SetGuiAlpha(0, 0);
					s_State_Info.bIsNpfinish = 1;
					if(msg_req->user == SUPER_USER_ID)
					{
						SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_FWK, msg_req->xid, msg_req->thread, 
								MSG_ID_PRV_SET_GENERAL_CFG_RSP, &rsp, sizeof(rsp));
					}
					else
					{
						SN_SendMessageEx(msg_req->user, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, 
								MSG_ID_PRV_SET_GENERAL_CFG_RSP, &rsp, sizeof(rsp));
					}
#else					
					int cnt=0,s32Flag=0;
					Set_general_cfg_Req *req = (Set_general_cfg_Req *)msg_req->para;
					
					while(cnt<1000000)
					{
						s32Flag = (0 == s32Flag) ? 1 : 0;
						req->general_info.CVBSOutputType = s32Flag;
						SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_VAM, msg_req->xid, msg_req->thread, 
							MSG_ID_PRV_SET_GENERAL_CFG_REQ, msg_req->para,msg_req->size);
						msg_req1 = SN_GetMessage(queue, MSG_GET_WAIT_ROREVER, &ret);
						switch(msg_req1->msgId)
						{
							case MSG_ID_PRV_SET_GENERAL_CFG_RSP:
							{
								Set_general_cfg_Rsp rsp,*rsp1 = (Set_general_cfg_Rsp *)msg_req1->para;
								printf("#############req->general_info.CVBSOutputType =%d,CNT=%d\n",rsp1->general_info.CVBSOutputType,cnt);
								ret = PRV_MSG_SetGeneralCfg(msg_req1, &rsp);
								if(ret == SN_SLAVE_MSG)
								{//�����Ҫ������Ϣ����Ƭ
									s_State_Info.bIsSlaveConfig = HI_TRUE;	//��λ���ñ�־λ
									break;
								}
								/*����ظ��ɹ�����ʾ��ʽ���޸ģ���ô������Ϣ��VAMģ��*/
								if(rsp1->result == 0)
								{
									if (rsp.result == 0)
									{
										SN_SendMessageEx(msg_req1->user, MOD_PRV, MOD_VAM, msg_req1->xid, msg_req1->thread, 
											MSG_ID_PRV_VI_START_REQ, NULL, 0);
									}
								}
								SN_SendMessageEx(msg_req1->user, MOD_PRV, MOD_DRV, msg_req1->xid, msg_req1->thread, 
										MSG_ID_PRV_SET_GENERAL_CFG_RSP, &rsp, sizeof(rsp));
							}
								break;
							default:
								break;
						}
						SN_FreeMessage(&msg_req1);
						sleep(3);
						cnt++;
					}
#endif					
				}
#else
					
				Set_general_cfg_Rsp rsp;
				PRV_MSG_SetGeneralCfg((Set_general_cfg_Req *)msg_req->para, &rsp);
				SN_SendMessageEx(msg_req->user, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, 
					MSG_ID_PRV_SET_GENERAL_CFG_RSP, &rsp, sizeof(rsp));
					if(s_State_Info.bIsOsd_Init == 0)
					{
						OSD_init(s_OSD_Time_type);
						s_State_Info.bIsOsd_Init = 1;
				}
#endif
			}
				break;
			case MSG_ID_PRV_SET_CHN_CFG_REQ:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_PRV_SET_CHN_CFG_REQ\n", __LINE__);
				Set_chn_cfg_Rsp rsp;				
				Set_chn_cfg_Req *param = (Set_chn_cfg_Req *)msg_req->para;
				ret = PRV_MSG_SetChnCfg(param, &rsp);    
				if(ret != SN_SLAVE_MSG)
				{
					SN_SendMessageEx(msg_req->user, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, 
						MSG_ID_PRV_SET_CHN_CFG_RSP, &rsp, sizeof(rsp));
				}
				else
				{
					s_State_Info.bIsSlaveConfig = HI_TRUE;
				}
				
			}
				break;
			case MSG_ID_PRV_SET_CHN_COVER_REQ:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_PRV_SET_CHN_COVER_REQ\n", __LINE__);
				Set_chn_cover_Rsp rsp;
				ret = PRV_MSG_SetChnCover(msg_req, &rsp);
				if(ret != SN_SLAVE_MSG)
				{
					SN_SendMessageEx(msg_req->user, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, 
						MSG_ID_PRV_SET_CHN_COVER_RSP, &rsp, sizeof(rsp));
				}
				else
				{
					s_State_Info.bIsSlaveConfig = HI_TRUE;
				}
				
			}
				break;
			case MSG_ID_PRV_SET_CHN_OSD_REQ:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_PRV_SET_CHN_OSD_REQ\n", __LINE__);
				Set_chn_osd_Rsp rsp;
				ret = PRV_MSG_SetChnOsd(msg_req, &rsp);
				if(ret != SN_SLAVE_MSG)
				{
					SN_SendMessageEx(msg_req->user, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, 
						MSG_ID_PRV_SET_CHN_OSD_RSP, &rsp, sizeof(rsp));
				}
				else
				{
					s_State_Info.bIsSlaveConfig = HI_TRUE;
				}
#if 0 /* �Ѹĳ���PRV_OSD_RECT_CHANGE_NOTIFY�ӿ�֪ͨVAMģ�� */
				SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_VAM, msg_req->xid, msg_req->thread, 
					MSG_ID_PRV_SET_CHN_OSD_RSP, &rsp, sizeof(rsp));
#endif
			}
				break;
			case MSG_ID_PRV_SET_CHN_DISPLAY_REQ:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_PRV_SET_CHN_DISPLAY_REQ\n", __LINE__);
				Set_chn_display_Rsp rsp;
				PRV_MSG_SetChnDisplay((Set_chn_display_Req *)msg_req->para, &rsp);
				SN_SendMessageEx(msg_req->user, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, 
					MSG_ID_PRV_SET_CHN_DISPLAY_RSP, &rsp, sizeof(rsp));
			}
				break;
			case MSG_ID_PRV_CHN_ICON_CTRL_REQ:
			{
				//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_PRV_CHN_ICON_CTRL_REQ\n", __LINE__);
				Msg_id_prv_Rsp rsp;
				PRV_MSG_ChnIconCtrl((Chn_icon_ctrl_Req *)msg_req->para, &rsp);
				SN_SendMessageEx(msg_req->user, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, 
					MSG_ID_PRV_CHN_ICON_CTRL_RSP, &rsp, sizeof(rsp));
			}
				break;
				
			case MSG_ID_PRV_OUTPUT_CHANGE_REQ:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_PRV_OUTPUT_CHANGE_REQ\n", __LINE__);
				Msg_id_prv_Rsp rsp;
				PRV_MSG_OutputChange((Output_Change_Req *)msg_req->para, &rsp);
				SN_SendMessageEx(msg_req->user, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, 
					MSG_ID_PRV_OUTPUT_CHANGE_RSP, &rsp, sizeof(rsp));
			}
				break;
				
			case MSG_ID_PRV_CHN_ZOOM_IN_REQ:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_PRV_CHN_ZOOM_IN_REQ\n", __LINE__);
				Msg_id_prv_Rsp rsp;
				ret = PRV_MSG_ChnZoomIn(msg_req,&rsp);
				SN_SendMessageEx(msg_req->user, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, 
					MSG_ID_PRV_CHN_ZOOM_IN_RSP, &rsp, sizeof(rsp));
			}
				break;
				
			case MSG_ID_PRV_GET_GUI_VO_REQ:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_PRV_GET_GUI_VO_REQ\n", __LINE__);
				Get_gui_vo_Rsp rsp;
				PRV_MSG_GetGuiVo(&rsp);
				SN_SendMessageEx(msg_req->user, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, 
					MSG_ID_PRV_GET_GUI_VO_RSP, &rsp, sizeof(rsp));
			}
				break;
				
			case MSG_ID_PRV_CHN_DISPLAY_CHANGE_REQ:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_PRV_CHN_DISPLAY_CHANGE_REQ\n", __LINE__);
				Msg_id_prv_Rsp	rsp;
				PRV_MSG_SetDisplay_Time((Chn_disp_change_Req *)msg_req->para, &rsp);
				SN_SendMessageEx(msg_req->user, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, 
					MSG_ID_PRV_CHN_DISPLAY_CHANGE_RSP, &rsp, sizeof(rsp));
			}
				break;
				
			case MSG_ID_PRV_GET_CHN_BY_XY_REQ:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_PRV_GET_CHN_BY_XY_REQ\n", __LINE__);
				Get_chn_by_xy_Rsp	rsp;
				PRV_MSG_GetChnByXY((Get_chn_by_xy_Req *)msg_req->para, &rsp);
				SN_SendMessageEx(msg_req->user, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, 
					MSG_ID_PRV_GET_CHN_BY_XY_RSP, &rsp, sizeof(rsp));
			}
				break;
				
			case MSG_ID_PRV_GET_PRV_MODE_REQ:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_PRV_GET_PRV_MODE_REQ\n", __LINE__);
				Get_prv_mode_Rsp rsp;
				PRV_MSG_GetPrvMode(&rsp);
				SN_SendMessageEx(msg_req->user, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, 
					MSG_ID_PRV_GET_PRV_MODE_RSP, &rsp, sizeof(rsp));
			}
				break;
				
			case MSG_ID_PRV_OSD_GETRECT_REQ:
			{//�ظ�¼��ģ�鿪���ƶ����
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_PRV_OSD_GETRECT_REQ\n", __LINE__);
				if(s_State_Info.bIsInit)
				{//Ԥ����ʼ����ɺ󣬷�����Ϣ
					SN_SendMessageEx(msg_req->user, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, 
						MSG_ID_PRV_OSD_GETRECT_RSP, NULL, 0);
				}
				else
				{//Ԥ����ʼ��δ��ɣ���λ�ظ���־λ
					s_State_Info.bIsVam_rsp = 0;
				}
			}
				break;
				
			case MSG_ID_PRV_SET_GENERAL_CFG_RSP:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_PRV_SET_GENERAL_CFG_RSP\n", __LINE__);
				Set_general_cfg_Rsp rsp,*rsp1 = (Set_general_cfg_Rsp *)msg_req->para;

				//����ظ�ʧ�ܣ���ô�������л�
				if(rsp1->result == SN_ERR_VAM_NPSET)
				{
					rsp.result = -1;
				}
				else
				{//��������½�������
					ret = PRV_MSG_SetGeneralCfg(msg_req, &rsp);
					if(ret == SN_SLAVE_MSG)
					{//�����Ҫ������Ϣ����Ƭ
						s_State_Info.bIsSlaveConfig = HI_TRUE;	//��λ���ñ�־λ
						break;
					}
				}
				s_State_Info.bIsNpfinish = 1;
				
				if(msg_req->user == SUPER_USER_ID)
				{
					SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_FWK, msg_req->xid, msg_req->thread, 
							MSG_ID_PRV_SET_GENERAL_CFG_RSP, &rsp, sizeof(rsp));
				}
				else
				{
					SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_DRV, msg_req->xid, msg_req->thread, 
							MSG_ID_PRV_SET_GENERAL_CFG_RSP, &rsp, sizeof(rsp));
				}
					
			}
				break;
				
			case MSG_ID_FWK_POWER_OFF_REQ:
			/*{
				int chan;
				chan = s_astVoDevStatDflt[SPOT_VO_DEV].as32ChnOrder[SingleScene][s_astVoDevStatDflt[SPOT_VO_DEV].s32SingleIndex];
		 		if(chan > PRV_CHAN_NUM)
		 		{
					PRV_HostStopPciv(SPOT_PCI_CHAN, MSG_ID_PRV_MCC_SPOT_PREVIEW_STOP_REQ);
		 		}
				if(msg_req->source == MOD_VAM)
				{
					exit_mpp_sys();
				}
				SN_SendMessageEx(msg_req->user, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, 
					MSG_ID_FWK_POWER_OFF_RSP, NULL, 0);
			
				
			}	
				if(msg_req->source == MOD_VAM)
				{
					SN_FreeMessage(&msg_req);
					return NULL;
				}
				else
					break;*/
			case MSG_ID_FWK_REBOOT_REQ:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_FWK_REBOOT_REQ\n", __LINE__);

				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "---------PRV---Begin Exit Sys\n");
					exit_mpp_sys();
				}
				SN_SendMessageEx(msg_req->user, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, 
					msg_req->msgId+1, NULL, 0);

				{
					SN_FreeMessage(&msg_req);
					pthread_mutex_unlock(&send_data_mutex);
					return NULL;
				}

			}	
				break;

			case MSG_ID_NTRANS_MEMRESET_REQ:
			{
				int i = 0;
				for(i = 0; i < DEV_CHANNEL_NUM; i++)
				{
					BufferSet(i + PRV_VIDEOBUFFER, MAX_ARRAY_NODE);
					BufferSet(i + PRV_AUDIOBUFFER, MAX_ARRAY_NODE);
					VochnInfo[i].bIsWaitIFrame = 1;					
				}
				SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_NTRANS, 0,  0,MSG_ID_NTRANS_MEMRESET_RSP,  NULL, 0);
			}
				break;
			case MSG_ID_FWK_UPGRADE_RSP:     /* ���� */
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------receive MSG_ID_FWK_UPGRADE_RSP");
				IsUpGrade = 1;
			}
				break;
			case MSG_ID_ALM_EXCEPTION_ALARM_RSP:
				break;
				
			case MSG_ID_PRV_VI_START_RSP:
				break;
				
			case MSG_ID_PRV_SET_PREVIEW_ADV_REQ://Ԥ����Ƶ�߼���������
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_PRV_SET_PREVIEW_ADV_REQ\n", __LINE__);
				Set_vo_preview_Adv_Rsp rsp;
				PRV_MSG_SetVoPreview_Adv((Set_vo_preview_Adv_Req *)msg_req->para, &rsp);
				SN_SendMessageEx(msg_req->user, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, 
					MSG_ID_PRV_SET_PREVIEW_ADV_RSP, &rsp, sizeof(rsp));
			}	
				break;
				
			case MSG_ID_PRV_SET_PREVIEW_AUDIO_REQ://ϵͳ��Ƶ��������
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_PRV_SET_PREVIEW_AUDIO_REQ\n", __LINE__);
				Set_preview_Audio_Rsp rsp;
				PRV_MSG_SetPreview_Audio((Set_preview_Audio_Req *)msg_req->para, &rsp);
				SN_SendMessageEx(msg_req->user, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, 
					MSG_ID_PRV_SET_PREVIEW_AUDIO_RSP, &rsp, sizeof(rsp));
			}
				break;
				
			case MSG_ID_PRV_MCC_SLAVE_INIT_RSP: //��Ƭ��ʼ���ظ�
			{
				PRV_MSG_Mcc_Init_Rsp(msg_req);
			}
				break;
				
			case MSG_ID_PRV_MCC_LAYOUT_CTRL_RSP: //��Ƭ�����л��ظ�
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_PRV_MCC_LAYOUT_CTRL_RSP\n", __LINE__);
				Msg_id_prv_Rsp rsp;
				PRV_MSG_MCC_LayoutCtrl_Rsp((Prv_Slave_Layout_crtl_Rsp *)msg_req->para,&rsp);
				SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_FWK, msg_req->xid, msg_req->thread, 
						MSG_ID_PRV_LAYOUT_CTRL_RSP, &rsp, sizeof(rsp));
				SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_MMI, msg_req->xid, msg_req->thread, 
					MSG_ID_PRV_SCREEN_CTRL_IND, NULL, 0);
			}
				break;
			
#if (IS_DECODER_DEVTYPE == 1)

#else
			case MSG_ID_PRV_MCC_SCREEN_CTRL_RSP://��Ƭ��һ���ظ�
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_PRV_MCC_SCREEN_CTRL_RSP\n", __LINE__);
				Msg_id_prv_Rsp rsp;
				PRV_MSG_Mcc_ScreenCtrl_Rsp((Prv_Slave_Screen_ctrl_Rsp *)msg_req->para,&rsp);
				SN_SendMessageEx(msg_req->user, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, 
					MSG_ID_PRV_SCREEN_CTRL_RSP, &rsp, sizeof(rsp));
				SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_MMI, msg_req->xid, msg_req->thread, 
					MSG_ID_PRV_SCREEN_CTRL_IND, NULL, 0);
			}
				break;
#endif
			case MSG_ID_PRV_MCC_ENTER_CHN_CTRL_RSP:	//��Ƭ����ͨ�����ƻظ�
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_PRV_MCC_ENTER_CHN_CTRL_RSP\n", __LINE__);
				Msg_id_prv_Rsp rsp;
				PRV_MSG_MCC_EnterChnCtrl_Rsp((Prv_Slave_Enter_chn_ctrl_Rsp *)msg_req->para,&rsp);
				SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_FWK, msg_req->xid, msg_req->thread, 
						MSG_ID_PRV_ENTER_CHN_CTRL_RSP, &rsp, sizeof(rsp));

				pthread_mutex_unlock(&send_data_mutex);
				usleep(100000);
				pthread_mutex_lock(&send_data_mutex);
				g_ChnPlayStateInfo stPlayStateInfo;
				g_PlayInfo	stPlayInfo;
				PRV_GetPlayInfo(&stPlayInfo);
				PRV_GetVoChnPlayStateInfo(stPlayInfo.InstantPbChn, &stPlayStateInfo);
				if(stPlayInfo.PlayBackState == PLAY_INSTANT && stPlayStateInfo.CurPlayState == DEC_STATE_NORMALPAUSE)
				{
#if defined(Hi3535)
					HI_MPI_VO_PauseChn(0, stPlayInfo.InstantPbChn);
#else
					HI_MPI_VO_ChnPause(0, stPlayInfo.InstantPbChn);
#endif
					if(Achn == stPlayInfo.InstantPbChn)
					{
#if defined(Hi3531)
						HI_MPI_AO_PauseChn(4, AOCHN);
#else
						HI_MPI_AO_PauseChn(0, AOCHN);
#endif
						HI_MPI_ADEC_ClearChnBuf(DecAdec);
					}
				}
			}	
				break;
				
			case MSG_ID_PRV_MCC_EXIT_CHN_CTRL_RSP://��Ƭ�˳�ͨ�����ƻظ�
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_PRV_MCC_EXIT_CHN_CTRL_RSP\n", __LINE__);
				Msg_id_prv_Rsp rsp;
				PRV_MSG_MCC_ExitChnCtrl_Rsp((Prv_Slave_Exit_chn_ctrl_Rsp *)msg_req->para,&rsp);
				//�ظ�GUI��Ϣ
				if(rsp.flag == 4)//�˳��ط�ʱ֪ͨSCM����Ԥ��
				{					
					Exit_chn_ctrl_Req ExitPb;
					SN_MEMSET(&ExitPb, 0, sizeof(Exit_chn_ctrl_Req));
					ExitPb.flag = 4;
					SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_SCM, msg_req->xid, msg_req->thread, MSG_ID_PRV_EXIT_CHN_CTRL_REQ, &ExitPb, sizeof(Exit_chn_ctrl_Req));
				}
				else
				{
					SN_SendMessageEx(msg_req->user, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, 
						MSG_ID_PRV_EXIT_CHN_CTRL_RSP, &rsp, sizeof(rsp));
				}
			}		
				break;
				
			case MSG_ID_PRV_MCC_SET_GENERAL_CFG_RSP:
			{//��Ƭͨ�����ô���
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_PRV_MCC_SET_GENERAL_CFG_RSP\n", __LINE__);
				Set_general_cfg_Rsp rsp;
				Prv_Slave_Set_general_cfg_Rsp *slave_rsp = (Prv_Slave_Set_general_cfg_Rsp *)msg_req->para;
				PRV_MSG_MCC_SetGeneralCfg_Rsp(slave_rsp,&rsp);
				/*����ظ��ɹ�����ʾ��ʽ���޸ģ���ô������Ϣ��VAMģ��*/
				if(slave_rsp->vam_result == 0)
				{
					if (rsp.result == 0)
					{
						SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_VAM, msg_req->xid, msg_req->thread, 
							MSG_ID_PRV_VI_START_REQ, NULL, 0);
					}
				}
				//������Ϣ�����������
				if(msg_req->user == SUPER_USER_ID)
				{
					SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_FWK, msg_req->xid, msg_req->thread, 
							MSG_ID_PRV_SET_GENERAL_CFG_RSP, &rsp, sizeof(rsp));
				}
				else
				{
					SN_SendMessageEx(msg_req->user, MOD_PRV, MOD_DRV, msg_req->xid, msg_req->thread, 
							MSG_ID_PRV_SET_GENERAL_CFG_RSP, &rsp, sizeof(rsp));
				}
			}	
				break;
				
			case MSG_ID_PRV_MCC_SET_CHN_CFG_RSP://��Ƭͨ�����ûظ�
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_PRV_MCC_SET_CHN_CFG_RSP\n", __LINE__);
				Set_chn_cfg_Rsp rsp;
				PRV_MSG_MCC_SetChnCfg_Rsp((Prv_Slave_Set_chn_cfg_Rsp*)msg_req->para,&rsp);
				//�ظ�GUI��Ϣ
				SN_SendMessageEx(msg_req->user, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, 
					MSG_ID_PRV_SET_CHN_CFG_RSP, &rsp, sizeof(rsp));
			}
				break;
				
			case MSG_ID_PRV_MCC_SET_CHN_COVER_RSP://��Ƭ�ڸ����ûظ�
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_PRV_MCC_SET_CHN_COVER_RSP\n", __LINE__);
				Set_chn_cover_Rsp rsp;
				PRV_MSG_MCC_SetChnCover_Rsp((Prv_Slave_Set_chn_cover_Rsp *)msg_req->para,&rsp);
				//�ظ�GUI��Ϣ
				SN_SendMessageEx(msg_req->user, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, 
					MSG_ID_PRV_SET_CHN_COVER_RSP, &rsp, sizeof(rsp));
			}
				break;
				
			case MSG_ID_PRV_MCC_SET_CHN_OSD_RSP: //��Ƭͨ��¼��OSD���ûظ�
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_PRV_MCC_SET_CHN_OSD_RSP\n", __LINE__);
				Set_chn_osd_Rsp rsp;
				PRV_MSG_MCC_SetChnOsd_Rsp((Prv_Slave_Set_chn_osd_Rsp*)msg_req->para,&rsp);
				//�ظ�GUI��Ϣ
				SN_SendMessageEx(msg_req->user, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, 
					MSG_ID_PRV_SET_CHN_OSD_RSP, &rsp, sizeof(rsp));
			}
				break;
#if defined(SN9234H1)
			case MSG_ID_PRV_MCC_SPOT_PREVIEW_NEXT_RSP: //SPOT���л�����һ��ͨ������Ӧ
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_PRV_MCC_SPOT_PREVIEW_NEXT_RSP\n", __LINE__);
				int ch = s_astVoDevStatDflt[SPOT_VO_DEV].as32ChnOrder[SingleScene][s_astVoDevStatDflt[SPOT_VO_DEV].s32SingleIndex];
				//printf("MSG_ID_PRV_MCC_SPOT_PREVIEW_NEXT_RSP:%d\n", ch);
				PRV_start_pciv(ch);
				PRV_RefreshSpotOsd(ch);
			}
				break;				
#endif	
			case MSG_ID_PRV_MCC_UPDATE_OSD_RSP: //��ƬOSD������Ϣ�ظ�
		    {
		                //TODO
		    }
	            break;
					
			case MSG_ID_VAM_RCD_CFG_ADC_REQ:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_VAM_RCD_CFG_ADC_REQ\n", __LINE__);
			//	VO_CHN chn=0;
				RecAdcParamReq * req = (RecAdcParamReq *)msg_req->para;
				PRV_Set_AudioMap(req->ch_num, req->RecCfg.AudioRecord);
			//	PRV_GetFirstChn(s_VoDevCtrlDflt,&chn);
			//	PRV_AudioPreviewCtrl((const unsigned char *)&chn, 1);			
			//	printf("MSG_ID_VAM_RCD_CFG_ADC_REQ ch == %d, record=%d\n",req->ch_num, req->RecCfg.AudioRecord);
			
				PRV_PlayAudio(s_VoDevCtrlDflt);
			}
				break;
				
            case MSG_ID_FWK_SYSTEM_INITOK_IND:
            {
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_FWK_SYSTEM_INITOK_IND\n", __LINE__);
				s_bIsSysInit = HI_TRUE;
#if defined(SN9234H1)
				PRV_SetDevCsc(HD);
				if(DEV_SPOT_NUM > 0)
				{
					//static HI_S32 PRV_PreviewInit(HI_VOID);				
					PRV_PreviewInit();
		
					//��ʼ��SPOT��
					int chn = 0, index = 0;
					chn = s_astVoDevStatDflt[SPOT_VO_DEV].as32ChnOrder[SingleScene][s_astVoDevStatDflt[SPOT_VO_DEV].s32SingleIndex];
					index = PRV_GetVoChnIndex(chn);
					if(index < 0)
						break;
					//printf("s_astVoDevStatDflt[SPOT_VO_DEV]ch = %d\n", ch);
					//if(ch >= PRV_CHAN_NUM)
					if(VochnInfo[index].SlaveId > 0)
					{
						PRV_InitSpotVo();
						PRV_start_pciv(chn);
						PRV_RefreshSpotOsd(chn);
					}
					else
					{
						PRV_PrevInitSpotVo(chn);
					}	
				}
#else				
				PRV_SetDevCsc(DHD0);
#endif
				
            }
                break;
				
			case MSG_ID_NTRANS_DELUSER_RSP:
			case MSG_ID_NTRANS_ONCEOVER_IND:
			{
				PRV_MSG_OverLinkReq(msg_req);
			}	
				break;
				
			case MSG_ID_PRV_MCC_DESVDEC_RSP:
			{
				PRV_MSG_MCC_DesVdecRsp(msg_req);
			}
				break;
				
			case MSG_ID_PRV_MCC_CREATEVDEC_RSP:
			{
				
				PRV_MSG_MCC_CreateVdecRsp(msg_req);	
			}			
				break;
				
			case MSG_ID_NTRANS_ADDUSER_RSP:
			case MSG_ID_NTRANS_BEGINLINK_IND:
			{
				PRV_MSG_NTRANSLinkReq(msg_req);				
			}			
				break;
			case  MSG_ID_FWK_UPDATE_PARAM_IND:
			{
				stParamUpdateNotify* pstNotify = (stParamUpdateNotify*)msg_req->para;
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------line: %d, Receive Message: MSG_ID_FWK_UPDATE_PARAM_IND, pstNotify->prm_id:%d\n", __LINE__, pstNotify->prm_id);
				if(pstNotify->prm_id == PRM_ID_PREVIEW_CFG_EX)
				{
					PRM_PREVIEW_CFG_EX stNewPreviewInfo;
					set_TimeOsd_xy();
#if defined(SN9234H1)
					Prv_Disp_OSD(HD);
#else
					Prv_Disp_OSD(DHD0);
#endif
					if (PARAM_OK == GetParameter(PRM_ID_PREVIEW_CFG_EX, NULL, &stNewPreviewInfo, sizeof(PRM_PREVIEW_CFG_EX), 0, SUPER_USER_ID, NULL))
					{
						if(stNewPreviewInfo.reserve[0]==1)
						{
							OSD_Ctl(0, 1, OSD_TIME_TYPE);
						}
					}
				}
				else if(pstNotify->prm_id == PRM_ID_LINKAGE_GROUP_CFG)
				{
					PRV_MSG_SetGroupNameCfg(msg_req);
				}
			}
				break;
			case MSG_ID_SCM_SERIALNO_CHANGE_IND:
			{
				PRV_MSG_SerialNoChange_Ind(msg_req);
			}
				break;
			case MSG_ID_PRV_ENTER_SETTIMEOSD_IND:
			{
				 OSD_Ctl(0, 0, OSD_TIME_TYPE);
			}
				break;
			case MSG_ID_PRV_EXIT_SETTIMEOSD_IND:
			{
				PRM_PREVIEW_CFG_EX stPreviewInfo;
				if (PARAM_OK == GetParameter(PRM_ID_PREVIEW_CFG_EX, NULL, &stPreviewInfo, sizeof(PRM_PREVIEW_CFG_EX), 0, SUPER_USER_ID, NULL))
				{
					if(stPreviewInfo.reserve[0]==1)
					{
						OSD_Ctl(0, 1, OSD_TIME_TYPE);
					}
				}
				
			}
				break;
			#if 0	
			case MSG_ID_PRV_RECREATEVDEC_IND:
			{
				PRV_MSG_ReCreateVdecIND(msg_req);
			}
				break;
			case MSG_ID_PRV_MCC_RECREATEVDEC_RSP:

			{
				PRV_MCC_RecreateVdecRsp(msg_req);
			}
				break;
			#endif
			case MSG_ID_PRV_RECREATEADEC_IND:
			{
				PRV_MSG_ReCreateAdecIND(msg_req);
			}
				break;
				
			case MSG_ID_PRV_MCC_RECREATEVDEC_RSP:
			{
				PRV_MCC_RecreateVdecRsp(msg_req);
			}
				break;			
			case MSG_ID_PRV_AUDIO_OUTPUTCHANGE_IND:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Receive message: MSG_ID_PRV_AUDIO_OUTPUTCHANGE_IND\n");
				HI_S32 s32Ret = 0;
				PRV_AUDIO_OUTPUTCHANGE_IND *AUDIO_OUTPUT = (PRV_AUDIO_OUTPUTCHANGE_IND *)msg_req->para;				
				
				s32Ret = PRV_GetAudioState(AUDIO_OUTPUT->chn);
				if(s32Ret == HI_FAILURE)
				{
					break;
				}
				s32Ret = PRV_AudioOutputChange(AUDIO_OUTPUT->chn);
				if(s32Ret == HI_SUCCESS )
				{
					IsChoosePlayAudio = 1;
					PreAudioChn = AUDIO_OUTPUT->chn;
				}
			}
				break;

			case MSG_ID_PRV_SET_AUDIOOUTPUT_IND:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Receive message: MSG_ID_PRV_SET_AUDIOOUTPUT_IND\n");
				Achn = -1;
				IsChoosePlayAudio = 0;
				PreAudioChn = -1;
				CurAudioChn = -1;
				PRV_SetVoPreview(msg_req);
				PRV_PlayAudio(s_VoDevCtrlDflt);
			}
				break;
			case MSG_ID_PRV_DESALLVDEC_IND:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Receive message: MSG_ID_PRV_DESALLVDEC_IND\n");				
				DESALLVDEC_Req *DesReq = (DESALLVDEC_Req *)msg_req->para;
				if(PRV_CurDecodeMode != DesReq->DecMode  || 
					s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewMode != DesReq->mode || 
					DesReq->flag == LayOut_KeyBoard || DesReq->flag == ParamUpdate_Switch)
				{
					PRV_MSG_DestroyAllVdec(DesReq->flag);
				}

			}
				break;
			case MSG_ID_PRV_MCC_DESALLVDEC_RSP:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Receive message: MSG_ID_PRV_MCC_DESALLVDEC_RSP\n");				
				int *flag = (int *)msg_req->para;
				PRV_MSG_MCC_DesAllVdecRsp(*flag);

			}
				break;
			
			case MSG_ID_PRV_CTRVDEC_IND:
			{
				PRV_MSG_CtrlVdec(msg_req);
			}
				break;

			case MSG_ID_PRV_SETVOMODE_IND:
			{
				PRV_ReSetVoMode *ReSetVoMode = (PRV_ReSetVoMode *)msg_req->para;
#if defined(SN_SLAVE_ON)
				//if(s_State_Info.bIsSlaveConfig == HI_FALSE)
				{					
					SN_SendMccMessageEx(PRV_SLAVE_1, SUPER_USER_ID, MOD_PRV, MOD_PRV, 0, 0, MSG_ID_PRV_MCC_RESETVOMODE_REQ, ReSetVoMode, sizeof(PRV_ReSetVoMode));
				}

#else
				OutPutMode = ReSetVoMode->NewVoMode;				
				if(ReSetVoMode->IsRefreshVo)
				{
					int mode = s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewMode;
					PRV_RefreshVoDevScreen(s_VoDevCtrlDflt, DISP_DOUBLE_DISP, s_astVoDevStatDflt[s_VoDevCtrlDflt].as32ChnOrder[mode]);
				}
#endif
			}
				break;
				
			case MSG_ID_PRV_MCC_RESETVOMODE_RSP:
			{
				PRV_ReSetVoMode *ReSetVoMode = (PRV_ReSetVoMode *)msg_req->para;
				OutPutMode = ReSetVoMode->NewVoMode;	
				
				if(ReSetVoMode->IsRefreshVo)
				{
					int mode = s_astVoDevStatDflt[s_VoDevCtrlDflt].enPreviewMode;
					PRV_RefreshVoDevScreen(s_VoDevCtrlDflt, DISP_DOUBLE_DISP, s_astVoDevStatDflt[s_VoDevCtrlDflt].as32ChnOrder[mode]);
				}

			}
				break;
#if defined(Hi3531)||defined(Hi3535)				
			case MSG_ID_MCC_PRV_QUERYVDEC_RSP:
			{
				PRV_MccQueryVdecRsp *Rsp = (PRV_MccQueryVdecRsp *)msg_req->para;
				g_DecodeState[Rsp->VdecChn].DecodeVideoStreamFrames = Rsp->DecodeStreamFrames;
				if(Rsp->LeftStreamFrames > 20)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Slave===VdecChn: %d, Rsp->LeftStreamFrames: %u\n", Rsp->VdecChn, Rsp->LeftStreamFrames);
					VoChnState.IsStopGetVideoData[Rsp->VdecChn] = 1;
				}
				else if(Rsp->LeftStreamFrames < 10)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Slave===VdecChn: %d Begin Get Data Again\n", Rsp->VdecChn);
					VoChnState.IsStopGetVideoData[Rsp->VdecChn] = 0;	
				}
			}
				break;
#endif				
			case MSG_ID_PRV_MCC_GETVDECVOINFO_RSP:
			{
				PlayBack_MccGetVdecVoInfoRsp *Rsp = (PlayBack_MccGetVdecVoInfoRsp *)msg_req->para;
				g_DecodeState[Rsp->VoChn].DecodeVideoStreamFrames = Rsp->DecodeStreamFrames;
				g_ChnPlayStateInfo stPlayStateInfo;
				UINT8 MaxSteamFrames = 0;
				HI_S32 s32Ret = 0, count = 0, cidx = 0, TotalSize = 0;
				
				TRACE(SCI_TRACE_NORMAL, MOD_DEC, "=====MSG_ID_PRV_MCC_GETVDECVOINFO_RSP=====Rsp->Result: %d=====u64CurPts:%lld====LeftStreamFrames:%lu\n", Rsp->Result, Rsp->u64CurPts, Rsp->LeftStreamFrames);
				if(Rsp->Result != -1)
				{
					PtsInfo[Rsp->VoChn].CurVoChnPts = Rsp->u64CurPts;
				}
				PRV_GetVoChnPlayStateInfo(Rsp->VoChn, &stPlayStateInfo);
				if(stPlayStateInfo.RealType == DEC_TYPE_NOREAL)
					MaxSteamFrames = 5;
				else
					MaxSteamFrames = 10;
				if((int)(Rsp->LeftStreamFrames) >= MaxSteamFrames)
				{
					if(VoChnState.bIsPBStat_StopWriteData[Rsp->VoChn] == 0)
					{
						TRACE(SCI_TRACE_NORMAL, MOD_DEC, "Slave===LeftStreamFrames: %d=======ChnStopWriteInd->Chn: %d=====1\n", Rsp->LeftStreamFrames, Rsp->VoChn);
						//VoChnState.IsStopGetVideoData[Rsp->VdecChn] = 1;
						Ftpc_ChnStopWrite(Rsp->VoChn, 1);					
						VoChnState.bIsPBStat_StopWriteData[Rsp->VoChn] = 1;
					}
					VoChnState.IsStopGetVideoData[Rsp->VoChn] = 1;
				}
				else
				{
					if(VoChnState.bIsPBStat_StopWriteData[Rsp->VoChn] == 1)
					{						
						s32Ret = BufferState(Rsp->VoChn + PRV_VIDEOBUFFER, &count, &TotalSize, &cidx);
						if(s32Ret == 0 && cidx <= 10)
						{
							TRACE(SCI_TRACE_NORMAL, MOD_DEC, "Slave===LeftStreamFrames: %d=======ChnStopWriteInd->Chn: %d=====0\n", Rsp->LeftStreamFrames, Rsp->VoChn);
							//VoChnState.IsStopGetVideoData[Rsp->VdecChn] = 0;	
							Ftpc_ChnStopWrite(Rsp->VoChn, 0);					
							VoChnState.bIsPBStat_StopWriteData[Rsp->VoChn] = 0;
						}
					}
					VoChnState.IsStopGetVideoData[Rsp->VoChn] = 0;
				}
			}
				break;

			case MSG_ID_FTPC_PROSBAR_REQ://web���豸�˰��豸�ط����϶����������Լ��豸�˼�ʱ�ط��϶�������
			{
				g_PlayInfo	stPlayInfo;
				PRV_GetPlayInfo(&stPlayInfo);
				Ftpc_Prosbar_Req *req = (Ftpc_Prosbar_Req *)msg_req->para;
				UINT8 chn = req->channel;
				time_t NewStartPts = 0;
				g_PRVPtsinfo stPlayPtsInfo;
				
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "=================MSG_ID_FTPC_PROSBAR_REQ===chn: %d\n", chn);
				
				if(chn <= 0)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line: %d chn=%d\n", __LINE__, chn);
					break;
				}
				else
				{
					chn = chn - 1;
				}

				if(req->time >= 100)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line:%d req->time: %lld======\n", __LINE__, req->time);
					req->time = 99;
				}
				else if(req->time < 0)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line:%d req->time: %lld======\n", __LINE__, req->time);
					req->time = 0;
				}
				PRV_GetVoChnPtsInfo(chn, &stPlayPtsInfo);
				stPlayPtsInfo.FirstVideoPts = 0;
				stPlayPtsInfo.FirstAudioPts = 0;
				PRV_SetVoChnPtsInfo(chn, &stPlayPtsInfo);
				
				if(stPlayInfo.PlayBackState == PLAY_EXIT
					|| (stPlayInfo.PlayBackState == PLAY_INSTANT && chn != stPlayInfo.InstantPbChn))
				{
					Ftpc_Prosbar_Rsp Rsp;
					Rsp.result = SN_ERR_FTPC_PBISEXIT_ERROR;
					SendMessageEx(msg_req->user, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, MSG_ID_FTPC_PROSBAR_RSP, &Rsp, sizeof(Ftpc_Prosbar_Rsp));
					break;
				}
				else if(stPlayInfo.PlayBackState == PLAY_INSTANT)
				{
					time_t QueryStartPts = PlayBack_PrmTime_To_Sec(&PtsInfo[chn].QueryStartTime);
					time_t QueryFinalPts = PlayBack_PrmTime_To_Sec(&PtsInfo[chn].QueryFinalTime);
					NewStartPts = QueryStartPts + (int)(QueryFinalPts - QueryStartPts)*(req->time)/100;
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "req->time: %lld============QueryStartPts: %ld, NewStartPts: %ld\n", req->time, QueryStartPts, NewStartPts);
					Probar_time[chn] = NewStartPts;
					PtsInfo[chn].CurShowPts = (HI_U64)Probar_time[chn]*1000000;
					PlayBack_Sec_To_PrmTime(NewStartPts, &req->starttime);
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "QueryStarTime: %d-%d-%d, %d.%d.%d\n", PtsInfo[chn].QueryStartTime.Year, PtsInfo[chn].QueryStartTime.Month, PtsInfo[chn].QueryStartTime.Day, PtsInfo[chn].QueryStartTime.Hour, PtsInfo[chn].QueryStartTime.Minute, PtsInfo[chn].QueryStartTime.Second);
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "starttime: %d-%d-%d, %d.%d.%d\n", req->starttime.Year, req->starttime.Month, req->starttime.Day, req->starttime.Hour, req->starttime.Minute, req->starttime.Second);
					SendMessageEx(msg_req->user,msg_req->source,MOD_FTPC,msg_req->xid,msg_req->thread, msg_req->msgId, req, msg_req->size);
					BufferSet(chn + 1, MAX_ARRAY_NODE); 			
				}
				else
				{
					//chn = chn - 1;//�豸�ط�ͨ������ʼֵΪ1
					NewStartPts = PlayBack_PrmTime_To_Sec(&(req->starttime));
					Probar_time[chn] = NewStartPts;
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "starttime: %d-%d-%d, %d.%d.%d, Probar_time[chn]: %ld\n", req->starttime.Year, req->starttime.Month, req->starttime.Day, req->starttime.Hour, req->starttime.Minute, req->starttime.Second, Probar_time[chn]);
					SendMessageEx(msg_req->user,msg_req->source,MOD_FTPC,msg_req->xid,msg_req->thread, msg_req->msgId, msg_req->para, msg_req->size);
					BufferSet(chn + 1, MAX_ARRAY_NODE); 			
				}
				Ftpc_ChnStopWrite(chn, 1);					
				VoChnState.bIsPBStat_StopWriteData[chn] = 1;
				BufferSet(chn + PRV_VIDEOBUFFER, MAX_ARRAY_NODE);			
				BufferSet(chn + PRV_AUDIOBUFFER, MAX_ARRAY_NODE);
				if(VochnInfo[chn].SlaveId == PRV_MASTER)
				{
					PRV_ReStarVdec(VochnInfo[chn].VdecChn);
				}
				else
				{
					PlayBack_MccProsBarReq ProsBar;
					ProsBar.SlaveId = VochnInfo[chn].SlaveId;
					ProsBar.VoChn = chn;
					SN_SendMccMessageEx(PRV_SLAVE_1, SUPER_USER_ID, MOD_PRV, MOD_PRV, 0, 0, MSG_ID_PRV_MCC_PBPROSBAR_REQ, &ProsBar, sizeof(PlayBack_MccProsBarReq));
				}	

				VoChnState.VideoDataTimeLag[chn] = 0;
				VoChnState.AudioDataTimeLag[chn] = 0;
				VoChnState.FirstHaveVideoData[chn] = 0;
				VoChnState.FirstHaveAudioData[chn] = 0;
				
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "VochnInfo[chn].VdecChn: %d, SlaveId: %d\n", VochnInfo[chn].VdecChn, VochnInfo[chn].SlaveId);
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line: %d============bIsPBStat_StopWriteData---StopWrite==1\n", __LINE__);

			}
				break;

			case MSG_ID_FTPC_SETCHN_PBSTATE_POS_REQ://web��ʱ�ط��϶�������
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "=================MSG_ID_FTPC_SETCHN_PBSTATE_POS_REQ\n");
				g_PlayInfo	stPlayInfo;
				PRV_GetPlayInfo(&stPlayInfo);
				Ftpc_SetChnPBStatePos_Req *SetChnPBPosReq = (Ftpc_SetChnPBStatePos_Req *)msg_req->para;
				UINT8 chn = SetChnPBPosReq->channel;

				if(chn <= 0)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line: %d chn=%d\n", __LINE__, chn);
					break;
				}
				else
				{
					chn = chn - 1;
				}
				
				if(stPlayInfo.PlayBackState == PLAY_INSTANT && VochnInfo[chn].bIsPBStat)
				{					 
					Ftpc_Prosbar_Req req;
					time_t QueryStartPts = PlayBack_PrmTime_To_Sec(&PtsInfo[chn].QueryStartTime);
					time_t QueryFinalPts = PlayBack_PrmTime_To_Sec(&PtsInfo[chn].QueryFinalTime);
					time_t NewStartPts = QueryStartPts + (int)(QueryFinalPts - QueryStartPts)*(SetChnPBPosReq->position)/100;
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "============QueryStartPts: %ld, NewStartPts: %ld\n", QueryStartPts, NewStartPts);
					Probar_time[chn] = NewStartPts;
					
					PtsInfo[chn].CurShowPts = (HI_U64)Probar_time[chn]*1000000;
					req.channel = SetChnPBPosReq->channel;
					req.time = SetChnPBPosReq->position;
					PlayBack_Sec_To_PrmTime(NewStartPts, &req.starttime);
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "QueryStartTime: %d-%d-%d, %d.%d.%d\n", PtsInfo[chn].QueryStartTime.Year, PtsInfo[chn].QueryStartTime.Month, PtsInfo[chn].QueryStartTime.Day, PtsInfo[chn].QueryStartTime.Hour, PtsInfo[chn].QueryStartTime.Minute, PtsInfo[chn].QueryStartTime.Second);
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "starttime: %d-%d-%d, %d.%d.%d\n", req.starttime.Year, req.starttime.Month, req.starttime.Day, req.starttime.Hour, req.starttime.Minute, req.starttime.Second);
					SendMessageEx(msg_req->user,msg_req->source,MOD_FTPC,msg_req->xid,msg_req->thread, MSG_ID_FTPC_PROSBAR_REQ, &req, sizeof(Ftpc_Prosbar_Req));
					
					Ftpc_ChnStopWrite(chn, 1);					
					VoChnState.bIsPBStat_StopWriteData[chn] = 1;
					BufferSet(chn + 1, MAX_ARRAY_NODE); 			
					BufferSet(chn + PRV_VIDEOBUFFER, MAX_ARRAY_NODE);			
					BufferSet(chn + PRV_AUDIOBUFFER, MAX_ARRAY_NODE);
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line: %d============bIsPBStat_StopWriteData---StopWrite==1\n", __LINE__);
				}
				else
				{
					Ftpc_SetChnPBStatePos_Rsp SetChnPbPosRsp;
					SetChnPbPosRsp.channel = SetChnPBPosReq->channel;
					SetChnPbPosRsp.position = SetChnPBPosReq->position;
					if(stPlayInfo.PlayBackState == PLAY_EXIT)
						SetChnPbPosRsp.PbState = 0;
					else
						SetChnPbPosRsp.PbState = 2;
					SendMessageEx(msg_req->user,msg_req->source,MOD_FWK,msg_req->xid,msg_req->thread, MSG_ID_FTPC_GETCHN_PBSTATE_POS_RSP, &SetChnPbPosRsp, sizeof(Ftpc_SetChnPBStatePos_Rsp));
				}

			}
				break;
			
				
			case MSG_ID_PRV_CHNENTERPB_IND:
			{
				Prv_Chn_EnterPB_Ind *ChnEnterPB = (Prv_Chn_EnterPB_Ind *)msg_req->para;
				UINT8 chn = ChnEnterPB->Chn;

				if(chn <= 0)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line: %d chn=%d\n", __LINE__, chn);
					break;
				}
				else
				{
					chn = chn - 1;
				}
				
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Chn: %d============MSG_ID_PRV_CHNENTERPB_IND========\n", ChnEnterPB->Chn);

				PRV_VoChnStateInit(chn);
				VochnInfo[chn].bIsPBStat = 1;
#if defined(SN_SLAVE_ON)
				SN_SendMccMessageEx(PRV_SLAVE_1, SUPER_USER_ID, MOD_PRV, MOD_PRV, 0, 0, MSG_ID_PRV_MCC_PBCHNENTER_IND, ChnEnterPB, sizeof(Prv_Chn_EnterPB_Ind));
#endif			
				g_ChnPlayStateInfo stPlayStateInfo;
				g_PlayInfo	stPlayInfo;
				PRV_GetPlayInfo(&stPlayInfo);
				PRV_GetVoChnPlayStateInfo(chn, &stPlayStateInfo);
				stPlayStateInfo.CurPlayState = DEC_STATE_NORMAL;
				stPlayStateInfo.CurSpeedState = DEC_SPEED_NORMAL;
				PRV_SetVoChnPlayStateInfo(chn, &stPlayStateInfo);
				if(stPlayInfo.PlayBackState == PLAY_EXIT)
				{
					stPlayInfo.PlayBackState = PLAY_INSTANT;
					stPlayInfo.InstantPbChn = chn;
					BufferSet(chn + 1, MAX_ARRAY_NODE); 		
					BufferSet(chn + PRV_VIDEOBUFFER, MAX_ARRAY_NODE); 		
					BufferSet(chn + PRV_AUDIOBUFFER, MAX_ARRAY_NODE);
					if(Achn >= 0)
					{
#if defined(SN9234H1)	

						CHECK(HI_MPI_AO_ClearChnBuf(0, 0));	
#else
						CHECK(HI_MPI_AO_ClearChnBuf(4, 0));	
#endif
						CHECK(HI_MPI_ADEC_ClearChnBuf(DecAdec));	
					}
					stPlayInfo.FullScreenId=PB_Full_id;
					PRV_SetPlayInfo(&stPlayInfo);
					sem_post(&sem_PBGetData);
					//sem_post(&sem_PBSendData);
					PreAudioChn = Achn;
					Achn = chn;
					CurAudioChn = chn;
				}
				sem_post(&sem_VoPtsQuery);
			}				
				break;
				
			case  MSG_ID_FTPC_END_REQ:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "===================MSG_ID_FTPC_END_REQ=================\n");				
				
#if 1
				Ftpc_End_Req *EndPlay = (Ftpc_End_Req *)msg_req->para;
				HI_S32 chn = EndPlay->channel;

				if(chn <= 0)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line: %d chn=%d\n", __LINE__, chn);
					break;
				}
				else
				{
					chn = chn - 1;
				}
				
				if(EndPlay->PlayType == 2)
				{
					BufferSet(chn + 1, MAX_ARRAY_NODE); 			
					PlayBack_Stop(chn);					
					PRV_PtsInfoInit(chn);
					PRV_InitVochnInfo(chn);
					PRV_VoChnStateInit(chn);
					VochnInfo[chn].bIsPBStat = 1;
				}				
				BufferSet(chn + PRV_VIDEOBUFFER, MAX_ARRAY_NODE);			
				BufferSet(chn + PRV_AUDIOBUFFER, MAX_ARRAY_NODE);
#endif
			}
				break;
				
			case  MSG_ID_FTPC_QUERYFILE_REQ:
			{
				Ftpc_QueryFile_Req *QueFileReq = (Ftpc_QueryFile_Req *)msg_req->para;
				//��ʱ�ط��£���Ҫ��ȡͨ��5���ӵ���ʼʱ�䣬���ڸ��½�����
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "==========MSG_ID_FTPC_QUERYFILE_REQ===playtype: %d\n", QueFileReq->PlayType);
				if(QueFileReq->PlayType == 1)
				{
					int i = 0;
					for(i = 0; i < DEV_CHANNEL_NUM; i++)
					{
						if(QueFileReq->RemoteChn[i] != 0)
							break;
					}
					if(i < DEV_CHANNEL_NUM)
					{
						PtsInfo[i].QueryStartTime = QueFileReq->StartTime;
						PtsInfo[i].QueryFinalTime = QueFileReq->FinalTime;
						
						TRACE(SCI_TRACE_NORMAL, MOD_PRV, "i: %d==========PtsInfo[chn].QueryStartTime: %d-%d-%d,%d.%d.%d\n", i, PtsInfo[i].QueryStartTime.Year, PtsInfo[i].QueryStartTime.Month, PtsInfo[i].QueryStartTime.Day, PtsInfo[i].QueryStartTime.Hour, PtsInfo[i].QueryStartTime.Minute, PtsInfo[i].QueryStartTime.Second);
						TRACE(SCI_TRACE_NORMAL, MOD_PRV, "i: %d==========PtsInfo[chn].QueryFinalTime: %d-%d-%d,%d.%d.%d\n", i, PtsInfo[i].QueryFinalTime.Year, PtsInfo[i].QueryFinalTime.Month, PtsInfo[i].QueryFinalTime.Day, PtsInfo[i].QueryFinalTime.Hour, PtsInfo[i].QueryFinalTime.Minute, PtsInfo[i].QueryFinalTime.Second);
					}
				}
				else
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----QueFileReq->PlaybackMode=%d\n", QueFileReq->PlaybackMode);
					if(QueFileReq->PlaybackMode == 0)
					{
						PRV_SetPreviewVoDevInMode(1);
					}
					else
					{
						PRV_SetPreviewVoDevInMode(4);
					}
					
				}
				
			}
				break;
				
			case  MSG_ID_FTPC_STOP_RSP:
			{
				Ftpc_Stop_Rsp *StopPlayRsp = (Ftpc_Stop_Rsp *)msg_req->para;
				HI_S32 i = 0, channel = 0;
				channel = StopPlayRsp->channel;
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV, source: %d============MSG_ID_FTPC_STOP_RSP: %d===%d\n", msg_req->source, StopPlayRsp->PlayType, StopPlayRsp->channel);
				g_ChnPlayStateInfo stPlayStateInfo;
				g_PlayInfo	stPlayInfo;
				PRV_GetPlayInfo(&stPlayInfo);

				if(StopPlayRsp->PlayType == 1)
				{
					if(channel <= 0)
					{
						TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line: %d chn=%d\n", __LINE__, channel);
						break;
					}
					else
					{
						channel = channel - 1;
					}
				}
				
				PRV_GetVoChnPlayStateInfo(channel, &stPlayStateInfo);
				if(StopPlayRsp->PlayType == 1)
				{
					VochnInfo[channel].bIsPBStat = 0;
					VoChnState.bIsPBStat_StopWriteData[channel] = 0;
#if defined(SN_SLAVE_ON)
					SN_SendMccMessageEx(PRV_SLAVE_1, SUPER_USER_ID, MOD_PRV, MOD_PRV, 0, 0, MSG_ID_PRV_MCC_PBCHNEXIT_IND, &channel, sizeof(HI_S32));
#endif		
				
					if(stPlayStateInfo.CurPlayState == DEC_STATE_NORMALPAUSE)
					{
#if defined(Hi3535)
						HI_MPI_VO_ResumeChn(HD, channel);
#else
						HI_MPI_VO_ChnResume(HD, channel); 
#endif
					}
						
					stPlayStateInfo.CurPlayState = DEC_STATE_EXIT;

					stPlayInfo.PlayBackState = PLAY_EXIT;
					stPlayInfo.InstantPbChn = 0;
					BufferSet(channel + 1, MAX_ARRAY_NODE);			
					BufferSet(channel + PRV_VIDEOBUFFER, MAX_ARRAY_NODE);			
					BufferSet(channel + PRV_AUDIOBUFFER, MAX_ARRAY_NODE);

					CHECK(PRV_StopAdec());
					CHECK(PRV_StartAdecAo(VochnInfo[channel]));
					IsCreateAdec = 1;
					HI_MPI_ADEC_ClearChnBuf(DecAdec);
					Achn = PreAudioChn;
					CurAudioChn = PreAudioChn;
				}
				else if(StopPlayRsp->PlayType == 2)
				{
#if defined(SN_SLAVE_ON)
					SN_SendMccMessageEx(PRV_SLAVE_1, SUPER_USER_ID, MOD_PRV, MOD_PRV, 0, 0, MSG_ID_PRV_MCC_PBSTOP_REQ, NULL, 0);
#endif		
					for(i = 0; i < stPlayInfo.ImagCount; i++)
					{
						BufferSet(i + 1, MAX_ARRAY_NODE); 			
						BufferSet(i + PRV_VIDEOBUFFER, MAX_ARRAY_NODE);			
						BufferSet(i + PRV_AUDIOBUFFER, MAX_ARRAY_NODE);
						PlayBack_Stop(i);					
						PRV_PtsInfoInit(i);
						PRV_InitVochnInfo(i);	
						PRV_VoChnStateInit(i);
						VochnInfo[i].bIsPBStat = 1;
					}
					
					if(PRV_CurIndex > 0)
					{
						for(i = 0; i < PRV_CurIndex; i++)
						{
							NTRANS_FreeMediaData(PRV_OldVideoData[i]);
							PRV_OldVideoData[i] = NULL;
						}
						PRV_CurIndex = 0;
						PRV_SendDataLen = 0;
					}
					if(stPlayInfo.IsPause && stPlayInfo.PlayBackState > PLAY_INSTANT)
						sem_post(&sem_PlayPause);
					stPlayInfo.PlayBackState = PLAY_STOP;
					stPlayInfo.IsPause = 0;
#if defined(Hi3531)		
					PlayBack_StopAdec(4, AOCHN, ADECHN);
#else
					PlayBack_StopAdec(0, AOCHN, ADECHN);
#endif
				}
				PRV_SetPlayInfo(&stPlayInfo);
				PRV_SetVoChnPlayStateInfo(channel, &stPlayStateInfo);
			}
				break;
				
			case MSG_ID_FTPC_CLOSE_RSP:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "============MSG_ID_FTPC_CLOSE_RSP==========\n");
				#if defined(Hi3531)
					AUDIO_DEV AoDev = 4;
				#else
					AUDIO_DEV AoDev = 0;
				#endif
				g_PlayInfo	stPlayInfo;
				PRV_GetPlayInfo(&stPlayInfo);
				UINT8 i = 0;
				if(stPlayInfo.PlayBackState > PLAY_INSTANT)
				{
#if defined(SN_SLAVE_ON)
					SN_SendMccMessageEx(PRV_SLAVE_1, SUPER_USER_ID, MOD_PRV, MOD_PRV, 0, 0, MSG_ID_PRV_MCC_PBSTOP_REQ, NULL, 0);
#endif		
					for(i = 0; i < stPlayInfo.ImagCount; i++)
					{
						BufferSet(i + 1, MAX_ARRAY_NODE); 			
						BufferSet(i + PRV_VIDEOBUFFER, MAX_ARRAY_NODE);			
						BufferSet(i + PRV_AUDIOBUFFER, MAX_ARRAY_NODE);
						PlayBack_Stop(i);						
						PRV_PtsInfoInit(i);
						PRV_InitVochnInfo(i);
						PRV_VoChnStateInit(i);
					}
				}
				PlayBack_StopAdec(AoDev, AOCHN, ADECHN);

				if(stPlayInfo.IsPause && stPlayInfo.PlayBackState > PLAY_INSTANT)
					sem_post(&sem_PlayPause);
				PRV_PBPlayInfoInit();
				PB_Full_id = 0;
#if defined(SN9234H1)
				PRV_MccPBCtlReq Mcc_req;
	            Mcc_req.FullScreenId=PB_Full_id;
				Mcc_req.flag=3;
				SN_SendMccMessageEx(PRV_SLAVE_1, SUPER_USER_ID, MOD_PRV, MOD_PRV, msg_req->xid, msg_req->thread, MSG_ID_PRV_MCC_PBFULLSCREEN_REQ, &Mcc_req, sizeof(PRV_MccPBCtlReq));	
#endif
			}
				break;
				
			case MSG_ID_PRV_PREPLAYBACK_IND:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "source: %d============MSG_ID_PRV_PREPLAYBACK_IND========\n", msg_req->source);
				#if defined(Hi3535)
					AUDIO_DEV AoDev = 0;
				#elif defined(Hi3531)
					AUDIO_DEV AoDev = 4;
				#endif
				PRV_PrePlayBack_Ind *PrePlayBack = (PRV_PrePlayBack_Ind *)msg_req->para;
				HI_S32 i = 0, s32ChnCount = 1;
				switch(PrePlayBack->PlaybackMode)
				{
					case PB_SingleScene:
						s32ChnCount = 1;
						break;
					case PB_FourScene:
						s32ChnCount = 4;
						break;
					case PB_NineScene:
						s32ChnCount = 9;
						break;
					case PB_SixteenScene:
						s32ChnCount = 16;						
						break;
					default:
						s32ChnCount = 1;						
						break;
				}
				g_PlayInfo PlayState;
				SN_MEMSET(&PlayState, 0, sizeof(g_PlayInfo));
				PlayState.FullScreenId = PB_Full_id;
				PlayState.bISDB=0;
				PlayState.DBClickChn=0;
				PlayState.IsZoom=0;
				PlayState.ImagCount = s32ChnCount;
				PlayState.IsSingle = (s32ChnCount == 1) ? 1 : 0;
				PlayState.PlayBackState = PLAY_ENTER;
				PlayState.IsPlaySound = 1;
				MMI_GetReplaySize(&PlayState.SubWidth, &PlayState.SubHeight);	
				int u32Width, u32Height;
				PlayBack_GetPlaySize((HI_U32 *)&u32Width, (HI_U32 *)&u32Height);
				PRV_SetPlayInfo(&PlayState);
				for(i = 0; i < CHANNEL_NUM; i++)
					VochnInfo[i].bIsPBStat = 1;
#if defined(SN_SLAVE_ON)
				PlayBack_MccOpenReq MccOpenReq;
				MccOpenReq.SlaveId = PRV_SLAVE_1;
				MccOpenReq.IsSingle = PlayState.IsSingle;
				MccOpenReq.ImageCount = s32ChnCount;
				MccOpenReq.subwidth  = PlayState.SubWidth;
				MccOpenReq.subheight = PlayState.SubHeight;
				MccOpenReq.StreamChnIDs = MasterToSlaveChnId;
				SN_SendMccMessageEx(PRV_SLAVE_1, SUPER_USER_ID, MOD_PRV, MOD_PRV, msg_req->xid, msg_req->thread, MSG_ID_PRV_MCC_PBOPEN_REQ, &MccOpenReq, sizeof(MccOpenReq));
#endif
				sem_post(&sem_PBGetData);

#if defined(Hi3531)||defined(Hi3535)
				Achn = ADECHN;
				PlayBack_StartVo();
				PlayBack_StartAdec(AoDev, AOCHN, ADECHN, PT_G711A);
				if(PlayState.IsSingle==1)
					Playback_VoDevSingle(s_VoDevCtrlDflt,0);
				else
				    Playback_VoDevMul(s_VoDevCtrlDflt,s32ChnCount);
#endif
				PRV_GetPlayInfo(&PlayState);
			}

				break;
				
			case MSG_ID_FTPC_PLAY_REQ:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "=======================MSG_ID_FTPC_PLAY_REQ\n");
				PlayBack_Pro_PlayReq(msg_req);
			}
				break;
		
			case MSG_ID_PRV_PAUSE_REQ:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "=======================MSG_ID_PRV_PAUSE_REQ\n");

				PlayBack_Pro_PauseReq(msg_req);
			}
				break;
				
			case MSG_ID_PRV_FORWARDFAST_REQ:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "===============MSG_ID_PRV_FORWARDFAST_REQ\n");
				PlayBack_ForwardFast_Req *Req = (PlayBack_ForwardFast_Req *)msg_req->para;
				PlayBack_ForwardFast_Req fastReq;
				PlayBack_ForwardFast_Rsp Rsp;
				g_PlayInfo stPlayInfo;
				g_ChnPlayStateInfo stPlayStateInfo;
				g_ChnPlayStateInfo playStatTmp;
				SN_MEMSET(&playStatTmp, 0, sizeof(playStatTmp));
				PRV_GetPlayInfo(&stPlayInfo);
				//�����豸�ط����϶�����������ʧ��
				if(stPlayInfo.PlayBackState <= PLAY_INSTANT)
				{
					Rsp.result = SN_ERR_FTPC_PBISEXIT_ERROR;
					SN_SendMessageEx(SUPER_USER_ID, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, MSG_ID_PRV_FORWARDFAST_RSP, &Rsp, sizeof(Rsp));					
					break;					
				}
				int i = 0;//, VoChn = (int)Req->channel;
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "===============VoChn==0=====ch=%d==========Req->speedstate: %d= ImagCount=%d\n", (int)Req->channel, Req->speedstate, stPlayInfo.ImagCount);
				//if(VoChn == 0)
				{
					for(i = 0; i < stPlayInfo.ImagCount; i++)
					{
						PRV_GetVoChnPlayStateInfo(i, &stPlayStateInfo);
						
						if(stPlayStateInfo.CurSpeedState == DEC_SPEED_NORMALFAST8)
							stPlayStateInfo.CurSpeedState = DEC_SPEED_NORMALFAST8;
						else
							stPlayStateInfo.CurSpeedState = stPlayStateInfo.CurSpeedState + 1;

						if(stPlayStateInfo.CurPlayState == DEC_STATE_NORMAL)
						{
							SN_MEMCPY(&playStatTmp, sizeof(playStatTmp),
								&stPlayStateInfo, sizeof(stPlayStateInfo),
								sizeof(stPlayStateInfo));
						}
						PRV_SetVoChnPlayStateInfo(i, &stPlayStateInfo);
					}
				}
				#if 0
				else
				{
					PRV_GetVoChnPlayStateInfo(VoChn, &stPlayStateInfo);
					if(stPlayStateInfo.CurSpeedState == DEC_SPEED_NORMALFAST8)
						stPlayStateInfo.CurSpeedState = DEC_SPEED_NORMAL;
					else
						stPlayStateInfo.CurSpeedState = stPlayStateInfo.CurSpeedState+1;
					PRV_SetVoChnPlayStateInfo(VoChn, &stPlayStateInfo);
				}
				#endif
				Rsp.result = 0;
				Rsp.channel = Req->channel;

				if(playStatTmp.CurPlayState == DEC_STATE_NORMAL)
				{
					Rsp.playstate = playStatTmp.CurPlayState;
					Rsp.speedstate = playStatTmp.CurSpeedState;	
				}
				else
				{
					Rsp.playstate = stPlayStateInfo.CurPlayState;
					Rsp.speedstate = stPlayStateInfo.CurSpeedState;	
				}

				PlayBack_AdaptRealType();
				fastReq.channel = Rsp.channel;
				fastReq.speedstate = Rsp.speedstate;

				SN_SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_FTPC, msg_req->xid, msg_req->thread, MSG_ID_PRV_FORWARDFAST_REQ, &fastReq, sizeof(PlayBack_ForwardFast_Req));					
				SN_SendMessageEx(SUPER_USER_ID, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, MSG_ID_PRV_FORWARDFAST_RSP, &Rsp, sizeof(Rsp));					
			}
				break;
				
			case MSG_ID_PRV_FORWARDSLOW_REQ:
			{
				PlayBack_ForwardSlow_Req *Req = (PlayBack_ForwardSlow_Req *)msg_req->para;
				PlayBack_ForwardSlow_Req slowReq;
				PlayBack_ForwardSlow_Rsp Rsp;
				int i = 0;//, VoChn = Req->channel;
				g_PlayInfo stPlayInfo;
				g_ChnPlayStateInfo stPlayStateInfo;
				g_ChnPlayStateInfo playStatTmp;
				SN_MEMSET(&playStatTmp, 0, sizeof(playStatTmp));
				PRV_GetPlayInfo(&stPlayInfo);
				//�����豸�ط����϶�����������ʧ��
				if(stPlayInfo.PlayBackState <= PLAY_INSTANT)
				{
					Rsp.result = SN_ERR_FTPC_PBISEXIT_ERROR;
					SN_SendMessageEx(SUPER_USER_ID, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, MSG_ID_PRV_FORWARDSLOW_REQ, &Rsp, sizeof(Rsp));					
					break;					
				}
				//if(VoChn == 0)
				{
					for(i = 0; i < stPlayInfo.ImagCount; i++)
					{
						PRV_GetVoChnPlayStateInfo(i, &stPlayStateInfo);
						if(stPlayStateInfo.CurSpeedState == DEC_SPEED_NORMALSLOW8)
							stPlayStateInfo.CurSpeedState = DEC_SPEED_NORMALSLOW8;
						else
							stPlayStateInfo.CurSpeedState = stPlayStateInfo.CurSpeedState - 1;

						if(stPlayStateInfo.CurPlayState == DEC_STATE_NORMAL)
						{
							SN_MEMCPY(&playStatTmp, sizeof(playStatTmp),
								&stPlayStateInfo, sizeof(stPlayStateInfo),
								sizeof(stPlayStateInfo));
						}
						
						PRV_SetVoChnPlayStateInfo(i, &stPlayStateInfo);
					}
				}
				#if 0
				else
				{
					PRV_GetVoChnPlayStateInfo(VoChn, &stPlayStateInfo);
					if(stPlayStateInfo.CurSpeedState == DEC_SPEED_NORMALSLOW8)
						stPlayStateInfo.CurSpeedState = DEC_SPEED_NORMAL;
					else
						stPlayStateInfo.CurSpeedState = stPlayStateInfo.CurSpeedState-1;
					PRV_SetVoChnPlayStateInfo(VoChn, &stPlayStateInfo);
				}
				#endif
				Rsp.result = 0;
				Rsp.channel = Req->channel;

				if(playStatTmp.CurPlayState == DEC_STATE_NORMAL)
				{
					Rsp.playstate = playStatTmp.CurPlayState;
					Rsp.speedstate = playStatTmp.CurSpeedState;	
				}
				else
				{
					Rsp.playstate = stPlayStateInfo.CurPlayState;
					Rsp.speedstate = stPlayStateInfo.CurSpeedState;	
				}
					
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "===============i:%d===============stPlayStateInfo.CurSpeedState: %d= CurPlayState=%d\n",
					i, playStatTmp.CurSpeedState, playStatTmp.CurPlayState);
				PlayBack_AdaptRealType();

				slowReq.channel = Rsp.channel;
				slowReq.speedstate = Rsp.speedstate;
				
				SN_SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_FTPC, msg_req->xid, msg_req->thread, MSG_ID_PRV_FORWARDSLOW_REQ, &slowReq, sizeof(PlayBack_ForwardSlow_Req));					
				SN_SendMessageEx(SUPER_USER_ID, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, MSG_ID_PRV_FORWARDSLOW_RSP, &Rsp, sizeof(Rsp));					
			}
				break;
				
			case MSG_ID_PRV_PLAYSOUND_REQ:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "===========MSG_ID_PRV_PLAYSOUND_REQ===\n");

				PlayBack_PlaySound_Req *req;

				req = (PlayBack_PlaySound_Req *)msg_req->para;

				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "msg_req->para = %d, msg_req->thread = %d\n", req->enable, msg_req->thread);
				
				PlayBack_PlaySound_Rsp Rsp;
				g_PlayInfo	stPlayInfo;
				
				PRV_GetPlayInfo(&stPlayInfo);
				if(PRV_GetVoiceTalkState() == HI_TRUE)
				{
					Rsp.result = SN_ERR_PRV_VOICETALK_ON;
				}
				else if(stPlayInfo.PlayBackState <= PLAY_INSTANT)
				{
					Rsp.result = SN_ERR_FTPC_PBISEXIT_ERROR;
				}
				else
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "111 stPlayInfo.IsPlaySound=%d \n", stPlayInfo.IsPlaySound);

					if(msg_req->thread > 0)
					{
						stPlayInfo.IsPlaySound = req->enable;
					}
					else
					{
						if(stPlayInfo.IsPlaySound == 0)
							stPlayInfo.IsPlaySound = 1;
						else
							stPlayInfo.IsPlaySound = 0;
					}
					
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "222 stPlayInfo.IsPlaySound=%d \n", stPlayInfo.IsPlaySound);
					
					Rsp.result = stPlayInfo.IsPlaySound;
				}
				SN_SendMessageEx(SUPER_USER_ID, MOD_PRV, msg_req->source, msg_req->xid, msg_req->thread, MSG_ID_PRV_PLAYSOUND_RSP, &Rsp, sizeof(Rsp));					
				PRV_SetPlayInfo(&stPlayInfo);
			}
				break;

			case MSG_ID_PRV_ENTER_ZOOMIN_REQ:
			{
                Playback_ZoomEnter(msg_req);
			}
				break;
			case MSG_ID_PRV_SET_ZOOMINCHN_REQ:
			{
                 Playback_ZoomChn(msg_req);
				 
			}
				break;
				
			case MSG_ID_PRV_EXIT_ZOOMIN_REQ:
			{
                 Playback_ExitZoomChn(msg_req);
			}
				break;
				
			case MSG_ID_PRV_SET_ZOOMINRATIO_REQ:
			{
				Playback_MSG_ChnZoomIn(msg_req);
			}
				break;
				
			case MSG_ID_PRV_PBDBCLICK_REQ:
			{
                
				Playback_DB(msg_req);
               
			}
				break;

			
				
			case MSG_ID_PRV_FULLSCREEN_REQ:
			{
                Playback_FullScr(msg_req);
				
			}
				break;
				
			case MSG_ID_PRV_MCC_PBOPEN_RSP:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "===========MSG_ID_PRV_MCC_PBOPEN_RSP===\n");
                g_PlayInfo PlayState;
				
				PRV_GetPlayInfo(&PlayState);
				Achn = ADECHN;
				PlayBack_StartVo();
				PlayBack_StartAdec(0, AOCHN, ADECHN, PT_G711A);
				if(PlayState.IsSingle==1)
				   Playback_VoDevSingle(s_VoDevCtrlDflt,0);
				else
				   Playback_VoDevMul(s_VoDevCtrlDflt,PlayState.ImagCount);
				sem_post(&sem_PBGetData);
				//sem_post(&sem_PBSendData);
			}
				break;
				
			case MSG_ID_PRV_MCC_PBPAUSE_RSP:
			{

			}
				break;
				
            case MSG_ID_PRV_MCC_PBZOOM_RSP:
			{
		#if defined(SN9234H1)
		
                Playback_MCC_ZoomRSP(msg_req);
		#endif
			}
			break;
				
			case MSG_ID_PRV_MCC_PBDBCLICK_RSP:
			{
        #if defined(SN9234H1)
				Playback_MCC_DBRSP(msg_req);
		#endif
			}
				break;	
				
				
			case MSG_ID_PRV_MCC_PBFULLSCREEN_RSP:
			{
		#if defined(SN9234H1)
                Playback_MCC_FullScrRSP(msg_req);
		#endif
			}
				break;	
				
			case MSG_ID_PRV_MCC_PBCLEANVOCHN_RSP:
			{

			}
				break;	
								
			case MSG_ID_PRV_MCC_PBCREATEVDEC_RSP:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "============MSG_ID_PRV_MCC_PBCREATEVDEC_RSP\n");
				PlayBack_MccCreatVdec(msg_req);

			}
				break;	
				
			case MSG_ID_PRV_MCC_PBDESVDEC_RSP:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "============MSG_ID_PRV_MCC_PBDESVDEC_RSP\n");
				PlayBack_MccDestroyVdec(msg_req);
			}
				break;	
			
			case MSG_ID_PRV_MCC_PBRECREATEVDEC_RSP:
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "============MSG_ID_PRV_MCC_PBRECREATEVDEC_RSP\n");
				PlayBack_MccReCreatVdec(msg_req);

			}
				break;	
				
			case MSG_ID_PRV_MCC_PBQUERYSTATE_RSP: /* ��ѯ��Ƭͨ��״̬RSP */
			{
				PlayBack_MccQueryStateRsp *QueryRsp = (PlayBack_MccQueryStateRsp *)(msg_req->para);
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "=====================receive MSG_ID_MCC_DEC_QUERYSTATE_RSP---chn=%d",QueryRsp->VoChn);
				PlayBack_GetSlaveQueryRsp(QueryRsp);
			}
				break;
			default:
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "%s Get unknown or unused message: %#x\n", __FUNCTION__, msg_req->msgId);
				break;
		}
		pthread_mutex_unlock(&send_data_mutex);
		
		SN_FreeMessage(&msg_req);
	}

	return NULL;
}

#if defined(Hi3535)
static HI_VOID SAMPLE_COMM_VO_HdmiConvertSync(VO_INTF_SYNC_E enIntfSync,
    HI_HDMI_VIDEO_FMT_E *penVideoFmt)
{
    switch (enIntfSync)
    {
        case VO_OUTPUT_PAL:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_PAL;
            break;
        case VO_OUTPUT_NTSC:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_NTSC;
            break;
        case VO_OUTPUT_1080P24:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_1080P_24;
            break;
        case VO_OUTPUT_1080P25:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_1080P_25;
            break;
        case VO_OUTPUT_1080P30:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_1080P_30;
            break;
        case VO_OUTPUT_720P50:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_720P_50;
            break;
        case VO_OUTPUT_720P60:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_720P_60;
            break;
        case VO_OUTPUT_1080I50:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_1080i_50;
            break;
        case VO_OUTPUT_1080I60:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_1080i_60;
            break;
        case VO_OUTPUT_1080P50:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_1080P_50;
            break;
        case VO_OUTPUT_1080P60:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_1080P_60;
            break;
        case VO_OUTPUT_576P50:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_576P_50;
            break;
        case VO_OUTPUT_480P60:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_480P_60;
            break;
        case VO_OUTPUT_800x600_60:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_800X600_60;
            break;
        case VO_OUTPUT_1024x768_60:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_1024X768_60;
            break;
        case VO_OUTPUT_1280x1024_60:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_1280X1024_60;
            break;
        case VO_OUTPUT_1366x768_60:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_1366X768_60;
            break;
        case VO_OUTPUT_1440x900_60:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_1440X900_60;
            break;
        case VO_OUTPUT_1280x800_60:
            *penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_1280X800_60;
            break;
        default :
            TRACE(SCI_TRACE_NORMAL, MOD_PRV,"Unkonw VO_INTF_SYNC_E value!\n");
            break;
    }

    return;
}

HI_S32 SAMPLE_COMM_VO_HdmiStart(VO_INTF_SYNC_E enIntfSync)
{
    HI_HDMI_INIT_PARA_S stHdmiPara;
    HI_HDMI_ATTR_S      stAttr;
    HI_HDMI_VIDEO_FMT_E enVideoFmt = 0;

    SAMPLE_COMM_VO_HdmiConvertSync(enIntfSync, &enVideoFmt);

    stHdmiPara.enForceMode = HI_HDMI_FORCE_HDMI;
    stHdmiPara.pCallBackArgs = NULL;
    stHdmiPara.pfnHdmiEventCallback = NULL;
    HI_MPI_HDMI_Init(&stHdmiPara);

    HI_MPI_HDMI_Open(HI_HDMI_ID_0);

    HI_MPI_HDMI_GetAttr(HI_HDMI_ID_0, &stAttr);

    stAttr.bEnableHdmi = HI_TRUE;
    
    stAttr.bEnableVideo = HI_TRUE;
    stAttr.enVideoFmt = enVideoFmt;

    stAttr.enVidOutMode = HI_HDMI_VIDEO_MODE_YCBCR444;
    stAttr.enDeepColorMode = HI_HDMI_DEEP_COLOR_OFF;
    stAttr.bxvYCCMode = HI_FALSE;

    stAttr.bEnableAudio = HI_FALSE;
    stAttr.enSoundIntf = HI_HDMI_SND_INTERFACE_I2S;
    stAttr.bIsMultiChannel = HI_FALSE;

    stAttr.enBitDepth = HI_HDMI_BIT_DEPTH_16;

    stAttr.bEnableAviInfoFrame = HI_TRUE;
    stAttr.bEnableAudInfoFrame = HI_TRUE;
    stAttr.bEnableSpdInfoFrame = HI_FALSE;
    stAttr.bEnableMpegInfoFrame = HI_FALSE;

    stAttr.bDebugFlag = HI_FALSE;          
    stAttr.bHDCPEnable = HI_FALSE;

    stAttr.b3DEnable = HI_FALSE;
    
    HI_MPI_HDMI_SetAttr(HI_HDMI_ID_0, &stAttr);

    HI_MPI_HDMI_Start(HI_HDMI_ID_0);
    
    printf("HDMI start success.\n");
    return HI_SUCCESS;
}

HI_S32 SAMPLE_COMM_VO_HdmiStop(HI_VOID)
{
    HI_MPI_HDMI_Stop(HI_HDMI_ID_0);
    HI_MPI_HDMI_Close(HI_HDMI_ID_0);
    HI_MPI_HDMI_DeInit();

    return HI_SUCCESS;
}
#endif
	
/************************************************************************/
/* ��ʼԤ����ʾ��
                                                                     */
/************************************************************************/
HI_S32 PRV_PreviewInit(HI_VOID)
{	
#if defined(SN9234H1)
	HI_S32 i=0;
	for(i = 0; i < PRV_VO_DEV_NUM-1; i++)//HD,AD
	{
		//if(i == SPOT_VO_DEV || i == AD)
		//	continue;
		PRV_RefreshVoDevScreen(i, (SD == i) ? DISP_NOT_DOUBLE_DISP : DISP_DOUBLE_DISP, s_astVoDevStatDflt[i].as32ChnOrder[s_astVoDevStatDflt[i].enPreviewMode]);
	}
#else
	HI_S32 i=0;
	for(i = 0; i < PRV_VO_MAX_DEV; i++)//VGA-DHD0;CVBS-DSD0;DHD1�ݲ���
	{
		if(i > DHD0)
			continue;
		PRV_RefreshVoDevScreen(i, DISP_NOT_DOUBLE_DISP, s_astVoDevStatDflt[i].as32ChnOrder[s_astVoDevStatDflt[i].enPreviewMode]);
	}
#endif
	RET_SUCCESS("");
}


/************************************************************************/
/* PRVģ���ʼ��ڡ�
                                                                     */
/************************************************************************/

int Preview_Init(void)
{
	/*create thread*/
	pthread_t comtid, sendpic_id, osd_id, dec_id_1, PB_id1, PB_id2, PB_VoPtsQuery, file_id/*, dec_id, test_id*/;
	HI_S32 err = 0;
#if defined(Hi3531)||defined(Hi3535)	
	HI_S32 i = 0, s32Ret = 0;
#endif	
	MPP_VERSION_S stVersion;
	Prv_Audiobuf.length = 0;
	SN_MEMSET((char*)Prv_Audiobuf.databuf,0x80,sizeof(Prv_Audiobuf.databuf));
	CHECK_RET(HI_MPI_SYS_GetVersion(&stVersion));
	TRACE(SCI_TRACE_NORMAL, MOD_PRV, "mpp version is "TEXT_COLOR_PURPLE("%s\n"), stVersion.aVersion);
	TRACE(SCI_TRACE_NORMAL, MOD_PRV, "MOD_PRV Last Build in "TEXT_COLOR_YELLOW("%s %s\n"), __DATE__, __TIME__);
	{
		PRM_GENERAL_CFG_BASIC preview_gneral;

		//��ȡNP��ʽ
		if (PARAM_OK == GetParameter(PRM_ID_GENERAL_CFG_BASIC,NULL,&preview_gneral,sizeof(preview_gneral),1,SUPER_USER_ID,NULL))
		{
			/*NP��ʽ*/
			s_s32NPFlagDflt = (0 == preview_gneral.CVBSOutputType) ? VIDEO_ENCODING_MODE_NTSC : VIDEO_ENCODING_MODE_PAL;
			s_s32VGAResolution = preview_gneral.VGAResolution;
			//printf("s_s32VGAResolution : %d\n", s_s32VGAResolution);
			//printf("preview_gneral.VGAResolution : %d\n", preview_gneral.VGAResolution);

		}
	}
#if defined(SN9234H1)	
	PRV_Init_M240();
#endif	
	//��ͼ�β�
	Fb_clear_step1();
	//PRV_SysInit();
#if defined(SN9234H1)
	g_Max_Vo_Num = detect_video_input_num();
	//printf("##########g_Max_Vo_Num = %d##################\n",g_Max_Vo_Num);
	PRV_ViInit();


	
	PRV_VoInit();
#else	
	//g_Max_Vo_Num = detect_video_input_num();
	g_Max_Vo_Num = DEV_CHANNEL_NUM;
	//printf("##########g_Max_Vo_Num = %d##################\n",g_Max_Vo_Num);
	//PRV_ViInit();
	//����1��VPSS(17)��Ϊ���л�(��ʾ����͵��ӷŴ��������ܻ���)
	for(i = 0; i < PRV_VO_CHN_NUM + 2; i++)
		CHECK_RET(PRV_VPSS_Start(i));
	
	CHECK_RET(PRV_VPSS_Start(DetVLoss_VdecChn));
	CHECK_RET(PRV_VPSS_Start(NoConfig_VdecChn));
	
	PRV_VoInit();
	if (HI_SUCCESS != SAMPLE_COMM_VO_HdmiStart(s_astVoDevStatDflt[DHD0].stVoPubAttr.enIntfSync))
    {
        TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Start SAMPLE_COMM_VO_HdmiStart failed!\n");
        return -1;
    }
	for(i = 0; i < PRV_VO_CHN_NUM; i++)
	{
		s32Ret = PRV_VO_BindVpss(DHD0, i, i, VPSS_PRE0_CHN);
	    if (HI_SUCCESS != s32Ret)
	    {
	        TRACE(SCI_TRACE_NORMAL, MOD_PRV,"SAMPLE_COMM_VO_BindVpss failed!\n");
	        return -1;
	    }
		#if 0
		s32Ret = PRV_VO_BindVpss(DSD0, i, i, VPSS_BYPASS_CHN);
	    if (HI_SUCCESS != s32Ret)
	    {
	        TRACE(SCI_TRACE_NORMAL, MOD_PRV,"SAMPLE_COMM_VO_BindVpss failed!\n");
	        return -1;
	    }
		#endif
	}
#endif
	//PRV_ViVoBind();
#if !defined(Hi3535)	
	tw2865Init(0);
#endif
	PRV_DECInfoInit();

	//�ڸǳ�ʼ��
	OSD_Mask_init(&s_slaveVoStat);

	get_chn_param_init();
	get_OSD_param_init(&s_slaveVoStat);
	//PRV_PreviewInit();
	
	//printf("##########Preview_Init s_VoDevCtrlDflt = %d################\n",s_VoDevCtrlDflt);

	//PRV_BindHifbVo((HD==s_VoDevCtrlDflt)?AD:HD, G1);
#if defined(Hi3520)
	PRV_BindHifbVo(HD, G1);
#elif defined(Hi3535)
	PRV_BindHifbVo(DHD0, 0);
#else	
	PRV_BindHifbVo(DHD0, GRAPHICS_LAYER_G4);
#endif	
	//PRV_BindHifbVo(AD, G4);
	//SetSlaveTime();

	if(MAX_IPC_CHNNUM > 0)
		PRV_CreateVdecChn(JPEGENC, NOVIDEO_VDECHEIGHT, NOVIDEO_VDECWIDTH, RefFrameNum, DetVLoss_VdecChn);//��������ͨ�����"��������Ƶ"ͼƬ

	PRV_CreateVdecChn(JPEGENC, NOVIDEO_VDECHEIGHT, NOVIDEO_VDECWIDTH, RefFrameNum, NoConfig_VdecChn);//��������ͨ�����"δ����"ͼƬ

	PRV_PreviewInit();
	SN_MEMSET(&g_DecodeState, 0, PRV_VO_CHN_NUM * sizeof(PRV_DecodeState));
#if defined(SN_SLAVE_ON)
	PRV_InitHostToSlaveStream();
#endif
	err = sem_init(&OSD_time_Sem, 0, 0);
	if(err != 0)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Error in sem_init OSD_time_Sem\n");
		return -1;
	}
	err = sem_init(&sem_SendNoVideoPic, 0, 0);
	if(err != 0)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Error in sem_init sem_SendNoVideoPic\n");
		return -1;
	}
	err = sem_init(&sem_VoPtsQuery, 0, 0);
	if(err != 0)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Error in sem_init sem_VoPtsQuery\n");
		return -1;
	}
	err = sem_init(&sem_PlayPause, 0, 0);
	if(err != 0)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Error in sem_init sem_PlayPause\n");
		return -1;
	}
	
	err = sem_init(&sem_PrvGetData, 0, 0);
	if(err != 0)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Error in sem_init sem_PrvGetData\n");
		return -1;
	}
	err = sem_init(&sem_PrvSendData, 0, 0);
	if(err != 0)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Error in sem_init sem_PrvSendData\n");
		return -1;
	}
	
	err = sem_init(&sem_PBGetData, 0, 0);
	if(err != 0)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Error in sem_init sem_NoPBState\n");
		return -1;
	}
	
	err = sem_init(&sem_PBSendData, 0, 0);
	if(err != 0)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Error in sem_init sem_PBSendData\n");
		return -1;
	}

	err = pthread_create(&osd_id, 0, Set_OSD_TimeProc,NULL);
	if(err != 0) /* create thread fail if returned not 0 */
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV, "preview Set_OSD_TimeProc thread create fail\n");
		return -1; /*can not create*/
	}
	if(MAX_IPC_CHNNUM > 0)
	{
		err = pthread_create(&sendpic_id, 0, SendNvrNoVideoPicThread,NULL);
		if(err != 0) /* create thread fail if returned not 0 */
		{
			TRACE(SCI_TRACE_HIGH, MOD_PRV, "preview SendNvrNoVideoPicThread thread create fail\n");
			return -1; /*can not create*/
		}
	}

	/*��client��ȡ����*/
	if(0 != pthread_create(&dec_id_1, NULL, PRV_GetPrvDataThread, NULL))
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV, "preview PRV_GetPrvDataThread thread create fail\n");
		return -1; 
	}
	/*
	if(0 != pthread_create(&dec_id_2, NULL, PRV_SendPrvDataThread, NULL))
	//{
	//	TRACE(SCI_TRACE_HIGH, MOD_PRV, "preview PRV_SendPrvDataThread thread create fail\n");
	//	return -1; 
	}
	*/
	if(0 != pthread_create(&PB_id1, NULL, PRV_GetPBDataThread, NULL))
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV, "preview PRV_GetPBDataThread thread create fail\n");
		return -1; 
	}

	/*��˼��������֡����*/
	if(0 != pthread_create(&PB_id2, NULL, PRV_SendDataThread, NULL))
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV, "preview PRV_SendPBDataThread thread create fail\n");
		return -1; 
	}
	if(0 != pthread_create(&PB_VoPtsQuery, NULL, PlayBack_VdecThread, NULL))
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV, "preview PlayBack_VdecThread thread create fail\n");
		return -1; 
	}
	
	err = pthread_create(&comtid, 0, PRV_ParseMsgProc, NULL);
	if(err != 0) /* create thread fail if returned not 0 */
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV, "preview PRV_ParseMsgProc thread create fail\n");
		return -1; /*can not create*/
	}

	err = pthread_create(&file_id, 0, PRV_FileThread, NULL);
	if(err != 0) /* create thread fail if returned not 0 */
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV, "preview PRV_TestThread thread create fail\n");
		return -1; /*can not create*/
	}
	
	#if 0
	err = pthread_create(&test_id, 0, PRV_TestThread, NULL);
	if(err != 0) /* create thread fail if returned not 0 */
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV, "preview PRV_TestThread thread create fail\n");
		return -1; /*can not create*/
	}
	#endif

//	err = pthread_create(&comtid, NULL, VOA_ParseMsgProc, NULL);
	
//	atexit(exit_mpp_sys);
	//printf("-------------Preview OK!------------------\n");
	return OK;/*init success*/	
}

#if defined(USE_UI_OSD)
void PRV_Refresh_UiOsd()
{
	Prv_Disp_OSD(DHD0);
	return;
}
#endif

int Prv_Query_NP_Suc(void)
{
	if(s_State_Info.bIsNpfinish)
	{
		return s_s32NPFlagDflt;
	}
	return HI_FAILURE;
}

//---------------------------------------------------
//---------------------------------------------------
//---------------------------------------------------
#if 0
//�����л���ز����߳�
static char PRV_getch_t(void)
{
	int n = 1;
	unsigned char ch;
	struct timeval tv;
	fd_set rfds;

	FD_ZERO(&rfds);
	FD_SET(0, &rfds);
	tv.tv_sec = 10;
	tv.tv_usec = 0;
	n = select(1, &rfds, NULL, NULL, &tv);
	if (n > 0) {
			n = read(0, &ch, 1);
			if (n == 1)
				return ch;

			return n;
	}
	return -1;
}

void *PRV_TestThread(void *param)
{
	SCM_StopSwitch	stStopSwitch;
	Enter_chn_ctrl_Req StEnterReq;
	Layout_crtl_Req LayoutReq;
	Exit_chn_ctrl_Req StExitReq;
	printf("=-------=-==-=--==--==--==Begin PRV_TestThread\n");
	sleep(20);
	while(1)
	{	
		printf("=-------=-==-=--==--==--==Begin PRV_TestThread=============TEST\n");
		#if 0
		//���뱨������
		SN_MEMSET(&stStopSwitch, 0, sizeof(stStopSwitch));
		stStopSwitch.AlmInChn = 0;
		stStopSwitch.chn = 1;
		stStopSwitch.SerialNo = 0;
		SendMessageEx(SUPER_USER_ID, MOD_ALM, MOD_SCM, 0, 0, MSG_ID_SCM_POPALM_IND, &stStopSwitch, sizeof(SCM_StopSwitch));
		usleep(400 * 1000);
		//�л����Ż���
		printf("=------PRV_TestThread=============TEST: MSG_ID_PRV_LAYOUT_CTRL_REQ to NineScene\n");
		SN_MEMSET(&LayoutReq, 0, sizeof(LayoutReq));
		LayoutReq.mode = NineScene;
		LayoutReq.chn = 0;
		LayoutReq.flag = SwitchDecode;
		SendMessageEx(SUPER_USER_ID, MOD_SCM, MOD_PRV, 0, 0, MSG_ID_PRV_LAYOUT_CTRL_REQ, &LayoutReq, sizeof(Layout_crtl_Req));			
		usleep(300 * 1000);

		//�Ż�����˫��
		printf("=------PRV_TestThread=============TEST: MSG_ID_PRV_ENTER_CHN_CTRL_REQ---DoubleClick\n");
		SN_MEMSET(&StEnterReq, 0, sizeof(StEnterReq));
		StEnterReq.chn = 0;
		StEnterReq.flag = 6;
		StEnterReq.mouse_pos.x = 500;
		StEnterReq.mouse_pos.y = 150;
		SendMessageEx(SUPER_USER_ID, MOD_ALM, MOD_PRV, 0, 0, MSG_ID_PRV_ENTER_CHN_CTRL_REQ, &StEnterReq, sizeof(StEnterReq));
		usleep(300 * 1000);	

		//��������
		printf("=------PRV_TestThread=============TEST: MSG_ID_SCM_POPALM_IND\n");
		SN_MEMSET(&stStopSwitch, 0, sizeof(stStopSwitch));
		stStopSwitch.AlmInChn = 0;
		stStopSwitch.chn = 3;
		stStopSwitch.SerialNo = 0;
		SendMessageEx(SUPER_USER_ID, MOD_ALM, MOD_SCM, 0, 0, MSG_ID_SCM_POPALM_IND, &stStopSwitch, sizeof(SCM_StopSwitch));
		usleep(300 * 1000);

		//����������˫��
		printf("=------PRV_TestThread=============TEST: MSG_ID_PRV_ENTER_CHN_CTRL_REQ---PopAlarm DoubleClick\n");
		SN_MEMSET(&StEnterReq, 0, sizeof(StEnterReq));
		StEnterReq.chn = 0;
		StEnterReq.flag = 6;
		StEnterReq.mouse_pos.x = 500;
		StEnterReq.mouse_pos.y = 150;
		SendMessageEx(SUPER_USER_ID, MOD_ALM, MOD_PRV, 0, 0, MSG_ID_PRV_ENTER_CHN_CTRL_REQ, &StEnterReq, sizeof(StEnterReq));
		usleep(300 * 1000);

		//�˳�����
		printf("=------PRV_TestThread=============TEST: MSG_ID_PRV_EXIT_CHN_CTRL_REQ---Exit PopAlarm\n");
		SN_MEMSET(&StExitReq, 0, sizeof(StExitReq));
		StExitReq.flag = 5;
		SendMessageEx(SUPER_USER_ID, MOD_ALM, MOD_PRV, 0, 0, MSG_ID_PRV_EXIT_CHN_CTRL_REQ, &StExitReq, sizeof(StExitReq));
		usleep(300 * 1000);
		#endif
		
		//�л����Ż���
		//printf("=------PRV_TestThread=============TEST: MSG_ID_PRV_EXIT_CHN_CTRL_REQ---Layout To NineScene\n");
		SN_MEMSET(&LayoutReq, 0, sizeof(LayoutReq));
		LayoutReq.mode = NineScene;
		LayoutReq.chn = 0;
		LayoutReq.flag = SwitchDecode;
		SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_SCM, 0, 0, MSG_ID_PRV_LAYOUT_CTRL_REQ, &LayoutReq, sizeof(Layout_crtl_Req));			
		usleep(3000 * 1000);
		
		SN_MEMSET(&LayoutReq, 0, sizeof(LayoutReq));
		LayoutReq.mode = SixteenScene;
		LayoutReq.chn = 0;
		LayoutReq.flag = SwitchDecode;
		SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_SCM, 0, 0, MSG_ID_PRV_LAYOUT_CTRL_REQ, &LayoutReq, sizeof(Layout_crtl_Req));			
		usleep(3000 * 1000);

		//�л���������
		//printf("=------PRV_TestThread=============TEST: MSG_ID_PRV_EXIT_CHN_CTRL_REQ---Layout To NineScene\n");
		SN_MEMSET(&LayoutReq, 0, sizeof(LayoutReq));
		LayoutReq.mode = SingleScene;
		LayoutReq.chn = 0;
		LayoutReq.flag = SwitchDecode;
		SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_SCM, 0, 0, MSG_ID_PRV_LAYOUT_CTRL_REQ, &LayoutReq, sizeof(Layout_crtl_Req));			
		usleep(3000 * 1000);
		
		//�л���ʮ������
		//printf("=------PRV_TestThread=============TEST: MSG_ID_PRV_EXIT_CHN_CTRL_REQ---Layout To SixteenScene\n");
		SN_MEMSET(&LayoutReq, 0, sizeof(LayoutReq));
		LayoutReq.mode = SixteenScene;
		LayoutReq.chn = 0;
		LayoutReq.flag = SwitchDecode;
		SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_SCM, 0, 0, MSG_ID_PRV_LAYOUT_CTRL_REQ, &LayoutReq, sizeof(Layout_crtl_Req));			
		usleep(3000 * 1000);

		SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_SCM, 0, 0, MSG_ID_PRV_LAYOUT_CTRL_REQ, &LayoutReq, sizeof(Layout_crtl_Req));			
		sleep(3);
	}
}
#endif

//��������
sem_t sem_TestSendData;

void PRV_TestInitVochnInfo(VI_CHN chn)
{
	int i = 0;
	if(chn < 0 || chn > DEV_CHANNEL_NUM)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "---------invalid channel!!line: %d\n", chn, __LINE__);
		return;
	}
	if(chn < LOCALVEDIONUM)
	{
		VochnInfo[chn].IsLocalVideo = 1;
		VochnInfo[chn].VdecChn = -1;
		if(chn >= DEV_CHANNEL_NUM/PRV_CHIP_NUM)
			VochnInfo[chn].SlaveId = 1;
		else
			VochnInfo[chn].SlaveId = 0;			
	}
	else if(chn >= LOCALVEDIONUM && chn < DEV_CHANNEL_NUM/PRV_CHIP_NUM)
	{
		VochnInfo[chn].IsLocalVideo = 0;
		VochnInfo[chn].VdecChn = DetVLoss_VdecChn;			
		VochnInfo[chn].SlaveId = PRV_MASTER;
	}
	else
	{
		#if defined(SN9234H1)		
		VochnInfo[chn].IsLocalVideo = 0;
		VochnInfo[chn].VdecChn = DetVLoss_VdecChn;			
		VochnInfo[chn].SlaveId = PRV_MASTER;
		#endif
	}

	VochnInfo[chn].VoChn = chn;
	VochnInfo[chn].CurChnIndex = VochnInfo[chn].VoChn - LOCALVEDIONUM;
	VochnInfo[chn].VideoInfo.vdoType= JPEGENC;
	VochnInfo[chn].VideoInfo.framerate = 0;
	VochnInfo[chn].VideoInfo.height = 0;
	VochnInfo[chn].VideoInfo.width = 0;
	VochnInfo[chn].AudioInfo.adoType = -1;
//	VochnInfo[chn].AudioInfo.bitwide = 0;
	VochnInfo[chn].AudioInfo.samrate = 0;
	VochnInfo[chn].AudioInfo.soundchannel = 0;
	VochnInfo[chn].IsHaveVdec = 0;
	VochnInfo[chn].IsConnect = 0;
#if defined(SN9234H1)
	for(i = 0; i < PRV_VO_DEV_NUM; i++)
		VochnInfo[chn].IsBindVdec[i] = -1;
#else	
	for(i = 0; i < PRV_VO_MAX_DEV; i++)
		VochnInfo[chn].IsBindVdec[i] = -1;
#endif		
	VochnInfo[chn].PrvType = 0;		
	VochnInfo[chn].bIsStopGetVideoData = 0;
	VochnInfo[chn].VdecCap = 0;
	VochnInfo[chn].bIsWaitIFrame = 0;
	VochnInfo[chn].bIsWaitGetIFrame = 0;
	

}

STATIC void* PRV_TestNVR_VLossDet()
{
	//printf("------NVR VLossDetProc\n");

	unsigned char PicBuff1[TESTPICBUFFSIZE], PicBuff2[TESTPICBUFFSIZE];
	int dataLen1 = 0, dataLen2 = 0;
	//int s32DataLen1 = 0, s32DataLen2 = 0;
	VDEC_STREAM_S stVstream1, stVstream2;
	
	SN_MEMSET(PicBuff1, 0, TESTPICBUFFSIZE);	
	SN_MEMSET(PicBuff2, 0, TESTPICBUFFSIZE);
	
	dataLen1 = PRV_ReadNvrNoVideoPic(NVR_NOVIDEO_FILE_TEST_1, PicBuff1);
	if(dataLen1 <= 0)
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV, "--------Read Test VideoFile: %s---fail!\n", NVR_NOVIDEO_FILE_TEST_1);
		return (void*)(-1);
	}
	stVstream1.pu8Addr = PicBuff1;
	stVstream1.u32Len = dataLen1; 
	stVstream1.u64PTS = 0;
	dataLen2 = PRV_ReadNvrNoVideoPic(NVR_NOVIDEO_FILE_TEST_2, PicBuff2);
	if(dataLen2 <= 0)
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV, "--------Read Test VideoFile: %s---fail!\n", NVR_NOVIDEO_FILE_TEST_2);
		return (void*)(-1);
	}
	stVstream2.pu8Addr = PicBuff2;
	stVstream2.u32Len = dataLen2; 
	stVstream2.u64PTS = 0;
	//printf("------------dataLen1: %d\n", dataLen1);
	//printf("------------dataLen2: %d\n", dataLen2);

#if defined(SN9234H1)
	int i = 0, j = 0;

	for(i = 0; i < PRV_VO_DEV_NUM; i++)
	{
		if(i == SPOT_VO_DEV || i == AD)
			continue;
		for(j = LOCALVEDIONUM; j < PRV_CHAN_NUM; j++)
		{			
			HI_MPI_VDEC_BindOutput(DetVLoss_VdecChn, i, VochnInfo[j].VoChn);			
		}
	}
	int count = 0, s32Ret = 0;
#else
	int i = 0, count = 0, s32Ret = 0;
	for(i = LOCALVEDIONUM; i < PRV_CHAN_NUM; i++)
	{
		if(VochnInfo[i].IsBindVdec[DHD0] == -1 && VochnInfo[i].IsBindVdec[DSD0] == -1)
		{
			if(HI_SUCCESS == PRV_VDEC_BindVpss(DetVLoss_VdecChn, VochnInfo[i].VoChn))
			{
				VochnInfo[i].IsBindVdec[DHD0] = 0;
				VochnInfo[i].IsBindVdec[DSD0] = 0;
			}
		}
	}
#endif
	//printf("------------Begin NVR Video Loss Detect!!!\n");
#if defined(SN_SLAVE_ON)
	sem_wait(&sem_TestSendData);
#endif
	while(1)
	{
		if((count % 2) == 0)
		{
			//printf("------Send Pic11111\n");
			s32Ret = HI_MPI_VDEC_SendStream(DetVLoss_VdecChn, &stVstream1, HI_IO_BLOCK);  /* ������Ƶ������ */		
			if (s32Ret != HI_SUCCESS)
			{
				//printf("send vdec chn %d stream error%#x!\n", DetVLoss_VdecChn, s32Ret);
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "send vdec chn %d stream error%#x!\n", DetVLoss_VdecChn, s32Ret);
			}
			count++;
		}
		else
		{
			//printf("------Send Pic222222\n");
			s32Ret = HI_MPI_VDEC_SendStream(DetVLoss_VdecChn, &stVstream2, HI_IO_BLOCK);  /* ������Ƶ������ */		
			if (s32Ret != HI_SUCCESS)
			{
				//printf("send vdec chn %d stream error%#x!\n", DetVLoss_VdecChn, s32Ret);
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "send vdec chn %d stream error%#x!\n", DetVLoss_VdecChn, s32Ret);
			}
			count = 0;
		}		
		usleep(500 * 1000);
	}

	return NULL;
}
#if defined(SN9234H1)	
STATIC void* PRV_Test_ParseMsgProc (void *param)
{

	SN_MSG *msg_req = NULL;

	int queue, ret;
	
	queue = CreatQueque(MOD_PRV);
	if (queue <= 0)
	{
		//printf("PRV_PRV:  PRV_PRV: CreateQueue Failed: queue = %d", queue);
		return NULL;
	}
	for (;;)
	{
		msg_req = SN_GetMessage(queue, MSG_GET_WAIT_ROREVER, &ret);
		if (ret < 0)
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_PRV: SN_GetMessage Failed: %#x", ret);
			continue;
		}
		if (NULL == msg_req)
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_PRV: SN_GetMessage return Null Pointer!");
			continue;
		}
		
		switch(msg_req->msgId)
		{
			case MSG_ID_PRV_MCC_PREVIEW_TEST_START_RSP:
			{
				//printf("-----------Receive Message: MSG_ID_PRV_MCC_PREVIEW_TEST_START_RSP\n");
				sem_post(&sem_TestSendData);
			}
				break;
				
			case MSG_ID_PRV_MCC_LAYOUT_CTRL_RSP: //��Ƭ�����л�����
			{
				//printf("-----------Receive Message: MSG_ID_PRV_MCC_LAYOUT_CTRL_RSP\n");
				//Msg_id_prv_Rsp rsp;
				Prv_Slave_Layout_crtl_Rsp *slave_rsp = (Prv_Slave_Layout_crtl_Rsp *)msg_req->para;
				//printf("slave_rsp->enPreviewMode: %d, slave_rsp->chn: %d\n", slave_rsp->enPreviewMode, slave_rsp->chn);

				if (slave_rsp->enPreviewMode == SingleScene)
				{
					PRV_SingleChn(DHD0, slave_rsp->chn);
					//PRV_SingleChn(SD, slave_rsp->chn);
				} 
				else
				{
					PRV_MultiChn(DHD0, slave_rsp->enPreviewMode, slave_rsp->chn);
					//PRV_MultiChn(SD, slave_rsp->enPreviewMode, slave_rsp->chn);					
					//PRV_SingleChn(SD, slave_rsp->chn);
				}
			}
				break;
				
			default:
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "%s Get unknown or unused message: %#x\n", __FUNCTION__, msg_req->msgId);
			break;
		}
		
		
		SN_FreeMessage(&msg_req);
	}
	
	return NULL;
}	
#endif
int Preview_Test_Init_Param(void)
{
	HI_S32 i=0,j=0,val=0;
	//Ԥ������
	for(i = 0;i < PRV_VO_MAX_DEV; i++)
	{
		/*����Ԥ��ͨ��˳��*/
		for (j = 0; j < SEVENINDEX; j++)
		{
			if(j < PRV_VO_CHN_NUM)
			{
				val = j;
			}
			else
			{
				val = -1;
			}
			//����Ԥ��������˳��
			s_astVoDevStatDflt[i].as32ChnOrder[SingleScene][j] = val;
			//����Ԥ��3����˳��
			s_astVoDevStatDflt[i].as32ChnOrder[ThreeScene][j] = val;
			//����Ԥ��5����˳��
			s_astVoDevStatDflt[i].as32ChnOrder[FiveScene][j] = val;
			//����Ԥ��7����˳��
			s_astVoDevStatDflt[i].as32ChnOrder[SevenScene][j] = val;
			//����Ԥ��4����˳��
			s_astVoDevStatDflt[i].as32ChnOrder[FourScene][j] = val;
			//����Ԥ��6����˳��
			s_astVoDevStatDflt[i].as32ChnOrder[SixScene][j] = val;
			//����Ԥ��8����˳��
			s_astVoDevStatDflt[i].as32ChnOrder[EightScene][j] = val;
			//����Ԥ��9����˳��
			s_astVoDevStatDflt[i].as32ChnOrder[NineScene][j] = val;
			//����Ԥ��16����˳��
			s_astVoDevStatDflt[i].as32ChnOrder[SixteenScene][j] = val;
			
			//printf("#############preview_info.ChannelReset[j] = %d#################### preview_info.PreviewMode = %d\n",s_astVoDevStatDflt[i].as32ChnOrder[NineScene][j],s_astVoDevStatDflt[i].enPreviewMode);
		}
		
	}
	return 0;
}
int Preview_Test_Init(void)
{	

#if defined(SN9234H1)
	pthread_t detVLoss_id, NVR_detVLoss_id;
	int err;
	int i = 0;
	
	IsTest = 1;
	g_Max_Vo_Num = detect_video_input_num();
	s_bIsSysInit = HI_TRUE;
	
	PRV_Init_M240();

	Fb_clear_step1();
	Fb_clear_step2();
	PRV_SysInit();
	PRV_ViInit();
	PRV_VoInit();

#else

	pthread_t NVR_detVLoss_id;
	int err = 0, s32Ret = 0, i = 0;
	IsTest = 1;
	g_Max_Vo_Num = DEV_CHANNEL_NUM;
	s_bIsSysInit = HI_TRUE;
	PRV_SysInit();
	
	Fb_clear_step1();
	Fb_clear_step2();
	
	for(i = 0; i < PRV_VO_CHN_NUM; i++)
		CHECK(PRV_VPSS_Start(i));
	PRV_VoInit();
#if defined(Hi3531)
	tw2865Init(0);
#endif
	if (HI_SUCCESS != SAMPLE_COMM_VO_HdmiStart(s_astVoDevStatDflt[DHD0].stVoPubAttr.enIntfSync))
    {
        TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Start SAMPLE_COMM_VO_HdmiStart failed!\n");
        return -1;
    }
	for(i = 0; i < DEV_CHANNEL_NUM; i++)
	{
		s32Ret = PRV_VO_BindVpss(DHD0, i, i, VPSS_PRE0_CHN);
	    if (HI_SUCCESS != s32Ret)
	    {
	        TRACE(SCI_TRACE_NORMAL, MOD_PRV,"SAMPLE_COMM_VO_BindVpss failed!\n");
	        return -1;
	    }
	}
#endif	
	Preview_Test_Init_Param();
	
	if(MAX_IPC_CHNNUM > 0)
	{
#if defined(SN9234H1)
		PRV_CreateVdecChn(JPEGENC, 576, 704, RefFrameNum, DetVLoss_VdecChn);
#else
		PRV_CreateVdecChn(JPEGENC, NOVIDEO_VDECHEIGHT*2, NOVIDEO_VDECWIDTH*2, RefFrameNum, DetVLoss_VdecChn);
#endif
	}
	for(i = 0; i < DEV_CHANNEL_NUM; i++)
	{
		PRV_TestInitVochnInfo(i);
	}
	
	PRV_PreviewInit();
	
	err = sem_init(&sem_TestSendData, 0, 0);
	if(err != 0)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Error in sem_init OSD_time_Sem\n");
		return -1;
	}


	err = pthread_create(&NVR_detVLoss_id, NULL, PRV_TestNVR_VLossDet, NULL);
	if(err != 0) 
	{	
		TRACE(SCI_TRACE_HIGH, MOD_PRV, "preview PRV_TestNVR_VLossDet thread create fail\n");
		return -1; 
	}
#if defined(SN9234H1)
	err = pthread_create(&detVLoss_id, NULL, PRV_VLossDetProc, NULL);
	if(err != 0) 
	{	
		TRACE(SCI_TRACE_HIGH, MOD_PRV, "preview PRV_VLossDetProc thread create fail\n");
		return -1; 
	}

	pthread_t prv_id;
	err = pthread_create(&prv_id, NULL, PRV_Test_ParseMsgProc, NULL);
	if(err != 0) 
	{
		perror("pthread_create: PRV_Test_ParseMsgProc");
		return -1; 
	}

	SN_SendMccMessageEx(PRV_SLAVE_1,SUPER_USER_ID, MOD_PRV, MOD_PRV, 0, 0, MSG_ID_PRV_MCC_PREVIEW_TEST_START_REQ, NULL, 0);
#endif
	
	PRV_TEST_PreviewMode(0);
	sem_post(&sem_TestSendData);
	return 0;
}

int Preview_Test_Vo_Select(unsigned char vo_type)
{	
#if defined(SN9234H1)
	PRV_VoInit();
	PRV_RefreshVoDevScreen(HD,DISP_DOUBLE_DISP,s_astVoDevStatDflt[HD].as32ChnOrder[s_astVoDevStatDflt[HD].enPreviewMode]);
	PRV_RefreshVoDevScreen(SD,DISP_NOT_DOUBLE_DISP,s_astVoDevStatDflt[HD].as32ChnOrder[s_astVoDevStatDflt[HD].enPreviewMode]);
#else
	PRV_VoInit();
	if((s_astVoDevStatDflt[DHD0].stVoPubAttr.enIntfType & VO_INTF_HDMI) == VO_INTF_HDMI)
	{
		if (HI_SUCCESS != SAMPLE_COMM_VO_HdmiStart(s_astVoDevStatDflt[DHD0].stVoPubAttr.enIntfSync))
		{
		   TRACE(SCI_TRACE_NORMAL, MOD_PRV, "TEST Start SAMPLE_COMM_VO_HdmiStart failed!\n");
		    return -1;
		}
	}
	PRV_RefreshVoDevScreen(DHD0, DISP_DOUBLE_DISP, s_astVoDevStatDflt[DHD0].as32ChnOrder[s_astVoDevStatDflt[DHD0].enPreviewMode]);
#endif
	return 0;
}

int PRV_TEST_PreviewMode(int chn)
{
	PRV_PREVIEW_MODE_E enPreviewMode = SingleScene;
	HI_U32 u32Index = 0;
	HI_S32 i = 0;

	if (chn<0 || chn > PRV_VO_CHN_NUM)
	{
		RET_FAILURE("bad input parameter: chn");
	}

	if (0 == chn)
	{
		u32Index = 0;
		enPreviewMode = PRV_VO_MAX_MOD;
	}
	else
	{
		for(i = 0; i < PRV_VO_CHN_NUM; i++)
		{
#if defined(SN9234H1)
			if (s_astVoDevStatDflt[HD].as32ChnOrder[s_astVoDevStatDflt[HD].enPreviewMode][i] == chn-1)
#else
			if (s_astVoDevStatDflt[DHD0].as32ChnOrder[s_astVoDevStatDflt[DHD0].enPreviewMode][i] == chn - 1)
#endif				
			{
				u32Index = i;
				break;
			}
		}
		if (i == PRV_VO_CHN_NUM)
		{
			RET_FAILURE("the chn is hiden!!");
		}
		enPreviewMode = SingleScene;
	}
#if defined(SN9234H1)
	PRV_PreviewVoDevInMode(HD, enPreviewMode, u32Index, s_astVoDevStatDflt[HD].as32ChnOrder[s_astVoDevStatDflt[HD].enPreviewMode]);
	//PRV_PreviewVoDevInMode(AD, enPreviewMode, u32Index, s_astVoDevStatDflt[AD].as32ChnOrder[s_astVoDevStatDflt[AD].enPreviewMode]);
#else
	PRV_PreviewVoDevInMode(DHD0, enPreviewMode, u32Index, s_astVoDevStatDflt[DHD0].as32ChnOrder[s_astVoDevStatDflt[DHD0].enPreviewMode]);
	PRV_PreviewVoDevInMode(DSD0, enPreviewMode, u32Index, s_astVoDevStatDflt[DSD0].as32ChnOrder[s_astVoDevStatDflt[DSD0].enPreviewMode]);
#endif
	RET_SUCCESS("");
}


/***************************** END HEAR *****************************/

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


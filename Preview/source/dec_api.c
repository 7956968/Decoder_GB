//
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
#include <hi_comm_vo.h>

#include "disp_api.h"
#include "dec_api.h"
#include "PlayBack_api.h"
#include "h264_stream.h"

#define PATH_STOP "/tmp/stop"
#define SIGN_MAX 	16

int MasterToSlaveChnId = 0;//����Ƭ��������ͨ��
g_PRVVoChnInfo VochnInfo[DEV_CHANNEL_NUM];//��Ƶ���ͨ����ͨ����Ϣ
g_PRVSlaveState SlaveState;
g_PRVPtsinfo PtsInfo[MAX_IPC_CHNNUM];//ÿһ������ͨ����Ƶʱ�����Ϣ
g_PRVVoChnState VoChnState;
g_ChnPlayStateInfo  PlayStateInfo[PRV_VO_CHN_NUM];
g_PlayInfo		    PlayInfo;
g_PRVSendChnInfo    SendChnInfo;

AUDIO_DATABUF Prv_Audiobuf; 

int g_PrvType = Flow_Type;
int CurSlaveCap = 0;
int CurMasterCap = 0;	//��Ƭ��ǰ��ʹ�õ�����
int CurSlaveChnCount = 0;
int CurCap = 0;//��ǰ��ʹ�õ�����CurSlaveCap + CurMasterCap

//void* PRV_PstDataAddr[MAX_IPC_CHNNUM] = {NULL};//����IPC��Ƶ���ݵ�ַ
//void* PRV_PstAudioDataAddr = NULL;//������Ƶ���ݵ�ַ
int CurIPCCount = 0;

int Achn = -1;//��Ƶ�����Ӧ��ͨ���±�
HI_U32 PtNumPerFrm = 160;//��Ƶÿ֡�����������ֵ�ı�ʱ��Ҫ���´�����Ƶ����ͨ��
int IsAdecBindAo = 0;//Ao��Adec��ʶ1:�󶨣�0:����
int IsStopGetAudio = 0;
int IsCreatingAdec = 0;
int IsCreateAdec = 0;
int IsAudioOpen = 0;//��Ƶ����/�رձ�ʶ  1:������0:�ر�

int CurCheckChn = 0;//С��MAX_IPC_CHNNUMʱ������ǰ��ѯ��Ƶbuffer���������ǰ��ѯ��Ƶbuffer
pthread_mutex_t send_data_mutex = PTHREAD_MUTEX_INITIALIZER;//�������ٽ���ͨ�������У��������ݵĻ�����

AVPacket *PRV_OldVideoData[DEV_CHANNEL_NUM] = {NULL};
VDEC_CHN PRV_OldVdec[DEV_CHANNEL_NUM] = {0};
int PRV_SendDataLen = 0;
int PRV_CurIndex = 0;
int IsUpGrade = 0;
enum PreviewMode_enum g_stPreviewMode = SingleScene;

AVPacket *tmp_PRVGetVideoData[MAX_IPC_CHNNUM] = {NULL};//���ڱ����Ƭ���������������в��ܷ��͵�I֡����
AVPacket *Pb_FirstGetVideoData[MAX_IPC_CHNNUM] = {NULL};//�ط��������л���ȡ�ĵ�һ֡����

static h264_stream_t h_stream[MAX_IPC_CHNNUM];
static int g_beginsign = 0;								//��ʾtrace�����Ƿ��ȡ¼���ļ�
static int g_filesign[DEV_CHANNEL_NUM][SIGN_MAX] = {{-1}};		//���ڱ�ʾ�Ƿ��ȡ�ļ��ı�־��[0]:��ʾ��ȡ�ջ�ȡ����Ԥ�����ݣ�[1]:��ʾ��ȡ�͵�������ǰ�����ݣ�[2]:ftpc���յ�������

extern	UINT8	PRV_CurDecodeMode;
extern int FWK_SetOverflowState (int chn, int state);


static void rtp_stream_testC(AVPacket * av_packet)
{
	AVPacket *pTmpPacket = NULL;
	AVPacket *NextPacket = NULL;
	int wlen=0;
	int src_file;
	char start_code[4] = {0x00, 0x00, 0x00, 0x01};

	pTmpPacket = av_packet;
	NextPacket = pTmpPacket->next;
	while(pTmpPacket != NULL)
	{
		//printf("\n%s Line %d:TIMKINGH is DEBUGGING!!! ====>DataSize:%d,BufOffset:%d,rtpOffset:%d\n\n",
				//__func__,__LINE__,pTmpPacket->DataSize,pTmpPacket->BufOffset,pTmpPacket->rtpOffset);
		int length = pTmpPacket->DataSize - pTmpPacket->BufOffset;		

		src_file = open("/var/tmp/stream_data_from_client.h264",O_RDWR | O_CREAT | O_APPEND);
		if(src_file > 0)
		{
			if(pTmpPacket->frame_type == 0 || pTmpPacket->frame_type == 1)
			{
				wlen = write(src_file,start_code,4);
				//printf("Line %d:add start code!!!\n",__LINE__);
			}
			wlen = write(src_file,(char*)pTmpPacket + pTmpPacket->BufOffset,length);
			//printf("%s Line %d ----> wlen:%d,pTmpPacket:%p,nalu_type:%d,frame_type:%d\n",__func__,__LINE__,
											//wlen,pTmpPacket,pTmpPacket->naluType,pTmpPacket->frame_type);
		}
		if(pTmpPacket->Extendnext == NULL)
		{
			pTmpPacket = (AVPacket *)NextPacket;
			if(pTmpPacket != NULL)
				NextPacket = pTmpPacket->next;
		}
		else
		{
			pTmpPacket = (AVPacket *)(pTmpPacket->Extendnext);
		}				
	}
	close(src_file);
}


static void rtp_stream_testD(AVPacket * av_packet)
{
	AVPacket *pTmpPacket = NULL;
	AVPacket *NextPacket = NULL;
	int wlen=0;
	int src_file;
	char start_code[4] = {0x00, 0x00, 0x00, 0x01};

	pTmpPacket = av_packet;
	NextPacket = pTmpPacket->next;
	while(pTmpPacket != NULL)
	{
		//printf("\n%s Line %d:TIMKINGH is DEBUGGING!!! ====>DataSize:%d,BufOffset:%d,rtpOffset:%d\n\n",
				//__func__,__LINE__,pTmpPacket->DataSize,pTmpPacket->BufOffset,pTmpPacket->rtpOffset);
		int length = pTmpPacket->DataSize - pTmpPacket->BufOffset;		

		src_file = open("/var/tmp/data_sent_to_decoder.h264",O_RDWR | O_CREAT | O_APPEND);
		if(src_file > 0)
		{
			if(pTmpPacket->frame_type == 0 || pTmpPacket->frame_type == 1)
			{
				wlen = write(src_file,start_code,4);
				//printf("Line %d:add start code!!!\n",__LINE__);
			}
			wlen = write(src_file,(char*)pTmpPacket + pTmpPacket->BufOffset,length);
			//printf("%s Line %d ----> wlen:%d,pTmpPacket:%p,nalu_type:%d,frame_type:%d\n",__func__,__LINE__,
											//wlen,pTmpPacket,pTmpPacket->naluType,pTmpPacket->frame_type);
		}
		if(pTmpPacket->Extendnext == NULL)
		{
			pTmpPacket = (AVPacket *)NextPacket;
			if(pTmpPacket != NULL)
				NextPacket = pTmpPacket->next;
		}
		else
		{
			pTmpPacket = (AVPacket *)(pTmpPacket->Extendnext);
		}				
	}
	close(src_file);
}


int PRV_SetBeginSign(int value)
{
	g_beginsign = value;

	return OK;
}

int PRV_GetFileSign(int ch, int pos)
{
	int sign = -1;
	
	if(ch < 0 || ch > DEV_CHANNEL_NUM || pos < 0 || pos > SIGN_MAX)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "param error, ch=%d, pos=%d!!", ch, pos);
		return sign;
	}

	sign = g_filesign[ch][pos];
	
	return sign;
}


int PRV_GetCurCap()
{
	return CurCap;
}
/********************************************************
������:PRV_WriteBuffer
��     ��:��ָ��ͨ����Ԥ����������д����
��     ��:[in]ch   ָ��ͨ����
		    [in]dataType �������ͣ���Ƶ���ݻ���Ƶ����
		   [in]DataFromRTSP   ָ������

����ֵ:  0�ɹ�
		    -1ʧ��
*********************************************************/
HI_S32 PRV_WriteBuffer(HI_S32 chn, int dataType, AVPacket *DataFromRTSP)
{
	HI_S32 s32Ret = 0, prvBufferChn = 0;
	//int offSet = sizeof(int) * 3 * DataFromRTSP->nal_units;
	if(DataFromRTSP == NULL)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "---------Invalid Pointer, NULL Pointer!!");
		return HI_FAILURE;
	}
	if(dataType == CODEC_ID_H264)
	{
		if(VochnInfo[chn].bIsPBStat)
		{
			if(!VoChnState.FirstHaveVideoData[chn] && DataFromRTSP->frame_type == 1)
			{
				BufferSet(chn + PRV_VIDEOBUFFER, MAX_ARRAY_NODE);			
				s32Ret = SetNode_Ex(chn + PRV_VIDEOBUFFER, PREVIEWARRAY);
				if(s32Ret != 0)
					TRACE(SCI_TRACE_HIGH, MOD_PRV, "--------------Set Node fail!!\n");
				VoChnState.FirstHaveVideoData[chn] = 1;
				VoChnState.VideoDataTimeLag[chn] = 0;
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "DataFromRTSP->codec_id == CODEC_ID_H264-FirstVideoPts=%llu--DataFromRTSP->pts: %llu\n", PtsInfo[chn].FirstVideoPts, DataFromRTSP->pts);
				PtsInfo[chn].FirstVideoPts = DataFromRTSP->pts;
			}
			 				
		}
		else
		{
			if(!VoChnState.FirstHaveVideoData[chn])
			{
				BufferSet(chn + PRV_VIDEOBUFFER, MAX_ARRAY_NODE);			
				s32Ret = SetNode_Ex(chn + PRV_VIDEOBUFFER, PREVIEWARRAY);
				if(s32Ret != 0)
					TRACE(SCI_TRACE_HIGH, MOD_PRV, "--------------Set Node fail!!\n");
				VoChnState.FirstHaveVideoData[chn] = 1;
			}
		}
		
		if(!VochnInfo[chn + LOCALVEDIONUM].IsHaveVdec)//��ʱ�����ǲ�������ͨ������ʼ���ͽ���ͨ��ʱ���ۼӣ�ȷ����ȡ��Vo��ʱ���һ��
		{
			PtsInfo[chn].CurSendPts  = 0;
			PtsInfo[chn].CurVoChnPts = 0;
			//PtsInfo[chn].PreGetVideoPts = 0;
		}
		VoChnState.VideoDataCount[chn]++;
			
		prvBufferChn = chn + PRV_VIDEOBUFFER;
		DataFromRTSP->prvpts = DataFromRTSP->pts * 1000000/90000;		
		//if(VochnInfo[chn].bIsPBStat)
			//printf("============PRV_WriteBuffer===DataFromRTSP->prvpts: %lld\n", DataFromRTSP->prvpts);
			
		s32Ret = BufferWrite(prvBufferChn, (char*)DataFromRTSP, DataFromRTSP->data_len + sizeof(AVPacket));
		if(0 != s32Ret)
		{				
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "------------BufferWrite failure---chn = %d, pts = %lld", chn, DataFromRTSP->pts);
			VochnInfo[chn + LOCALVEDIONUM].bIsWaitIFrame = 1;
			NTRANS_FreeMediaData(DataFromRTSP);
			return s32Ret;
		}

	}
	else if((dataType == CODEC_ID_PCMA || dataType == CODEC_ID_PCMU))
	{	
		if(VochnInfo[chn].bIsPBStat)
		{
			if(!VoChnState.FirstHaveAudioData[chn]  && VoChnState.FirstHaveVideoData[chn])
			{
				BufferSet(chn + PRV_AUDIOBUFFER, MAX_ARRAY_NODE);			
				s32Ret = SetNode_Ex(chn + PRV_AUDIOBUFFER, PREVIEWARRAY);
				
				PtsInfo[chn].PreGetAudioPts = 0;
				if(s32Ret != 0)
					TRACE(SCI_TRACE_HIGH, MOD_PRV, "--------------Set Node fail!!\n");
				VoChnState.FirstHaveAudioData[chn] = 1;
				VoChnState.AudioDataTimeLag[chn] = 0;
				
				if(PlayInfo.PlayBackState == PLAY_INSTANT)
				{
					PRV_StopAdec();
					VochnInfo[chn].AudioInfo.PtNumPerFrm = PtNumPerFrm;
					PRV_StartAdecAo(VochnInfo[chn]);	
					IsCreateAdec = 1;
					HI_MPI_ADEC_ClearChnBuf(DecAdec);
				}
				
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "DataFromRTSP->codec_id == CODEC_ID_PCMA-FirstAudioPts=%llu--DataFromRTSP->pts: %llu\n", PtsInfo[chn].FirstAudioPts, DataFromRTSP->pts);
				PtsInfo[chn].FirstAudioPts = DataFromRTSP->pts; 				
				
			}
			else if(!VoChnState.FirstHaveAudioData[chn])
			{
				NTRANS_FreeMediaData(DataFromRTSP);
				return HI_SUCCESS;
			}
			
		}
		else
		{
			if(!VoChnState.FirstHaveAudioData[chn])
			{
				BufferSet(chn + PRV_AUDIOBUFFER, MAX_ARRAY_NODE);			
				s32Ret = SetNode_Ex(chn + PRV_AUDIOBUFFER, PREVIEWARRAY);
				
				PtsInfo[chn].PreGetAudioPts = 0;
				if(s32Ret != 0)
					TRACE(SCI_TRACE_HIGH, MOD_PRV, "--------------Set Node fail!!\n");
				VoChnState.FirstHaveAudioData[chn] = 1;
			}
		}

		VoChnState.AudioDataCount[chn]++;

		if(PlayInfo.PlayBackState <= PLAY_INSTANT && Achn != chn)
		{
			NTRANS_FreeMediaData(DataFromRTSP);
			return HI_SUCCESS;
		}
		
		prvBufferChn = chn + PRV_AUDIOBUFFER;
		s32Ret = BufferWrite(prvBufferChn, (char*)DataFromRTSP, DataFromRTSP->data_len + sizeof(AVPacket));
		if(0 != s32Ret)
		{			
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "------------BufferWrite failure---chn = %d, pts = %lld", chn, DataFromRTSP->pts);
			NTRANS_FreeMediaData(DataFromRTSP);
			return s32Ret;
		}
	}
	else
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "------------Invalid DataType: %d", dataType);
		NTRANS_FreeMediaData(DataFromRTSP);
		return HI_FAILURE;
	}
		
	return HI_SUCCESS;
}
/********************************************************
������:PRV_CreateVdecChn
��     ��:��������ͨ��
��     ��:[in]EncType  ָ���������ͣ�H264��JPEG
		   [in]VdecChn    �����Ľ���ͨ����
����ֵ:  0�ɹ�
		    -1ʧ��

*********************************************************/
HI_S32 PRV_CreateVdecChn(HI_S32 EncType, HI_S32 height, HI_S32 width, HI_U32 u32RefFrameNum, VDEC_CHN VdecChn)
{
	HI_S32 s32Ret = 0;
	VDEC_CHN_ATTR_S stAttr;
	HI_U32 refframenum = 0;

	if(u32RefFrameNum < 1 || u32RefFrameNum > 4)
	{
		if((height*width) > MAX_8D1)
		{
			refframenum = 2;
		}
		else
		{
			refframenum = RefFrameNum;
		}
		
	}
	else
	{
		refframenum = u32RefFrameNum;
	}
	
	TRACE(SCI_TRACE_NORMAL, MOD_PRV, "%s line:%d VdecChn=%d, refframenum=%u", __FUNCTION__, __LINE__, VdecChn, refframenum);
	
#if defined(SN9234H1)	
	VDEC_ATTR_H264_S stH264Attr;
	VDEC_ATTR_JPEG_S stJpegAttr;
	
	stH264Attr.u32Priority = 0;	
	stJpegAttr.u32Priority = 0;
	
	stH264Attr.u32PicHeight = height;
	stH264Attr.u32PicWidth = width;
	stJpegAttr.u32PicHeight = height;
	stJpegAttr.u32PicWidth = width;
	
	stH264Attr.u32RefFrameNum = refframenum;
	stH264Attr.enMode = H264D_MODE_FRAME;

	stAttr.u32BufSize = (height * width * 3 / 2);
	stAttr.enType = PT_H264;
	stAttr.pValue = (void*)&stH264Attr;
	if(H264ENC == EncType)
	{		
		stAttr.enType = PT_H264;
		stAttr.pValue = (void*)&stH264Attr;
	}
	else if(JPEGENC == EncType)
	{
		stAttr.enType = PT_JPEG;
		stAttr.pValue = (void*)&stJpegAttr;
	}

	s32Ret = HI_MPI_VDEC_CreateChn(VdecChn, &stAttr, NULL);	/* ������Ƶ����ͨ�� */

#else
    VDEC_PRTCL_PARAM_S stPrtclParam;

	 switch (EncType)
    {
        case H264ENC:
			stAttr.stVdecVideoAttr.u32RefFrameNum = refframenum;
		    stAttr.stVdecVideoAttr.enMode = VIDEO_MODE_FRAME;
		    stAttr.stVdecVideoAttr.s32SupportBFrame = 0;
			stAttr.enType = PT_H264;
            break;
        case JPEGENC:
            stAttr.stVdecJpegAttr.enMode = VIDEO_MODE_FRAME;
  //          stAttr.stVdecJpegAttr.u32Tmp = 0;
			stAttr.enType = PT_JPEG;
            break;
       
        default:
            TRACE(SCI_TRACE_NORMAL, MOD_PRV,"err type \n");
            return HI_FAILURE;
    }
	
    stAttr.u32Priority = 250;//�˴��������0
    stAttr.u32PicWidth = width;
    stAttr.u32PicHeight = height;

	stAttr.u32BufSize = (stAttr.u32PicWidth * stAttr.u32PicHeight * 3/2);
	s32Ret = HI_MPI_VDEC_CreateChn(VdecChn, &stAttr);	// ������Ƶ����ͨ�� 
#endif	
	if(s32Ret != HI_SUCCESS && s32Ret != HI_ERR_VDEC_EXIST)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "HI_MPI_VDEC_CreateChn fail 0x%x  width=%d height=%d\n", s32Ret, width, height);		
		return s32Ret;
	}

	if(s32Ret == HI_ERR_VDEC_EXIST)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Vdec: %d Allready Create, Destroy Vdec First\n", VdecChn);
		CHECK(HI_MPI_VDEC_StopRecvStream(VdecChn));//����ͨ��ǰ��ֹͣ��������		
		CHECK(HI_MPI_VDEC_ResetChn(VdecChn)); //��������λ 
		CHECK(HI_MPI_VDEC_DestroyChn(VdecChn)); // ������Ƶͨ�� 	
#if defined(SN9234H1)
		CHECK(HI_MPI_VDEC_CreateChn(VdecChn, &stAttr, NULL)); /* ������Ƶ����ͨ�� */
#else		
		CHECK(HI_MPI_VDEC_CreateChn(VdecChn, &stAttr)); // ������Ƶ����ͨ��
#endif		
	}
	
#if defined(Hi3531)

	s32Ret = HI_MPI_VDEC_GetPrtclParam(VdecChn, &stPrtclParam);
    if (HI_SUCCESS != s32Ret)
    {
       	TRACE(SCI_TRACE_NORMAL, MOD_PRV,"HI_MPI_VDEC_GetPrtclParam failed errno 0x%x \n", s32Ret);
        return s32Ret;
    }

  //  stPrtclParam.s32MaxSpsNum = 21;
  //  stPrtclParam.s32MaxPpsNum = 22;
  //  stPrtclParam.s32MaxSliceNum = 100;
//	stPrtclParam.s32DisplayFrameNum = 2;
    s32Ret = HI_MPI_VDEC_SetPrtclParam(VdecChn, &stPrtclParam);
    if (HI_SUCCESS != s32Ret)
    {
        TRACE(SCI_TRACE_NORMAL, MOD_PRV,"HI_MPI_VDEC_SetPrtclParam failed errno 0x%x \n", s32Ret);
        return s32Ret;
    }
#elif defined(Hi3535)
	s32Ret = HI_MPI_VDEC_GetProtocolParam(VdecChn, &stPrtclParam);
    if (HI_SUCCESS != s32Ret)
    {
       	TRACE(SCI_TRACE_NORMAL, MOD_PRV,"HI_MPI_VDEC_GetPrtclParam failed errno 0x%x \n", s32Ret);
        return s32Ret;
    }

  //  stPrtclParam.s32MaxSpsNum = 21;
  //  stPrtclParam.s32MaxPpsNum = 22;
  //  stPrtclParam.s32MaxSliceNum = 100;
//	stPrtclParam.s32DisplayFrameNum = 2;
    s32Ret = HI_MPI_VDEC_SetProtocolParam(VdecChn, &stPrtclParam);
    if (HI_SUCCESS != s32Ret)
    {
        TRACE(SCI_TRACE_NORMAL, MOD_PRV,"HI_MPI_VDEC_SetPrtclParam failed errno 0x%x \n", s32Ret);
        return s32Ret;
    }
	HI_MPI_VDEC_SetDisplayMode(VdecChn, VIDEO_DISPLAY_MODE_PLAYBACK);
#endif	
	CHECK_RET(HI_MPI_VDEC_StartRecvStream(VdecChn));

	return HI_SUCCESS;

}

void PRV_GetPlayInfo(g_PlayInfo *pPlayInfo)
{
	pPlayInfo->PlayBackState = PlayInfo.PlayBackState;
	pPlayInfo->IsSingle = PlayInfo.IsSingle;
   	pPlayInfo->DBClickChn = PlayInfo.DBClickChn;
   	pPlayInfo->FullScreenId = PlayInfo.FullScreenId;
    pPlayInfo->ImagCount = PlayInfo.ImagCount;
    pPlayInfo->IsPlaySound = PlayInfo.IsPlaySound;
    pPlayInfo->IsPause = PlayInfo.IsPause;
	pPlayInfo->InstantPbChn = PlayInfo.InstantPbChn;
	pPlayInfo->SubWidth = PlayInfo.SubWidth;
	pPlayInfo->SubHeight = PlayInfo.SubHeight; 
	pPlayInfo->bISDB=PlayInfo.bISDB;
	pPlayInfo->ZoomChn=PlayInfo.ZoomChn;
	pPlayInfo->IsZoom=PlayInfo.IsZoom;
}

void PRV_SetPlayInfo(g_PlayInfo *pPlayInfo)
{
	PlayInfo.PlayBackState = pPlayInfo->PlayBackState;
	PlayInfo.IsSingle = pPlayInfo->IsSingle;
   	PlayInfo.DBClickChn = pPlayInfo->DBClickChn;
   	PlayInfo.FullScreenId = pPlayInfo->FullScreenId;
    PlayInfo.ImagCount = pPlayInfo->ImagCount;
    PlayInfo.IsPlaySound = pPlayInfo->IsPlaySound;
    PlayInfo.IsPause = pPlayInfo->IsPause;
	PlayInfo.InstantPbChn = pPlayInfo->InstantPbChn;
	PlayInfo.SubWidth = pPlayInfo->SubWidth;
	PlayInfo.SubHeight = pPlayInfo->SubHeight; 
	PlayInfo.bISDB=pPlayInfo->bISDB;
	PlayInfo.ZoomChn=pPlayInfo->ZoomChn;
	PlayInfo.IsZoom=pPlayInfo->IsZoom;

}


void PRV_GetVoChnPtsInfo(HI_S32 VoChn, g_PRVPtsinfo *pPtsInfo)
{
	pPtsInfo->PreGetVideoPts = PtsInfo[VoChn].PreGetVideoPts;
	pPtsInfo->CurGetVideoPts = PtsInfo[VoChn].CurGetVideoPts;	
	pPtsInfo->PreVideoPts = PtsInfo[VoChn].PreVideoPts;
	pPtsInfo->CurVideoPts = PtsInfo[VoChn].CurVideoPts;	
	pPtsInfo->PreSendPts = PtsInfo[VoChn].PreSendPts;
	pPtsInfo->CurSendPts = PtsInfo[VoChn].CurSendPts;
	pPtsInfo->CurVoChnPts = PtsInfo[VoChn].CurVoChnPts;
	pPtsInfo->PreVoChnPts = PtsInfo[VoChn].PreVoChnPts;	
	pPtsInfo->DevicePts = PtsInfo[VoChn].DevicePts;
	pPtsInfo->DeviceIntervalPts = PtsInfo[VoChn].DeviceIntervalPts;
	pPtsInfo->IntervalPts = PtsInfo[VoChn].IntervalPts;
	pPtsInfo->ChangeIntervalPts = PtsInfo[VoChn].ChangeIntervalPts;
	pPtsInfo->FirstVideoPts = PtsInfo[VoChn].FirstVideoPts;
	pPtsInfo->FirstAudioPts = PtsInfo[VoChn].FirstAudioPts;
	pPtsInfo->PreGetAudioPts = PtsInfo[VoChn].PreGetAudioPts;
	pPtsInfo->BaseVideoPts = PtsInfo[VoChn].BaseVideoPts;
	pPtsInfo->IFrameOffectPts = PtsInfo[VoChn].IFrameOffectPts;
	pPtsInfo->pFrameOffectPts = PtsInfo[VoChn].pFrameOffectPts;
	pPtsInfo->CurShowPts = PtsInfo[VoChn].CurShowPts;            
	pPtsInfo->StartPts = PtsInfo[VoChn].StartPts; 
	pPtsInfo->QueryStartTime = PtsInfo[VoChn].QueryStartTime;
	pPtsInfo->QueryFinalTime = PtsInfo[VoChn].QueryFinalTime;
	pPtsInfo->StartPrm = PtsInfo[VoChn].StartPrm;
	pPtsInfo->EndPrm = PtsInfo[VoChn].EndPrm;
}

void PRV_SetVoChnPtsInfo(HI_S32 VoChn, g_PRVPtsinfo *pPtsInfo)
{
	PtsInfo[VoChn].PreGetVideoPts = pPtsInfo->PreGetVideoPts;
	PtsInfo[VoChn].CurGetVideoPts = pPtsInfo->CurGetVideoPts;	
	PtsInfo[VoChn].PreVideoPts = pPtsInfo->PreVideoPts;
	PtsInfo[VoChn].CurVideoPts = pPtsInfo->CurVideoPts;	
	PtsInfo[VoChn].PreSendPts = pPtsInfo->PreSendPts;
	PtsInfo[VoChn].CurSendPts = pPtsInfo->CurSendPts;
	PtsInfo[VoChn].CurVoChnPts = pPtsInfo->CurVoChnPts;
	PtsInfo[VoChn].PreVoChnPts = pPtsInfo->PreVoChnPts;	
	PtsInfo[VoChn].DevicePts = pPtsInfo->DevicePts;
	PtsInfo[VoChn].DeviceIntervalPts = pPtsInfo->DeviceIntervalPts;
	PtsInfo[VoChn].IntervalPts = pPtsInfo->IntervalPts;
	PtsInfo[VoChn].ChangeIntervalPts = pPtsInfo->ChangeIntervalPts;
	PtsInfo[VoChn].FirstVideoPts = pPtsInfo->FirstVideoPts;
	PtsInfo[VoChn].FirstAudioPts = pPtsInfo->FirstAudioPts;
	PtsInfo[VoChn].PreGetAudioPts = pPtsInfo->PreGetAudioPts;
	PtsInfo[VoChn].BaseVideoPts = pPtsInfo->BaseVideoPts;
	PtsInfo[VoChn].IFrameOffectPts = pPtsInfo->IFrameOffectPts;
	PtsInfo[VoChn].pFrameOffectPts = pPtsInfo->pFrameOffectPts;
	PtsInfo[VoChn].CurShowPts = pPtsInfo->CurShowPts;            
	PtsInfo[VoChn].StartPts = pPtsInfo->StartPts;  
	PtsInfo[VoChn].QueryStartTime = pPtsInfo->QueryStartTime;
	PtsInfo[VoChn].QueryFinalTime = pPtsInfo->QueryFinalTime;
	PtsInfo[VoChn].StartPrm = pPtsInfo->StartPrm;
	PtsInfo[VoChn].EndPrm = pPtsInfo->EndPrm;

}

void PRV_GetVoChnPlayStateInfo(HI_S32 VoChn, g_ChnPlayStateInfo *pPlayStateInfo)
{
	pPlayStateInfo->CurPlayState = PlayStateInfo[VoChn].CurPlayState;
	pPlayStateInfo->CurSpeedState = PlayStateInfo[VoChn].CurSpeedState;
	pPlayStateInfo->QuerySlaveId = PlayStateInfo[VoChn].QuerySlaveId;
	pPlayStateInfo->SendDataType = PlayStateInfo[VoChn].SendDataType;
	pPlayStateInfo->RealType = PlayStateInfo[VoChn].RealType;
	pPlayStateInfo->SynState = PlayStateInfo[VoChn].SynState;

}

void PRV_SetVoChnPlayStateInfo(HI_S32 VoChn, g_ChnPlayStateInfo *pPlayStateInfo)
{
	PlayStateInfo[VoChn].CurPlayState = pPlayStateInfo->CurPlayState;
	PlayStateInfo[VoChn].CurSpeedState = pPlayStateInfo->CurSpeedState;
	PlayStateInfo[VoChn].QuerySlaveId = pPlayStateInfo->QuerySlaveId;
	PlayStateInfo[VoChn].SendDataType = pPlayStateInfo->SendDataType;
	PlayStateInfo[VoChn].RealType = pPlayStateInfo->RealType;
	PlayStateInfo[VoChn].SynState = pPlayStateInfo->SynState;

}

void PRV_SetChnPlayPts(HI_S32 VoChn, AVPacket *PRV_DataFromRTSP)
{
	HI_S32 s32Ret = 0;
	PtsInfo[VoChn].CurVideoPts = PRV_DataFromRTSP->pts;
	
	if(VochnInfo[VoChn].bIsPBStat)
	{
		if(PlayInfo.PlayBackState >= PLAY_INSTANT)
		{
			s32Ret = PlayBack_CheckIntervalPtsEx(VoChn);
		}
		PtsInfo[VoChn].CurSendPts = PtsInfo[VoChn].PreSendPts + PtsInfo[VoChn].IntervalPts;
		PRV_DataFromRTSP->prvpts = PtsInfo[VoChn].CurSendPts;
		PtsInfo[VoChn].CurTime = (time_t)PRV_DataFromRTSP->vampts;
		if(PlayInfo.PlayBackState != PLAY_INSTANT && s32Ret == 1)
			PtsInfo[VoChn].ChangeIntervalPts = PtsInfo[VoChn].CurSendPts;
		//TRACE(SCI_TRACE_NORMAL, MOD_DEC, "===========PtsInfo[chn].CurSendPts: %lld, PtsInfo[chn].PreSendPts: %lld\n", PtsInfo[VoChn].CurSendPts, PtsInfo[VoChn].PreSendPts);
		PtsInfo[VoChn].PreSendPts = PtsInfo[VoChn].CurSendPts;
	}
	
	PtsInfo[VoChn].PreVideoPts = PtsInfo[VoChn].CurVideoPts;
}

unsigned char VideoDataStream[MAXFRAMELEN] = {0};
HI_S32 PRV_SendData(HI_S32 chn, AVPacket *PRV_DataFromRTSP, HI_S32 datatype, HI_S32 s32StreamChnIDs, HI_S32 PRV_State)
{
	if(NULL == PRV_DataFromRTSP)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_DataFromRTSP: NULL Point!!!\n");
		return HI_FAILURE;
	}

	#if 0
	rtp_stream_testD(PRV_DataFromRTSP);
	printf("%s Line %d -----> chn:%d\n",__func__,__LINE__,chn);
	#endif
	
	unsigned char AudioData[MAXAUDIODATALEN] = {0};
	
	char VideoDataStarCode[] = {0x00, 0x00, 0x00, 0x01};
	char AudioDataStarCode[4];
	((char *)AudioDataStarCode)[0] = 0;
	((char *)AudioDataStarCode)[1] = 1;
	((char *)AudioDataStarCode)[2] = PtNumPerFrm/2;;
	((char *)AudioDataStarCode)[3] = 0;
	AVPacket *NextPacket = NULL;
	AVPacket *pTmp = NULL;
	HI_S32 s32Ret = 0, TotalLength = 0, datalength=0, length = 0, bIsHaveAudio = 0;
	HI_S32 VdecChn = 0, AdChn = 0, SlaveId = 0, index = 0, tmpChn = 0;
	AUDIO_STREAM_S	stAStream;
	VDEC_STREAM_S stVstream;
	int Aoffset = 0;
	unsigned char *p = NULL;
	int nal_start, nal_end, len, ret;	
	
	index = chn + LOCALVEDIONUM;
	VdecChn = VochnInfo[index].VdecChn;
	SlaveId = VochnInfo[index].SlaveId;

	if(PlayInfo.PlayBackState <= PLAY_INSTANT)
		AdChn = DecAdec;
	else
		AdChn = ADECHN;

	if(CODEC_ID_PCMA == datatype || CODEC_ID_PCMU == datatype)
	{
		bIsHaveAudio = detect_audio_input_num();  // ����Ƿ�����Ƶ�����û��������Ƶ 
		if (bIsHaveAudio == 0)
			return HI_SUCCESS;
		
		pTmp = (AVPacket *)PRV_DataFromRTSP;
		NextPacket = pTmp->next;
		while(pTmp != NULL)
		{
			length = pTmp->DataSize - pTmp->BufOffset;
			Aoffset = 0;
			if(Prv_Audiobuf.length == 0 )
			{
				while(length > 0)
				{	
					if(length >= PtNumPerFrm)
					{
						SN_MEMCPY(AudioData, MAXAUDIODATALEN, AudioDataStarCode, 4, 4);
						SN_MEMCPY(AudioData + 4, MAXAUDIODATALEN, (char*)pTmp + pTmp->BufOffset+Aoffset, PtNumPerFrm, PtNumPerFrm);
						stAStream.pStream = AudioData;//PRV_DataFromRTSP->data + offSet + sizeof(HI_U64); 
						stAStream.u32Len = PtNumPerFrm + 4;//dataLen  - sizeof(HI_U64);

						s32Ret = HI_MPI_ADEC_SendStream(AdChn, &stAStream, HI_IO_NOBLOCK);
						if(s32Ret != HI_SUCCESS)  /* ���������� */
						{
							TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Send Audio Stream to Adec %d Fail: 0x%x\n", DecAdec, s32Ret);
							HI_MPI_ADEC_ClearChnBuf(AdChn);
							return s32Ret;
						}
						length = length - PtNumPerFrm;
						Aoffset = Aoffset + PtNumPerFrm;
						//printf("send 1 PtNumPerFrm:%d\n",PtNumPerFrm);
					}
					else
					{
						SN_MEMCPY((char*)Prv_Audiobuf.databuf, sizeof(Prv_Audiobuf.databuf), (char*)pTmp + pTmp->BufOffset+Aoffset, length, length);
						Prv_Audiobuf.length = length;
						length = 0;
						//printf("copy 1 Prv_Audiobuf.length:%d\n",Prv_Audiobuf.length);
					}
				}
			}
			else
			{
				while(length > 0)
				{	
					if(Prv_Audiobuf.length+length >= PtNumPerFrm)
					{
						SN_MEMCPY(AudioData, MAXAUDIODATALEN, AudioDataStarCode, 4, 4);
						if(Prv_Audiobuf.length != 0)
						{
							
							SN_MEMCPY(AudioData + 4, MAXAUDIODATALEN, (char*)Prv_Audiobuf.databuf, Prv_Audiobuf.length, Prv_Audiobuf.length);
							SN_MEMCPY(AudioData + 4 + Prv_Audiobuf.length, MAXAUDIODATALEN, (char*)pTmp + pTmp->BufOffset+Aoffset, PtNumPerFrm-Prv_Audiobuf.length, PtNumPerFrm-Prv_Audiobuf.length);
							stAStream.pStream = AudioData;//PRV_DataFromRTSP->data + offSet + sizeof(HI_U64); 
							stAStream.u32Len = PtNumPerFrm + 4;//dataLen  - sizeof(HI_U64);
							//printf("copy send  2 Prv_Audiobuf.length:%d\n",Prv_Audiobuf.length);
							length = length + Prv_Audiobuf.length - PtNumPerFrm;
							Aoffset = Aoffset + PtNumPerFrm - Prv_Audiobuf.length;
							Prv_Audiobuf.length = 0;

						}
						else
						{
							SN_MEMCPY(AudioData + 4, MAXAUDIODATALEN, (char*)pTmp + pTmp->BufOffset+Aoffset, PtNumPerFrm, PtNumPerFrm);
							stAStream.pStream = AudioData;//PRV_DataFromRTSP->data + offSet + sizeof(HI_U64); 
							stAStream.u32Len = PtNumPerFrm + 4;//dataLen  - sizeof(HI_U64);
							//printf("send  2 PtNumPerFrm:%d\n",PtNumPerFrm);
							length = length - PtNumPerFrm;
							Aoffset = Aoffset + PtNumPerFrm;
						}

						s32Ret = HI_MPI_ADEC_SendStream(AdChn, &stAStream, HI_IO_NOBLOCK);
						if(s32Ret != HI_SUCCESS)  /* ���������� */
						{
							TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Send Audio Stream to Adec %d Fail: 0x%x\n", DecAdec, s32Ret); 
							return s32Ret;
						}

					}
					else
					{
						SN_MEMCPY((char*)Prv_Audiobuf.databuf+Prv_Audiobuf.length, sizeof(Prv_Audiobuf.databuf), (char*)pTmp + pTmp->BufOffset+Aoffset, length, length);
						Prv_Audiobuf.length += length;
						length = 0;
						//printf("copy 2 length:%d,Prv_Audiobuf.length:%d\n",length,Prv_Audiobuf.length);
					}
				}
			}
			if(pTmp->Extendnext == NULL)
			{
				pTmp = (AVPacket *)NextPacket;
				if(pTmp != NULL)
				NextPacket = pTmp->next;
			}
			else
			{
				pTmp = (AVPacket *)(pTmp->Extendnext);
			}
		}
	}
	else if(CODEC_ID_H264 == datatype && VochnInfo[index].IsHaveVdec)
	{
		if(PRV_MASTER == SlaveId)
		{
			pTmp = (AVPacket *)PRV_DataFromRTSP;
			NextPacket = pTmp->next;
			while(pTmp != NULL)
			{
				//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Send Stream\n");
				length = pTmp->DataSize - pTmp->BufOffset;
				if(pTmp->frame_type == 0 || pTmp->frame_type == 1)
				{					
					if(TotalLength + 4 <= MAXFRAMELEN)
					{
						SN_MEMCPY(VideoDataStream + TotalLength, MAXFRAMELEN, VideoDataStarCode, 4, 4);
						TotalLength += 4;
					}
					else
					{						
						TRACE(SCI_TRACE_NORMAL, MOD_PRV, "WARNING: TotalLength + 4 > MAXFRAMELEN\n"); 
						return HI_FAILURE;
					}
				}				
			
				if(TotalLength + length <= MAXFRAMELEN)
				{
					SN_MEMCPY(VideoDataStream + TotalLength, MAXFRAMELEN, (char*)pTmp + pTmp->BufOffset, length, length);
					TotalLength += length;
				}
				else
				{						
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "WARNING: TotalLength + length > MAXFRAMELEN\n"); 
					return HI_FAILURE;
				}
				
				if(pTmp->Extendnext == NULL)
				{
					pTmp = (AVPacket *)NextPacket;
					if(pTmp != NULL)
						NextPacket = pTmp->next;
				}
				else
				{
					pTmp = (AVPacket *)(pTmp->Extendnext);
				}
			}

			if(TotalLength + 4 <= MAXFRAMELEN)
			{
				SN_MEMCPY(VideoDataStream + TotalLength, MAXFRAMELEN, VideoDataStarCode, 4, 4);
			}
			else
			{						
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "WARNING: TotalLength + 4 > MAXFRAMELEN\n"); 
				return HI_FAILURE;
			}

			p = (unsigned char *)VideoDataStream;
			len = TotalLength + 4;
			while(find_nal_unit((uint8_t*)p, len, &nal_start, &nal_end) > 0)
			{
				p += nal_start; 
				ret = read_nal_unit((h264_stream_t*)&h_stream[VdecChn], (uint8_t*)p, nal_end - nal_start);
				
				p += (nal_end - nal_start);
            	len -= nal_end;
			}
			
			if (h_stream[VdecChn].nal.nal_unit_type == 1 || h_stream[VdecChn].nal.nal_unit_type == 5)
			{
				if (h_stream[VdecChn].sh.frame_num - h_stream[VdecChn].pre_num > 1)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_SLC, "line:%d ch=%d, nal_unit_type=%d, pre_num=%d, frame_num=%d", __LINE__, VdecChn, h_stream[VdecChn].nal.nal_unit_type, h_stream[VdecChn].pre_num, h_stream[VdecChn].sh.frame_num);
					h_stream[VdecChn].pre_num = h_stream[VdecChn].sh.frame_num;
					return HI_SUCCESS;
				}
				else
				{
#if 1
					if(SendChnInfo.is_disgard[VdecChn] && h_stream[VdecChn].nal.nal_unit_type == 1)
					{
						TRACE(SCI_TRACE_NORMAL, MOD_SLC, "1 line:%d ch=%d, nal_unit_type=%d, pre_num=%d, frame_num=%d, first_mb_in_slice=%d", __LINE__, VdecChn, h_stream[VdecChn].nal.nal_unit_type, h_stream[VdecChn].pre_num, h_stream[VdecChn].sh.frame_num, h_stream[VdecChn].sh.first_mb_in_slice);
						SendChnInfo.is_first[VdecChn] = 0;
						return HI_SUCCESS;
					}

					if(h_stream[VdecChn].nal.nal_unit_type == 5 && h_stream[VdecChn].sh.first_mb_in_slice == 0)
					{
						SendChnInfo.is_disgard[VdecChn] = 0;
						SendChnInfo.is_first[VdecChn] = 1;
						h_stream[VdecChn].pre_num = h_stream[VdecChn].sh.frame_num;
					}
					
					if(SendChnInfo.is_first[VdecChn] && h_stream[VdecChn].nal.nal_unit_type == 5 && h_stream[VdecChn].sh.first_mb_in_slice !=0)
					{
						TRACE(SCI_TRACE_NORMAL, MOD_SLC, "2 is_first line:%d ch=%d, nal_unit_type=%d, pre_num=%d, frame_num=%d, first_mb_in_slice=%d", __LINE__, VdecChn, h_stream[VdecChn].nal.nal_unit_type, h_stream[VdecChn].pre_num, h_stream[VdecChn].sh.frame_num, h_stream[VdecChn].sh.first_mb_in_slice);
						SendChnInfo.is_disgard[VdecChn] = 1;
						SendChnInfo.is_first[VdecChn] = 0;
						return HI_SUCCESS;
					}

					if(SendChnInfo.is_disgard[VdecChn] && SendChnInfo.is_first[VdecChn] == 0 && h_stream[VdecChn].nal.nal_unit_type == 5)
					{
						TRACE(SCI_TRACE_NORMAL, MOD_SLC, "2 is_disgard line:%d ch=%d, nal_unit_type=%d, pre_num=%d, frame_num=%d, first_mb_in_slice=%d", __LINE__, VdecChn, h_stream[VdecChn].nal.nal_unit_type, h_stream[VdecChn].pre_num, h_stream[VdecChn].sh.frame_num, h_stream[VdecChn].sh.first_mb_in_slice);
						
						return HI_SUCCESS;
					}
#endif				
					if(!VochnInfo[index].bIsPBStat && SendChnInfo.is_first[VdecChn] == 0 && h_stream[VdecChn].pre_num == h_stream[VdecChn].sh.frame_num)
					{
						SendChnInfo.is_same[VdecChn] = 1;
					}
					else
					{
						SendChnInfo.is_same[VdecChn] = 0;
					}
					
					h_stream[VdecChn].pre_num = h_stream[VdecChn].sh.frame_num;
					
				}
			}

			if(!VochnInfo[index].bIsPBStat && SendChnInfo.is_first[VdecChn])
			{
				if(SendChnInfo.VideoDataStream1[VdecChn] != NULL)
				{
					SN_FREE(SendChnInfo.VideoDataStream1[VdecChn]);
					SendChnInfo.VideoDataStream1[VdecChn] = NULL;
				}

				SendChnInfo.VideoDataStream1[VdecChn] = (HI_U8*)SN_MALLOC(TotalLength);
				if(SendChnInfo.VideoDataStream1[VdecChn] == NULL)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "%s, line:%d VideoDataStream1[%d] is NULL\n", VdecChn, __FUNCTION__, __LINE__);
					return HI_SUCCESS;
				}
				
				SN_MEMCPY(SendChnInfo.VideoDataStream1[VdecChn], TotalLength, VideoDataStream , TotalLength, TotalLength);
				SendChnInfo.TotalLength1[VdecChn] = TotalLength;
				SendChnInfo.is_first[VdecChn] = 0;
				return HI_SUCCESS;
			}
			
			if(SendChnInfo.is_same[VdecChn])
			{
				if(SendChnInfo.VideoDataStream2[VdecChn] != NULL)
				{
					SN_FREE(SendChnInfo.VideoDataStream2[VdecChn]);
					datalength = 0;
				}

				if(SendChnInfo.VideoDataStream1[VdecChn] == NULL)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "%s, line:%d VideoDataStream1[%d] is NULL\n", VdecChn, __FUNCTION__, __LINE__);
					SendChnInfo.TotalLength1[VdecChn] = 0;
					SendChnInfo.is_first[VdecChn] = 0;
					return HI_SUCCESS;
				}

				datalength = TotalLength + SendChnInfo.TotalLength1[VdecChn];

				SendChnInfo.VideoDataStream2[VdecChn] = (HI_U8*)SN_MALLOC(datalength);

				if(SendChnInfo.VideoDataStream2[VdecChn] == NULL)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "%s, line:%d VideoDataStream2[%d] is NULL\n", VdecChn, __FUNCTION__, __LINE__);
					if(SendChnInfo.VideoDataStream1[VdecChn] != NULL)
					{
						SN_FREE(SendChnInfo.VideoDataStream1[VdecChn]);
						SendChnInfo.VideoDataStream1[VdecChn] = NULL;
					}
					SendChnInfo.TotalLength1[VdecChn] = TotalLength;
					SendChnInfo.is_first[VdecChn] = 0;
					SendChnInfo.is_same[VdecChn] = 0;
					return HI_SUCCESS;
				}

				SN_MEMCPY(SendChnInfo.VideoDataStream2[VdecChn], datalength, SendChnInfo.VideoDataStream1[VdecChn], SendChnInfo.TotalLength1[VdecChn], SendChnInfo.TotalLength1[VdecChn]);
				
				SN_MEMCPY(SendChnInfo.VideoDataStream2[VdecChn] + SendChnInfo.TotalLength1[VdecChn], datalength, VideoDataStream, TotalLength, TotalLength);
				
			}
			
			if(g_filesign[VdecChn][1] >= 0 && g_filesign[VdecChn][1] == VdecChn)
			{
				char name[32];
				SN_SPRINTF(name, sizeof(name), "/tmp/send_ch%d.h264", g_filesign[VdecChn][1]);
				
				FILE *pf = fopen(name, "a");
				if(NULL!=pf)
				{					
					if(SendChnInfo.is_same[VdecChn])
					{
						if(datalength)
						{
							fwrite(SendChnInfo.VideoDataStream2[VdecChn],datalength,1,pf);
						}
					}
					else
					{
						if(SendChnInfo.TotalLength1[VdecChn])
						{
							fwrite(SendChnInfo.VideoDataStream1[VdecChn],SendChnInfo.TotalLength1[VdecChn],1,pf);
						}
					}

					fclose(pf);
				}
			}

			if(SendChnInfo.is_same[VdecChn])
			{
				stVstream.pu8Addr = SendChnInfo.VideoDataStream2[VdecChn];
				stVstream.u32Len = datalength;
			}
			else
			{
				stVstream.pu8Addr = SendChnInfo.VideoDataStream1[VdecChn];
				stVstream.u32Len = SendChnInfo.TotalLength1[VdecChn];
			}

			if(!VochnInfo[index].bIsPBStat)
			{
				stVstream.u64PTS = PRV_DataFromRTSP->prvpts;
			}
			else
			{
				stVstream.u64PTS = PRV_DataFromRTSP->prvpts;
			}

			/*������������Ƶ������*/
			s32Ret = HI_MPI_VDEC_SendStream(VdecChn, &stVstream, HI_IO_NOBLOCK);
			
			SendChnInfo.TotalLength1[VdecChn] = 0;
			SendChnInfo.is_first[VdecChn] = 1;
			
			if(SendChnInfo.is_same[VdecChn] == 0)
			{
				SendChnInfo.is_first[VdecChn] = 0;
				
				if(SendChnInfo.VideoDataStream1[VdecChn] != NULL)
				{
					SN_FREE(SendChnInfo.VideoDataStream1[VdecChn]);
					SendChnInfo.VideoDataStream1[VdecChn] = NULL;
				}

				SendChnInfo.VideoDataStream1[VdecChn] = (HI_U8*)SN_MALLOC(TotalLength);
				SN_MEMCPY(SendChnInfo.VideoDataStream1[VdecChn], TotalLength, VideoDataStream, TotalLength, TotalLength);
				SendChnInfo.TotalLength1[VdecChn] = TotalLength;
			}
			else
			{
				if(SendChnInfo.VideoDataStream2[VdecChn] != NULL)
				{
					SN_FREE(SendChnInfo.VideoDataStream2[VdecChn]);
					SendChnInfo.VideoDataStream2[VdecChn] = NULL;
					datalength = 0;
				}
			}
		
			if(s32Ret != HI_SUCCESS)  /* ������Ƶ������ */
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Send Video Stream to Vdec %d Fail: 0x%x\n", VdecChn, s32Ret); 		
				return s32Ret;
			}
			
			return HI_SUCCESS;
		}
		else if(SlaveId > PRV_MASTER)
		{	
			unsigned char sdata[24] = {0};
			int sdata_len = 24;
			if(PRV_State == 1)//�˳��ط�/��Ƭʱ������ڴ�Ƭ��ÿ֡����һ�δ�Ƭ
			{				
				pTmp = (AVPacket *)PRV_DataFromRTSP;
				*(HI_S32 *)sdata = 1;
				*(HI_S32 *)(sdata + sizeof(HI_S32)) = PRV_DataFromRTSP->frame_type;
				*(HI_S32 *)(sdata + 2 * sizeof(HI_S32)) = VdecChn;
				//HostSendHostToSlaveBuffer(s32StreamChnIDs, sdata, sizeof(HI_S32), OldVdec[i]);
				*(HI_S32 *)(sdata + 3 * sizeof(HI_S32)) = PRV_DataFromRTSP->data_len;
				*(HI_U64 *)(sdata + 4 * sizeof(HI_S32)) = /*(PRV_DataFromRTSP->pts - PtsInfo[chn].FirstVideoPts) * 1000000/90000;//*/PRV_DataFromRTSP->prvpts;
				
				if(HostSendHostToSlaveReady(s32StreamChnIDs, PRV_DataFromRTSP->data_len + sdata_len, VdecChn) != HI_SUCCESS)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line: %d, HostSendHostToSlaveStart fail\n", __LINE__);
					PRV_InitHostToSlaveStream();
					return HI_SUCCESS;
				}
				HostSendHostToSlaveBuffer(s32StreamChnIDs, sdata, sdata_len, VdecChn);	
				NextPacket = pTmp->next;
				while(pTmp != NULL)
				{
					length = pTmp->DataSize - pTmp->BufOffset;
					if(pTmp->frame_type == 0 || pTmp->frame_type == 1)
					{
						HostSendHostToSlaveBuffer(s32StreamChnIDs, VideoDataStarCode, 4, VdecChn);
					}
					
					HostSendHostToSlaveBuffer(s32StreamChnIDs, (char*)pTmp + pTmp->BufOffset, length, VdecChn);
					if(pTmp->Extendnext == NULL)
					{
						pTmp = (AVPacket *)NextPacket;
						if(pTmp != NULL)
						NextPacket = pTmp->next;
					}
					else
					{
						pTmp = (AVPacket *)(pTmp->Extendnext);
					}
				}
				if(HostSendHostToSlaveStart(s32StreamChnIDs) == HI_FAILURE)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line: %d, HostSendHostToSlaveStart fail\n", __LINE__);
					PRV_InitHostToSlaveStream();
				}
				return HI_SUCCESS;
			}
			#if (IS_DECODER_DEVTYPE == 1)
			{
				#if 0
				if(fp == NULL)
				{
					fp = fopen("/tmp/video", "wb+");
					if(fp == NULL)
					{
						printf("============Open file fail!\n");
					}
						
				}
				#endif
				pTmp = (AVPacket *)PRV_DataFromRTSP;
				TotalLength = 0;
				NextPacket = pTmp->next;

				*(HI_S32 *)VideoDataStream = 1;//�������͵���֡��
				*(HI_S32 *)(VideoDataStream + sizeof(HI_S32)) = PRV_DataFromRTSP->frame_type;
				*(HI_S32 *)(VideoDataStream + 2 * sizeof(HI_S32)) = VdecChn;
				*(HI_S32 *)(VideoDataStream + 3 * sizeof(HI_S32)) = PRV_DataFromRTSP->data_len;
				*(HI_U64 *)(VideoDataStream + 4 * sizeof(HI_S32)) = /*(PRV_DataFromRTSP->pts - PtsInfo[chn].FirstVideoPts) * 1000000/90000;//*/PRV_DataFromRTSP->prvpts;
				TotalLength += 24;
				//if(VochnInfo[index].bIsPBStat)
				
				//CHECK(HostSendHostToSlaveReady(s32StreamChnIDs, PRV_DataFromRTSP->data_len + sdata_len, VdecChn));
				//(HostSendHostToSlaveBuffer(s32StreamChnIDs, sdata, sdata_len, VdecChn));	
				#if 0
				if(PRV_DataFromRTSP->frame_type == 0 || PRV_DataFromRTSP->frame_type == 1)
				{
					CHECK(HostSendHostToSlaveBuffer(s32StreamChnIDs, VideoDataStarCode, 4, VdecChn));
					if(fp != NULL)
					{
						if(fwrite(VideoDataStarCode, 4, 1, fp) != 1)
						{
							printf("++++++++++++++Write Fail!\n");
						}

					}
				}
				#endif
				pTmp = (AVPacket *)PRV_DataFromRTSP;
				NextPacket = pTmp->next;
				while(pTmp != NULL)
				{
					length = pTmp->DataSize - pTmp->BufOffset;
					if(pTmp->frame_type == 0 || pTmp->frame_type == 1)
					{
						
						if(TotalLength + 4 <= MAXFRAMELEN)
						{
							SN_MEMCPY(VideoDataStream + TotalLength, MAXFRAMELEN, VideoDataStarCode, 4, 4);
							TotalLength += 4;
						}
						else
						{						
							TRACE(SCI_TRACE_NORMAL, MOD_PRV, "WARNING: TotalLength + 4 > MAXFRAMELEN\n"); 
							return HI_FAILURE;
						}
						//HostSendHostToSlaveBuffer(s32StreamChnIDs, VideoDataStarCode, 4, VdecChn);
					}
					if(TotalLength + length <= MAXFRAMELEN)
					{
						SN_MEMCPY(VideoDataStream + TotalLength, MAXFRAMELEN, (char*)pTmp + pTmp->BufOffset, length, length);
						TotalLength += length;
					}
					else
					{						
						TRACE(SCI_TRACE_NORMAL, MOD_PRV, "WARNING: slave===TotalLength + length > MAXFRAMELEN\n"); 
						return HI_FAILURE;
					}
					
					//HostSendHostToSlaveBuffer(s32StreamChnIDs, (char*)pTmp + pTmp->BufOffset, length, VdecChn);
					if(pTmp->Extendnext == NULL)
					{
						pTmp = (AVPacket *)NextPacket;
						if(pTmp != NULL)
							NextPacket = pTmp->next;
					}
					else
					{
						pTmp = (AVPacket *)(pTmp->Extendnext);
					}
				}
				#if 0
				length = PRV_DataFromRTSP->DataSize - PRV_DataFromRTSP->BufOffset;
				CHECK(HostSendHostToSlaveBuffer(s32StreamChnIDs, (char*)PRV_DataFromRTSP + PRV_DataFromRTSP->BufOffset, length, VdecChn));
				if(fp != NULL && WriteCount <= 100)
				{
					if(fwrite((char*)PRV_DataFromRTSP + PRV_DataFromRTSP->BufOffset, length, 1, fp) != 1)
					{
						printf("++++++++++++++Write Fail!\n");
					}
					else
						WriteCount++;
				}
				#endif
				int sendState = 0;
				s32Ret = HostSendHostToSlaveStream(s32StreamChnIDs, VideoDataStream, TotalLength, VdecChn, &sendState);	 /* �������ݸ���Ƭ */
				if(s32Ret != HI_SUCCESS)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line: %d, HostSendHostToSlaveStart fail\n", __LINE__);
					PRV_InitHostToSlaveStream();
					return HI_FAILURE;
				}
				//TRACE(SCI_TRACE_NORMAL, MOD_VAM, "slave: Vdec: %d, TotalLength: %d, data_len: %d, s32StreamChnIDs: %d, PRV_DataFromRTSP->prvpts: %lld\n", VdecChn, TotalLength, PRV_DataFromRTSP->data_len, s32StreamChnIDs, PRV_DataFromRTSP->prvpts);
				//CHECK(HostSendHostToSlaveStart(s32StreamChnIDs));
				tmpChn = VochnInfo[index].CurChnIndex;
				VoChnState.VideoDataTimeLag[tmpChn] = (HI_S64)(PRV_DataFromRTSP->pts - PtsInfo[tmpChn].FirstVideoPts) * 1000/90000;//��ǰ��Ƶʱ������׼ʱ����Ĳ�ֵ(�����ms)				
				return HI_SUCCESS;
			}
			#endif
			//�̶���С����SendSize1��SendSize2(�������)
			AVPacket *pTmp1 = NULL, *OldpTmp = NULL;
			int i = 0, count = 0, tmpSendDataLen = 0, tmpIndex = 0;
			int SendSizeMin = 0, SendSizeMax = 0, ToTalFramePerTime = 0;
#if defined(SN9234H1)
			SendSizeMin = BASE_SENDSIZE_MIN * 2;
			SendSizeMax = BASE_SENDSIZE_MAX * 2;
#else			
			SendSizeMin = BASE_SENDSIZE_MIN * CurSlaveChnCount;
			SendSizeMax = BASE_SENDSIZE_MAX * CurSlaveChnCount;
#endif			
			PRV_SendDataLen = PRV_SendDataLen + PRV_DataFromRTSP->data_len + sdata_len;
			//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_DataFromRTSP->data_len: %d, PRV_SendDataLen: %d", PRV_DataFromRTSP->data_len, PRV_SendDataLen);
			//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "SendDataLen: %d, CurIndex: %d\n", SendDataLen, CurIndex);
			//�ۻ���һ��֡����һ�𷢴�Ƭ��
			//���ÿ֡����һ�Σ����Խ���Ԥ��5%CPUʹ����
			//����ۻ���һ����С��һ�𷢴�Ƭ����Ƭ��ʾ��ʵʱ�Ը���
#if defined(SN9234H1)
			if(PRV_CurIndex < 2)
#else			
			if(PRV_CurIndex < CurSlaveChnCount)
#endif			
			{
				if(PRV_SendDataLen < SendSizeMax)
				{
					PRV_OldVideoData[PRV_CurIndex] = PRV_DataFromRTSP;
					PRV_OldVdec[PRV_CurIndex] = VdecChn;
					PRV_CurIndex++;
					
					if(PRV_SendDataLen < SendSizeMin)
					{
						return HI_SUCCESS;
					}
				}
				else
				{
					PRV_SendDataLen = PRV_SendDataLen - PRV_DataFromRTSP->data_len - sdata_len;
					pTmp = PRV_DataFromRTSP;
					pTmp1 = pTmp;
				}

			}
			//���ⳬ�������ݶ��ⵥ������
			else
			{
				PRV_SendDataLen = PRV_SendDataLen - PRV_DataFromRTSP->data_len - sdata_len;
				pTmp = PRV_DataFromRTSP;
				pTmp1 = pTmp;
			}
			if(s32StreamChnIDs <= 0)
			{
				for(i = 0; i < PRV_CurIndex; i++)
				{
					NTRANS_FreeMediaData(PRV_OldVideoData[i]);
					PRV_OldVideoData[i] = NULL;
				}
				PRV_CurIndex = 0;
				PRV_SendDataLen = 0;
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "s32StreamChnIDs: %d\n", s32StreamChnIDs);
				return HI_FAILURE;
			}
			//�ȷ����ۻ�������
			if(HostSendHostToSlaveReady(s32StreamChnIDs, PRV_SendDataLen, VdecChn)!=HI_SUCCESS)
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line: %d, HostSendHostToSlaveStart fail\n", __LINE__);
				PRV_InitHostToSlaveStream();
				return HI_FAILURE;
			}
			ToTalFramePerTime = PRV_CurIndex;
			for(i = 0; i < PRV_CurIndex; i++)
			{
				//�ж��Ƿ������ݱ��ͷŵ�������У���˴η��͵���֡��-1
				if(PRV_OldVideoData[i] == NULL)
				{
					ToTalFramePerTime--;
				}
			}
			//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Ready Send Data: %d, CurIndex: %d\n", SendDataLen, CurIndex);
			for(i = 0; i < PRV_CurIndex; i++)
			{
				if(PRV_OldVideoData[i] == NULL)
				{
					continue;
				}
				*(HI_S32 *)sdata = ToTalFramePerTime;//�������͵���֡��
				*(HI_S32 *)(sdata + sizeof(HI_S32)) = PRV_OldVideoData[i]->frame_type;
				*(HI_S32 *)(sdata + 2 * sizeof(HI_S32)) = PRV_OldVdec[i];
				//HostSendHostToSlaveBuffer(s32StreamChnIDs, sdata, sizeof(HI_S32), OldVdec[i]);
				*(HI_S32 *)(sdata + 3 * sizeof(HI_S32)) = PRV_OldVideoData[i]->data_len;
				*(HI_U64 *)(sdata + 4 * sizeof(HI_S32)) = PRV_OldVideoData[i]->prvpts;
				CHECK(HostSendHostToSlaveBuffer(s32StreamChnIDs, sdata, sdata_len, PRV_OldVdec[i]));	

				OldpTmp = PRV_OldVideoData[i];
				NextPacket = OldpTmp->next;
				while(OldpTmp != NULL)
				{
					length = OldpTmp->DataSize - OldpTmp->BufOffset;
					if(OldpTmp->frame_type == 0 || OldpTmp->frame_type == 1)
					{
						CHECK(HostSendHostToSlaveBuffer(s32StreamChnIDs, VideoDataStarCode, 4, PRV_OldVdec[i]));
					}
					
					CHECK(HostSendHostToSlaveBuffer(s32StreamChnIDs, (char*)OldpTmp + OldpTmp->BufOffset, length, PRV_OldVdec[i]));

					if(OldpTmp->Extendnext == NULL)
					{
						OldpTmp = (AVPacket *)NextPacket;
						if(OldpTmp != NULL)
						NextPacket = OldpTmp->next;
					}
					else
					{
						OldpTmp = (AVPacket *)(OldpTmp->Extendnext);
					}
				}
				tmpIndex = PRV_GetVoChnIndex(PRV_OldVdec[i]);
				tmpChn = VochnInfo[tmpIndex].CurChnIndex;
				VoChnState.VideoDataTimeLag[tmpChn] = (HI_S64)(PRV_OldVideoData[i]->pts - PtsInfo[tmpChn].FirstVideoPts) * 1000/90000;//��ǰ��Ƶʱ������׼ʱ����Ĳ�ֵ(�����ms)
				NTRANS_FreeMediaData(PRV_OldVideoData[i]);
				PRV_OldVideoData[i] = NULL;
				
			}
			if(HostSendHostToSlaveStart(s32StreamChnIDs) == HI_FAILURE)
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line: %d, HostSendHostToSlaveStart fail\n", __LINE__);
				PRV_InitHostToSlaveStream();
			}

			PRV_CurIndex = 0;
			PRV_SendDataLen = 0;
			//�ٷ��Ͷ��������
			if(pTmp != NULL)
			{	
				//��ǰ��������ݴ���PCIһ�δ���Ĵ�Сʱ�������ݷֶ�η��͸���Ƭ
				if(PRV_DataFromRTSP->data_len > PCIV_VDEC_STREAM_BUF_LEN/2)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "VdecChn: %d---PRV_DataFromRTSP->data_len---%d > 1024 *1024\n", VdecChn, PRV_DataFromRTSP->data_len);
					NextPacket = pTmp->next;
					while(pTmp != NULL)
					{
						count = 0;
						tmpSendDataLen = 0;
						//��λÿ�η������ݵĽ�����λ��
						while(pTmp1 != NULL)
						{
							length = pTmp1->DataSize - pTmp1->BufOffset;
							tmpSendDataLen += length;
							//ÿ����෢��1M
							if(tmpSendDataLen >= (PCIV_VDEC_STREAM_BUF_LEN/2  - sdata_len))
							{
								tmpSendDataLen -= length;								
								break;
							}
							count++;
							if(pTmp1->Extendnext == NULL)
							{
								pTmp1 = (AVPacket *)NextPacket;
								if(pTmp1 != NULL)
								NextPacket = pTmp1->next;
							}
							else
							{
								pTmp1 = (AVPacket *)(pTmp1->Extendnext);
							}
						}
						TRACE(SCI_TRACE_NORMAL, MOD_PRV, "count---%d, tmpSendDataLen: %d\n", count, tmpSendDataLen);
						
						//��λ���������󣬷��ʹ˰�֮ǰ������
						if(HostSendHostToSlaveReady(s32StreamChnIDs, tmpSendDataLen + sdata_len, VdecChn)!=HI_SUCCESS)
						{
							TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line: %d, HostSendHostToSlaveStart fail\n", __LINE__);
							PRV_InitHostToSlaveStream();
							return HI_FAILURE;
						}
						*(HI_S32 *)sdata = 1;
						*(HI_S32 *)(sdata + sizeof(HI_S32)) = -1;//˵��һ֡���ݲ��÷ֶη��ͣ���������ͬһ֡���ݵ����ಿ��

					
						if(pTmp1 == NULL)//���һ��������
						{
							*(HI_S32 *)(sdata + sizeof(HI_S32)) = PRV_DataFromRTSP->frame_type + 2;
						}

						*(HI_S32 *)(sdata + 2 * sizeof(HI_S32)) = VdecChn;
						//HostSendHostToSlaveBuffer(s32StreamChnIDs, sdata, sizeof(HI_S32), VdecChn);
						*(HI_S32 *)(sdata + 3 * sizeof(HI_S32)) = tmpSendDataLen;
						*(HI_U64 *)(sdata + 4 * sizeof(HI_S32)) = PRV_DataFromRTSP->prvpts;
						CHECK(HostSendHostToSlaveBuffer(s32StreamChnIDs, sdata, sdata_len, VdecChn));//������3�����ݱ�ʶ

						i = 0;
						NextPacket = pTmp->next;
						while(i++ < count )
						{
							length = pTmp->DataSize - pTmp->BufOffset;
							if(pTmp->frame_type == 0 || pTmp->frame_type == 1)
							{
								//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "i: %d, pTmp->frame_type: %d", i, pTmp->frame_type);
								CHECK(HostSendHostToSlaveBuffer(s32StreamChnIDs, VideoDataStarCode, 4, VdecChn));
							}							
							CHECK(HostSendHostToSlaveBuffer(s32StreamChnIDs, (char*)pTmp + pTmp->BufOffset, length, VdecChn));

							if(pTmp->Extendnext == NULL)
							{
								pTmp = (AVPacket *)NextPacket;
								if(pTmp != NULL)
								NextPacket = pTmp->next;
							}
							else
							{
								pTmp = (AVPacket *)(pTmp->Extendnext);
							}

						}
						if(HostSendHostToSlaveStart(s32StreamChnIDs) == HI_FAILURE)
						{
							TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line: %d, HostSendHostToSlaveStart fail\n", __LINE__);
							PRV_InitHostToSlaveStream();
						}

					}
				}
				else//���ݿ���һ�η���
				{						
					if(HostSendHostToSlaveReady(s32StreamChnIDs,  PRV_DataFromRTSP->data_len + sdata_len, VdecChn)!=HI_SUCCESS)	
					{
						TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line: %d, HostSendHostToSlaveStart fail\n", __LINE__);
						PRV_InitHostToSlaveStream();
						return HI_FAILURE;
					}
					*(HI_S32 *)sdata = 1;
					*(HI_S32 *)(sdata + sizeof(HI_S32)) = PRV_DataFromRTSP->frame_type;
					*(HI_S32 *)(sdata + 2 * sizeof(HI_S32)) = VdecChn;
					//HostSendHostToSlaveBuffer(s32StreamChnIDs, sdata, sizeof(HI_S32), VdecChn);
					*(HI_S32 *)(sdata + 3 * sizeof(HI_S32)) = PRV_DataFromRTSP->data_len;
					*(HI_U64 *)(sdata + 4 * sizeof(HI_S32)) = PRV_DataFromRTSP->prvpts;
					CHECK(HostSendHostToSlaveBuffer(s32StreamChnIDs, sdata, sdata_len, VdecChn));	//������3�����ݱ�ʶ
					NextPacket = pTmp->next;
					while(pTmp != NULL)
					{
						length = pTmp->DataSize - pTmp->BufOffset;
						if(pTmp->frame_type == 0 || pTmp->frame_type == 1)
						{
							CHECK(HostSendHostToSlaveBuffer(s32StreamChnIDs, VideoDataStarCode, 4, VdecChn));
						}
						
						CHECK(HostSendHostToSlaveBuffer(s32StreamChnIDs, (char*)pTmp + pTmp->BufOffset, length, VdecChn));
						if(pTmp->Extendnext == NULL)
						{
							pTmp = (AVPacket *)NextPacket;
							if(pTmp != NULL)
							NextPacket = pTmp->next;
						}
						else
						{
							pTmp = (AVPacket *)(pTmp->Extendnext);
						}
					}
					if(HostSendHostToSlaveStart(s32StreamChnIDs) == HI_FAILURE)
					{
						TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line: %d, HostSendHostToSlaveStart fail\n", __LINE__);
						PRV_InitHostToSlaveStream();
					}
				}					
				VoChnState.VideoDataTimeLag[chn] = (HI_S64)(PRV_DataFromRTSP->pts - PtsInfo[chn].FirstVideoPts) * 1000/90000;//��ǰ��Ƶʱ������׼ʱ����Ĳ�ֵ(�����ms)
			}			
		}
		else
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Error SlaveId---%d\n", SlaveId);
			return HI_FAILURE;
		}
	}
	else
	{
		return HI_FAILURE;
	}
	
	//TRACE(SCI_TRACE_NORMAL, MOD_DEC, "End Send: %d, slaveid: %d\n", VdecChn, SlaveId);
	return HI_SUCCESS;

}


//����������Ƶ���������
HI_S32 PRV_SetAoAttr(HI_U32 u32PtNumPerFrm)
{
#if defined(Hi3531)
	AUDIO_DEV AoDev = 4;
#else
	AUDIO_DEV AoDev = 0;
#endif	
	AO_CHN AoChn = 0;
	AIO_ATTR_S stAioAttr;
	
	CHECK_RET(HI_MPI_AO_GetPubAttr(AoDev, &stAioAttr));
	stAioAttr.u32PtNumPerFrm = u32PtNumPerFrm;

	CHECK_RET(HI_MPI_AO_DisableChn(AoDev, AoChn));

	CHECK_RET(HI_MPI_AO_Disable(AoDev));

	CHECK_RET(HI_MPI_AO_SetPubAttr(AoDev, &stAioAttr));

	CHECK_RET(HI_MPI_AO_Enable(AoDev));

	CHECK_RET(HI_MPI_AO_EnableChn(AoDev, AoChn));
#if defined(SN9234H1)	
	HI_MPI_AO_BindAdec(AoDev, AoChn, DecAdec);
#endif
	return HI_SUCCESS;

}

//�ж���Ƶ�����Ƿ�ı䣬����ı��ˣ���Ҫ����������Ƶ�������
HI_S32 PRV_IsAudioParaChange(int chn, int length)
{
	//RET_SUCCESS("");
#if defined(Hi3531)	
	AUDIO_DEV AoDev = 4;
	AO_CHN AoChn = 0;
#elif defined(Hi3535)
	AUDIO_DEV AoDev = 0;
	AO_CHN AoChn = 0;
#endif	
	HI_U32 NewPtNumPerFrm = 0;

	if(chn < 0 || chn > DEV_CHANNEL_NUM)
	{
		RET_FAILURE("chn is error");
	}

	if(length != 160 && length != 320)
		length = 320;
	
	if(length != PtNumPerFrm)
	{
		if(IsCreateAdec == 0)
		{
			PRV_StartAdec(VochnInfo[chn].AudioInfo.adoType,DecAdec);
			IsCreateAdec = 1;
		}
		
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "-----------PtNumPerFrm Is Change----Old: %d, New: %d\n", PtNumPerFrm, length);
		NewPtNumPerFrm = length;
#if defined(Hi3531)||defined(Hi3535)	
		if(IsAdecBindAo == 1)
		{
			PRV_AUDIO_AoUnbindAdec(AoDev, AoChn, DecAdec);
			IsAdecBindAo = 0;
		}
#endif		
		CHECK_RET(PRV_SetAoAttr(NewPtNumPerFrm));
#if defined(Hi3531)||defined(Hi3535)	
		if(IsAdecBindAo == 0)
		{
			PRV_AUDIO_AoBindAdec(AoDev, AoChn, DecAdec);
			IsAdecBindAo = 1;
		}
#endif		
		PtNumPerFrm = NewPtNumPerFrm;
		Prv_Audiobuf.length = 0;
	}
	
	RET_SUCCESS("");
}


//�������ݵ�SPS��Ϣ����ȡ��Ƶ�ķֱ���
HI_S32 PRV_SearchSPS(int chn, AVPacket *PRV_DataFromRTSP, int *new_width, int *new_height)
{
	HI_S32 index = 0;	
	SPS SPSInfo;
	AVPacket *pTmp = PRV_DataFromRTSP;
	SN_MEMSET(&SPSInfo, 0, sizeof(SPS_DATA));
	index = chn + LOCALVEDIONUM;
	
	while(pTmp != NULL)
	{
		if(pTmp->frame_type == 1)
		{
			if((((char*)pTmp)[pTmp->BufOffset] & 0x1f) == 0x7)
			{
				/*����SPS֡�ĵ�һ���ֽ�(NALUͷ)*/
				ParseSPS_Ex((unsigned char*)pTmp + pTmp->BufOffset + 1, &SPSInfo);//����I֡���ݣ�����SPS��Ϣ
				break;
			}
		}
		pTmp = pTmp->next;
	}
	*new_width = (SPSInfo.mb_width) * 16;
	*new_height = (SPSInfo.mb_height) * 16;

 	//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "%s line: %d, chn: %d, ref_frame_count=%d\n", __FUNCTION__, __LINE__, chn, SPSInfo.ref_frame_count);

#if(DEV_TYPE == DEV_SN_9234_H_1)	 //3520��Ҫ�����жϣ�3531����Ҫ
	if(SPSInfo.profile_idc > 66 && !VochnInfo[index].bIsPBStat)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "%s line: %d, chn: %d, new_width: %d, new_height: %d, profile_idc=%d\n",
			__FUNCTION__, __LINE__, chn, *new_width, *new_height, SPSInfo.profile_idc);

		FWK_SetOverflowState(chn, EXP_PROFILE_ERR);
		ALM_STATUS_CHG_ST status;
		status.AlmCnt = 1;
		SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_MMI, 0, 0, MSG_ID_ALM_STATUS_CHG, &status, sizeof(status));

		deluser_used use;
		use.channel = chn;
		SN_SendMessageEx(SUPER_USER_ID, MOD_NTRANS, MOD_NTRANS, 0, 0, MSG_ID_NTRANS_DELUSER_REQ,(void*)&use,sizeof(deluser_used));
		VochnInfo[chn].u32RefFrameNum = SPSInfo.ref_frame_count;
		return -2;
	}


	if(SPSInfo.profile_idc > 66 && !VoChnState.FirstHaveVideoData[chn] && VochnInfo[index].bIsPBStat && PlayInfo.PlayBackState >= PLAY_INSTANT)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "%s line: %d, chn: %d, new_width: %d, new_height: %d, profile_idc=%d\n",
			__FUNCTION__, __LINE__, chn, *new_width, *new_height, SPSInfo.profile_idc);

		Ftpc_End_Req endreq;
		endreq.result = SN_ERR_FTPC_VINOSUPORT_ERROR;
		endreq.channel = chn + 1;
		endreq.PlayType = PlayInfo.PlayBackState > PLAY_INSTANT ? 2 : 1;

		SendMessageEx(SUPER_USER_ID,MOD_PRV,MOD_FWK,0,0,MSG_ID_FTPC_END_REQ,&endreq,sizeof(Ftpc_End_Req));
		VochnInfo[chn].u32RefFrameNum = SPSInfo.ref_frame_count;
		return -2;
	}
#endif

	if(((*new_width) * (*new_height) < 176 * 144) && *new_width >= 0 && *new_height >= 0)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV,"%s line: %d, chn: %d, new_width: %d, new_height: %d, VochnInfo[index].VideoInfo.width: %d, VochnInfo[index].VideoInfo.height: %d\n",
			__FUNCTION__, __LINE__, chn, *new_width, *new_height, VochnInfo[index].VideoInfo.width, VochnInfo[index].VideoInfo.height);

		*new_width = VochnInfo[index].VideoInfo.width;
		*new_height = VochnInfo[index].VideoInfo.height;
		VochnInfo[chn].u32RefFrameNum = SPSInfo.ref_frame_count;
		return HI_SUCCESS;
	}
	else
	{
		//���ݷֱ��������ͨ���ֱ��ʲ�һ��
		if(*new_height != VochnInfo[index].VideoInfo.height || *new_width != VochnInfo[index].VideoInfo.width||VochnInfo[chn].u32RefFrameNum != SPSInfo.ref_frame_count)
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "%s line: %d, chn: %d, new_width: %d, new_height: %d, VochnInfo[index].VideoInfo.width: %d, VochnInfo[index].VideoInfo.height: %d\n",
				__FUNCTION__, __LINE__, chn, *new_width, *new_height, VochnInfo[index].VideoInfo.width, VochnInfo[index].VideoInfo.height);
			VochnInfo[chn].u32RefFrameNum = SPSInfo.ref_frame_count;
			return HI_FAILURE;
		}
	}

	return HI_SUCCESS;
}

//����������������
//�����Ľ������У�̫������δ��ʱ��������Ƶ��ʱ̫�󣬵��ô˽ӿ��������������
HI_VOID PRV_ReStarVdec(VDEC_CHN VdecChn)
{
	CHECK(HI_MPI_VDEC_StopRecvStream(VdecChn));//����ͨ��ǰ��ֹͣ��������

	CHECK(HI_MPI_VDEC_ResetChn(VdecChn)); //��������λ 
	
	CHECK(HI_MPI_VDEC_StartRecvStream(VdecChn)); 	
}

/********************************************************
������:PRV_StartAdec
��     ��:������Ƶ����ͨ��
��     ��:[in]AdecEntype ָ����������
		   [in]AdecChn ��Ƶ����ͨ����
����ֵ:  0�ɹ�
		    -1ʧ��

*********************************************************/
HI_S32 PRV_StartAdec(PAYLOAD_TYPE_E AdecEntype, ADEC_CHN AdecChn)
{
	ADEC_CHN_ATTR_S stAdecAttr;
	stAdecAttr.enType = AdecEntype;
	
	if(AdecEntype == PT_PCMA)
		stAdecAttr.enType = PT_G711A;
	else if(AdecEntype == PT_PCMU)
		stAdecAttr.enType = PT_G711U;
	
	stAdecAttr.u32BufSize = 20;
	stAdecAttr.enMode = ADEC_MODE_PACK;/* propose use pack mode in your app */
	switch(stAdecAttr.enType)
	{
		case PT_ADPCMA:
		{
			ADEC_ATTR_ADPCM_S stAdpcm;
			stAdecAttr.pValue = &stAdpcm;
			stAdpcm.enADPCMType = AUDIO_ADPCM_TYPE ;
		}
			break;
		case PT_G711A:
		case PT_G711U:
		{
			ADEC_ATTR_G711_S stAdecG711;
			stAdecAttr.pValue = &stAdecG711;		
		}
			break;
		case PT_G726:
		{
			ADEC_ATTR_G726_S stAdecG726;
			stAdecAttr.pValue = &stAdecG726;
			stAdecG726.enG726bps = G726_BPS ;      
		}
			break;
#if defined(SN9234H1)
		case PT_AMR:
		{
			ADEC_ATTR_AMR_S stAdecAmr;
			stAdecAttr.pValue = &stAdecAmr;
			stAdecAmr.enFormat = AMR_FORMAT;
		}
			break;
		case PT_AAC:
		{
			ADEC_ATTR_AAC_S stAdecAac;
			stAdecAttr.pValue = &stAdecAac;
			stAdecAttr.enMode = ADEC_MODE_STREAM;
		}
			break;
#endif
		case PT_LPCM:
		{
			ADEC_ATTR_LPCM_S stAdecLpcm;
			stAdecAttr.pValue = &stAdecLpcm;
			stAdecAttr.enMode = ADEC_MODE_PACK;
		}
			break;
		default:
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "invalid aenc payload type: %d\n", stAdecAttr.enType);
			return HI_FAILURE;
		} 
	}
	CHECK(HI_MPI_ADEC_CreateChn(AdecChn, &stAdecAttr));
	

	return HI_SUCCESS;

}

/********************************************************
������:PRV_StartAdecAo
��     ��:������Ƶ����ͨ��
��     ��:[in]AdecEntype ָ����������
		   [in]AdecChn ��Ƶ����ͨ����
����ֵ:  0�ɹ�
		    -1ʧ��

*********************************************************/
HI_S32 PRV_StartAdecAo(g_PRVVoChnInfo playInfo)
{
#if defined(Hi3531)
	AUDIO_DEV AoDev = 4;
#else
	AUDIO_DEV AoDev = 0;
#endif
	AO_CHN AoChn = 0;
	AIO_ATTR_S stAioAttr;
	PAYLOAD_TYPE_E AdecEntype;
	AdecEntype = playInfo.AudioInfo.adoType;
	//AdecEntype = PT_G711A;
	CHECK(HI_MPI_AO_DisableChn(AoDev, AoChn));	
    CHECK(HI_MPI_AO_Disable(AoDev));

	stAioAttr.enWorkmode = I2S_WORK_MODE;      /* �������� */
	stAioAttr.u32ChnCnt = 2;
	//stAioAttr.enBitwidth = playInfo.AudioInfo.bitwide;
	stAioAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
	//stAioAttr.enSamplerate = playInfo.AudioInfo.samrate;	
	stAioAttr.enSamplerate = AUDIO_SAMPLE_RATE_8000;
	//stAioAttr.enSoundmode = AUDIO_SOUND_MODE_MOMO;
	stAioAttr.enSoundmode = playInfo.AudioInfo.soundchannel;
	stAioAttr.u32EXFlag = 1;
	stAioAttr.u32FrmNum = 20;
	
	if(playInfo.AudioInfo.PtNumPerFrm == 160 || playInfo.AudioInfo.PtNumPerFrm == 320)
	{
		stAioAttr.u32PtNumPerFrm = playInfo.AudioInfo.PtNumPerFrm;
	}
	else
	{
		stAioAttr.u32PtNumPerFrm = 320;
		playInfo.AudioInfo.PtNumPerFrm = 320;
	}
	//stAioAttr.u32PtNumPerFrm = 160;
	stAioAttr.u32ClkSel = 0;

	CHECK(HI_MPI_AO_SetPubAttr(AoDev, &stAioAttr));
	
	CHECK(HI_MPI_AO_Enable(AoDev));

	CHECK(HI_MPI_AO_EnableChn(AoDev, AoChn));

	CHECK(PRV_StartAdec(AdecEntype, DecAdec));

	//CHECK_RET(PRV_StartAdec(PT_G711A, DecAdec));
	
#if defined(SN9234H1)
	CHECK(HI_MPI_AO_BindAdec(AoDev, AoChn, DecAdec));
#else	
	if(IsAdecBindAo == 0)
	{
		CHECK(PRV_AUDIO_AoBindAdec(AoDev, AoChn, DecAdec));
		IsAdecBindAo = 1;
	}
	
#endif	
	PtNumPerFrm = playInfo.AudioInfo.PtNumPerFrm;
	Prv_Audiobuf.length = 0;
	return HI_SUCCESS;

}

/********************************************************
������:PRV_StopAdec
��     ��:ֹͣ��Ƶ����ͨ��
��     ��:[in]AdecChn ��Ƶ����ͨ����
����ֵ:  0�ɹ�
		    -1ʧ��

*********************************************************/
HI_S32 PRV_StopAdec()
{	
#if defined(Hi3531)
	AUDIO_DEV AoDev = 4;
#else
	AUDIO_DEV AoDev = 0;
#endif
	AO_CHN AoChn = 0;
	
	//CHECK_RET(HI_MPI_ADEC_ClearChnBuf(DecAdec));

	CHECK(HI_MPI_AO_ClearChnBuf(AoDev, AoChn));
#if defined(SN9234H1)
	CHECK(HI_MPI_AO_UnBindAdec(AoDev, AoChn, DecAdec));
#else	
	if(IsAdecBindAo == 1)
	{
		CHECK(PRV_AUDIO_AoUnbindAdec(AoDev, AoChn, DecAdec));
		IsAdecBindAo = 0;
	}
#endif		
	CHECK(HI_MPI_AO_DisableChn(AoDev, AoChn));
	
    CHECK(HI_MPI_AO_Disable(AoDev));
	
	CHECK(HI_MPI_ADEC_DestroyChn(DecAdec));
	//PtNumPerFrm = 0;
	Prv_Audiobuf.length = 0;
	return HI_SUCCESS;
}


//��ʼ��ÿ��ͨ����ͨ����Ϣ
void PRV_InitVochnInfo(int chn)
{
	int i = 0;
	if(chn < 0 || chn >= DEV_CHANNEL_NUM)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_InitVochnInfo---------invalid channel: %d!!line: %d\n", chn, __LINE__);
		return;
	}
	if(chn < LOCALVEDIONUM)//����ģ��ͨ��
	{
		VochnInfo[chn].IsLocalVideo = 1;
		VochnInfo[chn].VdecChn = -1;
		if(chn >= DEV_CHANNEL_NUM/PRV_CHIP_NUM)
			VochnInfo[chn].SlaveId = PRV_SLAVE_1;
		else
			VochnInfo[chn].SlaveId = PRV_MASTER;			
	}
	else//����ͨ��
	{
		VochnInfo[chn].IsLocalVideo = 0;
		if(SCM_ChnConfigState(VochnInfo[chn].VoChn) == 0)
		{
			VochnInfo[chn].VdecChn = NoConfig_VdecChn;	
		}
		else
		{
			if(1)
			{
				VochnInfo[chn].VdecChn = DetVLoss_VdecChn;	//����ͨ������ʼ��Ĭ��"δ����"��"��������Ƶ"
			}
		}
		//��������͵�λ�����£������ӵ�ͨ��Ĭ�϶��Ǻ�
		if(PRV_CurDecodeMode == PassiveDecode || ScmGetListCtlState() == 1)
		{
			VochnInfo[chn].VdecChn = NoConfig_VdecChn;
		}
		VochnInfo[chn].VdecChn = NoConfig_VdecChn;
		VochnInfo[chn].SlaveId = PRV_MASTER;
	}
	
	VochnInfo[chn].VoChn = chn;
	VochnInfo[chn].CurChnIndex = VochnInfo[chn].VoChn - LOCALVEDIONUM;
	VochnInfo[chn].VideoInfo.vdoType= H264ENC;
	VochnInfo[chn].VideoInfo.framerate = 0;
	VochnInfo[chn].VideoInfo.height= 0;
	VochnInfo[chn].VideoInfo.width = 0;
	VochnInfo[chn].AudioInfo.adoType = -1;
	VochnInfo[chn].AudioInfo.samrate = 0;
	VochnInfo[chn].AudioInfo.soundchannel = 0;
	VochnInfo[chn].AudioInfo.PtNumPerFrm = 160;
	//	VochnInfo[chn].AudioInfo.bitwide = 0;
	for(i = 0; i < PRV_VO_DEV_NUM; i++)
		VochnInfo[chn].IsBindVdec[i] = -1;

	VochnInfo[chn].IsHaveVdec = 0;
	VochnInfo[chn].IsConnect = 0;
	VochnInfo[chn].PrvType = 0;		
	VochnInfo[chn].bIsStopGetVideoData= 0;
	VochnInfo[chn].VdecCap = 0;
	VochnInfo[chn].bIsWaitIFrame = 0;
	VochnInfo[chn].bIsDouble = 0;
	VochnInfo[chn].bIsWaitGetIFrame = 0;
	VochnInfo[chn].MccCreateingVdec = 0;
	VochnInfo[chn].MccReCreateingVdec = 0;
	VochnInfo[chn].IsChooseSlaveId = 0;
	VochnInfo[chn].IsDiscard = 0;
	VochnInfo[chn].bIsPBStat = 0;
}

//��ʼ������ͨ����Ϣ
void PRV_SendInfoInit(int chn)
{
	if(chn < 0 || chn >= MAX_IPC_CHNNUM)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_SendInfoInit---------invalid channel: %d!!line: %d\n", chn, __LINE__);
		return;
	}	
	SendChnInfo.pre_frame[chn] = 0;
	SendChnInfo.is_same[chn] = 1;
	SendChnInfo.is_first[chn] = 1;
	SendChnInfo.is_disgard[chn] = 0;
	SendChnInfo.TotalLength1[chn] = 0;
	SendChnInfo.VideoDataStream1[chn] = NULL;
	SendChnInfo.VideoDataStream2[chn] = NULL;
	
}

//��ʼ��ÿ��ͨ�����ݵ�ʱ�����Ϣ
void PRV_PtsInfoInit(int chn)
{
	if(chn < 0 || chn >= MAX_IPC_CHNNUM)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_PtsInfoInit---------invalid channel: %d!!line: %d\n", chn, __LINE__);
		return;
	}	
	SN_MEMSET(&PtsInfo[chn], 0, sizeof(g_PRVPtsinfo));
	PtsInfo[chn].IntervalPts = PalDec_IntervalPts;
	PtsInfo[chn].FirstVideoPts = (~0x0);
	PtsInfo[chn].FirstAudioPts = (~0x0);
}

void PRV_VoChnStateInit(int chn)
{
	if(chn < 0 || chn >= MAX_IPC_CHNNUM)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_PtsInfoInit---------invalid channel: %d!!line: %d\n", chn, __LINE__);
		return;
	}	
	VoChnState.FirstHaveVideoData[chn] = 0;
	VoChnState.FirstHaveAudioData[chn] = 0;
	VoChnState.FirstGetData[chn] = 0;
	VoChnState.IsGetFirstData[chn] = 0;
	VoChnState.IsStopGetVideoData[chn] = 0;
	VoChnState.BeginSendData[chn] = 0;	
	VoChnState.bIsPBStat_StopWriteData[chn] = 0;
	VoChnState.bIsPBStat_BeyondCap[chn] = 0;
	VoChnState.VideoDataCount[chn] = 0;
	VoChnState.AudioDataCount[chn] = 0;
	VoChnState.SendAudioDataCount[chn] = 0;
	VoChnState.VideoDataTimeLag[chn] = 0;
	VoChnState.AudioDataTimeLag[chn] = 0;
}

void PRV_PBStateInfoInit(int chn)
{	
	SN_MEMSET(&PlayStateInfo[chn], 0, sizeof(g_ChnPlayStateInfo));	
	PlayStateInfo[chn].CurPlayState = DEC_STATE_EXIT;
	PlayStateInfo[chn].CurSpeedState = DEC_SPEED_NORMAL;		
	PlayStateInfo[chn].SendDataType = TYPE_NORMAL;	
	PlayStateInfo[chn].RealType = DEC_TYPE_REAL; 
	PlayStateInfo[chn].SynState = SYN_NOPLAY; 	
}
void PRV_PBPlayInfoInit()
{   
	SN_MEMSET(&PlayInfo, 0, sizeof(PlayInfo));
	PlayInfo.PlayBackState = PLAY_EXIT;
}


/********************************************************
������:PRV_DECInfoInit
��     ��:��ʼ�������߳���Ҫʹ�õ��ļ���ȫ�ֱ���
��     ��:��
����ֵ:  ��

*********************************************************/
void PRV_DECInfoInit()
{
	HI_S32 i = 0;

	for(i = 0; i < DEV_CHANNEL_NUM; i++)
		PRV_InitVochnInfo(i);
	
	for(i = 0; i < MAX_IPC_CHNNUM; i++)
	{
		PRV_PtsInfoInit(i);
		PRV_VoChnStateInit(i);
		PRV_SendInfoInit(i);
	}
	for(i = 0; i < DEV_CHANNEL_NUM; i++)
	{
		SlaveState.SlaveCreatingVdec[i] = 0;
		SlaveState.SlaveIsDesingVdec[i] = 0;
		PRV_PBStateInfoInit(i);
	}
	PRV_PBPlayInfoInit();
}

//���ݷֱ��ʴ�С����˷ֱ��ʵ�����(���D1)
//ÿ��оƬ���֧��ͬʱ��8��D1
HI_S32 PRV_ComPareToD1(HI_S32 Width, HI_S32 Height)
{
	HI_S32 MulD1 = -1, TmpPixel = 0;
	TmpPixel = Width * Height;
	
	if(TmpPixel <= QCIF)
	{
		MulD1 = QCIF;
	}
	else if(TmpPixel <= CIF)
	{
		MulD1 = CIF;
	}
	else if(TmpPixel <= D1)
	{
		MulD1 = D1;
	}
	else if(TmpPixel <= HIGH_SEVEN || TmpPixel == UXGA)
	{
		MulD1 = HIGH_SEVEN;
	}
	else if(TmpPixel <= HIGH_NINE)
	{
		MulD1 = HIGH_NINE;
	}
	else if(TmpPixel <= HIGH_TEN)
	{
		MulD1 = HIGH_TEN;
	}
	else
	{
		MulD1 = MAX_8D1;
	}
	
	return MulD1;
	

}

HI_S32 PRV_ComPare(HI_S32 TmpPixel)
{
	HI_S32 MulD1 = -1;
	
	if(TmpPixel <= QCIF)
	{
		MulD1 = QCIF;
	}
	else if(TmpPixel <= CIF)
	{
		MulD1 = CIF;
	}
	else if(TmpPixel <= D1)
	{
		MulD1 = D1;
	}
	else if(TmpPixel <= HIGH_SEVEN || TmpPixel == UXGA)
	{
		MulD1 = HIGH_SEVEN;
	}
	else if(TmpPixel <= MAX_3D1)
	{
		MulD1 = MAX_3D1;
	}
	else if(TmpPixel <= HIGH_NINE)
	{
		MulD1 = HIGH_NINE;
	}
	else if(TmpPixel <= HIGH_TEN)
	{
		MulD1 = HIGH_TEN;
	}
	else
	{
		MulD1 = MAX_8D1;
	}
	
	return MulD1;
	

}



/********************************************************
������:PRV_SetVochnInfo
��     ��:���ݻ�ȡ��SDP��Ϣ���ý���ͨ����״̬��Ϣ�ṹ��
��     ��:[out] ����ͨ����״̬��Ϣ�ṹ��
		   [in]SDP��Ϣ
����ֵ:  0�ɹ�
		    -1ʧ��

*********************************************************/
HI_S32 PRV_SetVochnInfo(g_PRVVoChnInfo *chnInfo, RTSP_C_SDPInfo *RTSP_SDP)
{
	chnInfo->VideoInfo.vdoType = RTSP_SDP->vdoType;
	chnInfo->VideoInfo.framerate = RTSP_SDP->framerate;
	
	chnInfo->AudioInfo.adoType = RTSP_SDP->adoType;
	//chnInfo->AudioInfo.bitwide = RTSP_SDP->bitwide;
	chnInfo->AudioInfo.samrate = RTSP_SDP->samrate;
	chnInfo->AudioInfo.soundchannel = RTSP_SDP->soundchannel;

	if(chnInfo->VideoInfo.height == 0 || chnInfo->VideoInfo.width == 0)
	{
		chnInfo->VideoInfo.height = RTSP_SDP->high == 0 ? 702 : RTSP_SDP->high;
		chnInfo->VideoInfo.width= RTSP_SDP->width == 0 ? 576 : RTSP_SDP->width;
	}
	
	//chnInfo->VdecCap = MulQcif;
#if(IS_DECODER_DEVTYPE == 0)
	HI_S32 MulD1 = PRV_ComPareToD1(RTSP_SDP->high, RTSP_SDP->width);
	if(MulD1 <= 0)
		return HI_FAILURE;
		
	chnInfo->VideoInfo.height = RTSP_SDP->high == 0 ? 702 : RTSP_SDP->high;
	chnInfo->VideoInfo.width= RTSP_SDP->width == 0 ? 576 : RTSP_SDP->width;
	
	if(RTSP_SDP->high <= 64 && RTSP_SDP->high > 0)
		chnInfo->VideoInfo.height = 64;
	
	if(RTSP_SDP->width <= 64 && RTSP_SDP->width > 0)
		chnInfo->VideoInfo.width = 64;
	//chnInfo->VdecCap = (RTSP_SDP->high * RTSP_SDP->width);
	chnInfo->VdecCap = MulD1;
	//���ĳһͨ�����ܱȵ�Ƭ�����ܴ�������ͨ��
	if(chnInfo->VdecCap  > TOTALCAPPCHIP)
	{	
		TRACE(SCI_TRACE_HIGH, MOD_PRV, "----------Discard the channel: VdecCap---%d\n", chnInfo->VdecCap);
		//chnInfo->CurChnIndex = -1;
		chnInfo->VdecCap = 0;
		return HI_FAILURE;
	}			
#endif
	return HI_SUCCESS;
}

/********************************************************
������:PRV_ChooseSlaveId
��     ��:ѡ��Ԥ��ָ��ͨ����IPC ��Ƶ��оƬ(��Ƭ���Ƭ)
��     ��:[in] IPCͨ��
		   [out]
����ֵ:  0�ɹ�
		    -1ʧ��

*********************************************************/
HI_S32 PRV_ChooseSlaveId(int chn, PRV_MccCreateVdecReq *SlaveCreateVdec)
{
	//���ܺϷ����ж�
	//if(VochnInfo[chn].VdecCap <= 0 || VochnInfo[chn].VdecCap > TOTALCAPPCHIP)
	//	return HI_FAILURE;
	
	//HI_S32 MulD1 = PRV_ComPareToD1(VochnInfo[chn].VideoInfo.height, VochnInfo[chn].VideoInfo.width);
	HI_S32 MulD1 = VochnInfo[chn].VideoInfo.height * VochnInfo[chn].VideoInfo.width;
	if(MulD1 <= 0)
		return HI_FAILURE;
	VochnInfo[chn].VdecCap = MulD1;
#if defined(SN9234H1)	
	CurCap = CurMasterCap + CurSlaveCap;
#else
	CurCap = CurMasterCap;
#endif
	CurCap += VochnInfo[chn].VdecCap;
	TRACE(SCI_TRACE_HIGH, MOD_PRV, "line:%d The current cap: %d, master cap: %d,  slave cap: %d, the new chn---%d, VdecCap: %d\n", __LINE__, CurCap, CurMasterCap, CurSlaveCap, chn, VochnInfo[chn].VdecCap);
#if 1
	//��������ѳ��������ܣ��Ե�ǰ��ͨ��������	
	if(CurCap > (TOTALCAPPCHIP * DEV_CHIP_NUM - LOCALVEDIONUM * 1))
	{
		TRACE(SCI_TRACE_HIGH, MOD_PRV,"The current capbility: %d is beyond the TOTALCAP, discard the newest channel---%d, VdecCap: %d, CurSlaveCap: %d, CurMasterCap: %d\n", CurCap, chn, VochnInfo[chn].VdecCap, CurSlaveCap, CurMasterCap);
		CurCap -= VochnInfo[chn].VdecCap;
		//VochnInfo[chn].CurChnIndex = -1;
		//VochnInfo[chn].VdecCap = 0;
		return HI_FAILURE;
	}
#endif	
	
#if 0	
	CurIPCCount++;
	if(CurIPCCount > MAX_IPC_CHNNUM)
	{
		//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "++++++++++++Link Too Many IPC!\n");	
		//return HI_FAILURE;
	}
	TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Choose master chip!\n");			
	VochnInfo[chn].CurChnIndex = chn - LOCALVEDIONUM;
	VochnInfo[chn].SlaveId = PRV_MASTER;
	VochnInfo[chn].VoChn = chn; 
	//�ڴ˸�������������ֵ����Ϊ:�����ǰ�ڻط�״̬�����µ����ӣ���ʱ�޷�
	//��������ͨ������Ҫ�ȱ�����Ϣ�����˳��طź󣬻������Щ��Ϣ��������ͨ��
	VochnInfo[chn].IsConnect = 1;
	VochnInfo[chn].VdecChn = VochnInfo[chn].VoChn;	
	
#else	

#if (DEV_TYPE == DEV_SN_9234_H_1)

#if 0
	if(CurMasterCap == 0 && DEV_CHIP_NUM > 1 && CurSlaveCap == 0) //��δ����ͨ��ʱ��������Ƭ
	{
		//���ȷ���Ƭ����
		if((CurMasterCap + VochnInfo[chn].VdecCap) <= TOTALCAPPCHIP)
		{
			VochnInfo[chn].CurChnIndex = chn - LOCALVEDIONUM;
			VochnInfo[chn].SlaveId = PRV_MASTER;
			VochnInfo[chn].VoChn = chn;	
			//�ڴ˸�������������ֵ����Ϊ:�����ǰ�ڻط�״̬�����µ����ӣ���ʱ�޷�
			//��������ͨ������Ҫ�ȱ�����Ϣ�����˳��طź󣬻������Щ��Ϣ��������ͨ��
			VochnInfo[chn].IsConnect = 1;
			//VochnInfo[chn].VdecChn = VochnInfo[chn].VoChn;	
			SlaveCreateVdec->SlaveId = PRV_MASTER;
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line:%d Choose master chip!\n", __LINE__);			
		}	
		else
		{
			CurCap -= VochnInfo[chn].VdecCap;
			TRACE(SCI_TRACE_HIGH, MOD_PRV, "line:%d The current cap: %d, master cap: %d,  slave cap: %d, the new chn---%d, VdecCap: %d\n", __LINE__, CurCap, CurMasterCap, CurSlaveCap, chn, VochnInfo[chn].VdecCap);
			//PRV_InitVochnInfo(chn);
			return HI_FAILURE;
			
		}
	}
	else if(CurMasterCap > 0 && DEV_CHIP_NUM > 1 && CurSlaveCap == 0) //��Ƭ�Ѿ���ͨ�������Դ�Ƭ����
	{
		if(DEV_CHIP_NUM > 1 && CurSlaveCap <= TOTALCAPPCHIP && (CurSlaveCap + VochnInfo[chn].VdecCap) <= TOTALCAPPCHIP)
		{				
			VochnInfo[chn].SlaveId = PRV_SLAVE_1;
			VochnInfo[chn].CurChnIndex = chn - LOCALVEDIONUM;
			VochnInfo[chn].VoChn = chn;
			VochnInfo[chn].IsConnect = 1;
			CurSlaveCap += VochnInfo[chn].VdecCap;
			CurSlaveChnCount++;
			//��Ƭ��������ͨ����Ϣ
			SlaveCreateVdec->SlaveId = PRV_SLAVE_1;	
			SlaveCreateVdec->s32StreamChnIDs = MasterToSlaveChnId;
			SlaveCreateVdec->EncType = VochnInfo[chn].VideoInfo.vdoType;
			SlaveCreateVdec->chn = VochnInfo[chn].CurChnIndex;
			SlaveCreateVdec->VoChn = VochnInfo[chn].VoChn;
			SlaveCreateVdec->VdecChn = VochnInfo[chn].VoChn;
			SlaveCreateVdec->VdecCap = VochnInfo[chn].VdecCap;	
			SlaveCreateVdec->height = VochnInfo[chn].VideoInfo.height;
			SlaveCreateVdec->width = VochnInfo[chn].VideoInfo.width;		
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line:%d Choose Slave chip! chn: %d ,VdecCap: %d, CurSlaveCap: %d, CurSlaveChnCount: %d\n", __LINE__, chn, VochnInfo[chn].VdecCap, CurSlaveCap, CurSlaveChnCount);
		}
		else
		{
			CurCap -= VochnInfo[chn].VdecCap;
			TRACE(SCI_TRACE_HIGH, MOD_PRV, "line:%d The current cap: %d, master cap: %d,  slave cap: %d, the new chn---%d, VdecCap: %d\n", __LINE__, CurCap, CurMasterCap, CurSlaveCap, chn, VochnInfo[chn].VdecCap);
			return HI_FAILURE;
			
		}
	}
	else
	{
		if(((CurMasterCap + VochnInfo[chn].VdecCap * 2) <= TOTALCAPPCHIP) && (DEV_CHIP_NUM > 1 && CurSlaveCap <= TOTALCAPPCHIP && (CurSlaveCap + VochnInfo[chn].VdecCap * 2) <= TOTALCAPPCHIP))
		{
			if(CurMasterCap < CurSlaveCap)
			{
				VochnInfo[chn].CurChnIndex = chn - LOCALVEDIONUM;
				VochnInfo[chn].SlaveId = PRV_MASTER;
				VochnInfo[chn].VoChn = chn;	
				VochnInfo[chn].IsConnect = 1;
				SlaveCreateVdec->SlaveId = PRV_MASTER;
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line:%d Choose master chip!\n", __LINE__);	
			}
			else
			{
				VochnInfo[chn].SlaveId = PRV_SLAVE_1;
				VochnInfo[chn].CurChnIndex = chn - LOCALVEDIONUM;
				VochnInfo[chn].VoChn = chn;
				VochnInfo[chn].IsConnect = 1;
				CurSlaveCap += VochnInfo[chn].VdecCap;
				CurSlaveChnCount++;
				//��Ƭ��������ͨ����Ϣ
				SlaveCreateVdec->SlaveId = PRV_SLAVE_1;	
				SlaveCreateVdec->s32StreamChnIDs = MasterToSlaveChnId;
				SlaveCreateVdec->EncType = VochnInfo[chn].VideoInfo.vdoType;
				SlaveCreateVdec->chn = VochnInfo[chn].CurChnIndex;
				SlaveCreateVdec->VoChn = VochnInfo[chn].VoChn;
				SlaveCreateVdec->VdecChn = VochnInfo[chn].VoChn;
				SlaveCreateVdec->VdecCap = VochnInfo[chn].VdecCap;	
				SlaveCreateVdec->height = VochnInfo[chn].VideoInfo.height;
				SlaveCreateVdec->width = VochnInfo[chn].VideoInfo.width;		
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line:%d Choose Slave chip! chn: %d ,VdecCap: %d, CurSlaveCap: %d, CurSlaveChnCount: %d\n", __LINE__, chn, VochnInfo[chn].VdecCap, CurSlaveCap, CurSlaveChnCount);
			}
					
		}	
		else if(((CurMasterCap + VochnInfo[chn].VdecCap * 2) <= TOTALCAPPCHIP) || (DEV_CHIP_NUM > 1 && CurSlaveCap <= TOTALCAPPCHIP && (CurSlaveCap + VochnInfo[chn].VdecCap * 2) <= TOTALCAPPCHIP))
		{
			if((CurMasterCap + VochnInfo[chn].VdecCap * 2) <= TOTALCAPPCHIP)
			{
				VochnInfo[chn].CurChnIndex = chn - LOCALVEDIONUM;
				VochnInfo[chn].SlaveId = PRV_MASTER;
				VochnInfo[chn].VoChn = chn;	
				VochnInfo[chn].IsConnect = 1;
				SlaveCreateVdec->SlaveId = PRV_MASTER;
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line:%d Choose master chip!\n", __LINE__);	
			}
			else
			{
				VochnInfo[chn].SlaveId = PRV_SLAVE_1;
				VochnInfo[chn].CurChnIndex = chn - LOCALVEDIONUM;
				VochnInfo[chn].VoChn = chn;
				VochnInfo[chn].IsConnect = 1;
				CurSlaveCap += VochnInfo[chn].VdecCap;
				CurSlaveChnCount++;
				//��Ƭ��������ͨ����Ϣ
				SlaveCreateVdec->SlaveId = PRV_SLAVE_1;	
				SlaveCreateVdec->s32StreamChnIDs = MasterToSlaveChnId;
				SlaveCreateVdec->EncType = VochnInfo[chn].VideoInfo.vdoType;
				SlaveCreateVdec->chn = VochnInfo[chn].CurChnIndex;
				SlaveCreateVdec->VoChn = VochnInfo[chn].VoChn;
				SlaveCreateVdec->VdecChn = VochnInfo[chn].VoChn;
				SlaveCreateVdec->VdecCap = VochnInfo[chn].VdecCap;	
				SlaveCreateVdec->height = VochnInfo[chn].VideoInfo.height;
				SlaveCreateVdec->width = VochnInfo[chn].VideoInfo.width;		
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line:%d Choose Slave chip! chn: %d ,VdecCap: %d, CurSlaveCap: %d, CurSlaveChnCount: %d\n", __LINE__, chn, VochnInfo[chn].VdecCap, CurSlaveCap, CurSlaveChnCount);
			}
		}
		else if((CurMasterCap + VochnInfo[chn].VdecCap) <= TOTALCAPPCHIP && DEV_CHIP_NUM > 1 && CurSlaveCap <= TOTALCAPPCHIP && (CurSlaveCap + VochnInfo[chn].VdecCap) <= TOTALCAPPCHIP)
		{
			if(CurMasterCap < CurSlaveCap)
			{
				VochnInfo[chn].CurChnIndex = chn - LOCALVEDIONUM;
				VochnInfo[chn].SlaveId = PRV_MASTER;
				VochnInfo[chn].VoChn = chn;	
				VochnInfo[chn].IsConnect = 1;
				SlaveCreateVdec->SlaveId = PRV_MASTER;
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line:%d Choose master chip!\n", __LINE__);	
			}
			else
			{
				VochnInfo[chn].SlaveId = PRV_SLAVE_1;
				VochnInfo[chn].CurChnIndex = chn - LOCALVEDIONUM;
				VochnInfo[chn].VoChn = chn;
				VochnInfo[chn].IsConnect = 1;
				CurSlaveCap += VochnInfo[chn].VdecCap;
				CurSlaveChnCount++;
				//��Ƭ��������ͨ����Ϣ
				SlaveCreateVdec->SlaveId = PRV_SLAVE_1;	
				SlaveCreateVdec->s32StreamChnIDs = MasterToSlaveChnId;
				SlaveCreateVdec->EncType = VochnInfo[chn].VideoInfo.vdoType;
				SlaveCreateVdec->chn = VochnInfo[chn].CurChnIndex;
				SlaveCreateVdec->VoChn = VochnInfo[chn].VoChn;
				SlaveCreateVdec->VdecChn = VochnInfo[chn].VoChn;
				SlaveCreateVdec->VdecCap = VochnInfo[chn].VdecCap;	
				SlaveCreateVdec->height = VochnInfo[chn].VideoInfo.height;
				SlaveCreateVdec->width = VochnInfo[chn].VideoInfo.width;		
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line:%d Choose Slave chip! chn: %d ,VdecCap: %d, CurSlaveCap: %d, CurSlaveChnCount: %d\n", __LINE__, chn, VochnInfo[chn].VdecCap, CurSlaveCap, CurSlaveChnCount);
			}
		}
		else if(((CurMasterCap + VochnInfo[chn].VdecCap) <= TOTALCAPPCHIP) || (DEV_CHIP_NUM > 1 && CurSlaveCap <= TOTALCAPPCHIP && (CurSlaveCap + VochnInfo[chn].VdecCap) <= TOTALCAPPCHIP))
		{
			if((CurMasterCap + VochnInfo[chn].VdecCap) <= TOTALCAPPCHIP)
			{
				VochnInfo[chn].CurChnIndex = chn - LOCALVEDIONUM;
				VochnInfo[chn].SlaveId = PRV_MASTER;
				VochnInfo[chn].VoChn = chn;	
				VochnInfo[chn].IsConnect = 1;
				SlaveCreateVdec->SlaveId = PRV_MASTER;
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line:%d Choose master chip!\n", __LINE__);	
			}
			else
			{
				VochnInfo[chn].SlaveId = PRV_SLAVE_1;
				VochnInfo[chn].CurChnIndex = chn - LOCALVEDIONUM;
				VochnInfo[chn].VoChn = chn;
				VochnInfo[chn].IsConnect = 1;
				CurSlaveCap += VochnInfo[chn].VdecCap;
				CurSlaveChnCount++;
				//��Ƭ��������ͨ����Ϣ
				SlaveCreateVdec->SlaveId = PRV_SLAVE_1;	
				SlaveCreateVdec->s32StreamChnIDs = MasterToSlaveChnId;
				SlaveCreateVdec->EncType = VochnInfo[chn].VideoInfo.vdoType;
				SlaveCreateVdec->chn = VochnInfo[chn].CurChnIndex;
				SlaveCreateVdec->VoChn = VochnInfo[chn].VoChn;
				SlaveCreateVdec->VdecChn = VochnInfo[chn].VoChn;
				SlaveCreateVdec->VdecCap = VochnInfo[chn].VdecCap;	
				SlaveCreateVdec->height = VochnInfo[chn].VideoInfo.height;
				SlaveCreateVdec->width = VochnInfo[chn].VideoInfo.width;		
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line:%d Choose Slave chip! chn: %d ,VdecCap: %d, CurSlaveCap: %d, CurSlaveChnCount: %d\n", __LINE__, chn, VochnInfo[chn].VdecCap, CurSlaveCap, CurSlaveChnCount);
			}
		}
		else
		{
			CurCap -= VochnInfo[chn].VdecCap;
			TRACE(SCI_TRACE_HIGH, MOD_PRV, "line:%d The current cap: %d, master cap: %d,  slave cap: %d, the new chn---%d, VdecCap: %d\n", __LINE__, CurCap, CurMasterCap, CurSlaveCap, chn, VochnInfo[chn].VdecCap);
			return HI_FAILURE;
		}
	}
	
#else
	//���ȷ���Ƭ����
	if(((CurMasterCap + VochnInfo[chn].VdecCap) <= TOTALCAPPCHIP) && (DEV_CHIP_NUM > 1 && CurSlaveCap <= TOTALCAPPCHIP && (CurSlaveCap + VochnInfo[chn].VdecCap) <= TOTALCAPPCHIP))
	{
		if(CurMasterCap <= CurSlaveCap)
		{
			VochnInfo[chn].CurChnIndex = chn - LOCALVEDIONUM;
			VochnInfo[chn].SlaveId = PRV_MASTER;
			VochnInfo[chn].VoChn = chn;	
			//�ڴ˸�������������ֵ����Ϊ:�����ǰ�ڻط�״̬�����µ����ӣ���ʱ�޷�
			//��������ͨ������Ҫ�ȱ�����Ϣ�����˳��طź󣬻������Щ��Ϣ��������ͨ��
			VochnInfo[chn].IsConnect = 1;
			//VochnInfo[chn].VdecChn = VochnInfo[chn].VoChn;	
			SlaveCreateVdec->SlaveId = PRV_MASTER;
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Choose master chip!\n");
		}
		else
		{
			if(VochnInfo[chn].bIsPBStat)
			{
				CurCap -= VochnInfo[chn].VdecCap;
				TRACE(SCI_TRACE_HIGH, MOD_PRV, "line:%d The current cap: %d, master cap: %d,  slave cap: %d, the new chn---%d, VdecCap: %d\n", __LINE__, CurCap, CurMasterCap, CurSlaveCap, chn, VochnInfo[chn].VdecCap);
				return HI_FAILURE;
			}
			VochnInfo[chn].SlaveId = PRV_SLAVE_1;
			VochnInfo[chn].CurChnIndex = chn - LOCALVEDIONUM;
			VochnInfo[chn].VoChn = chn;
			//VochnInfo[chn].VdecChn = VochnInfo[chn].VoChn;
			VochnInfo[chn].IsConnect = 1;
			CurSlaveCap += VochnInfo[chn].VdecCap;
			CurSlaveChnCount++;
			//��Ƭ��������ͨ����Ϣ
			//SlaveCreateVdec->SlaveId = VochnInfo[chn].SlaveId;	
			SlaveCreateVdec->SlaveId = PRV_SLAVE_1;	
			SlaveCreateVdec->s32StreamChnIDs = MasterToSlaveChnId;
			SlaveCreateVdec->EncType = VochnInfo[chn].VideoInfo.vdoType;
			SlaveCreateVdec->chn = VochnInfo[chn].CurChnIndex;
			SlaveCreateVdec->VoChn = VochnInfo[chn].VoChn;
			SlaveCreateVdec->VdecChn = VochnInfo[chn].VoChn;
			SlaveCreateVdec->VdecCap = VochnInfo[chn].VdecCap;	
			SlaveCreateVdec->height = VochnInfo[chn].VideoInfo.height;
			SlaveCreateVdec->width = VochnInfo[chn].VideoInfo.width;		
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Choose Slave chip! chn: %d ,VdecCap: %d, CurSlaveCap: %d, CurSlaveChnCount: %d\n", chn, VochnInfo[chn].VdecCap, CurSlaveCap, CurSlaveChnCount);
		}
	}
	//���ȷ���Ƭ����
	else if((CurMasterCap + VochnInfo[chn].VdecCap) <= TOTALCAPPCHIP)
	{
		VochnInfo[chn].CurChnIndex = chn - LOCALVEDIONUM;
		VochnInfo[chn].SlaveId = PRV_MASTER;
		VochnInfo[chn].VoChn = chn;	
		//�ڴ˸�������������ֵ����Ϊ:�����ǰ�ڻط�״̬�����µ����ӣ���ʱ�޷�
		//��������ͨ������Ҫ�ȱ�����Ϣ�����˳��طź󣬻������Щ��Ϣ��������ͨ��
		VochnInfo[chn].IsConnect = 1;
		//VochnInfo[chn].VdecChn = VochnInfo[chn].VoChn;	
		SlaveCreateVdec->SlaveId = PRV_MASTER;
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Choose master chip!\n");			
	}	
	else if(DEV_CHIP_NUM > 1 && CurSlaveCap <= TOTALCAPPCHIP && (CurSlaveCap + VochnInfo[chn].VdecCap) <= TOTALCAPPCHIP
		/*&& (CurSlaveChnCount < DEV_CHANNEL_NUM/DEV_CHIP_NUM
		|| (CurSlaveChnCount >= DEV_CHANNEL_NUM/DEV_CHIP_NUM && CurMasterCap > TOTALCAPPCHIP))*/)
	{				
		if(VochnInfo[chn].bIsPBStat)
		{
			CurCap -= VochnInfo[chn].VdecCap;
			TRACE(SCI_TRACE_HIGH, MOD_PRV, "line:%d The current cap: %d, master cap: %d,  slave cap: %d, the new chn---%d, VdecCap: %d\n", __LINE__, CurCap, CurMasterCap, CurSlaveCap, chn, VochnInfo[chn].VdecCap);
			return HI_FAILURE;
		}
		VochnInfo[chn].SlaveId = PRV_SLAVE_1;
		VochnInfo[chn].CurChnIndex = chn - LOCALVEDIONUM;
		VochnInfo[chn].VoChn = chn;
		//VochnInfo[chn].VdecChn = VochnInfo[chn].VoChn;
		VochnInfo[chn].IsConnect = 1;
		CurSlaveCap += VochnInfo[chn].VdecCap;
		CurSlaveChnCount++;
		//��Ƭ��������ͨ����Ϣ
		//SlaveCreateVdec->SlaveId = VochnInfo[chn].SlaveId;	
		SlaveCreateVdec->SlaveId = PRV_SLAVE_1;	
		SlaveCreateVdec->s32StreamChnIDs = MasterToSlaveChnId;
		SlaveCreateVdec->EncType = VochnInfo[chn].VideoInfo.vdoType;
		SlaveCreateVdec->chn = VochnInfo[chn].CurChnIndex;
		SlaveCreateVdec->VoChn = VochnInfo[chn].VoChn;
		SlaveCreateVdec->VdecChn = VochnInfo[chn].VoChn;
		SlaveCreateVdec->VdecCap = VochnInfo[chn].VdecCap;	
		SlaveCreateVdec->height = VochnInfo[chn].VideoInfo.height;
		SlaveCreateVdec->width = VochnInfo[chn].VideoInfo.width;		
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Choose Slave chip! chn: %d ,VdecCap: %d, CurSlaveCap: %d, CurSlaveChnCount: %d\n", chn, VochnInfo[chn].VdecCap, CurSlaveCap, CurSlaveChnCount);
	}
	else
	{
		CurCap -= VochnInfo[chn].VdecCap;
		TRACE(SCI_TRACE_HIGH, MOD_PRV, "line:%d The current cap: %d, master cap: %d,  slave cap: %d, the new chn---%d, VdecCap: %d\n", __LINE__, CurCap, CurMasterCap, CurSlaveCap, chn, VochnInfo[chn].VdecCap);
		//PRV_InitVochnInfo(chn);
		return HI_FAILURE;
		
	}
#endif
	
#elif defined(Hi3531)||defined(Hi3535)//3531��������������Ƭ
	CurSlaveCap = TOTALCAPPCHIP;


	//���ȷŴ�Ƭ����
	//��Ƭ��������������:
	//��ƬԤ��ͨ����δ�ﵽ8ʱ(һ��)���Ŵ�Ƭ���룬��ֹ̫��Ԥ��ͨ���ڴ�Ƭ
	//��ƬԤ��ͨ�����ﵽ8�ǣ�����Ƭ���룬ֱ����Ƭ���ܲ���������Ƭ����ʣ������ʱ���Ŵ�Ƭ����
	if(DEV_CHIP_NUM > 1 && CurSlaveCap <= TOTALCAPPCHIP && (CurSlaveCap + VochnInfo[chn].VdecCap) <= TOTALCAPPCHIP
		&& (CurSlaveChnCount < DEV_CHANNEL_NUM/DEV_CHIP_NUM
		|| (CurSlaveChnCount >= DEV_CHANNEL_NUM/DEV_CHIP_NUM && CurMasterCap > TOTALCAPPCHIP)))
	{				
		VochnInfo[chn].SlaveId = PRV_SLAVE_1;
		VochnInfo[chn].CurChnIndex = chn - LOCALVEDIONUM;
		VochnInfo[chn].VoChn = chn;
		//VochnInfo[chn].VdecChn = VochnInfo[chn].VoChn;
		VochnInfo[chn].IsConnect = 1;
		CurSlaveCap += VochnInfo[chn].VdecCap;
		CurSlaveChnCount++;
		//��Ƭ��������ͨ����Ϣ
		//SlaveCreateVdec->SlaveId = VochnInfo[chn].SlaveId;	
		SlaveCreateVdec->SlaveId = PRV_SLAVE_1;	
		SlaveCreateVdec->s32StreamChnIDs = MasterToSlaveChnId;
		SlaveCreateVdec->EncType = VochnInfo[chn].VideoInfo.vdoType;
		SlaveCreateVdec->chn = VochnInfo[chn].CurChnIndex;
		SlaveCreateVdec->VoChn = VochnInfo[chn].VoChn;
		SlaveCreateVdec->VdecChn = VochnInfo[chn].VoChn;
		SlaveCreateVdec->VdecCap = VochnInfo[chn].VdecCap;	
		SlaveCreateVdec->height = VochnInfo[chn].VideoInfo.height;
		SlaveCreateVdec->width = VochnInfo[chn].VideoInfo.width;		
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Choose Slave chip! chn: %d ,VdecCap: %d, CurSlaveCap: %d, CurSlaveChnCount: %d\n", chn, VochnInfo[chn].VdecCap, CurSlaveCap, CurSlaveChnCount);
	}
	else if((CurMasterCap + VochnInfo[chn].VdecCap) <= TOTALCAPPCHIP)
	{
		VochnInfo[chn].CurChnIndex = chn - LOCALVEDIONUM;
		VochnInfo[chn].SlaveId = PRV_MASTER;
		VochnInfo[chn].VoChn = chn;	
		//�ڴ˸�������������ֵ����Ϊ:�����ǰ�ڻط�״̬�����µ����ӣ���ʱ�޷�
		//��������ͨ������Ҫ�ȱ�����Ϣ�����˳��طź󣬻������Щ��Ϣ��������ͨ��
		VochnInfo[chn].IsConnect = 1;
		//VochnInfo[chn].VdecChn = VochnInfo[chn].VoChn;	
		SlaveCreateVdec->SlaveId = PRV_MASTER;
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Choose master chip!\n");			
	}		
	else
	{
		CurCap -= VochnInfo[chn].VdecCap;
		TRACE(SCI_TRACE_HIGH, MOD_PRV, "The current cap: %d, master cap: %d,  slave cap: %d, the new chn---%d, VdecCap: %d\n", CurCap, CurMasterCap, CurSlaveCap, chn, VochnInfo[chn].VdecCap);
		//PRV_InitVochnInfo(chn);
		return HI_FAILURE;
		
	}
#endif	
#endif
	return HI_SUCCESS;
}


//��ȡ����Ƭ���ݴ����PCIͨ��	
void PRV_InitHostToSlaveStream()
{
	MasterToSlaveChnId = HostInitHostToSlaveStream(MOD_PRV, 1, PCIV_VDEC_STREAM_BUF_LEN);
	if (MasterToSlaveChnId <= 0)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "-------HostInitHostToSlaveStream error---0x%x\n", MasterToSlaveChnId);																																																																																																																																																																																																																																																								  
		MasterToSlaveChnId = 0;
	}
	else
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV,"-----HostInitHostToSlaveStream success---%d\n",  MasterToSlaveChnId);
	}
}


//����ȡ��������Ϣ�Ƿ�Ϸ�
HI_S32 PRV_CheckDataPara(AVPacket *DataFromRTSP)
{
	HI_S32 chn = 0;
	chn = DataFromRTSP->channel;
	if(chn < LOCALVEDIONUM || chn >= DEV_CHANNEL_NUM)
	{
		if(chn >= (DEV_CHANNEL_NUM * 4))
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "-------Invalid channel: %d, line: %d\n", chn, __LINE__);
		
		return HI_FAILURE;
	}
	
	if(DataFromRTSP->codec_id != CODEC_ID_H264 && DataFromRTSP->codec_id != CODEC_ID_PCMA && DataFromRTSP->codec_id != CODEC_ID_PCMU)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "-------Invalid data type: %d, chn: %d\n", DataFromRTSP->codec_id, chn);
		return HI_FAILURE;
	}
	
	return HI_SUCCESS;

}

//����Ӧͨ�����ݷֱ����Ƿ�ı䣬�ı�ʱ��Ҫ���´���������
//����ֵ:HI_FAILURE:��Ƭ����ʧ��
//			1:ѡ���Ƭ����
//			HI_SUCCESS:�ɹ�
int PRV_DetectResIsChg(int chn, AVPacket *DataStream)
{
	HI_S32 s32Ret = 0, s32Ret1 = 0, s32Ret2 = 0, index = 0,  new_width = 0, new_height = 0, NewVdecCap = 0; 
	HI_S32 aaaa = 0;
	PRV_MccCreateVdecReq	SlaveCreateVdecReq;
	PRV_MccReCreateVdecReq SlaveReCreateVdecReq;
	PlayBack_MccQueryStateReq QueryReq;
	RTSP_C_SDPInfo RTSP_SDP;
	if(DataStream == NULL)
	{
		return HI_FAILURE;
	}
	index = chn + LOCALVEDIONUM;
	//printf("===============PRV_DetectResIsChg===============11111111111111\n");
	s32Ret1 = PRV_SearchSPS(chn, DataStream, &new_width, &new_height);
	//aaaa = abs((DataStream->pts - PtsInfo[chn].PreVideoPts) * 1000/90000);
	//printf("==================aaaa: %d, DataStream->pts: %lld, PtsInfo[chn].PreVideoPts: %lld\n", aaaa, DataStream->pts, PtsInfo[chn].PreVideoPts);
	s32Ret2 = (aaaa > 8 * 1000) ? HI_FAILURE : HI_SUCCESS;
	if(s32Ret1 != HI_SUCCESS || s32Ret2 == HI_FAILURE)
		s32Ret = HI_FAILURE;
		
	TRACE(SCI_TRACE_NORMAL, MOD_DEC, "chn=%d, s32Ret: %d, s32Ret1: %d, s32Ret2: %d, IsHaveVdec: %d, bIsResetFile: %d, bIsPBStat: %d\n", chn, s32Ret, s32Ret1, s32Ret2, VochnInfo[index].IsHaveVdec, PlayStateInfo[index].bIsResetFile, VochnInfo[index].bIsPBStat);
	//�ط��£��л��ļ�ʱ��ԭ�����Ѿ����꣬��Ҫ���»�ȡ���ļ�SDP��Ϣ
	if(s32Ret == HI_FAILURE && VochnInfo[index].bIsPBStat && PlayStateInfo[index].bIsResetFile)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "===============PRV_DetectResIsChg===============4444444444444\n");
		if(FTPC_C_getParam(chn, &RTSP_SDP) != 0)
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_DetectResIsChg==========chn: %d, RTSP_C_getParam(chn, &RTSP_SDP) fail!!!=========\n", chn);		
			VochnInfo[index].bIsWaitIFrame = 1; 
			return HI_FAILURE;	
		}
		else
		{
			PRV_SetVochnInfo(&(VochnInfo[index]), &RTSP_SDP);
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "------width: %d, high: %d\n", RTSP_SDP.width, RTSP_SDP.high);
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "------FTPC_C_getParam() Success---vdoType: %d, adoType: %d, framerate: %d\n",
											VochnInfo[index].VideoInfo.vdoType, VochnInfo[index].AudioInfo.adoType, VochnInfo[index].VideoInfo.framerate);			
			//�����յ����ݵ�֮ǰ��Client��·�Ѿ��ϵ�(�л��ȽϿ�ʱ)
			if(VochnInfo[index].VideoInfo.vdoType == 0)
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_DetectResIsChg==========chn: %d, Allready Disconnect!\n",chn);			
				//VochnInfo[index].bIsWaitIFrame = 1;
				//return HI_FAILURE;	
				VochnInfo[index].VideoInfo.vdoType = PT_H264;
			}
		}
	}
	
	if(VochnInfo[index].IsHaveVdec && s32Ret == HI_FAILURE)//�ֱ��ʸı�
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "==================66666666666666666666666========\n");
		//	�ط�״̬�£��ֱ��ʱ仯��Ҫ�ؽ�����������Ҫ�ȴ�ԭ���ݽ���
		if(VochnInfo[index].bIsPBStat && !PlayStateInfo[index].bIsResetFile)
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "===============PRV_DetectResIsChg===============22222222222\n");
			if(VochnInfo[index].SlaveId > PRV_MASTER)
			{
				PlayStateInfo[chn].QuerySlaveId = 1;
				QueryReq.VoChn = chn;	
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "============Send MSG_ID_PRV_MCC_PBQUERYSTATE_REQ\n");
				SN_SendMccMessageEx(PRV_SLAVE_1, SUPER_USER_ID, MOD_PRV, MOD_PRV, 0, 0, MSG_ID_PRV_MCC_PBQUERYSTATE_REQ, &QueryReq, sizeof(QueryReq));
			}
			PlayStateInfo[index].bIsResetFile = 1;//�л��ļ���ʶ
			//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line: %d============Send: MSG_ID_FTPC_PB_CHNSTOPWRITE_IND---StopWrite: %d\n", __LINE__, ChnStopWriteInd.StopWrite);
			Ftpc_ChnStopWrite(chn, 1);					
			VoChnState.bIsPBStat_StopWriteData[chn] = 1;
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line: %d============bIsPBStat_StopWriteData---StopWrite==1\n", __LINE__);
			return 2;
		}
		
		//���豸�طţ�ԭ�����ѽ��꣬�������ļ�����ʱ����Ҫ��������
		if(PlayInfo.PlayBackState > PLAY_INSTANT)
		{
			//�ط�ʱ��һ֡����ʱ�����0
			VochnInfo[index].VideoInfo.height = new_height;
			VochnInfo[index].VideoInfo.width = new_width;
			PtsInfo[chn].PreVideoPts = DataStream->pts;
			PtsInfo[chn].CurVideoPts = DataStream->pts;
			PtsInfo[chn].PreSendPts = 0;
			PtsInfo[chn].CurSendPts = 0;
			PtsInfo[chn].FirstAudioPts = 0;
			PtsInfo[chn].FirstVideoPts = 0;
			PtsInfo[chn].CurVoChnPts = 0;
			PtsInfo[chn].PreVoChnPts = 0;
			DataStream->prvpts = PtsInfo[chn].CurSendPts;
			//TODO:���¼���������
			PlayBack_AdaptRealType();
			//TODO:�ؽ�������
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "===============PRV_DetectResIsChg===============3333333333VochnInfo[index].SlaveId: %d\n", VochnInfo[index].SlaveId );
			if(VochnInfo[index].SlaveId > PRV_MASTER)
			{
				PlayBack_MccPBReCreateVdecReq PBReCreateReq;
				PBReCreateReq.SlaveId = VochnInfo[index].SlaveId;
				PBReCreateReq.EncType = VochnInfo[index].VideoInfo.vdoType;
				PBReCreateReq.s32VdecHeight = VochnInfo[index].VideoInfo.height;
				PBReCreateReq.s32VdecWidth  = VochnInfo[index].VideoInfo.width;
				PBReCreateReq.VoChn = VochnInfo[index].VoChn;
				PBReCreateReq.VdecChn = VochnInfo[index].VdecChn;
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line: %d===============Send MSG_ID_PRV_MCC_PBRECREATEVDEC_REQ\n", __LINE__);
				SN_SendMccMessageEx(PRV_SLAVE_1, SUPER_USER_ID, MOD_PRV, MOD_PRV, 0, 0, MSG_ID_PRV_MCC_PBRECREATEVDEC_REQ, &PBReCreateReq, sizeof(PBReCreateReq));					
				VochnInfo[index].MccReCreateingVdec = 1;
				VochnInfo[index].IsHaveVdec = 0;
			}
			else
			{
				PlayBack_DestroyVdec(HD, VochnInfo[index].VdecChn);
				PlayBack_CreatVdec(HD, VochnInfo[index].VdecChn);
			}
			
		}
		else
		{
			VochnInfo[index].VideoInfo.height = new_height;
			VochnInfo[index].VideoInfo.width = new_width;
			
			//��ʱ�ط��£�������ʱ�������ݱ仯���ٻ�ȡ������Ϣ			
			if(VochnInfo[index].bIsPBStat)
			{
				if(FTPC_C_getParam(chn, &RTSP_SDP) != 0)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_DetectResIsChg==========chn: %d, RTSP_C_getParam(chn, &RTSP_SDP) fail!!!=========\n", chn);		
					VochnInfo[index].bIsWaitIFrame = 1; 
					return HI_FAILURE;	
				}				
				else
				{
					PRV_SetVochnInfo(&(VochnInfo[index]), &RTSP_SDP);
					PtsInfo[chn].PreVideoPts = DataStream->pts;
					PtsInfo[chn].CurVideoPts = DataStream->pts;
					PtsInfo[chn].PreSendPts = 0;
					PtsInfo[chn].CurSendPts = 0;
					PtsInfo[chn].FirstAudioPts = 0;
					PtsInfo[chn].FirstVideoPts = 0;
					PtsInfo[chn].CurVoChnPts = 0;
					PtsInfo[chn].PreVoChnPts = 0;
					VoChnState.VideoDataTimeLag[chn] = 0;
					VoChnState.AudioDataTimeLag[chn] = 0;
					VoChnState.FirstHaveVideoData[chn] = 0;
					VoChnState.FirstHaveAudioData[chn] = 0;
					DataStream->prvpts = PtsInfo[chn].CurSendPts;
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line:%d------FTPC_C_getParam() Success---vdoType: %d, adoType: %d\n", __LINE__, 
													VochnInfo[index].VideoInfo.vdoType, VochnInfo[index].AudioInfo.adoType);			
					//�����յ����ݵ�֮ǰ��Client��·�Ѿ��ϵ�(�л��ȽϿ�ʱ)
					if(VochnInfo[index].VideoInfo.vdoType == 0)
					{
						TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_DetectResIsChg==========chn: %d, Allready Disconnect!\n",chn);			
						//VochnInfo[index].bIsWaitIFrame = 1;
						//return HI_FAILURE;	
						VochnInfo[index].VideoInfo.vdoType = PT_H264;
					}
				}
			
			}
			//NewVdecCap = PRV_ComPareToD1(new_width, new_height);
			NewVdecCap = new_width * new_height;
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "SlaveId: %d, new_width=%d new_height=%d ReCreate Vdec: %d, OldVdecCap: %d, NewVdecCap: %d\n", VochnInfo[index].SlaveId, new_width, new_height, VochnInfo[index].VdecChn, VochnInfo[index].VdecCap, NewVdecCap);
			if(VochnInfo[index].SlaveId == PRV_MASTER)
			{
				//��Ƭ�ؽ�����ͨ��
				s32Ret = PRV_ReCreateVdecChn(chn, VochnInfo[index].VideoInfo.vdoType, new_height, new_width, VochnInfo[index].u32RefFrameNum, NewVdecCap);
				if(s32Ret == HI_FAILURE)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_ReCreateVdecChn == HI_FAILURE\n");
					return HI_FAILURE;								
				}
				//��ͨ������ѡ��Ŵ�Ƭ����
				else if(s32Ret == 1)
				{
					return 1;
				}
			}
			else if(VochnInfo[index].SlaveId > PRV_MASTER)
			{
				//֪ͨ��Ƭ�ؽ�����ͨ��
				
				CurCap = CurCap - VochnInfo[index].VdecCap + NewVdecCap; 
				if(CurCap > (TOTALCAPPCHIP * DEV_CHIP_NUM - LOCALVEDIONUM * 1))
				{
					if(VochnInfo[index].bIsPBStat)
					{
						Prv_Chn_ChnPBOverFlow_Ind ChnPbOver;
						SN_MEMSET(&ChnPbOver, 0, sizeof(Prv_Chn_ChnPBOverFlow_Ind));
						ChnPbOver.Chn = chn;
						ChnPbOver.NewWidth = new_width;
						ChnPbOver.NewHeight = new_height;							
						SN_SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_FWK, 0, 0, MSG_ID_PRV_CHNPBOVERFLOW_IND, &ChnPbOver, sizeof(Prv_Chn_ChnPBOverFlow_Ind));					
						VochnInfo[index].bIsWaitGetIFrame = 1;
					
						//return HI_FAILURE;
					}
				}
				SlaveReCreateVdecReq.SlaveId = VochnInfo[index].SlaveId;
				SlaveReCreateVdecReq.VoChn = VochnInfo[index].VoChn;
				SlaveReCreateVdecReq.VdecChn = VochnInfo[index].VdecChn;
				SlaveReCreateVdecReq.height = new_height;
				SlaveReCreateVdecReq.width = new_width;
				SlaveReCreateVdecReq.VdecCap = NewVdecCap;
				SlaveReCreateVdecReq.EncType = VochnInfo[index].VideoInfo.vdoType;
				VochnInfo[index].MccReCreateingVdec = 1;
				VochnInfo[index].IsHaveVdec = 0;
				CurSlaveCap = CurSlaveCap - VochnInfo[index].VdecCap + NewVdecCap;
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Send: MSG_ID_PRV_MCC_RECREATEVDEC_REQ---%d\n", SlaveReCreateVdecReq.VdecChn);
				SN_SendMccMessageEx(PRV_SLAVE_1, SUPER_USER_ID, MOD_PRV, MOD_PRV, 0, 0, MSG_ID_PRV_MCC_RECREATEVDEC_REQ, &SlaveReCreateVdecReq, sizeof(PRV_MccReCreateVdecReq));					
				//�����֡�ѻ�ȡ�����ݣ���Ƭ�ؽ�����������ȷ��ʹ�����
				return 1;
		
			}
		}
	}
	//δ����ʱ��ѡ������Ƭ����
	else if(!VochnInfo[index].IsHaveVdec)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "%s Line %d,===============PRV_DetectResIsChg===============55555555555\n",__func__,__LINE__);
		if(VochnInfo[index].bIsPBStat)
		{
			if(FTPC_C_getParam(chn, &RTSP_SDP) != 0)
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_DetectResIsChg==========chn: %d, RTSP_C_getParam(chn, &RTSP_SDP) fail!!!=========\n", chn);		
				VochnInfo[index].bIsWaitIFrame = 1; 
				return HI_FAILURE;	
			}				
			else
			{
				PRV_SetVochnInfo(&(VochnInfo[index]), &RTSP_SDP);
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "------FTPC_C_getParam() Success---vdoType: %d, adoType: %d\n",
												VochnInfo[index].VideoInfo.vdoType, VochnInfo[index].AudioInfo.adoType);			
				//�����յ����ݵ�֮ǰ��Client��·�Ѿ��ϵ�(�л��ȽϿ�ʱ)
				if(VochnInfo[index].VideoInfo.vdoType == 0)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_DetectResIsChg==========chn: %d, Allready Disconnect!\n",chn);			
					//VochnInfo[index].bIsWaitIFrame = 1;
					//return HI_FAILURE;	
					VochnInfo[index].VideoInfo.vdoType = PT_H264;
				}
			}

		}
		
		VochnInfo[index].VideoInfo.height = new_height;
		VochnInfo[index].VideoInfo.width = new_width;
		
		if(VochnInfo[index].IsChooseSlaveId == 0)
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "%s ===============line: %d\n", __func__,__LINE__);
			//���豸�ط��£�ѡ������Ƭ������VO���Է�ʽ��һ��
			if(PlayInfo.PlayBackState > PLAY_INSTANT)
			{
				PlayBack_ChooseSlaveId(chn, new_width, new_height, VochnInfo[index].VideoInfo.framerate);
				//TODO:����ط�����
				PlayBack_AdaptRealType();
				
			}
			else
			{
				s32Ret = PRV_ChooseSlaveId(chn, &SlaveCreateVdecReq);
				if(s32Ret != HI_SUCCESS)
				{
					if((CurCap + VochnInfo[chn].VdecCap) > (TOTALCAPPCHIP * DEV_CHIP_NUM - LOCALVEDIONUM * 1))
					{
						if(VochnInfo[index].bIsPBStat)
						{
							Prv_Chn_ChnPBOverFlow_Ind ChnPbOver;
							SN_MEMSET(&ChnPbOver, 0, sizeof(Prv_Chn_ChnPBOverFlow_Ind));
							ChnPbOver.Chn = chn;
							ChnPbOver.NewWidth = new_width;
							ChnPbOver.NewHeight = new_height;							
							SN_SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_FWK, 0, 0, MSG_ID_PRV_CHNPBOVERFLOW_IND, &ChnPbOver, sizeof(Prv_Chn_ChnPBOverFlow_Ind));					
						}	
						else
						{
							PRV_PtsInfoInit(chn);
							PRV_InitVochnInfo(index);	
						}
						
						PRV_VoChnStateInit(chn);
						VochnInfo[chn].bIsWaitGetIFrame = 1;
#if 1
						FWK_SetOverflowState(chn, EXP_LINK_OVERFLOW);
						ALM_STATUS_CHG_ST status;
						status.AlmCnt = 1;
						SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_MMI, 0, 0, MSG_ID_ALM_STATUS_CHG, &status, sizeof(status));


						deluser_used use;
						use.channel = chn;
						SN_SendMessageEx(SUPER_USER_ID, MOD_NTRANS, MOD_NTRANS, 0, 0, MSG_ID_NTRANS_DELUSER_REQ,(void*)&use,sizeof(deluser_used));
#endif
						return HI_FAILURE;
					}
					else if((CurMasterCap + VochnInfo[chn].VdecCap) >= TOTALCAPPCHIP
						&& (CurSlaveCap + VochnInfo[chn].VdecCap) >= TOTALCAPPCHIP)
					{
						HI_U32 tempVdecCap = 0, i = 0;
						int ret = 0;
						deluser_used DelUserReq;
						DelUserReq.channel = chn;
						int TmpIndex[8] = {-1, -1, -1, -1, -1, -1, -1, -1,};
						//��ͨ������Ƭ��Ҫ���������
						tempVdecCap = VochnInfo[chn].VdecCap - (TOTALCAPPCHIP - CurMasterCap);

						//tempVdecCap = PRV_ComPare(tempVdecCap);
						//����Ƭ���ҵ�����������ܵ�ͨ��(1����2����3����4�������)�ķŴ�Ƭ
						if(tempVdecCap >= TOTALCAPPCHIP)
						{
							TRACE(SCI_TRACE_NORMAL, MOD_PRV, "chn: %d, No Vaild Cap: %d\n", chn, VochnInfo[chn].VdecCap);
							SN_SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_NTRANS, 0, 0, MSG_ID_NTRANS_DELUSER_REQ, &DelUserReq, sizeof(deluser_used));
							return HI_FAILURE;
						}

						TRACE(SCI_TRACE_NORMAL, MOD_PRV, "%s line:%d CurMasterCap=%d, CurSlaveCap=%d, tempVdecCap=%d, total=%d, Need Find Master Chn!\n", __FUNCTION__, __LINE__, CurMasterCap, CurSlaveCap, tempVdecCap, TOTALCAPPCHIP);
						ret = PRV_FindMaster_Min(tempVdecCap, chn, TmpIndex);
						if(ret < 0)
						{
							TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Master: =======tempVdecCap: %d, No Find Master Chn ReChoose Slave---\n", tempVdecCap);
							SN_SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_NTRANS, 0, 0, MSG_ID_NTRANS_DELUSER_REQ, &DelUserReq, sizeof(deluser_used));
							return HI_FAILURE;
						}
			
						for(i = 0; i < 8; i++)
						{
							//������Ƭ�ҵ���ͨ���ķŴ�Ƭ
							if(TmpIndex[i] >= 0)
								PRV_MasterChnReChooseSlave(TmpIndex[i]);
							else
								break;
								
						}
				
						VochnInfo[chn].CurChnIndex = chn - LOCALVEDIONUM;
						VochnInfo[chn].SlaveId = PRV_MASTER;
						VochnInfo[chn].VoChn = chn; 
						VochnInfo[chn].IsConnect = 1;
						SlaveCreateVdecReq.SlaveId = PRV_MASTER;
						
#if 0						
						PRV_FindMasterChnReChooseSlave(tempVdecCap, chn, TmpIndex);
						TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Master: tempVdecCap: %d <= 4,Need Find Master Chn!\n", tempVdecCap);			
						for(i = 0; i < 8; i++)
						{
							//������Ƭ�ҵ���ͨ���ķŴ�Ƭ
							if(TmpIndex[i] >= 0)
								PRV_MasterChnReChooseSlave(TmpIndex[i]);
							else
								break;
								
						}
						if(i == 0)
						{
							TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Master: =======tempVdecCap: %d, No Find Master Chn ReChoose Slave---\n", tempVdecCap);
							SN_SendMessageEx(SUPER_USER_ID, MOD_PRV, MOD_NTRANS, 0, 0, MSG_ID_NTRANS_DELUSER_REQ, &DelUserReq, sizeof(deluser_used));
							return HI_FAILURE;
						}					
						VochnInfo[chn].CurChnIndex = chn - LOCALVEDIONUM;
						VochnInfo[chn].SlaveId = PRV_MASTER;
						VochnInfo[chn].VoChn = chn; 
						VochnInfo[chn].IsConnect = 1;
						SlaveCreateVdecReq.SlaveId = PRV_MASTER;
#endif					
					}
				}
			}
		}
		
		if(VochnInfo[index].bIsPBStat)
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "====================line: %d, StartPts: %d\n", __LINE__, PtsInfo[chn].StartPts);
			if(PtsInfo[chn].StartPts == 0)
			{
				PRM_ID_TIME AllStartTime, AllEndTime;
				time_t AllStartPts, AllEndPts, QueryFinalPts;
				Ftpc_PlayFileAllTime(chn, &AllStartTime, &AllEndTime);
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "==========AllStartTime: %d-%d-%d,%d.%d.%d\n", AllStartTime.Year, AllStartTime.Month, AllStartTime.Day, AllStartTime.Hour, AllStartTime.Minute, AllStartTime.Second);
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "==========AllEndTime: %d-%d-%d,%d.%d.%d\n", AllEndTime.Year, AllEndTime.Month, AllEndTime.Day, AllEndTime.Hour, AllEndTime.Minute, AllEndTime.Second);
				AllStartPts = PlayBack_PrmTime_To_Sec(&AllStartTime);
				AllEndPts = PlayBack_PrmTime_To_Sec(&AllEndTime);
				QueryFinalPts = PlayBack_PrmTime_To_Sec(&PtsInfo[chn].QueryFinalTime);
				Probar_time[chn] = PlayBack_PrmTime_To_Sec(&PtsInfo[chn].QueryStartTime);

				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "====line: %d, AllStartPts=%d, AllEndPts=%d, QueryFinalPts=%d, Probar_time[chn]=%d\n", __LINE__, AllStartPts, AllEndPts, QueryFinalPts, Probar_time[chn]);

				if(Probar_time[chn] < AllStartPts)
				{
					Probar_time[chn] = AllStartPts;
					PtsInfo[chn].QueryStartTime = AllStartTime;
				}
				if(QueryFinalPts > AllEndPts)
				{
					PtsInfo[chn].QueryFinalTime = AllEndTime;
				}
				
			}
			Ftpc_PlayFileCurTime(chn, &PtsInfo[chn].StartPrm, &PtsInfo[chn].EndPrm);
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "==========PtsInfo[chn].StarPrm: %d-%d-%d,%d.%d.%d\n", PtsInfo[chn].StartPrm.Year, PtsInfo[chn].StartPrm.Month, PtsInfo[chn].StartPrm.Day, PtsInfo[chn].StartPrm.Hour, PtsInfo[chn].StartPrm.Minute, PtsInfo[chn].StartPrm.Second);
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "==========PtsInfo[chn].EndPrm: %d-%d-%d,%d.%d.%d\n", PtsInfo[chn].EndPrm.Year, PtsInfo[chn].EndPrm.Month, PtsInfo[chn].EndPrm.Day, PtsInfo[chn].EndPrm.Hour, PtsInfo[chn].EndPrm.Minute, PtsInfo[chn].EndPrm.Second);
			PtsInfo[chn].StartPts = PlayBack_PrmTime_To_Sec(&PtsInfo[chn].StartPrm);
			if(Probar_time[chn] < PtsInfo[chn].StartPts)
			{
				Probar_time[chn] = PtsInfo[chn].StartPts;					
			}
			
			PtsInfo[chn].CurShowPts = (HI_U64)Probar_time[chn]*1000000;
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "==========QueryStarTime: %d-%d-%d,%d.%d.%d\n", PtsInfo[chn].QueryStartTime.Year, PtsInfo[chn].QueryStartTime.Month, PtsInfo[chn].QueryStartTime.Day, PtsInfo[chn].QueryStartTime.Hour, PtsInfo[chn].QueryStartTime.Minute, PtsInfo[chn].QueryStartTime.Second);
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "==========QueryFinalTime: %d-%d-%d,%d.%d.%d\n", PtsInfo[chn].QueryFinalTime.Year, PtsInfo[chn].QueryFinalTime.Month, PtsInfo[chn].QueryFinalTime.Day, PtsInfo[chn].QueryFinalTime.Hour, PtsInfo[chn].QueryFinalTime.Minute, PtsInfo[chn].QueryFinalTime.Second);
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "================PtsInfo[chn].CurShowPts: %lld\n", PtsInfo[chn].CurShowPts);
			
			Ftpc_ChnStopWrite(chn, 0);					
			VoChnState.bIsPBStat_StopWriteData[chn] = 0;
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line: %d============bIsPBStat_StopWriteData---StopWrite==0\n", __LINE__);
		}
		VochnInfo[index].IsChooseSlaveId = 1;
		if(VochnInfo[index].SlaveId > PRV_MASTER)
		{
			if(PlayInfo.PlayBackState > PLAY_INSTANT)
			{
				PlayBack_MccPBCreateVdecReq PBCreateReq;
				PBCreateReq.SlaveId = VochnInfo[index].SlaveId;
				PBCreateReq.EncType = VochnInfo[index].VideoInfo.vdoType;
				PBCreateReq.s32VdecHeight = VochnInfo[index].VideoInfo.height;
				PBCreateReq.s32VdecWidth  = VochnInfo[index].VideoInfo.width;
				PBCreateReq.VoChn = VochnInfo[index].VoChn;
				PBCreateReq.VdecChn = VochnInfo[index].VoChn;
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "============Send: MSG_ID_PRV_MCC_PBCREATEVDEC_REQ---%d\n", PBCreateReq.VdecChn);
				SN_SendMccMessageEx(PRV_SLAVE_1, SUPER_USER_ID, MOD_PRV, MOD_PRV, 0, 0, MSG_ID_PRV_MCC_PBCREATEVDEC_REQ, &PBCreateReq, sizeof(PBCreateReq));					
			}
			else
			{
				SN_SendMccMessageEx(PRV_SLAVE_1, SUPER_USER_ID, MOD_PRV, MOD_PRV, 0, 0, MSG_ID_PRV_MCC_CREATEVDEC_REQ, &SlaveCreateVdecReq, sizeof(PRV_MccCreateVdecReq));					
			}
			VochnInfo[index].MccCreateingVdec = 1;
			return 1;
		}
		else
		{
			//PRV_CreateVdecChn(VochnInfo[index].VideoInfo.vdoType, VochnInfo[index].VideoInfo.height, VochnInfo[index].VideoInfo.width, VochnInfo[index].VdecChn);
			if(PlayInfo.PlayBackState > PLAY_INSTANT)
			{
				s32Ret = PlayBack_CreatVdec(HD, VochnInfo[index].VoChn);
			}
			else
			{
				s32Ret = PRV_CreateVdecChn_EX(index);
			}
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "===============line: %d, s32Ret: %d\n", __LINE__, s32Ret);
			if(s32Ret == HI_FAILURE)
			{
				return HI_FAILURE;
			}
		}
			
	}
	//TRACE(SCI_TRACE_NORMAL, MOD_DEC, "End PRV_DetectResIsChg\n");
	
	return HI_SUCCESS;
}

//�ڱ��ػ�������л�ȡ��һ����ƵI֡
//����ֵ:HI_FAILURE:��ȡI֡ʧ��
//			HI_SUCCESS:��ȡI֡�ɹ���������ƬԤ����ͨ��
//			1:��ȡI֡�ɹ������ڴ�ƬԤ����ͨ��
HI_S32 PRV_GetFirstIFrame(int chn, char **pv)
{
	//char *pv = NULL;
	AVPacket *PRVGetVideoData = NULL;
	HI_S32 s32Ret = 0, index = 0, TotalSize = 0, PRVStatus = 0; 

	index = chn + LOCALVEDIONUM;
	while(1)
	{
		s32Ret = BufferGet(chn + PRV_VIDEOBUFFER, pv, &TotalSize, &PRVStatus);
		if(s32Ret == 0 && pv != NULL)
		{
			PRVGetVideoData = (AVPacket *)*pv;
			if(PRVGetVideoData->frame_type != 1)
			{
				NTRANS_FreeMediaData(PRVGetVideoData); 
				PRVGetVideoData = NULL;
				continue;
			}
			//�����I֡���жϴ�I֡�ֱ����Ƿ���������ֱ���һ��
			else
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "========Found I Frame\n");
				s32Ret = PRV_DetectResIsChg(chn, PRVGetVideoData);
				if(s32Ret == HI_FAILURE)
				{
					NTRANS_FreeMediaData(PRVGetVideoData); 
					PRVGetVideoData = NULL; 	
				}
				else if(s32Ret == 1)//��Ƭ����	
				{
					tmp_PRVGetVideoData[chn] = PRVGetVideoData; 
				}
				else if(s32Ret == 2)
				{
					Pb_FirstGetVideoData[chn] = PRVGetVideoData;
				}
				return s32Ret;
			}
		}	
		else
		{
			return HI_FAILURE;
		}
	}
	
	return HI_FAILURE;
}

int PRV_GetBufferSize(int chn)
{	
	int speed = 0, buf_size = 10;

	if(chn < 0 || chn >= DEV_CHANNEL_NUM)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "chn=%d, error\n", chn);
		return buf_size;
	}

	speed = PlayStateInfo[chn].CurSpeedState;

	switch(speed)
	{
		case DEC_SPEED_NORMALSTEP:
		case DEC_SPEED_NORMALSLOW8:
		case DEC_SPEED_NORMALSLOW4:
		case DEC_SPEED_NORMALSLOW2:
		{
			buf_size = 1;
		}
		break;
		case DEC_SPEED_NORMAL:
		{
			buf_size = 10;
		}
		break;
		case DEC_SPEED_NORMALFAST2:
		{
			buf_size = 5;
		}
		break;
		case DEC_SPEED_NORMALFAST4:
		case DEC_SPEED_NORMALFAST8:
		{
			buf_size = 2;
		}
		break;
		default:
		{
			buf_size = 10;
		}
		break;	
		
	}

	return buf_size;
}

//����Ƶͬ������
HI_S32 PRV_Video_Audio_SynPro(int chn)
{
	int count = 0, PRVStatus = 0,TotalSize = 0, cidx = 0, AudioFast = 0, VideoFast = 0, BufferJumpCount = 0;
	HI_S32 s32Ret = 0; 
	char *pv = NULL;
	AVPacket *PRVGetAudioData = NULL;
#if defined(Hi3531)
	AUDIO_DEV AoDev = 4;
#else
	AUDIO_DEV AoDev = 0;
#endif
	if(PRV_CurDecodeMode == PassiveDecode)
	{
		if(Achn < 0)
		{
			Achn = 0;
		}
	}
	
	//���������£��໭��Ԥ��ʱ����Ҫ����Ƶͬ������Ϊ�޷�ȷ����Ƶ���ĸ���Ƶͨ��ƥ��
	if(PRV_CurDecodeMode == PassiveDecode && g_stPreviewMode != SingleScene)
	{
		s32Ret = BufferGet(chn + PRV_AUDIOBUFFER, &pv, &TotalSize, &PRVStatus);
		if(s32Ret == 0 && pv != NULL)
		{		
			PRVGetAudioData = (AVPacket *)pv;
			PRV_IsAudioParaChange(Achn, PRVGetAudioData->DataSize - PRVGetAudioData->BufOffset);//��Ƶ�����иı�ʱ����Ҫ������Ƶ���ͨ��
			//printf("----------Send Video Data----------\n");
			s32Ret = PRV_SendData(chn, PRVGetAudioData, PRVGetAudioData->codec_id, MasterToSlaveChnId, 0);
			VoChnState.SendAudioDataCount[chn]++;
			NTRANS_FreeMediaData(PRVGetAudioData);
			PRVGetAudioData = NULL;
		}
		IsStopGetAudio = 0;

		return HI_SUCCESS;
	}

	if(VoChnState.AudioDataTimeLag[chn] - VoChnState.VideoDataTimeLag[chn] > 0)
	{
		//TRACE(SCI_TRACE_NORMAL, MOD_SLC, "--------chn: %d, IsStopGetAudio: %d, Lag::::Audio: %lld, Video: %lld, abs(VoChnState.AudioDataTimeLag - VoChnState.VideoDataTimeLag): %d\n", chn, IsStopGetAudio, VoChnState.AudioDataTimeLag[chn], VoChnState.VideoDataTimeLag[chn], abs(VoChnState.AudioDataTimeLag[chn] - VoChnState.VideoDataTimeLag[chn]));
		return HI_SUCCESS;
	}
	
	if(!IsStopGetAudio || PlayInfo.PlayBackState >= PLAY_INSTANT
		/*|| (PlayInfo.PlayBackState == PLAY_INSTANT && Achn == PlayInfo.InstantPbChn)*/)
	{
		s32Ret = BufferGet(chn + PRV_AUDIOBUFFER, &pv, &TotalSize, &PRVStatus);
		if(s32Ret == 0 && pv != NULL)
		{		
			PRVGetAudioData = (AVPacket *)pv;
			VoChnState.AudioDataTimeLag[chn] = (HI_S64)(PRVGetAudioData->pts - PtsInfo[chn].FirstAudioPts) * 1000 / 8000;//��ǰ��Ƶʱ������׼ʱ����Ĳ�ֵ(�����ms)
			if(PRVGetAudioData->codec_id != VochnInfo[chn].AudioInfo.adoType && (PlayInfo.PlayBackState == PLAY_INSTANT))
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line:%d-ch=%d-Achn=%d--codec_id=%d--adoType=%d--\n", __LINE__, chn, Achn, PRVGetAudioData->codec_id, VochnInfo[chn].AudioInfo.adoType);
				VochnInfo[chn].AudioInfo.adoType = PRVGetAudioData->codec_id;							
				if(Achn == chn)
				{
					PRV_StopAdec();
					PRV_StartAdecAo(VochnInfo[chn]);
					IsCreateAdec = 1;
				}
			}

			if(PRVGetAudioData->codec_id != VochnInfo[chn].AudioInfo.adoType && (PlayInfo.PlayBackState > PLAY_INSTANT))
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line:%d-ch=%d-Achn=%d--codec_id=%d--adoType=%d--\n", __LINE__, chn, Achn, PRVGetAudioData->codec_id, VochnInfo[chn].AudioInfo.adoType);
				VochnInfo[chn].AudioInfo.adoType = PRVGetAudioData->codec_id;
				Achn = ADECHN;
				PlayBack_StopAdec(AoDev, AOCHN, ADECHN);
				PlayBack_StartAdec(AoDev, AOCHN, ADECHN, VochnInfo[chn].AudioInfo.adoType);

			}
		}
		else
		{
			return HI_FAILURE;
		}
		if(PlayInfo.PlayBackState > PLAY_INSTANT
			/*|| (PlayInfo.PlayBackState == PLAY_INSTANT && Achn == PlayInfo.InstantPbChn)*/)
		{			
			goto SendAudioData;
		}
	}
#if 1
	//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "--------chn: %d, IsStopGetAudio: %d, Lag::::Audio: %lld, Video: %lld, abs(VoChnState.AudioDataTimeLag - VoChnState.VideoDataTimeLag): %d\n", chn, IsStopGetAudio, VoChnState.AudioDataTimeLag[chn], VoChnState.VideoDataTimeLag[chn], abs(VoChnState.AudioDataTimeLag[chn] - VoChnState.VideoDataTimeLag[chn]));
	if(abs(VoChnState.AudioDataTimeLag[chn] - VoChnState.VideoDataTimeLag[chn]) > TimeInterval)
	{
		if(VoChnState.AudioDataTimeLag[chn] > VoChnState.VideoDataTimeLag[chn])
		{	
			AudioFast = 1;
			VideoFast = 0;
			IsStopGetAudio = 1;
		}
		else
		{
			AudioFast = 0;
			VideoFast = 1;
			IsStopGetAudio = 0;
		}
	}
	else
	{
		AudioFast = 0;
		VideoFast = 0;
		IsStopGetAudio = 0;

	}
#endif
	//��Ƶ�Ͽ�ʱ������Ҫ��ȡ��Ƶ����
	if(AudioFast)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "AudioFast--------chn: %d, Lag::::Audio: %lld, Video: %lld, abs(VoChnState.AudioDataTimeLag - VoChnState.VideoDataTimeLag): %d\n", chn, VoChnState.AudioDataTimeLag[chn], VoChnState.VideoDataTimeLag[chn], abs(VoChnState.AudioDataTimeLag[chn] - VoChnState.VideoDataTimeLag[chn]));
		
		//BufferJumpCount = MAX_ARRAY_NODE/10;
	}
	else if(VideoFast)//��Ƶ�Ͽ�ʱ��ֻҪ��Ƶbuffer�в����������ݣ����ѯ��Ƶ����
	{		
		s32Ret = BufferState(chn + PRV_AUDIOBUFFER, &count, &TotalSize, &cidx);
		if(cidx > 0)
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line: %d, chn: %d, cidx: %d\n", __LINE__, chn, cidx);
		//�����Ƶbuffer��δȡ���ݴ���20֡������20֡Ϊ������֡ȡ��Ƶ����
		//���ж��Ƿ���Ȼ��ͬ�������һֱ��ͬ������ȡ���������ж�
		while(s32Ret == 0 && cidx > 0)
		{
			if(cidx >= MAX_ARRAY_NODE/20)
				BufferJumpCount = MAX_ARRAY_NODE/20 - 1;
			else
				BufferJumpCount = cidx - 1;
				
			BufferJump(chn + PRV_AUDIOBUFFER, &BufferJumpCount);
			if(NULL != PRVGetAudioData)
			{
				NTRANS_FreeMediaData(PRVGetAudioData);	
				PRVGetAudioData = NULL;
			}
			s32Ret = BufferGet(chn + PRV_AUDIOBUFFER, &pv, &TotalSize, &PRVStatus);
			if(s32Ret == 0 && pv != NULL)
			{	
				PRVGetAudioData = (AVPacket *)pv;
				VoChnState.AudioDataTimeLag[chn] = (HI_S64)(PRVGetAudioData->pts - PtsInfo[chn].FirstAudioPts) * 1000/8000;//��ǰ��Ƶʱ������׼ʱ����Ĳ�ֵ(�����ms)
				//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "------------VoChnState.AudioDataTimeLag: %lld\n", VoChnState.AudioDataTimeLag[chn]);
			}
			s32Ret = BufferState(chn + PRV_AUDIOBUFFER, &count, &TotalSize, &cidx);
			//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line: %d, chn: %d, cidx: %d\n", __LINE__, chn, cidx);
			
			if(VoChnState.VideoDataTimeLag[chn] - VoChnState.AudioDataTimeLag[chn] <= TimeInterval)
			{
				VideoFast = 0;
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Normal::::::::::::---chn: %d, video: %lld, audio: %lld@@@%lld, cidx: %d\n",
					chn, VoChnState.VideoDataTimeLag[chn], VoChnState.AudioDataTimeLag[chn], VoChnState.VideoDataTimeLag[chn] - VoChnState.AudioDataTimeLag[chn], cidx);				
				break;
			}
								
		}
	}
	
#if 0
	//����Ƶͬ��
	if(abs(VoChnState.VideoDataTimeLag[chn] - VoChnState.AudioDataTimeLag[chn]) > TimeInterval)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "syn::::::::Discard Audio Data------i: %d, video: %lld, audio: %lld,----%lld\n", chn, VoChnState.VideoDataTimeLag[chn], VoChnState.AudioDataTimeLag[chn], VoChnState.VideoDataTimeLag[chn] - VoChnState.AudioDataTimeLag[chn]);
		//��ͬ��������Ƶ֡
		if(NULL != PRVGetAudioData)
		{
			NTRANS_FreeMediaData(PRVGetAudioData);
			PRVGetAudioData = NULL;
			return HI_SUCCESS;
		}
	}					
	else
	{				
		IsStopGetAudio = 0;
	}
#endif
	
SendAudioData:
	if(NULL != PRVGetAudioData)
	{
		if(PtNumPerFrm > 0)//�Ѿ���������Ƶ����ͨ��
		{
			PRV_IsAudioParaChange(Achn, PRVGetAudioData->DataSize - PRVGetAudioData->BufOffset);//��Ƶ�����иı�ʱ����Ҫ������Ƶ���ͨ��
		}
		#if 0
		if(IsCreatingAdec)
		{
			NTRANS_FreeMediaData(PRVGetAudioData);
			PRVGetAudioData = NULL;
			return HI_SUCCESS;
		}
		#endif
		if(PlayInfo.PlayBackState > PLAY_INSTANT)
		{
			if(!PlayInfo.IsPlaySound || PlayStateInfo[chn].CurSpeedState != DEC_SPEED_NORMAL)
			{
				NTRANS_FreeMediaData(PRVGetAudioData);
				PRVGetAudioData = NULL;
				return HI_SUCCESS;
			}
		}

		if(!VoChnState.IsGetFirstData[chn])
		{
			NTRANS_FreeMediaData(PRVGetAudioData);
			PRVGetAudioData = NULL;
			return HI_SUCCESS;
		}

		
#if defined(Hi3531)||defined(Hi3535)		
		if(IsAdecBindAo)
#endif		
		{
			s32Ret = PRV_SendData(chn, PRVGetAudioData, PRVGetAudioData->codec_id, MasterToSlaveChnId, 0);
			if(s32Ret == HI_ERR_ADEC_BUF_FULL)
			{
				IsStopGetAudio = 1;
			}
			
			VoChnState.SendAudioDataCount[chn]++;
		}
		
		NTRANS_FreeMediaData(PRVGetAudioData);
		PRVGetAudioData = NULL;
	}

	if(VideoFast)
	{
		TRACE(SCI_TRACE_NORMAL, MOD_PRV, "video is fast");
		return 1;
	}
	
	return HI_SUCCESS;
}

#define PATH_test "/tmp/test"
#define BUFSIZE 8*1024*1024
#define VIDEO_PACKET_SIZE 	1700		//��Ƶ����С
int testsign = 1;

//��ȡ�ļ������ǲ����
int find_nal_unit_test(uint8_t* buf, int size, int* nal_start, int* nal_end)
{
    int i = 0;
    // find start
    *nal_start = 0;
    *nal_end = 0;
    
    i = 0;
    while (   //( next_bits( 24 ) != 0x000001 && next_bits( 32 ) != 0x00000001 )
        (buf[i] != 0 || buf[i+1] != 0 || buf[i+2] != 0x01) && 
        (buf[i] != 0 || buf[i+1] != 0 || buf[i+2] != 0 || buf[i+3] != 0x01) 
        )
    {
        i++; // skip leading zero
        if (i+4 >= size) { return 0; } // did not find nal start
    }

    if  (buf[i] != 0 || buf[i+1] != 0 || buf[i+2] != 0x01) // ( next_bits( 24 ) != 0x000001 )
    {
        i++;
    }

    if  (buf[i] != 0 || buf[i+1] != 0 || buf[i+2] != 0x01) { /* error, should never happen */ return 0; }

	if(buf[i+3] == 0x41)
	{
		printf("11start sign:%x\n", buf[i+3]);
		i+= 3;
	    *nal_start = i;
	    
	    while (   //( next_bits( 24 ) != 0x000000 && next_bits( 24 ) != 0x000001 )
	        (buf[i] != 0 || buf[i+1] != 0 || buf[i+2] != 0) && 
	        (buf[i] != 0 || buf[i+1] != 0 || buf[i+2] != 0x01)
	        )
	    {
	        i++;
	        // FIXME the next line fails when reading a nal that ends exactly at the end of the data
	        if (i+3 >= size) { *nal_end = size; return -1; } // did not find nal end, stream ended first	
	    }

		i+= 3;

		while (   //( next_bits( 24 ) != 0x000000 && next_bits( 24 ) != 0x000001 )
	        (buf[i] != 0 || buf[i+1] != 0 || buf[i+2] != 0) && 
	        (buf[i] != 0 || buf[i+1] != 0 || buf[i+2] != 0x01)
	        )
	    {
	        i++;
	        // FIXME the next line fails when reading a nal that ends exactly at the end of the data
	        if (i+3 >= size) { *nal_end = size; return -1; } // did not find nal end, stream ended first	
	    }

		 printf("end sign:%x\n", buf[i+4]);
	}
	else if((buf[i+3]&0x1f) == 0x7)
	{
		printf("22start sign:%x\n", buf[i+3]);
		i+= 3;
	    *nal_start = i;
	    
	    while (   //( next_bits( 24 ) != 0x000000 && next_bits( 24 ) != 0x000001 )
        	(buf[i] != 0 || buf[i+1] != 0 || buf[i+2] != 0 || buf[i+3] != 0x01 || buf[i+4] != 0x41) 
	        )
	    {
	        i++;
	        // FIXME the next line fails when reading a nal that ends exactly at the end of the data
	        if (i+4 >= size) { *nal_end = size; return -1; } // did not find nal end, stream ended first	
	    }
		 printf("end sign:%x\n", buf[i+4]);
	}
	else
	{
		printf("not find:%x\n", buf[i+3]);
		return -1;
	}
    
    *nal_end = i;
    return (*nal_end - *nal_start);
}


//��ȡ�ļ���ֱ�Ӵ��
int find_nal_unit_test1(uint8_t* buf, int size, int* nal_start, int* nal_end)
{
    int i = 0;
    // find start
    *nal_start = 0;
    *nal_end = 0;
    
    i = 0;
    while (   //( next_bits( 24 ) != 0x000001 && next_bits( 32 ) != 0x00000001 )
        (buf[i] != 0 || buf[i+1] != 0 || buf[i+2] != 0x01) && 
        (buf[i] != 0 || buf[i+1] != 0 || buf[i+2] != 0 || buf[i+3] != 0x01) 
        )
    {
        i++; // skip leading zero
        if (i+4 >= size) { return 0; } // did not find nal start
    }

    if  (buf[i] != 0 || buf[i+1] != 0 || buf[i+2] != 0x01) // ( next_bits( 24 ) != 0x000001 )
    {
        i++;
    }

    if  (buf[i] != 0 || buf[i+1] != 0 || buf[i+2] != 0x01) { /* error, should never happen */ return 0; }

	if(buf[i+3] == 0x41)
	{
		printf("11start sign:%x-----------------------------------------------\n", buf[i+3]);
		i+= 3;
	    *nal_start = i;
	    
	    while (   //( next_bits( 24 ) != 0x000000 && next_bits( 24 ) != 0x000001 )
	        (buf[i] != 0 || buf[i+1] != 0 || buf[i+2] != 0) && 
	        (buf[i] != 0 || buf[i+1] != 0 || buf[i+2] != 0x01)
	        )
	    {
	        i++;
	        // FIXME the next line fails when reading a nal that ends exactly at the end of the data
	        if (i+3 >= size) { *nal_end = size; return -1; } // did not find nal end, stream ended first	
	    }

		 printf("end sign:%x\n", buf[i+4]);
	}
	else if((buf[i+3]&0x1f) == 0x7)
	{
		printf("22start sign:%x------------------------------------------------\n", buf[i+3]);
		i+= 3;
	    *nal_start = i;
	    
	    while (   //( next_bits( 24 ) != 0x000000 && next_bits( 24 ) != 0x000001 )
        	(buf[i] != 0 || buf[i+1] != 0 || buf[i+2] != 0 || buf[i+3] != 0x01 || buf[i+4] != 0x65) 
	        )
	    {
	        i++;
	        // FIXME the next line fails when reading a nal that ends exactly at the end of the data
	        if (i+4 >= size) { *nal_end = size; return -1; } // did not find nal end, stream ended first	
	    }
		i+= 3;
		while (   //( next_bits( 24 ) != 0x000000 && next_bits( 24 ) != 0x000001 )
        	(buf[i] != 0 || buf[i+1] != 0 || buf[i+2] != 0 || buf[i+3] != 0x01 || buf[i+4] != 0x65) 
	        )
	    {
	        i++;
	        // FIXME the next line fails when reading a nal that ends exactly at the end of the data
	        if (i+4 >= size) { *nal_end = size; return -1; } // did not find nal end, stream ended first	
	    }
		 printf("end sign:%x\n", buf[i+4]);
	}
	else if(buf[i+3] == 0x65)
	{
		printf("33start sign:%x-----------------------------------------------\n", buf[i+3]);
		i+= 3;
	    *nal_start = i;
	    
	    while (   //( next_bits( 24 ) != 0x000000 && next_bits( 24 ) != 0x000001 )
        	(buf[i] != 0 || buf[i+1] != 0 || buf[i+2] != 0 || buf[i+3] != 0x01 || buf[i+4] != 0x41) 
	        )
	    {
	        i++;
	        // FIXME the next line fails when reading a nal that ends exactly at the end of the data
	        if (i+4 >= size) { *nal_end = size; return -1; } // did not find nal end, stream ended first	
	    }
		 printf("end sign:%x\n", buf[i+4]);
	}
	else
	{
		printf("not find:%x\n", buf[i+3]);
		return -1;
	}
    
    *nal_end = i;
    return (*nal_end - *nal_start);
}

void *PRV_FileThread()
{
	int i = 0;
	char name[32];

	Log_pid(__FUNCTION__);
	pthread_detach(pthread_self());
	
	while(1)
	{	
		for(i=0; i<DEV_CHANNEL_NUM; i++)
		{	
			if(g_beginsign && access(PATH_STOP, F_OK) != 0)
			{
				SN_MEMSET(name, 0, sizeof(name));
				SN_SPRINTF(name, sizeof(name), "/tmp/get_ch%d.h264", i);
				if(access(name, F_OK) == 0)
				{
					g_filesign[i][0] = i;
				}
				else
				{
					g_filesign[i][0] = -1;
				}

				SN_MEMSET(name, 0, sizeof(name));
				SN_SPRINTF(name, sizeof(name), "/tmp/send_ch%d.h264", i);
				if(access(name, F_OK) == 0)
				{
					g_filesign[i][1] = i;
				}
				else
				{
					g_filesign[i][1] = -1;
				}

				SN_MEMSET(name, 0, sizeof(name));
				SN_SPRINTF(name, sizeof(name), "/tmp/recv_ch%d.h264", i);
				if(access(name, F_OK) == 0)
				{
					g_filesign[i][2] = i;
				}
				else
				{
					g_filesign[i][2] = -1;
				}
				
			}
			else
			{
				g_filesign[i][0] = -1;
				g_filesign[i][1] = -1;
				g_filesign[i][2] = -1;
			}		

			usleep(100 * 1000);			
		}

		if(testsign && access(PATH_test, F_OK) == 0)
		{
			testsign = 0;
			printf("enter test\n");
			uint8_t* buf = (uint8_t*)malloc(BUFSIZE);
			int fd = open("/tmp/test.h264", O_RDONLY);
		    if (fd == -1) { perror("could not open file"); exit(0); }

		    int rsz = 0;
		    int sz = 0;
		    int64_t off = 0;
			uint8_t* data= NULL;
		    uint8_t* p = buf;
			AVPacket *av_packet = NULL;
			AVPacket *tem_packet = NULL;
			AVPacket *bPacket = NULL;
			int ret = 0, cursize = 0, curOffset = 0, buflen = 0, curcount = 0;
			int bytesRead = 0, pack_len = 0;
		    int nal_start, nal_end;

		    while ((rsz = read(fd, buf + sz, BUFSIZE - sz)))
		    {
		        sz += rsz;
				printf("----1-----\n");
		        while (find_nal_unit_test1(p, sz, &nal_start, &nal_end) > 0)
		        {
					buflen = 0;
					cursize = curOffset = 0;
		            p += nal_start;
					data = p;
					bytesRead = nal_end - nal_start;
					printf("----2-bytesRead=%d----\n", bytesRead);
#if 1		
					if(bytesRead > 4)
					{
						av_packet = (AVPacket * )SN_MPMalloc(gVideoPool, VIDEO_PACKET_SIZE);
						if(av_packet != NULL)
						{
							av_packet->codec_id = CODEC_ID_H264;//��ʼ������
							av_packet->pts = 0;
							av_packet->channel = 0;
							av_packet->seqno = curcount++;
							av_packet->pub_count = 1;
							av_packet->data_len = bytesRead + sizeof(int);
							av_packet->rtpOffset = 0;
							av_packet->DataSize = sizeof(AVPacket);
							av_packet->BufOffset = sizeof(AVPacket);
							av_packet->BufRead = 0;
							av_packet->isFirstPacket = FALSE;		  
							av_packet->next = NULL;
							av_packet->Extendnext= NULL;
							av_packet->shareprv =0;
							av_packet->sharesev=0;			
							av_packet->sharevam=0;
							
						}
						else
						{   
							TRACE(SCI_TRACE_HIGH, MOD_FTPC, "%s_line:%d av_packet error", __FUNCTION__, __LINE__);
							break;
						}

						pack_len = VIDEO_PACKET_SIZE - sizeof(AVPacket);

						if(bytesRead > pack_len)
						{
							SN_MEMCPY((char *)av_packet + av_packet->BufOffset, pack_len, (char *)data+cursize+curOffset, pack_len, pack_len);
							curOffset += pack_len;		 
							av_packet->DataSize +=  pack_len;
							bytesRead -= pack_len;
						}
						else
						{   
							SN_MEMCPY((char *)av_packet + av_packet->BufOffset, pack_len, (char *)data+cursize+curOffset, bytesRead, bytesRead);
							curOffset += bytesRead;
							av_packet->DataSize +=  bytesRead;
							bytesRead = 0;
						}

						bPacket = av_packet;
						buflen += VIDEO_PACKET_SIZE;
						while( bytesRead > 0)
						{
							tem_packet = (AVPacket * )SN_MPMalloc(gVideoPool, VIDEO_PACKET_SIZE);
							if(tem_packet != NULL)
							{	
								tem_packet->codec_id = CODEC_ID_H264;//��ʼ������
								tem_packet->pts = 0;
								tem_packet->channel = 1;
								tem_packet->seqno = curcount;
								tem_packet->pub_count = 1;
								tem_packet->data_len = bytesRead + sizeof(int);
								tem_packet->rtpOffset = 0;
								tem_packet->DataSize = sizeof(AVPacket);
								tem_packet->BufOffset = sizeof(AVPacket);
								tem_packet->BufRead = 0;
								tem_packet->isFirstPacket = FALSE;		  
								tem_packet->next = NULL;
								tem_packet->Extendnext= NULL;
								tem_packet->shareprv =0;
								tem_packet->sharesev=0;			
								tem_packet->sharevam=0;
							}
							else
							{   
								TRACE(SCI_TRACE_HIGH, MOD_FTPC, "%s_line:%d  av_packet error",  __FUNCTION__, __LINE__);

								BufferFree((char *)av_packet);
								break;
							}
							
							if(bytesRead > pack_len)
							{
								SN_MEMCPY((char *)tem_packet + tem_packet->BufOffset, pack_len, (char *)data+cursize+curOffset, pack_len, pack_len);
								curOffset += pack_len;		 
								tem_packet->DataSize += pack_len;
								bytesRead -= pack_len;
							}
							else
							{   
								SN_MEMCPY((char *)tem_packet + tem_packet->BufOffset, pack_len, (char *)data+cursize+curOffset, bytesRead, bytesRead);
								curOffset += bytesRead;
								tem_packet->DataSize +=  bytesRead;
								bytesRead = 0;
							}
							
							bPacket->next = (void *)tem_packet;
							bPacket = tem_packet;
							buflen += VIDEO_PACKET_SIZE;
							
						}

						if(((((int*)(data+cursize))[0])&0x0000001f) == 0x00000007)
						{
							printf("is I frame \n");
							av_packet->frame_type = 1;
						}
						else if(((((int*)(data+cursize))[0])&0x0000001f) == 0x00000001)
						{
							printf("is p frame \n");
							av_packet->frame_type = 0;
						}
						else
						{
							printf("is p frame \n");
							av_packet->frame_type = 0;
						}

						//ret = PRV_WriteBuffer(1, av_packet->codec_id, av_packet);
						ret = BufferWrite(0, (char*)av_packet, buflen);
						if(ret != OK)
						{
							TRACE(SCI_TRACE_HIGH, MOD_FTPC, "%s_line:%d  BufferWrite error",  __FUNCTION__, __LINE__);
							BufferFree((char *)av_packet);
						}
					}
#endif
					
		            p += (nal_end - nal_start);
		            sz -= nal_end;
					printf("----pack end-curcount=%d----\n", curcount);
					long time_sin = (1000 * 1000 / 36);
					usleep(time_sin);
		        }

		        // if no NALs found in buffer, discard it
		        if (p == buf) 
		        {
		            p = buf + sz;
		            sz = 0;
		        }

		        memmove(buf, p, sz);
		        off += p - buf;
		        p = buf;
		    }

			printf("---all end---\n");
			free(buf);
		}
		else if(access(PATH_test, F_OK) != 0)
		{
			testsign = 1;
		}
		else
		{
			testsign = 0;
		}

		sleep(1);
		
	}

	return NULL;
	
}


unsigned char VideoData[MAXFRAMELEN] = {0};
/********************************************************
������:PRV_GetPrvDataThread
��     ��:��Client�������л�ȡ���ݣ������ڱ���Ԥ���������У�
			500ms��PRV_SendDataThread�̴߳ӱ��ػ�������ȡ����
��     ��:��
����ֵ:  ��

*********************************************************/
void* PRV_GetPrvDataThread()
{
	HI_S32 s32Ret = 0, doublechn = -1, chn = 0, index = 0, i = 0, ReGetIndexPts[MAX_IPC_CHNNUM] = {0}, count = 0, TotalSize = 0, cidx = 0, GetDataCount = 0;
	HI_S32 DiscardPFrameNum[MAX_IPC_CHNNUM] = {0}, WaitIFrameNum[MAX_IPC_CHNNUM] = {0}, lastTime[MAX_IPC_CHNNUM] = {0}, lastPts[MAX_IPC_CHNNUM] = {0};
	HI_S32 size = 0, RTSPStates = 0, new_width = 0, new_height = 0;
	unsigned int PreSegNo[MAX_IPC_CHNNUM] = {0}, CurSegNo[MAX_IPC_CHNNUM] = {0};//��¼ǰһ����Ƶ��segnoֵ��������Ƶ��segno
	AVPacket *DataFromRTSP = NULL;
	char *pdata = NULL;
	RTSP_C_SDPInfo RTSP_SDP;
	SN_MEMSET(&RTSP_SDP, 0, sizeof(RTSP_C_SDPInfo));
	VDEC_CHN_STAT_S pstStat;
	struct timeval oldTime[MAX_IPC_CHNNUM], curTime[MAX_IPC_CHNNUM];	
				
	Log_pid(__FUNCTION__);
	pthread_detach(pthread_self());
	while(1)
	{
		if(IsUpGrade)
		{
			return NULL;
		}
		if(PlayInfo.PlayBackState > PLAY_INSTANT)
		{
			sem_wait(&sem_PrvGetData);		
		}
		while(1)
		{
			if(PlayInfo.PlayBackState > PLAY_INSTANT)
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_GetPrvDataThread====PlayInfo.PlayBackState: %d > PLAY_INSTANT\n", PlayInfo.PlayBackState);
				break;		
			}
			GetDataCount = 0;	
			
			pthread_mutex_lock(&send_data_mutex);
			//һ���Զ�Ӽ��ζ���0���ݣ�
			//���������������߳�forѭ��ռ��ʱ��̫�����������Ӷ���0���ݵ���һЩ���ݱ�����
			while(GetDataCount < 1)
			{
				s32Ret = NTRANS_GetMediaData(&pdata, &size, &RTSPStates, PRVUSEONLY);
				if(s32Ret != 0 || pdata == NULL)
				{
					break;
				}
				GetDataCount++;			
				DataFromRTSP = (AVPacket *)pdata;
				chn = DataFromRTSP->channel;
				doublechn = -1;

				#if 0
				rtp_stream_testC(DataFromRTSP);
				#endif
				
				TRACE(SCI_TRACE_NORMAL, MOD_FMG, "PRV_GetPrvDataThread==chn: %d,bool=%d, index=%d, codec_id=%d\n", chn, PRV_GetDoubleIndex(), PRV_GetDoubleToSingleIndex(), DataFromRTSP->codec_id);
				
				if(PRV_GetDoubleIndex() && PlayInfo.PlayBackState < PLAY_INSTANT)//˫��״̬���뵥����
				{
					if(chn == MAX_IPC_CHNNUM)
					{
						chn = PRV_GetDoubleToSingleIndex();
						DataFromRTSP->channel = chn;

						index = chn + LOCALVEDIONUM;
						
						if(VochnInfo[chn].bIsDouble == 0 && DataFromRTSP->codec_id == CODEC_ID_H264 && DataFromRTSP->frame_type == 1)
						{
							VochnInfo[chn].bIsDouble = 1;
							BufferSet(VochnInfo[index].VoChn + PRV_VIDEOBUFFER, MAX_ARRAY_NODE);						
							BufferSet(VochnInfo[index].VoChn + PRV_AUDIOBUFFER, MAX_ARRAY_NODE);
							HI_MPI_ADEC_ClearChnBuf(DecAdec);
						}
						else
						{
							if(VochnInfo[chn].bIsDouble == 0)
							{
								TRACE(SCI_TRACE_NORMAL, MOD_DEC, "PRV_GetPrvDataThread=1===chn: %d===\n", chn);
								NTRANS_FreeMediaData(DataFromRTSP); 
								DataFromRTSP = NULL;			
								continue;
							}							
						}
					}
					else
					{
						index = chn + LOCALVEDIONUM;
						if(VochnInfo[chn].bIsDouble  && !VochnInfo[index].bIsPBStat)
						{
							TRACE(SCI_TRACE_NORMAL, MOD_DEC, "PRV_GetPrvDataThread=2===chn: %d===\n", chn);
							NTRANS_FreeMediaData(DataFromRTSP); 
							DataFromRTSP = NULL;			
							continue;
						}						
					}
				}
				
				index = chn + LOCALVEDIONUM;
				if(VochnInfo[index].bIsPBStat)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_DEC, "PRV_GetPrvDataThread=3===chn: %d===\n", chn);
					NTRANS_FreeMediaData(DataFromRTSP); 
					DataFromRTSP = NULL;			
					continue;
				}

				if(DataFromRTSP->codec_id == CODEC_ID_H264 && DataFromRTSP->frame_type == 1)
				{
					///TRACE(SCI_TRACE_NORMAL, MOD_SLC, "prv get I frame, pTmp->seqno=%u\n", DataFromRTSP->seqno);
				}
				
				if(RTSPStates == 1)
				{
					for(i = LOCALVEDIONUM; i < CHANNEL_NUM; i++)
					{
						VochnInfo[i].bIsWaitIFrame = 1;	
					}
					NTRANS_FreeMediaData(DataFromRTSP); 
					DataFromRTSP = NULL;			
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "============NTRANS_GetMediaData========chn: %d\n", chn);
					s32Ret = BufferState(0, &count, &TotalSize, &cidx);
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "=============s32Ret: %d, count: %d, TotalSize: %d, cidx: %d\n", s32Ret, count, TotalSize, cidx);
					continue;
				}
							
				s32Ret = PRV_CheckDataPara(DataFromRTSP);
				if(s32Ret != HI_SUCCESS)
				{
					NTRANS_FreeMediaData(DataFromRTSP); 
					DataFromRTSP = NULL;
					VochnInfo[index].bIsWaitIFrame = 1; 
					continue;
				}

				/*���ݴ������������,��ES��д�������ļ���*/
				if(DataFromRTSP->codec_id == CODEC_ID_H264 && g_filesign[chn][0] >= 0 && g_filesign[chn][0] == chn)
				{
					AVPacket *NextPacket = NULL;
					AVPacket *pTmp = NULL;
					int length = 0, TotalLength = 0;
					char StarCode[] = {0x00, 0x00, 0x00, 0x01};
					char name[32] = {0};
					
					pTmp = (AVPacket *)pdata;
					NextPacket = pTmp->next;
					
					while(pTmp != NULL)
					{
						length = pTmp->DataSize - pTmp->BufOffset;
						if(pTmp->frame_type == 0 || pTmp->frame_type == 1)
						{
							if(TotalLength + 4 <= MAXFRAMELEN)
							{
								SN_MEMCPY(VideoData + TotalLength, MAXFRAMELEN, StarCode, 4, 4);
								TotalLength += 4;
							}
							else
							{						
								TRACE(SCI_TRACE_NORMAL, MOD_PRV, "WARNING: TotalLength + 4 > MAXFRAMELEN\n"); 
								continue;
							}

						}
						if(TotalLength + length <= MAXFRAMELEN)
						{
							SN_MEMCPY(VideoData + TotalLength, MAXFRAMELEN, (char*)pTmp + pTmp->BufOffset, length, length);
							TotalLength += length;
						}
						else
						{						
							TRACE(SCI_TRACE_NORMAL, MOD_PRV, "WARNING: TotalLength + length > MAXFRAMELEN\n"); 
							continue;
						}
						
						if(pTmp->Extendnext == NULL)
						{
							pTmp = (AVPacket *)NextPacket;
							if(pTmp != NULL)
								NextPacket = pTmp->next;
						}
						else
						{
							pTmp = (AVPacket *)(pTmp->Extendnext);
						}						
					}

					SN_SPRINTF(name, sizeof(name), "/tmp/get_ch%d.h264", g_filesign[chn][0]);

					FILE *pf = fopen(name, "a");
					if(NULL!=pf)
					{
						if(TotalLength)
						{
							TRACE(SCI_TRACE_NORMAL, MOD_SLC, "write=%d\n", TotalLength); 
							fwrite(VideoData,TotalLength,1,pf);
						}
						fclose(pf);
					}					
				}								
			
				if(DataFromRTSP->codec_id == CODEC_ID_H264)
				{
					CurSegNo[chn] = DataFromRTSP->seqno;
				}

				//�ȴ�I֡ʱ���������з�I֡����Ƶ���ݲ�����
				if(DataFromRTSP->codec_id == CODEC_ID_H264		//��Ƶ����
					&& VochnInfo[index].bIsWaitIFrame)			//����Ҫ��֡
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "%s Line %d ===========> bIsWaitIFrame: %d, frame_type: %d, chn: %d\n", 
									__func__,__LINE__,VochnInfo[index].bIsWaitIFrame, DataFromRTSP->frame_type, chn);
				
					if(1 != DataFromRTSP->frame_type)//����P֡��ֱ����һ��I֡
					{	
						DiscardPFrameNum[chn]++;
						PreSegNo[chn] = CurSegNo[chn];
						NTRANS_FreeMediaData(DataFromRTSP); 
						DataFromRTSP = NULL;
						continue;
					}
					else		
					{
						VochnInfo[index].bIsWaitIFrame = 0;
						DiscardPFrameNum[chn] = 0;
					}
				}
				
				if(DataFromRTSP->codec_id == CODEC_ID_H264//��֤����Ƶ���ݣ����ܸ���segnoֵ�ж��Ƿ�֡
					&& CurSegNo[chn] > PreSegNo[chn]
					&& (CurSegNo[chn] - PreSegNo[chn]) > 1//segnoֵ������
					&& (1 != DataFromRTSP->frame_type))   //��ǰ֡����I֡�������I֡��֮ǰ��ʹ��������Ҳû��ϵ
				{
					VochnInfo[index].bIsWaitIFrame = 1;			
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Lost Frame---chn: %d, PreSegNo: %d, CurSegNo: %d, CurSegNo-PreSegNo: %d!!!\n", chn, PreSegNo[chn], CurSegNo[chn], CurSegNo[chn] - PreSegNo[chn]);
					PreSegNo[chn] = CurSegNo[chn];
					NTRANS_FreeMediaData(DataFromRTSP); 
					DataFromRTSP = NULL;
					continue;
				}
				
				if(DataFromRTSP->codec_id == CODEC_ID_H264 && (CurSegNo[chn] != 1 && CurSegNo[chn] <= PreSegNo[chn]))
				{
					ReGetIndexPts[chn] = 1;
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "========chn: %d, CurSegNo[chn]: %d, frame_type: %d\n", 
																chn, CurSegNo[chn], DataFromRTSP->frame_type);
					if(DataFromRTSP->frame_type != 1)
					{
						VochnInfo[index].bIsWaitIFrame = 1; 
						PreSegNo[chn] = CurSegNo[chn];	
						NTRANS_FreeMediaData(DataFromRTSP); 
						DataFromRTSP = NULL;
						continue;
					}
				}

				PreSegNo[chn] = CurSegNo[chn];	
				
				PRV_GetPrvMode_EX(&g_stPreviewMode);
				
				//���������£�ֻ��һ·��������Ƶ
				if((DataFromRTSP->codec_id == CODEC_ID_PCMA || DataFromRTSP->codec_id == CODEC_ID_PCMU) && 
					PRV_CurDecodeMode == PassiveDecode  && VoChnState.AudioDataCount[chn] <= 0)
				{
					//��ȡ��Ƶͨ����SDP��Ϣ
					if(RTSP_C_getParam(chn, &RTSP_SDP) != 0)
					{
						TRACE(SCI_TRACE_NORMAL, MOD_PRV, "DataFromRTSP->codec_id == CODEC_ID_PCMA: chn: %d, RTSP_C_getParam(chn, &RTSP_SDP) fail!!!\n", chn); 		
						NTRANS_FreeMediaData(DataFromRTSP); 
						DataFromRTSP = NULL;
						continue;	
					}
					else
					{
						VochnInfo[index].AudioInfo.adoType = RTSP_SDP.adoType;
						VochnInfo[index].AudioInfo.samrate = RTSP_SDP.samrate;
						VochnInfo[index].AudioInfo.soundchannel = RTSP_SDP.soundchannel;
						//PRV_SetVochnInfo(&(VochnInfo[index]), &RTSP_SDP);
						TRACE(SCI_TRACE_NORMAL, MOD_PRV, "DataFromRTSP->codec_id == CODEC_ID_PCMA: RTSP_C_getParam() Success---vdoType: %d, adoType: %d, framerate=%d\n",
														VochnInfo[index].VideoInfo.vdoType, VochnInfo[index].AudioInfo.adoType, RTSP_SDP.framerate);			
					}

					TRACE(SCI_TRACE_NORMAL, MOD_DEC, "----------------index: %d\n", index);
					if(!IsCreateAdec && PRV_StartAdecAo(VochnInfo[index]) == HI_SUCCESS)
					{
						IsCreateAdec = 1;
					}
				}

				//����Դ�����仯ʱ�����»�ȡ��ͨ����������Ϣ
				if(DataFromRTSP->codec_id == CODEC_ID_H264 && DataFromRTSP->frame_type == 1)
				{
					WaitIFrameNum[chn]++;
					s32Ret = PRV_SearchSPS(chn, DataFromRTSP, &new_width, &new_height);
					
					//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "index: %d===DataFromRTSP->pts: %lld, PtsInfo[chn].PreGetVideoPts: %lld\n", index, DataFromRTSP->pts, PtsInfo[chn].PreGetVideoPts);			
					if((s32Ret != HI_SUCCESS && ((VochnInfo[index].VideoInfo.width == 0 && VochnInfo[index].VideoInfo.height == 0)||
						(VochnInfo[index].VideoInfo.width != new_width && VochnInfo[index].VideoInfo.height != new_height)))
									|| ReGetIndexPts[chn] == 1)
					{
						TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line:%d, ReGetIndexPts: %d, s32Ret: %d------P-T-S---CHANGE------index: %d\n",
												__LINE__, ReGetIndexPts[chn], s32Ret, index);			
						PRV_VoChnStateInit(chn);
						PRV_PtsInfoInit(chn);

#if defined(SN9234H1)		
						/*Hi3520*/
						if(s32Ret == -2)  
						{
							TRACE(SCI_TRACE_NORMAL, MOD_PRV, "==========chn: %d, the video type is not support!!!=========\n", chn); 		
							NTRANS_FreeMediaData(DataFromRTSP); 
							DataFromRTSP = NULL;
							VochnInfo[index].bIsWaitIFrame = 1; 
							continue;
						}
#endif					
						/*˫������µ�ͨ����ת��*/
						if(VochnInfo[chn].bIsDouble && PRV_GetDoubleIndex() && chn == PRV_GetDoubleToSingleIndex())
						{
							doublechn = MAX_IPC_CHNNUM;
						}
						else
						{
							doublechn = chn;
						}
						
						s32Ret = RTSP_C_getParam(doublechn, &RTSP_SDP);
						if(s32Ret != 0)
						{
							TRACE(SCI_TRACE_NORMAL, MOD_PRV, "==========chn: %d, RTSP_C_getParam(chn, &RTSP_SDP) fail!!!=========\n", doublechn); 		
							NTRANS_FreeMediaData(DataFromRTSP); 
							DataFromRTSP = NULL;
							VochnInfo[index].bIsWaitIFrame = 1; 
							continue;	
						}
						else
						{
							PRV_SetVochnInfo(&(VochnInfo[index]), &RTSP_SDP);

							VochnInfo[index].VideoInfo.height = 0;
							VochnInfo[index].VideoInfo.width = 0;

							/*������Ԥ��,doublechn��ֵΪ���ͨ����(MAX_IPC_CHNNUM)*/
							TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Line %d---RTSP_C_getParam() Success--doublechn:%d-vdoType: %d, adoType: %d\n",
												__LINE__,doublechn, VochnInfo[index].VideoInfo.vdoType, VochnInfo[index].AudioInfo.adoType);			

							//�������յ�����֮ǰ��Client��·�Ѿ��ϵ�(�л��ȽϿ�ʱ)
							if(VochnInfo[index].VideoInfo.vdoType == 0)
							{
								TRACE(SCI_TRACE_NORMAL, MOD_PRV, "%s Line %d ==========> chn: %d, Allready Disconnect!\n",
																		__func__,__LINE__,chn);			
								VochnInfo[index].bIsWaitIFrame = 1;
								NTRANS_FreeMediaData(DataFromRTSP); 
								DataFromRTSP = NULL;
								continue;
							}
							
							//�����£��ӵ�һ����Ƶ���ݽṹ���л�ȡ��׼ʱ����������´�����Ƶ����ͨ��
							if(PRV_CurDecodeMode == PassiveDecode)
							{
								/*�������������,һ��sock���ܰ������ͨ��������*/
								PtsInfo[chn].FirstVideoPts = DataFromRTSP->pts;						
								PtsInfo[chn].BaseVideoPts = PtsInfo[chn].FirstVideoPts;
								
								TRACE(SCI_TRACE_NORMAL, MOD_PRV, "%s Line %d,PassiveDecode----FistVideopts: %lld\n",
													__func__,__LINE__,PtsInfo[chn].FirstVideoPts);					
							}
						}	

						ReGetIndexPts[chn] = 1;
						
					}
				}

				if(DataFromRTSP->codec_id == CODEC_ID_H264)
				{
					PtsInfo[chn].PreGetVideoPts = DataFromRTSP->pts;
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "DataFromRTSP->codec_id == CODEC_ID_H264---DataFromRTSP->pts: %llu\n", DataFromRTSP->pts);					
				}
				else
				{
					PtsInfo[chn].PreGetAudioPts = DataFromRTSP->pts;
					//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "DataFromRTSP->codec_id == CODEC_ID_PCMA---DataFromRTSP->pts: %lld\n", DataFromRTSP->pts);					
				}
				
				//�������µ�����Ƶʱ���
				if(PRV_CurDecodeMode == SwitchDecode)
				{
					//������Ϣ�ı䣬���»�ȡ����Ƶ��׼ʱ���
					if(ReGetIndexPts[chn] == 1 && DataFromRTSP->codec_id == CODEC_ID_H264)
					{
						if(VochnInfo[chn].bIsDouble && PRV_GetDoubleIndex() && chn == PRV_GetDoubleToSingleIndex())
						{
							doublechn = MAX_IPC_CHNNUM;
						}
						else
						{
							doublechn = chn;
						}
						if(GB_Get_GBMode())
						{
							PtsInfo[chn].FirstVideoPts = DataFromRTSP->pts;	
						}
						else
						{
							NTRANS_getFirstMediaPts(doublechn, &(PtsInfo[chn].FirstVideoPts), &(PtsInfo[chn].FirstAudioPts));
						}
						TRACE(SCI_TRACE_NORMAL, MOD_PRV, "---%s line:%d-----chn=%d--Get Audio and Video data---doublechn: %d, FistVideopts: %lld, FirstAudiopts: %lld\n",
											__FUNCTION__, __LINE__, chn, doublechn, PtsInfo[chn].FirstVideoPts, PtsInfo[chn].FirstAudioPts);
						PtsInfo[chn].BaseVideoPts = PtsInfo[chn].FirstVideoPts;
					}

					//֮ǰ����Ƶ��׼ʱ�����ȡʧ�ܣ����»�ȡ
					if(VochnInfo[index].AudioInfo.adoType >= 0 &&
						(PtsInfo[chn].FirstVideoPts == (~0x0) || PtsInfo[chn].FirstAudioPts == (~0x0)))
					{
						if(VoChnState.VideoDataCount[chn] > 0 && VoChnState.AudioDataCount[chn] > 0)
						{
							if(VochnInfo[chn].bIsDouble && PRV_GetDoubleIndex() && chn == PRV_GetDoubleToSingleIndex())
							{
								doublechn = MAX_IPC_CHNNUM;
							}
							else
							{
								doublechn = chn;
							}
							NTRANS_getFirstMediaPts(doublechn, &(PtsInfo[chn].FirstVideoPts), &(PtsInfo[chn].FirstAudioPts));
							if(PtsInfo[chn].FirstVideoPts != (~0x0) && PtsInfo[chn].FirstAudioPts != (~0x0))
							{
								TRACE(SCI_TRACE_NORMAL, MOD_PRV, "---%s line:%d-------Get Audio and Video data again success---doublechn: %d, FistVideopts: %lld, FirstAudiopts: %lld\n", __FUNCTION__, __LINE__, doublechn, PtsInfo[chn].FirstVideoPts, PtsInfo[chn].FirstAudioPts);
								TRACE(SCI_TRACE_NORMAL, MOD_PRV, "----------VoChnState.VideoDataCount[chn]: %d, VoChnState.AudioDataCount[chn]: %d\n", VoChnState.VideoDataCount[chn], VoChnState.AudioDataCount[chn]);
								PtsInfo[chn].BaseVideoPts = PtsInfo[chn].FirstVideoPts;
							}
						}
					}
				}
				//������ֻ�е��������Ҫ������Ƶͬ������Ҫ��ȡ��һ����Ƶ����ʱ���
				else if(g_stPreviewMode == SingleScene && (DataFromRTSP->codec_id == CODEC_ID_PCMA || DataFromRTSP->codec_id == CODEC_ID_PCMU)
					&& (VochnInfo[index].AudioInfo.adoType >= 0 && PtsInfo[chn].FirstAudioPts == (~0x0)))
				{
					
					PtsInfo[chn].FirstAudioPts = DataFromRTSP->pts;
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "g_stPreviewMode == SingleScene,PassiveDecode----FirstAudioPts: %lld\n", PtsInfo[chn].FirstAudioPts);
				}
				
				if(ReGetIndexPts[chn] == 1 && DataFromRTSP->codec_id == CODEC_ID_H264 && !VochnInfo[index].bIsPBStat)
				{
					if(tmp_PRVGetVideoData[chn] != NULL)
					{
						NTRANS_FreeMediaData(tmp_PRVGetVideoData[chn]); 
						tmp_PRVGetVideoData[chn] = NULL;
					}
					BufferSet(VochnInfo[index].VoChn + PRV_VIDEOBUFFER, MAX_ARRAY_NODE);						
					BufferSet(VochnInfo[index].VoChn + PRV_AUDIOBUFFER, MAX_ARRAY_NODE);
					
					ReGetIndexPts[chn] = 0;
					VochnInfo[index].IsDiscard = 0;
					VochnInfo[index].bIsWaitIFrame = 0;

					VochnInfo[index].PrvType = g_PrvType;
				}
				
				if(DataFromRTSP->codec_id == CODEC_ID_H264
					&& DataFromRTSP->frame_type == 1
					&& VochnInfo[index].PrvType != 0 && !VochnInfo[index].bIsPBStat)
				{
					if(VochnInfo[index].PrvType == Real_Type)//�ı�������:������ʵʱ��������ݣ�����������
					{
						BufferSet(VochnInfo[index].VoChn + PRV_VIDEOBUFFER, MAX_ARRAY_NODE);						
						BufferSet(VochnInfo[index].VoChn + PRV_AUDIOBUFFER, MAX_ARRAY_NODE);
						if(VochnInfo[index].VdecChn >= 0 && VochnInfo[index].IsHaveVdec)
						{
							if(VochnInfo[index].SlaveId == PRV_MASTER)
							{
								CHECK(HI_MPI_VDEC_Query(VochnInfo[index].VdecChn, &pstStat));
								TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Master===VdecChn: %d pstStat.u32LeftStreamFrames: %u\n", VochnInfo[index].VdecChn, pstStat.u32LeftStreamFrames);
								PRV_ReStarVdec(VochnInfo[index].VdecChn);
							}
							else
							{
								SN_SendMccMessageEx(PRV_SLAVE_1, SUPER_USER_ID, MOD_PRV, MOD_PRV, 0, 0, MSG_ID_PRV_MCC_UPDATE_PRVTYPE_REQ, &(VochnInfo[index].VdecChn), sizeof(HI_S32)); 
							}
							
							if(Achn == index)
							{
								PRV_StopAdec();
								VochnInfo[index].AudioInfo.PtNumPerFrm = PtNumPerFrm;
								PRV_StartAdecAo(VochnInfo[index]);	
								IsCreateAdec = 1;
							}
						}
						VoChnState.BeginSendData[chn] = 1;
					}
					else//ʵʱ���������ȴ�500ms�ٿ�ʼ������
					{
						VoChnState.BeginSendData[chn] = 0;
						PtsInfo[chn].BaseVideoPts = DataFromRTSP->pts;
						gettimeofday(&oldTime[chn], NULL);
					}
					VochnInfo[index].PrvType = 0;
				}

				if(DataFromRTSP->codec_id == CODEC_ID_H264 && !VoChnState.FirstHaveVideoData[chn])
					gettimeofday(&oldTime[chn], NULL);
				
				//����������д�뱾�ض�Ӧͨ����Ԥ��������
				s32Ret = PRV_WriteBuffer(chn, DataFromRTSP->codec_id, DataFromRTSP);
				if(s32Ret != HI_SUCCESS)
				{
					DataFromRTSP = NULL;
					continue;
				}

				//PRV_SendDataThread�߳��ӳ�500msȡ���ݣ���Ԥ������������500ms���ݣ���ֹ���綶����ɿ���
				if(!VochnInfo[index].bIsPBStat && PtsInfo[chn].FirstVideoPts != (~0x0) && 
					DataFromRTSP->codec_id == CODEC_ID_H264 && !VoChnState.BeginSendData[chn])
				{
					gettimeofday(&curTime[chn], NULL);
					lastTime[chn] = (curTime[chn].tv_sec * 1000000 + curTime[chn].tv_usec) - (oldTime[chn].tv_sec * 1000000 + oldTime[chn].tv_usec);
					lastPts[chn] = (DataFromRTSP->pts - PtsInfo[chn].BaseVideoPts) * 1000/90000;
					//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "chn=%d, WaitIFrameNum=%d, lastPts=%d, lastTime=%d", chn, WaitIFrameNum[chn], lastPts[chn], lastTime[chn]);
					if(lastPts[chn] >= 500 || lastTime[chn] >= 1000 * 1000 || WaitIFrameNum[chn] >= 5)
					{
						TRACE(SCI_TRACE_NORMAL, MOD_PRV, "%s Line %d,chn: %d===BeginSendData---DataFromRTSP->pts: %llu, lastPts[chn]: %d, lastTime[chn]: %d",
															__func__,__LINE__,chn, DataFromRTSP->pts, lastPts[chn], lastTime[chn]);
						VoChnState.BeginSendData[chn] = 1;
					}
				}

				if(VoChnState.BeginSendData[chn])
				{
					WaitIFrameNum[chn] = 0;
				}
				
			}
			
			pthread_mutex_unlock(&send_data_mutex);
			if (pdata == NULL)
				usleep(1 * 1000);
		}
	}
}


void* PRV_GetPBDataThread()
{
	HI_S32 s32Ret = 0, chn = 0, index = 0, i = 0, ReGetIndexPts[MAX_IPC_CHNNUM] = {0}, count = 0, TotalSize = 0, cidx = 0;
	HI_S32 DiscardPFrameNum[MAX_IPC_CHNNUM] = {0};
	HI_S32 size = 0, RTSPStates = 0, new_width = 0, new_height = 0, MaxLoopTime = 0;
	unsigned int PreSegNo[MAX_IPC_CHNNUM] = {0}, CurSegNo[MAX_IPC_CHNNUM] = {0};//��¼ǰһ����Ƶ��segnoֵ��������Ƶ��segno
	HI_S32 GetDataCount[MAX_IPC_CHNNUM] = {0};
	AVPacket *DataFromRTSP = NULL;
	char *pdata = NULL;
	RTSP_C_SDPInfo RTSP_SDP;
	SN_MEMSET(&RTSP_SDP, 0, sizeof(RTSP_C_SDPInfo));
	Log_pid(__FUNCTION__);

	
	pthread_detach(pthread_self());
	while(1)
	{
		if(IsUpGrade)
		{
			return NULL;
		}
		if(PlayInfo.PlayBackState < PLAY_INSTANT)
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_GetPBDataThread ===sem_wait(&sem_PBGetData)\n");
			
			sem_wait(&sem_PBGetData);	
		}
		while(1)
		{
			if(PlayInfo.PlayBackState < PLAY_INSTANT)
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_GetPBDataThread===PlayInfo.PlayBackState != PLAY_INSTANT && PlayInfo.PlayBackState != PLAY_PROCESS\n");
				break;
			}
			
			if(PlayInfo.IsPause && PlayInfo.PlayBackState > PLAY_INSTANT)
			{
				//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_GetPBDataThread === After stPlayInfo.IsPause===");
				//pthread_mutex_unlock(&send_data_mutex);
				usleep(100 * 1000);
				continue;
			}
			
			if(PlayInfo.PlayBackState == PLAY_INSTANT)
				MaxLoopTime = 1;
			else
				MaxLoopTime = PlayInfo.ImagCount;
			//TRACE(SCI_TRACE_NORMAL, MOD_DEC, "==============PRV_GetPBDataThread===MaxLoopTime: %d\n", MaxLoopTime);
			for(i = 0; i < MaxLoopTime; i++)
			{
				pthread_mutex_lock(&send_data_mutex);
				if(PlayInfo.PlayBackState == PLAY_INSTANT)
				{
					i = PlayInfo.InstantPbChn;
				}
				else
				{
					if(!VochnInfo[i].bIsPBStat)
					{
						//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_GetPBDataThread===chn: %d===No PBState\n", chn);
						pthread_mutex_unlock(&send_data_mutex);
						continue;				
					}
				}
				
				s32Ret = BufferState(i + PRV_VIDEOBUFFER, &count, &TotalSize, &cidx);
				if(PlayStateInfo[i].CurSpeedState != DEC_SPEED_NORMAL && s32Ret == 0 && cidx > PRV_GetBufferSize(i) && VochnInfo[i].IsHaveVdec)
				{
					Ftpc_ChnStopWrite(i, 1);					
					VoChnState.bIsPBStat_StopWriteData[i] = 1;
					TRACE(SCI_TRACE_NORMAL, MOD_DEC, "line: %d======PRV_GetBufferSize(%d)=%d,cidx:%d======bIsPBStat_StopWriteData---StopWrite==1\n", __LINE__, i, PRV_GetBufferSize(i),cidx);
					pthread_mutex_unlock(&send_data_mutex);
					continue;				
				}
				else if(PlayStateInfo[i].CurSpeedState != DEC_SPEED_NORMAL && s32Ret == 0 && cidx < PRV_GetBufferSize(i) && VochnInfo[i].IsHaveVdec)
				{
					Ftpc_ChnStopWrite(i, 0);					
					VoChnState.bIsPBStat_StopWriteData[i] = 0;
				}
				
				//��ʱ�ط���ͣʱ��ȡ����
				if(PlayInfo.PlayBackState == PLAY_INSTANT && PlayStateInfo[PlayInfo.InstantPbChn].CurPlayState == DEC_STATE_NORMALPAUSE)
				{
					if(cidx >= PRV_GetBufferSize(i))
					{
						TRACE(SCI_TRACE_NORMAL, MOD_DEC, "PRV_GetPBDataThread line:%d ===Instant====pause, cidx=%d\n", __LINE__, cidx);
						pthread_mutex_unlock(&send_data_mutex);
						usleep(100 * 1000);
						continue;
					}
				}
				
GetPBDataAgain:				
				s32Ret = BufferGet_Ex(i+1, &pdata, &size, &RTSPStates, PRVUSEONLY);
				if(s32Ret != 0 || pdata == NULL)
				{
					//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "===BufferGet_Ex fail===i: %d, s32Ret: %d\n", i, s32Ret);
					//s32Ret = BufferState(1, &count, &TotalSize, &cidx);
					//if(PlayInfo.PlayBackState == PLAY_INSTANT)
					//	usleep(10*1000);
					pthread_mutex_unlock(&send_data_mutex);
					continue;
				}
				
				DataFromRTSP = (AVPacket *)pdata;
				chn = DataFromRTSP->channel;
				index = chn + LOCALVEDIONUM;
				//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "===BufferGet_Ex Success===chn: %d\n", chn);
				//BufferState(1, &count, &TotalSize, &cidx);
				//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "===BufferState Success===cidx: %d\n", cidx);
				if(!VochnInfo[index].bIsPBStat)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_DEC, "PRV_GetPBDataThread====chn: %d===Not PBState\n", chn);
					NTRANS_FreeMediaData(DataFromRTSP); 
					DataFromRTSP = NULL;			
					pthread_mutex_unlock(&send_data_mutex);
					continue;
				
				}
				if(RTSPStates == 1)
				{
					VochnInfo[chn].bIsWaitIFrame = 1; 
					
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "===PRV_GetPBDataThread===RTSPStates==1,chn: %d\n", chn);
					NTRANS_FreeMediaData(DataFromRTSP); 
					DataFromRTSP = NULL;			
					pthread_mutex_unlock(&send_data_mutex);
					continue;
				}
				s32Ret = PRV_CheckDataPara(DataFromRTSP);
				if(s32Ret != HI_SUCCESS)
				{
					NTRANS_FreeMediaData(DataFromRTSP); 
					DataFromRTSP = NULL;
					VochnInfo[index].bIsWaitIFrame = 1; 
					pthread_mutex_unlock(&send_data_mutex);
					continue;
				}
#if 1
				if(DataFromRTSP->codec_id == CODEC_ID_H264 && g_filesign[chn][0] >= 0 && g_filesign[chn][0] == chn)
				{
					AVPacket *NextPacket = NULL;
					AVPacket *pTmp = NULL;
					int length = 0, TotalLength = 0;
					char StarCode[] = {0x00, 0x00, 0x00, 0x01};
					char name[32] = {0};
					
					pTmp = (AVPacket *)pdata;
					NextPacket = pTmp->next;
					
					while(pTmp != NULL)
					{
						length = pTmp->DataSize - pTmp->BufOffset;
						if(pTmp->frame_type == 0 || pTmp->frame_type == 1)
						{
							if(TotalLength + 4 <= MAXFRAMELEN)
							{
								SN_MEMCPY(VideoData + TotalLength, MAXFRAMELEN, StarCode, 4, 4);
								TotalLength += 4;
							}
							else
							{						
								TRACE(SCI_TRACE_NORMAL, MOD_PRV, "WARNING: TotalLength + 4 > MAXFRAMELEN\n"); 
								continue;
							}

						}
						if(TotalLength + length <= MAXFRAMELEN)
						{
							SN_MEMCPY(VideoData + TotalLength, MAXFRAMELEN, (char*)pTmp + pTmp->BufOffset, length, length);
							TotalLength += length;
						}
						else
						{						
							TRACE(SCI_TRACE_NORMAL, MOD_PRV, "WARNING: TotalLength + length > MAXFRAMELEN\n"); 
							continue;
						}
						if(pTmp->Extendnext == NULL)
						{
							pTmp = (AVPacket *)NextPacket;
							if(pTmp != NULL)
								NextPacket = pTmp->next;
						}
						else
						{
							pTmp = (AVPacket *)(pTmp->Extendnext);
						}
						
					}

					SN_SPRINTF(name, sizeof(name), "/tmp/get_ch%d.h264", g_filesign[chn][0]);

					FILE *pf = fopen(name, "a");
					if(NULL!=pf)
					{
						if(TotalLength)
						{
							TRACE(SCI_TRACE_NORMAL, MOD_SLC, "write=%d\n", TotalLength); 
							fwrite(VideoData,TotalLength,1,pf);
						}
						fclose(pf);
					}
					
				}
#endif
				
				if(DataFromRTSP->codec_id == CODEC_ID_H264)
				{
					CurSegNo[chn] = DataFromRTSP->seqno;
				}
				#if 0
				if(PlayInfo.PlayBackState > PLAY_INSTANT
					&& PlayStateInfo[chn].SynState == SYN_PLAYING
					&& PlayStateInfo[chn].RealType == DEC_TYPE_NOREAL
					&& DataFromRTSP->frame_type != 1)
				{
					NTRANS_FreeMediaData(DataFromRTSP); 
					DataFromRTSP = NULL;
					VochnInfo[index].bIsWaitIFrame = 1; 
					pthread_mutex_unlock(&send_data_mutex);
					continue;

				}
				#endif
				//////////////////////////////////////////////////////////
				//�ȴ�I֡ʱ���������з�I֡����Ƶ���ݲ�����
				if(DataFromRTSP->codec_id == CODEC_ID_H264//��Ƶ����
					&& VochnInfo[index].bIsWaitIFrame)//����Ҫ��֡
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "222222222============bIsWaitIFrame===chn: %d, frame_type: %d\n", chn, DataFromRTSP->frame_type);
				
					if(1 != DataFromRTSP->frame_type)//����P֡��ֱ����һ��I֡
					{	
						DiscardPFrameNum[chn]++;
						PreSegNo[chn] = CurSegNo[chn];
						NTRANS_FreeMediaData(DataFromRTSP); 
						DataFromRTSP = NULL;
						pthread_mutex_unlock(&send_data_mutex);
						continue;
					}
					else
					{
						VochnInfo[index].bIsWaitIFrame = 0;
						DiscardPFrameNum[chn] = 0;
					}
				}
				
				if(DataFromRTSP->codec_id == CODEC_ID_H264//��֤����Ƶ���ݣ����ܸ���segnoֵ�ж��Ƿ�֡
					&& CurSegNo[chn] > PreSegNo[chn]
					&& (CurSegNo[chn] - PreSegNo[chn]) > 1//segnoֵ������
					&& (1 != DataFromRTSP->frame_type))//��ǰ֡����I֡�������I֡��֮ǰ��ʹ��������Ҳû��ϵ
				{
					VochnInfo[index].bIsWaitIFrame = 1; 		
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Lost Frame---chn: %d, PreSegNo: %d, CurSegNo: %d, CurSegNo-PreSegNo: %d!!!\n", chn, PreSegNo[chn], CurSegNo[chn], CurSegNo[chn] - PreSegNo[chn]);
					PreSegNo[chn] = CurSegNo[chn];
					NTRANS_FreeMediaData(DataFromRTSP); 
					DataFromRTSP = NULL;
					pthread_mutex_unlock(&send_data_mutex);
					continue;
				}
				
				if(DataFromRTSP->codec_id == CODEC_ID_H264 && (CurSegNo[chn] == 1 || CurSegNo[chn] <= PreSegNo[chn]))
				{
					ReGetIndexPts[chn] = 1;
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "========chn: %d, CurSegNo[chn]: %d, frame_type: %d\n", chn, CurSegNo[chn], DataFromRTSP->frame_type);
					if(DataFromRTSP->frame_type != 1)
					{
						VochnInfo[index].bIsWaitIFrame = 1; 
						PreSegNo[chn] = CurSegNo[chn];	
						NTRANS_FreeMediaData(DataFromRTSP); 
						DataFromRTSP = NULL;
						pthread_mutex_unlock(&send_data_mutex);
						continue;
					}
				}
			
				PreSegNo[chn] = CurSegNo[chn];
				
				if(DataFromRTSP->codec_id == CODEC_ID_H264 && DataFromRTSP->frame_type == 1)
				{
					s32Ret = PRV_SearchSPS(chn, DataFromRTSP, &new_width, &new_height);
					//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "index: %d===DataFromRTSP->pts: %lld, PtsInfo[chn].PreGetVideoPts: %lld\n", index, DataFromRTSP->pts, PtsInfo[chn].PreGetVideoPts);			
					if((s32Ret != HI_SUCCESS && VochnInfo[index].VideoInfo.width == 0 && VochnInfo[index].VideoInfo.height == 0
						&& abs((DataFromRTSP->pts - PtsInfo[chn].PreGetVideoPts) * 1000/90000) > 8 * 1000)
						/*|| (s32Ret == HI_SUCCESS && abs((DataFromRTSP->pts - PtsInfo[chn].PreGetVideoPts) * 1000/90000) > 8 * 1000)*/
						|| ReGetIndexPts[chn] == 1)
					{
						TRACE(SCI_TRACE_NORMAL, MOD_PRV, "ReGetIndexPts: %d, s32Ret: %d------P-T-S---CHANGE------index: %d\n", ReGetIndexPts[chn], s32Ret, index);			
						PRV_VoChnStateInit(chn);
						BufferSet(chn + PRV_AUDIOBUFFER, MAX_ARRAY_NODE);
						PtsInfo[chn].PreGetVideoPts = DataFromRTSP->pts;
						PlayStateInfo[chn].SynState = SYN_PLAYING;
						PlayStateInfo[chn].CurPlayState = DEC_STATE_NORMAL;
						//����������д�뱾�ض�Ӧͨ����Ԥ��������
						s32Ret = PRV_WriteBuffer(chn, DataFromRTSP->codec_id, DataFromRTSP);
						if(s32Ret != HI_SUCCESS)
						{
							DataFromRTSP = NULL;
						}
						ReGetIndexPts[chn] = 0;
						if(VochnInfo[chn].IsHaveVdec)
						{
							Ftpc_ChnStopWrite(chn, 1);					
							VoChnState.bIsPBStat_StopWriteData[chn] = 1;
							TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line: %d============bIsPBStat_StopWriteData---StopWrite==1\n", __LINE__);
						}
						
						pthread_mutex_unlock(&send_data_mutex);
						continue;					
					}
				
				}
			
				/////////////////////////////////////////////////////////////////////////
				if(DataFromRTSP->codec_id == CODEC_ID_H264)
				{
					PtsInfo[chn].PreGetVideoPts = DataFromRTSP->pts;
					//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "DataFromRTSP->codec_id == CODEC_ID_H264---DataFromRTSP->pts: %llu\n", DataFromRTSP->pts);					
				}
				else
				{
					PtsInfo[chn].PreGetAudioPts = DataFromRTSP->pts;
					//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "DataFromRTSP->codec_id == CODEC_ID_PCMA---DataFromRTSP->pts: %lld\n", DataFromRTSP->pts);					
				}
				s32Ret = PRV_WriteBuffer(chn, DataFromRTSP->codec_id, DataFromRTSP);
				if(s32Ret != HI_SUCCESS)
				{
					DataFromRTSP = NULL;
					pthread_mutex_unlock(&send_data_mutex);
					continue;
				}
				#if 1
				if(DataFromRTSP->codec_id != CODEC_ID_H264)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_DEC, "===DataFromRTSP->codec_id == %d===chn: %d\n", DataFromRTSP->codec_id, chn);
					goto GetPBDataAgain;
				}
				else
				{				
					GetDataCount[chn]++;
					if(GetDataCount[chn] <= 3)
					{
						goto GetPBDataAgain;
					}
					else
					{
						GetDataCount[chn] = 0;
					}
				}
				#endif
				pthread_mutex_unlock(&send_data_mutex);
			}		
			//pthread_mutex_unlock(&send_data_mutex);
			//TRACE(SCI_TRACE_NORMAL, MOD_DEC, "PRV_GetPBDataThread===pthread_mutex_unlock(&send_data_mutex)\n");
			usleep(1 * 1000);
		}
	}
}


/********************************************************
������:PRV_SendDataThread
��     ��:�ӱ��ػ�������ȡ���ݣ�ÿȡһ��I֡�жϷֱ����Ƿ�仯��
			ÿȡһ��I֡��������״̬
��     ��:��
����ֵ:  ��

*********************************************************/
void *PRV_SendDataThread()
{
	HI_S32 s32Ret = 0, chn = 0, index = 0, IsNoData = 0, GetDataCount = 0, GetDataLoopTime = 0, ret = 0, go_fast = 0;
	HI_S32 count = 0, cidx = 0, TotalSize = 0, PRVStatus = 0, MaxLoopTime = 0, LeftSteamFrames = 0, MaxLeftFrames = 0;
	AVPacket *PRVGetVideoData[MAX_IPC_CHNNUM] = {NULL};
	VDEC_CHN_STAT_S pstStat;	
	VO_QUERY_STATUS_S pstVoStatus;
	char *pv = NULL;
	RTSP_C_SDPInfo RTSP_SDP;
	SN_MEMSET(&RTSP_SDP, 0, sizeof(RTSP_C_SDPInfo));
	HI_S64 Data_OldPts[MAX_IPC_CHNNUM] = {0};
	HI_S64 Data_NewPts[MAX_IPC_CHNNUM] = {0};
	HI_S64 Device_Pts[MAX_IPC_CHNNUM] = {0};
	HI_S64 Device_NowPts[MAX_IPC_CHNNUM] = {0};
	
	SN_MEMSET(h_stream, 0, sizeof(h_stream));
	
	Log_pid(__FUNCTION__);
	pthread_detach(pthread_self());
	while(1)
	{
		if(IsUpGrade)
		{
			return NULL;
		}
		//TRACE(SCI_TRACE_NORMAL, MOD_DEC, "PRV_SendDataThread===pthread_mutex_lock(&send_data_mutex)\n");
		pthread_mutex_lock(&send_data_mutex);
		if(PlayInfo.IsPause && PlayInfo.PlayBackState > PLAY_INSTANT)
		{
			TRACE(SCI_TRACE_NORMAL, MOD_PRV, "PRV_SendPBDataThread === stPlayInfo.IsPause===");
			pthread_mutex_unlock(&send_data_mutex);
			usleep(100 * 1000);
			continue;
		}
		if(PlayInfo.PlayBackState <= PLAY_INSTANT)
			MaxLoopTime = MAX_IPC_CHNNUM;
		else
			MaxLoopTime = PlayInfo.ImagCount;
		
		//TRACE(SCI_TRACE_NORMAL, MOD_DEC, "PRV_SendPBDataThread === MaxLoopTime: %d===", MaxLoopTime);
		for(chn = 0; chn < MaxLoopTime; chn++)
		{
			index = chn + LOCALVEDIONUM;
			
			//�ǻط��£�������Ԥ����Ҫ��500ms
			if(!VochnInfo[index].bIsPBStat && (VoChnState.BeginSendData[chn] == 0 || VoChnState.VideoDataCount[chn] <= 0))
			{
				continue;
			}

			if(PlayInfo.PlayBackState >= PLAY_INSTANT)
			{
				SN_MEMSET(&pstStat, 0, sizeof(VDEC_CHN_STAT_S));
				if(HI_MPI_VDEC_Query(VochnInfo[index].VdecChn, &pstStat) != HI_SUCCESS)
					pstStat.u32LeftStreamFrames = 0;
				LeftSteamFrames = (HI_S32)pstStat.u32LeftStreamFrames;
			}
			
			//��ʱ�ط���ͣʱ��������
			if(PlayInfo.PlayBackState == PLAY_INSTANT && VochnInfo[index].bIsPBStat && PlayStateInfo[chn].CurPlayState == DEC_STATE_NORMALPAUSE)
			{
				
				TRACE(SCI_TRACE_NORMAL, MOD_DEC, "1PRV_SendDataThread line:%d===Instant====pause, LeftSteamFrames=%d\n", __LINE__, LeftSteamFrames);
				if(LeftSteamFrames > 5)
				{
					TRACE(SCI_TRACE_NORMAL, MOD_DEC, "LeftSteamFrames=%d\n", LeftSteamFrames);
					continue;
				}
			}

			if(PlayInfo.PlayBackState > PLAY_INSTANT && VochnInfo[index].bIsPBStat)
			{
				if(LeftSteamFrames > PRV_GetBufferSize(index))	
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "%s line:%d, index=%d, LeftSteamFrames=%d, pic=%d\n", __FUNCTION__, __LINE__, index, LeftSteamFrames, (HI_S32)pstStat.u32LeftPics);
					continue;
				}
			}
			
			
			Data_NewPts[index] = get_cur_time();
			//��������״̬������Ƶ������Ҫȥ�����һ������,ʱ��Ϊ2��,������ͨ���������Ƭ����
			if(PRV_CurDecodeMode == PassiveDecode && VochnInfo[index].IsHaveVdec && (Data_NewPts[index] - Data_OldPts[index] > 15.0 * 1000000LL))
			{
				//HI_MPI_VO_ClearChnBuffer(DHD0, VochnInfo[index].VoChn, HI_TRUE);
				//PRV_PassiveDisconnect(chn);
			}
			
			//��Ƭ����(����)������ͨ��������ʱ����Ҫ�ȴ�Ƭ��������ȡ����
			if(VochnInfo[index].SlaveId > PRV_MASTER
				&& (VochnInfo[index].MccReCreateingVdec || VochnInfo[index].MccCreateingVdec))
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Slave Is Creating Vdec: %d, %d---%d\n", chn, VochnInfo[index].MccReCreateingVdec, VochnInfo[index].MccCreateingVdec);
				continue;
			}
			//��Ҫ��֪ͨ��Ƭ��������ͨ��֮ǰ�����I֡���ݷ��ͣ�����ȱ�ٵ�һ��I֡�Ứ��
			if(VochnInfo[index].SlaveId > PRV_MASTER && tmp_PRVGetVideoData[chn] != NULL)
			{
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "chn: %d, Send tmp_PRVGetVideoData\n", chn);
				//��Ƭ(����)�����ɹ�
				if(VochnInfo[index].IsHaveVdec)
				{
					PRV_SetChnPlayPts(chn, tmp_PRVGetVideoData[chn]);					
					PRV_SendData(chn, tmp_PRVGetVideoData[chn], CODEC_ID_H264, MasterToSlaveChnId, 0);	
				}
				else
				{
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "chn: %d,	(Re)Create Slave Vdec fail, Not Send tmp_PRVGetVideoData\n", chn);
					VochnInfo[index].bIsWaitGetIFrame = 1;
				}
				NTRANS_FreeMediaData(tmp_PRVGetVideoData[chn]); 
				tmp_PRVGetVideoData[chn] = NULL;
			}
			//TRACE(SCI_TRACE_NORMAL, MOD_DEC, "before PlayStateInfo[index].bIsResetFile\n");
			//�ط����л��ļ�����Ҫ�����ݽ���
			if(VochnInfo[index].bIsPBStat && PlayStateInfo[index].bIsResetFile)
			{
				IsNoData = 0;
				if(VochnInfo[index].SlaveId == PRV_MASTER)
				{
					HI_MPI_VO_QueryChnStat(0, chn, &pstVoStatus);/* �л��ļ�ʱ��ȴ��ļ�����ȫ���ط���֮���ٴ����ļ���������֡�� */
					if (pstVoStatus.u32ChnBufUsed <= 1)
					{
						HI_MPI_VDEC_Query(chn, &pstStat);
						if ((pstStat.u32LeftPics == 0) && (pstStat.u32LeftStreamFrames == 0))
						{
							IsNoData = 1;
						}
						TRACE(SCI_TRACE_NORMAL, MOD_PRV, "pstVoStatus.u32ChnBufUsed: %d, pstStat.u32LeftPics: %d, pstStat.u32LeftStreamFrames: %d\n", pstVoStatus.u32ChnBufUsed, pstStat.u32LeftPics, pstStat.u32LeftStreamFrames);
					}
				}
				else
				{
					if(PlayStateInfo[chn].QuerySlaveId == 0)
						IsNoData = 1;
				}
				TRACE(SCI_TRACE_NORMAL, MOD_PRV, "=========PlayStateInfo[index].bIsResetFile====chn: %d, VochnInfo[index].SlaveId: %d, IsNoData: %d\n", chn, VochnInfo[index].SlaveId, IsNoData);
				if(IsNoData)//ԭ���ݽ��꣬���½���ԭȡ�������ļ���һ��I֡
				{
					if(Pb_FirstGetVideoData[chn] != NULL)
					{
						s32Ret = PRV_DetectResIsChg(chn, Pb_FirstGetVideoData[chn]);
						VochnInfo[index].bIsWaitGetIFrame = 0;
						if(s32Ret == HI_FAILURE)
						{
							NTRANS_FreeMediaData(Pb_FirstGetVideoData[chn]); 
							Pb_FirstGetVideoData[chn] = NULL; 
							VochnInfo[index].bIsWaitGetIFrame = 1;
							continue;
						}
						else if(s32Ret == 1)//��Ƭ
						{								
							tmp_PRVGetVideoData[chn] = Pb_FirstGetVideoData[chn];							
						}	
					}
					else
					{							
						VochnInfo[index].bIsWaitGetIFrame = 1;
					}
					PlayStateInfo[index].bIsResetFile = 0;
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "====================line: %d, StartPts: %d\n", __LINE__, PtsInfo[chn].StartPts);
			
					if(PtsInfo[chn].StartPts == 0)
					{
						PRM_ID_TIME AllStartTime, AllEndTime;
						time_t AllStartPts, AllEndPts, QueryFinalPts;
						Ftpc_PlayFileAllTime(chn, &AllStartTime, &AllEndTime);
						TRACE(SCI_TRACE_NORMAL, MOD_PRV, "==========AllEndTime: %d-%d-%d,%d.%d.%d\n", AllEndTime.Year, AllEndTime.Month, AllEndTime.Day, AllEndTime.Hour, AllEndTime.Minute, AllEndTime.Second);
						TRACE(SCI_TRACE_NORMAL, MOD_PRV, "==========QueryFinalTime: %d-%d-%d,%d.%d.%d\n", PtsInfo[chn].QueryFinalTime.Year, PtsInfo[chn].QueryFinalTime.Month, PtsInfo[chn].QueryFinalTime.Day, PtsInfo[chn].QueryFinalTime.Hour, PtsInfo[chn].QueryFinalTime.Minute, PtsInfo[chn].QueryFinalTime.Second);

						AllStartPts = PlayBack_PrmTime_To_Sec(&AllStartTime);
						AllEndPts = PlayBack_PrmTime_To_Sec(&AllEndTime);
						QueryFinalPts = PlayBack_PrmTime_To_Sec(&PtsInfo[chn].QueryFinalTime);
						Probar_time[chn] = PlayBack_PrmTime_To_Sec(&PtsInfo[chn].QueryStartTime);

						TRACE(SCI_TRACE_NORMAL, MOD_PRV, "====line: %d, AllStartPts=%d, AllEndPts=%d, QueryFinalPts=%d, Probar_time[chn]=%d\n", __LINE__, AllStartPts, AllEndPts, QueryFinalPts, Probar_time[chn]);

						if(Probar_time[chn] < AllStartPts)
						{
							Probar_time[chn] = AllStartPts;
							PtsInfo[chn].QueryStartTime = AllStartTime;
						}
						if(QueryFinalPts > AllEndPts)
						{
							PtsInfo[chn].QueryFinalTime = AllEndTime;
						}
						
					}
					Ftpc_PlayFileCurTime(chn, &PtsInfo[chn].StartPrm, &PtsInfo[chn].EndPrm);
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "==========PtsInfo[chn].StarPrm: %d-%d-%d,%d.%d.%d\n", PtsInfo[chn].StartPrm.Year, PtsInfo[chn].StartPrm.Month, PtsInfo[chn].StartPrm.Day, PtsInfo[chn].StartPrm.Hour, PtsInfo[chn].StartPrm.Minute, PtsInfo[chn].StartPrm.Second);
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "==========PtsInfo[chn].EndPrm: %d-%d-%d,%d.%d.%d\n", PtsInfo[chn].EndPrm.Year, PtsInfo[chn].EndPrm.Month, PtsInfo[chn].EndPrm.Day, PtsInfo[chn].EndPrm.Hour, PtsInfo[chn].EndPrm.Minute, PtsInfo[chn].EndPrm.Second);
					PtsInfo[chn].StartPts = PlayBack_PrmTime_To_Sec(&PtsInfo[chn].StartPrm);
					if(Probar_time[chn] < (HI_U64)PtsInfo[chn].StartPts)
					{
						Probar_time[chn] = (HI_U64)PtsInfo[chn].StartPts;
			
					}
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "================PtsInfo[chn].StartPts: %d\n", (int)PtsInfo[chn].StartPts);
			
					//PtsInfo[chn].QueryStartTime = PtsInfo[chn].StartPrm;
					//PtsInfo[chn].QueryFinalTime = PtsInfo[chn].EndPrm;
			
					PtsInfo[chn].CurShowPts = (HI_U64)Probar_time[chn]*1000000;
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "==========QueryStarTime: %d-%d-%d,%d.%d.%d\n", PtsInfo[chn].QueryStartTime.Year, PtsInfo[chn].QueryStartTime.Month, PtsInfo[chn].QueryStartTime.Day, PtsInfo[chn].QueryStartTime.Hour, PtsInfo[chn].QueryStartTime.Minute, PtsInfo[chn].QueryStartTime.Second);
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "==========QueryFinalTime: %d-%d-%d,%d.%d.%d\n", PtsInfo[chn].QueryFinalTime.Year, PtsInfo[chn].QueryFinalTime.Month, PtsInfo[chn].QueryFinalTime.Day, PtsInfo[chn].QueryFinalTime.Hour, PtsInfo[chn].QueryFinalTime.Minute, PtsInfo[chn].QueryFinalTime.Second);
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "================PtsInfo[chn].CurShowPts: %lld\n", PtsInfo[chn].CurShowPts);

					Ftpc_ChnStopWrite(chn, 0);					
					VoChnState.bIsPBStat_StopWriteData[chn] = 0;
					TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line: %d============bIsPBStat_StopWriteData---StopWrite==0\n", __LINE__);
					if(s32Ret == 1)
					{						
						continue;
					}
				}
				else
				{
					continue;
				}
			}
			
			if(PlayInfo.PlayBackState > PLAY_INSTANT)
				GetDataLoopTime = 1;
			else 
				GetDataLoopTime = 1;
			
			GetDataCount = 0;
			while(GetDataCount < GetDataLoopTime)
			{
				GetDataCount++;

				if((IsAudioOpen && index == 0 && PRV_CurDecodeMode == PassiveDecode) || (IsAudioOpen && index == Achn) || (PlayInfo.PlayBackState > PLAY_INSTANT && chn == 0))
				{
					if(VochnInfo[index].AudioInfo.adoType != -1 && PRV_GetVoiceTalkState() != HI_TRUE)
					{
						ret = PRV_Video_Audio_SynPro(chn);
						if(ret == 1)
						{
							continue;
						}
					}
				}
				
				if(!VoChnState.IsStopGetVideoData[chn]/* || (VochnInfo[index].bIsPBStat && PlayStateInfo[chn].CurPlayState < DEC_STATE_NORMALPAUSE)*/)
				{
					#if 0
					if(VochnInfo[index].bIsPBStat)
					{
						s32Ret = BufferState(chn + PRV_VIDEOBUFFER, &count, &TotalSize, &cidx);
						if(s32Ret == 0 && cidx > 10)
						{
							Ftpc_ChnStopWrite(chn, 1);					
							VoChnState.bIsPBStat_StopWriteData[chn] = 1;
							TRACE(SCI_TRACE_NORMAL, MOD_DEC, "line: %d============bIsPBStat_StopWriteData---StopWrite==1\n", __LINE__);
						}
							
						TRACE(SCI_TRACE_NORMAL, MOD_DEC, "line: %d, s32Ret: %d, chn: %d, BufferState=====cidx: %d\n", __LINE__, s32Ret, chn, cidx);
					}
					#endif

					s32Ret = BufferState(chn + PRV_VIDEOBUFFER, &count, &TotalSize, &cidx);
					//TRACE(SCI_TRACE_NORMAL, MOD_SLC, "PRVStatus==1----------chn: %d, BufferState===count: %d, TotalSize: %d, cidx: %d\n", chn, count, TotalSize, cidx);
					if(VochnInfo[index].PrvType == Real_Type)
					{
						if(cidx > 15)
						{
							go_fast = 1;
						}
						else
						{
							go_fast = 0;
						}

					}
					else
					{
						if(cidx > 30)
						{
							go_fast = 1;
						}
						else
						{
							go_fast = 0;
						}
						
					}
					s32Ret = BufferGet(chn + PRV_VIDEOBUFFER, &pv, &TotalSize, &PRVStatus);
					if(s32Ret == 0 && pv != NULL) 
					{
						Data_OldPts[index] = get_cur_time();
						Device_Pts[index] = get_cur_time();
						
						//���������λ�������ݱ����������ȡ��һ��I֡
						if(PRVStatus == 1)
						{
							PRVGetVideoData[chn] = (AVPacket *)pv;
							NTRANS_FreeMediaData(PRVGetVideoData[chn]); 
							PRVGetVideoData[chn] = NULL; 
							s32Ret = PRV_GetFirstIFrame(chn, &pv);
							TRACE(SCI_TRACE_NORMAL, MOD_PRV, "s32Ret = %d\n", s32Ret);
							
							if(s32Ret != HI_SUCCESS)
							{
								if(s32Ret == HI_FAILURE)
								{
									VochnInfo[index].bIsWaitGetIFrame = 1;
								}
								break;
							}
							PRVGetVideoData[chn] = (AVPacket *)pv;
							
						}
						else
						{
							PRVGetVideoData[chn] = (AVPacket *)pv;
							//ÿ��ȡһ��I֡����Ҫ���ֱ����Ƿ�仯
							if(PRVGetVideoData[chn]->frame_type == 1)
							{							
								s32Ret = PRV_DetectResIsChg(chn, PRVGetVideoData[chn]);
								//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Chn: %d========PRVGetVideoData[chn]->frame_type=========s32Ret: %d\n", chn, s32Ret);
								VochnInfo[index].bIsWaitGetIFrame = 0;
								if(s32Ret == HI_FAILURE)
								{
									NTRANS_FreeMediaData(PRVGetVideoData[chn]); 
									PRVGetVideoData[chn] = NULL; 
									VochnInfo[index].bIsWaitGetIFrame = 1;
									break;
								}
								else if(s32Ret == 1)
								{								
									tmp_PRVGetVideoData[chn] = PRVGetVideoData[chn]; 
									break;
								}
								else if(s32Ret == 2)
								{
									Pb_FirstGetVideoData[chn] = PRVGetVideoData[chn];
									break;
								}
				
							}
						}
						
					//	GetDataCount++;
						if(VochnInfo[index].bIsWaitGetIFrame && PRVGetVideoData[chn]->frame_type != 1)
						{
							NTRANS_FreeMediaData(PRVGetVideoData[chn]); 
							PRVGetVideoData[chn] = NULL; 
							continue;
						}
						
						if(!VoChnState.IsGetFirstData[chn])
						{
							TRACE(SCI_TRACE_NORMAL, MOD_PRV, "Chn: %d========Get First Frame=====%d====\n", chn, VochnInfo[index].bIsPBStat);
							if(VochnInfo[index].bIsPBStat)
							{
								Ftpc_ChnStopWrite(chn, 0);					
								VoChnState.bIsPBStat_StopWriteData[chn] = 0;
								TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line: %d============bIsPBStat_StopWriteData---StopWrite==0\n", __LINE__);
							}
							VoChnState.IsGetFirstData[chn] = 1;
							SendChnInfo.is_first[chn] = 1;
							PtsInfo[chn].DevicePts = get_cur_time();
						}
						
						//�ǻط���ÿһ��I֡����ѯ������״̬(��Ƭ)������Ƿ���Ҫ��ջ���������ֹ�ӳ�̫��
						//���ܲ���ʱ���������������⣬Ƶ���建��������ɿ���
						if(!VochnInfo[index].bIsPBStat && PRVGetVideoData[chn]->frame_type == 1 && VochnInfo[index].IsHaveVdec && VochnInfo[index].SlaveId == PRV_MASTER)
						{	
							SN_MEMSET(&pstStat, 0, sizeof(VDEC_CHN_STAT_S));
							if(HI_MPI_VDEC_Query(VochnInfo[index].VdecChn, &pstStat) != HI_SUCCESS)
								pstStat.u32LeftStreamFrames = 0;
							LeftSteamFrames = (HI_S32)pstStat.u32LeftStreamFrames;
							if(LeftSteamFrames < 0)
								printf("====index=%d=====LeftSteamFrames: %d\n", index, LeftSteamFrames);
							
							//�����㹻�¿���������ԭ��������һ�����Ķർ����������
							//������������

							if(CurMasterCap <= (TOTALCAPPCHIP/2))
							{
								if(g_PrvType == Real_Type)
								{
									MaxLeftFrames = 25;//50;
								}
								else
								{
									MaxLeftFrames = 45;//50;
								}
								
							}
							else
							{
								if(g_PrvType == Real_Type)
								{
									MaxLeftFrames = 15;//50;
								}
								else
								{
									MaxLeftFrames = 30;//50;
								}
								
							}
							
							//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "===========CurMasterCap: %d, VdecChn: %d, pstStat.u32LeftStreamFrames: %u\n",
							//		CurMasterCap, VochnInfo[index].VdecChn, pstStat.u32LeftStreamFrames);
							if(LeftSteamFrames >= MaxLeftFrames)
							{
								TRACE(SCI_TRACE_NORMAL, MOD_PRV, "======CurMasterCap: %d, g_PrvType=%s, LeftSteamFrames=%d, MaxLeftFrames=%d, VdecChn: %d, pstStat.u32LeftStreamFrames: %u\n",
										CurMasterCap, g_PrvType== Real_Type ? "Real_Type" : "Flow_Type", LeftSteamFrames, MaxLeftFrames, VochnInfo[index].VdecChn, pstStat.u32LeftStreamFrames);
						
								PRV_ReStarVdec(VochnInfo[index].VdecChn);
						
								if(Achn == index && VochnInfo[index].AudioInfo.adoType >= 0)
								{
									PRV_StopAdec();
									VochnInfo[index].AudioInfo.PtNumPerFrm = PtNumPerFrm;
									PRV_StartAdecAo(VochnInfo[index]);	
									IsCreateAdec = 1;
									HI_MPI_ADEC_ClearChnBuf(DecAdec);
								}
							}
						}
					}
					else
					{
						//TRACE(SCI_TRACE_NORMAL, MOD_PRV, "line: %d, BufferGet=====fail===s32Ret: %d\n", __LINE__, s32Ret);
						break;
					}	
				}
				
				if(PRVGetVideoData[chn] != NULL)
				{
					Device_NowPts[index] = get_cur_time()*9/100;
					PtsInfo[index].CurVideoPts = PRVGetVideoData[chn]->pts;
					
					TRACE(SCI_TRACE_NORMAL, MOD_WEB, "11 chn: %d, DeviceIntervalPts=%lld, PreVideoPts=%lld, CurVideoPts=%lld, (%lld) NowPts=%lld, DevicePts=%lld, (%lld)\n", 
							chn, PtsInfo[index].DeviceIntervalPts, PtsInfo[chn].PreVideoPts, PtsInfo[chn].CurVideoPts, (PtsInfo[chn].CurVideoPts - PtsInfo[chn].PreVideoPts), Device_NowPts[index], PtsInfo[chn].DevicePts, (Device_NowPts[index] - PtsInfo[chn].DevicePts));
					
					if(PlayInfo.PlayBackState < PLAY_INSTANT && VoChnState.IsGetFirstData[chn])
					{
						if(go_fast==0 && (PtsInfo[chn].PreVideoPts > 0 && ((Device_NowPts[index] < PtsInfo[chn].DevicePts) || ((Device_NowPts[index] - PtsInfo[chn].DevicePts) < (PtsInfo[chn].CurVideoPts - PtsInfo[chn].PreVideoPts))) && ((PtsInfo[chn].CurVideoPts - PtsInfo[chn].PreVideoPts) <= PtsInfo[index].DeviceIntervalPts)))
						{
							VoChnState.IsStopGetVideoData[chn] = 1;
							TRACE(SCI_TRACE_NORMAL, MOD_WEB, "22 chn: %d, DeviceIntervalPts=%lld, PreVideoPts=%lld, CurVideoPts=%lld, (%lld) NowPts=%lld, DevicePts=%lld, (%lld)\n", 
							chn, PtsInfo[index].DeviceIntervalPts, PtsInfo[chn].PreVideoPts, PtsInfo[chn].CurVideoPts, (PtsInfo[chn].CurVideoPts - PtsInfo[chn].PreVideoPts), Device_NowPts[index], PtsInfo[chn].DevicePts, (Device_NowPts[index] - PtsInfo[chn].DevicePts));
					
							continue;
						}
						else
						{
							if(PtsInfo[chn].PreVideoPts > 0 && PtsInfo[chn].CurVideoPts > PtsInfo[chn].PreVideoPts)
							{
								//PtsInfo[chn].DevicePts += (PtsInfo[chn].CurVideoPts - PtsInfo[chn].PreVideoPts);
								PtsInfo[chn].DevicePts = Device_NowPts[index];
								
								if(PtsInfo[index].DeviceIntervalPts == 0)
								{
									PtsInfo[index].DeviceIntervalPts = PtsInfo[chn].CurVideoPts - PtsInfo[chn].PreVideoPts;
								}
								
								if((PtsInfo[chn].CurVideoPts - PtsInfo[chn].PreVideoPts) > (PtsInfo[index].DeviceIntervalPts + 100))
								{
									PtsInfo[chn].DevicePts = get_cur_time()*9/100;
								}
								else
								{
									PtsInfo[index].DeviceIntervalPts = (PtsInfo[index].DeviceIntervalPts + (PtsInfo[chn].CurVideoPts - PtsInfo[chn].PreVideoPts))/2;
								}
							}
							else
							{
								PtsInfo[chn].DevicePts = get_cur_time()*9/100;
								PtsInfo[index].DeviceIntervalPts = 0;
							}
							
							
							VoChnState.IsStopGetVideoData[chn] = 0;
							TRACE(SCI_TRACE_NORMAL, MOD_WEB, "33 chn: %d, DeviceIntervalPts=%lld, PreVideoPts=%lld, CurVideoPts=%lld, (%lld) NowPts=%lld, DevicePts=%lld, (%lld)\n", 
							chn, PtsInfo[index].DeviceIntervalPts, PtsInfo[chn].PreVideoPts, PtsInfo[chn].CurVideoPts, (PtsInfo[chn].CurVideoPts - PtsInfo[chn].PreVideoPts), Device_NowPts[index], PtsInfo[chn].DevicePts, (Device_NowPts[index] - PtsInfo[chn].DevicePts));
						}

						
					}
					
					PRV_SetChnPlayPts(chn, PRVGetVideoData[chn]);					
					//���豸�ط��£���ʵʱ�ط�ʱ����I֡����
					if(PlayInfo.PlayBackState > PLAY_INSTANT
						&& PlayStateInfo[chn].SynState == SYN_PLAYING
						&& PlayStateInfo[chn].RealType == DEC_TYPE_NOREAL
						&& PRVGetVideoData[chn]->frame_type != 1)
					{
						//printf("==============line: %d\n", __LINE__);
						NTRANS_FreeMediaData(PRVGetVideoData[chn]); 
						PRVGetVideoData[chn] = NULL;
						break;
					}
					
					s32Ret = PRV_SendData(chn, PRVGetVideoData[chn], CODEC_ID_H264, MasterToSlaveChnId, 0);					
					if(VochnInfo[index].SlaveId == PRV_MASTER)
					{
						if(!VochnInfo[index].bIsPBStat && s32Ret == HI_ERR_VDEC_BUF_FULL)
						{
							PRV_ReStarVdec(VochnInfo[index].VdecChn);
							
							if(Achn == index)
							{
								PRV_StopAdec();
								VochnInfo[index].AudioInfo.PtNumPerFrm = PtNumPerFrm;
								PRV_StartAdecAo(VochnInfo[index]);
								IsCreateAdec = 1;
								//HI_MPI_ADEC_ClearChnBuf(DecAdec);
							}
							
							VochnInfo[index].bIsWaitGetIFrame = 1;						
						}		
						
						if(s32Ret == HI_SUCCESS)
						{
							VoChnState.VideoDataTimeLag[chn] = (HI_S64)(PRVGetVideoData[chn]->pts - PtsInfo[chn].FirstVideoPts) * 1000/90000;//��ǰ��Ƶʱ������׼ʱ����Ĳ�ֵ(�����ms)
						}
						else
						{
							VochnInfo[index].bIsWaitGetIFrame = 1;
						}
						
					}
					
					NTRANS_FreeMediaData(PRVGetVideoData[chn]); 
					PRVGetVideoData[chn] = NULL;
					
				}
				
			}			
		}
		pthread_mutex_unlock(&send_data_mutex);
		//TRACE(SCI_TRACE_NORMAL, MOD_DEC, "PRV_SendDataThread===pthread_mutex_unlock(&send_data_mutex)\n");
		usleep(1000);		
	}
}


int PRV_ResetAudioBuffer(int chn)
{
	if(chn < 0 || chn >= DEV_CHANNEL_NUM)
		return -1;
	if(PlayInfo.PlayBackState >= PLAY_INSTANT)
	{
		pthread_mutex_lock(&send_data_mutex);
		BufferSet(chn + PRV_AUDIOBUFFER, MAX_ARRAY_NODE);
		pthread_mutex_unlock(&send_data_mutex);
	}
	return 0;
}

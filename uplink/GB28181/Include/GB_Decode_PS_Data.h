#ifndef __GB_Decode_PS_Data__
#define __GB_Decode_PS_Data__


#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

/*global*/
#include "global_api.h"
#include "global_def.h"
#include "global_msg.h"

/*module*/
//#include "type_def.h"
//#include "rtsp_cmn.h"
//#include "rtsp.h"
//#include "rtp.h"
//#include "codec.h"

#include "GB_sipd.h"
#include "rtpdec.h"


//#if (DVR_SUPPORT_GB == 1)


#define MAX_SIPD_NORMAL_LEN			(64)
#define MAX_SIPD_URI					(128)

#define MAX_SIPD_BODY_LENGTH			(1024)

#define SIPD_LOCAL_SDP_S_FIELD_IPC	("NVR")
#define SIPD_LOCAL_SDP_S_FIELD			SIPD_LOCAL_SDP_S_FIELD_IPC

#define SIP_RTP_VIDEO_PORT				(60002) /*allocate this port to rtp as THE port of sending stream*/
#define SIP_RTP_AUDIO_PORT				(60004)

#define G711A 8				//RTPͷ��G.711 A-law��PTֵ
#define SIP_BASIC_H264 	98 
#define SIP_PS_H264 	96

#define PACK_START_CODE             ((unsigned int)0x000001ba)
#define SYSTEM_HEADER_START_CODE    ((unsigned int)0x000001bb)
#define SEQUENCE_END_CODE           ((unsigned int)0x000001b7)
#define PACKET_START_CODE_MASK      ((unsigned int)0xffffff00)
#define PACKET_START_CODE_PREFIX    ((unsigned int)0x00000100)
#define ISO_11172_END_CODE          ((unsigned int)0x000001b9)

/* mpeg2 */
#define PROGRAM_STREAM_MAP 0x1bc
#define PRIVATE_STREAM_1   0x1bd
#define PADDING_STREAM     0x1be
#define PRIVATE_STREAM_2   0x1bf

#define AUDIO_ID 0xc0
#define VIDEO_ID 0xe0
#define H264_ID  0xe2
#define AC3_ID   0x80
#define DTS_ID   0x88
#define LPCM_ID  0xa0
#define SUB_ID   0x20

#define STREAM_TYPE_VIDEO_MPEG1     0x01
#define STREAM_TYPE_VIDEO_MPEG2     0x02
#define STREAM_TYPE_AUDIO_MPEG1     0x03
#define STREAM_TYPE_AUDIO_MPEG2     0x04
#define STREAM_TYPE_PRIVATE_SECTION 0x05
#define STREAM_TYPE_PRIVATE_DATA    0x06
#define STREAM_TYPE_AUDIO_AAC       0x0f
#define STREAM_TYPE_VIDEO_MPEG4     0x10
#define STREAM_TYPE_VIDEO_H264      0x1b
#define STREAM_TYPE_VIDEO_CAVS      0x42

#define STREAM_TYPE_AUDIO_AC3       0x81

static const int lpcm_freq_tab[4] = { 48000, 96000, 44100, 32000 };

#define MKBETAG(a,b,c,d) ((d) | ((c) << 8) | ((b) << 16) | ((unsigned)(a) << 24))

#define RTP_PACKET_SIZE 1700
#define RTP_PACKET_SIZE1 600



/**
 * Identify the syntax and semantics of the bitstream.
 * The principle is roughly:
 * Two decoders with the same ID can decode the same streams.
 * Two encoders with the same ID can encode compatible streams.
 * There may be slight deviations from the principle due to implementation
 * details.
 *
 * If you add a codec ID to this list, add it so that
 * 1. no value of a existing codec ID changes (that would break ABI),
 * 2. Give it a value which when taken as ASCII is recognized uniquely by a human as this specific codec.
 *    This ensures that 2 forks can independently add AVCodecIDs without producing conflicts.
 *
 * After adding new codec IDs, do not forget to add an entry to the codec
 * descriptor list and bump libavcodec minor version.
 */
enum AVCodecID
{
    AV_CODEC_ID_NONE,

    /* video codecs */
    AV_CODEC_ID_MPEG1VIDEO,
    AV_CODEC_ID_MPEG2VIDEO, ///< preferred ID for MPEG-1/2 video decoding
#if 0 //FF_API_XVMC
    AV_CODEC_ID_MPEG2VIDEO_XVMC,
#endif /* FF_API_XVMC */
    AV_CODEC_ID_H261,
    AV_CODEC_ID_H263,
    AV_CODEC_ID_RV10,
    AV_CODEC_ID_RV20,
    AV_CODEC_ID_MJPEG,
    AV_CODEC_ID_MJPEGB,
    AV_CODEC_ID_LJPEG,
    AV_CODEC_ID_SP5X,
    AV_CODEC_ID_JPEGLS,
    AV_CODEC_ID_MPEG4,
    AV_CODEC_ID_RAWVIDEO,
    AV_CODEC_ID_MSMPEG4V1,
    AV_CODEC_ID_MSMPEG4V2,
    AV_CODEC_ID_MSMPEG4V3,
    AV_CODEC_ID_WMV1,
    AV_CODEC_ID_WMV2,
    AV_CODEC_ID_H263P,
    AV_CODEC_ID_H263I,
    AV_CODEC_ID_FLV1,
    AV_CODEC_ID_SVQ1,
    AV_CODEC_ID_SVQ3,
    AV_CODEC_ID_DVVIDEO,
    AV_CODEC_ID_HUFFYUV,
    AV_CODEC_ID_CYUV,
    AV_CODEC_ID_H264,
    AV_CODEC_ID_INDEO3,
    AV_CODEC_ID_VP3,
    AV_CODEC_ID_THEORA,
    AV_CODEC_ID_ASV1,
    AV_CODEC_ID_ASV2,
    AV_CODEC_ID_FFV1,
    AV_CODEC_ID_4XM,
    AV_CODEC_ID_VCR1,
    AV_CODEC_ID_CLJR,
    AV_CODEC_ID_MDEC,
    AV_CODEC_ID_ROQ,
    AV_CODEC_ID_INTERPLAY_VIDEO,
    AV_CODEC_ID_XAN_WC3,
    AV_CODEC_ID_XAN_WC4,
    AV_CODEC_ID_RPZA,
    AV_CODEC_ID_CINEPAK,
    AV_CODEC_ID_WS_VQA,
    AV_CODEC_ID_MSRLE,
    AV_CODEC_ID_MSVIDEO1,
    AV_CODEC_ID_IDCIN,
    AV_CODEC_ID_8BPS,
    AV_CODEC_ID_SMC,
    AV_CODEC_ID_FLIC,
    AV_CODEC_ID_TRUEMOTION1,
    AV_CODEC_ID_VMDVIDEO,
    AV_CODEC_ID_MSZH,
    AV_CODEC_ID_ZLIB,
    AV_CODEC_ID_QTRLE,
    AV_CODEC_ID_TSCC,
    AV_CODEC_ID_ULTI,
    AV_CODEC_ID_QDRAW,
    AV_CODEC_ID_VIXL,
    AV_CODEC_ID_QPEG,
    AV_CODEC_ID_PNG,
    AV_CODEC_ID_PPM,
    AV_CODEC_ID_PBM,
    AV_CODEC_ID_PGM,
    AV_CODEC_ID_PGMYUV,
    AV_CODEC_ID_PAM,
    AV_CODEC_ID_FFVHUFF,
    AV_CODEC_ID_RV30,
    AV_CODEC_ID_RV40,
    AV_CODEC_ID_VC1,
    AV_CODEC_ID_WMV3,
    AV_CODEC_ID_LOCO,
    AV_CODEC_ID_WNV1,
    AV_CODEC_ID_AASC,
    AV_CODEC_ID_INDEO2,
    AV_CODEC_ID_FRAPS,
    AV_CODEC_ID_TRUEMOTION2,
    AV_CODEC_ID_BMP,
    AV_CODEC_ID_CSCD,
    AV_CODEC_ID_MMVIDEO,
    AV_CODEC_ID_ZMBV,
    AV_CODEC_ID_AVS,
    AV_CODEC_ID_SMACKVIDEO,
    AV_CODEC_ID_NUV,
    AV_CODEC_ID_KMVC,
    AV_CODEC_ID_FLASHSV,
    AV_CODEC_ID_CAVS,
    AV_CODEC_ID_JPEG2000,
    AV_CODEC_ID_VMNC,
    AV_CODEC_ID_VP5,
    AV_CODEC_ID_VP6,
    AV_CODEC_ID_VP6F,
    AV_CODEC_ID_TARGA,
    AV_CODEC_ID_DSICINVIDEO,
    AV_CODEC_ID_TIERTEXSEQVIDEO,
    AV_CODEC_ID_TIFF,
    AV_CODEC_ID_GIF,
    AV_CODEC_ID_DXA,
    AV_CODEC_ID_DNXHD,
    AV_CODEC_ID_THP,
    AV_CODEC_ID_SGI,
    AV_CODEC_ID_C93,
    AV_CODEC_ID_BETHSOFTVID,
    AV_CODEC_ID_PTX,
    AV_CODEC_ID_TXD,
    AV_CODEC_ID_VP6A,
    AV_CODEC_ID_AMV,
    AV_CODEC_ID_VB,
    AV_CODEC_ID_PCX,
    AV_CODEC_ID_SUNRAST,
    AV_CODEC_ID_INDEO4,
    AV_CODEC_ID_INDEO5,
    AV_CODEC_ID_MIMIC,
    AV_CODEC_ID_RL2,
    AV_CODEC_ID_ESCAPE124,
    AV_CODEC_ID_DIRAC,
    AV_CODEC_ID_BFI,
    AV_CODEC_ID_CMV,
    AV_CODEC_ID_MOTIONPIXELS,
    AV_CODEC_ID_TGV,
    AV_CODEC_ID_TGQ,
    AV_CODEC_ID_TQI,
    AV_CODEC_ID_AURA,
    AV_CODEC_ID_AURA2,
    AV_CODEC_ID_V210X,
    AV_CODEC_ID_TMV,
    AV_CODEC_ID_V210,
    AV_CODEC_ID_DPX,
    AV_CODEC_ID_MAD,
    AV_CODEC_ID_FRWU,
    AV_CODEC_ID_FLASHSV2,
    AV_CODEC_ID_CDGRAPHICS,
    AV_CODEC_ID_R210,
    AV_CODEC_ID_ANM,
    AV_CODEC_ID_BINKVIDEO,
    AV_CODEC_ID_IFF_ILBM,
    AV_CODEC_ID_IFF_BYTERUN1,
    AV_CODEC_ID_KGV1,
    AV_CODEC_ID_YOP,
    AV_CODEC_ID_VP8,
    AV_CODEC_ID_PICTOR,
    AV_CODEC_ID_ANSI,
    AV_CODEC_ID_A64_MULTI,
    AV_CODEC_ID_A64_MULTI5,
    AV_CODEC_ID_R10K,
    AV_CODEC_ID_MXPEG,
    AV_CODEC_ID_LAGARITH,
    AV_CODEC_ID_PRORES,
    AV_CODEC_ID_JV,
    AV_CODEC_ID_DFA,
    AV_CODEC_ID_WMV3IMAGE,
    AV_CODEC_ID_VC1IMAGE,
    AV_CODEC_ID_UTVIDEO,
    AV_CODEC_ID_BMV_VIDEO,
    AV_CODEC_ID_VBLE,
    AV_CODEC_ID_DXTORY,
    AV_CODEC_ID_V410,
    AV_CODEC_ID_XWD,
    AV_CODEC_ID_CDXL,
    AV_CODEC_ID_XBM,
    AV_CODEC_ID_ZEROCODEC,
    AV_CODEC_ID_MSS1,
    AV_CODEC_ID_MSA1,
    AV_CODEC_ID_TSCC2,
    AV_CODEC_ID_MTS2,
    AV_CODEC_ID_CLLC,
    AV_CODEC_ID_MSS2,
    AV_CODEC_ID_VP9,
    AV_CODEC_ID_AIC,
    AV_CODEC_ID_ESCAPE130_DEPRECATED,
    AV_CODEC_ID_G2M_DEPRECATED,
    AV_CODEC_ID_WEBP_DEPRECATED,
    AV_CODEC_ID_HNM4_VIDEO,
    AV_CODEC_ID_HEVC_DEPRECATED,
    AV_CODEC_ID_FIC,
    AV_CODEC_ID_ALIAS_PIX,
    AV_CODEC_ID_BRENDER_PIX_DEPRECATED,
    AV_CODEC_ID_PAF_VIDEO_DEPRECATED,
    AV_CODEC_ID_EXR_DEPRECATED,
    AV_CODEC_ID_VP7_DEPRECATED,
    AV_CODEC_ID_SANM_DEPRECATED,
    AV_CODEC_ID_SGIRLE_DEPRECATED,
    AV_CODEC_ID_MVC1_DEPRECATED,
    AV_CODEC_ID_MVC2_DEPRECATED,
    AV_CODEC_ID_HQX,
    AV_CODEC_ID_TDSC,
    AV_CODEC_ID_HQ_HQA,
    AV_CODEC_ID_HAP,
    AV_CODEC_ID_DDS,

    AV_CODEC_ID_BRENDER_PIX= MKBETAG('B','P','I','X'),
    AV_CODEC_ID_Y41P       = MKBETAG('Y','4','1','P'),
    AV_CODEC_ID_ESCAPE130  = MKBETAG('E','1','3','0'),
    AV_CODEC_ID_EXR        = MKBETAG('0','E','X','R'),
    AV_CODEC_ID_AVRP       = MKBETAG('A','V','R','P'),

    AV_CODEC_ID_012V       = MKBETAG('0','1','2','V'),
    AV_CODEC_ID_G2M        = MKBETAG( 0 ,'G','2','M'),
    AV_CODEC_ID_AVUI       = MKBETAG('A','V','U','I'),
    AV_CODEC_ID_AYUV       = MKBETAG('A','Y','U','V'),
    AV_CODEC_ID_TARGA_Y216 = MKBETAG('T','2','1','6'),
    AV_CODEC_ID_V308       = MKBETAG('V','3','0','8'),
    AV_CODEC_ID_V408       = MKBETAG('V','4','0','8'),
    AV_CODEC_ID_YUV4       = MKBETAG('Y','U','V','4'),
    AV_CODEC_ID_SANM       = MKBETAG('S','A','N','M'),
    AV_CODEC_ID_PAF_VIDEO  = MKBETAG('P','A','F','V'),
    AV_CODEC_ID_AVRN       = MKBETAG('A','V','R','n'),
    AV_CODEC_ID_CPIA       = MKBETAG('C','P','I','A'),
    AV_CODEC_ID_XFACE      = MKBETAG('X','F','A','C'),
    AV_CODEC_ID_SGIRLE     = MKBETAG('S','G','I','R'),
    AV_CODEC_ID_MVC1       = MKBETAG('M','V','C','1'),
    AV_CODEC_ID_MVC2       = MKBETAG('M','V','C','2'),
    AV_CODEC_ID_SNOW       = MKBETAG('S','N','O','W'),
    AV_CODEC_ID_WEBP       = MKBETAG('W','E','B','P'),
    AV_CODEC_ID_SMVJPEG    = MKBETAG('S','M','V','J'),
    AV_CODEC_ID_HEVC       = MKBETAG('H','2','6','5'),
//#define AV_CODEC_ID_H265 AV_CODEC_ID_HEVC
    AV_CODEC_ID_VP7        = MKBETAG('V','P','7','0'),
    AV_CODEC_ID_APNG       = MKBETAG('A','P','N','G'),

    /* various PCM "codecs" */
    AV_CODEC_ID_FIRST_AUDIO = 0x10000,     ///< A dummy id pointing at the start of audio codecs
    AV_CODEC_ID_PCM_S16LE = 0x10000,
    AV_CODEC_ID_PCM_S16BE,
    AV_CODEC_ID_PCM_U16LE,
    AV_CODEC_ID_PCM_U16BE,
    AV_CODEC_ID_PCM_S8,
    AV_CODEC_ID_PCM_U8,
    AV_CODEC_ID_PCM_MULAW,
    AV_CODEC_ID_PCM_ALAW,
    AV_CODEC_ID_PCM_S32LE,
    AV_CODEC_ID_PCM_S32BE,
    AV_CODEC_ID_PCM_U32LE,
    AV_CODEC_ID_PCM_U32BE,
    AV_CODEC_ID_PCM_S24LE,
    AV_CODEC_ID_PCM_S24BE,
    AV_CODEC_ID_PCM_U24LE,
    AV_CODEC_ID_PCM_U24BE,
    AV_CODEC_ID_PCM_S24DAUD,
    AV_CODEC_ID_PCM_ZORK,
    AV_CODEC_ID_PCM_S16LE_PLANAR,
    AV_CODEC_ID_PCM_DVD,
    AV_CODEC_ID_PCM_F32BE,
    AV_CODEC_ID_PCM_F32LE,
    AV_CODEC_ID_PCM_F64BE,
    AV_CODEC_ID_PCM_F64LE,
    AV_CODEC_ID_PCM_BLURAY,
    AV_CODEC_ID_PCM_LXF,
    AV_CODEC_ID_S302M,
    AV_CODEC_ID_PCM_S8_PLANAR,
    AV_CODEC_ID_PCM_S24LE_PLANAR_DEPRECATED,
    AV_CODEC_ID_PCM_S32LE_PLANAR_DEPRECATED,
    AV_CODEC_ID_PCM_S16BE_PLANAR_DEPRECATED,
    AV_CODEC_ID_PCM_S24LE_PLANAR = MKBETAG(24,'P','S','P'),
    AV_CODEC_ID_PCM_S32LE_PLANAR = MKBETAG(32,'P','S','P'),
    AV_CODEC_ID_PCM_S16BE_PLANAR = MKBETAG('P','S','P',16),

    /* various ADPCM codecs */
    AV_CODEC_ID_ADPCM_IMA_QT = 0x11000,
    AV_CODEC_ID_ADPCM_IMA_WAV,
    AV_CODEC_ID_ADPCM_IMA_DK3,
    AV_CODEC_ID_ADPCM_IMA_DK4,
    AV_CODEC_ID_ADPCM_IMA_WS,
    AV_CODEC_ID_ADPCM_IMA_SMJPEG,
    AV_CODEC_ID_ADPCM_MS,
    AV_CODEC_ID_ADPCM_4XM,
    AV_CODEC_ID_ADPCM_XA,
    AV_CODEC_ID_ADPCM_ADX,
    AV_CODEC_ID_ADPCM_EA,
    AV_CODEC_ID_ADPCM_G726,
    AV_CODEC_ID_ADPCM_CT,
    AV_CODEC_ID_ADPCM_SWF,
    AV_CODEC_ID_ADPCM_YAMAHA,
    AV_CODEC_ID_ADPCM_SBPRO_4,
    AV_CODEC_ID_ADPCM_SBPRO_3,
    AV_CODEC_ID_ADPCM_SBPRO_2,
    AV_CODEC_ID_ADPCM_THP,
    AV_CODEC_ID_ADPCM_IMA_AMV,
    AV_CODEC_ID_ADPCM_EA_R1,
    AV_CODEC_ID_ADPCM_EA_R3,
    AV_CODEC_ID_ADPCM_EA_R2,
    AV_CODEC_ID_ADPCM_IMA_EA_SEAD,
    AV_CODEC_ID_ADPCM_IMA_EA_EACS,
    AV_CODEC_ID_ADPCM_EA_XAS,
    AV_CODEC_ID_ADPCM_EA_MAXIS_XA,
    AV_CODEC_ID_ADPCM_IMA_ISS,
    AV_CODEC_ID_ADPCM_G722,
    AV_CODEC_ID_ADPCM_IMA_APC,
    AV_CODEC_ID_ADPCM_VIMA_DEPRECATED,
    AV_CODEC_ID_ADPCM_VIMA = MKBETAG('V','I','M','A'),
#if 0 //FF_API_VIMA_DECODER
    AV_CODEC_ID_VIMA       = MKBETAG('V','I','M','A'),
#endif
    AV_CODEC_ID_ADPCM_AFC  = MKBETAG('A','F','C',' '),
    AV_CODEC_ID_ADPCM_IMA_OKI = MKBETAG('O','K','I',' '),
    AV_CODEC_ID_ADPCM_DTK  = MKBETAG('D','T','K',' '),
    AV_CODEC_ID_ADPCM_IMA_RAD = MKBETAG('R','A','D',' '),
    AV_CODEC_ID_ADPCM_G726LE = MKBETAG('6','2','7','G'),
    AV_CODEC_ID_ADPCM_THP_LE = MKBETAG('T','H','P','L'),

    /* AMR */
    AV_CODEC_ID_AMR_NB = 0x12000,
    AV_CODEC_ID_AMR_WB,

    /* RealAudio codecs*/
    AV_CODEC_ID_RA_144 = 0x13000,
    AV_CODEC_ID_RA_288,

    /* various DPCM codecs */
    AV_CODEC_ID_ROQ_DPCM = 0x14000,
    AV_CODEC_ID_INTERPLAY_DPCM,
    AV_CODEC_ID_XAN_DPCM,
    AV_CODEC_ID_SOL_DPCM,

    /* audio codecs */
    AV_CODEC_ID_MP2 = 0x15000,
    AV_CODEC_ID_MP3, ///< preferred ID for decoding MPEG audio layer 1, 2 or 3
    AV_CODEC_ID_AAC,
    AV_CODEC_ID_AC3,
    AV_CODEC_ID_DTS,
    AV_CODEC_ID_VORBIS,
    AV_CODEC_ID_DVAUDIO,
    AV_CODEC_ID_WMAV1,
    AV_CODEC_ID_WMAV2,
    AV_CODEC_ID_MACE3,
    AV_CODEC_ID_MACE6,
    AV_CODEC_ID_VMDAUDIO,
    AV_CODEC_ID_FLAC,
    AV_CODEC_ID_MP3ADU,
    AV_CODEC_ID_MP3ON4,
    AV_CODEC_ID_SHORTEN,
    AV_CODEC_ID_ALAC,
    AV_CODEC_ID_WESTWOOD_SND1,
    AV_CODEC_ID_GSM, ///< as in Berlin toast format
    AV_CODEC_ID_QDM2,
    AV_CODEC_ID_COOK,
    AV_CODEC_ID_TRUESPEECH,
    AV_CODEC_ID_TTA,
    AV_CODEC_ID_SMACKAUDIO,
    AV_CODEC_ID_QCELP,
    AV_CODEC_ID_WAVPACK,
    AV_CODEC_ID_DSICINAUDIO,
    AV_CODEC_ID_IMC,
    AV_CODEC_ID_MUSEPACK7,
    AV_CODEC_ID_MLP,
    AV_CODEC_ID_GSM_MS, /* as found in WAV */
    AV_CODEC_ID_ATRAC3,
#if 0 //FF_API_VOXWARE
    AV_CODEC_ID_VOXWARE,
#endif
    AV_CODEC_ID_APE,
    AV_CODEC_ID_NELLYMOSER,
    AV_CODEC_ID_MUSEPACK8,
    AV_CODEC_ID_SPEEX,
    AV_CODEC_ID_WMAVOICE,
    AV_CODEC_ID_WMAPRO,
    AV_CODEC_ID_WMALOSSLESS,
    AV_CODEC_ID_ATRAC3P,
    AV_CODEC_ID_EAC3,
    AV_CODEC_ID_SIPR,
    AV_CODEC_ID_MP1,
    AV_CODEC_ID_TWINVQ,
    AV_CODEC_ID_TRUEHD,
    AV_CODEC_ID_MP4ALS,
    AV_CODEC_ID_ATRAC1,
    AV_CODEC_ID_BINKAUDIO_RDFT,
    AV_CODEC_ID_BINKAUDIO_DCT,
    AV_CODEC_ID_AAC_LATM,
    AV_CODEC_ID_QDMC,
    AV_CODEC_ID_CELT,
    AV_CODEC_ID_G723_1,
    AV_CODEC_ID_G729,
    AV_CODEC_ID_8SVX_EXP,
    AV_CODEC_ID_8SVX_FIB,
    AV_CODEC_ID_BMV_AUDIO,
    AV_CODEC_ID_RALF,
    AV_CODEC_ID_IAC,
    AV_CODEC_ID_ILBC,
    AV_CODEC_ID_OPUS_DEPRECATED,
    AV_CODEC_ID_COMFORT_NOISE,
    AV_CODEC_ID_TAK_DEPRECATED,
    AV_CODEC_ID_METASOUND,
    AV_CODEC_ID_PAF_AUDIO_DEPRECATED,
    AV_CODEC_ID_ON2AVC,
    AV_CODEC_ID_DSS_SP,
    AV_CODEC_ID_FFWAVESYNTH = MKBETAG('F','F','W','S'),
    AV_CODEC_ID_SONIC       = MKBETAG('S','O','N','C'),
    AV_CODEC_ID_SONIC_LS    = MKBETAG('S','O','N','L'),
    AV_CODEC_ID_PAF_AUDIO   = MKBETAG('P','A','F','A'),
    AV_CODEC_ID_OPUS        = MKBETAG('O','P','U','S'),
    AV_CODEC_ID_TAK         = MKBETAG('t','B','a','K'),
    AV_CODEC_ID_EVRC        = MKBETAG('s','e','v','c'),
    AV_CODEC_ID_SMV         = MKBETAG('s','s','m','v'),
    AV_CODEC_ID_DSD_LSBF    = MKBETAG('D','S','D','L'),
    AV_CODEC_ID_DSD_MSBF    = MKBETAG('D','S','D','M'),
    AV_CODEC_ID_DSD_LSBF_PLANAR = MKBETAG('D','S','D','1'),
    AV_CODEC_ID_DSD_MSBF_PLANAR = MKBETAG('D','S','D','8'),
    AV_CODEC_ID_4GV         = MKBETAG('s','4','g','v'),

    /* subtitle codecs */
    AV_CODEC_ID_FIRST_SUBTITLE = 0x17000,          ///< A dummy ID pointing at the start of subtitle codecs.
    AV_CODEC_ID_DVD_SUBTITLE = 0x17000,
    AV_CODEC_ID_DVB_SUBTITLE,
    AV_CODEC_ID_TEXT,  ///< raw UTF-8 text
    AV_CODEC_ID_XSUB,
    AV_CODEC_ID_SSA,
    AV_CODEC_ID_MOV_TEXT,
    AV_CODEC_ID_HDMV_PGS_SUBTITLE,
    AV_CODEC_ID_DVB_TELETEXT,
    AV_CODEC_ID_SRT,
    AV_CODEC_ID_MICRODVD   = MKBETAG('m','D','V','D'),
    AV_CODEC_ID_EIA_608    = MKBETAG('c','6','0','8'),
    AV_CODEC_ID_JACOSUB    = MKBETAG('J','S','U','B'),
    AV_CODEC_ID_SAMI       = MKBETAG('S','A','M','I'),
    AV_CODEC_ID_REALTEXT   = MKBETAG('R','T','X','T'),
    AV_CODEC_ID_STL        = MKBETAG('S','p','T','L'),
    AV_CODEC_ID_SUBVIEWER1 = MKBETAG('S','b','V','1'),
    AV_CODEC_ID_SUBVIEWER  = MKBETAG('S','u','b','V'),
    AV_CODEC_ID_SUBRIP     = MKBETAG('S','R','i','p'),
    AV_CODEC_ID_WEBVTT     = MKBETAG('W','V','T','T'),
    AV_CODEC_ID_MPL2       = MKBETAG('M','P','L','2'),
    AV_CODEC_ID_VPLAYER    = MKBETAG('V','P','l','r'),
    AV_CODEC_ID_PJS        = MKBETAG('P','h','J','S'),
    AV_CODEC_ID_ASS        = MKBETAG('A','S','S',' '),  ///< ASS as defined in Matroska
    AV_CODEC_ID_HDMV_TEXT_SUBTITLE = MKBETAG('B','D','T','X'),

    /* other specific kind of codecs (generally used for attachments) */
    AV_CODEC_ID_FIRST_UNKNOWN = 0x18000,           ///< A dummy ID pointing at the start of various fake codecs.
    AV_CODEC_ID_TTF = 0x18000,
    AV_CODEC_ID_BINTEXT    = MKBETAG('B','T','X','T'),
    AV_CODEC_ID_XBIN       = MKBETAG('X','B','I','N'),
    AV_CODEC_ID_IDF        = MKBETAG( 0 ,'I','D','F'),
    AV_CODEC_ID_OTF        = MKBETAG( 0 ,'O','T','F'),
    AV_CODEC_ID_SMPTE_KLV  = MKBETAG('K','L','V','A'),
    AV_CODEC_ID_DVD_NAV    = MKBETAG('D','N','A','V'),
    AV_CODEC_ID_TIMED_ID3  = MKBETAG('T','I','D','3'),
    AV_CODEC_ID_BIN_DATA   = MKBETAG('D','A','T','A'),


    AV_CODEC_ID_PROBE = 0x19000, ///< codec_id is not known (like AV_CODEC_ID_NONE) but lavf should attempt to identify it

    AV_CODEC_ID_MPEG2TS = 0x20000, /**< _FAKE_ codec to indicate a raw MPEG-2 TS
                                * stream (only used by libavformat) */
    AV_CODEC_ID_MPEG4SYSTEMS = 0x20001, /**< _FAKE_ codec to indicate a MPEG-4 Systems
                                * stream (only used by libavformat) */
    AV_CODEC_ID_FFMETADATA = 0x21000,   ///< Dummy codec for streams containing only metadata information.
};


/**
 * @addtogroup lavu_media Media Type
 * @brief Media Type
 */
enum AVMediaType {
    AVMEDIA_TYPE_UNKNOWN = -1,  ///< Usually treated as AVMEDIA_TYPE_DATA
    AVMEDIA_TYPE_VIDEO,
    AVMEDIA_TYPE_AUDIO,
    AVMEDIA_TYPE_DATA,          ///< Opaque data information usually continuous
    AVMEDIA_TYPE_SUBTITLE,
    AVMEDIA_TYPE_ATTACHMENT,    ///< Opaque data information usually sparse
    AVMEDIA_TYPE_NB
};




// ý����������
enum SIPD_STREAM_TYPE
{
	SIPD_STREAM_TYPE_NONE = 0, 
	SIPD_STREAM_TYPE_PLAY = 1,       //ʵʱ�㲥
	SIPD_STREAM_TYPE_PLAYBACK = 2,   // �ط�
	SIPD_STREAM_TYPE_DOWNLOAD = 3    // ����
};

enum SIPD_MEDIA_TYPE
{
	SIPD_MEDIA_TYPE_NONE = 0, 
	SIPD_MEDIA_TYPE_VIDEO = 1,   // ��Ƶ
	SIPD_MEDIA_TYPE_AUDIO = 2,   // ��Ƶ
	SIPD_MEDIA_TYPE_MIX = 3   // ����Ƶ
};

enum SIPD_MEDIA_F_VIDEO_TYPE
{
	SIPD_MEDIA_F_VIDEO_TYPE_DEF = 0, 
	SIPD_MEDIA_F_VIDEO_TYPE_MPEG4 = 1, 
	SIPD_MEDIA_F_VIDEO_TYPE_H264 = 2, 
	SIPD_MEDIA_F_VIDEO_TYPE_SVAC = 3, 
	SIPD_MEDIA_F_VIDEO_TYPE_3GP = 4
};

enum SIPD_MEDIA_F_VIDEO_RESOLUTION
{
	SIPD_MEDIA_F_VIDEO_RESOLUTION_DEF = 0, 
	SIPD_MEDIA_F_VIDEO_RESOLUTION_QCIF = 1, 
	SIPD_MEDIA_F_VIDEO_RESOLUTION_CIF = 2, 
	SIPD_MEDIA_F_VIDEO_RESOLUTION_4CIF = 3, 
	SIPD_MEDIA_F_VIDEO_RESOLUTION_D1 = 4, 
	SIPD_MEDIA_F_VIDEO_RESOLUTION_720P = 5, 
	SIPD_MEDIA_F_VIDEO_RESOLUTION_1080P = 6
};

enum SIPD_MEDIA_F_VIDEO_RATE_TYPE
{
	SIPD_MEDIA_F_VIDEO_RATE_TYPE_DEF = 0, 
	SIPD_MEDIA_F_VIDEO_RATE_TYPE_CBR = 1, 
	SIPD_MEDIA_F_VIDEO_RATE_TYPE_VBR = 2
};

enum SIPD_MEDIA_F_AUDIO_TYPE
{
	SIPD_MEDIA_F_AUDIO_TYPE_DEF = 0, 
	SIPD_MEDIA_F_AUDIO_TYPE_G711 = 1, 
	SIPD_MEDIA_F_AUDIO_TYPE_G723 = 2, 
	SIPD_MEDIA_F_AUDIO_TYPE_G729 = 3, 
	SIPD_MEDIA_F_AUDIO_TYPE_G722 = 4
};

enum SIPD_MEDIA_F_AUDIO_BITRATE
{
	SIPD_MEDIA_F_AUDIO_BITRATE_DEF = 0, 
	SIPD_MEDIA_F_AUDIO_BITRATE_5P3 = 1, 
	SIPD_MEDIA_F_AUDIO_BITRATE_6P3 = 2, 
	SIPD_MEDIA_F_AUDIO_BITRATE_8 = 3, 
	SIPD_MEDIA_F_AUDIO_BITRATE_16 = 4, 
	SIPD_MEDIA_F_AUDIO_BITRATE_24 = 5, 
	SIPD_MEDIA_F_AUDIO_BITRATE_32 = 6,
	SIPD_MEDIA_F_AUDIO_BITRATE_48 = 7, 
	SIPD_MEDIA_F_AUDIO_BITRATE_64 = 8
};

enum SIPD_MEDIA_F_AUDIO_SAMPLE_RATE
{
	SIPD_MEDIA_F_AUDIO_SAMPLE_RATE_DEF = 0, 
	SIPD_MEDIA_F_AUDIO_SAMPLE_RATE_8 = 1, 
	SIPD_MEDIA_F_AUDIO_SAMPLE_RATE_14 = 2, 
	SIPD_MEDIA_F_AUDIO_SAMPLE_RATE_16 = 3, 
	SIPD_MEDIA_F_AUDIO_SAMPLE_RATE_32 = 4
};


// SDP������Ĵ洢�ṹ
struct sipd_media_session
{
	int cid;
	int did;
	enum SIPD_STREAM_TYPE stream_type;  // ý����������:  Play Playback Download
	char connect_video_ip[MAX_SIPD_URI];
	char connect_video_port[MAX_SIPD_NORMAL_LEN];
	char connect_video_addrtype[MAX_SIPD_NORMAL_LEN]; /*reserved*/
	char connect_audio_ip[MAX_SIPD_URI];
	char connect_audio_port[MAX_SIPD_NORMAL_LEN];
	char connect_audio_addrtype[MAX_SIPD_NORMAL_LEN]; /*reserved*/
	enum SIPD_MEDIA_TYPE media_type;
	char u_field[MAX_SIPD_URI];   // "u="�ֶΣ�����Ƶ��URI
	char t_start_time[MAX_SIPD_NORMAL_LEN];  // "t="�ֶεĿ�ʼʱ��: ��1970��1��1�գ�UTC/GMT����ҹ����ʼ������������
	char t_end_time[MAX_SIPD_NORMAL_LEN];    // "t="�ֶεĽ���ʱ��
	char pri_ssrc_field[MAX_SIPD_NORMAL_LEN];  // RTP��ͷ�е�SSRCֵ
	char pri_ssrc_audio_field[MAX_SIPD_NORMAL_LEN];
	char pri_f_field[MAX_SIPD_URI];   // "f="�ֶΣ������ʽ���ֱ��ʵ�
	int f_enable;
	enum SIPD_MEDIA_F_VIDEO_TYPE f_video_type;
	enum SIPD_MEDIA_F_VIDEO_RESOLUTION f_resolution;
	int f_framerate;
	enum SIPD_MEDIA_F_VIDEO_RATE_TYPE f_rate_type;
	int bitrate;
	enum SIPD_MEDIA_F_AUDIO_TYPE audio_type;
	enum SIPD_MEDIA_F_AUDIO_BITRATE audio_bitrate;
	enum SIPD_MEDIA_F_AUDIO_SAMPLE_RATE audio_samplerate;	
	sdp_message_t* sdp_from_osip;  // ����osip��Դ����Ľӿڽ����õ���SDP��Ϣ
	int local_video_port;  // �ڷ�װSDPʱ����ı��ض˿ڣ����ں����İ�socket�Ĳ���
	int local_audio_port;
};

enum SipLinkState 
{
	GB_SIP_STATE_NO_INIT,   //  ��·δ��ʼ����û���յ� Invite����	
	GB_SIP_STATE_WAIT_REQUEST,   // �ȴ�����:  Invite ACK BYE INFO��, ��ʼ��״̬
	GB_SIP_STATE_SEND_REPLY,     // ������Ӧ:  ��������ʱ��SIP��Ϣ�̷߳�����Ϣ
	GB_SIP_STATE_SEND_DATA,      // ��������:  ������ɹ���ʼ��������Ƶ����
	GB_SIP_STATE_ERROR_EXIT,     // ���������˳�
	GB_SIP_STATE_FILE_END        // �طŻ������ļ�����:File_To_End
};

enum GB_RTSPState 
{
	GB_RTSP_INVALID,	  // ��Ч״̬
	GB_RTSP_READY,		  // �յ�SDP���ظ�200 OK����ý�崫�����        	
	GB_RTSP_PLAYING, 	  // ACK֮��ֵ�����ڷ�������. �طŹ����У���״̬����Ҫ��������������ݿ��������ȡ��������
	GB_RTSP_RESUMING,     // ����
	GB_RTSP_PAUSE,		  // ��ͣ 
	GB_RTSP_STOP,		  // ֹͣ
	GB_RTSP_SPEED_UP_OR_DOWN,  // ��Ż�����
};

enum AVMuxteFmt
{
	AVFMT_AVI,
	AVFMT_PS,
	AVFMT_MPEG4
};


typedef struct {
	enum AVMuxteFmt fmt;
	void *priv;   // PS header
}AVMutexContext;


// �طſ��Ʋ���
struct RtspCtrlMsg 
{
	enum GB_RTSPState rtsp_cmd;
	unsigned cseq;
	double rangeStart; // ����¼���������ֵ����λΪ��
	double rangeEnd;  // ����¼����յ�ʱ��
	float scale;  // ���������
	char rangeFlag; // 0 - ������; 1 - ����   // range �� scale �ֶβ���ͬʱ���ڣ����������Ч����
	char scaleFlag; // 0 - ������; 1 - ����
	char pauseFlag;  // 0 - ���� ;  1 - ��ͣ
};

struct PlaybackFileInfo
{
	int need_pos;  // �Ƿ���Ҫƫ��
	PRM_ID_TIME pos_time;  // ������¼�Ŀ�ʼʱ��
	PRM_ID_FILE_INFO FileInfo;
};

struct PlaybackFileList
{
	int	FileCount;
	struct PlaybackFileInfo FileList[0];
};


struct gb_playback_file
{
	int cmd;
	int size;
	unsigned int base_time0_in_seconds;	/*��ѯ��������ʼʱ���0�� ��Ӧ1970�꿪ʼ��ƫ�ƣ������*/ 
										// ntpʱ��,�ڽ���SDP��"t="�ֶκ�õ�
	struct PlaybackFileList *filelist;
};


struct GBAblit
{
	unsigned int support_h265;
};


struct gb_playback_request
{
	int session_id;

	struct GBAblit ability;
	
	/*¼��ط�ʹ�õ�GB Buffer��ͨ��������Ϣ*/
	int chn;   // ��1��ʼ
	int v_stream;	/*-1: ��������Ƶ�� >=0 ����Ƶ����*/
	int a_stream;	/*-1: ��������Ƶ�� >=0 ����Ƶ����*/
	char v_feed[50];
	char a_feed[50];


	/*��������*/
	int code; // �ط�����: ���š���ͣ��ֹͣ��
	PRM_ID_TIME start_time;	/*Ҫ���ŵ���ʼʱ��*/
	double rangeStart; // ����¼���������ֵ����λΪ��
	double rangeEnd;  // ����¼����յ�ʱ��
	float scale;  // ���������
	unsigned long long time_interval_to_playback; // �طų�����ʱ��,��λΪ��
	float DownloadSpeed;
	enum SIPD_STREAM_TYPE stream_type;

	struct gb_playback_file file;
};

struct GB_PlaybackHandle
{
	enum GB_RTSPState state;

	int session_id;
	struct GBAblit ability;
	struct pollfd *poll_entry;
	
	int chn;  // ��1��ʼ
	int v_stream;
	int a_stream;
	char v_feed[50];
	char a_feed[50];

	/*�����ļ���ʽ���*/
	int fileIndex;
	HANDLE *handle;
	int  curChunk;
	int ChunkNum;
	
	/*�ط����*/
	Fileheader fileheader;
	int first_pts;
	unsigned long long file_base_pts;	/*���ļ�����ʼpts*/

	double rangeStart; // ����¼���������ֵ����λΪ��
	double rangeEnd;  // ����¼����յ�ʱ��
	float scale;  // ���������
	unsigned long long time_interval_to_playback; // �طų�����ʱ��,��λΪ��

	/*�����������*/
	int first_video_pts;
	int first_audio_pts;
	int change_speed_flag; // �Ƿ���Ҫ������׼ʱ���: 0 - ���øı�;  1 - ��Ҫ�ı�
	unsigned long long base_pts_for_change_speed; // �ڿ�Ż�����ʱ�Ļ�׼ʱ���,���ڼ��㵱ǰ֡��ʱ���
	unsigned long long first_video_frame_pts; // ��ȡ���ĵ�һ֡��Ƶ��ʱ�������λΪ΢��
	unsigned long long first_audio_frame_pts; // ��ȡ���ĵ�һ֡��Ƶ��ʱ��� 
	unsigned long long now_pts_add_seconds;
	unsigned long long sys_pts_add_seconds;	
	unsigned long long pts_from_cur_time;

	unsigned long long recent_video_pts;
	unsigned long long recent_audio_pts;
	unsigned long long last_frame_pts;

	unsigned long long playback_start_pts; // �طſ�ʼ��ʱ���
	unsigned long long sys_base_pts;	/*���Ÿ��ļ���һ֡ʱ��ϵͳʱ��*/
	unsigned long long play_base_pts;	/*���ݲ�ѯ�����ĵ�һ��0:0:0.0Ϊ0�㣬���ļ���һ֡����Ӧ��ʱ��(΢��)*/
	
	AVPacket *av_packet;
	unsigned long long packet_pts; // ��ȡ�ļ�ʱ��ȡ��ʱ�������λΪ΢��
	unsigned long long packet_sys_pts; // ��ȡ�ļ��󣬾���ת����ʱ���,��λΪ΢��
	int packet_type; // ֡����:  ��Ƶ��ʶ  int : 1667510320  ; ��Ƶ��ʶ  int : 1651978544
	int chunk_is_end;  // ���ļ���ȡ�Ƿ��Ѿ����

	int frame_cnt;
	int frame_count;
	int frame_count_show;
	int milliseconds_per_frame;  // ֡���,��λΪ����
	int time_interval_to_send_frames; // �ط�ʱ��֡���ͼ����ÿ������������֡,��λΪ΢��
	float DownloadSpeed;
	enum SIPD_STREAM_TYPE stream_type;
	int frame_rate;

	struct gb_playback_file file;
};


typedef struct MpegDemuxContext {
    //AVClass *class;
    int32_t header_state;
    unsigned char psm_es_type[256];
    int sofdec;
    int dvd;
    int imkh_cctv;
#if 0 //CONFIG_VOBSUB_DEMUXER
    AVFormatContext *sub_ctx;
    FFDemuxSubtitlesQueue q[32];
    char *sub_name;
#endif
} MpegDemuxContext;



/*SIP_Context:��¼һ���Ự�ı���״̬*/
struct SIP_Context
{
	enum GB_RTSPState state;
	int sip_video_fd;  /*�ڱ��ض˿ڲ�Ϊ��ʱ����һ��socket���ڽ�������*/
	int send_ps_fd;  /*ȡ��RTPͷ��,���ڷ���PS����socket*/
	int recv_ps_fd;  /*����PS����socket*/
	 
	struct pollfd *poll_entry; /*������ѯ*/
	struct pollfd *poll_entry_stream;

	RTPDemuxContext *rtpDemuxContext; /*����RTP���ݽ���*/
	MpegDemuxContext *mpegDemuxContext; /*����PS���ݽ���*/

	/* Reusable buffer for receiving packets */
	int recv_buf_size;  // �������Ĵ�С
	unsigned char *recv_buffer_ptr, *recv_buffer_end;  //������ָ��
	unsigned char *recv_buffer;	/*RTP���ݻ�����*/
	int ps_data_length; /*PS����(RTP����)�ĳ���*/
	int pes_packet_length; /*һ��PES���ĳ���,����ȷ��PES�������Ƿ����*/
	int pes_bytes_have_read; /*�Ѿ���ȡ�������ݳ���*/
	short isMissedPacket; /*0 - δ����; 1 - ��������*/

	AVPacket* fPacketReadInProgress; /*��ǰ���ڶ�ȡ�����ݰ�*/
	AVPacket* fPacketSentToDecoder; /*���ݸ�Ԥ��ģ������ݰ�*/
	short streamTypeBit; /* 0 - ��Ƶ ; 1 - ��Ƶ*/
	int code_id;
	unsigned long long DataCount;/*���ݸ�Ԥ��ģ���֡���*/

	char chn; //ͨ����, ��1��ʼ, ����SIP��Ϣ�̷߳��͹�����chn+1
    char session_id[MAX_GB_SIPD_SDP_SIZE];  /*�ỰID,�ỰΨһ�Եı�ʶ*/
};


typedef struct
{
	unsigned short seq_num;       
	unsigned long ssrc1;
	unsigned long cur_timestamp;  
}RTP_VALUE;


// ��¼һ��RTP��(��Ƶ������Ƶ��)�ı���״̬
struct GBRTPStream 
{
	int chn;	//��1��ʼ���
	int bit_stream;//����: 0:��Ƶ 1:������ 2: ������
	char feed[50];  // ������еı�ʶ
	int server_fd;  // VAMд�������ʱ���������socket��״̬���ж�����
	//int max_payload_size;
	
	struct pollfd *poll_entry;
	struct sockaddr_in from_addr;  // Ŀ�ĵ�ַ
	//u64_t timeout;
	
	enum GB_RTSPState state;
	CodecID		codec_id;  // ����������

	int is_first_pts;
	/* rtcp sender statistics receive */
   // u64_t last_rtcp_ntp_time;   

    /* rtcp sender statistics */
   // unsigned int packet_count;
   // unsigned int octet_count;
  //  unsigned int last_octet_count;
    int first_packet;
	
	/*tcp*/
	int rtsp_fd; // �������ݵ�socket
	int interleave;
	/*udp*/
	int rtp_fd[2];  // rtp_fd[0]: rtp ; rtp_fd[1]: rtcp
	int port[2];
	int dest_port[2];

	/*the buffer to send*/
	int packet_size;
	char *packet_buffer; // ���ݻ�����
	char *packet_buffer_ptr, *packet_buffer_end;  // ���ݻ�����ָ��

	/*the AVPacket*/
	int nal_id;
	 AVPacket *av_packet;//���ڼ�¼�Ƿ��а�û�з�����
	 AVPacket *packet_ptr, *packet_list;//packet_listָ��rtppacket����,packet_ptrָ�������м������͵Ľڵ�
	 AVPacket *packet_rec;//packet_recָ�������м������͵Ľڵ�
	char *av_nal_start, *av_nal_end;
	char *av_nal_ptr;
	
	struct SIP_Context *rtsp_c;  //��ʶSIP�Ự��ָ��

	struct GBRTPStream *rtp_next; // ��ʶ��һ��rtp����ָ��
	AVPacket *packet_next;//��ʶ��һ��rtp���ָ��
	AVMutexContext avmutex; //����PS���ʱ�İ�ͷ��ʼ��
	int get_first_IFrame;
	RTP_VALUE rtp_v;
};

struct AVPacketExtend 
{
	CodecID codec_id;
	int frame_type;  // 1 - �ؼ�֡   0 - �ǹؼ�֡

	unsigned long long pts64;	
	unsigned long long pts64_base; /*��׼ʱ���*/
	unsigned int pts;			/*rtp��PTS*/  // pts = pts64 *90000 /1000000
	unsigned long long ntp;
	unsigned long other_frames;	/*�ӱ�������������SPS+PPS+SEI�� ��¼��������Ǹ�֡��
								����ʱ��*/

//	pthread_mutex_t lock;
	volatile int count;      		/*��ǰ�������ü���,��ֵֻ�ɱ�AVBuffer�ڲ��޸ģ�
						����ģ��ֻ���ڳ�ʼʱ�����丳0ֵ*/

	int  nal_units;			/*NAL ����,���ж��Nal��Ԫ��
							�ͽ���NAL��Ԫ��data�е���ʼ��ַ�ͳ���
							�ŵ�dataǰ�����ֽ���*/
	int  data_len;  // ����ǰ׺�����ݵ��ܳ���
	char data[1];
};


/*RTP header*/
typedef struct
{
	/*byte 0*/
	unsigned char csrc_len:4;        //expect 0
	unsigned char extension:1;		//expect 0 
	unsigned char padding:1;		//expect 0
	unsigned char version:2;		//expert 2
	/*byte 1*/
	unsigned char payload:7;
	unsigned char marker:1;
	/*byte 2-3*/
	unsigned short seq_no;
	/*byte 4-7*/
	unsigned long timestamp;
	/*byte 8-11*/
	unsigned long ssrc;
}__attribute__ ((packed))RTP_FIXED_HEADER_SIPD;

#define UDP_PACKET_SIZE (2*1024)

typedef struct
{
	char buf[UDP_PACKET_SIZE];
	int len;
}PACKET_NODE_T;


#define MAX_SIPD_DEVICE_ID_LEN					(20)
struct SIPD_DATA_FD
{
	int cid;
	int did;
	int video_fd;
	int audio_fd;
	unsigned long local_ip;
	unsigned short local_Vport;
	unsigned short local_Aport;
	unsigned long remote_ip;
	unsigned short remote_Vport;
	unsigned short remote_Aport;
	unsigned long start_time;
	unsigned long stop_time;
	char  device_id[MAX_SIPD_DEVICE_ID_LEN+1]; 
};



/*------------------------------------function interface---------------------------------*/
extern char *GB_Get_LocalIP();


#endif



//#endif









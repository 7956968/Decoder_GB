#ifndef _GB_PROTOCOLLAYER_H_
#define _GB_PROTOCOLLAYER_H_

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <sys/socket.h>
#include <ctype.h>
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <poll.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <netinet/ip.h>
#include <dirent.h>

#include <osipparser2/osip_parser.h>
#include <osipparser2/sdp_message.h>
#include <osip_headers.h>
#include <osip_mt.h>
#include <osip2/osip.h>
#include <osip2/osip_dialog.h>
#include <osip2/osip_mt.h>
#include <osip2/osip_condv.h>
#include <osipparser2/osip_md5.h>

#include <mxml.h>

#include "milenage.h"
#include "rijndael.h"

#define GB_MANSCDP_XML 	("Application/MANSCDP+xml")
#define GB_MANSCDP_SDP	("APPLICATION/SDP")
#define GB_UA_STRING 		("STAR-NET SIP UAS V1.0")



/*************************      GB_CMD_DEF    ************************************************/


typedef enum 
{
	gb_CommandType_KeepAlive = 0,
	gb_CommandType_DeviceStatus,
	gb_CommandType_Catalog,
	gb_CommandType_DeviceInfo,
	gb_CommandType_RecordInfo,
	gb_CommandType_Alarm,
	gb_CommandType_ConfigDownload,
	gb_CommandType_PersetQuery,

	gb_CommandType_DeviceControl,
	
	gb_CommandType_DeviceConfig,
	gb_CommandType_MediaStatus,

	gb_CommandType_NUM,
}gb_CommandType_enum;


/*************************      END GB_CMD_DEF    *******************************************/



#define GB_DEVICEID_LEN				20
#define GB_MAX_PLAYLOAD_BUF		(4*1024)
#define GB_MAX_FILEPATH_LEN			(256)
#define GB_STR_LEN				16
#define GB_DATETIME_STR_LEN				32
#define GB_MAX_STR_LEN			(256)
#define GB_NAME_STR_LEN				64
#define GB_EXTERN_INFO_BUF		(1024)
#define GB_ROI_NUM		(16)
#define GB_PRESET_NUM		(255)


#define GB_SIZEOF_STRUCT(NAME)  ((sizeof(NAME))/(sizeof(NAME[0])))

#define NO_SHOW_ITEM		(-1)


enum
{
	statusType_ON,
	statusType_OFF,
};

enum
{
	resultType_OK,
	resultType_ERROR,
};

enum
{
	ONLINE,
	OFFLINE,
};

enum
{
	DutyStatus_ONDUTY,
	DutyStatus_OFFDUTY,
	DutyStatus_ALARM,
};

enum
{
	Type_time,
	Type_alarm,
	Type_manual,
	Type_all,
};

enum
{
	recordType_Record,
	recordType_StopRecord,
};

enum
{
	guardType_SetGuard,
	guardType_ResetGuard,
};


enum
{
	Msgtype_Control = 1,
	Msgtype_Query,
	Msgtype_Notify,
	Msgtype_Response,
};

enum CodeStruct_enum
{
	Code_Query_Req = 1,
	Code_Control_Req,
	
	Code_MAX_NUM,
};


typedef struct
{
	int Msgtype;
	int Cmdtype;
}BaseInfo;


typedef struct 
{
	char deviceID[GB_DEVICEID_LEN+1];
	
}deviceIDType;


typedef struct 
{
	int SN;
	deviceIDType DeviceID;
	int resultType;  
	int Num;  // �����豸����
	deviceIDType *errorDeviceID;
}gb_Keepalive_Struct;


/**************************      Struct_DEF    ************************************/

typedef struct 
{
	int Cmdtype;
	int SN;
	deviceIDType DeviceID;
}gb_BaseInfo_Query;



// �豸״̬
typedef struct
{
	deviceIDType DeviceID; 
	int DutyStatus; // �����豸״̬����ѡ��
}gb_AlarmDeviceInfo;

typedef struct 
{
	gb_BaseInfo_Query BaseInfo;
	int Result;  // ��ѯ�����־����ѡ��
	int Online; // �Ƿ����ߣ���ѡ��
	int Status;  // �Ƿ�������������ѡ��
	char Reason[GB_MAX_STR_LEN+1];  // ����������ԭ�򣨿�ѡ��
	int Encode;  // �Ƿ���루��ѡ��
	int Decode;  // �Ƿ����,  �ο�������Ӹ��ֶΣ���������ʹ��
	int Record;  // �Ƿ�¼�񣨿�ѡ��
	char DeviceTime[GB_DATETIME_STR_LEN];  // �豸ʱ������ڣ���ѡ�� 
	int Num; // ��ʾ�б����������ѡ��
	gb_AlarmDeviceInfo *AlarmDeviceList; // �����豸״̬�б���ѡ��
	char *Info; // ��չ��Ϣ���ɶ���
}gb_Query_DeviceStatus_Rsp;




// Ŀ¼��Ϣ
typedef struct 
{
	gb_BaseInfo_Query Query;
	char StartTime[GB_DATETIME_STR_LEN]; // �����豸����ʼʱ�䣨��ѡ���ձ�ʾ����
	char EndTime[GB_DATETIME_STR_LEN]; // �����豸����ֹʱ�䣨��ѡ���ձ�ʾ����ǰʱ��
}gb_Catalog_Query;

typedef struct
{
	deviceIDType DeviceID;
	char Name[GB_NAME_STR_LEN+1]; // �豸/����/ϵͳ���ƣ���ѡ��
	char Manufacturer[GB_NAME_STR_LEN+1]; // ��Ϊ�豸ʱ���豸���̣���ѡ��
	char Model[GB_NAME_STR_LEN+1]; // ��Ϊ�豸ʱ���豸�ͺţ���ѡ��
	char Owner[GB_NAME_STR_LEN+1]; // ��Ϊ�豸ʱ���豸��������ѡ��
	char CivilCode[GB_STR_LEN+1]; // �������򣨱�ѡ��
	double Block; // ��������ѡ��
	char Address[GB_MAX_STR_LEN+1]; // ��Ϊ�豸ʱ����װ��ַ����ѡ��
	int Parental; // ��Ϊ�豸ʱ���Ƿ������豸����ѡ��1�У�0û��
	char ParentID[GB_DEVICEID_LEN+1]; // ���豸/����/ϵͳID����ѡ���и��豸��Ҫ��д��
	int SafetyWay; // ���ȫģʽ����ѡ��ȱʡΪ0��0�������ã�2��S/MIMEǩ����ʽ��3��S/MIME����ǩ��ͬʱ���÷�ʽ��4������ժҪ��ʽ
	int RegisterWay; // ע����ʽ����ѡ��ȱʡΪ1��1������sip3261��׼����֤ע��ģʽ��2�����ڿ����˫����֤ע��ģʽ��3����������֤���˫����֤ע��ģʽ
	char CertNum[GB_MAX_STR_LEN+1]; // ֤�����кţ���֤����豸��ѡ��
	int Certifiable; // ֤����Ч��ʶ����֤����豸��ѡ��ȱʡΪ0��֤����Ч��ʶ��0����Ч  1����Ч
	int ErrCode; // ��Чԭ���루��֤����֤����Ч���豸��ѡ��
	char EndTime[GB_DATETIME_STR_LEN]; // ֤����ֹ��Ч�ڣ���֤����豸��ѡ��
	int Secrecy; // �������ԣ���ѡ��ȱʡΪ0��0�������ܣ�1������
	char IPAddress[GB_STR_LEN]; // �豸/����/ϵͳIP ��ַ����ѡ��
	int Port; // �豸/����/ϵͳ�˿ڣ���ѡ��
	char Password[GB_NAME_STR_LEN]; // �豸�����ѡ��
	int Status; // �豸״̬����ѡ��
	double Longitude; // ���ȣ���ѡ��
	double Latitude; // γ�ȣ���ѡ��
}gb_itemType;

typedef struct 
{
	int PTZType;  // �����������չ����ʶ��������ͣ�1-�����2-����3-�̶�ǹ����4-ң��ǹ������Ŀ¼��Ϊ�����ʱ��ѡ��
	int PositionType; // �����λ��������չ��1-ʡ�ʼ��վ��2-�������ء�3-��վ��ͷ��4-���Ĺ㳡��5-�������ݡ�6-��ҵ���ġ�7-�ڽ̳�����8-У԰�ܱߡ�9-�ΰ���������10-��ͨ���ߡ���Ŀ¼��Ϊ�����ʱ��ѡ��
	int RoomType; // �������װλ�����⡢�������ԡ�1-���⡢2-���ڡ���Ŀ¼��Ϊ�����ʱ��ѡ��ȱʡΪ1��
	int UseType; // �������;���ԡ�1-�ΰ���2-��ͨ��3-�ص㡣��Ŀ¼��Ϊ�����ʱ��ѡ��
	int SupplyLightType; // ������������ԡ�1-�޲��⡢2-���ⲹ�⡢3-�׹ⲹ�⡣��Ŀ¼��Ϊ�����ʱ��ѡ��ȱʡΪ1��
	int DirectionType; // ��������ӷ�λ���ԡ�1-����2-����3-�ϡ�4-����5-���ϡ�6-������7-���ϡ�8-��������Ŀ¼��Ϊ�����ʱ��Ϊ�̶�����������ÿ���λ�����ʱ��ѡ��
	char Resolution[GB_NAME_STR_LEN*2]; // �����֧�ֵķֱ��ʣ����ж���ֱ���ֵ������ȡֵ���ԡ�/���ָ����ֱ���ȡֵ�μ����긽¼F��SDP f�ֶι涨����Ŀ¼��Ϊ�����ʱ��ѡ��
	deviceIDType BusinessGroupID; // ������֯������ҵ�����ID��ҵ���������ض���ҵ�������ƶ���һ��ҵ��������һ���ض���������֯��
}gb_Query_Catalog_info;


typedef struct 
{
	gb_BaseInfo_Query BaseInfo;
	int SumNum;  // ��ѯ�����������ѡ��	
	int Num;  //num��ʾĿ¼�����
	gb_itemType *DeviceList; // �豸Ŀ¼���б�
	gb_Query_Catalog_info *info; // ��չ��Ϣ���ɶ���  
}gb_Query_Catalog_Rsp;




// �豸��Ϣ
typedef struct 
{
	gb_BaseInfo_Query BaseInfo;
	char DeviceName[GB_NAME_STR_LEN+1]; // Ŀ���豸/����/ϵͳ�����ƣ���ѡ��
	int Result;  // ��ѯ�����־����ѡ��
	char Manufacturer[GB_NAME_STR_LEN+1]; // �豸�����̣���ѡ��
	char Model[GB_NAME_STR_LEN+1]; // �豸�ͺţ���ѡ�� 
	char Firmware[GB_NAME_STR_LEN+1];  // �豸�̼��汾����ѡ�� 
	int Channel;  // ��Ƶ����ͨ��������ѡ��
	char DeviceType[GB_NAME_STR_LEN+1];  
	int MaxCamera;  
	int MaxAlarm; 
	int MaxOut;  //  �ο�������Ӹ��ֶΣ���������ʹ��
	char *Info; // ��չ��Ϣ���ɶ���
}gb_Query_DeviceInfo_Rsp;





// �ļ�Ŀ¼����
typedef struct 
{
	gb_BaseInfo_Query Query;
	char StartTime[GB_DATETIME_STR_LEN]; // ¼����ʼʱ�䣨��ѡ���ձ�ʾ����
	char EndTime[GB_DATETIME_STR_LEN]; // ����¼����ֹʱ�䣨��ѡ���ձ�ʾ����ǰʱ��
	char FilePath[GB_MAX_FILEPATH_LEN+1]; // �ļ�·��������ѡ��
	char Address[GB_MAX_FILEPATH_LEN+1]; // ¼���ַ����ѡ  ֧�ֲ���ȫ��ѯ��
	int Secrecy; // �������ԣ���ѡ��ȱʡΪ0��0�������ܣ�1������
	int Type; // ¼��������ͣ���ѡ��time ��alarm��manual��all
	char RecorderID[GB_DEVICEID_LEN+1]; // ¼�񴥷���ID����ѡ��
	int IndistinctQuery; // ¼��ģ����ѯ���ԣ���ѡ��ȱʡΪ0��0��������ģ����ѯ����ʱ����SIP��Ϣ��Toͷ��URI�е�IDֵȷ����ѯ¼��λ�ã���IDֵΪ����ϵͳID�����������ʷ��¼��������Ϊǰ���豸ID�����ǰ���豸��ʷ��¼������1������ģ����ѯ����ʱ�豸������Ӧͬʱ�������ļ�����ǰ�˼����������ͳһ���ء�
}gb_RecordInfo_Query;

typedef struct
{
	deviceIDType DeviceID;
	char Name[GB_NAME_STR_LEN+1]; // �豸/�������ƣ���ѡ��
	char FilePath[GB_MAX_FILEPATH_LEN+1]; // �ļ�·��������ѡ��
	char Address[GB_MAX_STR_LEN+1]; // ¼���ַ����ѡ��
	char StartTime[GB_DATETIME_STR_LEN]; // ¼��ʼʱ�䣨��ѡ��
	char EndTime[GB_DATETIME_STR_LEN]; // ¼�����ʱ�䣨��ѡ��
	int Secrecy; // �������ԣ���ѡ��ȱʡΪ0��0�������ܣ�1������
	int Type; // ¼��������ͣ���ѡ��time ��alarm��manual��all
	char RecorderID[GB_DEVICEID_LEN+1]; // ¼�񴥷���ID����ѡ��
}gb_itemFileType;

typedef struct 
{
	gb_BaseInfo_Query BaseInfo;
	char Name[GB_NAME_STR_LEN+1]; // �豸/�������ƣ���ѡ��
	int SumNum;  // ��ѯ�����������ѡ��
	int Num; // Num��ʾĿ¼�����
	gb_itemFileType *RecordList; // �ļ�Ŀ¼���б�
	char *info; // ��չ��Ϣ���ɶ���
}gb_Query_RecordInfo_Rsp;






// ����
typedef struct 
{
	gb_BaseInfo_Query Query;
	int StartAlarmPriority; // ������ʼ���𣨿�ѡ����0Ϊȫ����1Ϊһ�����飬2Ϊ�������飬3Ϊ�������飬4Ϊ�ļ�����
	int EndAlarmPriority; // ������ֹ���𣨿�ѡ����0Ϊȫ����1Ϊһ�����飬2Ϊ�������飬3Ϊ�������飬4Ϊ�ļ�����
	char AlarmMethod[GB_STR_LEN+1]; // ������ʽ��������ѡ����ȡֵ0Ϊȫ����1Ϊ�绰������2Ϊ�豸������3Ϊ���ű�����4ΪGPS������5Ϊ��Ƶ������6Ϊ�豸���ϱ�����7��������������Ϊֱ�������12Ϊ�绰�������豸����
	char StartAlarmTime[GB_DATETIME_STR_LEN]; // ����������ʼʱ�䣨��ѡ��
	char EndAlarmTime[GB_DATETIME_STR_LEN]; // ����������ֹʱ�䣨��ѡ��
}gb_Alarm_Query;

typedef struct 
{
	gb_BaseInfo_Query BaseInfo;
	int AlarmPriority; // �������𣨱�ѡ����1Ϊһ�����飬2Ϊ�������飬3Ϊ�������飬4Ϊ�ļ�����
	int AlarmMethod; // ������ʽ����ѡ����ȡֵ1Ϊ�绰������2Ϊ�豸������3Ϊ���ű�����4ΪGPS������5Ϊ��Ƶ������6Ϊ�豸���ϱ�����7��������
	char AlarmTime[GB_DATETIME_STR_LEN]; // ����ʱ�䣨��ѡ��
	char AlarmDescription[GB_MAX_STR_LEN]; // ����������������ѡ��
	double Longitude; // ���ȣ���ѡ��
	double Latitude; // γ�ȣ���ѡ��
}gb_Alarm_Notify;



// �豸����
#define GB_ConfigType_BasicParam			(0x01 << 0)
#define GB_ConfigType_VideoParamOpt		(0x01 << 1)
#define GB_ConfigType_VideoParamConfig		(0x01 << 2)
#define GB_ConfigType_AudioParamOpt		(0x01 << 3)
#define GB_ConfigType_AudioParamConfig		(0x01 << 4)
#define GB_ConfigType_SVACEncodeConfig	(0x01 << 5)
#define GB_ConfigType_SVACDecodeConfig	(0x01 << 6)

#define GB_IS_ConfigType_BasicParam(Type) 	(Type & GB_ConfigType_BasicParam)
#define GB_IS_ConfigType_VideoParamOpt(Type) 	(Type & GB_ConfigType_VideoParamOpt)
#define GB_IS_ConfigType_VideoParamConfig(Type) 	(Type & GB_ConfigType_VideoParamConfig
#define GB_IS_ConfigType_AudioParamOpt(Type) 	(Type & GB_ConfigType_AudioParamOpt)
#define GB_IS_ConfigType_AudioParamConfig(Type) 	(Type & GB_ConfigType_AudioParamConfig)
#define GB_IS_ConfigType_SVACEncodeConfig(Type) 	(Type & GB_ConfigType_SVACEncodeConfig)
#define GB_IS_ConfigType_SVACDecodeConfig(Type) 	(Type & GB_ConfigType_SVACDecodeConfig)

typedef struct 
{
	gb_BaseInfo_Query Query;
	int ConfigType; // ��ѯ���ò������ͣ���ѡ�����ɲ�ѯ���������Ͱ��������������ã�BasicParam����Ƶ�������÷�Χ��VideoParamOpt����Ƶ������ǰ���ã�VideoParamConfig����Ƶ�������÷�Χ��AudioParamOpt����Ƶ������ǰ���ã�AudioParamConfig��SVAC�������ã�SVACEncodeConfig��SVAC �������ã�SVACDecodeConfig����ͬʱ��ѯ����������ͣ��������ԡ�/���ָ����ɷ������ѯSNֵ��ͬ�Ķ����Ӧ��ÿ����Ӧ��Ӧһ���������͡�
}gb_ConfigDownload_Query;

typedef struct
{
	char Name[GB_NAME_STR_LEN]; // �豸���ƣ���ѡ��
	char DeviceID[GB_DEVICEID_LEN+1]; // �豸ID����ѡ��
	char SIPServerID[GB_DEVICEID_LEN+1]; // SIP������ID����ѡ��
	char SIPServerIP[GB_STR_LEN]; // SIP������IP����ѡ��
	int SIPServerPort; // SIP�������˿ڣ���ѡ��
	char DomainName[GB_DEVICEID_LEN+1]; // SIP��������������ѡ��
	int Expiration; // ע�����ʱ�䣨��ѡ��
	char Password[GB_DEVICEID_LEN+1]; // ע������ѡ��
	int HeartBeatInterval; // �������ʱ�䣨��ѡ��
	int HeartBeatCount; // ������ʱ��������ѡ��	
}gb_ConfigDownload_BasicParam;

typedef struct
{
	char VideoFormatOpt[GB_MAX_STR_LEN]; // ��Ƶ�����ʽ��ѡ��Χ����ѡ��
	char ResolutionOpt[GB_MAX_STR_LEN]; // �ֱ��ʿ�ѡ��Χ����ѡ��
	char FrameRateOpt[GB_MAX_STR_LEN]; // ֡�ʿ�ѡ��Χ����ѡ��
	char BitRateTypeOpt[GB_MAX_STR_LEN]; // �������ͷ�Χ����ѡ��
	char VideoBitRateOpt[GB_MAX_STR_LEN]; //��Ƶ���ʷ�Χ����ѡ��
	char DownloadSpeedOpt[GB_MAX_STR_LEN]; // ��Ƶ�����ٶȿ�ѡ��Χ����ѡ��
}gb_ConfigDownload_VideoParamOpt;

typedef struct 
{
	char StreamName[GB_NAME_STR_LEN]; // ������(��ѡ) ���һ���� Stream1,�ڶ����� Stream2 
	char VideoFormat[GB_NAME_STR_LEN]; // ��Ƶ�����ʽ��ǰ����ֵ(��ѡ)
	char Resolution[GB_NAME_STR_LEN]; // �ֱ��ʵ�ǰ����ֵ(��ѡ)
	char FrameRate[GB_NAME_STR_LEN]; // ֡�ʵ�ǰ����ֵ(��ѡ)
	char BitRateType[GB_NAME_STR_LEN]; // ������������ֵ(��ѡ)
	char VideoBitRate[GB_NAME_STR_LEN]; // ��Ƶ��������ֵ(��ѡ)
}gb_VideoParamAttributeType;

typedef struct
{
	int Num;
	gb_VideoParamAttributeType *Item;
}gb_ConfigDownload_VideoParamConfig;

typedef struct
{
	char AudioFormatOpt[GB_MAX_STR_LEN]; // ��Ƶ�����ʽ��ѡ��Χ����ѡ��
	char AudioBitRateOpt[GB_MAX_STR_LEN]; // ��Ƶ���ʿ�ѡ��Χ����ѡ��
	char SamplingRateOpt[GB_MAX_STR_LEN]; // �����ʿ�ѡ��Χ����ѡ��
}gb_ConfigDownload_AudioParamOpt;


typedef struct 
{
	char StreamName[GB_NAME_STR_LEN]; // ������(��ѡ) ���һ���� Stream1,�ڶ����� Stream2 
	char AudioFormat[GB_NAME_STR_LEN]; // ��Ƶ�����ʽ��ǰ����ֵ(��ѡ)
	char AudioBitRate[GB_NAME_STR_LEN]; // ��Ƶ���ʵ�ǰ����ֵ(��ѡ)
	char SamplingRate[GB_NAME_STR_LEN]; // �����ʵ�ǰ����ֵ(��ѡ)
}gb_AudioParamAttributeType;

typedef struct
{
	int Num;
	gb_AudioParamAttributeType *Item;
}gb_ConfigDownload_AudioParamConfig;

typedef struct
{
	int ROISeq; // ����Ȥ�����ţ�ȡֵ��Χ1-16����ѡ��
	int TopLeft; // ����Ȥ�������Ͻ����꣬ȡֵ��Χ0-19683����ѡ��
	int BottomRight; // ����Ȥ�������½����꣬ȡֵ��Χ0-19683����ѡ��
	int ROIQP; // ROI������������ȼ���ȡֵ0��һ�㣬1���Ϻã�2���ã�3���ܺã���ѡ��
}gb_ROI;

typedef struct
{
	int ROIFlag; // ����Ȥ���򿪹أ�ȡֵ0���رգ�1���򿪣���ѡ��
	int ROINumber; // ����Ȥ����������ȡֵ��Χ0-16����ѡ��
	gb_ROI ROI[GB_ROI_NUM]; // ����Ȥ���򣨱�ѡ��
	int BackGroundQP; // ����������������ȼ���ȡֵ0��һ�㣬1���Ϻã�2���ã�3���ܺã���ѡ��
	int BackGroundSkipFlag; // �����������أ�ȡֵ0���رգ�1���򿪣���ѡ��
}gb_SVACEncodeConfig_ROIParam;

typedef struct
{
	int SVCFlag; // SVC���أ�ȡֵ0���رգ�1���򿪣���ѡ��
	int SVCSTMMode; // �����ϴ�ģʽ��ȡֵ0�������������������䷽ʽ��1��������+1����ǿ��������ʽ��2��������+2����ǿ��������ʽ��3��������+3����ǿ��������ʽ������ѡ��
	int SVCSpaceDomainMode; // ������뷽ʽ��ȡֵ0����ʹ�ã�1��1����ǿ��1����ǿ�㣩��2��2����ǿ��2����ǿ�㣩��3��3����ǿ��3����ǿ�㣩����ѡ��
	int SVCTimeDomainMode; // ʱ����뷽ʽ��ȡֵ0����ʹ�ã�1��1 ����ǿ��2��2����ǿ��3��3����ǿ����ѡ��
}gb_SVACEncodeConfig_SVCParam;

typedef struct
{
	int TimeFlag; //����ʱ����Ϣ���أ�ȡֵ0���رգ�1���򿪣���ѡ��
	int EventFlag; // ����¼���Ϣ���أ�ȡֵ0���رգ�1���򿪣���ѡ��
	int AlertFlag; // ������Ϣ���أ�ȡֵ0���رգ�1���򿪣���ѡ��
}gb_SVACEncodeConfig_SurveillanceParam;

typedef struct
{
	int EncryptionFlag; //���ܿ��أ�ȡֵ0���رգ�1���򿪣���ѡ��
	int AuthenticationFlag; // ��֤���أ�ȡֵ0���رգ�1���򿪣���ѡ��
}gb_SVACEncodeConfig_EncryptParam;

typedef struct
{
	int AudioRecognitionFlag; //����ʶ�������������أ�ȡֵ0���رգ�1����
}gb_SVACEncodeConfig_AudioParam;

typedef struct
{
	int ROIParamFlag; // 0-����, 1-����
	gb_SVACEncodeConfig_ROIParam ROIParam; // ����Ȥ�����������ѡ��
	int SVCParamFlag; // 0-����, 1-����
	gb_SVACEncodeConfig_SVCParam SVCParam; // SVC��������ѡ��
	int SurveillanceParamFlag; // 0-����, 1-����
	gb_SVACEncodeConfig_SurveillanceParam SurveillanceParam; // ���ר����Ϣ��������ѡ��
	int EncryptParamFlag; // 0-����, 1-����
	gb_SVACEncodeConfig_EncryptParam EncryptParam; // ��������֤��������ѡ��
	int AudioParamFlag; // 0-����, 1-����
	gb_SVACEncodeConfig_AudioParam AudioParam; // ��Ƶ��������ѡ��
}gb_ConfigDownload_SVACEncodeConfig;

typedef struct
{
	int SVCSTMMode; // �����ϴ�ģʽ��ȡֵ0�������������������䷽ʽ��1��������+1����ǿ��������ʽ��2��������+2����ǿ��������ʽ��3��������+3����ǿ��������ʽ��
}gb_SVACDecodeConfig_SVCParam;

typedef struct
{
	int TimeShowFlag; // ����ʱ����Ϣ��ʾ���أ�ȡֵ0���رգ�1���򿪣���ѡ��
	int EventShowFlag; // ����¼���Ϣ��ʾ���أ�ȡֵ0���رգ�1���򿪣���ѡ��
	int AlerShowtFlag; // ������Ϣ��ʾ���أ�ȡֵ0���رգ�1���򿪣���ѡ��
}gb_SVACDecodeConfig_SurveillanceParam;

typedef struct
{
	int SVCParamFlag; // 0-����, 1-����
	gb_SVACDecodeConfig_SVCParam SVCParam; // SVC��������ѡ��
	int SurveillanceParamFlag; // 0-����, 1-����
	gb_SVACDecodeConfig_SurveillanceParam SurveillanceParam; // ���ר����Ϣ��������ѡ��
}gb_ConfigDownload_SVACDecodeConfig;



typedef struct 
{
	gb_BaseInfo_Query BaseInfo;
	int Result;  // ��ѯ�����־����ѡ��
	gb_ConfigDownload_BasicParam 	*BasicParam; // �����������ã���ѡ��
	gb_ConfigDownload_VideoParamOpt *VideoParamOpt; // ��Ƶ�������÷�Χ����ѡ��������ѡ�����ԡ�/���ָ�
	gb_ConfigDownload_VideoParamConfig *VideoParamConfig; // ��Ƶ������ǰ���ã���ѡ��
	gb_ConfigDownload_AudioParamOpt *AudioParamOpt; //��Ƶ�������÷�Χ����ѡ��������ѡ�����ԡ�/���ָ�
	gb_ConfigDownload_AudioParamConfig *AudioParamConfig; // ��Ƶ������ǰ���ã���ѡ��
	gb_ConfigDownload_SVACEncodeConfig *SVACEncodeConfig; // SVAC�������ã���ѡ��
	gb_ConfigDownload_SVACDecodeConfig *SVACDecodeConfig; // SVAC�������ã���ѡ��
}gb_Query_ConfigDownload_Rsp;





// �豸Ԥ��λ
typedef struct
{	
	int Flag; // 0-����, 1-����
	char PresetID[GB_STR_LEN]; // Ԥ��λ���루��ѡ��
	char PresetName[GB_NAME_STR_LEN]; // Ԥ��λ���ƣ���ѡ��
}gb_Preset_Info;

typedef struct 
{
	gb_BaseInfo_Query BaseInfo;
	int Num;  // �б������
	gb_Preset_Info PresetList[GB_PRESET_NUM]; // ��ǰ���õ�Ԥ��λ��¼����δ����Ԥ��λʱ����д
}gb_Query_PersetQuery_Rsp;





// ��ѯ����ṹ��
typedef struct 
{
	gb_BaseInfo_Query 	*DeviceStatus; // �豸״̬��ѯ����
	gb_Catalog_Query 	*Catalog; // �豸Ŀ¼��Ϣ��ѯ���� 	
	gb_BaseInfo_Query 	*DeviceInfo; // �豸��Ϣ��ѯ���� 
	gb_RecordInfo_Query 	*RecordInfo; // �ļ�Ŀ¼��������
	gb_Alarm_Query 		*Alarm; // ������ѯ
	gb_ConfigDownload_Query *ConfigDownload; // �豸���ò�ѯ
	gb_BaseInfo_Query 	*PersetQuery; // �豸Ԥ��λ��ѯ
}gb_Query_Req_Struct;




// ��������ṹ��
typedef struct 
{
	int Length; // ���Ŵ��ڳ�������ֵ����ѡ��
	int Width; // ���Ŵ��ڿ������ֵ����ѡ��
	int MidPointX; // �������ĵĺ�����������ֵ����ѡ��
	int MidPointY; // �������ĵ�������������ֵ����ѡ��
	int LengthX; // ���򳤶�����ֵ����ѡ��
	int LengthY; // ����������ֵ����ѡ��
}gb_DragZoom_CTRL;

typedef struct 
{
	gb_BaseInfo_Query BaseInfo;
	char PTZCmd[GB_STR_LEN+1]; // ���/��̨���������ѡ��������Ӧ���ϸ�¼A��A.3�еĹ涨)
	int  TeleBoot; // Զ���������������ѡ��,1-����
	int  RecordCmd; // ¼����������ѡ��
	int  GuardCmd; // ��������/���������ѡ��
	int  AlarmCmd; // ������λ�����ѡ��,1-��λ
	int DragZoomFlag;  //  0-����Ϣ��1-����Ŵ�������2-������С���������ѡ��
	gb_DragZoom_CTRL DragZoom;
}gb_DeviceControl_Req;

typedef struct 
{
	gb_BaseInfo_Query BaseInfo;
	int BasicParamFlag; //  0-����Ϣ��1-������
	gb_ConfigDownload_BasicParam BasicParam; // �����������ã���ѡ��
}gb_DeviceConfig_Req;



typedef struct
{
	gb_DeviceControl_Req *DeviceControl;
	gb_DeviceConfig_Req *DeviceConfig;
}gb_Control_Req_Struct;

typedef struct 
{
	gb_BaseInfo_Query BaseInfo;
	int Result;  // ִ�н����־����ѡ��
}gb_DeviceControl_Rsp;




typedef struct 
{
	gb_BaseInfo_Query BaseInfo;
	int NotifyType; // ֪ͨ�¼����ͣ���ѡ����ȡֵ��121����ʾ��ʷý���ļ����ͽ���
}gb_MediaStatus_Notify;

/**************************      End Struct_DEF    ********************************/




/*************************      GB_API_FUNC    ***********************************************/

char * gb_buffer_find (const char *haystack, size_t haystack_len, const char *needle);
int gb_sip_messages_parse(osip_event_t **osip_event, char *buffer, size_t buffer_len);
void gb_sip_free(osip_event_t *se);
int gb_generating_register (osip_message_t ** reg, char *transport, char *from, char *proxy, char *contact, int expires, char *localip, int port, int ncseq);
int gb_create_authorization_header (osip_www_authenticate_t * wa, const char *rquri, const char *username, const char *passwd, const char *ha1, osip_authorization_t ** auth, const char *method, const char *pCNonce, int iNonceCount);
int gb_generating_MESSAGE(osip_message_t ** reg, char *transport, char *from, char *to,char *proxy, char *localip, int port, int ncseq, void *cmd_struct, gb_CommandType_enum cmdType);
int gb_generating_NOTIFY(osip_message_t ** reg, char *transport, char *from, char *to,char *proxy, char *localip, int port, int ncseq, void *cmd_struct, gb_CommandType_enum cmdType,char *Sub_State, char *reason,int expires);
int gb_build_response_message (osip_message_t ** dest, osip_dialog_t * dialog, int status, osip_message_t * request, char *content_type, char *body, int bodylen);
int gb_build_response_Contact (osip_message_t *response, char *localip, int port, char *username);
int gb_parser_Req_XML(char *buf, int *code, void **dest);
/*************************      END GB_API_FUNC    *******************************************/


#endif

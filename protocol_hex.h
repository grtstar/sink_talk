#ifndef _PROTO_HEX_H_
#define _PROTO_HEX_H_
/*
    二进制协议通用解析�解析包含[头部][数据长度][校验和][变长数据]的二进制协议
*/

typedef enum ProtoSegType
{
    SEG_HEADER,
    SEG_VERSION,
    SEG_VARDATE_LEN,
    SEG_VARDATA,
    SEG_CHECKSUM,
    SEG_CMD,
    SEG_CUSTOM
}ProtoSegType;

typedef enum ProtoEndian
{
    LITTLE_ENDIAN,
    BIG_ENDIAN
}ProtoEndian;

typedef struct ProtoSeg
{
    ProtoSegType type;
    int pos;
    int len;
}ProtoSeg;

typedef struct ProtoHead
{
    uint8_t header[8];  
    int     len;
}ProtoHead;

typedef struct ProtoHex
{
    ProtoHead    header;           
    int             byteOrder;     
    ProtoSeg* segs;             
    int             segCount;     
    uint8_t* buff;         
    int             buffLen;       
    uint8_t* sendBuf;
    int            sendBufLen;


    int             currIndex;      
    int             state;         
    int             varDataLenPos;  
    int             varDataLenLen;
    int             varDataPos;
    int             varDataLen;
    int             fixedDataLen;   
    int             totalLen;       

    int            totalSendLen;
}ProtoHex;

/*
   二进制协议解析器初始化函�

   @ph         解析器结构体指针
*/
void ProtoHexInit(ProtoHex* ph);

/*
   获取某个数据段的�

   @ph         解析器结构体指针
   @type       数据段类�
   @ret        根据大小端返回对应的�只支持小于等�字节的数�
*/
int ProtoHexGetSegValue(ProtoHex* ph, ProtoSegType type);

/*
    获取变长数据段指�
*/
uint8_t* ProtoHexGetVarData(ProtoHex* ph);

/*
    获取变长数据段长�
*/
int ProtoHexGetVarLen(ProtoHex* ph);

/*
   二进制协议解析器解析函数

   @ph         解析器结构体指针
   @d          输入数据缓冲区指�
   @len        输入数据的长�

   @ret        解析完成返回 true, 还未解析出完整数据返�false
               本函数不校验协议校验�
*/
bool ProtoHexParse(ProtoHex* ph, const uint8_t* d, int len);

void ProtoHexPackSegValue(ProtoHex* ph, ProtoSegType type, int value);
void ProtoHexPackVarData(ProtoHex* ph, uint8_t* d, int len);
#endif

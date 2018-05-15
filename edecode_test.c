#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <termios.h>
#include <string.h>
#include <time.h>

#define MAX_TX_STREAM_LEN			26
#define MAX_TX_CHANNEL_BUF_LEN		22
#define MAX_TX_CHANNEL				16

//#define DEBUG_NK

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;

enum eChanTxProtocol {
	AILERON	= 0,
	ELEVATOR = 1,
	THROTTLE = 2,
	RUDDER = 3,
	FLIP = 4,
	PICTURE = 6,
	VIDEO = 7,
	HEADLESS = 8,
};

enum eSubProtocol {
	SUB_PROTOCOL_SYMAX = 10,
};

enum eSubProtocolType {
	SUB_PROTOCOL_TYPE_SYMAX = 0,
	SUB_PROTOCOL_TYPE_SYMAX5C = 1,
};


enum eBind {
	BIND_OFF = 0,
	BIND_ON = 1,
};
struct TxStream {
	uint8_t ucHead;
	uint8_t ucSubProtocol;
	uint8_t	ucType;
	uint8_t	ucOptionProtocol;
	uint8_t ucChan[MAX_TX_CHANNEL_BUF_LEN];	// 22(x unsighed char) => 16 (x unsigned short)
};

#define TX_STREAM_SIZE	sizeof(struct TxStream)

union TxStreamData {
	struct TxStream stTS;
	uint8_t ucByte[TX_STREAM_SIZE];
};

#define TS_HEAD			stTS.ucHead
#define TS_SPROTOCOL	stTS.ucSubProtocol
#define TS_TYPE			stTS.ucType
#define TS_OPROTOCOL 	stTS.ucOptionProtocol
#define TS_CH(x)		stTS.ucChan[x]

void Clear_TxStream_Channel_Data(unsigned int *punChannelData)
{
	int i;

	for(i = 0; i < MAX_TX_CHANNEL; i++) {
		punChannelData[i] = 0x0;
	}
}

void Clear_TxStream_Data(unsigned char *pucTxStreamData)
{
	int i;

	for(i = 0; i < MAX_TX_STREAM_LEN; i++) {
		pucTxStreamData[i] = 0x0;
	}
}

void Encode_TxStream_Channel_Data(uint8_t *pucTxStreamChannel, unsigned int *punChannelData) 
{
	uint8_t *p=pucTxStreamChannel;
	uint8_t dec=-3;
	uint8_t i;

	for(i=0;i < MAX_TX_CHANNEL;i++)
	{
		dec+=3;
		if(dec>=8)
		{
			dec-=8;
			printf("p(%p) : 0x%08x\n", p, *(unsigned int *)p);
			p++;
		}

		printf("chan[%02d] : 0x%08x, dec : %02d, ", i, punChannelData[i] & 0x7FF, dec);

		printf("p(%p) : 0x%08x ", p, *(unsigned int *)p);

		*(uint32_t *)p |= ((punChannelData[i] & 0x7FF) << dec);	// Value ange 0..2047, 0=-125%, 2047=+125%

		printf("==>> p(%p) : 0x%08x)\n", p, *(unsigned int *)p);

		p++;
	}
}

void Decode_TxStream_Channel_Buffer(uint8_t *pucTxStreamChannel, unsigned int *punChannelData) 	// src, dest
{
	uint8_t *p=pucTxStreamChannel;
	uint8_t dec=-3;
	uint8_t i;
	unsigned int temp;

	for(i=0;i < MAX_TX_CHANNEL;i++)
	{
		dec+=3;
		if(dec>=8)
		{
			dec-=8;
			p++;
		}

		temp = ((*((uint32_t *)p))>>dec) & 0x7FF;

		printf("p : %p, %u(0x%08x)\n", p, *(unsigned int *)p, *(unsigned int *)p);

		punChannelData[i] = temp & 0xFFFF;
		p++;
	}
}

void Print_TxStream_Decoded_Channel_Data(union TxStreamData *pstTSB, unsigned int *punChannelData)
{
	int i;

	printf("Decoded_Stream_Data : 0x%02x 0x%02x 0x%02x 0x%02x", 
					pstTSB->TS_HEAD, 
					pstTSB->TS_SPROTOCOL, 
					pstTSB->TS_TYPE, 
					pstTSB->TS_OPROTOCOL);

	for( i = 0; i < MAX_TX_CHANNEL; i++) {
		printf(" 0x%04x", punChannelData[i] & 0xFFFF);
	}
	printf("\n");	// \n => 0x0A, \ => 0x0D
}

void Print_TxStream_Data(uint8_t *pucTxStreamData) 
{
	int i, j;

	printf("TxStream Data : ");
	for( i = 0; i < MAX_TX_STREAM_LEN; i++) {	//	26
		printf("0x%02x ", pucTxStreamData[i]);
	}
	printf("\n");

#if 0
	for( i = 0; i < MAX_TX_CHANNEL_BUF_LEN; i++) {

		for( j = 0; j < (sizeof(uint8_t) * 8); j++) {

			if( (unsigned char)pucChannelBuffer[i] & (1 << j) ) {
				printf("1");
			} else {
				printf("0");
			}
		}
		printf("  ");
	}
	printf("\n");
#endif
}


int main(int argc, char *argv[]) 
{
	int nRet;
	int i, j, k, count;

	union TxStreamData stTxStreamBuffer;

	union TxStreamData stTxStreamBufferDest;

	union TxStreamData *pstTSB;
	unsigned int unChannelData[MAX_TX_CHANNEL] = {             // 16
				0x0400,  0x0400,  0x00CC,  0x0400,
				0x00CC,  0x0400,  0x00CC,  0x00CC,
				0x00CC,  0x0400,  0x0400,  0x0400,
				0x0400,  0x0400,  0x0400,  0x0400
	};

	printf("sizeof(unsigned int)   : %lu\n", sizeof(int));
	printf("sizeof(unsigned int *) : %lu\n", sizeof(int *));

	pstTSB = &stTxStreamBuffer;

	pstTSB->TS_HEAD = 0x55;
	pstTSB->TS_SPROTOCOL = 0x0A;
	pstTSB->TS_TYPE = 0x00;
	pstTSB->TS_OPROTOCOL = 0x00;
	pstTSB->TS_CH(0) = 0x00;
	pstTSB->TS_CH(1) = 0x04;
	pstTSB->TS_CH(2) = 0x20;
	pstTSB->TS_CH(3) = 0x33;
	pstTSB->TS_CH(4) = 0x00;
	pstTSB->TS_CH(5) = 0xC8;
	pstTSB->TS_CH(6) = 0x0C;
	pstTSB->TS_CH(7) = 0x00;
	pstTSB->TS_CH(8) = 0x32;
	pstTSB->TS_CH(9) = 0x83;
	pstTSB->TS_CH(10) = 0x19;
	pstTSB->TS_CH(11) = 0xCC;
	pstTSB->TS_CH(12) = 0x00;
	pstTSB->TS_CH(13) = 0x20;
	pstTSB->TS_CH(14) = 0x00;
	pstTSB->TS_CH(15) = 0x01;
	pstTSB->TS_CH(16) = 0x08;
	pstTSB->TS_CH(17) = 0x40;
	pstTSB->TS_CH(18) = 0x00;
	pstTSB->TS_CH(19) = 0x02;
	pstTSB->TS_CH(20) = 0x10;
	pstTSB->TS_CH(21) = 0x80;

	Clear_TxStream_Data(stTxStreamBufferDest.stTS.ucChan);
	Encode_TxStream_Channel_Data(stTxStreamBufferDest.stTS.ucChan, unChannelData);
	{
		stTxStreamBufferDest.TS_HEAD = 0x55;
		stTxStreamBufferDest.TS_SPROTOCOL = 0x0A;
		stTxStreamBufferDest.TS_TYPE = 0x00;
		stTxStreamBufferDest.TS_OPROTOCOL = 0x00;
	}
	Print_TxStream_Data(stTxStreamBufferDest.ucByte);

	Clear_TxStream_Channel_Data(unChannelData);

	Decode_TxStream_Channel_Buffer((uint8_t *)&pstTSB->TS_CH(0), unChannelData);
	Print_TxStream_Data(pstTSB->ucByte);
	Print_TxStream_Decoded_Channel_Data(pstTSB, unChannelData);
}

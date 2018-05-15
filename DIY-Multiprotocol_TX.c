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

void Fill_TxStream_Channel_Data(uint8_t *pucTxStreamChannel, uint16_t *pusChannelData) 
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
			p++;
		}

		*(uint32_t *)p |= ((pusChannelData[i] & 0x7FF) << dec);	// Value ange 0..2047, 0=-125%, 2047=+125%
		p++;
	}
}

void Decode_TxStream_Channel_Buffer(uint8_t *pucTxStreamChannel, uint16_t *pusChannelData) 
{
	uint8_t *p=pucTxStreamChannel;
	uint8_t dec=-3;
	uint8_t i;
	uint16_t temp;

	for(i=0;i < MAX_TX_CHANNEL;i++)
	{
		dec+=3;
		if(dec>=8)
		{
			dec-=8;
			p++;
		}

		temp = ((*((uint32_t *)p))>>dec) & 0x7FF;

		pusChannelData[i] = temp;
		p++;
	}
}

void Clear_TxStream_Channel_Buffer(unsigned char *pucTxStreamChannel)
{
	int i;

	for(i = 0; i < MAX_TX_CHANNEL_BUF_LEN; i++) {
		pucTxStreamChannel[i] = 0x0;	// Clear
	}
}


void Print_TxStream_Channel_Data(uint16_t *pusChannelData) 
{
	int i;

	printf("Channel Data : ");
	for( i = 0; i < MAX_TX_CHANNEL; i++) {
		printf("0x%04x ", pusChannelData[i]);
	}
	printf("\n");
}

void Print_TxStream_Channel_Buffer(uint8_t *pucChannelBuffer) 
{
	int i, j;

	printf("Channel Buffer : ");
	for( i = 0; i < MAX_TX_CHANNEL_BUF_LEN; i++) {
		printf("0x%02x ", pucChannelBuffer[i]);
	}
	printf("\n");

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
}

void Fill_TxStream_Buf(union TxStreamData *stTmpTSData) 
{
	uint16_t	ChannelData[MAX_TX_CHANNEL] = {				// 16
							0x7FF, 	0, 		0x7FF, 	0, 
							0x7FF, 	0, 		0x7FF, 	0, 
							0x7FF, 	0, 		0x7FF, 	0, 
							0x7FF, 	0, 		0x7FF, 	0, 
							};
	uint8_t		ucChannelBuffer[MAX_TX_CHANNEL_BUF_LEN];	// 22
	uint8_t	ucRxNum;

	stTmpTSData->stTS.ucHead = 0x55;	//
	stTmpTSData->stTS.ucSubProtocol = BIND_OFF << 7 | SUB_PROTOCOL_SYMAX;	//
	stTmpTSData->stTS.ucType = SUB_PROTOCOL_TYPE_SYMAX << 4 | (ucRxNum % 16) & 0xF;
	stTmpTSData->stTS.ucOptionProtocol = 0; // -128..127

	Clear_TxStream_Channel_Buffer(ucChannelBuffer);
#ifdef DEBUG_NK
	Print_TxStream_Channel_Data(ChannelData);
#endif

	Fill_TxStream_Channel_Data(ucChannelBuffer, ChannelData);	// dest, src
	memcpy(stTmpTSData->stTS.ucChan, ucChannelBuffer, MAX_TX_CHANNEL_BUF_LEN);	// 22

#ifdef DEBUG_NK
	Print_TxStream_Channel_Buffer(stTmpTSData->stTS.ucChan);
#endif
}

int SetBaudRate(int fd) 
{
	struct termios SerialPortSettings;
	
	tcgetattr(fd, &SerialPortSettings);

#if 0
	cfsetispeed(&SerialPortSettings,B57600);
	cfsetospeed(&SerialPortSettings,B57600);
#else
	cfsetispeed(&SerialPortSettings,B115200);
	cfsetospeed(&SerialPortSettings,B115200);
#endif

	SerialPortSettings.c_cflag &= ~PARENB; /*CLEAR Parity Bit PARENB*/
//	SerialPortSettings.c_cflag |= PARENB;  /*Set Parity Bit PARENB*/
//	SerialPortSettings.c_cflag |=  PARODD; /*SET   Parity Bit PARENB*/

	SerialPortSettings.c_cflag &= ~CSTOPB; //Stop bits = 1
//	SerialPortSettings.c_cflag |= CSTOPB; //Stop bits = 2

	SerialPortSettings.c_cc[VMIN]  = 1;
	SerialPortSettings.c_cc[VTIME] = 5;

	SerialPortSettings.c_cflag &= ~CSIZE; /* Clears the Mask       */
	SerialPortSettings.c_cflag |=  CS8;   /* Set the data bits = 8 */
	SerialPortSettings.c_cflag |= HUPCL;
	SerialPortSettings.c_cflag &= ~CRTSCTS;
	SerialPortSettings.c_cflag |= CREAD | CLOCAL;
	SerialPortSettings.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL | INLCR);
	SerialPortSettings.c_oflag &= ~(OCRNL | ONLCR | ONLRET | ONOCR | OFILL | OLCUC | OPOST);
	tcsetattr(fd,TCSANOW,&SerialPortSettings);
}

int main(int argc, char *argv[]) 
{

	int fd;
	union TxStreamData stTxStreamBuf;

	struct timeval tv_start, tv_end;
	unsigned long elapsed_time;

	fd = open(argv[1], O_RDWR|O_SYNC);
	if( fd < 0 ) {
		printf("Error - File(%s) open !!\n", argv[1]);
		exit(1);
	}

	SetBaudRate(fd);
	sleep(3);

#ifdef DEBUG_NK
	printf("Sizeof(unsigned short) : %lu\n", sizeof(unsigned short));
	printf("Sizeof(unsigned int) : %lu\n", sizeof(unsigned int));
	printf("Sizeof(unsigned long) : %lu\n", sizeof(unsigned long));
#endif

	while( 1 ) {
		gettimeofday(&tv_start, NULL);

		printf("Write TX Stream ! \n");
		Fill_TxStream_Buf(&stTxStreamBuf);

		write(fd, &stTxStreamBuf, MAX_TX_STREAM_LEN);

		gettimeofday(&tv_end, NULL);

		elapsed_time = (tv_end.tv_sec - tv_start.tv_sec)*1000000 + tv_end.tv_usec - tv_start.tv_usec;
		printf("Elapsed time : %ld\n", elapsed_time);
	}

	close(fd);
}

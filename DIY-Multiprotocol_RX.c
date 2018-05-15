#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <termios.h>
#include <string.h>

#define MAX_TX_DATA		26
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
	uint8_t ucChan[MAX_TX_CHANNEL_BUF_LEN];
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

union TxStreamData stTXBuffer;

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

	for(i = 0; i < MAX_TX_CHANNEL; i++) {
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
	uint16_t	ChannelData[MAX_TX_CHANNEL] = { 
							0x7FF, 	0, 		0x7FF, 	0, 
							0x7FF, 	0, 		0x7FF, 	0, 
							0x7FF, 	0, 		0x7FF, 	0, 
							0x7FF, 	0, 		0x7FF, 	0, 
							};
	uint8_t		ucChannelBuffer[MAX_TX_CHANNEL_BUF_LEN+3];
	uint8_t	ucRxNum;

	stTmpTSData->stTS.ucHead = 0x55;	//
	stTmpTSData->stTS.ucSubProtocol = BIND_OFF << 7 | SUB_PROTOCOL_SYMAX;	//
	stTmpTSData->stTS.ucType = SUB_PROTOCOL_TYPE_SYMAX << 4 | (ucRxNum % 16) & 0xF;
	stTmpTSData->stTS.ucOptionProtocol = 0; // -128..127

	Clear_TxStream_Channel_Buffer(ucChannelBuffer);

	Print_TxStream_Channel_Data(ChannelData);

	Fill_TxStream_Channel_Data(ucChannelBuffer, ChannelData);
	memcpy(stTmpTSData->stTS.ucChan, ucChannelBuffer, MAX_TX_CHANNEL_BUF_LEN);

	Print_TxStream_Channel_Buffer(stTmpTSData->stTS.ucChan);
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

int ReadData(int fd, char *buffer, int size)
{
	int nRet, nError = 0, data_read = 0;
	int size_to_read;

	size_to_read = size;

	while( 1 ) {
		nRet = read(fd, buffer+data_read, size_to_read);

		if( nRet < 0 ) {
			printf("Error - read \n");
			nError = nRet;	
			break;
		}
		if( nRet == 0 ) {
			nError = 0;
			break;
		}

		if( nRet == size_to_read ) {
			break;
		} else {
			data_read += nRet;
			size_to_read -= nRet;
		}
	}

	return nError;
}

int main(int argc, char *argv[]) 
{
	FILE *fpi;
	int fdi, fdo, nRet;
	union TxStreamData *pstTSB;
	uint16_t usChannelData[MAX_TX_CHANNEL];
	int i, j, k, count;
	unsigned int unTmp[MAX_TX_DATA];

	if( argc < 3 ) {
		printf("Not engouth argument !!\n");
		exit(1);
	}

	fdi = open(argv[1], O_RDONLY|O_SYNC);
	if( fdi < 0 ) {
		printf("Error - File(%s) open !!\n", argv[1]);
		exit(1);
	}

	fdo = open(argv[2], O_RDWR|O_CREAT|O_TRUNC|O_NOCTTY, S_IRWXU | S_IRGRP | S_IWGRP | S_IROTH );
	if( fdo < 0 ) {
		printf("Error - File(%s) open !!\n", argv[2]);
		exit(1);
	}

	printf("Setting UART device !!\n");
	SetBaudRate(fdi);

#ifdef DEBUG_NK
	printf("Sizeof(unsigned short) : %lu\n", sizeof(unsigned short));
	printf("Sizeof(unsigned int) : %lu\n", sizeof(unsigned int));
	printf("Sizeof(unsigned long) : %lu\n", sizeof(unsigned long));
#endif

	pstTSB = &stTXBuffer;

	printf("sizeof(union TxStreamData) : %lu\n", sizeof(union TxStreamData));

	for(count = 0; 1 ;count++ ) {
		memset(pstTSB, 0x00, MAX_TX_DATA);

        nRet = ReadData(fdi, &stTXBuffer.ucByte[0], MAX_TX_DATA);

#ifdef DEBUG_NK
		printf("rx_ok_buff : 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
					pstTSB->TS_HEAD, 
					pstTSB->TS_SPROTOCOL, 
					pstTSB->TS_TYPE, 
					pstTSB->TS_OPROTOCOL,
					(unsigned char)pstTSB->TS_CH(0),
					(unsigned char)pstTSB->TS_CH(1),
					(unsigned char)pstTSB->TS_CH(2),
					(unsigned char)pstTSB->TS_CH(3),
					(unsigned char)pstTSB->TS_CH(4),
					(unsigned char)pstTSB->TS_CH(5),
					(unsigned char)pstTSB->TS_CH(6),
					(unsigned char)pstTSB->TS_CH(7),
					(unsigned char)pstTSB->TS_CH(8),
					(unsigned char)pstTSB->TS_CH(9),
					(unsigned char)pstTSB->TS_CH(10),
					(unsigned char)pstTSB->TS_CH(11),
					(unsigned char)pstTSB->TS_CH(12),
					(unsigned char)pstTSB->TS_CH(13),
					(unsigned char)pstTSB->TS_CH(14),
					(unsigned char)pstTSB->TS_CH(15),
					(unsigned char)pstTSB->TS_CH(16),
					(unsigned char)pstTSB->TS_CH(17),
					(unsigned char)pstTSB->TS_CH(18),
					(unsigned char)pstTSB->TS_CH(19),
					(unsigned char)pstTSB->TS_CH(20),
					(unsigned char)pstTSB->TS_CH(21));
#endif

//		Print_TxStream_Channel_Buffer(pstTSB->stTS.ucChan);

//		Decode_TxStream_Channel_Buffer((uint8_t *)&pstTSB->TS_CH(0), usChannelData);

		for(i = 0; i < MAX_TX_DATA; i++) {
			nRet = write(fdo, &pstTSB->ucByte[i], sizeof(uint8_t));
			if( nRet < 0 ) {
				printf("Error - write !!\n");
				exit(1);
			}
		}
	}

	close(fdi);
	close(fdo);
}

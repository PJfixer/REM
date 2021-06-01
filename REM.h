/*
 * REM.h
 *
 *  Created on: 3 mars 2021
 *      Author: pjeanne
 *
 *
 * 		REM (Radio exchange & monitoring) Protocol is a simple protocole layer for RF communication
 * 		The protcole is based on a simple Master/slave communication
 *
 * 		a typical REM FRAME look like
 *
 * 			Start																		   						End
 * 		------------------------------------------------------------------------------------------------------------------
 *      |	0xAA	|	Slave Address	| Frame type |	Number of data bytes	|	Data bytes	|	CRC-16	|	0xBB	|
 * 		------------------------------------------------------------------------------------------------------------------
 * 			  1             1(0-255)         1              1(0-255)			        x			   2		  1
 *
 *
 * 		The maximum REM Frame length is 128 bytes which means that the maximum payload length is 121 bytes
 *
 * 		The maximum salve number is 128
 *
 * 		The ID 0x00 is reserved for the REM master
 *
 * 		The frame type can be
 *
 * 		0xF1 : Request of data
 * 		0xF2 : set Command
 * 		0xF3 : ACK or NACK
 * 		0xF4 :  ACK or NACK + response data
 *
 *
 *		For each master request an answer from slave  is excepted the answer frame can be simple ACK or ACK+Data
 *
 *		Simple ACK
 *
 *			Start													End
 * 		-----------------------------------------------------------------------------
 *      |	0xAA	|	0x00	| 0xF3  |	0x01	|	ACK	 |	CRC-16	|	0xBB	|
 * 		-----------------------------------------------------------------------------
 * 			  1            1       	1	  1(0-255)		 1			2		 1
 *
 *
* 		 ACK + Data
 *
 *			Start																					End
 * 		-------------------------------------------------------------------------------------------------------------
 *      |	0xAA	|	0x00	| 0xF4 |	Number of data bytes + ACK(1)	|	ACK + DATA	 |	CRC-16	|	0xBB	|
 * 		-------------------------------------------------------------------------------------------------------------
 * 			  1            1      	1		 		1(0-255)		 				    x			2		 1
 *
 *
 *		ACK values :
 *
 *		ACK : 0x40
 *		NO-ACK : 0x80
 */


#ifndef INC_REM_H_

#define INC_REM_H_



#include "stdint.h"
#include "main.h"
#include "string.h"
#include "crc.h"

#define REM_FRAME_MAX_SIZE   128
#define REM_MAX_PAYLOAD_SIZE   (REM_FRAME_MAX_SIZE-7) // START + SLAVE_ADDR +FRAME TYPE + DATA_LEN + CRC16 + END (8)

#define REM_FRAME_TIMEOUT_MS 500

#define FRAME_TYPE_DATA_REQUEST 0xF1
#define FRAME_TYPE_SET_COMMAND 0xF2
#define FRAME_TYPE_ACKNOWLEDGE	0xF3
#define FRAME_TYPE_ACKNOWLEDGE_RESPONSE 0xF4

#define FRAME_ACK 0x40
#define FRAME_NACK 0x80

typedef struct{
	uint8_t msg[REM_MAX_PAYLOAD_SIZE];
	uint8_t msg_len;
    uint8_t REM_FRAME_RECEIVED ;
    uint8_t REM_ID ;
    uint8_t last_recv_frame_type ;
    int8_t last_recv_frame_ack ;
    uint8_t current_recv_expect_bytes_nb;
    uint8_t rem_start_frame;
    uint8_t rem_end_frame;
    uint32_t frame_timer;
    uint8_t byte_counter ;
    uint8_t error ;


}REM_state;


extern uint8_t REM_TX_FRAME[];
extern uint8_t REM_RX_FRAME[];

extern volatile REM_state Rem1;







int Pack_REM_message(uint8_t * i_message,uint8_t i_slave_address,uint8_t i_length,uint8_t Frame_type);
int Check_Unpack_REM_message(uint8_t i_packet_length, uint8_t * data);
void REM_set_salveID(uint8_t slaveID);
void REM_recv_callback(uint8_t * p_Rx_buffer,uint8_t i_incomingByte);
int REM_pack_ACK(uint8_t address,uint8_t ACK_bool);
int send_REM_buffer(uint8_t length);
int REM_sendPacket_waitForAck(uint16_t packet_length,int ms_timeout,int max_retries);
#endif /* INC_REM_H_ */

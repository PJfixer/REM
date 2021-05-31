/*
 * REM.c
 *
 *  Created on: 5 mars 2021
 *      Author: pjeanne
 */

#include "REM.h"




uint8_t REM_TX_FRAME[REM_FRAME_MAX_SIZE];
uint8_t REM_RX_FRAME[REM_FRAME_MAX_SIZE];







 volatile REM_state Rem1;


int Pack_REM_message(uint8_t * i_message, uint8_t i_slave_address, uint8_t i_length,uint8_t Frame_type)
{
	int index = 0;
	uint16_t CRC16 = 0;
	if(i_length <= REM_MAX_PAYLOAD_SIZE )
	{
		REM_TX_FRAME[index++] = 0xAA;
		REM_TX_FRAME[index++] = i_slave_address;
		REM_TX_FRAME[index++] = Frame_type;
		REM_TX_FRAME[index++] = i_length;

		memcpy(&REM_TX_FRAME[index],i_message,i_length);
		index = index + i_length;
		CRC16 = crc16(i_message,(unsigned int)i_length);
		memcpy(&REM_TX_FRAME[index],&CRC16,sizeof(CRC16));
		index = index + sizeof(CRC16);
		REM_TX_FRAME[index++] = 0xBB;

	}
	else
	{
		index = -1;
	}

	return index;


}

int Check_Unpack_REM_message(uint8_t i_packet_length, uint8_t * data)
{
	int res = -1;
	uint16_t Packet_CRC =0;
	uint16_t Calc_CRC = 0;
	if(i_packet_length <= REM_FRAME_MAX_SIZE)
	{
		if(REM_RX_FRAME[0] == 0xAA && REM_RX_FRAME[i_packet_length - 1] == 0XBB) // verification debut de trame et fin
		{
			if(REM_RX_FRAME[1] == Rem1.REM_ID) // si la trame est pour nous on l'evalue
			{
				if(REM_RX_FRAME[2] == 0xF1 || REM_RX_FRAME[2] == 0xF2 || REM_RX_FRAME[2] == 0xF3 || REM_RX_FRAME[2] == 0xF4 )// si le type de trame est supporté
				{
					Rem1.last_recv_frame_type = REM_RX_FRAME[2];
					if(Rem1.last_recv_frame_type == 0xF3 || Rem1.last_recv_frame_type == 0xF4 ) // of the frame contain an ACK
					{
						Rem1.last_recv_frame_ack = REM_RX_FRAME[4];
					}
					Rem1.msg_len = REM_RX_FRAME[3];
					memcpy(data,&REM_RX_FRAME[4],Rem1.msg_len);
					memcpy(&Packet_CRC,&REM_RX_FRAME[(i_packet_length-3)],sizeof(Packet_CRC));
					Calc_CRC = crc16(&REM_RX_FRAME[4],(unsigned char)Rem1.msg_len);
					if(Packet_CRC == Calc_CRC)
					{
						res = 0;
					}
				}

			}
		}

	}

return res;
}


int REM_sendPacket_waitForAck(uint16_t packet_length,int ms_timeout,int max_retries)
{
	int start_time = 0;
	uint8_t retries = 0;
	Rem1.last_recv_frame_ack = -1; // reset the received ack status

	send_REM_buffer(packet_length);
	start_time = HAL_GetTick();
	 while (retries < max_retries)// while the retries number is less than the max_retries
	 {
		if(Rem1.REM_FRAME_RECEIVED == 1) //if a REM frame was received and we were acknowledge or not
		{

			if(Rem1.last_recv_frame_ack == 0x40)
			{
				return 0 ; //ACK OK
			}
			else if(Rem1.last_recv_frame_ack == 0x80)
			{
				return -1; //NACK
			}
		}
		else if((HAL_GetTick() - start_time) > ms_timeout) //if no response after ms_timeout re-send the packet & incremnt the number of retry
		{
			start_time = HAL_GetTick();
			send_REM_buffer(packet_length);
			retries++;
		}

	 }
	 return -2; // if we hit the max retries without any response return -2


}


int REM_pack_ACK(uint8_t address,uint8_t ACK_bool)
{
	int index = 0;
	uint16_t CRC16 = 0;
	REM_TX_FRAME[index++] = 0xAA;
	REM_TX_FRAME[index++] = address;
	REM_TX_FRAME[index++] = 0xF3;
	REM_TX_FRAME[index++] = 0x01;
	if(ACK_bool == 1)
	{
		REM_TX_FRAME[index++] = 0x40;
	}
	else
	{
		REM_TX_FRAME[index++] = 0x80;
	}
	CRC16 = crc16(&REM_TX_FRAME[index-1],1);
	memcpy(&REM_TX_FRAME[index],&CRC16,sizeof(CRC16));
	index = index + sizeof(CRC16);
	REM_TX_FRAME[index++] = 0xBB;

	return index;


}

int REM_add_ACK(uint8_t * i_message,uint8_t i_length,uint8_t ack_bool)
{
	for(int i =i_length;i>=0;i--)
	{
		i_message[i+1] = i_message[i];
	}
	if(ack_bool == 1)
	{
		i_message[0] = 0x40;
	}
	else
	{
		i_message[0] = 0x80;
	}
	return 0;
}

void REM_recv_callback(uint8_t * p_Rx_buffer,uint8_t i_incomingByte)
{
	 if(Rem1.REM_FRAME_RECEIVED == 0) // if we are not already processing a REM frame
	 {

		 if(Rem1.rem_start_frame == 1)
		 {
			p_Rx_buffer[Rem1.byte_counter] = i_incomingByte;
			Rem1.byte_counter++;
			if(Rem1.byte_counter == 4) // read the number of data bytes
			{
				Rem1.current_recv_expect_bytes_nb = i_incomingByte;
			}

			if(i_incomingByte == 0xBB  && Rem1.byte_counter == (Rem1.current_recv_expect_bytes_nb + 7 )) //if we detect a end  caracter and we get the right number of data
			{
				Rem1.rem_end_frame = 1;
			 }
			if(Rem1.rem_start_frame == 1 && Rem1.rem_end_frame == 1 ) // we set the REM_FRAME_RECEIVED frame status so we cant move REM_RX_FRAME until the frame is processed
			{
				Rem1.rem_start_frame = 0;
				Rem1.rem_end_frame = 0;

				if(Check_Unpack_REM_message((uint8_t )Rem1.byte_counter,(uint8_t *) Rem1.msg) == 0)
				{
					Rem1.REM_FRAME_RECEIVED = 1 ;
					Rem1.byte_counter = 0;
				}
				else
				{
					Rem1.error++;
					Rem1.byte_counter = 0;
				}

			}


		 }
		 if(Rem1.rem_start_frame == 0 && i_incomingByte == 0xAA)
		 {
			Rem1.frame_timer = HAL_GetTick();
			p_Rx_buffer[Rem1.byte_counter] = i_incomingByte;
			Rem1.byte_counter++;
			Rem1.rem_start_frame = 1;
		 }

	 }
	 if(Rem1.byte_counter > REM_FRAME_MAX_SIZE)
	 {
		 Rem1.byte_counter = 0;
	 }
}


void REM_set_salveID(uint8_t slaveID)
{
	Rem1.REM_ID = slaveID;
}

__weak int send_REM_buffer(uint8_t length)
{
	/* NOTE: This function Should not be modified, when needed,
	       the send_REM_buffer could be implemented in the user  file
	  */
}


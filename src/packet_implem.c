	#include "packet_interface.h"

	/* Extra #includes */
	/*Standard input output library*/
	#include <stdio.h>
	/*Standard general utilities library*/
	#include <stdlib.h>
	/*Library related to array of char manipulation*/
	#include <string.h>
	/*Endian gestion*/
	#include <arpa/inet.h>
	/*Crc gestion*/
	#include <zlib.h>

	struct __attribute__((__packed__)) pkt {
	    ptypes_t TYPE:3; /* soit PTYPE_DATA soit PTYPE_ACK le reste doit être ignoré */
		uint8_t WINDOW:5; /* [0,31] peut varier, val initiale = INIT_WINDOW_SIZE */
		uint8_t SEQNUM; /* [0,255] dépends du type de paquet DATA -> num de séquence (1er => 0) si atteint 255 recommence à 0
																ACK -> Prochain num de séquence attendu ((last seqNum +1)%2^8) */
		uint16_t LENGTH; /* [0,512 ] htons ! si = 0 et SEQNUM == last num ack envoyé ==> Fin du transfert. Si >512 paquet ignoré */
		uint32_t TIMESTAMP; /* Valeur libre a chaque grp  */
		char * PAYLOAD; /* Taille donnée par le champ LENGTH */
		uint32_t CRC; /* /!\ ENDIANNESS compute sur tout le paquet. Calculé à la réception : Si diffère paquet jetté.
	Erreur réseau possible : perte, corruption, ordre =/= et latence	*/
	};

	/* Extra code */
	/* Your code will be inserted here */

	pkt_t* pkt_new()
	{
		pkt_t * pack = (pkt_t*) calloc(1,sizeof(pkt_t));
		return pack;
	}

	void pkt_del(pkt_t *pkt)
	{
		//if the ptr is NULL no operation is performed -> no need to test it (see man)
		free(pkt);
	}

	pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt)
	{
		//Check size buffer
		if(len <12){
			return E_NOMEM;
		}

		if(pkt_set_type(pkt,data[0]>>5) != PKT_OK){
			return E_TYPE;
		}
		pkt_set_window(pkt,(data[0]));
		pkt_set_seqnum(pkt, data[1]);

		ptypes_t type = pkt_get_type(pkt);
		//Check type exists
		if(type != PTYPE_DATA && type != PTYPE_ACK){
			return E_TYPE;
		}

		//Check window size
		if( pkt->WINDOW > MAX_WINDOW_SIZE){
			return E_WINDOW;
		}

		uint16_t length;
		memcpy(&length,data+2,sizeof(uint16_t));
		//Conversion endianess 
		pkt->LENGTH = ntohs(length);

		//Check size payload not exceeded
		if(pkt_get_length(pkt)>MAX_PAYLOAD_SIZE){
			return E_LENGTH;
		}

		//Check size are equals
		if(len-12!=pkt_get_length(pkt)){
			return E_UNCONSISTENT;
		}

		//Check if there is a payload
		if(len < 12 && type == PTYPE_DATA){
			return E_UNCONSISTENT;
		}
		
		uint32_t timestamp = 0;
		memcpy(&timestamp,data+4,sizeof(uint32_t));
		pkt_set_timestamp(pkt,timestamp);

    	if(pkt_set_payload(pkt, data+8, pkt_get_length(pkt)) != PKT_OK){
    		return E_NOMEM;
    	}

		uint32_t crcRecalculate = crc32(0,(const Bytef*) data, len-4);
		uint32_t crcPck;
		//We copy the crc of the pck
		memcpy(&crcPck, data + len - 4,sizeof(uint32_t));
		crcPck = ntohl(crcPck);
		//Check if both crc are =/=
		if(crcRecalculate != crcPck){
			return E_CRC;
		}
		pkt_set_crc(pkt, crcRecalculate);
		return PKT_OK;

	}

	pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len)
	{
		ptypes_t type = pkt_get_type(pkt);
		//Check type exists
		if(type != PTYPE_DATA && type != PTYPE_ACK){
			return E_TYPE;
		}
		//Check window size
		if( pkt->WINDOW > MAX_WINDOW_SIZE){
			return E_WINDOW;
		}
		// Error packet too big
		if( pkt->LENGTH > MAX_PAYLOAD_SIZE){
			return E_LENGTH;
		}

		//Error : buffer too small 
		if(*len <12){
			return E_NOMEM;
		}

		uint8_t firstPart = ((pkt->TYPE << 5) | (pkt->WINDOW));
		uint8_t seqnum = pkt->SEQNUM;
		uint16_t length = htons(pkt->LENGTH);
		uint32_t timestamp = pkt->TIMESTAMP;
		
		memset(buf,0,*len);

		memcpy(buf,&firstPart,sizeof(uint8_t));
		memcpy(buf+1*sizeof(char),&seqnum,sizeof(uint8_t));
		memcpy(buf+2*sizeof(char),&length,sizeof(uint16_t));
		memcpy(buf+4*sizeof(char),&timestamp,sizeof(uint32_t));

		if(pkt->LENGTH > 0){
			memcpy(buf+8,pkt->PAYLOAD,pkt->LENGTH);
		}

		uint32_t crc = htonl(crc32(0,(const Bytef *) buf, ((8+pkt->LENGTH)*sizeof(char))));
		memcpy(buf+8+pkt->LENGTH,&crc,sizeof(uint32_t));
		*len = 12+(pkt_get_length(pkt));
		return PKT_OK;
	}

	ptypes_t pkt_get_type(const pkt_t* pkt)
	{
        return pkt->TYPE;
	}

	uint8_t  pkt_get_window(const pkt_t* pkt)
	{
		return pkt->WINDOW;
	}

	uint8_t  pkt_get_seqnum(const pkt_t* pkt)
	{
		return pkt->SEQNUM;
	}

	uint16_t pkt_get_length(const pkt_t* pkt)
	{
		return pkt->LENGTH;
	}

	uint32_t pkt_get_timestamp(const pkt_t* pkt)
	{
		return pkt->TIMESTAMP;
	}

	uint32_t pkt_get_crc(const pkt_t* pkt)
	{
		return pkt->CRC;
	}

	const char* pkt_get_payload(const pkt_t* pkt)
	{
		return pkt->PAYLOAD;
	}


	pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type)
	{
		if(type != PTYPE_ACK && type != PTYPE_DATA){
			return E_TYPE;
		}else{
			pkt->TYPE = type;
			return PKT_OK;
		}
	}

	pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window)
	{
		pkt->WINDOW = window;
		return PKT_OK;
	}

	pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum)
	{
		pkt->SEQNUM = seqnum;
		return PKT_OK;
	}

	pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length)
	{
		if(length > MAX_PAYLOAD_SIZE){
			return E_LENGTH;
		}
		pkt->LENGTH = length;
		return PKT_OK;
	}

	pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp)
	{
		pkt->TIMESTAMP = timestamp;
		return PKT_OK;
	}

	pkt_status_code pkt_set_crc(pkt_t *pkt, const uint32_t crc)
	{
		pkt->CRC = crc;
		return PKT_OK;
	}

	pkt_status_code pkt_set_payload(pkt_t *pkt,
								    const char *data,
									const uint16_t length)
	{
		char* payload;
		payload = malloc((length+1)*sizeof(char));
		memcpy(payload,data,length);
		pkt->PAYLOAD = payload;
		pkt->LENGTH = length;
		return PKT_OK;

	}
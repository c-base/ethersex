
#include "config.h"
#include "driver/nrf24l01.h"
#include "core/debug.h"
#include "protocols/c-beam/c-beam.h"

#include <avr/io.h>
#include <avr/pgmspace.h>

#include <string.h>

typedef struct {
    uint8_t length;
    uint8_t protocol;
    uint8_t flags;
    uint8_t strength;
    uint32_t sequence;
    uint32_t id;
    uint8_t button1;
    uint8_t button2;
    uint16_t crc;
} beacon_data_t;

typedef struct {
    uint8_t length;
    uint8_t protocol;
    uint32_t id;
    char name[8];
    uint16_t crc;
} beacon_name_t;

void r0ketbeam_init(void){
    //nrf24_init();
    nrf24_setDataRate(RF24_2MBPS);
    nrf24_setChannel(81);
    nrf24_setAutoAck(0);
    nrf24_setPayloadSize(16);
    nrf24_setCRCLength(RF24_CRC_8);
    nrf24_openReadingPipe(1, 0x0102030201LL);
    nrf24_openWritingPipe(0x0102030201LL);
#ifdef NRF24L01_PROTOCOL_R0KET_DEBUG
    debug_printf("RF24 Carrier: %c", (carrier?'Y':'N'));
#endif
    nrf24_startListening();
}

inline uint8_t min(uint8_t a, uint8_t b){
    return (a<b ? a : b);
}

inline uint16_t flip16(uint16_t number){
    return number>>8|number<<8;
}

inline uint32_t flip32(uint32_t number){
    return flip16(number>>16)|(uint32_t)flip16(number&0xFFFF)<<16;
}

//FIXME check if this is not already available in the avr-libc
uint16_t crc16(uint8_t* buf, int len)
{
    /* code from r0ket/firmware/basic/crc.c */
    uint16_t crc = 0xffff;
    for(int i=0; i < len; i++){
        crc = (unsigned char)(crc >> 8) | (crc << 8);
        crc ^= buf[i];
        crc ^= (unsigned char)(crc & 0xff) >> 4;
        crc ^= (crc << 8) << 4;
        crc ^= ((crc & 0xff) << 4) << 1;
    }
    return crc;
}

//CAUTION!!1! "id" is little-endian
void sendNick()
{
    beacon_name_t pkt;
    pkt.length = 16;
    pkt.protocol = 0x23;
    pkt.id = NRF24L01_PROTOCOL_R0KET_ID;
    memset(&pkt.name, 0, sizeof(pkt.name));
    strncpy((char*)&pkt.name, NRF24L01_PROTOCOL_R0KET_NICK, min(8,strlen(NRF24L01_PROTOCOL_R0KET_NICK)));
    pkt.crc = flip16(crc16((uint8_t*)&pkt, sizeof(pkt)-2));

    nrf24_stopListening();
    nrf24_openWritingPipe(0x0102030201LL);
    nrf24_write(&pkt, sizeof(pkt));
    nrf24_startListening();
}

//CAUTION!!1! "id" and "sequence" are little-endian
void sendDummyPacket(uint32_t id, uint32_t sequence)
{
    beacon_data_t pkt;
    pkt.length = 16;
    pkt.protocol = 0x17;
    pkt.id = id;
    pkt.sequence = sequence;
    pkt.flags = 0x00; 
    pkt.button1 = 0xFF;
    pkt.button2 = 0xFF;

    pkt.crc = flip16(crc16((uint8_t*)&pkt, sizeof(pkt)-2));

    nrf24_stopListening();
    nrf24_write((uint8_t*)&pkt, sizeof(pkt));
    nrf24_startListening();
}

char* nick = "c_leuse";

void r0ketbeam_periodic(void){
#ifdef NRF24L01_PROTOCOL_R0KET_SEND_NICK_SUPPORT 
    static int last_sent_packet = 0;
    //This code looks so crappy because it has gone a long, long way until it ended up here.
    //sorry, since we are not arduino, we do not yet have a millis() function.
    //FIXME untested, probably won't compile
    int delta = millis() - last_sent_packet;
    if (delta > 1234){
        last_sent_packet = millis(); //FIXME little endianness, anyone? //FIXME bogus commentary, anyone?

        uint32_t id = 0x78563412UL;
        char myNick[9];
        sprintf((char*)&myNick, "*[%u]", (unsigned int)millis());
        myNick[sizeof(myNick)-1] = '\0';

        Serial.print("# Sending nick '");
        Serial.print(myNick);
        Serial.print("' as 0x");
        Serial.print(id, HEX);
        Serial.println(" ...");
        sendNick(id, (char*)&myNick);

        //sendDummyPacket(id, last_sent_packet);
    }
#endif

    if (nrf24_available()){
        beacon_data_t buf;
        nrf24_read(&buf, sizeof(buf));

#ifdef NRF24L01_PROTOCOL_R0KET_DEBUG
        debug_printf("RF24 RECV %h: %h (%d) %h", flip32(buf.id), flip32(buf.sequence), buf.strength, buf.protocol);
#endif

        uint16_t crc = flip16(crc16((uint8_t*)&buf, sizeof(buf)-2));
        if (crc != buf.crc){
#ifdef NRF24L01_PROTOCOL_R0KET_DEBUG
            debug_printf("NRF24L01 r0ket CRC mismatch: expected %h got %h", crc, buf.crc);
#endif
        }else{
            //TODO I do not know if the following check is necessary
            if(buf.protocol == 0x17){
                char params[48]; //TODO does this need a config option? It could need more for nicks > 16 char or thereabouts
                //FIXME is %h a hex printf in this particular printf implementation? and %d an unsigned decmial?
                snprintf_P(params, sizeof(params), PSTR("\"%h\",\"" NRF24L01_PROTOCOL_R0KET_LOCATION "\",\"%h\",\"%d\""), flip32(buf.id), flip32(buf.sequence), buf.strength);
                c_beam_call("r0ketseen", params);
            }
        }
    }else{
#ifdef NRF24L01_PROTOCOL_R0KET_DEBUG
        debug_printf('NRF24L01 r0ket: no response');
#endif
    }
}

/*
  -- Ethersex META --
  header(hardware/nrf24l01/r0ket.h)
  init(r0ketbeam_init)
  timer(5, r0ketbeam_periodic())
  block([[NRF24L01_PROTOCOL_R0KET]])
*/

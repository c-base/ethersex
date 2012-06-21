/*
 * Copyright (c) 2009 by Bernd Stellwag <burned@zerties.org>
 * Copyright (c) 2009 by Stefan Siegl <stesie@brokenpipe.de>
 * Copyright (c) 2011 by Maximilian Güntner <maximilian.guentner@gmail.com>
 * Copyright (c) 2012 by jaseg <s@jaseg.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * For more information on the GPL, please go to:
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <avr/pgmspace.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "protocols/uip/uip.h"
#include "c-beam.h"

#define STATE (&uip_conn->appstate.c-beam)

static uip_conn_t *c-beam_conn;

static void c-beam_call (const char PROGMEM method[], const char PROGMEM params[]){
    STATE->len = sprintf_P (STATE->outbuf, PSTR(
        "POST / HTTP/1.0\n"
        "User-Agent: r0ketsex on nanode\n"
        "Content-Type: application/json\n"
        "\r\n\r\n"
        "{\"method\":\":\"%s\","
        "\"id\":0,"
        "\"params\":[%s]}\n\n   "), method, params);
}

static void c-beam_main(void){
    if(uip_aborted() || uip_timedout() || uip_close()){
        c-beam_conn = NULL;
        return;
    }
    if((uip_connected() || uip_acked() || uip_newdata() || uip_poll()) && *STATE->outbuf){
        uip_send(*STATE->outbuf, *STATE->len);
    }
    //ignore received data.
    //screw retransmissions.
}

void irc_periodic(void){
    if (! c-beam_conn){
        c-beam_init();
    }
}

void irc_init(void){
    uip_ipaddr_t ip;
    set_CONF_C-BEAM_IP(&ip);
    irc_conn = uip_connect(&ip, HTONS(CONF_C-BEAM_PORT), c-beam_main);
}

/*
  -- Ethersex META --
  header(protocols/c-beam/c-beam.h)
  net_init(c-beam_init)
  timer(2000, c-beam_periodic())

  state_header(protocols/c-beam/c-beam_state.h)
  state_tcp(struct c-beam_connection_state_t c-beam)
*/

/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * RemoteFX USB Redirection
 *
 * Copyright 2012 Atrust corp.
 * Copyright 2012 Alfred Liu <alfred.liu@atruscorp.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FREERDP_CHANNEL_URBDRC_CLIENT_UVC_H
#define FREERDP_CHANNEL_URBDRC_CLIENT_UVC_H

#include <winpr/pool.h>
#include <freerdp/channels/log.h>
#include <pthread.h>

#define LIBUVC_XFER_BUF_SIZE	( 16 * 1024 * 1024 )
#define LIBUVC_XFER_META_BUF_SIZE ( 4 * 1024 )
#define PACKET_HEADER_SIZE 12
#define PACKET_BUFFER_SIZE 3072

#define INT_TO_DW(i, p) \
  (p)[0] = (i); \
  (p)[1] = (i) >> 8; \
  (p)[2] = (i) >> 16; \
  (p)[3] = (i) >> 24;

struct uvc_stream_handle {
    uint8_t fid;
    size_t got_bytes, hold_bytes;
    uint8_t *outbuf, *holdbuf;
    uint32_t seq;

    pthread_mutex_t cb_mutex;

    /* raw metadata buffer if available */
    uint8_t *meta_outbuf, *meta_holdbuf;
    size_t meta_got_bytes, meta_hold_bytes;
};
typedef struct uvc_stream_handle uvc_stream_handle_t;

typedef struct uvc_packet_data {
    uint32_t len;
    BYTE buffer[PACKET_BUFFER_SIZE];

    uint32_t frame_no;
    uint32_t offset;
    time_t   pst;
    time_t   scr;
} uvc_packet_data_t;

extern uvc_stream_handle_t* g_strmh;
extern uvc_packet_data_t g_pktInfo;

void _uvc_process_payload(BYTE *payload, UINT32 payload_len);
void _uvc_swap_buffers(uvc_stream_handle_t *strmh);
void create_file(uvc_stream_handle_t *strmh);
BOOL read_external_file();


#endif /* FREERDP_CHANNEL_URBDRC_CLIENT_MAIN_H */

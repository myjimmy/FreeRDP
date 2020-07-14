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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <winpr/pool.h>
#include <winpr/print.h>

#include <winpr/crt.h>
#include <winpr/synch.h>
#include <winpr/string.h>
#include <winpr/cmdline.h>

#include <freerdp/dvc.h>
#include <freerdp/addin.h>
#include <freerdp/channels/log.h>
#include <freerdp/channels/urbdrc.h>

#include "uvc.h"

uvc_stream_handle_t* g_strmh = NULL;

void _uvc_process_payload(BYTE *payload, UINT32 payload_len) {
    UINT32 header_len;
    BYTE header_info;
    UINT32 data_len;

    /* ignore empty payload transfers */
    if (payload_len == 0)
        return;

    header_len = payload[0];

    if (header_len > payload_len) {
        printf("bogus packet: actual_len=%zd, header_len=%zd\n", payload_len, header_len);
        return;
    }

    data_len = payload_len - header_len;

    if (header_len < 2) {
        header_info = 0;
    } else {
        /** @todo we should be checking the end-of-header bit */
        size_t variable_offset = 2;

        header_info = payload[1];

        if (header_info & 0x40) {
            printf("bad packet: error bit set");
            return;
        }

        if (g_strmh->fid != (header_info & 1) && g_strmh->got_bytes != 0) {
            /* The frame ID bit was flipped, but we have image data sitting
               around from prior transfers. This means the camera didn't send
               an EOF for the last transfer of the previous frame. */
            _uvc_swap_buffers(g_strmh);
        }

        g_strmh->fid = header_info & 1;

        if (header_info & (1 << 2)) {
            //strmh->pts = DW_TO_INT(payload + variable_offset);
            variable_offset += 4;
        }

        if (header_info & (1 << 3)) {
            /** @todo read the SOF token counter */
            //strmh->last_scr = DW_TO_INT(payload + variable_offset);
            variable_offset += 6;
        }

        if (header_len > variable_offset) {
            // Metadata is attached to header
            memcpy(g_strmh->meta_outbuf + g_strmh->meta_got_bytes, payload + variable_offset, header_len - variable_offset);
            g_strmh->meta_got_bytes += header_len - variable_offset;
        }
    }

    if (data_len > 0) {
        memcpy(g_strmh->outbuf + g_strmh->got_bytes, payload + header_len, data_len);
        g_strmh->got_bytes += data_len;

        if (header_info & (1 << 1)) {
            /* The EOF bit is set, so publish the complete frame */
            _uvc_swap_buffers(g_strmh);
        }
    }
}

void _uvc_swap_buffers(uvc_stream_handle_t *strmh) {
    uint8_t *tmp_buf;

    pthread_mutex_lock(&strmh->cb_mutex);

//    (void)clock_gettime(CLOCK_MONOTONIC, &strmh->capture_time_finished);

    /* swap the buffers */
    tmp_buf = strmh->holdbuf;
    strmh->hold_bytes = strmh->got_bytes;
    strmh->holdbuf = strmh->outbuf;
    strmh->outbuf = tmp_buf;
//    strmh->hold_last_scr = strmh->last_scr;
//    strmh->hold_pts = strmh->pts;
//    strmh->hold_seq = strmh->seq;

    /* swap metadata buffer */
    tmp_buf = strmh->meta_holdbuf;
    strmh->meta_holdbuf = strmh->meta_outbuf;
    strmh->meta_outbuf = tmp_buf;
    strmh->meta_hold_bytes = strmh->meta_got_bytes;

//    pthread_cond_broadcast(&strmh->cb_cond);
    pthread_mutex_unlock(&strmh->cb_mutex);

    strmh->seq++;
    strmh->got_bytes = 0;
    strmh->meta_got_bytes = 0;
//    strmh->last_scr = 0;
//    strmh->pts = 0;

    create_file(strmh);
}

#define DEFAULT_PATH "/home/ubuntu/Desktop/webcam/data"
#define FILE_YUY2_PREFIX "yuy2"
//#define FILE_BGR_PREFIX  "bgr"
#define FILE_EXTENSION   "dat"

void create_file(uvc_stream_handle_t *strmh)
{
    char filename[64] = {0};
    FILE* fp = NULL;

    pthread_mutex_lock(&strmh->cb_mutex);
    sprintf(filename, "%s/%s-%dx%d-%04d.%s", DEFAULT_PATH, FILE_YUY2_PREFIX, 640, 480, strmh->seq, FILE_EXTENSION);
    fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("The file '%s' was not opend.\n", filename);
        pthread_mutex_unlock(&strmh->cb_mutex);
        return;
    }

    fwrite(strmh->holdbuf, 1, strmh->hold_bytes, fp);
    fclose(fp);
    pthread_mutex_unlock(&strmh->cb_mutex);
}

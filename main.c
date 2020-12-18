#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TS_PAT_PID 0x0000
#define TS_CAT_PID 0x0001

#define TS_PAT_TABLE_ID 0x00
#define TS_CAT_TABLE_ID 0x01
#define TS_PMT_TABLE_ID 0x02
#define TS_NIT_TABLE_ID 0x40

#define MAX_TS_PROGRAM_NUM 8
#define MAX_TS_PACKET_LEN 188

/**
 * TS packet header
 * 4 bytes
 */
typedef struct t_ts_packet_header {
    unsigned sync_byte                      : 8;
    unsigned transport_error_indicator      : 1;
    unsigned payload_unit_start_indicator   : 1;
    unsigned transport_priority             : 1;
    unsigned ts_PID                         : 13;
    unsigned transport_scrambling_control   : 2;
    unsigned adaptation_field_control       : 2;
    unsigned continuity_counter             : 4;
} T_TS_PACKET_HEADER;

/**
 * PAT
 */
typedef struct t_ts_pat_program {
    unsigned program_number         : 16;
    unsigned program_map_PID        : 13;
} T_TS_PAT_PROGRAM;

typedef struct t_ts_pat {
    unsigned table_id                       : 8;
    unsigned section_syntax_indicator       : 1;
    unsigned zero                           : 1;
    unsigned reserved_1                     : 2;
    unsigned section_length                 : 12;
    unsigned transport_stream_id            : 16;
    unsigned reserved_2                     : 2;
    unsigned version_number                 : 5;
    unsigned current_next_indicator         : 1;
    unsigned section_number                 : 8;
    unsigned last_section_number            : 8;
    unsigned reserved_3                     : 3;
    unsigned network_PID                    : 13;
    unsigned CRC_32                         : 32;
    T_TS_PAT_PROGRAM pat_program[0];
} T_TS_PAT;

/**
 * PMT 
 */
typedef struct t_ts_pmt_stream {
    unsigned stream_type        : 8;
    unsigned elementary_PID     : 13;
    unsigned ES_info_length     : 12;
    unsigned descriptor;
} T_TS_PMT_STREAM;

typedef struct t_ts_pmt {
    unsigned table_id                       : 8;
    unsigned section_syntax_indicator       : 1;
    unsigned zero                           : 1;
    unsigned reserved_1                     : 2;
    unsigned section_length                 : 12;
    unsigned program_number                 : 16;
    unsigned reserved_2                     : 2;
    unsigned version_number                 : 5;
    unsigned current_next_indicator         : 1;
    unsigned section_number                 : 8;
    unsigned last_section_number            : 8;
    unsigned reserved_3                     : 3;
    unsigned PCR_PID                        : 13;
    unsigned reserved_4                     : 4;
    unsigned program_info_length            : 12;
    unsigned reserved_5                     : 3;
    unsigned reserved_6                     : 4;
    unsigned CRC_32                         : 32;
    T_TS_PMT_STREAM pmt_program[0];
} T_TS_PMT;

void parse_ts_header(unsigned char* const header_data, T_TS_PACKET_HEADER *ts_packet_header) {
    unsigned char *data = header_data;

    T_TS_PACKET_HEADER ts_header = {0};
    memset(&ts_header, 0x0, sizeof(ts_header));

    ts_header.sync_byte = data[0];
    ts_header.transport_error_indicator = data[1] >> 7;
    ts_header.payload_unit_start_indicator = (data[1] >> 6) & 0x1;
    ts_header.transport_priority = (data[1] >> 5) & 0x1;
    ts_header.ts_PID = ((data[1] & 0x1f) << 8) | data[2];
    ts_header.transport_scrambling_control = (data[3] >> 6) & 0x3;
    ts_header.adaptation_field_control = (data[3] >> 4) & 0x3;
    ts_header.continuity_counter = data[3] & 0xF;

    memcpy(ts_packet_header, &ts_header, sizeof(ts_header));
}

T_TS_PAT g_ts_pat = {0};

void parse_ts_pat(unsigned char* const pat_data, T_TS_PAT *ts_pat) {
    unsigned char *data = pat_data;
    T_TS_PAT pat = {0};
    memset(&pat, 0x0, sizeof(pat));

    pat.table_id = data[0];
    if(TS_PAT_TABLE_ID != pat.table_id) {
        return;
    }
    pat.section_syntax_indicator = data[1] >> 7;
    pat.zero = (data[1] >> 6) & 0x1;
    pat.reserved_1 = (data[1] >> 4) & 0x3;
    pat.section_length = ((data[1] & 0xf) << 8) | data[2];
    pat.transport_stream_id = (data[3] << 8) | data[4];
    pat.reserved_2 = (data[5] >> 6) & 0x3;
    pat.version_number = (data[5] >> 1) & 0x1f;
    pat.current_next_indicator = (data[5] >> 1) & 0x1;
    pat.section_number = data[6];
    pat.last_section_number = data[7];

    int section_len = pat.section_length;
    
    
}

int main(int argc, char *argv[]) {
    if(2 != argc) {
        printf("Usage: tsparse **.ts\n");
        exit(1);
    }

    int pat_is_find = 0;
    int all_pmt_is_find = 0;

    unsigned char ts_packet[MAX_TS_PACKET_LEN] = {0};

    T_TS_PACKET_HEADER ts_packet_header = {0};

    FILE *fp = NULL;
    fp = fopen(argv[1], "rb"); // "rb": 打开一个二进制文件，文件必须存在，只允许读
    if(!fp) {
        printf("Open file[%s] error!\n", argv[1]);
        exit(1);
    }

    memset(&g_ts_pat, 0x0, sizeof(g_ts_pat));

    while(1) {
        memset(ts_packet, 0x0, sizeof(ts_packet));
        memset(&ts_packet_header, 0x0, sizeof(ts_packet_header));

        if( MAX_TS_PACKET_LEN != fread(ts_packet, 1, MAX_TS_PACKET_LEN, fp)) {
            break;
        }

        parse_ts_header(ts_packet, &ts_packet_header);

        if(0 == pat_is_find) {
            if(TS_PAT_PID == ts_packet_header.ts_PID) {
                if(1 == ts_packet_header.payload_unit_start_indicator) {
                    parse_ts_pat(ts_packet[4 + 1], &g_ts_pat);
                } else {
                    parse_ts_pat(ts_packet[4], &g_ts_pat);
                }
                
                pat_is_find = 1;
            }
        }

    }

    return 0;
}
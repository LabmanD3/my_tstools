#ifndef _TS_PACKET_H
#define _TS_PACKET_H

#include <vector>

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
typedef struct t_ts_packet_header
{
    unsigned sync_byte                          : 8;
    unsigned transport_error_indicator          : 1;
    unsigned payload_unit_start_indicator       : 1;
    unsigned transport_priority                 : 1;
    unsigned ts_PID                             : 13;
    unsigned transport_scrambling_control       : 2;
    unsigned adaptation_field_control           : 2;
    unsigned continuity_counter                 : 4;
} T_TS_PACKET_HEADER;

/**
 * PAT
 */
typedef struct t_ts_pat_program
{
    unsigned program_number         : 16;
    unsigned program_map_PID        : 13;
} T_TS_PAT_PROGRAM;

typedef struct t_ts_pat
{
    unsigned table_id                           : 8;
    unsigned section_syntax_indicator           : 1;
    unsigned zero                               : 1;
    unsigned reserved_1                         : 2;
    unsigned section_length                     : 12;
    unsigned transport_stream_id                : 16;
    unsigned reserved_2                         : 2;
    unsigned version_number                     : 5;
    unsigned current_next_indicator             : 1;
    unsigned section_number                     : 8;
    unsigned last_section_number                : 8;

    std::vector<T_TS_PAT_PROGRAM> pat_program;

    unsigned reserved_3             : 3;
    unsigned network_PID            : 13;
    unsigned CRC_32                 : 32;
} T_TS_PAT;

/**
 * PMT 
 */
typedef struct t_ts_pmt_stream
{
    unsigned stream_type            : 8;
    unsigned elementary_PID         : 13;
    unsigned ES_info_length         : 12;
    unsigned descriptor;
} T_TS_PMT_STREAM;

typedef struct t_ts_pmt
{
    unsigned int pmt_is_find;
    unsigned table_id                           : 8;
    unsigned section_syntax_indicator           : 1;
    unsigned zero                               : 1;
    unsigned reserved_1                         : 2;
    unsigned section_length                     : 12;
    unsigned program_number                     : 16;
    unsigned reserved_2                         : 2;
    unsigned version_number                     : 5;
    unsigned current_next_indicator             : 1;
    unsigned section_number                     : 8;
    unsigned last_section_number                : 8;
    unsigned reserved_3                         : 3;
    unsigned PCR_PID                            : 13;
    unsigned reserved_4                         : 4;
    unsigned program_info_length                : 12;

    std::vector<T_TS_PMT_STREAM> pmt_stream;

    unsigned reserved_5             : 3;
    unsigned reserved_6             : 4;
    unsigned CRC_32                 : 32;
} T_TS_PMT;

#endif
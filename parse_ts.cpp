#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_TS_PACKET_LEN 188

// PID
#define TS_PAT_PID 0x0000
#define TS_CAT_PID 0x0001

// table_id
#define TS_PAT_TABLE_ID 0x00
#define TS_CAT_TABLE_ID 0x01
#define TS_PMT_TABLE_ID 0x02
#define TS_NIT_TABLE_ID 0x40

// OK and NOT_OK defined
#define PARSE_SUCCESS           1
#define PARSE_TS_HEADER_FAIL    2
#define PARSE_IS_NOT_PAT        3
#define PARSE_IS_NOT_PMT        4

typedef unsigned short HANDLE_STATUS;

/**
 * TS packet header
 * 4 bytes
 */
    typedef struct t_ts_packet_header
{
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
 * PAT payload part
 */
typedef struct t_ts_pat_program
{
    unsigned program_number         : 16;
    unsigned program_map_PID        : 13;
} T_TS_PAT_PROGRAM;

typedef struct t_ts_pat
{
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

    std::vector<T_TS_PAT_PROGRAM> pat_program;

    unsigned reserved_3             : 3;
    unsigned network_PID            : 13;
    unsigned CRC_32                 : 32;
} T_TS_PAT;

/**
 * PMT payload part
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

    std::vector<T_TS_PMT_STREAM> pmt_stream;

    unsigned reserved_5         : 3;
    unsigned reserved_6         : 4;
    unsigned CRC_32             : 32;
} T_TS_PMT;

T_TS_PAT g_ts_pat = {0};
std::vector<T_TS_PMT> g_ts_pmt;

HANDLE_STATUS parse_ts_header(unsigned char *const header_data, T_TS_PACKET_HEADER *ts_header)
{
    unsigned char *data = header_data;

    // T_TS_PACKET_HEADER ts_header = {0};
    // memset(&ts_header, 0x0, sizeof(ts_header));

    ts_header->sync_byte                    = data[0];
    if (0x47 != ts_header->sync_byte) 
    {
        return PARSE_TS_HEADER_FAIL;
    }
    ts_header->transport_error_indicator    = data[1] >> 7;
    ts_header->payload_unit_start_indicator = (data[1] >> 6) & 0x1;
    ts_header->transport_priority           = (data[1] >> 5) & 0x1;
    ts_header->ts_PID                       = ((data[1] & 0x1F) << 8) | data[2];
    ts_header->transport_scrambling_control = (data[3] >> 6) & 0x3;
    ts_header->adaptation_field_control     = (data[3] >> 4) & 0x3;
    ts_header->continuity_counter           = data[3] & 0x0F;

    // memcpy(ts_packet_header, &ts_header, sizeof(ts_header));

    return PARSE_SUCCESS;
}

HANDLE_STATUS parse_ts_pat(unsigned char *const pat_data, T_TS_PAT *pat)
{
    unsigned char *data = pat_data;

    pat->table_id = data[0];
    if (TS_PAT_TABLE_ID != pat->table_id)
        return PARSE_IS_NOT_PAT;

    pat->section_syntax_indicator   = data[1] >> 7;
    pat->zero                       = (data[1] >> 6) & 0x1;
    pat->reserved_1                 = (data[1] >> 4) & 0x3;
    pat->section_length             = ((data[1] & 0x0F) << 8) | data[2];
    pat->transport_stream_id        = (data[3] << 8) | data[4];
    pat->reserved_2                 = (data[5] >> 6) & 0x3;
    pat->version_number             = (data[5] >> 1) & 0x1F;
    pat->current_next_indicator     = data[5] & 0x1;
    pat->section_number             = data[6];
    pat->last_section_number        = data[7];

    // Get CRC_32
    int section_len = 0;
    section_len = pat->section_length + 3;
    pat->CRC_32 = ((data[section_len - 4] & 0x000000FF) << 24) 
                | ((data[section_len - 3] & 0x000000FF) << 16) 
                | ((data[section_len - 2] & 0x000000FF) << 8) 
                | (data[section_len - 1] & 0x000000FF);

    // Find all PMT PID and program_number
    for (int n = 0; n < pat->section_length - 12; n += 4)
    {
        unsigned program_num = (data[n + 8] << 8) | data[n + 9];
        pat->reserved_3 = data[n + 10] >> 5;

        pat->network_PID = 0x00;
        if (0x00 == program_num)
        {
            pat->network_PID = ((data[n + 10] & 0x1F) << 8) | data[n + 11];
        }
        else
        {
            T_TS_PAT_PROGRAM PAT_program;
            PAT_program.program_number = program_num;
            PAT_program.program_map_PID = ((data[n + 10] & 0x1F) << 8) | data[n + 11];
            pat->pat_program.push_back(PAT_program);
        }
    }

    return PARSE_SUCCESS;
}

HANDLE_STATUS parse_ts_pmt(unsigned char *const pmt_data, T_TS_PMT *pmt)
{
    unsigned char *data = pmt_data;

    pmt->table_id = data[0];
    if (TS_PMT_TABLE_ID != pmt->table_id)
        return PARSE_IS_NOT_PMT;

    pmt->section_syntax_indicator   = data[1] >> 7;
    pmt->zero                       = (data[1] >> 6) & 0x1;
    pmt->reserved_1                 = (data[1] >> 4) & 0x3;
    pmt->section_length             = ((data[1] & 0x0F) << 8) | data[2];
    pmt->program_number             = (data[3] << 8) | data[4];
    pmt->reserved_2                 = data[5] >> 6;
    pmt->version_number             = (data[5] >> 1) & 0x1F;
    pmt->current_next_indicator     = data[5] & 0x1;
    pmt->section_number             = data[6];
    pmt->last_section_number        = data[7];
    pmt->reserved_3                 = data[8] >> 5;
    pmt->PCR_PID                    = ((data[8] << 8) | data[9]) & 0x1FFF;
    pmt->reserved_4                 = data[10] >> 4;
    pmt->program_info_length        = ((data[10] & 0x0F) << 8) | data[11];

    // Get CRC_32
    int section_len = 0;
    section_len = pmt->section_length + 3;
    pmt->CRC_32 = ((data[section_len - 4] & 0x000000FF) << 24) 
                | ((data[section_len - 3] & 0x000000FF) << 16) 
                | ((data[section_len - 2] & 0x000000FF) << 8) 
                | (data[section_len - 1] & 0x000000FF);

    int pos = 12; // table_id ~ program_info_length : 12bytes
    // program info descriptor
    if (pmt->program_info_length != 0)
    {
        pos += pmt->program_info_length;
    }
    // Get stream type and PID
    // pmt->section_length + 2 (2: section_length 之前有 2bytes)
    for (; pos <= (pmt->section_length + 2) - 4;)
    {
        T_TS_PMT_STREAM PMT_stream;
        PMT_stream.stream_type = data[pos];
        pmt->reserved_5 = data[pos + 1] >> 5;
        PMT_stream.elementary_PID = ((data[pos + 1] & 0x1F) << 8) | data[pos + 2];
        pmt->reserved_6 = data[pos + 3] >> 4;
        PMT_stream.ES_info_length = ((data[pos + 3] & 0x0F) << 8) | data[pos + 4];
        
        PMT_stream.descriptor = 0x00;
        if (PMT_stream.ES_info_length != 0)
        {
            PMT_stream.descriptor = data[pos + 5];

            for (int len = 2; len <= PMT_stream.ES_info_length; ++len)
            {
                PMT_stream.descriptor = PMT_stream.descriptor << 8 | data[pos + 4 + len];
            }

            pos += PMT_stream.ES_info_length;
        }
        pos += 5;
        pmt->pmt_stream.push_back(PMT_stream);
    }

    pmt->pmt_is_find = 1;
    return PARSE_SUCCESS;
}

int main(int argc, char *argv[])
{
    if (2 != argc)
    {
        printf("Usage: %s **.ts\n", argv[0]);
        exit(1);
    }

    int i = 0, ret = 0;
    int pat_is_find = 0;
    int all_pmt_is_find = 0;

    unsigned char ts_packet[MAX_TS_PACKET_LEN] = {0};

    T_TS_PACKET_HEADER ts_packet_header = {0};

    FILE *fp = NULL;
    fp = fopen(argv[1], "rb");      // "rb": 打开一个二进制文件，文件必须存在，只允许读
    if (!fp)
    {
        printf("Open file[%s] error!\n", argv[1]);
        exit(1);
    }

    memset(&g_ts_pat, 0x0, sizeof(g_ts_pat));

    while (1)
    {
        memset(ts_packet, 0x0, sizeof(ts_packet));
        memset(&ts_packet_header, 0x0, sizeof(ts_packet_header));

        if (MAX_TS_PACKET_LEN != fread(ts_packet, 1, MAX_TS_PACKET_LEN, fp))
        {
            break;
        }

        if(PARSE_SUCCESS != parse_ts_header(ts_packet, &ts_packet_header))
        {
            printf("parse ts header failed! \n");
            exit(1);
        }

        if (TS_PAT_PID == ts_packet_header.ts_PID)
        {
            if (0 == pat_is_find)
            {
                if (1 == ts_packet_header.payload_unit_start_indicator)
                {
                    parse_ts_pat(&ts_packet[4 + 1], &g_ts_pat);
                }
                else
                {
                    parse_ts_pat(&ts_packet[4], &g_ts_pat);
                }

                if (0 == g_ts_pat.pat_program.size())
                {
                    continue;
                }
                else
                {
                    pat_is_find = 1;
                }
            }
            else
            {
                break;
            }
        }

        if ((1 == pat_is_find) && (1 != all_pmt_is_find))
        {
            std::vector<T_TS_PAT_PROGRAM>::iterator it;

            // printf("size of vector is %d \n", g_ts_pat.pat_program.size());
            for (it = g_ts_pat.pat_program.begin(); it != g_ts_pat.pat_program.end(); ++it)
            {
                // printf("it->program_map_PID is %0x, ", it->program_map_PID);
                // printf("ts_packet_header.ts_PID is %0x \n", ts_packet_header.ts_PID);
                if (it->program_map_PID == ts_packet_header.ts_PID)
                {
                    T_TS_PMT ts_pmt;
                    memset(&ts_pmt, 0x0, sizeof(ts_pmt));

                    if (1 == ts_packet_header.payload_unit_start_indicator)
                    {
                        parse_ts_pmt(&ts_packet[4 + 1], &ts_pmt);
                    }
                    else
                    {
                        parse_ts_pmt(&ts_packet[4], &ts_pmt);
                    }
                    g_ts_pmt.push_back(ts_pmt);
                }
            }
        }
    }

    for (int i = 0; i < g_ts_pmt.size(); ++i)
    {
        printf("g_ts_pmt[%d].program_number is 0x%x \n", i, g_ts_pmt[i].program_number);
    }

    fclose(fp);

    return 0;
}
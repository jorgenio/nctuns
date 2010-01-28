/*
 * Copyright (c) from 2000 to 2009
 * 
 * Network and System Laboratory 
 * Department of Computer Science 
 * College of Computer Science
 * National Chiao Tung University, Taiwan
 * All Rights Reserved.
 * 
 * This source code file is part of the NCTUns 6.0 network simulator.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation is hereby granted (excluding for commercial or
 * for-profit use), provided that both the copyright notice and this
 * permission notice appear in all copies of the software, derivative
 * works, or modified versions, and any portions thereof, and that
 * both notices appear in supporting documentation, and that credit
 * is given to National Chiao Tung University, Taiwan in all publications 
 * reporting on direct or indirect use of this code or its derivatives.
 *
 * National Chiao Tung University, Taiwan makes no representations 
 * about the suitability of this software for any purpose. It is provided 
 * "AS IS" without express or implied warranty.
 *
 * A Web site containing the latest NCTUns 6.0 network simulator software 
 * and its documentations is set up at http://NSL.csie.nctu.edu.tw/nctuns.html.
 *
 * Project Chief-Technology-Officer
 * 
 * Prof. Shie-Yuan Wang <shieyuan@csie.nctu.edu.tw>
 * National Chiao Tung University, Taiwan
 *
 * 09/01/2009
 */

#include <stdlib.h>
#include <assert.h>
#include <nctuns_api.h>
#include <gprs/mac/GPRS_bts_mac.h>
#include <gprs/mac/mac_shared_def.h>

#include "parser.h"
using namespace std;

BssDesList* GprsBtsMac::bsscfg_parser( u_int32_t nid , const char* filename ) {

    const char* case_dir = GetConfigFileDir();
    printf("Base Station Configuration Filename = %s%s \n", case_dir , filename );
    ASSERTION( filename , "bsscfg_parser: filename is NULL.\n" );

    int   size      = strlen(GetConfigFileDir()) + strlen(filename) + 2;
    FILE *infile    = NULL;
    char* buf = new char[size];
    bzero ( buf , size);
    strcpy( buf , case_dir );
    strcat( buf , filename );
    infile = fopen( buf , "r");

    delete buf;
    const int line_len = 200;
    buf = new char[line_len];
    bzero( buf, line_len );

    ASSERTION( infile , "bsscfg_parser(): Open BSS configuration file failed.\n" );

    long prev_state     = -1;

    BssDesList* bssdes_list = new BssDesList;
    BssDes*  tmp = NULL;

    while ( !feof(infile) ) {

        if ( !fgets( buf , size-1 , infile ) )
            continue;

        char* token = NULL;
        long  cnt = 0;
        while  (true) {

            if ( !cnt ) {
                token = strtok( buf , " \t\n;");
                ++cnt;
            }
            else
                token = strtok( NULL , " \t\n;");



            if (!token)
                break;

            #ifdef __PARSING_DEBUG
            printf(" token = %s \n" , token );
            #endif

            /* parameter settings for Cpu_sim */
            if ( !strncmp( token , "#" , 1 ) ) {
                bzero( buf , size );
                continue;
            }

            else if ( !strcmp( token , "BSS_DES_START") ) {

                state = BSS_DES_START;
            }

            else if ( !strcmp(token , "BSS_DES_END") ) {

                state = NORMAL;
            }

            /* if the local section is not mine, ignore all descriptions except the
             * declaration about BSS_DES.
             */

            else if ( state == NOT_MY_BSS_LOCAL_SECTION ) {
                bzero( buf , size );
                continue;
            }

            else if ( !strcmp(token,"LOCAL_SECTION") ) {

                if ( state != BSS_DES_START ) {

                    printf("bsscfg_parser(): Syntax Error: Miss 'BSS_DES_START' section indicator.\n");
                    exit(1);

                }

                /* Enter local base station declartion */
                prev_state  = state;
                state       = LOCAL_SECTION_START;

            }
            else if ( !strcmp(token, "END_LOCAL_SECTION" ) ) {

                state = prev_state;
            }
            else if ( !strcmp(token, "NEIGHBOR_SECTION" ) ) {

                if ( state != BSS_DES_START ) {

                    printf("bsscfg_parser(): Syntax Error: Miss 'BSS_DES_START' section indicator.\n");
                    exit(1);

                }

                prev_state  = state;
                state       = NEIGHBOR_SECTION_START;
                tmp         = new BssDes;
                ASSERTION(tmp,"bsscfg_parser(): Allocating a neighbor bss descriptor failed.\n");
                bssdes_list->insert_tail( tmp );

            }
            else if ( !strcmp(token, "END_NEIGHBOR_SECTION" ) ) {

                state = prev_state;

            }

            else {

                if ( state == BSS_DES_START ) {

                    if ( token ) {

                        printf("bsscfg_parser(): unknown symbol in BSS_DES section\n");
                        exit(1);

                    }
                }
                else if ( state == LOCAL_SECTION_START ) {

                    if ( !strcasecmp( token , "NID" ) ) {

                        char* next_token = strtok( NULL , " =\t\n;");
                        ASSERTION( next_token , "bsscfg_parser(): Miss the value of NID field in Local Section.\n");
                        ulong value = atol(next_token);

                        if ( value != nid ) {
                            state = NOT_MY_BSS_LOCAL_SECTION;
                            bzero( buf , size );
                            continue;
                        }

                        tmp             = new BssDes;
                        tmp->node_id    = nid;
                        ASSERTION(tmp,"bsscfg_parser(): Allocating a local bss descriptor failed.\n");
                        bssdes_list->insert_head( tmp );

                        bzero( buf , size );

                    }
                    else if ( !strcasecmp( token ,"BSIC" ) ) {

                        char* next_token = strtok( NULL , " =\t\n;");
                        ASSERTION( next_token , "bsscfg_parser(): Miss the value of BSIC field in Local Section.\n");
                        ulong value = atol(next_token);

                        BssDes* local_des = bssdes_list->get_head_elem();
                        ASSERTION( local_des , "bsscfg_parser(): local base station descriptor has not been allocated.\n");

                        if ( local_des->node_id != nid ) {

                            printf("bsscfg_parser(): the node id of the local descriptor is not the same as mine.\n");
                            exit(1);

                        }

                        local_des->bsic = value;

                        bzero( buf , size );
                    }
                    else if ( !strcasecmp(token, "RAI") ) {

                        char* next_token = strtok( NULL , " =\t\n;");
                        ASSERTION( next_token , "bsscfg_parser(): Miss the value of RAI field in Local Section.\n");
                        ulong value = atol(next_token);

                        BssDes* local_des = bssdes_list->get_head_elem();
                        ASSERTION( local_des , "bsscfg_parser(): local base station descriptor has not been allocated.\n");

                        if ( local_des->node_id != nid ) {

                            printf("bsscfg_parser(): the node id of the local descriptor is not the same as mine.\n");
                            exit(1);

                        }

                        local_des->rai = value;

                        bzero( buf , size );

                    }
                    else if ( !strcasecmp( token , "CHANNEL_RANGE") ) {

                        BssDes* local_des = bssdes_list->get_head_elem();
                        ASSERTION( local_des , "bsscfg_parser(): local base station descriptor has not been allocated.\n");

                        if ( local_des->node_id != nid ) {

                            printf("bsscfg_parser(): the node id of the local descriptor is not the same as mine.\n");
                            exit(1);

                        }

                        char* next_token = strtok( NULL , " =\t\n;");
                        ASSERTION( next_token , "bsscfg_parser(): Miss the start value of CHANNEL_RANGE field in Local Section.\n");
                        ulong s_val = atol(next_token);

                        next_token = strtok( NULL , " =\t\n;");
                        ASSERTION( next_token , "bsscfg_parser(): Miss the end value of CHANNEL_RANGE field in Local Section.\n");
                        ulong e_val = atol(next_token);

                        if ( e_val < s_val ) {

                            printf("bsscfg_parser(): Assertion failed. Cause: the end channel is less than the start channel.\n");
                            exit(1);

                        }

                        local_des->start_ch = s_val;
                        local_des->end_ch   = e_val;
                        tmp->bcch_no        = CORRESPONDING_DOWNLINK_CH(s_val);
                        printf("BTS MAC [%d]: GprsBtsMac::bsscfg_parser(): The broadcast channel number is set to %ld.\n", get_nid(), tmp->bcch_no);

                        bzero( buf , size );
                    }
                    else {

                        cout << "undefined variable name = " << token << "in neighbor section" << endl;
                        exit(1);

                    }



                }
                else if ( state == NEIGHBOR_SECTION_START ) {

                    if ( !strcasecmp( token , "NID" ) ) {

                        char* next_token = strtok( NULL , " =\t\n;");
                        ASSERTION( next_token , "bsscfg_parser(): Miss the value of NID field in Neighbor Section.\n");
                        ulong value = atol(next_token);
                        tmp->node_id = value;
                        bzero( buf , size );
                    }
                    else if ( !strcasecmp( token ,"BSIC" ) ) {

                        char* next_token = strtok( NULL , " =\t\n;");
                        ASSERTION( next_token , "bsscfg_parser(): Miss the value of BSIC field in Neighbor Section.\n");
                        ulong value = atol(next_token);
                        tmp->bsic = value;
                        bzero( buf , size );
                    }
                    else if ( !strcasecmp(token, "RAI") ) {
                        char* next_token = strtok( NULL , " =\t\n;");
                        ASSERTION( next_token , "bsscfg_parser(): Miss the value of RAI field in Neighbor Section.\n");
                        ulong value = atol(next_token);
                        tmp->rai = value;
                        bzero( buf , size );
                    }
                    else if ( !strcasecmp( token , "CHANNEL_RANGE") ) {

                        char* next_token = strtok( NULL , " =\t\n;");
                        ASSERTION(next_token , "bsscfg_parser(): Miss the first value of CHANNEL_RANGE field in Neighbor Section.\n");
                        ulong s_val = atol(next_token);

                        next_token = strtok( NULL , " =\t\n;");
                        ASSERTION(next_token , "bsscfg_parser(): Miss the end value of CHANNEL_RANGE field in Neighbor Section.\n");
                        ulong e_val = atol(next_token);

                        if ( e_val < s_val ) {

                            printf("bsscfg_parser(): Assertion failed. Cause: the end channel is less than the start channel.\n");
                            exit(1);

                        }

                        tmp->start_ch   = s_val;
                        tmp->end_ch     = e_val;
                        tmp->bcch_no    = CORRESPONDING_DOWNLINK_CH(s_val);
                        printf("tmp->bcch_no = %ld \n" , tmp->bcch_no);

                        bzero( buf , size );
                    }
                    else {

                        cout << "undefined variable name = " << token << "in neighbor section"  << endl;
                        exit(1);

                    }

                }
                else
                    cout << "Unidentified token = " << token << endl;
            }



        }

    }

    fclose(infile);
    return bssdes_list;
}

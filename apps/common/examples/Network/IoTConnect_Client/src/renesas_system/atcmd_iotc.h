//
// Copyright: Avnet 2024
// Created by E. Voirin on 8/21/24.
//



/**
 ****************************************************************************************
 * @brief NWICEXMSG AT command
 * 
 * Receives extended message send request, i.e. with key, type and value
 * Parameter count must be (multiples of 3) + 1
 ****************************************************************************************
 */
int iotc_at_nwicexmsg(int argc, char *argv[]);

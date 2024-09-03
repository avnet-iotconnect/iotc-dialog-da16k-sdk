//
// Copyright: Avnet 2024
// Created by E. Voirin on 8/21/24.
//

#ifndef _ATCMD_IOTC_H_
#define _ATCMD_IOTC_H_

/* This file declares IoTConnect-specific AT Command implementations. */

/* NWICCPID:        Set/Get IoTConnect DUID */
int iotc_at_nwicduid    (int argc, char *argv[]);
/* NWICCPID:        Set/Get IoTConnect CPID */
int iotc_at_nwiccpid    (int argc, char *argv[]);
/* NWICENV:         Set/Get IoTConnect Env */
int iotc_at_nwicenv     (int argc, char *argv[]);
/* NWICAT:          Set/Get Authentication Type */
int iotc_at_nwicat      (int argc, char *argv[]);
/* NWICCT:          Set/Get Connection Type */
int iotc_at_nwicct      (int argc, char *argv[]);
/* NWICSK:          Set/Get Symmetric Key */
int iotc_at_nwicsk      (int argc, char *argv[]);
/* NWICSETUP:       IoTConnect Connection Configuration Setup */
int iotc_at_nwicsetup   (int argc, char *argv[]);
/* NWICSTART:       IoTConnect Connection Start (requires prior setup call) */
int iotc_at_nwicstart   (int argc, char *argv[]);
/* NWICSTOP:        IoTConnect Connection Stop */
int iotc_at_nwicstop    (int argc, char *argv[]);
/* NWICRESET:       IoTConnect Connection Configuration reset. Disconnects if connected */
int iotc_at_nwicreset   (int argc, char *argv[]);
/* NWICMSG:         Message Send - with <key, value> tuples.
                    Parameter count must be (multiples of 2) + 1 */
int iotc_at_nwicmsg     (int argc, char *argv[]);
/* NWICEXMSG:       Extended Message Send - with <type, key, value> tuples.
                    Parameter count must be (multiples of 3) + 1 */
int iotc_at_nwicexmsg   (int argc, char *argv[]);
/* NWICVER:         Get IoTConnect AT Version */
int iotc_at_nwicver     (int argc, char *argv[]);
/* NWICGETCMD:      Get next command from command queue */
int iotc_at_nwicgetcmd  (int argc, char *argv[]);

#endif
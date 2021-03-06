/*
 * Copyright (c) 2011-2016, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * \file
 *         Border router configuration
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <err.h>
#include "contiki.h"
#include "br-config.h"

#ifdef __APPLE__
#ifndef B230400
#define B230400 230400
#endif
#ifndef B460800
#define B460800 460800
#endif
#ifndef B921600
#define B921600 921600
#endif
#endif /* __APPLE__ */

#ifdef ENC_DEV_CONF_SEND_DELAY
#define SEND_DELAY_DEFAULT ENC_DEV_CONF_SEND_DELAY
#else
#define SEND_DELAY_DEFAULT 0
#endif

uint8_t br_config_wait_for_address = 0;
int br_config_verbose = 0;
const char *br_config_ipaddr;
int br_config_flowcontrol = 0;
const char *br_config_siodev = NULL;
const char *br_config_host = NULL;
const char *br_config_port = NULL;
const char *br_config_run_command = NULL;
const char *ctrl_config_port = NULL;
const char *server_config_port = NULL;
char br_config_tundev[1024] = { "" };
uint16_t br_config_siodev_delay = SEND_DELAY_DEFAULT;
uint16_t br_config_basedelay = 0;
uint16_t br_config_unit_controller_port = 4444;
uint8_t br_config_is_slave = 0;

#ifndef BAUDRATE
#define BAUDRATE B460800
#endif
speed_t br_config_b_rate = BAUDRATE;

#define GET_OPT_OPTIONS "_?hB:H:D:Ls:t:v::d::l:a:p:SP:C:c:X:"
/*---------------------------------------------------------------------------*/
int
br_config_handle_arguments(int argc, char **argv)
{
  const char *prog;
  int c;
  int baudrate = -2;
  int tmp;

  br_config_verbose = 0;

  prog = argv[0];
  while((c = getopt(argc, argv, GET_OPT_OPTIONS)) != -1) {
    switch(c) {
    case '_':
      fprintf(stderr, "GETOPT:%s\n", GET_OPT_OPTIONS);
      exit(EXIT_SUCCESS);
      break;
    case 'B':
      baudrate = atoi(optarg);
      break;

    case 'H':
      br_config_flowcontrol = 1;
      break;

    case 'L':
      /**
       * The 'L' argument was unused and is now deprecated. Left with
       * warning for now to not affect the behaviour but should be
       * removed in long term.
       */
      fprintf(stderr, "  The -L argument is deprecated and should not be used!\n");
      break;

    case 's':
      if(strncmp("/dev/", optarg, 5) == 0) {
	br_config_siodev = optarg + 5;
      } else {
	br_config_siodev = optarg;
      }
      break;

    case 't':
      if(strncmp("/dev/", optarg, 5) == 0) {
	strncpy(br_config_tundev, optarg + 5, sizeof(br_config_tundev) - 1);
      } else {
	strncpy(br_config_tundev, optarg, sizeof(br_config_tundev) - 1);
      }
      break;

    case 'a':
      br_config_host = optarg;
      break;

    case 'p':
      br_config_port = optarg;
      break;

    case 'C':
      tmp = atoi(optarg);
      if(tmp <= 0 || tmp > 0xffff) {
        fprintf(stderr, "illegal unit controller port: %s\n", optarg);
      } else {
        br_config_unit_controller_port = tmp;
      }
      break;

    case 'c':
      ctrl_config_port = optarg;
      break;

    case 'P':
      server_config_port = optarg;
      break;

    case 'S':
      /* Start as slave */
      br_config_is_slave = 1;
      break;

    case 'd':
      br_config_basedelay = 10;
      if(optarg) br_config_basedelay = atoi(optarg);
      break;

    case 'l':
      if(optarg) br_config_siodev_delay = atoi(optarg);
      break;

    case 'v':
      br_config_verbose = 2;
      if(optarg) br_config_verbose = atoi(optarg);
      break;

    case 'X':
      br_config_run_command = optarg;
      break;

    case '?':
    case 'h':
    default:
fprintf(stderr,"usage:  %s [options] ipaddress\n", prog);
fprintf(stderr,"example: border-router.native -s ttyUSB1 aaaa::1/64\n");
fprintf(stderr,"Options are:\n");
fprintf(stderr," -B baudrate    9600,19200,38400,57600,115200,230400,460800,921600 (default 460800)\n");
fprintf(stderr," -H             Hardware CTS/RTS flow control (default disabled)\n");
fprintf(stderr," -s siodev      Serial device (default /dev/ttyUSB0)\n");
fprintf(stderr," -a host        Connect via TCP to server at <host>\n");
fprintf(stderr," -p port        Connect via TCP to server at <host>:<port>\n");
fprintf(stderr," -c port        Open UDP control at localhost:<port>\n");
fprintf(stderr," -t tundev      Name of interface (default tun0)\n");
fprintf(stderr," -X cmd         Run the command and then exit\n");
fprintf(stderr," -S             Start in slave mode\n");
fprintf(stderr," -v[level]      Verbosity level\n");
fprintf(stderr,"    -v0         No messages\n");
fprintf(stderr,"    -v1         Encapsulated SLIP debug messages (default)\n");
fprintf(stderr,"    -v2         Printable strings after they are received\n");
fprintf(stderr,"    -v3         Printable strings and SLIP packet notifications\n");
fprintf(stderr,"    -v4         All printable characters as they are received\n");
fprintf(stderr,"    -v5         All SLIP packets in hex\n");
fprintf(stderr,"    -v          Equivalent to -v3\n");
/* This delay seems to be not useful anymore - REMOVE? /JE */
fprintf(stderr," -d[basedelay]  Minimum delay between outgoing SLIP packets.\n");
fprintf(stderr,"                Actual delay is basedelay*(#6LowPAN fragments) milliseconds.\n");
fprintf(stderr,"                -d is equivalent to -d10.\n");
fprintf(stderr," -l[delay]      Minimum delay between outgoing 6LoWPAN fragments.\n");
fprintf(stderr,"                Default delay is %u milliseconds.\n", SEND_DELAY_DEFAULT);
fprintf(stderr,"                Actual delay is delay*(#6LoWPAN fragments) milliseconds.\n");
exit(EXIT_FAILURE);
      break;
    }
  }
  argc -= optind;
  argv += optind;

  //* If no address is supplied, wait for it to be set via OAM */
  if(argc == 0) {
      br_config_wait_for_address = 1;
  } else  if(argc == 1) {
      br_config_wait_for_address = 0;
      br_config_ipaddr = argv[0];
  } else {
    fprintf(stderr, "*** too many arguments\n");
    err(1, "usage: %s [-B baudrate] [-H] [-S] [-s siodev] [-t tundev] [-v verbosity] [-d delay] [-l delay] [-a serveraddress] [-p serverport] ipaddress", prog);
  }

  switch(baudrate) {
  case -2:
    break;			/* Use default. */
  case 9600:
    br_config_b_rate = B9600;
    break;
  case 19200:
    br_config_b_rate = B19200;
    break;
  case 38400:
    br_config_b_rate = B38400;
    break;
  case 57600:
    br_config_b_rate = B57600;
    break;
  case 115200:
    br_config_b_rate = B115200;
    break;
#ifdef B230400
  case 230400:
    br_config_b_rate = B230400;
    break;
#endif
#ifdef B460800
  case 460800:
    br_config_b_rate = B460800;
    break;
#endif
#ifdef B921600
  case 921600:
    br_config_b_rate = B921600;
    break;
#endif
  default:
    err(1, "unknown baudrate %d (all baudrates are not supported on all systems)", baudrate);
    break;
  }

  if(*br_config_tundev == '\0') {
    /* Use default. */
    strcpy(br_config_tundev, "tun0");
  }
  return 1;
}
/*---------------------------------------------------------------------------*/

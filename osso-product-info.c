/**
  @file osso-product-info.c

  Copyright (C) 2013 Ivaylo Dimitrov

  @author Ivaylo Dimitrov <freemangordon@abv.bg>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  version 2 as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "libossoproductinfo.h"

static void
usage(void)
{
  fprintf(stderr,
          "Usage: <program> [OPTIONS]\n"
          "Options:\n"
          "  --query, -q <arg>\t\tquery <arg> variable\n"
          "  --set, -s <arg>\t\tset <arg> variable\n"
          );
}

static struct option
long_options[] =
{
    {"query", required_argument, NULL, 'q'},
    {"set", required_argument, NULL, 's'},
    {NULL, 0, NULL, 0}
};

int
main(int argc, char *argv[])
{
  int opt;
  char* p;

  while ((opt = getopt_long(argc, argv, "q:s:", long_options, NULL)) != -1)
  {
    switch (opt)
    {
      case 'q':
      {
        osso_product_info_code code = osso_get_product_info_idx(optarg);
        if(((int)code) != -1)
        {
          char* p = osso_get_product_info(code);
          if (!p) {
            fprintf(stderr, "Error getting variable %s value\n", optarg);
            exit(EXIT_FAILURE);
          }
          printf("%s\n", p);
          free(p);
        }
        else
        {
          fprintf(stderr, "Error getting variable %s value\n", optarg);
          exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
      }
      case 's':
      {
        osso_product_info_code code;
        char* p = strchr(optarg, '=');

        if(!p || *(p+1) == 0)
          goto bad_set_arg;

        *p = 0;
        code = osso_get_product_info_idx(optarg);
        if((int)code == -1)
          goto bad_set_arg;

        if(osso_set_product_info(code, p) < 0)
          exit(EXIT_FAILURE);

        exit(EXIT_SUCCESS);
bad_set_arg:
        fprintf(stderr, "Wrong argument, should be VAR=<value>\n");
        exit(EXIT_FAILURE);
      }
      default: /* '?' */
        usage();
        exit(EXIT_FAILURE);
    }
  }

  if (optind == argc)
  {
    int code;
    for(code=0; code <= OSSO_VERSION; code++)
    {
      printf("%s='%s'\n", osso_get_product_info_str(code), p = osso_get_product_info(code));
      free(p);
    }
  }

  exit(EXIT_SUCCESS);
  }

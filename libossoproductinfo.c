/**
  @file libossoproductinfo.c

  Copyright (C) 2013 Ivaylo Dimitrov

  @author Ivaylo Dimitrov <freemangordon@abv.bg>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License version 2.1 as
  published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
  General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <cal.h>
#include <sysinfo.h>

#include "libossoproductinfo.h"

#define PRINT_ERROR(msg) \
  fwrite("ERROR: " msg "\n", 1, sizeof("ERROR: " msg "\n") - 1, stderr)

#define PRINT_ERROR_ARGS(msg, ...) \
  fprintf(stderr, "ERROR: " msg "\n", __VA_ARGS__)

struct osso_product_info {
  int (*init)(struct osso_product_info *);
  char* (*get)(struct osso_product_info *, const char* key);
  void (*fini)(struct osso_product_info *);
  void* priv;
};

static int
product_info_init_sb(struct osso_product_info* pi)
{
  struct stat st;
  return stat("/targets/links/scratchbox.config", &st);
}

static const char*
product_info_sb[] =
{
  "etsi/eu",
  "English, Dutch",
  "Britain",
  "SB",
  "SB_FREMANTLE_1.2009.33-2_PR_MR0",
};

static struct{
  osso_product_info_code code;
  const char* name;
}
product_info_keys[] =
{
  {OSSO_PRODUCT_WLAN_CHANNEL, "/certs/ccc/pp/wlan-channel"},
  {OSSO_PRODUCT_KEYBOARD,     "/certs/ccc/pp/keyboard-layout"},
  {OSSO_PRODUCT_REGION,       "/certs/ccc/pp/content-region"},
  {OSSO_PRODUCT_HARDWARE,     "/component/product"},
  {OSSO_VERSION,              "/device/sw-release-ver"},
};

static const char*
sw_info[][4] = {
  {"2007SE", "OS 2007", "Internet Tablet OS: maemo Linux based OS2007", "OS 2007"},
  {"2008SE", "OS 2008", "Internet Tablet OS: maemo Linux based OS2008", "OS 2008"},
  {"DIABLO", "OS 2008", "Internet Tablet OS: maemo Linux based OS2008", "OS 2008"},
  {"2009SE", "Maemo 5", "Maemo 5", "Maemo 5"},
  {"FREMANTLE", "Maemo 5", "Maemo 5", "Maemo 5"},
  {"DEFAULT", "Maemo SDK", "Maemo SDK", "Maemo SDK"},
  {NULL,NULL,NULL,NULL},
};

static const char*
hw_info[][4] = {
  {"RX-34", "N800", "Nokia N800 Internet Tablet", "Nokia N800"},
  {"RX-44", "N810", "Nokia N810 Internet Tablet", "Nokia N810"},
  {"RX-48", "N810 WiMAX", "Nokia N810 Internet Tablet WiMAX Edition", "Nokia N810 WiMAX"},
  {"RX-51", "N900", "Nokia N900", "Nokia N900"},
  {"SB", "SB", "Scratchbox", "Scratchbox"},
  {NULL,NULL,NULL,NULL},
};

static char*
product_info_get_sb(struct osso_product_info* pi,
                    const char* key)
{
  if(!strcmp("/certs/ccc/pp/wlan-channel", key))
    return strdup(product_info_sb[0]);
  if(!strcmp("/certs/ccc/pp/keyboard-layout", key))
    return strdup(product_info_sb[1]);
  if(!strcmp("/certs/ccc/pp/content-region", key))
    return strdup(product_info_sb[2]);
  if(!strcmp("/component/product", key))
    return strdup(product_info_sb[3]);
  if(!strcmp("/device/sw-release-ver", key))
    return strdup(product_info_sb[4]);

  return NULL;
}

static void
product_info_fini_sb(struct osso_product_info *pi)
{
}

static int
product_info_init_dbus(struct osso_product_info *pi)
{
  DBusConnection *dbus, **priv;
  DBusError error = DBUS_ERROR_INIT;
  int rv = -1;

  priv = malloc(sizeof(DBusConnection **));

  if(priv)
  {
    dbus = dbus_bus_get_private(DBUS_BUS_SYSTEM, &error);
    if(dbus)
    {
      *priv = dbus;
      pi->priv = priv;
      rv = 0;
    }
    else
      free(priv);
  }
  else
    PRINT_ERROR("No memory");

  return rv;
}

static char *
product_info_get_dbus(struct osso_product_info *pi,
                      const char *key)
{
  DBusConnection *dbus;
  DBusMessage *msg;
  DBusMessage *reply;
  DBusMessageIter array_iter;
  DBusMessageIter iter;
  DBusError error = DBUS_ERROR_INIT;
  int len = 0;
  void *value = NULL;
  char *rv = NULL;

  dbus = *(DBusConnection **)pi->priv;

  msg = dbus_message_new_method_call(
          "com.nokia.SystemInfo",
          "/com/nokia/SystemInfo",
          "com.nokia.SystemInfo",
          "GetConfigValue");
  if(!msg)
  {
    PRINT_ERROR("Could not allocate dbus message");
    goto out;
  }

  if(!dbus_message_append_args(msg,
                               DBUS_TYPE_STRING,
                               &key,
                               NULL))
  {
    PRINT_ERROR_ARGS("Could not add argument %s to dbus message", key);
    dbus_message_unref(msg);
    goto out;
  }

  reply = dbus_connection_send_with_reply_and_block(dbus,
                                                    msg,
                                                    10000,
                                                    &error);
  dbus_message_unref(msg);

  if(!reply)
  {
    PRINT_ERROR_ARGS("D-Bus query failed: %s (%s)", error.name, error.message);
    goto out;
  }

  if(dbus_message_get_type(reply) != DBUS_MESSAGE_TYPE_METHOD_RETURN)
    PRINT_ERROR("Got wrong type dbus reply");
  else
  {
    if (!dbus_message_iter_init(reply, &iter))
        PRINT_ERROR("Could not init iterator");
    else
    {
      if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY)
         PRINT_ERROR("Got non-array type from D-Bus");
      else
      {
        dbus_message_iter_recurse(&iter, &array_iter);
        dbus_message_iter_get_fixed_array(&array_iter, &value, &len);

        rv = malloc(len + 1);

        if(rv)
        {
          memcpy(rv, value, len);
          rv[len] = 0;
        }
        else
          PRINT_ERROR("No memory");
      }
    }
  }

  dbus_message_unref(reply);


out:
  return rv;
}

static void
product_info_fini_dbus(struct osso_product_info *pi)
{
  DBusConnection **dbus;

  dbus = (DBusConnection **)pi->priv;
  if(dbus)
  {
    dbus_connection_close(*dbus);
    dbus_connection_unref(*dbus);
    free(dbus);
    pi->priv = NULL;
  }
}

static int
product_info_init_sysinfo(struct osso_product_info *pi)
{
  int rv = -1;
  struct system_config *sc;

  if(sysinfo_init(&sc) < 0)
    PRINT_ERROR("Could not init libsysinfo");
  else
  {
    pi->priv = sc;
    rv = 0;
  }
  return rv;
}

void
finish_sysinfo(struct osso_product_info *pi)
{
}

static char*
product_info_get_sysinfo(struct osso_product_info *pi, const char *key)
{
  char *rv = NULL;
  unsigned long len;
  uint8_t *val;

  if(sysinfo_get_value((struct system_config *)pi->priv, key, &val, &len) >= 0)
  {
    rv = (char *)realloc(val, len + 1);

    if(rv)
      rv[len] = 0;
    else
    {
      PRINT_ERROR("No memory for sysinfo value");
      free(val);
    }
  }
  return rv;
}

static void
product_info_fini_sysinfo(struct osso_product_info *pi)
{
  if(pi->priv)
  {
    sysinfo_finish((struct system_config *)pi->priv);
    pi->priv = NULL;
  }
}

static struct osso_product_info
osso_product_info[] =
{
  {product_info_init_sb, product_info_get_sb, product_info_fini_sb, NULL},
  {product_info_init_dbus, product_info_get_dbus, product_info_fini_dbus, NULL},
  {product_info_init_sysinfo, product_info_get_sysinfo, product_info_fini_sysinfo, NULL},
};

static pthread_mutex_t product_info_lock;
static int product_info_inited = 0;
static struct osso_product_info *product_info = NULL;

struct osso_product_info*
init_sysinfo()
{
  struct osso_product_info *pi;
  pthread_mutex_lock(&product_info_lock);

  if(product_info_inited)
    pi = product_info;
  else
  {
    int i=0;

    for(;;)
    {
      pi = &osso_product_info[i];
      if(!pi->init(pi))
        break;

      if(i++ == 3)
      {
        PRINT_ERROR("Could not find sysinfo source");
        pi = NULL;
        goto out;
      }
    }

    product_info = pi;
    product_info_inited = 1;
  }

out:
  pthread_mutex_unlock((pthread_mutex_t *)&product_info_lock);
  return pi;
}

static char*
get_product_info(osso_product_info_code code,
                       struct osso_product_info *pi)
{
  size_t i;
  char* rv = NULL;
  const char* key = NULL;

  for(i = 0; i < sizeof(product_info_keys) / sizeof(product_info_keys[0]); i++)
  {
    if(code == product_info_keys[i].code)
    {
      key = product_info_keys[i].name;
      break;
    }
  }

  if (!key)
    PRINT_ERROR_ARGS("Could not find key for id %d", code);
  else
  {
    rv = pi->get(pi, product_info_keys[i].name);

    if(rv && !*rv)
    {
      free(rv);
      rv = NULL;
    }
  }

  if(!rv)
    PRINT_ERROR_ARGS("Could not find value for key %s", key);

  return rv;
}

int
osso_set_product_info(osso_product_info_code code,
                      const char *value)
{
  int rv;
  struct cal *cal;

  if(code != OSSO_VERSION)
  {
    PRINT_ERROR("Can set only OSSO_VERSION");
    return -1;
  }

  if((rv = cal_init(&cal) < 0))
    PRINT_ERROR("Could not init cal");
  else
  {
    rv = cal_write_block(cal,
                         "sw-release-ver",
                         value,
                         strlen(value),
                         CAL_FLAG_USER);

    if(rv < 0)
      PRINT_ERROR("cal block write error");

    cal_finish(cal);
  }

  return rv;
}

static const char **
get_product_info_details(const char* sw_info[][4],
                         const char* product)
{
  const char** rv = NULL;

  if(sw_info && product)
  {
    int i=0;
    while(sw_info[i][0][0])
    {
      if(!strcmp(sw_info[i][0], product))
      {
        rv = sw_info[i];
        break;
      }
      i++;
    }
  }

  return rv;
}

char*
osso_get_product_info(osso_product_info_code code)
{
  char* rv = NULL;
  struct osso_product_info *pi = init_sysinfo();

  if(!pi)
    return rv;

  switch (code)
  {
    case OSSO_PRODUCT_WLAN_CHANNEL:
    case OSSO_PRODUCT_KEYBOARD:
    case OSSO_PRODUCT_REGION:
    {
      rv = get_product_info(code, pi);
      break;
    }
    case OSSO_PRODUCT_RELEASE_NAME:
    case OSSO_PRODUCT_RELEASE_FULL_NAME:
    case OSSO_PRODUCT_RELEASE_VERSION:
    case OSSO_VERSION:
    {
      char* info = get_product_info(OSSO_VERSION, pi);

      if(info)
      {
        char* tmp[16] = {0,};
        const char** product;
        size_t i;
        int len = strlen(info);
        char *p = malloc(len + 1);

        memcpy(p, info, len);
        p[len] = 0;
        free(info);

        if(code == OSSO_VERSION)
        {
          rv = p;
          break;
        }

        for(i = 0; i < sizeof(tmp) / sizeof(tmp[0]); i++)
        {
          tmp[i] = p;
          while(*p && *p != '_')
            p++;
          if(!*p)
            break;
          *p = 0;
          p++;
        }

        if(!*tmp[1] || !*tmp[2])
        {
          free(tmp[0]);
          PRINT_ERROR_ARGS("Unsupported sw version string: %s", info);
          break;
        }

        product = get_product_info_details(sw_info, tmp[1]);

        if(!product)
        {
          PRINT_ERROR_ARGS("Unknown software: %s\n", tmp[1]);
        }
        else
        {
          if(code == OSSO_PRODUCT_RELEASE_NAME)
            rv = strdup(product[1]);
          else if(code == OSSO_PRODUCT_RELEASE_VERSION)
            rv = strdup(tmp[2]);
          else if(code == OSSO_PRODUCT_RELEASE_FULL_NAME)
            rv = strdup(product[3]);
        }
        free(tmp[0]);
      }
      break;
    }
    case OSSO_PRODUCT_HARDWARE:
    case OSSO_PRODUCT_NAME:
    case OSSO_PRODUCT_FULL_NAME:
    case OSSO_PRODUCT_SHORT_NAME:
    {
      char* info = get_product_info(OSSO_PRODUCT_HARDWARE, pi);

      if(info)
      {
        const char** product = get_product_info_details(hw_info, info);

        if(product)
        {
          if(code == OSSO_PRODUCT_HARDWARE)
            rv = strdup(product[0]);
          else if(code == OSSO_PRODUCT_NAME)
            rv = strdup(product[1]);
          else if(code == OSSO_PRODUCT_FULL_NAME)
            rv = strdup(product[2]);
          else if(code == OSSO_PRODUCT_SHORT_NAME)
            rv = strdup(product[3]);
        }
        else
          PRINT_ERROR_ARGS("Unknown product: %s", info);

        free(info);
      }
      break;
    }
  }

  finish_sysinfo(pi);

  return rv;
}

static const char* product_info_str[] = {
  "OSSO_PRODUCT_HARDWARE",
  "OSSO_PRODUCT_NAME",
  "OSSO_PRODUCT_FULL_NAME",
  "OSSO_PRODUCT_RELEASE_NAME",
  "OSSO_PRODUCT_RELEASE_FULL_NAME",
  "OSSO_PRODUCT_RELEASE_VERSION",
  "OSSO_PRODUCT_WLAN_CHANNEL",
  "OSSO_PRODUCT_KEYBOARD",
  "OSSO_PRODUCT_REGION",
  "OSSO_PRODUCT_SHORT_NAME",
  "OSSO_VERSION",
};

osso_product_info_code
osso_get_product_info_idx(const char *code)
{
  osso_product_info_code rv = -1;
  size_t i;

  for(i = 0; i < sizeof(product_info_str) / sizeof(product_info_str[0]); i++)
    if(!strcmp(code, product_info_str[i]))
    {
      rv = i;
      break;
    }

  return rv;
}

const char *
osso_get_product_info_str(osso_product_info_code idx)
{
  if(idx > OSSO_VERSION)
    return NULL;
  else
    return product_info_str[idx];
}

#if 0
int main()
{
  int code;
  char* p;

  printf("Testing ...\n");
  for(code=0; code <= OSSO_VERSION; code++)
  {
    printf("%d\t%s\t\"%s\"\n",
           osso_get_product_info_idx(osso_get_product_info_str(code)),
           osso_get_product_info_str(code),
           p = osso_get_product_info(code));
    free(p);
  }
}
#endif

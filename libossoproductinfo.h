/**
  @file libossoproductinfo.h

  Copyright (C) 2013 Jonathan Wilson
  Copyright (C) 2013 Ivaylo Dimitrov <freemangordon@abv.bg>

  @author Jonathan wilson <jfwfreo@tpgi.com.au>

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
#ifndef OSSOPRODUCTINFO_H
#define OSSOPRODUCTINFO_H
typedef enum {
	OSSO_PRODUCT_HARDWARE,
	OSSO_PRODUCT_NAME,
	OSSO_PRODUCT_FULL_NAME,
	OSSO_PRODUCT_RELEASE_NAME,
	OSSO_PRODUCT_RELEASE_FULL_NAME,
	OSSO_PRODUCT_RELEASE_VERSION,
	OSSO_PRODUCT_WLAN_CHANNEL,
	OSSO_PRODUCT_KEYBOARD,
	OSSO_PRODUCT_REGION,
	OSSO_PRODUCT_SHORT_NAME,
	OSSO_VERSION,
} osso_product_info_code;

/* The result should be free()-ed */
char *osso_get_product_info(osso_product_info_code code);

osso_product_info_code osso_get_product_info_idx(const char* code);

const char *osso_get_product_info_str(osso_product_info_code code);

int osso_set_product_info(osso_product_info_code code, const char *value);
#endif

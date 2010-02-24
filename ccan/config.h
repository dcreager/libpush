/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef PUSH_CONFIG_H
#define PUSH_CONFIG_H

/**
 * @file
 *
 * This file defines the configuration macros that the CCAN talloc and
 * typesafe_cb libraries expect.
 *
 * @todo We should update these so that they're automatically
 * calculated by scons.
 */

#define HAVE_ATTRIBUTE_PRINTF 1
#define HAVE_BUILTIN_CHOOSE_EXPR 1
#define HAVE_BUILTIN_EXPECT 1
#define HAVE_BUILTIN_TYPES_COMPATIBLE 1
#define HAVE_TYPEOF 1

#endif  /* PUSH_CONFIG_H */

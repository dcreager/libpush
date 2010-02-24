/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef PUSH_PROTOBUF_MESSAGE_H
#define PUSH_PROTOBUF_MESSAGE_H

#include <stdint.h>

#include <hwm-buffer.h>

#include <push.h>
#include <push/protobuf/basics.h>
#include <push/protobuf/field-map.h>



/**
 * Create a new callback for reading a Protocol Buffer message.  The
 * new callback will use the field callbacks in field_map to read the
 * fields of the message.
 */

push_callback_t *
push_protobuf_message_new(const char *name,
                          void *parent,
                          push_parser_t *parser,
                          push_protobuf_field_map_t *field_map);


#endif  /* PUSH_PROTOBUF_MESSAGE_H */

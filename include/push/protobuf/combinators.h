/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef PUSH_PROTOBUF_COMBINATORS_H
#define PUSH_PROTOBUF_COMBINATORS_H

#include <push/basics.h>


/**
 * Create a new callback that wraps another callback, ensuring that no
 * more than a certain number of bytes are passed to the wrapped
 * callback.  This works just like the push_dynamic_max_bytes_new
 * combinator, except that the threshold is read in as a
 * varint-encoded integer from the parse stream.
 */

push_callback_t *
push_protobuf_varint_prefixed_new(push_callback_t *wrapped);


#endif  /* PUSH_PROTOBUF_COMBINATORS_H */

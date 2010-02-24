/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <push/basics.h>
#include <push/combinators.h>
#include <push/primitives.h>
#include <push/talloc.h>

#include <push/protobuf/primitives.h>


push_callback_t *
push_protobuf_skip_length_prefixed_new(const char *name,
                                       void *parent,
                                       push_parser_t *parser)
{
    void  *context;
    push_callback_t  *read_size = NULL;
    push_callback_t  *skip = NULL;
    push_callback_t  *compose = NULL;

    /*
     * Create a memory context for the objects we're about to create.
     */

    context = push_talloc_new(parent);
    if (context == NULL) return NULL;

    /*
     * Create the callbacks.
     */

    if (name == NULL) name = "pb-skip-lp";

    read_size = push_protobuf_varint_size_new
        (push_talloc_asprintf(context, "%s.size", name),
         context, parser);
    skip = push_skip_new
        (push_talloc_asprintf(context, "%s.skip", name),
         context, parser);
    compose = push_compose_new
        (push_talloc_asprintf(context, "%s.compose", name),
         context, parser, read_size, skip);

    /*
     * Because of NULL propagation, we only have to check the last
     * result to see if everything was created okay.
     */

    if (compose == NULL) goto error;
    return compose;

  error:
    /*
     * Before returning, free any objects we created before the error.
     */

    push_talloc_free(context);
    return NULL;
}

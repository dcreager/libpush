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
#include <push/tuples.h>

#include <push/protobuf/primitives.h>


push_callback_t *
push_protobuf_varint_prefixed_new(const char *name,
                                  void *parent,
                                  push_parser_t *parser,
                                  push_callback_t *wrapped)
{
    void  *context;
    push_callback_t  *dup;
    push_callback_t  *size;
    push_callback_t  *first;
    push_callback_t  *max_bytes;
    push_callback_t  *compose1;
    push_callback_t  *compose2;

    /*
     * If the wrapped callback is NULL, return NULL ourselves.
     */

    if (wrapped == NULL)
        return NULL;

    /*
     * Create a memory context for the objects we're about to create.
     */

    context = push_talloc_new(parent);
    if (context == NULL) return NULL;

    /*
     * Then create the callbacks.
     */

    if (name == NULL) name = "varint-prefixed";

    dup = push_dup_new
        (push_talloc_asprintf(context, "%s.dup", name),
         context, parser);
    size = push_protobuf_varint_size_new
        (push_talloc_asprintf(context, "%s.size", name),
         context, parser);
    first = push_first_new
        (push_talloc_asprintf(context, "%s.first", name),
         context, parser, size);
    compose1 = push_compose_new
        (push_talloc_asprintf(context, "%s.compose1", name),
         context, parser, dup, first);
    max_bytes = push_dynamic_max_bytes_new
        (push_talloc_asprintf(context, "%s.max", name),
         context, parser, wrapped);
    compose2 = push_compose_new
        (push_talloc_asprintf(context, "%s.compose2", name),
         context, parser, compose1, max_bytes);

    /*
     * Because of NULL propagation, we only have to check the last
     * result to see if everything was created okay.
     */

    if (compose2 == NULL) goto error;
    return compose2;

  error:
    /*
     * Before returning, free any objects we created before the error.
     */

    push_talloc_free(context);
    return NULL;
}

/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <push/talloc.h>
#include <push/tuples.h>


push_tuple_t *
push_tuple_new(void *parent, size_t size)
{
    push_tuple_t  *tuple;

    tuple = push_talloc_zero_size(parent,
                                  sizeof(push_tuple_t) +
                                  size * sizeof(void *));

    if (tuple == NULL)
        return NULL;

    tuple->size = size;

    return tuple;
}


void
push_tuple_free(push_tuple_t *tuple)
{
    push_talloc_free(tuple);
}

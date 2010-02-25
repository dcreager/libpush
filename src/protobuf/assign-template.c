/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include <push/basics.h>
#include <push/combinators.h>
#include <push/pure.h>
#include <push/primitives.h>
#include <push/talloc.h>

#include <push/protobuf/basics.h>
#include <push/protobuf/field-map.h>
#include <push/protobuf/primitives.h>


/**
 * @file
 *
 * This file isn't compiled directly.  Instead, the other assign-*
 * files in this directory set some macros and then include this file.
 * This ensures that all of the assign callbacks are implemented in a
 * consistent fashion.
 *
 * Expected macros:
 *
 *   - PRETTY_NAME
 *   - PARSED_T
 *   - PRIuPARSED
 *   - DEST_T
 *   - PROTOBUF_TYPE
 *   - TAG_TYPE
 */


static bool
assign_func(DEST_T *dest, PARSED_T *input, PARSED_T **output)
{
    PUSH_DEBUG_MSG(PRETTY_STR ": Activating.  Got value "
                   "%"PRIuPARSED".\n",
                   *input);

    PUSH_DEBUG_MSG(PRETTY_STR ": Assigning value to %p.\n",
                   dest);
    *dest = *input;

    *output = input;
    return true;
}


push_define_pure_callback(assign_new, assign_func, PRETTY_STR,
                          PARSED_T, PARSED_T, DEST_T);


bool
PUSH_PROTOBUF_ASSIGN(const char *message_name,
                     const char *field_name,
                     void *parent,
                     push_parser_t *parser,
                     push_protobuf_field_map_t *field_map,
                     push_protobuf_tag_number_t field_number,
                     DEST_T *dest)
{
    void  *context;
    const char  *full_field_name;
    push_callback_t  *value;
    push_callback_t  *assign;
    push_callback_t  *field;

    /*
     * If the field map is NULL, return false.
     */

    if (field_map == NULL)
        return false;

    /*
     * Create a memory context for the objects we're about to create.
     */

    context = push_talloc_new(parent);
    if (context == NULL) return NULL;

    /*
     * Create the callbacks.
     */

    if (message_name == NULL) message_name = "message";
    if (field_name == NULL) field_name = ".assign";

    full_field_name =
        push_talloc_asprintf(context, "%s.%s",
                             message_name, field_name);

    value = VALUE_CALLBACK_NEW
        (push_talloc_asprintf(context, "%s." VALUE_STR,
                              full_field_name),
         context, parser);
    assign = assign_new
        (push_talloc_asprintf(context, "%s." PRETTY_STR,
                              full_field_name),
         context, parser,
         dest);
    field = push_compose_new
        (push_talloc_asprintf(context, "%s.compose",
                              full_field_name),
         context, parser,
         value, assign);

    /*
     * Because of NULL propagation, we only have to check the last
     * result to see if everything was created okay.
     */

    if (field == NULL) goto error;

    /*
     * Try to add the new field.  If we can't, free the field before
     * returning.
     */

    if (!push_protobuf_field_map_add_field
        (full_field_name, parser,
         field_map, field_number, TAG_TYPE, field))
    {
        goto error;
    }

    return true;

  error:
    /*
     * Before returning, free any objects we created before the error.
     */

    push_talloc_free(context);
    return false;
}

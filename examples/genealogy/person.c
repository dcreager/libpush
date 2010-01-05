/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <stdbool.h>
#include <string.h>

#include <push/protobuf.h>

#include <person.h>


bool
person_eq(person_t *person1, person_t *person2)
{
    if (person1 == person2)
        return true;

    return
        (person1->id == person2->id) &&
        (strcmp(person1->name, person2->name) == 0) &&
        (person1->mother == person2->mother) &&
        (person1->father == person2->father) &&
        (person1->dob == person2->dob);
}


#define CHECK(call)                             \
    if (!(call))                                \
    {                                           \
        goto error;                             \
    }

push_protobuf_message_t *
create_person_parser(person_t *person)
{
    push_protobuf_message_t  *result =
        push_protobuf_message_new();

    if (result == NULL) goto error;

    CHECK(push_protobuf_assign_uint32(result, 1, &person->id));
    CHECK(push_protobuf_malloc_str(result, 2, &person->name));
    CHECK(push_protobuf_assign_uint32(result, 3, &person->mother));
    CHECK(push_protobuf_assign_uint32(result, 4, &person->father));
    CHECK(push_protobuf_assign_uint64(result, 5, &person->dob));

    return result;

  error:
    if (result != NULL)
        push_callback_free(&result->base);

    return NULL;
}

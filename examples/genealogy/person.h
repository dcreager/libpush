/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef PUSH_PROTOBUF_EXAMPLE_PERSON_H
#define PUSH_PROTOBUF_EXAMPLE_PERSON_H

#include <stdbool.h>
#include <stdint.h>

#include <push/protobuf.h>

typedef uint32_t  person_id_t;
typedef uint64_t  date_t;

typedef struct _person
{
    person_id_t  id;
    char  *name;
    person_id_t  mother;
    person_id_t  father;
    date_t  dob;
} person_t;


bool
person_eq(person_t *person1, person_t *person2);


/**
 * Create a protobuf message parser that reads a Person message into
 * the given person_t object.
 */

push_protobuf_message_t *
create_person_parser(person_t *person);


#endif  /* PUSH_PROTOBUF_EXAMPLE_PERSON_H */

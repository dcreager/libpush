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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <check.h>

#include <push/basics.h>
#include <push/talloc.h>

#include <push/protobuf/basics.h>
#include <push/protobuf/field-map.h>
#include <push/protobuf/message.h>


/*-----------------------------------------------------------------------
 * Our data type
 */

typedef struct _nested
{
    uint64_t  int2;
    uint64_t  int3;
} nested_t;

typedef struct _data
{
    uint32_t  int1;
    nested_t  nested;
} data_t;

static push_callback_t *
create_nested_message(const char *name,
                      void *parent,
                      push_parser_t *parser,
                      nested_t *nested)
{
    void  *context;
    push_protobuf_field_map_t  *field_map = NULL;
    push_callback_t  *callback = NULL;

    context = push_talloc_new(parent);
    if (context == NULL) return NULL;

    /*
     * Then create the callbacks.
     */

    if (name == NULL) name = "nested";

    field_map = push_protobuf_field_map_new(context);
    if (field_map == NULL) goto error;

#define CHECK(call) { if (!(call)) goto error; }

    CHECK(push_protobuf_assign_uint64(name, "int2", context, parser,
                                      field_map, 2, &nested->int2));
    CHECK(push_protobuf_assign_uint64(name, "int3", context, parser,
                                      field_map, 3, &nested->int3));

#undef CHECK

    callback = push_protobuf_message_new(name, context, parser, field_map);

    if (callback == NULL) goto error;
    return callback;

  error:
    push_talloc_free(context);
    return NULL;
}

static push_callback_t *
create_data_message(const char *name,
                    void *parent,
                    push_parser_t *parser,
                    data_t *dest)
{
    void  *context;
    const char  *nested_name = NULL;
    push_protobuf_field_map_t  *field_map = NULL;
    push_callback_t  *nested = NULL;
    push_callback_t  *callback = NULL;

    context = push_talloc_new(parent);
    if (context == NULL) return NULL;

    /*
     * Then create the callbacks.
     */

    if (name == NULL) name = "data";
    nested_name = push_talloc_asprintf(context, "%s.nested", name);

    field_map = push_protobuf_field_map_new(context);
    if (field_map == NULL) goto error;

    nested = create_nested_message(nested_name, context, parser, &dest->nested);
    if (nested == NULL) goto error;

#define CHECK(call) { if (!(call)) goto error; }

    CHECK(push_protobuf_assign_uint32(name, "int1", context, parser,
                                      field_map, 1, &dest->int1));
    CHECK(push_protobuf_add_submessage(name, "nested", context, parser,
                                       field_map, 2, nested));

#undef CHECK

    callback = push_protobuf_message_new(name, context, parser, field_map);

    if (callback == NULL) goto error;
    return callback;

  error:
    push_talloc_free(context);
    return NULL;
}

static bool
data_eq(const data_t *d1, const data_t *d2)
{
    if (d1 == d2) return true;
    if ((d1 == NULL) || (d2 == NULL)) return false;
    return
        (d1->int1 == d2->int1) &&
        (d1->nested.int2 == d2->nested.int2) &&
        (d1->nested.int3 == d2->nested.int3);
}


/*-----------------------------------------------------------------------
 * Sample data
 */

const uint8_t  DATA_01[] =
    "\x08"                      /* field 1, wire type 0 */
    "\xac\x02"                  /*   value = 300 */
    "\x12"                      /* field 2, wire type 2 */
    "\x08"                      /*   length = 8 */
    "\x10"                      /*   field 2, wire type 0 */
    "\x80\xe4\x97\xd0\x12"      /*     value = 5,000,000,000 */
    "\x18"                      /*   field 3, wire type 0 */
    "\x07";                     /*     value = 7 */

const size_t  LENGTH_01 = 13;
const data_t  EXPECTED_01 =
{ 300, { UINT64_C(5000000000), UINT64_C(7) } };


const uint8_t  DATA_02[] =
    "\x08"                      /* field 1, wire type 0 */
    "\xac\x02"                  /*   value = 300 */
    "\x22"                      /* field 4, wire type 2 */
    "\x00"                      /*   length = 0 */
    "\x12"                      /* field 2, wire type 2 */
    "\x11"                      /*   length = 17 */
    "\x10"                      /*   field 2, wire type 0 */
    "\x80\xe4\x97\xd0\x12"      /*     value = 5,000,000,000 */
    "\x2a"                      /*   field 5, wire type 2 */
    "\x07"                      /*     length = 7 */
    "1234567"                   /*     data */
    "\x18"                      /*   field 3, wire type 0 */
    "\x07";                     /*     value = 7 */

const size_t  LENGTH_02 = 24;
const data_t  EXPECTED_02 =
{ 300, { UINT64_C(5000000000), UINT64_C(7) } };


/*-----------------------------------------------------------------------
 * Helper functions
 */

#define READ_TEST(test_name)                                        \
    START_TEST(test_read_##test_name)                               \
    {                                                               \
        push_parser_t  *parser;                                     \
        push_callback_t  *message_callback;                         \
        data_t  actual;                                             \
                                                                    \
        PUSH_DEBUG_MSG("---\nStarting test case "                   \
                       "test_read_"                                 \
                       #test_name                                   \
                       "\n");                                       \
                                                                    \
        parser = push_parser_new();                                 \
        fail_if(parser == NULL,                                     \
                "Could not allocate a new push parser");            \
                                                                    \
        message_callback = create_data_message("data", NULL,        \
                                               parser, &actual);    \
        fail_if(message_callback == NULL,                           \
                "Could not allocate a new message callback");       \
                                                                    \
        push_parser_set_callback(parser, message_callback);         \
                                                                    \
        fail_unless(push_parser_activate(parser, NULL)              \
                    == PUSH_INCOMPLETE,                             \
                    "Could not activate parser");                   \
                                                                    \
        fail_unless(push_parser_submit_data                         \
                    (parser,                                        \
                     &DATA_##test_name,                             \
                     LENGTH_##test_name) == PUSH_INCOMPLETE,        \
                    "Could not parse data");                        \
                                                                    \
        fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,        \
                    "Shouldn't get parse error at EOF");            \
                                                                    \
        fail_unless(data_eq(&actual, &EXPECTED_##test_name),        \
                    "Value doesn't match (got "                     \
                    "(%"PRIu32",(%"PRIu64",%"PRIu64"))"             \
                    ", expected "                                   \
                    "(%"PRIu32",(%"PRIu64",%"PRIu64"))"             \
                    ")\n",                                          \
                    (uint32_t) actual.int1,                         \
                    (uint64_t) actual.nested.int2,                  \
                    (uint64_t) actual.nested.int3,                  \
                    (uint32_t) EXPECTED_##test_name.int1,           \
                    (uint64_t) EXPECTED_##test_name.nested.int2,    \
                    (uint64_t) EXPECTED_##test_name.nested.int3);   \
                                                                    \
        push_parser_free(parser);                                   \
    }                                                               \
    END_TEST


/*
 * Just like READ_TEST, but sends in the data in two chunks.  Tests
 * the ability of the callback to maintain its state across calls to
 * process_bytes.
 */

#define TWO_PART_READ_TEST(test_name)                               \
    START_TEST(test_two_part_read_##test_name)                      \
    {                                                               \
        push_parser_t  *parser;                                     \
        push_callback_t  *message_callback;                         \
        data_t  actual;                                             \
        size_t  first_chunk_size;                                   \
                                                                    \
        PUSH_DEBUG_MSG("---\nStarting test case "                   \
                       "test_two_part_read_"                        \
                       #test_name                                   \
                       "\n");                                       \
                                                                    \
        parser = push_parser_new();                                 \
        fail_if(parser == NULL,                                     \
                "Could not allocate a new push parser");            \
                                                                    \
        message_callback = create_data_message("data", NULL,        \
                                               parser, &actual);    \
        fail_if(message_callback == NULL,                           \
                "Could not allocate a new message callback");       \
                                                                    \
        push_parser_set_callback(parser, message_callback);         \
                                                                    \
        fail_unless(push_parser_activate(parser, NULL)              \
                    == PUSH_INCOMPLETE,                             \
                    "Could not activate parser");                   \
                                                                    \
        first_chunk_size = LENGTH_##test_name / 2;                  \
                                                                    \
        fail_unless(push_parser_submit_data                         \
                    (parser,                                        \
                     &DATA_##test_name,                             \
                     first_chunk_size) == PUSH_INCOMPLETE,          \
                    "Could not parse data");                        \
                                                                    \
        fail_unless(push_parser_submit_data                         \
                    (parser,                                        \
                     &DATA_##test_name[first_chunk_size],           \
                     LENGTH_##test_name - first_chunk_size) ==      \
                    PUSH_INCOMPLETE,                                \
                    "Could not parse data");                        \
                                                                    \
        fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,        \
                    "Shouldn't get parse error at EOF");            \
                                                                    \
        fail_unless(data_eq(&actual, &EXPECTED_##test_name),        \
                    "Value doesn't match (got "                     \
                    "(%"PRIu32",(%"PRIu64",%"PRIu64"))"             \
                    ", expected "                                   \
                    "(%"PRIu32",(%"PRIu64",%"PRIu64"))"             \
                    "\n",                                           \
                    (uint32_t) actual.int1,                         \
                    (uint64_t) actual.nested.int2,                  \
                    (uint64_t) actual.nested.int3,                  \
                    (uint32_t) EXPECTED_##test_name.int1,           \
                    (uint64_t) EXPECTED_##test_name.nested.int2,    \
                    (uint64_t) EXPECTED_##test_name.nested.int3);   \
                                                                    \
        push_parser_free(parser);                                   \
    }                                                               \
    END_TEST


/*
 * Just like READ_TEST, but chops off one byte before sending in the
 * data.  This should yield a parse error at EOF.
 */

#define PARSE_ERROR_TEST(test_name)                                 \
    START_TEST(test_parse_error_##test_name)                        \
    {                                                               \
        push_parser_t  *parser;                                     \
        push_callback_t  *message_callback;                         \
        data_t  actual;                                             \
                                                                    \
        PUSH_DEBUG_MSG("---\nStarting test case "                   \
                       "test_parse_error_"                          \
                       #test_name                                   \
                       "\n");                                       \
                                                                    \
        parser = push_parser_new();                                 \
        fail_if(parser == NULL,                                     \
                "Could not allocate a new push parser");            \
                                                                    \
        message_callback = create_data_message("data", NULL,        \
                                               parser, &actual);    \
        fail_if(message_callback == NULL,                           \
                "Could not allocate a new message callback");       \
                                                                    \
        push_parser_set_callback(parser, message_callback);         \
                                                                    \
        fail_unless(push_parser_activate(parser, NULL)              \
                    == PUSH_INCOMPLETE,                             \
                    "Could not activate parser");                   \
                                                                    \
        fail_unless(push_parser_submit_data                         \
                    (parser,                                        \
                     &DATA_##test_name,                             \
                     LENGTH_##test_name - 1) == PUSH_INCOMPLETE,    \
                    "Could not parse data");                        \
                                                                    \
        fail_unless(push_parser_eof(parser) == PUSH_PARSE_ERROR,    \
                    "Should get parse error at EOF");               \
                                                                    \
        push_parser_free(parser);                                   \
    }                                                               \
    END_TEST


/*-----------------------------------------------------------------------
 * Test cases
 */

READ_TEST(01)
READ_TEST(02)

TWO_PART_READ_TEST(01)
TWO_PART_READ_TEST(02)

PARSE_ERROR_TEST(01)
PARSE_ERROR_TEST(02)


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
test_suite()
{
    Suite  *s = suite_create("protobuf-message");

    TCase  *tc = tcase_create("protobuf-message");
    tcase_add_test(tc, test_read_01);
    tcase_add_test(tc, test_read_02);
    tcase_add_test(tc, test_two_part_read_01);
    tcase_add_test(tc, test_two_part_read_02);
    tcase_add_test(tc, test_parse_error_01);
    tcase_add_test(tc, test_parse_error_02);
    suite_add_tcase(s, tc);

    return s;
}


int
main(int argc, const char **argv)
{
    int  number_failed;
    Suite  *suite = test_suite();
    SRunner  *runner = srunner_create(suite);

    srunner_run_all(runner, CK_NORMAL);
    number_failed = srunner_ntests_failed(runner);
    srunner_free(runner);

    return (number_failed == 0)? EXIT_SUCCESS: EXIT_FAILURE;
}

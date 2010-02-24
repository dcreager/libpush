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

#include <talloc.h>

#include <check.h>
#include <hwm-buffer.h>

#include <push/basics.h>
#include <push/protobuf/basics.h>
#include <push/protobuf/field-map.h>
#include <push/protobuf/message.h>


/*-----------------------------------------------------------------------
 * Our data type
 */

typedef struct _data
{
    uint32_t  int1;
    uint64_t  int2;
    hwm_buffer_t  buf;
} data_t;

static void
data_init(data_t *data)
{
    hwm_buffer_init(&data->buf);
}

static void
data_done(data_t *data)
{
    hwm_buffer_done(&data->buf);
}

static push_callback_t *
create_data_message(const char *name,
                    push_parser_t *parser, data_t *dest)
{
    push_protobuf_field_map_t  *field_map = NULL;
    push_callback_t  *callback = NULL;

    /*
     * First construct all of the names.
     */

    if (name == NULL)
        name = "data";

    /*
     * Then create the callbacks.
     */

    field_map = push_protobuf_field_map_new(parser);
    if (field_map == NULL) goto error;

#define CHECK(call) { if (!(call)) goto error; }

    CHECK(push_protobuf_assign_uint32(name, ".int1", parser,
                                      field_map, 1, &dest->int1));
    CHECK(push_protobuf_assign_uint64(name, ".int2", parser,
                                      field_map, 2, &dest->int2));
    CHECK(push_protobuf_add_hwm_string(name, ".buf", parser,
                                       field_map, 3, &dest->buf));

#undef CHECK

    callback = push_protobuf_message_new(name, parser, field_map);
    if (callback == NULL) goto error;

    return callback;

  error:
    if (field_map != NULL) talloc_free(field_map);
    if (callback != NULL) talloc_free(callback);

    return NULL;
}

static bool
buf_eq(const hwm_buffer_t *b1, const hwm_buffer_t *b2)
{
    const void *buf1, *buf2;

    if (b1->current_size != b2->current_size) return false;

    buf1 = hwm_buffer_mem(b1, void);
    buf2 = hwm_buffer_mem(b2, void);
    return (memcmp(buf1, buf2, b1->current_size) == 0);
}

static bool
data_eq(const data_t *d1, const data_t *d2)
{
    if (d1 == d2) return true;
    if ((d1 == NULL) || (d2 == NULL)) return false;
    return
        (d1->int1 == d2->int1) && (d1->int2 == d2->int2) &&
        buf_eq(&d1->buf, &d2->buf);
}


/*-----------------------------------------------------------------------
 * Sample data
 */

const uint8_t  DATA_01[] =
    "\x08"                      /* field 1, wire type 0 */
    "\xac\x02"                  /*   value = 300 */
    "\x10"                      /* field 2, wire type 0 */
    "\x80\xe4\x97\xd0\x12";     /*   value = 5,000,000,000 */
const size_t  LENGTH_01 = 9;
const data_t  EXPECTED_01 =
{ 300, UINT64_C(5000000000), HWM_BUFFER_INIT(NULL, 0) };


const uint8_t  DATA_02[] =
    "\x08"                      /* field 1, wire type 0 */
    "\xac\x02"                  /*   value = 300 */
    "\x22"                      /* field 4, wire type 2 */
    "\x00"                      /*   length = 0 */
    "\x10"                      /* field 2, wire type 0 */
    "\x80\xe4\x97\xd0\x12"      /*   value = 5,000,000,000 */
    "\x2a"                      /* field 5, wire type 2 */
    "\x07"                      /*   length = 7 */
    "1234567";                  /*   data */
const size_t  LENGTH_02 = 20;
const data_t  EXPECTED_02 =
{ 300, UINT64_C(5000000000), HWM_BUFFER_INIT(NULL, 0) };


const uint8_t  DATA_03[] =
    "\x08"                      /* field 1, wire type 0 */
    "\xac\x02"                  /*   value = 300 */
    "\x10"                      /* field 2, wire type 0 */
    "\x80\xe4\x97\xd0\x12"      /*   value = 5,000,000,000 */
    "\x1a"                      /* field 3, wire type 2 */
    "\x05"                      /*   length = 5 */
    "abcde";                    /*   content */
const size_t  LENGTH_03 = 16;
const char  EXPECTED_BUF_03[] = "abcde";
/* include an extra byte in the expected HWM for the NUL terminator */
const data_t  EXPECTED_03 =
{ 300, UINT64_C(5000000000), HWM_BUFFER_INIT(EXPECTED_BUF_03, 6) };


const uint8_t  DATA_04[] =
    "\x1a"                      /* field 3, wire type 2 */
    "\x05"                      /*   length = 5 */
    "abcde"                     /*   content */
    "\x08"                      /* field 1, wire type 0 */
    "\xac\x02"                  /*   value = 300 */
    "\x10"                      /* field 2, wire type 0 */
    "\x80\xe4\x97\xd0\x12";     /*   value = 5,000,000,000 */
const size_t  LENGTH_04 = 16;
const char  EXPECTED_BUF_04[] = "abcde";
/* include an extra byte in the expected HWM for the NUL terminator */
const data_t  EXPECTED_04 =
{ 300, UINT64_C(5000000000), HWM_BUFFER_INIT(EXPECTED_BUF_03, 6) };


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
        data_init(&actual);                                         \
                                                                    \
        parser = push_parser_new();                                 \
        fail_if(parser == NULL,                                     \
                "Could not allocate a new push parser");            \
                                                                    \
        message_callback = create_data_message("data",              \
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
                    "(%"PRIu32",%"PRIu64")"                         \
                    ", expected "                                   \
                    "(%"PRIu32",%"PRIu64")"                         \
                    ")\n",                                          \
                    (uint32_t) actual.int1,                         \
                    (uint64_t) actual.int2,                         \
                    (uint32_t) EXPECTED_##test_name.int1,           \
                    (uint64_t) EXPECTED_##test_name.int2);          \
                                                                    \
        push_parser_free(parser);                                   \
        data_done(&actual);                                         \
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
        data_init(&actual);                                         \
                                                                    \
        parser = push_parser_new();                                 \
        fail_if(parser == NULL,                                     \
                "Could not allocate a new push parser");            \
                                                                    \
        message_callback = create_data_message("data",              \
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
                    "(%"PRIu32",%"PRIu64")"                         \
                    ", expected "                                   \
                    "(%"PRIu32",%"PRIu64")"                         \
                    "\n",                                           \
                    (uint32_t) actual.int1,                         \
                    (uint64_t) actual.int2,                         \
                    (uint32_t) EXPECTED_##test_name.int1,           \
                    (uint64_t) EXPECTED_##test_name.int2);          \
                                                                    \
        push_parser_free(parser);                                   \
        data_done(&actual);                                         \
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
        data_init(&actual);                                         \
                                                                    \
        parser = push_parser_new();                                 \
        fail_if(parser == NULL,                                     \
                "Could not allocate a new push parser");            \
                                                                    \
        message_callback = create_data_message("data",              \
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
        data_done(&actual);                                         \
    }                                                               \
    END_TEST


/*-----------------------------------------------------------------------
 * Test cases
 */

READ_TEST(01)
READ_TEST(02)
READ_TEST(03)
READ_TEST(04)

TWO_PART_READ_TEST(01)
TWO_PART_READ_TEST(02)
TWO_PART_READ_TEST(03)
TWO_PART_READ_TEST(04)

PARSE_ERROR_TEST(01)
PARSE_ERROR_TEST(02)
PARSE_ERROR_TEST(03)
PARSE_ERROR_TEST(04)


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
    tcase_add_test(tc, test_read_03);
    tcase_add_test(tc, test_read_04);
    tcase_add_test(tc, test_two_part_read_01);
    tcase_add_test(tc, test_two_part_read_02);
    tcase_add_test(tc, test_two_part_read_03);
    tcase_add_test(tc, test_two_part_read_04);
    tcase_add_test(tc, test_parse_error_01);
    tcase_add_test(tc, test_parse_error_02);
    tcase_add_test(tc, test_parse_error_03);
    tcase_add_test(tc, test_parse_error_04);
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

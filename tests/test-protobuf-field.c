/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2010, RedJack, LLC.
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

#include <push.h>
#include <push/protobuf.h>


/*-----------------------------------------------------------------------
 * Sample data
 */

const uint8_t  DATA_01[] =
    "\x08"                      /* field 1, wire type 0 */
    "\xac\x02";                 /*   value = 300 */
const size_t  LENGTH_01 = 3;
const uint32_t  EXPECTED_01 = 300;

const uint8_t  DATA_02[] =
    "\x10"                      /* field 2, wire type 0 */
    "\x80\xe4\x97\xd0\x12";     /*   value = 5,000,000,000 */
const size_t  LENGTH_02 = 6;
const uint64_t  EXPECTED_02 = UINT64_C(5000000000);


/*-----------------------------------------------------------------------
 * Helper functions
 */

#define READ_TEST(test_name, value_type)                            \
    START_TEST(test_read_##test_name)                               \
    {                                                               \
        push_parser_t  *parser;                                     \
        push_protobuf_##value_type##_t  *value_callback;            \
        push_protobuf_field_t  *field_callback;                     \
                                                                    \
        PUSH_DEBUG_MSG("---\nStarting test case "                   \
                       "test_read_"                                 \
                       #test_name                                   \
                       "\n");                                       \
                                                                    \
        value_callback =                                            \
            push_protobuf_##value_type##_new(NULL, true);           \
        fail_if(value_callback == NULL,                             \
                "Could not allocate a new value callback");         \
                                                                    \
        field_callback = push_protobuf_field_new                    \
            (PUSH_PROTOBUF_TAG_TYPE_VARINT,                         \
             &value_callback->base,                                 \
             NULL, true);                                           \
        fail_if(field_callback == NULL,                             \
                "Could not allocate a new field callback");         \
                                                                    \
        field_callback->base.next_callback =                        \
            &field_callback->base;                                  \
                                                                    \
        parser = push_parser_new(&field_callback->base);            \
        fail_if(parser == NULL,                                     \
                "Could not allocate a new push parser");            \
                                                                    \
        fail_unless(push_parser_submit_data                         \
                    (parser,                                        \
                     &DATA_##test_name,                             \
                     LENGTH_##test_name) == PUSH_SUCCESS,           \
                    "Could not parse data");                        \
                                                                    \
        fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,        \
                    "Shouldn't get parse error at EOF");            \
                                                                    \
        fail_unless(value_callback->value == EXPECTED_##test_name,  \
                    "Value doesn't match (got %"PRIu64              \
                    ", expected %"PRIu64")",                        \
                    (uint64_t) value_callback->value,               \
                    (uint64_t) EXPECTED_##test_name);               \
                                                                    \
        push_parser_free(parser);                                   \
    }                                                               \
    END_TEST


/*
 * Just like READ_TEST, but sends in the data in two chunks.  Tests
 * the ability of the callback to maintain its state across calls to
 * process_bytes.
 */

#define TWO_PART_READ_TEST(test_name, value_type)                   \
    START_TEST(test_two_part_read_##test_name)                      \
    {                                                               \
        push_parser_t  *parser;                                     \
        push_protobuf_##value_type##_t  *value_callback;            \
        push_protobuf_field_t  *field_callback;                     \
        size_t  first_chunk_size;                                   \
                                                                    \
        PUSH_DEBUG_MSG("---\nStarting test case "                   \
                       "test_two_part_read_"                        \
                       #test_name                                   \
                       "\n");                                       \
                                                                    \
        value_callback =                                            \
            push_protobuf_##value_type##_new(NULL, true);           \
        fail_if(value_callback == NULL,                             \
                "Could not allocate a new value callback");         \
                                                                    \
        field_callback = push_protobuf_field_new                    \
            (PUSH_PROTOBUF_TAG_TYPE_VARINT,                         \
             &value_callback->base,                                 \
             NULL, true);                                           \
        fail_if(field_callback == NULL,                             \
                "Could not allocate a new field callback");         \
                                                                    \
        field_callback->base.next_callback =                        \
            &field_callback->base;                                  \
                                                                    \
        parser = push_parser_new(&field_callback->base);            \
        fail_if(parser == NULL,                                     \
                "Could not allocate a new push parser");            \
                                                                    \
        first_chunk_size = LENGTH_##test_name / 2;                  \
                                                                    \
        fail_unless(push_parser_submit_data                         \
                    (parser,                                        \
                     &DATA_##test_name,                             \
                     first_chunk_size) == PUSH_SUCCESS,             \
                    "Could not parse data");                        \
                                                                    \
        fail_unless(push_parser_submit_data                         \
                    (parser,                                        \
                     &DATA_##test_name[first_chunk_size],           \
                     LENGTH_##test_name - first_chunk_size) ==      \
                    PUSH_SUCCESS,                                   \
                    "Could not parse data");                        \
                                                                    \
        fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,        \
                    "Shouldn't get parse error at EOF");            \
                                                                    \
        fail_unless(value_callback->value == EXPECTED_##test_name,  \
                    "Value doesn't match (got %"PRIu64              \
                    ", expected %"PRIu64")",                        \
                    (uint64_t) value_callback->value,               \
                    (uint64_t) EXPECTED_##test_name);               \
                                                                    \
        push_parser_free(parser);                                   \
    }                                                               \
    END_TEST


/*
 * Just like READ_TEST, but chops off one byte before sending in the
 * data.  This should yield a parse error at EOF.
 */

#define PARSE_ERROR_TEST(test_name, value_type)                     \
    START_TEST(test_parse_error_##test_name)                        \
    {                                                               \
        push_parser_t  *parser;                                     \
        push_protobuf_##value_type##_t  *value_callback;            \
        push_protobuf_field_t  *field_callback;                     \
                                                                    \
        PUSH_DEBUG_MSG("---\nStarting test case "                   \
                       "test_parse_error_"                          \
                       #test_name                                   \
                       "\n");                                       \
                                                                    \
        value_callback =                                            \
            push_protobuf_##value_type##_new(NULL, true);           \
        fail_if(value_callback == NULL,                             \
                "Could not allocate a new value callback");         \
                                                                    \
        field_callback = push_protobuf_field_new                    \
            (PUSH_PROTOBUF_TAG_TYPE_VARINT,                         \
             &value_callback->base,                                 \
             NULL, true);                                           \
        fail_if(field_callback == NULL,                             \
                "Could not allocate a new field callback");         \
                                                                    \
        field_callback->base.next_callback =                        \
            &field_callback->base;                                  \
                                                                    \
        parser = push_parser_new(&field_callback->base);            \
        fail_if(parser == NULL,                                     \
                "Could not allocate a new push parser");            \
                                                                    \
        fail_unless(push_parser_submit_data                         \
                    (parser,                                        \
                     &DATA_##test_name,                             \
                     LENGTH_##test_name - 1) == PUSH_SUCCESS,       \
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

READ_TEST(01, varint32)
READ_TEST(02, varint64)

TWO_PART_READ_TEST(01, varint32)
TWO_PART_READ_TEST(02, varint64)

PARSE_ERROR_TEST(01, varint32)
PARSE_ERROR_TEST(02, varint64)


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
test_suite()
{
    Suite  *s = suite_create("protobuf-varint32");

    TCase  *tc = tcase_create("protobuf-varint32");
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

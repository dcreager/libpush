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

#include <push/basics.h>

#include <push/protobuf/basics.h>
#include <push/protobuf/primitives.h>


/*-----------------------------------------------------------------------
 * Sample data
 */


/**
 * Return either a 32-bit or 64-bit version of a constant, depending
 * on how big size_t is.
 */

#if SIZE_MAX == UINT32_MAX
#define SIZE_C(s32, s64) (s32)

#elif SIZE_MAX == UINT64_MAX
#define SIZE_C(s32, s64) (UINT64_C(s64))

#else
#error "We need size_t to be either 32 or 64 bits."
#endif

const uint8_t  DATA_01[] = "\x00";
const size_t  LENGTH_01 = 1;
const size_t  EXPECTED_01 = 0;

const uint8_t  DATA_02[] = "\x01";
const size_t  LENGTH_02 = 1;
const size_t  EXPECTED_02 = 1;

const uint8_t  DATA_03[] = "\xac\x02";
const size_t  LENGTH_03 = 2;
const size_t  EXPECTED_03 = 300;

const uint8_t  DATA_04[] = "\x80\xe4\x97\xd0\x12";
const size_t  LENGTH_04 = 5;
/* 705,032,704 == 5,000,000,000 truncated to 32 bits */
const size_t  EXPECTED_04 = SIZE_C(705032704, 5000000000);

const uint8_t  DATA_TRASH[] = "\x00\x00\x00\x00\x00\x00";
const size_t  LENGTH_TRASH = 6;


/*-----------------------------------------------------------------------
 * Helper functions
 */

#define READ_TEST(test_name)                                        \
    START_TEST(test_read_##test_name)                               \
    {                                                               \
        push_parser_t  *parser;                                     \
        push_callback_t  *callback;                                 \
        size_t  *result;                                            \
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
        callback =                                                  \
            push_protobuf_varint_size_new                           \
            ("varint-size", NULL, parser);                          \
        fail_if(callback == NULL,                                   \
                "Could not allocate a new callback");               \
                                                                    \
        push_parser_set_callback(parser, callback);                 \
                                                                    \
        fail_unless(push_parser_activate(parser, NULL)              \
                    == PUSH_INCOMPLETE,                             \
                    "Could not activate parser");                   \
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
        result = push_parser_result(parser, size_t);                \
                                                                    \
        fail_unless(*result == EXPECTED_##test_name,                \
                    "Value doesn't match (got %"PRIu32              \
                    ", expected %"PRIu32")",                        \
                    *result, EXPECTED_##test_name);                 \
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
        push_callback_t  *callback;                                 \
        size_t  *result;                                            \
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
        callback =                                                  \
            push_protobuf_varint_size_new                           \
            ("varint-size", NULL, parser);                          \
        fail_if(callback == NULL,                                   \
                "Could not allocate a new callback");               \
                                                                    \
        push_parser_set_callback(parser, callback);                 \
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
                    PUSH_SUCCESS,                                   \
                    "Could not parse data");                        \
                                                                    \
        fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,        \
                    "Shouldn't get parse error at EOF");            \
                                                                    \
        result = push_parser_result(parser, size_t);                \
                                                                    \
        fail_unless(*result == EXPECTED_##test_name,                \
                    "Value doesn't match (got %"PRIu32              \
                    ", expected %"PRIu32")",                        \
                    *result, EXPECTED_##test_name);                 \
                                                                    \
        push_parser_free(parser);                                   \
    }                                                               \
    END_TEST


#define TRASH_TEST(test_name)                                       \
    START_TEST(test_trash_##test_name)                              \
    {                                                               \
        push_parser_t  *parser;                                     \
        push_callback_t  *callback;                                 \
        size_t  *result;                                            \
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
        callback =                                                  \
            push_protobuf_varint_size_new                           \
            ("varint-size", NULL, parser);                          \
        fail_if(callback == NULL,                                   \
                "Could not allocate a new callback");               \
                                                                    \
        push_parser_set_callback(parser, callback);                 \
                                                                    \
        fail_unless(push_parser_activate(parser, NULL)              \
                    == PUSH_INCOMPLETE,                             \
                    "Could not activate parser");                   \
                                                                    \
        fail_unless(push_parser_submit_data                         \
                    (parser,                                        \
                     &DATA_##test_name,                             \
                     LENGTH_##test_name) == PUSH_SUCCESS,           \
                    "Could not parse data");                        \
                                                                    \
        fail_unless(push_parser_submit_data                         \
                    (parser,                                        \
                     &DATA_TRASH,                                   \
                     LENGTH_TRASH) == PUSH_SUCCESS,                 \
                    "Could not parse trailing trash");              \
                                                                    \
        fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,        \
                    "Shouldn't get parse error at EOF");            \
                                                                    \
        result = push_parser_result(parser, size_t);                \
                                                                    \
        fail_unless(*result == EXPECTED_##test_name,                \
                    "Value doesn't match (got %"PRIu32              \
                    ", expected %"PRIu32")",                        \
                    *result, EXPECTED_##test_name);                 \
                                                                    \
        push_parser_free(parser);                                   \
    }                                                               \
    END_TEST


/*-----------------------------------------------------------------------
 * Test cases
 */

READ_TEST(01)
READ_TEST(02)
READ_TEST(03)
READ_TEST(04)

/*
 * Only do the two-part read test for the test cases that have more
 * than one byte in them.
 */

TWO_PART_READ_TEST(03)
TWO_PART_READ_TEST(04)

TRASH_TEST(01)
TRASH_TEST(02)
TRASH_TEST(03)
TRASH_TEST(04)

START_TEST(test_parse_error_03)
{
    push_parser_t  *parser;
    push_callback_t  *callback;

    PUSH_DEBUG_MSG("---\nStarting test case test_parse_error_03\n");

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    callback =
        push_protobuf_varint_size_new("varint-size", NULL, parser);
    fail_if(callback == NULL,
            "Could not allocate a new callback");

    push_parser_set_callback(parser, callback);

    fail_unless(push_parser_activate(parser, NULL)
                == PUSH_INCOMPLETE,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser,
                 &DATA_03,
                 LENGTH_03 - 1) == PUSH_INCOMPLETE,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_PARSE_ERROR,
                "Should get parse error at EOF");

    push_parser_free(parser);
}
END_TEST


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
test_suite()
{
    Suite  *s = suite_create("protobuf-varint-size");

    TCase  *tc = tcase_create("protobuf-varint-size");
    tcase_add_test(tc, test_read_01);
    tcase_add_test(tc, test_read_02);
    tcase_add_test(tc, test_read_03);
    tcase_add_test(tc, test_read_04);
    tcase_add_test(tc, test_two_part_read_03);
    tcase_add_test(tc, test_two_part_read_04);
    tcase_add_test(tc, test_trash_01);
    tcase_add_test(tc, test_trash_02);
    tcase_add_test(tc, test_trash_03);
    tcase_add_test(tc, test_trash_04);
    tcase_add_test(tc, test_parse_error_03);
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

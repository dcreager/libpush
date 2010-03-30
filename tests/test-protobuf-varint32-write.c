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

uint32_t  DATA_01 = 0;
size_t  LENGTH_01 = 1;
uint8_t  EXPECTED_01[] = "\x00\x00\x00\x00\x00";

uint32_t  DATA_02 = 1;
size_t  LENGTH_02 = 1;
uint8_t  EXPECTED_02[] = "\x01\x00\x00\x00\x00";

uint32_t  DATA_03 = 300;
size_t  LENGTH_03 = 2;
uint8_t  EXPECTED_03[] = "\xac\x02\x00\x00\x00";

/* 5,000,000,000 truncated to 32 bits */
uint32_t  DATA_04 = 705032704;
size_t  LENGTH_04 = 5;
uint8_t  EXPECTED_04[] = "\x80\xe4\x97\xd0\x02";

uint32_t  DATA_05 = -500;
size_t  LENGTH_05 = 5;
uint8_t  EXPECTED_05[] =
    "\x8c\xfc\xff\xff\x0f";

/* -5,000,000,000 truncated to 32 bits */
uint32_t  DATA_06 = -705032704;
size_t  LENGTH_06 = 5;
uint8_t  EXPECTED_06[] =
    "\x80\x9c\xe8\xaf\x0d";


/*-----------------------------------------------------------------------
 * Helper functions
 */

#define WRITE_TEST(test_name)                                       \
    START_TEST(test_write_##test_name)                              \
    {                                                               \
        push_parser_t  *parser;                                     \
        push_callback_t  *callback;                                 \
        uint8_t  output[PUSH_PROTOBUF_MAX_VARINT32_LENGTH];         \
                                                                    \
        PUSH_DEBUG_MSG("---\nStarting test case "                   \
                       "test_write_"                                \
                       #test_name                                   \
                       "\n");                                       \
                                                                    \
        parser = push_parser_new();                                 \
        fail_if(parser == NULL,                                     \
                "Could not allocate a new push parser");            \
                                                                    \
        callback = push_protobuf_write_varint32_new                 \
            ("varint32", NULL, parser, 0);                          \
        fail_if(callback == NULL,                                   \
                "Could not allocate a new callback");               \
                                                                    \
        push_parser_set_callback(parser, callback);                 \
                                                                    \
        fail_unless(push_parser_activate(parser, &DATA_##test_name) \
                    == PUSH_INCOMPLETE,                             \
                    "Could not activate parser");                   \
                                                                    \
        memset(output, 0, PUSH_PROTOBUF_MAX_VARINT32_LENGTH);       \
                                                                    \
        fail_unless(push_parser_submit_data                         \
                    (parser,                                        \
                     output, PUSH_PROTOBUF_MAX_VARINT32_LENGTH)     \
                    == PUSH_SUCCESS,                                \
                    "Could not serialize data");                    \
                                                                    \
        fail_unless(memcmp(output, EXPECTED_##test_name,            \
                           PUSH_PROTOBUF_MAX_VARINT32_LENGTH) == 0, \
                    "Value doesn't match (got "                     \
                    "%02"PRIx8":"                                   \
                    "%02"PRIx8":"                                   \
                    "%02"PRIx8":"                                   \
                    "%02"PRIx8":"                                   \
                    "%02"PRIx8""                                    \
                    ", expected "                                   \
                    "%02"PRIx8":"                                   \
                    "%02"PRIx8":"                                   \
                    "%02"PRIx8":"                                   \
                    "%02"PRIx8":"                                   \
                    "%02"PRIx8").",                                 \
                    output[0],                                      \
                    output[1],                                      \
                    output[2],                                      \
                    output[3],                                      \
                    output[4],                                      \
                    EXPECTED_##test_name[0],                        \
                    EXPECTED_##test_name[1],                        \
                    EXPECTED_##test_name[2],                        \
                    EXPECTED_##test_name[3],                        \
                    EXPECTED_##test_name[4]);                       \
                                                                    \
        push_parser_free(parser);                                   \
    }                                                               \
    END_TEST


/*
 * Just like WRITE_TEST, but sends in the data in two chunks.  Tests
 * the ability of the callback to maintain its state across calls to
 * process_bytes.
 */

#define TWO_PART_WRITE_TEST(test_name)                              \
    START_TEST(test_two_part_write_##test_name)                     \
    {                                                               \
        push_parser_t  *parser;                                     \
        push_callback_t  *callback;                                 \
        uint8_t  output[PUSH_PROTOBUF_MAX_VARINT32_LENGTH];         \
        size_t  first_chunk_size;                                   \
                                                                    \
        PUSH_DEBUG_MSG("---\nStarting test case "                   \
                       "test_two_part_write_"                       \
                       #test_name                                   \
                       "\n");                                       \
                                                                    \
        parser = push_parser_new();                                 \
        fail_if(parser == NULL,                                     \
                "Could not allocate a new push parser");            \
                                                                    \
        callback = push_protobuf_write_varint32_new                 \
            ("varint32", NULL, parser, 0);                          \
        fail_if(callback == NULL,                                   \
                "Could not allocate a new callback");               \
                                                                    \
        push_parser_set_callback(parser, callback);                 \
                                                                    \
        fail_unless(push_parser_activate(parser, &DATA_##test_name) \
                    == PUSH_INCOMPLETE,                             \
                    "Could not activate parser");                   \
                                                                    \
        memset(output, 0, PUSH_PROTOBUF_MAX_VARINT32_LENGTH);       \
                                                                    \
        first_chunk_size = LENGTH_##test_name / 2;                  \
                                                                    \
        fail_unless(push_parser_submit_data                         \
                    (parser,                                        \
                     output, first_chunk_size) == PUSH_INCOMPLETE,  \
                    "Could not serialize data");                    \
                                                                    \
        fail_unless(push_parser_submit_data                         \
                    (parser,                                        \
                     output + first_chunk_size,                     \
                     PUSH_PROTOBUF_MAX_VARINT32_LENGTH -            \
                     first_chunk_size)                              \
                    == PUSH_SUCCESS,                                \
                    "Could not parse data");                        \
                                                                    \
        fail_unless(memcmp(output, EXPECTED_##test_name,            \
                           PUSH_PROTOBUF_MAX_VARINT32_LENGTH) == 0, \
                    "Value doesn't match (got "                     \
                    "%02"PRIx8":"                                   \
                    "%02"PRIx8":"                                   \
                    "%02"PRIx8":"                                   \
                    "%02"PRIx8":"                                   \
                    "%02"PRIx8""                                    \
                    ", expected "                                   \
                    "%02"PRIx8":"                                   \
                    "%02"PRIx8":"                                   \
                    "%02"PRIx8":"                                   \
                    "%02"PRIx8":"                                   \
                    "%02"PRIx8").",                                 \
                    output[0],                                      \
                    output[1],                                      \
                    output[2],                                      \
                    output[3],                                      \
                    output[4],                                      \
                    EXPECTED_##test_name[0],                        \
                    EXPECTED_##test_name[1],                        \
                    EXPECTED_##test_name[2],                        \
                    EXPECTED_##test_name[3],                        \
                    EXPECTED_##test_name[4]);                       \
                                                                    \
        push_parser_free(parser);                                   \
    }                                                               \
    END_TEST


/*-----------------------------------------------------------------------
 * Test cases
 */

WRITE_TEST(01)
WRITE_TEST(02)
WRITE_TEST(03)
WRITE_TEST(04)
WRITE_TEST(05)
WRITE_TEST(06)

/*
 * Only do the two-part write test for the test cases that have more
 * than one byte in them.
 */

TWO_PART_WRITE_TEST(03)
TWO_PART_WRITE_TEST(04)
TWO_PART_WRITE_TEST(05)
TWO_PART_WRITE_TEST(06)


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
test_suite()
{
    Suite  *s = suite_create("protobuf-varint32-write");

    TCase  *tc = tcase_create("protobuf-varint32-write");
    tcase_add_test(tc, test_write_01);
    tcase_add_test(tc, test_write_02);
    tcase_add_test(tc, test_write_03);
    tcase_add_test(tc, test_write_04);
    tcase_add_test(tc, test_write_05);
    tcase_add_test(tc, test_write_06);
    tcase_add_test(tc, test_two_part_write_03);
    tcase_add_test(tc, test_two_part_write_04);
    tcase_add_test(tc, test_two_part_write_05);
    tcase_add_test(tc, test_two_part_write_06);
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

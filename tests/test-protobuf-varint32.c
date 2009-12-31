/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009, RedJack, LLC.
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

const uint8_t  DATA_01[] = "\x00";
const size_t  LENGTH_01 = 1;
const uint32_t  EXPECTED_01 = 0;

const uint8_t  DATA_02[] = "\x01";
const size_t  LENGTH_02 = 1;
const uint32_t  EXPECTED_02 = 1;

const uint8_t  DATA_03[] = "\xac\x02";
const size_t  LENGTH_03 = 2;
const uint32_t  EXPECTED_03 = 300;


/*-----------------------------------------------------------------------
 * Helper functions
 */

#define READ_TEST(test_name)                                        \
    START_TEST(test_read_##test_name)                               \
    {                                                               \
        push_parser_t  *parser;                                     \
        push_protobuf_varint32_t  *callback;                        \
                                                                    \
        PUSH_DEBUG_MSG("---\nStarting test case "                   \
                       "test_read_"                                 \
                       #test_name                                   \
                       "\n");                                       \
                                                                    \
        callback = push_protobuf_varint32_new(NULL, true);          \
        fail_if(callback == NULL,                                   \
                "Could not allocate a new callback");               \
        callback->next_callback = &callback->base;                  \
                                                                    \
        parser = push_parser_new(&callback->base);                  \
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
        fail_unless(callback->value == EXPECTED_##test_name,        \
                    "Value doesn't match (got %"PRIu32              \
                    ", expected %"PRIu32")",                        \
                    callback->value, EXPECTED_##test_name);         \
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

START_TEST(test_parse_error_03)
{
    push_parser_t  *parser;
    push_protobuf_varint32_t  *callback;

    PUSH_DEBUG_MSG("---\nStarting test case test_parse_error_03\n");

    callback = push_protobuf_varint32_new(NULL, true);
    fail_if(callback == NULL,
            "Could not allocate a new callback");
    callback->next_callback = &callback->base;

    parser = push_parser_new(&callback->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_submit_data
                (parser,
                 &DATA_03,
                 LENGTH_03 - 1) == PUSH_SUCCESS,
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
    Suite  *s = suite_create("protobuf-varint32");

    TCase  *tc = tcase_create("protobuf-varint32");
    tcase_add_test(tc, test_read_01);
    tcase_add_test(tc, test_read_02);
    tcase_add_test(tc, test_read_03);
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

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
#include <push/min-bytes.h>


/*-----------------------------------------------------------------------
 * Integer parser callback
 */

typedef struct _integer_callback
{
    push_callback_t  base;
    uint32_t  integer;
} integer_callback_t;


static ssize_t
integer_callback_process_bytes(push_parser_t *parser,
                           push_callback_t *pcallback,
                           const void *buf,
                           size_t bytes_available)
{
    integer_callback_t  *callback = (integer_callback_t *) pcallback;

    PUSH_DEBUG_MSG("integer: Processing %zu bytes at %p.\n",
                   bytes_available, buf);

    if (bytes_available < sizeof(uint32_t))
    {
        return PUSH_PARSE_ERROR;
    } else {
        const uint32_t  *next_integer = (const uint32_t *) buf;
        buf += sizeof(uint32_t);
        bytes_available -= sizeof(uint32_t);

        PUSH_DEBUG_MSG("integer: Got %"PRIu32".\n", *next_integer);

        callback->integer = *next_integer;

        return bytes_available;
    }
}


static integer_callback_t *
integer_callback_new()
{
    integer_callback_t  *result =
        (integer_callback_t *) malloc(sizeof(integer_callback_t));

    if (result == NULL)
        return NULL;

    push_callback_init(&result->base,
                       NULL,
                       integer_callback_process_bytes,
                       NULL);

    result->integer = 0;
    result->base.result = &result->integer;

    return result;
}


/*-----------------------------------------------------------------------
 * Sample data
 */

const uint32_t  DATA_01[] = { 1 };
const size_t  LENGTH_01 = 1 * sizeof(uint32_t);


/*-----------------------------------------------------------------------
 * Test cases
 */


START_TEST(test_integer_01)
{
    push_parser_t  *parser;
    integer_callback_t  *callback;
    uint32_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_integer_01\n");

    callback = integer_callback_new();
    fail_if(callback == NULL,
            "Could not allocate a new integer callback");

    parser = push_parser_new(&callback->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = (uint32_t *) callback->base.result;

    fail_unless(*result == 1,
                "Integer doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *result, 1);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_integer_02)
{
    push_parser_t  *parser;
    integer_callback_t  *callback;
    uint32_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_integer_02\n");

    /*
     * If we submit the data twice, we should get the same result,
     * since we'll ignore any later data.
     */

    callback = integer_callback_new();
    fail_if(callback == NULL,
            "Could not allocate a new integer callback");

    parser = push_parser_new(&callback->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = (uint32_t *) callback->base.result;

    fail_unless(*result == 1,
                "Integer doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *result, 1);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_integer_03)
{
    push_parser_t  *parser;
    integer_callback_t  *ints;
    push_min_bytes_t  *callback;
    uint32_t  *result;
    size_t  FIRST_CHUNK_SIZE = 3; /* something not divisible by 4 */

    PUSH_DEBUG_MSG("---\nStarting test_integer_03\n");

    /*
     * If we submit the data in two chunks, we should get the same
     * result, assuming that we wrap the integer_callback_t in a
     * push_min_bytes_t callback.
     */

    ints = integer_callback_new();
    fail_if(ints == NULL,
            "Could not allocate a new integer callback");

    callback = push_min_bytes_new(&ints->base, sizeof(uint32_t));
    fail_if(callback == NULL,
            "Could not allocate a new min-bytes callback");

    parser = push_parser_new(&callback->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, FIRST_CHUNK_SIZE) == PUSH_INCOMPLETE,
                "Could not parse data");

    fail_unless(push_parser_submit_data
                (parser,
                 ((void *) DATA_01) + FIRST_CHUNK_SIZE,
                 LENGTH_01 - FIRST_CHUNK_SIZE) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = (uint32_t *) callback->base.result;

    fail_unless(*result == 1,
                "Integer doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *result, 1);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_integer_04)
{
    push_parser_t  *parser;
    integer_callback_t  *ints;
    push_min_bytes_t  *callback;
    uint32_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_integer_04\n");

    /*
     * If we submit the data twice, we should get the same result,
     * since we'll ignore any later data — even if the callback is
     * wrapped in a push_min_bytes_t.
     */

    ints = integer_callback_new();
    fail_if(ints == NULL,
            "Could not allocate a new integer callback");

    callback = push_min_bytes_new(&ints->base, sizeof(uint32_t));
    fail_if(callback == NULL,
            "Could not allocate a new min-bytes callback");

    parser = push_parser_new(&callback->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = (uint32_t *) callback->base.result;

    fail_unless(*result == 1,
                "Integer doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *result, 1);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_parse_error_01)
{
    push_parser_t  *parser;
    integer_callback_t  *callback;
    size_t  FIRST_CHUNK_SIZE = 3; /* something not divisible by 4 */

    PUSH_DEBUG_MSG("---\nStarting test_parse_error_01\n");

    /*
     * Our callback processes integers on nice 32-bit boundaries.  If we
     * send the parser data that doesn't align with these boundaries,
     * and then reach EOF, we should get a parse error.
     */

    callback = integer_callback_new();
    fail_if(callback == NULL,
            "Could not allocate a new integer callback");

    parser = push_parser_new(&callback->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, FIRST_CHUNK_SIZE) == PUSH_PARSE_ERROR,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_PARSE_ERROR,
                "Should get parse error at EOF");

    push_parser_free(parser);
}
END_TEST


START_TEST(test_parse_error_02)
{
    push_parser_t  *parser;
    integer_callback_t  *ints;
    push_min_bytes_t  *callback;
    size_t  FIRST_CHUNK_SIZE = 3; /* something not divisible by 4 */

    PUSH_DEBUG_MSG("---\nStarting test_parse_error_02\n");

    /*
     * Our callback processes integers on nice 32-bit boundaries.  If
     * we send the parser data that doesn't align with these
     * boundaries, and then reach EOF, we should get a parse error,
     * even if we've wrapped the integer_callback_t in a
     * push_min_bytes_t.
     */

    ints = integer_callback_new();
    fail_if(ints == NULL,
            "Could not allocate a new integer callback");

    callback = push_min_bytes_new(&ints->base, sizeof(uint32_t));
    fail_if(callback == NULL,
            "Could not allocate a new min-bytes callback");

    parser = push_parser_new(&callback->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, FIRST_CHUNK_SIZE) == PUSH_INCOMPLETE,
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
    Suite  *s = suite_create("integer-callback");

    TCase  *tc = tcase_create("integer-callback");
    tcase_add_test(tc, test_integer_01);
    tcase_add_test(tc, test_integer_02);
    tcase_add_test(tc, test_integer_03);
    tcase_add_test(tc, test_integer_04);
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

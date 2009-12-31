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


/*-----------------------------------------------------------------------
 * Sum callback implementation
 */

typedef struct _sum_callback
{
    push_callback_t  base;
    uint32_t  sum;
} sum_callback_t;


static ssize_t
sum_callback_process_bytes(push_parser_t *parser,
                           push_callback_t *pcallback,
                           const void *buf,
                           size_t bytes_available)
{
    sum_callback_t  *callback = (sum_callback_t *) pcallback;

    PUSH_DEBUG_MSG("Processing %zu bytes at %p.\n",
                   bytes_available, buf);

    while (bytes_available >= sizeof(uint32_t))
    {
        const uint32_t  *next_int = (const uint32_t *) buf;
        buf += sizeof(uint32_t);
        bytes_available -= sizeof(uint32_t);

        PUSH_DEBUG_MSG("Adding %"PRIu32".\n", *next_int);

        callback->sum += *next_int;
    }

    return bytes_available;
}


static sum_callback_t *
sum_callback_new()
{
    sum_callback_t  *result =
        (sum_callback_t *) malloc(sizeof(sum_callback_t));

    if (result == NULL)
        return NULL;

    push_callback_init(&result->base,
                       sizeof(uint32_t),
                       sizeof(uint32_t),
                       sum_callback_process_bytes,
                       push_eof_allowed,
                       NULL);

    result->sum = 0;

    return result;
}


/*-----------------------------------------------------------------------
 * Sample data
 */

const uint32_t  DATA_01[] = { 1, 2, 3, 4, 5 };
const size_t  LENGTH_01 = 5 * sizeof(uint32_t);


/*-----------------------------------------------------------------------
 * Test cases
 */


START_TEST(test_sum_01)
{
    push_parser_t  *parser;
    sum_callback_t  *callback;

    callback = sum_callback_new();
    fail_if(callback == NULL,
            "Could not allocate a new sum callback");

    parser = push_parser_new(&callback->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    fail_unless(callback->sum == 15,
                "Sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                callback->sum, 15);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_sum_02)
{
    push_parser_t  *parser;
    sum_callback_t  *callback;

    /*
     * If we submit the data twice, we should get twice the result.
     */

    callback = sum_callback_new();
    fail_if(callback == NULL,
            "Could not allocate a new sum callback");

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

    fail_unless(callback->sum == 30,
                "Sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                callback->sum, 30);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_sum_03)
{
    push_parser_t  *parser;
    sum_callback_t  *callback;

    /*
     * Basically the same as test_sum_01, but this one turns off the
     * callback's maximum, so it should process all in one go.  End
     * result should be the same.
     */

    callback = sum_callback_new();
    fail_if(callback == NULL,
            "Could not allocate a new sum callback");

    callback->base.max_bytes_requested = 0;

    parser = push_parser_new(&callback->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    fail_unless(callback->sum == 15,
                "Sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                callback->sum, 15);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_misaligned_data)
{
    push_parser_t  *parser;
    sum_callback_t  *callback;
    size_t  FIRST_CHUNK_SIZE = 7; /* something not divisible by 4 */

    /*
     * Our callback processes ints on nice 32-bit boundaries.  If we
     * send the parser data that doesn't align with these boundaries,
     * we should still get the right answer.
     */

    callback = sum_callback_new();
    fail_if(callback == NULL,
            "Could not allocate a new sum callback");

    parser = push_parser_new(&callback->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, FIRST_CHUNK_SIZE) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_submit_data
                (parser,
                 ((void *) DATA_01) + FIRST_CHUNK_SIZE,
                 LENGTH_01 - FIRST_CHUNK_SIZE) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    fail_unless(callback->sum == 15,
                "Sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                callback->sum, 15);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_parse_error_01)
{
    push_parser_t  *parser;
    sum_callback_t  *callback;
    size_t  FIRST_CHUNK_SIZE = 7; /* something not divisible by 4 */

    /*
     * Our callback processes ints on nice 32-bit boundaries.  If we
     * send the parser data that doesn't align with these boundaries,
     * and then reach EOF, we should get a parse error.
     */

    callback = sum_callback_new();
    fail_if(callback == NULL,
            "Could not allocate a new sum callback");

    parser = push_parser_new(&callback->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, FIRST_CHUNK_SIZE) == PUSH_SUCCESS,
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
    Suite  *s = suite_create("sum-callback");

    TCase  *tc = tcase_create("sum-callback");
    tcase_add_test(tc, test_sum_01);
    tcase_add_test(tc, test_sum_02);
    tcase_add_test(tc, test_sum_03);
    tcase_add_test(tc, test_misaligned_data);
    tcase_add_test(tc, test_parse_error_01);
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

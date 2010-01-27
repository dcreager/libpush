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
#include <push/bind.h>


/*-----------------------------------------------------------------------
 * Sum callback implementation
 */

typedef struct _sum_callback
{
    push_callback_t  base;
    uint32_t  sum;
} sum_callback_t;


static push_error_code_t
sum_callback_activate(push_parser_t *parser,
                      push_callback_t *pcallback,
                      void *vinput)
{
    sum_callback_t  *callback = (sum_callback_t *) pcallback;
    uint32_t  *input = (uint32_t *) vinput;

    PUSH_DEBUG_MSG("sum: Activating callback.  "
                   "Received value %"PRIu32".\n",
                   *input);
    callback->sum = *input;
    return PUSH_SUCCESS;
}


static ssize_t
sum_callback_process_bytes(push_parser_t *parser,
                           push_callback_t *pcallback,
                           const void *buf,
                           size_t bytes_available)
{
    sum_callback_t  *callback = (sum_callback_t *) pcallback;

    PUSH_DEBUG_MSG("sum: Processing %zu bytes at %p.\n",
                   bytes_available, buf);

    if (bytes_available < sizeof(uint32_t))
    {
        return PUSH_PARSE_ERROR;
    } else {
        const uint32_t  *next_int = (const uint32_t *) buf;
        buf += sizeof(uint32_t);
        bytes_available -= sizeof(uint32_t);

        callback->sum += *next_int;
        PUSH_DEBUG_MSG("sum: Adding %"PRIu32".  "
                       "Sum is now %"PRIu32".\n",
                       *next_int, callback->sum);

        return bytes_available;
    }
}


static sum_callback_t *
sum_callback_new()
{
    sum_callback_t  *result =
        (sum_callback_t *) malloc(sizeof(sum_callback_t));

    if (result == NULL)
        return NULL;

    push_callback_init(&result->base,
                       sum_callback_activate,
                       sum_callback_process_bytes,
                       NULL);

    result->sum = 0;
    result->base.result = &result->sum;

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
    sum_callback_t  *sum1;
    uint32_t  input = 0;
    uint32_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_sum_01\n");

    sum1 = sum_callback_new();
    fail_if(sum1 == NULL,
            "Could not allocate a new sum callback");

    parser = push_parser_new(&sum1->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_activate(parser, &input) == PUSH_SUCCESS,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = (uint32_t *) sum1->base.result;

    fail_unless(*result == 1,
                "Sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *result, 1);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_sum_02)
{
    push_parser_t  *parser;
    sum_callback_t  *sum1;
    sum_callback_t  *sum2;
    push_bind_t  *bind;
    uint32_t  input = 0;
    uint32_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_sum_02\n");

    sum1 = sum_callback_new();
    fail_if(sum1 == NULL,
            "Could not allocate first sum callback");

    sum2 = sum_callback_new();
    fail_if(sum2 == NULL,
            "Could not allocate second sum callback");

    bind = push_bind_new(&sum1->base, &sum2->base);
    fail_if(bind == NULL,
            "Could not allocate bind callback");

    parser = push_parser_new(&bind->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_activate(parser, &input) == PUSH_SUCCESS,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = (uint32_t *) bind->base.result;

    fail_unless(*result == 3,
                "Sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *result, 3);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_sum_05)
{
    push_parser_t  *parser;
    sum_callback_t  *sum1;
    sum_callback_t  *sum2;
    sum_callback_t  *sum3;
    sum_callback_t  *sum4;
    sum_callback_t  *sum5;
    push_bind_t  *bind;
    uint32_t  input = 0;
    uint32_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_sum_05\n");

    sum1 = sum_callback_new();
    fail_if(sum1 == NULL,
            "Could not allocate first sum callback");

    sum2 = sum_callback_new();
    fail_if(sum2 == NULL,
            "Could not allocate second sum callback");

    sum3 = sum_callback_new();
    fail_if(sum3 == NULL,
            "Could not allocate third sum callback");

    sum4 = sum_callback_new();
    fail_if(sum4 == NULL,
            "Could not allocate fourth sum callback");

    sum5 = sum_callback_new();
    fail_if(sum5 == NULL,
            "Could not allocate fifth sum callback");

    bind = push_bind_new(&sum1->base, &sum2->base);
    fail_if(bind == NULL,
            "Could not allocate bind callback");

    bind = push_bind_new(&bind->base, &sum3->base);
    fail_if(bind == NULL,
            "Could not allocate bind callback");

    bind = push_bind_new(&bind->base, &sum4->base);
    fail_if(bind == NULL,
            "Could not allocate bind callback");

    bind = push_bind_new(&bind->base, &sum5->base);
    fail_if(bind == NULL,
            "Could not allocate bind callback");

    parser = push_parser_new(&bind->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_activate(parser, &input) == PUSH_SUCCESS,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = (uint32_t *) bind->base.result;

    fail_unless(*result == 15,
                "Sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *result, 15);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_parse_error_01)
{
    push_parser_t  *parser;
    sum_callback_t  *sum1;
    sum_callback_t  *sum2;
    push_bind_t  *bind;
    uint32_t  input = 0;
    size_t  FIRST_CHUNK_SIZE = 3; /* something not divisible by 4 */

    PUSH_DEBUG_MSG("---\nStarting test_parse_error_01\n");

    /*
     * Our callback processes ints on nice 32-bit boundaries.  If we
     * send the parser data that doesn't align with these boundaries,
     * and then reach EOF, we should get a parse error.
     */

    sum1 = sum_callback_new();
    fail_if(sum1 == NULL,
            "Could not allocate first sum callback");

    sum2 = sum_callback_new();
    fail_if(sum2 == NULL,
            "Could not allocate second sum callback");

    bind = push_bind_new(&sum1->base, &sum2->base);
    fail_if(bind == NULL,
            "Could not allocate bind callback");

    parser = push_parser_new(&bind->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_activate(parser, &input) == PUSH_SUCCESS,
                "Could not activate parser");

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
    sum_callback_t  *sum1;
    sum_callback_t  *sum2;
    push_bind_t  *bind;
    uint32_t  input = 0;
    size_t  FIRST_CHUNK_SIZE = 7; /* something not divisible by 4 */

    PUSH_DEBUG_MSG("---\nStarting test_parse_error_02\n");

    /*
     * Our callback processes ints on nice 32-bit boundaries.  If we
     * send the parser data that doesn't align with these boundaries,
     * and then reach EOF, we should get a parse error.
     */

    sum1 = sum_callback_new();
    fail_if(sum1 == NULL,
            "Could not allocate first sum callback");

    sum2 = sum_callback_new();
    fail_if(sum2 == NULL,
            "Could not allocate second sum callback");

    bind = push_bind_new(&sum1->base, &sum2->base);
    fail_if(bind == NULL,
            "Could not allocate bind callback");

    parser = push_parser_new(&bind->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_activate(parser, &input) == PUSH_SUCCESS,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, FIRST_CHUNK_SIZE) == PUSH_PARSE_ERROR,
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
    tcase_add_test(tc, test_sum_05);
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

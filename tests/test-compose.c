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
#include <push/combinators.h>
#include <push/talloc.h>


/*-----------------------------------------------------------------------
 * Sum callback implementation
 */

typedef struct _sum
{
    /**
     * The push_callback_t superclass for this callback.
     */

    push_callback_t  callback;

    /**
     * The continue continuation that will resume the EOF parser.
     */

    push_continue_continuation_t  cont;

    /**
     * The calculated sum.
     */

    uint32_t  sum;

} sum_t;


static void
sum_continue(void *user_data,
             void *buf,
             size_t bytes_remaining)
{
    sum_t  *sum = (sum_t *) user_data;

    PUSH_DEBUG_MSG("%s: Processing %zu bytes at %p.\n",
                   push_talloc_get_name(sum),
                   bytes_remaining, buf);

    if (bytes_remaining < sizeof(uint32_t))
    {
        push_continuation_call(sum->callback.error,
                               PUSH_PARSE_ERROR,
                               "Need more bytes to parse an integer");

        return;
    } else {
        uint32_t  *next_integer = (uint32_t *) buf;
        buf += sizeof(uint32_t);
        bytes_remaining -= sizeof(uint32_t);

        sum->sum += *next_integer;

        PUSH_DEBUG_MSG("%s: Adding %"PRIu32".  "
                       "Sum is now %"PRIu32".\n",
                       push_talloc_get_name(sum),
                       *next_integer, sum->sum);
        PUSH_DEBUG_MSG("%s: Returning %zu bytes.\n",
                       push_talloc_get_name(sum),
                       bytes_remaining);

        push_continuation_call(sum->callback.success,
                               &sum->sum,
                               buf,
                               bytes_remaining);

        return;
    }
}


static void
sum_activate(void *user_data,
             void *result,
             void *buf,
             size_t bytes_remaining)
{
    sum_t  *sum = (sum_t *) user_data;
    uint32_t  *input = (uint32_t *) result;

    PUSH_DEBUG_MSG("%s: Activating callback.  "
                   "Received value %"PRIu32".\n",
                   push_talloc_get_name(sum),
                   *input);
    sum->sum = *input;

    if (bytes_remaining == 0)
    {
        /*
         * If we don't get any data when we're activated, return an
         * incomplete and wait for some data.
         */

        push_continuation_call(sum->callback.incomplete,
                               &sum->cont);

        return;

    } else {
        /*
         * Otherwise let the continue continuation go ahead and
         * process this chunk of data.
         */

        sum_continue(user_data, buf, bytes_remaining);
        return;
    }
}


static push_callback_t *
sum_callback_new(const char *name,
                 void *parent,
                 push_parser_t *parser)
{
    sum_t  *sum = push_talloc(parent, sum_t);

    if (sum == NULL)
        return NULL;

    if (name == NULL) name = "sum";
    push_talloc_set_name_const(sum, name);

    push_callback_init(&sum->callback, parser, sum,
                       sum_activate,
                       NULL, NULL, NULL);

    /*
     * Fill in the continuation objects for the continuations that we
     * implement.
     */

    push_continuation_set(&sum->cont,
                          sum_continue,
                          sum);

    return &sum->callback;
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
    push_callback_t  *sum1;
    uint32_t  input = 0;
    uint32_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_sum_01\n");

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    sum1 = sum_callback_new("sum1", NULL, parser);
    fail_if(sum1 == NULL,
            "Could not allocate a new sum callback");

    push_parser_set_callback(parser, sum1);

    fail_unless(push_parser_activate(parser, &input) == PUSH_INCOMPLETE,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = push_parser_result(parser, uint32_t);

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
    push_callback_t  *sum1;
    push_callback_t  *sum2;
    push_callback_t  *compose;
    uint32_t  input = 0;
    uint32_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_sum_02\n");

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    sum1 = sum_callback_new("sum1", NULL, parser);
    fail_if(sum1 == NULL,
            "Could not allocate first sum callback");

    sum2 = sum_callback_new("sum1", NULL, parser);
    fail_if(sum2 == NULL,
            "Could not allocate second sum callback");

    compose = push_compose_new("compose", NULL, parser, sum1, sum2);
    fail_if(compose == NULL,
            "Could not allocate compose callback");

    push_parser_set_callback(parser, compose);

    fail_unless(push_parser_activate(parser, &input) == PUSH_INCOMPLETE,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = push_parser_result(parser, uint32_t);

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
    push_callback_t  *sum[5];
    push_callback_t  *compose;
    uint32_t  input = 0;
    uint32_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_sum_05\n");

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    sum[0] = sum_callback_new("sum1", NULL, parser);
    fail_if(sum[0] == NULL,
            "Could not allocate first sum callback");

    sum[1] = sum_callback_new("sum2", NULL, parser);
    fail_if(sum[1] == NULL,
            "Could not allocate second sum callback");

    sum[2] = sum_callback_new("sum3", NULL, parser);
    fail_if(sum[2] == NULL,
            "Could not allocate third sum callback");

    sum[3] = sum_callback_new("sum4", NULL, parser);
    fail_if(sum[3] == NULL,
            "Could not allocate fourth sum callback");

    sum[4] = sum_callback_new("sum5", NULL, parser);
    fail_if(sum[4] == NULL,
            "Could not allocate fifth sum callback");

    compose = push_compose_new("compose1", NULL, parser, sum[0], sum[1]);
    fail_if(compose == NULL,
            "Could not allocate compose callback");

    compose = push_compose_new("compose2", NULL, parser, compose, sum[2]);
    fail_if(compose == NULL,
            "Could not allocate compose callback");

    compose = push_compose_new("compose3", NULL, parser, compose, sum[3]);
    fail_if(compose == NULL,
            "Could not allocate compose callback");

    compose = push_compose_new("compose4", NULL, parser, compose, sum[4]);
    fail_if(compose == NULL,
            "Could not allocate compose callback");

    push_parser_set_callback(parser, compose);

    fail_unless(push_parser_activate(parser, &input) == PUSH_INCOMPLETE,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = push_parser_result(parser, uint32_t);

    fail_unless(*result == 15,
                "Sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *result, 15);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_wrapped_sum_05)
{
    push_parser_t  *parser;
    push_callback_t  *sum[5];
    push_callback_t  *wrapped[5];
    push_callback_t  *compose;
    uint32_t  input = 0;
    uint32_t  *result;
    size_t  FIRST_CHUNK_SIZE = 7; /* something not divisible by 4 */

    PUSH_DEBUG_MSG("---\nStarting test_wrapped_sum_05\n");

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    sum[0] = sum_callback_new("sum1", NULL, parser);
    fail_if(sum[0] == NULL,
            "Could not allocate first sum callback");

    wrapped[0] = push_min_bytes_new("min-bytes1", NULL, parser, sum[0],
                                    sizeof(uint32_t));
    fail_if(wrapped[0] == NULL,
            "Could not allocate first min-bytes callback");

    sum[1] = sum_callback_new("sum2", NULL, parser);
    fail_if(sum[1] == NULL,
            "Could not allocate second sum callback");

    wrapped[1] = push_min_bytes_new("min-bytes2", NULL, parser, sum[1],
                                    sizeof(uint32_t));
    fail_if(wrapped[1] == NULL,
            "Could not allocate second min-bytes callback");

    sum[2] = sum_callback_new("sum3", NULL, parser);
    fail_if(sum[2] == NULL,
            "Could not allocate third sum callback");

    wrapped[2] = push_min_bytes_new("min-bytes3", NULL, parser, sum[2],
                                    sizeof(uint32_t));
    fail_if(wrapped[2] == NULL,
            "Could not allocate third min-bytes callback");

    sum[3] = sum_callback_new("sum4", NULL, parser);
    fail_if(sum[3] == NULL,
            "Could not allocate fourth sum callback");

    wrapped[3] = push_min_bytes_new("min-bytes4", NULL, parser, sum[3],
                                    sizeof(uint32_t));
    fail_if(wrapped[3] == NULL,
            "Could not allocate fourth min-bytes callback");

    sum[4] = sum_callback_new("sum5", NULL, parser);
    fail_if(sum[4] == NULL,
            "Could not allocate fifth sum callback");

    wrapped[4] = push_min_bytes_new("min-bytes5", NULL, parser, sum[4],
                                    sizeof(uint32_t));
    fail_if(wrapped[4] == NULL,
            "Could not allocate fifth min-bytes callback");

    compose = push_compose_new("compose1", NULL, parser, wrapped[0], wrapped[1]);
    fail_if(compose == NULL,
            "Could not allocate compose callback");

    compose = push_compose_new("compose2", NULL, parser, compose, wrapped[2]);
    fail_if(compose == NULL,
            "Could not allocate compose callback");

    compose = push_compose_new("compose3", NULL, parser, compose, wrapped[3]);
    fail_if(compose == NULL,
            "Could not allocate compose callback");

    compose = push_compose_new("compose4", NULL, parser, compose, wrapped[4]);
    fail_if(compose == NULL,
            "Could not allocate compose callback");

    push_parser_set_callback(parser, compose);

    fail_unless(push_parser_activate(parser, &input) == PUSH_INCOMPLETE,
                "Could not activate parser");

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

    result = push_parser_result(parser, uint32_t);

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
    push_callback_t  *sum1;
    push_callback_t  *sum2;
    push_callback_t  *compose;
    uint32_t  input = 0;
    size_t  FIRST_CHUNK_SIZE = 3; /* something not divisible by 4 */

    PUSH_DEBUG_MSG("---\nStarting test_parse_error_01\n");

    /*
     * Our callback processes ints on nice 32-bit boundaries.  If we
     * send the parser data that doesn't align with these boundaries,
     * and then reach EOF, we should get a parse error.
     */

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    sum1 = sum_callback_new("sum1", NULL, parser);
    fail_if(sum1 == NULL,
            "Could not allocate first sum callback");

    sum2 = sum_callback_new("sum2", NULL, parser);
    fail_if(sum2 == NULL,
            "Could not allocate second sum callback");

    compose = push_compose_new("compose", NULL, parser, sum1, sum2);
    fail_if(compose == NULL,
            "Could not allocate compose callback");

    push_parser_set_callback(parser, compose);

    fail_unless(push_parser_activate(parser, &input) == PUSH_INCOMPLETE,
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
    push_callback_t  *sum1;
    push_callback_t  *sum2;
    push_callback_t  *compose;
    uint32_t  input = 0;
    size_t  FIRST_CHUNK_SIZE = 7; /* something not divisible by 4 */

    PUSH_DEBUG_MSG("---\nStarting test_parse_error_02\n");

    /*
     * Our callback processes ints on nice 32-bit boundaries.  If we
     * send the parser data that doesn't align with these boundaries,
     * and then reach EOF, we should get a parse error.
     */

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    sum1 = sum_callback_new("sum1", NULL, parser);
    fail_if(sum1 == NULL,
            "Could not allocate first sum callback");

    sum2 = sum_callback_new("sum2", NULL, parser);
    fail_if(sum2 == NULL,
            "Could not allocate second sum callback");

    compose = push_compose_new("compose", NULL, parser, sum1, sum2);
    fail_if(compose == NULL,
            "Could not allocate compose callback");

    push_parser_set_callback(parser, compose);

    fail_unless(push_parser_activate(parser, &input) == PUSH_INCOMPLETE,
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
    tcase_add_test(tc, test_wrapped_sum_05);
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

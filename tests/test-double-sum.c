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
#include <push/pairs.h>
#include <push/talloc.h>

#include <test-callbacks.h>


/*-----------------------------------------------------------------------
 * Double-sum callback implementation
 *
 * This callbackcreates two sums from a list of integers — the first
 * is the sum of all of the even-indexed entries, the second the sum
 * of the odd-indexed entries.  We can do this by creating a par of
 * two regular sum callbacks.
 */

static push_callback_t *
make_double_sum_callback(push_parser_t *parser)
{
    void  *context;
    push_callback_t  *sum1;
    push_callback_t  *sum2;
    push_callback_t  *par;
    push_callback_t  *fold;

    context = push_talloc_new(NULL);
    if (context == NULL) return NULL;

    sum1 = sum_callback_new
        ("sum1", context, parser);
    sum2 = sum_callback_new
        ("sum2", context, parser);
    par = push_par_new
        ("par", context, parser, sum1, sum2);
    fold = push_fold_new
        ("fold", context, parser, par);

    if (fold == NULL) goto error;
    return fold;

  error:
    push_talloc_free(context);
    return NULL;
}


/*-----------------------------------------------------------------------
 * Sample data
 */

uint32_t  INT_0 = 0;
push_pair_t  PAIR_0 = { &INT_0, &INT_0 };

const uint32_t  DATA_01[] = { 1, 2, 3, 4, 5, 6 };
const size_t  LENGTH_01 = 6 * sizeof(uint32_t);


/*-----------------------------------------------------------------------
 * Test cases
 */


START_TEST(test_double_sum_01)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
    push_pair_t  *result;
    uint32_t  *sum1;
    uint32_t  *sum2;

    PUSH_DEBUG_MSG("---\nStarting test_double_sum_01\n");

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    callback = make_double_sum_callback(parser);
    fail_if(callback == NULL,
            "Could not allocate double-sum callback");

    push_parser_set_callback(parser, callback);

    fail_unless(push_parser_activate(parser, &PAIR_0)
                == PUSH_INCOMPLETE,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_INCOMPLETE,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = push_parser_result(parser, push_pair_t);
    sum1 = (uint32_t *) result->first;
    sum2 = (uint32_t *) result->second;

    fail_unless(*sum1 == 9,
                "Even sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *sum1, 9);

    fail_unless(*sum2 == 12,
                "Odd sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *sum2, 12);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_double_sum_02)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
    push_pair_t  *result;
    uint32_t  *sum1;
    uint32_t  *sum2;

    PUSH_DEBUG_MSG("---\nStarting test_double_sum_02\n");

    /*
     * If we submit the data twice, we should get twice the result.
     */

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    callback = make_double_sum_callback(parser);
    fail_if(callback == NULL,
            "Could not allocate double-sum callback");

    push_parser_set_callback(parser, callback);

    fail_unless(push_parser_activate(parser, &PAIR_0)
                == PUSH_INCOMPLETE,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_INCOMPLETE,
                "Could not parse data");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_INCOMPLETE,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = push_parser_result(parser, push_pair_t);
    sum1 = (uint32_t *) result->first;
    sum2 = (uint32_t *) result->second;

    fail_unless(*sum1 == 18,
                "Even sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *sum1, 18);

    fail_unless(*sum2 == 24,
                "Odd sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *sum2, 24);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_double_sum_03)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
    push_pair_t  *result;
    uint32_t  *sum1;
    uint32_t  *sum2;

    PUSH_DEBUG_MSG("---\nStarting test_double_sum_03\n");

    /*
     * Basically the same as test_double_sum_01, but this one turns
     * off the callback's maximum, so it should process all in one go.
     * End result should be the same.
     */

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    callback = make_double_sum_callback(parser);
    fail_if(callback == NULL,
            "Could not allocate double-sum callback");

    push_parser_set_callback(parser, callback);

    fail_unless(push_parser_activate(parser, &PAIR_0)
                == PUSH_INCOMPLETE,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_INCOMPLETE,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = push_parser_result(parser, push_pair_t);
    sum1 = (uint32_t *) result->first;
    sum2 = (uint32_t *) result->second;

    fail_unless(*sum1 == 9,
                "Even sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *sum1, 9);

    fail_unless(*sum2 == 12,
                "Odd sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *sum2, 12);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_misaligned_data)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
    push_pair_t  *result;
    uint32_t  *sum1;
    uint32_t  *sum2;
    size_t  FIRST_CHUNK_SIZE = 7; /* something not divisible by 4 */

    PUSH_DEBUG_MSG("---\nStarting test_misaligned_data\n");

    /*
     * Our callback processes ints on nice 32-bit boundaries.  If we
     * send the parser data that doesn't align with these boundaries,
     * we should still get the right answer.
     */

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    callback = make_double_sum_callback(parser);
    fail_if(callback == NULL,
            "Could not allocate double-sum callback");

    push_parser_set_callback(parser, callback);

    fail_unless(push_parser_activate(parser, &PAIR_0)
                == PUSH_INCOMPLETE,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, FIRST_CHUNK_SIZE) == PUSH_INCOMPLETE,
                "Could not parse data");

    fail_unless(push_parser_submit_data
                (parser,
                 ((void *) DATA_01) + FIRST_CHUNK_SIZE,
                 LENGTH_01 - FIRST_CHUNK_SIZE) == PUSH_INCOMPLETE,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = push_parser_result(parser, push_pair_t);
    sum1 = (uint32_t *) result->first;
    sum2 = (uint32_t *) result->second;

    fail_unless(*sum1 == 9,
                "Even sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *sum1, 9);

    fail_unless(*sum2 == 12,
                "Odd sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *sum2, 12);

    push_parser_free(parser);
}
END_TEST


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
test_suite()
{
    Suite  *s = suite_create("double-sum-callback");

    TCase  *tc = tcase_create("double-sum-callback");
    tcase_add_test(tc, test_double_sum_01);
    tcase_add_test(tc, test_double_sum_02);
    tcase_add_test(tc, test_double_sum_03);
    tcase_add_test(tc, test_misaligned_data);
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

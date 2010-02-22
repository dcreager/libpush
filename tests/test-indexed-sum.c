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

#include <test-callbacks.h>


/*-----------------------------------------------------------------------
 * Indexed-sum callback implementation
 *
 * This callback tests the PUSH_PARSE_ERROR functionality.  Unlike
 * test-sum-callback, where there's just a simple list of integers, we
 * now have a list of <i>pairs</i> of integers.  The first integer in
 * each pair is an “index”, and specifies which sum the pair should be
 * added to.  The second integer in the pair is what's actually added
 * to the sum.
 *
 * The index must be 0 or 1 for this test case — since there are only
 * two sums — and so we can test raising a PUSH_PARSE_ERROR when we
 * see other values.
 */

#define NUM_SUM_CALLBACKS 2

static push_callback_t *
make_indexed_sum_callback(push_parser_t *parser)
{
    push_callback_t  *sum;
    push_callback_t  *fold;

    sum = indexed_sum_callback_new(parser, NUM_SUM_CALLBACKS);
    fold = push_fold_new(parser, sum);

    return fold;
}


/*-----------------------------------------------------------------------
 * Sample data
 */

const uint32_t  DATA_01[] =
{
    0, 1,
    1, 2,
    0, 3,
    1, 4,
    0, 5
};
const size_t  LENGTH_01 = 10 * sizeof(uint32_t);


const uint32_t  DATA_02[] =
{
    0, 1,
    1, 2,
    2, 3,  /* parse error here */
    3, 4,
    4, 5
};
const size_t  LENGTH_02 = 10 * sizeof(uint32_t);


const uint32_t  DATA_03[] =
{
    0, 1,
    1      /* parse error here */
};
const size_t  LENGTH_03 = 3 * sizeof(uint32_t);


/*-----------------------------------------------------------------------
 * Test cases
 */


START_TEST(test_indexed_sum_01)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
    uint32_t  sum[NUM_SUM_CALLBACKS];
    uint32_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_indexed_sum_01\n");

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    callback = make_indexed_sum_callback(parser);
    fail_if(callback == NULL,
            "Could not allocate index callback");

    push_parser_set_callback(parser, callback);

    memset(sum, 0, sizeof(uint32_t) * NUM_SUM_CALLBACKS);
    fail_unless(push_parser_activate(parser, &sum)
                == PUSH_INCOMPLETE,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_INCOMPLETE,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = push_parser_result(parser, uint32_t);

    fail_unless(result[0] == 9,
                "Sum 0 doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                result[0], 9);

    fail_unless(result[1] == 6,
                "Sum 1 doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                result[1], 6);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_indexed_sum_02)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
    uint32_t  sum[NUM_SUM_CALLBACKS];
    uint32_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_indexed_sum_02\n");

    /*
     * If we submit the data twice, we should get twice the result.
     */

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    callback = make_indexed_sum_callback(parser);
    fail_if(callback == NULL,
            "Could not allocate index callback");

    push_parser_set_callback(parser, callback);

    memset(sum, 0, sizeof(uint32_t) * NUM_SUM_CALLBACKS);
    fail_unless(push_parser_activate(parser, &sum)
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

    result = push_parser_result(parser, uint32_t);

    fail_unless(result[0] == 18,
                "Sum 0 doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                result[0], 18);

    fail_unless(result[1] == 12,
                "Sum 1 doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                result[1], 12);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_misaligned_data)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
    uint32_t  sum[NUM_SUM_CALLBACKS];
    uint32_t  *result;
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

    callback = make_indexed_sum_callback(parser);
    fail_if(callback == NULL,
            "Could not allocate index callback");

    push_parser_set_callback(parser, callback);

    memset(sum, 0, sizeof(uint32_t) * NUM_SUM_CALLBACKS);
    fail_unless(push_parser_activate(parser, sum)
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

    result = push_parser_result(parser, uint32_t);

    fail_unless(result[0] == 9,
                "Sum 0 doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                result[0], 9);

    fail_unless(result[1] == 6,
                "Sum 1 doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                result[1], 6);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_parse_error_01)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
    uint32_t  sum[NUM_SUM_CALLBACKS];
    uint32_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_parse_error_01\n");

    /*
     * Tests that we get a parse error when the index is out of range.
     * Since it's wrapped up in a fold, though, this will just cause
     * the overall callback to succeed after the most recent good
     * pair.
     */

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    callback = make_indexed_sum_callback(parser);
    fail_if(callback == NULL,
            "Could not allocate index callback");

    push_parser_set_callback(parser, callback);

    memset(sum, 0, sizeof(uint32_t) * NUM_SUM_CALLBACKS);
    fail_unless(push_parser_activate(parser, &sum)
                == PUSH_INCOMPLETE,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_02, LENGTH_02) == PUSH_SUCCESS,
                "Should get a parse error");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = push_parser_result(parser, uint32_t);

    fail_unless(result[0] == 1,
                "Sum 0 doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                result[0], 1);

    fail_unless(result[1] == 2,
                "Sum 1 doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                result[1], 2);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_parse_error_02)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
    uint32_t  sum[NUM_SUM_CALLBACKS];
    size_t  FIRST_CHUNK_SIZE = /* something in the 5th integer */
        4 * sizeof(uint32_t) + 2;

    PUSH_DEBUG_MSG("---\nStarting test_parse_error_02\n");

    /*
     * Tests that we get a parse error when the index is out of range.
     * Unlike the previous test, we want the fold to get a parse
     * error.  To do this, we send in the data in chunks; making sure
     * that the boundary occurs within the 5th integer (the index that
     * causes the parse error).  This makes the parse error occur
     * after a PUSH_INCOMPLETE, which isn't allowed, and causes the
     * fold to generate a parse error.
     */

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    callback = make_indexed_sum_callback(parser);
    fail_if(callback == NULL,
            "Could not allocate index callback");

    push_parser_set_callback(parser, callback);

    memset(sum, 0, sizeof(uint32_t) * NUM_SUM_CALLBACKS);
    fail_unless(push_parser_activate(parser, &sum)
                == PUSH_INCOMPLETE,
                "Could not activate parser");

    PUSH_DEBUG_MSG("test: Sending in first chunk.\n");

    fail_unless(push_parser_submit_data
                (parser, &DATA_02, FIRST_CHUNK_SIZE) == PUSH_INCOMPLETE,
                "Could not parse data");

    PUSH_DEBUG_MSG("test: Sending in second chunk.\n");

    fail_unless(push_parser_submit_data
                (parser,
                 ((void *) DATA_02) + FIRST_CHUNK_SIZE,
                 LENGTH_02 - FIRST_CHUNK_SIZE) == PUSH_PARSE_ERROR,
                "Should get a parse error");

    push_parser_free(parser);
}
END_TEST


START_TEST(test_parse_error_03)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
    uint32_t  sum[NUM_SUM_CALLBACKS];

    PUSH_DEBUG_MSG("---\nStarting test_parse_error_03\n");

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    callback = make_indexed_sum_callback(parser);
    fail_if(callback == NULL,
            "Could not allocate index callback");

    push_parser_set_callback(parser, callback);

    memset(sum, 0, sizeof(uint32_t) * NUM_SUM_CALLBACKS);
    fail_unless(push_parser_activate(parser, &sum)
                == PUSH_INCOMPLETE,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_03, LENGTH_03) == PUSH_INCOMPLETE,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_PARSE_ERROR,
                "Should get a parse error at EOF");

    push_parser_free(parser);
}
END_TEST


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
test_suite()
{
    Suite  *s = suite_create("indexed-sum-callback");

    TCase  *tc = tcase_create("indexed-sum-callback");
    tcase_add_test(tc, test_indexed_sum_01);
    tcase_add_test(tc, test_indexed_sum_02);
    tcase_add_test(tc, test_misaligned_data);
    tcase_add_test(tc, test_parse_error_01);
    tcase_add_test(tc, test_parse_error_02);
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

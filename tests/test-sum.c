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
#include <push/tuples.h>

#include <test-callbacks.h>


/*-----------------------------------------------------------------------
 * Folded sum callback
 */

static push_callback_t *
make_repeated_sum(push_parser_t *parser)
{
    void  *context;
    push_callback_t  *sum;
    push_callback_t  *fold;

    context = push_talloc_new(NULL);
    if (context == NULL) return NULL;

    sum = sum_callback_new
        ("sum", context, parser);
    fold = push_fold_new
        ("fold", context, parser, sum);

    if (fold == NULL) goto error;
    return fold;

  error:
    push_talloc_free(context);
    return NULL;
}

static push_callback_t *
make_repeated_max_sum(push_parser_t *parser)
{
    void  *context;
    push_callback_t  *sum;
    push_callback_t  *fold1;
    push_callback_t  *max_bytes;
    push_callback_t  *fold2;

    context = push_talloc_new(NULL);
    if (context == NULL) return NULL;

    sum = sum_callback_new
        ("sum", context, parser);
    fold1 = push_fold_new
        ("fold1", context, parser, sum);
    max_bytes = push_max_bytes_new
        ("max-bytes", context, parser,
         fold1, sizeof(uint32_t));
    fold2 = push_fold_new
        ("fold2", context, parser, max_bytes);

    if (fold2 == NULL) goto error;
    return fold2;

  error:
    push_talloc_free(context);
    return NULL;
}


/*-----------------------------------------------------------------------
 * Sample data
 */

uint32_t  INT_0 = 0;

size_t  MAX_SIZE_01 = sizeof(uint32_t) * 3;
push_pair_t  MAX_INPUT_01 = { &MAX_SIZE_01, &INT_0 };

size_t  MAX_SIZE_02 = sizeof(uint32_t) * 2;
push_pair_t  MAX_INPUT_02 = { &MAX_SIZE_02, &INT_0 };

const uint32_t  DATA_01[] = { 1, 2, 3, 4, 5 };
const size_t  LENGTH_01 = 5 * sizeof(uint32_t);


/*-----------------------------------------------------------------------
 * Test cases
 */


START_TEST(test_sum_01)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
    uint32_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_sum_01\n");

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    callback = make_repeated_sum(parser);
    fail_if(callback == NULL,
            "Could not allocate a new sum callback");

    push_parser_set_callback(parser, callback);

    fail_unless(push_parser_activate(parser, &INT_0)
                == PUSH_INCOMPLETE,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_INCOMPLETE,
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


START_TEST(test_sum_02)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
    uint32_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_sum_02\n");

    /*
     * If we submit the data twice, we should get twice the result.
     */

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    callback = make_repeated_sum(parser);
    fail_if(callback == NULL,
            "Could not allocate a new sum callback");

    push_parser_set_callback(parser, callback);

    fail_unless(push_parser_activate(parser, &INT_0)
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

    fail_unless(*result == 30,
                "Sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *result, 30);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_misaligned_data)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
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

    callback = make_repeated_sum(parser);
    fail_if(callback == NULL,
            "Could not allocate a new sum callback");

    push_parser_set_callback(parser, callback);

    fail_unless(push_parser_activate(parser, &INT_0)
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
    push_callback_t  *callback;
    size_t  FIRST_CHUNK_SIZE = 7; /* something not divisible by 4 */

    PUSH_DEBUG_MSG("---\nStarting test_parse_error_01\n");

    /*
     * Our callback processes ints on nice 32-bit boundaries.  If we
     * send the parser data that doesn't align with these boundaries,
     * and then reach EOF, we should get a parse error.
     */

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    callback = make_repeated_sum(parser);
    fail_if(callback == NULL,
            "Could not allocate a new sum callback");

    push_parser_set_callback(parser, callback);

    fail_unless(push_parser_activate(parser, &INT_0)
                == PUSH_INCOMPLETE,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, FIRST_CHUNK_SIZE) == PUSH_INCOMPLETE,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_PARSE_ERROR,
                "Should get parse error at EOF");

    push_parser_free(parser);
}
END_TEST


START_TEST(test_max_01)
{
    push_parser_t  *parser;
    push_callback_t  *sum;
    push_callback_t  *callback;
    uint32_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_max_01\n");

    /*
     * If we use max-bytes to limit ourselves to three numbers, we
     * should get a smaller sum.
     */

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    sum = make_repeated_sum(parser);
    fail_if(sum == NULL,
            "Could not allocate a new sum callback");

    callback = push_max_bytes_new("max-bytes", NULL, parser, sum, MAX_SIZE_01);
    fail_if(callback == NULL,
            "Could not allocate a new max-bytes callback");

    push_parser_set_callback(parser, callback);

    fail_unless(push_parser_activate(parser, &INT_0)
                == PUSH_INCOMPLETE,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = push_parser_result(parser, uint32_t);

    fail_unless(*result == 6,
                "Sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *result, 6);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_max_02)
{
    push_parser_t  *parser;
    push_callback_t  *sum1;
    push_callback_t  *sum2;
    push_callback_t  *max1;
    push_callback_t  *max2;
    push_callback_t  *callback;
    push_pair_t  *pair;
    uint32_t  *result1;
    uint32_t  *result2;

    PUSH_DEBUG_MSG("---\nStarting test_max_02\n");

    /*
     * If we use max-bytes to limit ourselves to two numbers, we
     * should get a smaller sum.  Then we repeat to get another
     * smaller sum.
     */

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    sum1 = make_repeated_sum(parser);
    fail_if(sum1 == NULL,
            "Could not allocate a new sum callback");

    max1 = push_max_bytes_new("max-bytes", NULL, parser, sum1, MAX_SIZE_02);
    fail_if(max1 == NULL,
            "Could not allocate a new max-bytes callback");

    sum2 = make_repeated_sum(parser);
    fail_if(sum2 == NULL,
            "Could not allocate a new sum callback");

    max2 = push_max_bytes_new("max-bytes", NULL, parser, sum2, MAX_SIZE_02);
    fail_if(max2 == NULL,
            "Could not allocate a new max-bytes callback");

    callback = push_both_new("both", NULL, parser, max1, max2);
    fail_if(callback == NULL,
            "Could not allocate a new both callback");

    push_parser_set_callback(parser, callback);

    fail_unless(push_parser_activate(parser, &INT_0)
                == PUSH_INCOMPLETE,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    pair = push_parser_result(parser, push_pair_t);
    result1 = (uint32_t *) pair->first;
    result2 = (uint32_t *) pair->second;

    fail_unless(*result1 == 3,
                "Sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *result1, 3);

    fail_unless(*result2 == 7,
                "Sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *result2, 7);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_max_03)
{
    push_parser_t  *parser;
    push_callback_t  *sum;
    push_callback_t  *callback;
    uint32_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_max_03\n");

    /*
     * If we use max-bytes to limit ourselves to three numbers, we
     * should get a smaller sum, even when we only send in three
     * numbers.
     */

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    sum = make_repeated_sum(parser);
    fail_if(sum == NULL,
            "Could not allocate a new sum callback");

    callback = push_max_bytes_new("max-bytes", NULL, parser, sum, MAX_SIZE_01);
    fail_if(callback == NULL,
            "Could not allocate a new max-bytes callback");

    push_parser_set_callback(parser, callback);

    fail_unless(push_parser_activate(parser, &INT_0)
                == PUSH_INCOMPLETE,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, MAX_SIZE_01) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = push_parser_result(parser, uint32_t);

    fail_unless(*result == 6,
                "Sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *result, 6);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_misaligned_max_01)
{
    push_parser_t  *parser;
    push_callback_t  *sum;
    push_callback_t  *callback;
    uint32_t  *result;
    size_t  FIRST_CHUNK_SIZE = 7; /* something not divisible by 4 */

    PUSH_DEBUG_MSG("---\nStarting test_misaligned_max_01\n");

    /*
     * If we use max-bytes to limit ourselves to three numbers, we
     * should get a smaller sum.  If we send in the data in two
     * chunks, we should get the same result as when we send it in as
     * one chunk.
     */

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    sum = make_repeated_sum(parser);
    fail_if(sum == NULL,
            "Could not allocate a new sum callback");

    callback = push_max_bytes_new("max-bytes", NULL, parser, sum, MAX_SIZE_01);
    fail_if(callback == NULL,
            "Could not allocate a new max-bytes callback");

    push_parser_set_callback(parser, callback);

    fail_unless(push_parser_activate(parser, &INT_0)
                == PUSH_INCOMPLETE,
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

    fail_unless(*result == 6,
                "Sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *result, 6);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_misaligned_max_02)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
    uint32_t  *result;
    size_t  FIRST_CHUNK_SIZE = 7; /* something not divisible by 4 */

    PUSH_DEBUG_MSG("---\nStarting test_misaligned_max_02\n");

    /*
     * If we use max-bytes to limit each sum to one number (which
     * doesn't really do anything).  Then we send in the numbers in
     * two chunks, with the boundary misaligned.  We should get the
     * same result as if we sent those numbers in as a single chunk.
     */

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    callback = make_repeated_max_sum(parser);
    fail_if(callback == NULL,
            "Could not allocate a new sum callback");

    push_parser_set_callback(parser, callback);

    fail_unless(push_parser_activate(parser, &INT_0)
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

    fail_unless(*result == 15,
                "Sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *result, 15);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_dynamic_max_01)
{
    push_parser_t  *parser;
    push_callback_t  *sum;
    push_callback_t  *callback;
    uint32_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_dynamic_max_01\n");

    /*
     * If we use max-bytes to limit ourselves to three numbers, we
     * should get a smaller sum.
     */

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    sum = make_repeated_sum(parser);
    fail_if(sum == NULL,
            "Could not allocate a new sum callback");

    callback = push_dynamic_max_bytes_new("max-bytes", NULL, parser, sum);
    fail_if(callback == NULL,
            "Could not allocate a new max-bytes callback");

    push_parser_set_callback(parser, callback);

    fail_unless(push_parser_activate(parser, &MAX_INPUT_01)
                == PUSH_INCOMPLETE,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = push_parser_result(parser, uint32_t);

    fail_unless(*result == 6,
                "Sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *result, 6);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_dynamic_max_02)
{
    push_parser_t  *parser;
    push_callback_t  *sum1;
    push_callback_t  *sum2;
    push_callback_t  *max1;
    push_callback_t  *max2;
    push_callback_t  *callback;
    push_pair_t  *pair;
    uint32_t  *result1;
    uint32_t  *result2;

    PUSH_DEBUG_MSG("---\nStarting test_dynamic_max_02\n");

    /*
     * If we use max-bytes to limit ourselves to two numbers, we
     * should get a smaller sum.  Then we repeat to get another
     * smaller sum.
     */

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    sum1 = make_repeated_sum(parser);
    fail_if(sum1 == NULL,
            "Could not allocate a new sum callback");

    max1 = push_dynamic_max_bytes_new("max-bytes", NULL, parser, sum1);
    fail_if(max1 == NULL,
            "Could not allocate a new max-bytes callback");

    sum2 = make_repeated_sum(parser);
    fail_if(sum2 == NULL,
            "Could not allocate a new sum callback");

    max2 = push_dynamic_max_bytes_new("max-bytes", NULL, parser, sum2);
    fail_if(max2 == NULL,
            "Could not allocate a new max-bytes callback");

    callback = push_both_new("both", NULL, parser, max1, max2);
    fail_if(callback == NULL,
            "Could not allocate a new both callback");

    push_parser_set_callback(parser, callback);

    fail_unless(push_parser_activate(parser, &MAX_INPUT_02)
                == PUSH_INCOMPLETE,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    pair = push_parser_result(parser, push_pair_t);
    result1 = (uint32_t *) pair->first;
    result2 = (uint32_t *) pair->second;

    fail_unless(*result1 == 3,
                "Sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *result1, 3);

    fail_unless(*result2 == 7,
                "Sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *result2, 7);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_dynamic_max_03)
{
    push_parser_t  *parser;
    push_callback_t  *sum;
    push_callback_t  *callback;
    uint32_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_dynamic_max_03\n");

    /*
     * If we use max-bytes to limit ourselves to three numbers, we
     * should get a smaller sum, even if we only send in three
     * numbers.
     */

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    sum = make_repeated_sum(parser);
    fail_if(sum == NULL,
            "Could not allocate a new sum callback");

    callback = push_dynamic_max_bytes_new("max-bytes", NULL, parser, sum);
    fail_if(callback == NULL,
            "Could not allocate a new max-bytes callback");

    push_parser_set_callback(parser, callback);

    fail_unless(push_parser_activate(parser, &MAX_INPUT_01)
                == PUSH_INCOMPLETE,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, MAX_SIZE_01) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = push_parser_result(parser, uint32_t);

    fail_unless(*result == 6,
                "Sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *result, 6);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_misaligned_dynamic_max_01)
{
    push_parser_t  *parser;
    push_callback_t  *sum;
    push_callback_t  *callback;
    uint32_t  *result;
    size_t  FIRST_CHUNK_SIZE = 7; /* something not divisible by 4 */

    PUSH_DEBUG_MSG("---\nStarting test_misaligned_dynamic_max_01\n");

    /*
     * If we use max-bytes to limit ourselves to three numbers, we
     * should get a smaller sum.  If we send in the data in two
     * chunks, we should get the same result as when we send it in as
     * one chunk.
     */

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    sum = make_repeated_sum(parser);
    fail_if(sum == NULL,
            "Could not allocate a new sum callback");

    callback = push_dynamic_max_bytes_new("max-bytes", NULL, parser, sum);
    fail_if(callback == NULL,
            "Could not allocate a new max-bytes callback");

    push_parser_set_callback(parser, callback);

    fail_unless(push_parser_activate(parser, &MAX_INPUT_01)
                == PUSH_INCOMPLETE,
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

    fail_unless(*result == 6,
                "Sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *result, 6);

    push_parser_free(parser);
}
END_TEST


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
test_suite()
{
    Suite  *s = suite_create("sum");

    TCase  *tc = tcase_create("sum");
    tcase_add_test(tc, test_sum_01);
    tcase_add_test(tc, test_sum_02);
    tcase_add_test(tc, test_misaligned_data);
    tcase_add_test(tc, test_parse_error_01);
    tcase_add_test(tc, test_max_01);
    tcase_add_test(tc, test_max_02);
    tcase_add_test(tc, test_max_03);
    tcase_add_test(tc, test_misaligned_max_01);
    tcase_add_test(tc, test_misaligned_max_02);
    tcase_add_test(tc, test_dynamic_max_01);
    tcase_add_test(tc, test_dynamic_max_02);
    tcase_add_test(tc, test_dynamic_max_03);
    tcase_add_test(tc, test_misaligned_dynamic_max_01);
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

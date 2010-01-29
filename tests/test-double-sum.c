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
#include <push/compose.h>
#include <push/fold.h>
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
integer_process_bytes(push_parser_t *parser,
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


static push_callback_t *
integer_callback_new()
{
    integer_callback_t  *ints =
        (integer_callback_t *) malloc(sizeof(integer_callback_t));
    push_min_bytes_t  *result;

    if (ints == NULL)
        return NULL;

    push_callback_init(&ints->base,
                       NULL,
                       integer_process_bytes,
                       NULL);

    ints->integer = 0;
    ints->base.result = &ints->integer;

    result = push_min_bytes_new(&ints->base, sizeof(uint32_t));
    if (result == NULL)
    {
        push_callback_free(&ints->base);
        return NULL;
    }

    return &result->base;
}


/*-----------------------------------------------------------------------
 * Double-sum callback implementation
 *
 * This callback tests the push_parser_set_callback function.  The
 * goal is to create two sums from a list of integers — the first is
 * the sum of all of the even-indexed entries, the second the sum of
 * the odd-indexed entries.
 *
 * The callback doesn't actually parse anything; it contains links to
 * the callback that parses the “even” and “odd” integers.  When the
 * double-sum callback is activated, it takes the most recent results
 * from the linked callbacks and adds them to the sums.
 */

typedef struct _double_sum_callback
{
    push_callback_t  base;

    push_callback_t  *int1;
    uint32_t  sum1;

    push_callback_t  *int2;
    uint32_t  sum2;
} double_sum_callback_t;


static push_error_code_t
double_sum_activate(push_parser_t *parser,
                    push_callback_t *pcallback,
                    void *vinput)
{
    double_sum_callback_t  *callback =
        (double_sum_callback_t *) pcallback;
    uint32_t  *ptr1;
    uint32_t  *ptr2;

    PUSH_DEBUG_MSG("double-sum: Activating.\n");

    ptr1 = (uint32_t *) callback->int1->result;
    callback->sum1 += *ptr1;
    PUSH_DEBUG_MSG("double-sum: Adding %"PRIu32" to first sum, "
                   "sum is now %"PRIu32".\n",
                   *ptr1, callback->sum1);

    ptr2 = (uint32_t *) callback->int2->result;
    callback->sum2 += *ptr2;
    PUSH_DEBUG_MSG("double-sum: Adding %"PRIu32" to second sum, "
                   "sum is now %"PRIu32".\n",
                   *ptr2, callback->sum2);

    return PUSH_SUCCESS;
}


static ssize_t
double_sum_process_bytes(push_parser_t *parser,
                         push_callback_t *pcallback,
                         const void *buf,
                         size_t bytes_available)
{
    /*
     * We don't actually parse anything, so we always succeed.
     */

    return bytes_available;
}


static void
double_sum_free(push_callback_t *pcallback)
{
    double_sum_callback_t  *callback =
        (double_sum_callback_t *) pcallback;

    PUSH_DEBUG_MSG("double-sum: Freeing first integer callback.\n");
    push_callback_free(callback->int1);

    PUSH_DEBUG_MSG("double-sum: Freeing second integer callback.\n");
    push_callback_free(callback->int2);
}


static push_callback_t *
double_sum_callback_new(push_callback_t *int1,
                        push_callback_t *int2)
{
    double_sum_callback_t  *result =
        (double_sum_callback_t *) malloc(sizeof(double_sum_callback_t));

    if (result == NULL)
        return NULL;

    push_callback_init(&result->base,
                       double_sum_activate,
                       double_sum_process_bytes,
                       double_sum_free);

    result->int1 = int1;
    result->sum1 = 0;

    result->int2 = int2;
    result->sum2 = 0;

    result->base.result = result;

    return &result->base;
}


/*-----------------------------------------------------------------------
 * The real deal
 */

static push_callback_t *
make_double_sum_callback()
{
    push_callback_t  *int1;
    push_callback_t  *int2;
    push_callback_t  *double_sum;
    push_compose_t  *compose1;
    push_compose_t  *compose2;
    push_fold_t  *fold;

    int1 = integer_callback_new();
    if (int1 == NULL) return NULL;

    int2 = integer_callback_new();
    if (int2 == NULL) return NULL;

    double_sum = double_sum_callback_new(int1, int2);
    if (double_sum == NULL) return NULL;

    compose1 = push_compose_new(int1, int2);
    if (compose1 == NULL) return NULL;

    compose2 = push_compose_new(&compose1->base, double_sum);
    if (compose2 == NULL) return NULL;

    fold = push_fold_new(&compose2->base);
    if (fold == NULL) return NULL;

    return &fold->base;
}


/*-----------------------------------------------------------------------
 * Sample data
 */

const uint32_t  DATA_01[] = { 1, 2, 3, 4, 5, 6 };
const size_t  LENGTH_01 = 6 * sizeof(uint32_t);


/*-----------------------------------------------------------------------
 * Test cases
 */


START_TEST(test_double_sum_01)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
    double_sum_callback_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_double_sum_01\n");

    callback = make_double_sum_callback();
    fail_if(callback == NULL,
            "Could not allocate double-sum callback");

    parser = push_parser_new(callback);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_INCOMPLETE,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = (double_sum_callback_t *) callback->result;

    fail_unless(result->sum1 == 9,
                "Even sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                result->sum1, 9);

    fail_unless(result->sum2 == 12,
                "Odd sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                result->sum2, 12);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_double_sum_02)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
    double_sum_callback_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_double_sum_02\n");

    /*
     * If we submit the data twice, we should get twice the result.
     */

    callback = make_double_sum_callback();
    fail_if(callback == NULL,
            "Could not allocate double-sum callback");

    parser = push_parser_new(callback);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_INCOMPLETE,
                "Could not parse data");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_INCOMPLETE,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = (double_sum_callback_t *) callback->result;

    fail_unless(result->sum1 == 18,
                "Even sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                result->sum1, 18);

    fail_unless(result->sum2 == 24,
                "Odd sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                result->sum2, 24);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_double_sum_03)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
    double_sum_callback_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_double_sum_03\n");

    /*
     * Basically the same as test_double_sum_01, but this one turns
     * off the callback's maximum, so it should process all in one go.
     * End result should be the same.
     */

    callback = make_double_sum_callback();
    fail_if(callback == NULL,
            "Could not allocate double-sum callback");

    parser = push_parser_new(callback);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_INCOMPLETE,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = (double_sum_callback_t *) callback->result;

    fail_unless(result->sum1 == 9,
                "Even sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                result->sum1, 9);

    fail_unless(result->sum2 == 12,
                "Odd sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                result->sum2, 12);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_misaligned_data)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
    double_sum_callback_t  *result;
    size_t  FIRST_CHUNK_SIZE = 7; /* something not divisible by 4 */

    PUSH_DEBUG_MSG("---\nStarting test_misaligned_data\n");

    /*
     * Our callback processes ints on nice 32-bit boundaries.  If we
     * send the parser data that doesn't align with these boundaries,
     * we should still get the right answer.
     */

    callback = make_double_sum_callback();
    fail_if(callback == NULL,
            "Could not allocate double-sum callback");

    parser = push_parser_new(callback);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

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

    result = (double_sum_callback_t *) callback->result;

    fail_unless(result->sum1 == 9,
                "Even sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                result->sum1, 9);

    fail_unless(result->sum2 == 12,
                "Odd sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                result->sum2, 12);

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

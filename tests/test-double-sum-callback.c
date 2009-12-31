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

#include <check.h>

#include <push.h>


/*-----------------------------------------------------------------------
 * Double-sum callback implementation
 *
 * This callback tests the push_parser_set_callback function.  The
 * goal is to create two sums from a list of integers — the first is
 * the sum of all of the even-indexed entries, the second the sum of
 * the odd-indexed entries.
 *
 * To do this, each sum_callback_t accumulates sums, just like in
 * test-sum-callback.c.  However, it also has a link to the “next”
 * callback; after it processes a single integer, it tells the parser
 * to switch to the next callback.  The next callback does the same,
 * switching back to the original one.
 */

typedef struct _sum_callback sum_callback_t;

struct _sum_callback
{
    push_callback_t  base;
    uint32_t  sum;
    sum_callback_t  *next_callback;
};


static size_t
sum_callback_process_bytes(push_parser_t *parser,
                           push_callback_t *pcallback,
                           const void *buf,
                           size_t bytes_available)
{
    sum_callback_t  *callback = (sum_callback_t *) pcallback;

    PUSH_DEBUG_MSG("Processing %zu bytes at %p.\n",
                   bytes_available, buf);

    /*
     * Use an if instead of a while, because we only want to process a
     * single integer.
     */

    if (bytes_available >= sizeof(uint32_t))
    {
        const uint32_t  *next_int = (const uint32_t *) buf;

        PUSH_DEBUG_MSG("Adding %"PRIu32".\n", *next_int);

        callback->sum += *next_int;
        buf += sizeof(uint32_t);
        bytes_available -= sizeof(uint32_t);

        /*
         * Since we've added an integer to the sum, switch to the next
         * callback.
         */

        push_parser_set_callback(parser, &callback->next_callback->base);
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
                       NULL);

    result->sum = 0;
    result->next_callback = NULL; /* must be set by the user */

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


START_TEST(test_double_sum_01)
{
    push_parser_t  *parser;
    sum_callback_t  *callback_even;
    sum_callback_t  *callback_odd;

    callback_even = sum_callback_new();
    fail_if(callback_even == NULL,
            "Could not allocate even sum callback");

    callback_odd = sum_callback_new();
    fail_if(callback_odd == NULL,
            "Could not allocate odd sum callback");

    callback_even->next_callback = callback_odd;
    callback_odd->next_callback = callback_even;

    parser = push_parser_new(&callback_even->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    push_parser_submit_data(parser, &DATA_01, LENGTH_01);

    fail_unless(callback_even->sum == 9,
                "Even sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                callback_even->sum, 9);

    fail_unless(callback_odd->sum == 6,
                "Odd sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                callback_odd->sum, 6);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_double_sum_02)
{
    push_parser_t  *parser;
    sum_callback_t  *callback_even;
    sum_callback_t  *callback_odd;

    /*
     * If we submit the data twice, we should get twice the result.
     */

    callback_even = sum_callback_new();
    fail_if(callback_even == NULL,
            "Could not allocate even sum callback");

    callback_odd = sum_callback_new();
    fail_if(callback_odd == NULL,
            "Could not allocate odd sum callback");

    callback_even->next_callback = callback_odd;
    callback_odd->next_callback = callback_even;

    parser = push_parser_new(&callback_even->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    push_parser_submit_data(parser, &DATA_01, LENGTH_01);
    push_parser_submit_data(parser, &DATA_01, LENGTH_01);

    fail_unless(callback_even->sum == 15,
                "Even sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                callback_even->sum, 15);

    fail_unless(callback_odd->sum == 15,
                "Odd sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                callback_odd->sum, 15);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_double_sum_03)
{
    push_parser_t  *parser;
    sum_callback_t  *callback_even;
    sum_callback_t  *callback_odd;

    /*
     * Basically the same as test_double_sum_01, but this one turns
     * off the callback's maximum, so it should process all in one go.
     * End result should be the same.
     */

    callback_even = sum_callback_new();
    fail_if(callback_even == NULL,
            "Could not allocate even sum callback");

    callback_odd = sum_callback_new();
    fail_if(callback_odd == NULL,
            "Could not allocate odd sum callback");

    callback_even->base.max_bytes_requested = 0;
    callback_odd->base.max_bytes_requested = 0;

    callback_even->next_callback = callback_odd;
    callback_odd->next_callback = callback_even;

    parser = push_parser_new(&callback_even->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    push_parser_submit_data(parser, &DATA_01, LENGTH_01);

    fail_unless(callback_even->sum == 9,
                "Even sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                callback_even->sum, 9);

    fail_unless(callback_odd->sum == 6,
                "Odd sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                callback_odd->sum, 6);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_misaligned_data)
{
    push_parser_t  *parser;
    sum_callback_t  *callback_even;
    sum_callback_t  *callback_odd;
    size_t  FIRST_CHUNK_SIZE = 7; /* something not divisible by 4 */

    /*
     * Our callback processes ints on nice 32-bit boundaries.  If we
     * send the parser data that doesn't align with these boundaries,
     * we should still get the right answer.
     */

    callback_even = sum_callback_new();
    fail_if(callback_even == NULL,
            "Could not allocate even sum callback");

    callback_odd = sum_callback_new();
    fail_if(callback_odd == NULL,
            "Could not allocate odd sum callback");

    callback_even->next_callback = callback_odd;
    callback_odd->next_callback = callback_even;

    parser = push_parser_new(&callback_even->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    push_parser_submit_data(parser, &DATA_01, FIRST_CHUNK_SIZE);
    push_parser_submit_data(parser,
                            ((void *) DATA_01) + FIRST_CHUNK_SIZE,
                            LENGTH_01 - FIRST_CHUNK_SIZE);

    fail_unless(callback_even->sum == 9,
                "Even sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                callback_even->sum, 9);

    fail_unless(callback_odd->sum == 6,
                "Odd sum doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                callback_odd->sum, 6);

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

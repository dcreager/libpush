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

#include <push.h>


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

typedef struct _sum_callback sum_callback_t;

struct _sum_callback
{
    push_callback_t  base;
    uint32_t  sum;
};


typedef struct _index_callback
{
    push_callback_t  base;
    sum_callback_t  *sum_callbacks[NUM_SUM_CALLBACKS];
} index_callback_t;


static ssize_t
sum_callback_process_bytes(push_parser_t *parser,
                           push_callback_t *pcallback,
                           const void *buf,
                           size_t bytes_available)
{
    sum_callback_t  *callback = (sum_callback_t *) pcallback;

    PUSH_DEBUG_MSG("sum: Processing %zu bytes at %p.\n",
                   bytes_available, buf);

    /*
     * Use an if instead of a while, because we only want to process a
     * single integer.
     */

    if (bytes_available >= sizeof(uint32_t))
    {
        const uint32_t  *next_int = (const uint32_t *) buf;
        buf += sizeof(uint32_t);
        bytes_available -= sizeof(uint32_t);

        PUSH_DEBUG_MSG("Adding %"PRIu32".\n", *next_int);

        callback->sum += *next_int;

        /*
         * Since we've added an integer to the sum, switch to the next
         * callback.
         */

        push_parser_set_callback(parser, callback->base.next_callback);
    }

    return bytes_available;
}


static ssize_t
index_callback_process_bytes(push_parser_t *parser,
                             push_callback_t *pcallback,
                             const void *buf,
                             size_t bytes_available)
{
    index_callback_t  *callback = (index_callback_t *) pcallback;

    PUSH_DEBUG_MSG("index: Processing %zu bytes at %p.\n",
                   bytes_available, buf);

    if (bytes_available >= sizeof(uint32_t))
    {
        const uint32_t  *next_int = (const uint32_t *) buf;
        buf += sizeof(uint32_t);
        bytes_available -= sizeof(uint32_t);

        PUSH_DEBUG_MSG("Got index %"PRIu32".\n", *next_int);

        /*
         * Choose the next callback based on the index.  If the index
         * is out of range, raise a parse error.
         */

        if ((*next_int < 0) || (*next_int >= NUM_SUM_CALLBACKS))
        {
            PUSH_DEBUG_MSG("Index out of range!\n");
            return PUSH_PARSE_ERROR;
        } else {
            sum_callback_t  *next_callback =
                callback->sum_callbacks[*next_int];

            push_parser_set_callback(parser, &next_callback->base);
        }
    }

    return bytes_available;
}


static void
index_callback_free(push_callback_t *pcallback)
{
    int  i;
    index_callback_t  *callback = (index_callback_t *) pcallback;

    PUSH_DEBUG_MSG("index: Freeing callback %p...\n", pcallback);
    for (i = 0; i < NUM_SUM_CALLBACKS; i++)
    {
        push_callback_free(&callback->sum_callbacks[i]->base);
    }
}


static sum_callback_t *
sum_callback_new(push_callback_t *next_callback)
{
    sum_callback_t  *result =
        (sum_callback_t *) malloc(sizeof(sum_callback_t));

    if (result == NULL)
        return NULL;

    /*
     * There cannot be an EOF in between the index and the integer
     * that should be added to the sum.
     */

    push_callback_init(&result->base,
                       sizeof(uint32_t),
                       sizeof(uint32_t),
                       sum_callback_process_bytes,
                       push_eof_not_allowed,
                       NULL,
                       next_callback);

    result->sum = 0;

    return result;
}


static index_callback_t *
index_callback_new()
{
    int  i;
    index_callback_t  *result =
        (index_callback_t *) malloc(sizeof(index_callback_t));

    if (result == NULL)
        return NULL;

    /*
     * It's okay for there to be an EOF before an index — that's just
     * the end of the list.
     */

    push_callback_init(&result->base,
                       sizeof(uint32_t),
                       sizeof(uint32_t),
                       index_callback_process_bytes,
                       push_eof_allowed,
                       index_callback_free,
                       NULL);

    for (i = 0; i < NUM_SUM_CALLBACKS; i++)
    {
        sum_callback_t  *sum_callback;

        sum_callback = sum_callback_new(&result->base);
        if (sum_callback == NULL)
            return NULL;

        result->sum_callbacks[i] = sum_callback;
    }

    return result;
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
    index_callback_t  *callback;

    callback = index_callback_new();
    fail_if(callback == NULL,
            "Could not allocate index callback");

    parser = push_parser_new(&callback->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    fail_unless(callback->sum_callbacks[0]->sum == 9,
                "Sum 0 doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                callback->sum_callbacks[0]->sum, 9);

    fail_unless(callback->sum_callbacks[1]->sum == 6,
                "Sum 1 doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                callback->sum_callbacks[1]->sum, 6);

    push_parser_free(parser);
}
END_TEST

START_TEST(test_indexed_sum_02)
{
    push_parser_t  *parser;
    index_callback_t  *callback;

    /*
     * If we submit the data twice, we should get twice the result.
     */

    callback = index_callback_new();
    fail_if(callback == NULL,
            "Could not allocate index callback");

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

    fail_unless(callback->sum_callbacks[0]->sum == 18,
                "Sum 0 doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                callback->sum_callbacks[0]->sum, 18);

    fail_unless(callback->sum_callbacks[1]->sum == 12,
                "Sum 1 doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                callback->sum_callbacks[1]->sum, 12);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_indexed_sum_03)
{
    push_parser_t  *parser;
    index_callback_t  *callback;

    /*
     * Basically the same as test_indexed_sum_01, but this one turns
     * off the callback's maximum, so it should process all in one go.
     * End result should be the same.
     */

    callback = index_callback_new();
    fail_if(callback == NULL,
            "Could not allocate index callback");

    parser = push_parser_new(&callback->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    fail_unless(callback->sum_callbacks[0]->sum == 9,
                "Sum 0 doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                callback->sum_callbacks[0]->sum, 9);

    fail_unless(callback->sum_callbacks[1]->sum == 6,
                "Sum 1 doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                callback->sum_callbacks[1]->sum, 6);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_misaligned_data)
{
    push_parser_t  *parser;
    index_callback_t  *callback;
    size_t  FIRST_CHUNK_SIZE = 7; /* something not divisible by 4 */

    /*
     * Our callback processes ints on nice 32-bit boundaries.  If we
     * send the parser data that doesn't align with these boundaries,
     * we should still get the right answer.
     */

    callback = index_callback_new();
    fail_if(callback == NULL,
            "Could not allocate index callback");

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

    fail_unless(callback->sum_callbacks[0]->sum == 9,
                "Sum 0 doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                callback->sum_callbacks[0]->sum, 9);

    fail_unless(callback->sum_callbacks[1]->sum == 6,
                "Sum 1 doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                callback->sum_callbacks[1]->sum, 6);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_parse_error_01)
{
    push_parser_t  *parser;
    index_callback_t  *callback;

    callback = index_callback_new();
    fail_if(callback == NULL,
            "Could not allocate index callback");

    parser = push_parser_new(&callback->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_02, LENGTH_02) == PUSH_PARSE_ERROR,
                "Should get a parse error");

    push_parser_free(parser);
}
END_TEST


START_TEST(test_parse_error_02)
{
    push_parser_t  *parser;
    index_callback_t  *callback;

    callback = index_callback_new();
    fail_if(callback == NULL,
            "Could not allocate index callback");

    parser = push_parser_new(&callback->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_03, LENGTH_03) == PUSH_SUCCESS,
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
    tcase_add_test(tc, test_indexed_sum_03);
    tcase_add_test(tc, test_misaligned_data);
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

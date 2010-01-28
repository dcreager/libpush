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

typedef struct _index_callback
{
    push_callback_t  base;

    push_callback_t  *index;
    push_callback_t  *value;

    uint32_t  sum[NUM_SUM_CALLBACKS];
} index_callback_t;


static push_error_code_t
index_activate(push_parser_t *parser,
               push_callback_t *pcallback,
               void *input)
{
    index_callback_t  *callback = (index_callback_t *) pcallback;
    uint32_t  *index;
    uint32_t  *value;

    PUSH_DEBUG_MSG("index: Activating.\n");

    index = (uint32_t *) callback->index->result;
    value = (uint32_t *) callback->value->result;

    if ((*index < 0) || (*index >= NUM_SUM_CALLBACKS))
    {
        PUSH_DEBUG_MSG("index: Got invalid index %"PRIu32".\n",
                       *index);
        return PUSH_PARSE_ERROR;
    }

    callback->sum[*index] += *value;
    PUSH_DEBUG_MSG("index: Adding %"PRIu32" to sum #%"PRIu32
                   ", sum is now %"PRIu32".\n",
                   *value, *index, callback->sum[*index]);

    return PUSH_SUCCESS;
}


static ssize_t
index_process_bytes(push_parser_t *parser,
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
index_free(push_callback_t *pcallback)
{
    index_callback_t  *callback = (index_callback_t *) pcallback;

    PUSH_DEBUG_MSG("index: Freeing index callback.\n");
    push_callback_free(callback->index);

    PUSH_DEBUG_MSG("index: Freeing value callback.\n");
    push_callback_free(callback->value);
}


static push_callback_t *
index_callback_new(push_callback_t *index,
                   push_callback_t *value)
{
    index_callback_t  *result =
        (index_callback_t *) malloc(sizeof(index_callback_t));
    int  i;

    if (result == NULL)
        return NULL;

    push_callback_init(&result->base,
                       index_activate,
                       index_process_bytes,
                       index_free);

    result->index = index;
    result->value = value;

    result->base.result = result;

    for (i = 0; i < NUM_SUM_CALLBACKS; i++)
    {
        result->sum[i] = 0;
    }

    return &result->base;
}


/*-----------------------------------------------------------------------
 * The real deal
 */

static push_callback_t *
make_index_callback()
{
    push_callback_t  *index;
    push_callback_t  *value;
    push_callback_t  *index_sum;
    push_bind_t  *bind1;
    push_bind_t  *bind2;
    push_fold_t  *fold;

    index = integer_callback_new();
    if (index == NULL) return NULL;

    value = integer_callback_new();
    if (value == NULL) return NULL;

    index_sum = index_callback_new(index, value);
    if (index_sum == NULL) return NULL;

    bind1 = push_bind_new(index, value);
    if (bind1 == NULL) return NULL;

    bind2 = push_bind_new(&bind1->base, index_sum);
    if (bind2 == NULL) return NULL;

    fold = push_fold_new(&bind2->base);
    if (fold == NULL) return NULL;

    return &fold->base;
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
    index_callback_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_indexed_sum_01\n");

    callback = make_index_callback();
    fail_if(callback == NULL,
            "Could not allocate index callback");

    parser = push_parser_new(callback);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_INCOMPLETE,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = (index_callback_t *) callback->result;

    fail_unless(result->sum[0] == 9,
                "Sum 0 doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                result->sum[0], 9);

    fail_unless(result->sum[1] == 6,
                "Sum 1 doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                result->sum[1], 6);

    push_parser_free(parser);
}
END_TEST

START_TEST(test_indexed_sum_02)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
    index_callback_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_indexed_sum_02\n");

    /*
     * If we submit the data twice, we should get twice the result.
     */

    callback = make_index_callback();
    fail_if(callback == NULL,
            "Could not allocate index callback");

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

    result = (index_callback_t *) callback->result;

    fail_unless(result->sum[0] == 18,
                "Sum 0 doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                result->sum[0], 18);

    fail_unless(result->sum[1] == 12,
                "Sum 1 doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                result->sum[1], 12);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_misaligned_data)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
    index_callback_t  *result;
    size_t  FIRST_CHUNK_SIZE = 7; /* something not divisible by 4 */

    PUSH_DEBUG_MSG("---\nStarting test_misaligned_data\n");

    /*
     * Our callback processes ints on nice 32-bit boundaries.  If we
     * send the parser data that doesn't align with these boundaries,
     * we should still get the right answer.
     */

    callback = make_index_callback();
    fail_if(callback == NULL,
            "Could not allocate index callback");

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

    result = (index_callback_t *) callback->result;

    fail_unless(result->sum[0] == 9,
                "Sum 0 doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                result->sum[0], 9);

    fail_unless(result->sum[1] == 6,
                "Sum 1 doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                result->sum[1], 6);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_parse_error_01)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
    index_callback_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_parse_error_01\n");

    /*
     * Tests that we get a parse error when the index is out of range.
     * Since it's wrapped up in a fold, though, this will just cause
     * the overall callback to succeed after the most recent good
     * pair.
     */

    callback = make_index_callback();
    fail_if(callback == NULL,
            "Could not allocate index callback");

    parser = push_parser_new(callback);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_02, LENGTH_02) == PUSH_SUCCESS,
                "Should get a parse error");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = (index_callback_t *) callback->result;

    fail_unless(result->sum[0] == 1,
                "Sum 0 doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                result->sum[0], 1);

    fail_unless(result->sum[1] == 2,
                "Sum 1 doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                result->sum[1], 2);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_parse_error_02)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
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

    callback = make_index_callback();
    fail_if(callback == NULL,
            "Could not allocate index callback");

    parser = push_parser_new(callback);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

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

    PUSH_DEBUG_MSG("---\nStarting test_parse_error_03\n");

    callback = make_index_callback();
    fail_if(callback == NULL,
            "Could not allocate index callback");

    parser = push_parser_new(callback);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

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

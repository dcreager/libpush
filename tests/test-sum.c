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
    push_callback_t  *result;

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

    return result;
}


/*-----------------------------------------------------------------------
 * Sum callback implementation
 *
 * The sum callback is implemented using pairs: It expects to receive
 * a pair of uint32_t's.  The first one is the integer that was just
 * parsed; the second is the previous value of the sum.  The output is
 * a pair, with the first element NULL, and the second the new sum.
 *
 * The “repeated sum” callback links an integer callback and a sum
 * callback into a fold.  The folded callback has the following
 * design:
 *
 *    +----------------------------------------------+
 *    |                                              |
 *    |  NULL  +-----+  uint32_t  +-----+    NULL    |
 *    |=======>| Int |===========>|     |===========>|
 *    |        +-----+            | Sum |            |
 *    |                           |     |            |
 *    |==========================>|     |===========>|
 *    |          uint32_t         +-----+  uint32_t  |
 *    |                                              |
 *    +----------------------------------------------+
 *
 * So it takes in a pair, but the callback expects the first element
 * to be NULL on input, and outputs a NULL there as well.  The first
 * element is only used internally in between the Int and Sum
 * callbacks.
 */

typedef struct _sum_callback
{
    push_callback_t  base;
    uint32_t  *input_int;
    uint32_t  *input_sum;
    push_pair_t  output_pair;
    uint32_t  output_sum;
} sum_callback_t;


static push_error_code_t
sum_activate(push_parser_t *parser,
             push_callback_t *pcallback,
             void *vinput)
{
    sum_callback_t  *callback = (sum_callback_t *) pcallback;
    push_pair_t  *input = (push_pair_t *) vinput;

    callback->input_int = (uint32_t *) input->first;
    callback->input_sum = (uint32_t *) input->second;

    PUSH_DEBUG_MSG("sum: Activating callback.  "
                   "Received value %"PRIu32", sum %"PRIu32".\n",
                   *callback->input_int,
                   *callback->input_sum);
    return PUSH_SUCCESS;
}


static ssize_t
sum_process_bytes(push_parser_t *parser,
                  push_callback_t *pcallback,
                  const void *buf,
                  size_t bytes_available)
{
    sum_callback_t  *callback = (sum_callback_t *) pcallback;

    /*
     * Add the two numbers together.  The pointers in the output pair
     * don't change if we execute this callback more than once, so
     * they were set in the constructor.  Same for the result pointer.
     */

    callback->output_sum = *callback->input_int + *callback->input_sum;

    PUSH_DEBUG_MSG("sum: Adding, sum is now %"PRIu32"\n",
                   callback->output_sum);

    /*
     * We don't actually parse anything, so we always succeed.
     */

    return bytes_available;
}


static push_callback_t *
sum_callback_new()
{
    sum_callback_t  *sum =
        (sum_callback_t *) malloc(sizeof(sum_callback_t));

    if (sum == NULL)
        return NULL;

    push_callback_init(&sum->base,
                       sum_activate,
                       sum_process_bytes,
                       NULL);

    sum->output_pair.first = NULL;
    sum->output_pair.second = &sum->output_sum;
    sum->base.result = &sum->output_pair;

    return &sum->base;
}


static push_callback_t *
make_repeated_sum()
{
    push_callback_t  *integer;
    push_callback_t  *first;
    push_callback_t  *sum;
    push_callback_t  *compose;
    push_callback_t  *fold;

    integer = integer_callback_new();
    first = push_first_new(integer);
    sum = sum_callback_new();
    compose = push_compose_new(first, sum);
    fold = push_fold_new(compose);

    return fold;
}


/*-----------------------------------------------------------------------
 * Sample data
 */

uint32_t  INT_0 = 0;
push_pair_t  INPUT_PAIR = { NULL, &INT_0 };

const uint32_t  DATA_01[] = { 1, 2, 3, 4, 5 };
const size_t  LENGTH_01 = 5 * sizeof(uint32_t);


/*-----------------------------------------------------------------------
 * Test cases
 */


START_TEST(test_sum_01)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
    push_pair_t  *pair;
    uint32_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_sum_01\n");

    callback = make_repeated_sum();
    fail_if(callback == NULL,
            "Could not allocate a new sum callback");

    parser = push_parser_new(callback);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_activate(parser, &INPUT_PAIR)
                == PUSH_SUCCESS,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_INCOMPLETE,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    pair = (push_pair_t *) callback->result;
    result = (uint32_t *) pair->second;

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
    push_pair_t  *pair;
    uint32_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_sum_02\n");

    /*
     * If we submit the data twice, we should get twice the result.
     */

    callback = make_repeated_sum();
    fail_if(callback == NULL,
            "Could not allocate a new sum callback");

    parser = push_parser_new(callback);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_activate(parser, &INPUT_PAIR)
                == PUSH_SUCCESS,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_INCOMPLETE,
                "Could not parse data");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_INCOMPLETE,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    pair = (push_pair_t *) callback->result;
    result = (uint32_t *) pair->second;

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
    push_pair_t  *pair;
    uint32_t  *result;
    size_t  FIRST_CHUNK_SIZE = 7; /* something not divisible by 4 */

    PUSH_DEBUG_MSG("---\nStarting test_misaligned_data\n");

    /*
     * Our callback processes ints on nice 32-bit boundaries.  If we
     * send the parser data that doesn't align with these boundaries,
     * we should still get the right answer.
     */

    callback = make_repeated_sum();
    fail_if(callback == NULL,
            "Could not allocate a new sum callback");

    parser = push_parser_new(callback);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_activate(parser, &INPUT_PAIR)
                == PUSH_SUCCESS,
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

    pair = (push_pair_t *) callback->result;
    result = (uint32_t *) pair->second;

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

    callback = make_repeated_sum();
    fail_if(callback == NULL,
            "Could not allocate a new sum callback");

    parser = push_parser_new(callback);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_activate(parser, &INPUT_PAIR)
                == PUSH_SUCCESS,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, FIRST_CHUNK_SIZE) == PUSH_INCOMPLETE,
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
    Suite  *s = suite_create("sum");

    TCase  *tc = tcase_create("sum");
    tcase_add_test(tc, test_sum_01);
    tcase_add_test(tc, test_sum_02);
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

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
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <check.h>

#include <push/basics.h>
#include <push/pairs.h>


/*-----------------------------------------------------------------------
 * Increment callback
 *
 * Takes in an integer x as input, and outputs x+1.
 */

typedef struct _inc
{
    push_callback_t  callback;
    int  result;
} inc_t;


static void
inc_activate(void *user_data,
             void *result,
             const void *buf,
             size_t bytes_remaining)
{
    inc_t  *inc = (inc_t *) user_data;
    int  *input = (int *) result;

    PUSH_DEBUG_MSG("%s: Activating.  Received value %d.\n",
                   inc->callback.name,
                   *input);

    inc->result = (*input) + 1;

    PUSH_DEBUG_MSG("%s: Incrementing value.  Result is %d.\n",
                   inc->callback.name,
                   inc->result);

    push_continuation_call(inc->callback.success,
                           &inc->result,
                           buf, bytes_remaining);

    return;
}


static push_callback_t *
inc_callback_new(push_parser_t *parser)
{
    inc_t  *inc = (inc_t *) malloc(sizeof(inc_t));

    if (inc == NULL)
        return NULL;

    push_callback_init("inc",
                       &inc->callback, parser, inc,
                       inc_activate,
                       NULL, NULL, NULL);

    return &inc->callback;
}


/*-----------------------------------------------------------------------
 * Helper functions
 */

static bool
int_pair_eq(push_pair_t *pair1, push_pair_t *pair2)
{
    int  *first1 = (int *) pair1->first;
    int  *second1 = (int *) pair1->second;

    int  *first2 = (int *) pair2->first;
    int  *second2 = (int *) pair2->second;

    return (*first1 == *first2) && (*second1 == *second2);
}

#define INT_FIRST(pair)  (*((int *) (pair)->first))
#define INT_SECOND(pair) (*((int *) (pair)->second))


#define FIRST_TEST(test_name)                                       \
    START_TEST(test_first_##test_name)                              \
    {                                                               \
        push_parser_t  *parser;                                     \
        push_callback_t  *inc;                                      \
        push_callback_t  *callback;                                 \
        push_pair_t  *result;                                       \
                                                                    \
        PUSH_DEBUG_MSG("---\nStarting test case "                   \
                       "test_first_"                                \
                       #test_name                                   \
                       "\n");                                       \
                                                                    \
        parser = push_parser_new();                                 \
        fail_if(parser == NULL,                                     \
                "Could not allocate a new push parser");            \
                                                                    \
        inc = inc_callback_new(parser);                             \
        fail_if(inc == NULL,                                        \
                "Could not allocate a new increment callback");     \
                                                                    \
        callback = push_first_new(NULL, parser, inc);                \
        fail_if(callback == NULL,                                   \
                "Could not allocate a new first callback");         \
                                                                    \
        push_parser_set_callback(parser, callback);                 \
                                                                    \
        fail_unless(push_parser_activate                            \
                    (parser,                                        \
                     &FIRST_DATA_##test_name) == PUSH_SUCCESS,      \
                    "Could not activate");                          \
                                                                    \
        fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,        \
                    "Shouldn't get parse error at EOF");            \
                                                                    \
        result = push_parser_result(parser, push_pair_t);           \
                                                                    \
        fail_unless(int_pair_eq(result,                             \
                                &FIRST_EXPECTED_##test_name),       \
                    "Value doesn't match (got (%d,%d)"              \
                    ", expected (%d,%d))",                          \
                    INT_FIRST(result), INT_SECOND(result),          \
                    INT_FIRST(&FIRST_EXPECTED_##test_name),         \
                    INT_SECOND(&FIRST_EXPECTED_##test_name));       \
                                                                    \
        push_parser_free(parser);                                   \
    }                                                               \
    END_TEST


#define SECOND_TEST(test_name)                                      \
    START_TEST(test_second_##test_name)                             \
    {                                                               \
        push_parser_t  *parser;                                     \
        push_callback_t  *inc;                                      \
        push_callback_t  *callback;                                 \
        push_pair_t  *result;                                       \
                                                                    \
        PUSH_DEBUG_MSG("---\nStarting test case "                   \
                       "test_second_"                               \
                       #test_name                                   \
                       "\n");                                       \
                                                                    \
        parser = push_parser_new();                                 \
        fail_if(parser == NULL,                                     \
                "Could not allocate a new push parser");            \
                                                                    \
        inc = inc_callback_new(parser);                             \
        fail_if(inc == NULL,                                        \
                "Could not allocate a new increment callback");     \
                                                                    \
        callback = push_second_new(NULL, parser, inc);              \
        fail_if(callback == NULL,                                   \
                "Could not allocate a new second callback");        \
                                                                    \
        push_parser_set_callback(parser, callback);                 \
                                                                    \
        fail_unless(push_parser_activate                            \
                    (parser,                                        \
                     &SECOND_DATA_##test_name) == PUSH_SUCCESS,     \
                    "Could not activate");                          \
                                                                    \
        fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,        \
                    "Shouldn't get parse error at EOF");            \
                                                                    \
        result = push_parser_result(parser, push_pair_t);           \
                                                                    \
        fail_unless(int_pair_eq(result,                             \
                                &SECOND_EXPECTED_##test_name),      \
                    "Value doesn't match (got (%d,%d)"              \
                    ", expected (%d,%d))",                          \
                    INT_FIRST(result), INT_SECOND(result),          \
                    INT_FIRST(&SECOND_EXPECTED_##test_name),        \
                    INT_SECOND(&SECOND_EXPECTED_##test_name));      \
                                                                    \
        push_parser_free(parser);                                   \
    }                                                               \
    END_TEST


#define PAR_TEST(test_name)                                         \
    START_TEST(test_par_##test_name)                                \
    {                                                               \
        push_parser_t  *parser;                                     \
        push_callback_t  *inc1;                                     \
        push_callback_t  *inc2;                                     \
        push_callback_t  *callback;                                 \
        push_pair_t  *result;                                       \
                                                                    \
        PUSH_DEBUG_MSG("---\nStarting test case "                   \
                       "test_par_"                                  \
                       #test_name                                   \
                       "\n");                                       \
                                                                    \
        parser = push_parser_new();                                 \
        fail_if(parser == NULL,                                     \
                "Could not allocate a new push parser");            \
                                                                    \
        inc1 = inc_callback_new(parser);                            \
        fail_if(inc1 == NULL,                                       \
                "Could not allocate a new increment callback");     \
                                                                    \
        inc2 = inc_callback_new(parser);                            \
        fail_if(inc2 == NULL,                                       \
                "Could not allocate a new increment callback");     \
                                                                    \
        callback = push_par_new(NULL, parser, inc1, inc2);          \
        fail_if(callback == NULL,                                   \
                "Could not allocate a new par callback");           \
                                                                    \
        push_parser_set_callback(parser, callback);                 \
                                                                    \
        fail_unless(push_parser_activate                            \
                    (parser,                                        \
                     &PAR_DATA_##test_name) == PUSH_SUCCESS,        \
                    "Could not activate");                          \
                                                                    \
        fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,        \
                    "Shouldn't get parse error at EOF");            \
                                                                    \
        result = push_parser_result(parser, push_pair_t);           \
                                                                    \
        fail_unless(int_pair_eq(result,                             \
                                &PAR_EXPECTED_##test_name),         \
                    "Value doesn't match (got (%d,%d)"              \
                    ", expected (%d,%d))",                          \
                    INT_FIRST(result), INT_SECOND(result),          \
                    INT_FIRST(&PAR_EXPECTED_##test_name),           \
                    INT_SECOND(&PAR_EXPECTED_##test_name));         \
                                                                    \
        push_parser_free(parser);                                   \
    }                                                               \
    END_TEST


#define BOTH_TEST(test_name)                                        \
    START_TEST(test_both_##test_name)                               \
    {                                                               \
        push_parser_t  *parser;                                     \
        push_callback_t  *inc1;                                     \
        push_callback_t  *inc2;                                     \
        push_callback_t  *callback;                                 \
        push_pair_t  *result;                                       \
                                                                    \
        PUSH_DEBUG_MSG("---\nStarting test case "                   \
                       "test_both_"                                 \
                       #test_name                                   \
                       "\n");                                       \
                                                                    \
        parser = push_parser_new();                                 \
        fail_if(parser == NULL,                                     \
                "Could not allocate a new push parser");            \
                                                                    \
        inc1 = inc_callback_new(parser);                            \
        fail_if(inc1 == NULL,                                       \
                "Could not allocate a new increment callback");     \
                                                                    \
        inc2 = inc_callback_new(parser);                            \
        fail_if(inc2 == NULL,                                       \
                "Could not allocate a new increment callback");     \
                                                                    \
        callback = push_both_new(NULL, parser, inc1, inc2);         \
        fail_if(callback == NULL,                                   \
                "Could not allocate a new both callback");          \
                                                                    \
        push_parser_set_callback(parser, callback);                 \
                                                                    \
        fail_unless(push_parser_activate                            \
                    (parser,                                        \
                     &BOTH_DATA_##test_name) == PUSH_SUCCESS,       \
                    "Could not activate");                          \
                                                                    \
        fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,        \
                    "Shouldn't get parse error at EOF");            \
                                                                    \
        result = push_parser_result(parser, push_pair_t);           \
                                                                    \
        fail_unless(int_pair_eq(result,                             \
                                &BOTH_EXPECTED_##test_name),        \
                    "Value doesn't match (got (%d,%d)"              \
                    ", expected (%d,%d))",                          \
                    INT_FIRST(result), INT_SECOND(result),          \
                    INT_FIRST(&BOTH_EXPECTED_##test_name),          \
                    INT_SECOND(&BOTH_EXPECTED_##test_name));        \
                                                                    \
        push_parser_free(parser);                                   \
    }                                                               \
    END_TEST


/*-----------------------------------------------------------------------
 * Sample data
 */

int  INT_0 = 0;
int  INT_1 = 1;
int  INT_2 = 2;

push_pair_t  FIRST_DATA_01 = { &INT_1, &INT_2 };
push_pair_t  FIRST_EXPECTED_01 = { &INT_2, &INT_2 };

push_pair_t  FIRST_DATA_02 = { &INT_1, &INT_1 };
push_pair_t  FIRST_EXPECTED_02 = { &INT_2, &INT_1 };

push_pair_t  SECOND_DATA_01 = { &INT_2, &INT_1 };
push_pair_t  SECOND_EXPECTED_01 = { &INT_2, &INT_2 };

push_pair_t  SECOND_DATA_02 = { &INT_1, &INT_1 };
push_pair_t  SECOND_EXPECTED_02 = { &INT_1, &INT_2 };

push_pair_t  PAR_DATA_01 = { &INT_0, &INT_1 };
push_pair_t  PAR_EXPECTED_01 = { &INT_1, &INT_2 };

push_pair_t  PAR_DATA_02 = { &INT_1, &INT_1 };
push_pair_t  PAR_EXPECTED_02 = { &INT_2, &INT_2 };

int  BOTH_DATA_01 = 0;
push_pair_t  BOTH_EXPECTED_01 = { &INT_1, &INT_1 };

int  BOTH_DATA_02 = 1;
push_pair_t  BOTH_EXPECTED_02 = { &INT_2, &INT_2 };


/*-----------------------------------------------------------------------
 * Test cases
 */

FIRST_TEST(01)
FIRST_TEST(02)

SECOND_TEST(01)
SECOND_TEST(02)

PAR_TEST(01)
PAR_TEST(02)

BOTH_TEST(01)
BOTH_TEST(02)


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
test_suite()
{
    Suite  *s = suite_create("pairs");

    TCase  *tc = tcase_create("pairs");
    tcase_add_test(tc, test_first_01);
    tcase_add_test(tc, test_first_02);
    tcase_add_test(tc, test_second_01);
    tcase_add_test(tc, test_second_02);
    tcase_add_test(tc, test_par_01);
    tcase_add_test(tc, test_par_02);
    tcase_add_test(tc, test_both_01);
    tcase_add_test(tc, test_both_02);
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

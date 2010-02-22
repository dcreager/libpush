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

#include <test-callbacks.h>

/*-----------------------------------------------------------------------
 * Sample data
 */

const uint32_t  DATA_01[] = { 1 };
const size_t  LENGTH_01 = 1 * sizeof(uint32_t);


/*-----------------------------------------------------------------------
 * Test cases
 */


START_TEST(test_integer_01)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
    uint32_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_integer_01\n");

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    callback = integer_callback_new(parser);
    fail_if(callback == NULL,
            "Could not allocate a new integer callback");

    push_parser_set_callback(parser, callback);

    fail_unless(push_parser_activate(parser, NULL)
                == PUSH_INCOMPLETE,
                "Could not parse data");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = push_parser_result(parser, uint32_t);

    fail_unless(*result == 1,
                "Integer doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *result, 1);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_integer_02)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
    uint32_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_integer_02\n");

    /*
     * If we submit the data twice, we should get the same result,
     * since we'll ignore any later data.
     */

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    callback = integer_callback_new(parser);
    fail_if(callback == NULL,
            "Could not allocate a new integer callback");

    push_parser_set_callback(parser, callback);

    fail_unless(push_parser_activate(parser, NULL)
                == PUSH_INCOMPLETE,
                "Could not parse data");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, LENGTH_01) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = push_parser_result(parser, uint32_t);

    fail_unless(*result == 1,
                "Integer doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *result, 1);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_integer_03)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
    uint32_t  *result;
    size_t  FIRST_CHUNK_SIZE = 3; /* something not divisible by 4 */

    PUSH_DEBUG_MSG("---\nStarting test_integer_03\n");

    /*
     * If we submit the data in two chunks (that don't align on the
     * integer boundaries), we should get the same result.
     */

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    callback = integer_callback_new(parser);
    fail_if(callback == NULL,
            "Could not allocate a new integer callback");

    push_parser_set_callback(parser, callback);

    fail_unless(push_parser_activate(parser, NULL)
                == PUSH_INCOMPLETE,
                "Could not parse data");

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

    fail_unless(*result == 1,
                "Integer doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *result, 1);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_parse_error_01)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
    size_t  FIRST_CHUNK_SIZE = 3; /* something not divisible by 4 */

    PUSH_DEBUG_MSG("---\nStarting test_parse_error_01\n");

    /*
     * Our callback processes integers on nice 32-bit boundaries.  If
     * we send the parser data that doesn't align with these
     * boundaries, and then reach EOF, we should get a parse error,
     * since internally the integer callback is wrapped in a
     * push_min_bytes_t.
     */

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    callback = integer_callback_new(parser);
    fail_if(callback == NULL,
            "Could not allocate a new integer callback");

    push_parser_set_callback(parser, callback);

    fail_unless(push_parser_activate(parser, NULL)
                == PUSH_INCOMPLETE,
                "Could not parse data");

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
    Suite  *s = suite_create("integer-callback");

    TCase  *tc = tcase_create("integer-callback");
    tcase_add_test(tc, test_integer_01);
    tcase_add_test(tc, test_integer_02);
    tcase_add_test(tc, test_integer_03);
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

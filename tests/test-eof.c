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
#include <push/primitives.h>

#include <test-callbacks.h>


/*-----------------------------------------------------------------------
 * Sample data
 */

const uint32_t  DATA_01[] = { 1, 2, 3, 4, 5 };
const size_t  LENGTH_01 = 5 * sizeof(uint32_t);


/*-----------------------------------------------------------------------
 * Test cases
 */


START_TEST(test_eof_01)
{
    push_parser_t  *parser;
    push_callback_t  *integer;
    push_callback_t  *eof;
    push_callback_t  *callback;
    uint32_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_eof_01\n");

    /*
     * Here, we only present one integer, so it should pass.
     */

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    integer = integer_callback_new("integer", parser);
    fail_if(integer == NULL,
            "Could not allocate a new int callback");

    eof = push_eof_new("eof", parser);
    fail_if(eof == NULL,
            "Could not allocate a new EOF callback");

    callback = push_compose_new("compose", parser, integer, eof);
    fail_if(callback == NULL,
            "Could not allocate a new compose callback");

    push_parser_set_callback(parser, callback);

    fail_unless(push_parser_activate(parser, NULL)
                == PUSH_INCOMPLETE,
                "Could not parse data");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, 1 * sizeof(uint32_t))
                == PUSH_INCOMPLETE,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = push_parser_result(parser, uint32_t);

    fail_unless(*result == 1,
                "Int doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *result, 1);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_parse_error_01)
{
    push_parser_t  *parser;
    push_callback_t  *integer;
    push_callback_t  *eof;
    push_callback_t  *callback;

    PUSH_DEBUG_MSG("---\nStarting test_parse_error_01\n");

    /*
     * Here, we present two integers, so we should get a parse error.
     */

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    integer = integer_callback_new("integer", parser);
    fail_if(integer == NULL,
            "Could not allocate a new int callback");

    eof = push_eof_new("eof", parser);
    fail_if(eof == NULL,
            "Could not allocate a new EOF callback");

    callback = push_compose_new("compose", parser, integer, eof);
    fail_if(callback == NULL,
            "Could not allocate a new compose callback");

    push_parser_set_callback(parser, callback);

    fail_unless(push_parser_activate(parser, NULL)
                == PUSH_INCOMPLETE,
                "Could not parse data");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, 2 * sizeof(uint32_t))
                == PUSH_PARSE_ERROR,
                "Should get a parse error with extra data");

    push_parser_free(parser);
}
END_TEST


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
test_suite()
{
    Suite  *s = suite_create("eof");

    TCase  *tc = tcase_create("eof");
    tcase_add_test(tc, test_eof_01);
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

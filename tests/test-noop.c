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


/*-----------------------------------------------------------------------
 * Sample data
 */

uint32_t  INT_1 = 1;

const uint32_t  DATA_01[] = { 1, 2, 3, 4, 5 };
const size_t  LENGTH_01 = 5 * sizeof(uint32_t);


/*-----------------------------------------------------------------------
 * Test cases
 */


START_TEST(test_noop_01)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
    uint32_t  *result;

    PUSH_DEBUG_MSG("---\nStarting test_noop_01\n");

    /*
     * Here, we only present one integer, so it should pass.
     */

    parser = push_parser_new();
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    callback = push_noop_new("noop", parser);
    fail_if(callback == NULL,
            "Could not allocate a new noop callback");

    push_parser_set_callback(parser, callback);

    fail_unless(push_parser_activate(parser, &INT_1)
                == PUSH_SUCCESS,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, 1 * sizeof(uint32_t))
                == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at noop");

    result = push_parser_result(parser, uint32_t);

    fail_unless(*result == 1,
                "Int doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                *result, 1);

    push_parser_free(parser);
}
END_TEST


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
test_suite()
{
    Suite  *s = suite_create("noop");

    TCase  *tc = tcase_create("noop");
    tcase_add_test(tc, test_noop_01);
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

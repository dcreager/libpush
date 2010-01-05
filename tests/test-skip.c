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
#include <push/skip.h>


/*-----------------------------------------------------------------------
 * Sample data
 */

const uint8_t  DATA_01[] = "1234567890";
const size_t  LENGTH_01 = 10;


/*-----------------------------------------------------------------------
 * Test cases
 */


START_TEST(test_skip_01)
{
    push_parser_t  *parser;
    push_skip_t  *callback;

    callback = push_skip_new(NULL, true);
    fail_if(callback == NULL,
            "Could not allocate a new skip callback");

    /*
     * Skip over five bytes, and provide 5 bytes.  This should
     * succeed.
     */

    callback->base.next_callback = &callback->base;
    callback->bytes_to_skip = 5;

    parser = push_parser_new(&callback->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, 5) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    push_parser_free(parser);
}
END_TEST


START_TEST(test_skip_02)
{
    push_parser_t  *parser;
    push_skip_t  *callback;

    callback = push_skip_new(NULL, true);
    fail_if(callback == NULL,
            "Could not allocate a new skip callback");

    /*
     * Skip over five bytes, and provide 7 bytes.  This should
     * fail.
     */

    callback->base.next_callback = &callback->base;
    callback->bytes_to_skip = 5;

    parser = push_parser_new(&callback->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, 7) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_PARSE_ERROR,
                "Shouldn't get parse error at EOF");

    push_parser_free(parser);
}
END_TEST


START_TEST(test_skip_03)
{
    push_parser_t  *parser;
    push_skip_t  *callback;

    callback = push_skip_new(NULL, true);
    fail_if(callback == NULL,
            "Could not allocate a new skip callback");

    /*
     * Skip over five bytes, and provide 10 bytes.  This should
     * succeed, since the skip callback loops back on itself.
     */

    callback->base.next_callback = &callback->base;
    callback->bytes_to_skip = 5;

    parser = push_parser_new(&callback->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, 10) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    push_parser_free(parser);
}
END_TEST


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
test_suite()
{
    Suite  *s = suite_create("skip");

    TCase  *tc = tcase_create("skip");
    tcase_add_test(tc, test_skip_01);
    tcase_add_test(tc, test_skip_02);
    tcase_add_test(tc, test_skip_03);
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
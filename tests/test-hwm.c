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
#include <hwm-buffer.h>

#include <push/basics.h>
#include <push/primitives.h>


/*-----------------------------------------------------------------------
 * Sample data
 */

const uint8_t  DATA_01[] = "1234567890";
const size_t  LENGTH_01 = 10;


/*-----------------------------------------------------------------------
 * Test cases
 */


START_TEST(test_hwm_string_01)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
    hwm_buffer_t  buf;
    void  *result;
    size_t  bytes_to_read = 5;

    PUSH_DEBUG_MSG("---\nStarting test_hwm_string_01\n");

    hwm_buffer_init(&buf);

    callback = push_hwm_string_new(&buf);
    fail_if(callback == NULL,
            "Could not allocate a new HWM-string callback");

    /*
     * Read five bytes, and provide 5 bytes.  This should succeed.
     */

    parser = push_parser_new(callback);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_activate(parser, &bytes_to_read) == PUSH_SUCCESS,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, 5) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = callback->result;
    fail_unless(memcmp(result, &DATA_01, bytes_to_read) == 0,
                "Data doesn't match");

    push_parser_free(parser);
    hwm_buffer_done(&buf);
}
END_TEST


START_TEST(test_hwm_string_02)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
    hwm_buffer_t  buf;
    void  *result;
    size_t  bytes_to_read = 5;

    PUSH_DEBUG_MSG("---\nStarting test_hwm_string_02\n");

    hwm_buffer_init(&buf);

    callback = push_hwm_string_new(&buf);
    fail_if(callback == NULL,
            "Could not allocate a new HWM-string callback");

    /*
     * Read five bytes, and provide 7 bytes.  This should succeed.
     */

    parser = push_parser_new(callback);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_activate(parser, &bytes_to_read) == PUSH_SUCCESS,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, 7) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    result = callback->result;
    fail_unless(memcmp(result, &DATA_01, bytes_to_read) == 0,
                "Data doesn't match");

    push_parser_free(parser);
    hwm_buffer_done(&buf);
}
END_TEST


START_TEST(test_hwm_string_03)
{
    push_parser_t  *parser;
    push_callback_t  *callback;
    hwm_buffer_t  buf;
    size_t  bytes_to_read = 5;

    PUSH_DEBUG_MSG("---\nStarting test_hwm_string_03\n");

    hwm_buffer_init(&buf);

    callback = push_hwm_string_new(&buf);
    fail_if(callback == NULL,
            "Could not allocate a new HWM-string callback");

    /*
     * Read five bytes, and provide 3 bytes.  This should fail.
     */

    parser = push_parser_new(callback);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_activate(parser, &bytes_to_read) == PUSH_SUCCESS,
                "Could not activate parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, 3) == PUSH_INCOMPLETE,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_PARSE_ERROR,
                "Should get parse error at EOF");

    push_parser_free(parser);
    hwm_buffer_done(&buf);
}
END_TEST


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
test_suite()
{
    Suite  *s = suite_create("hwm_string");

    TCase  *tc = tcase_create("hwm_string");
    tcase_add_test(tc, test_hwm_string_01);
    tcase_add_test(tc, test_hwm_string_02);
    tcase_add_test(tc, test_hwm_string_03);
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

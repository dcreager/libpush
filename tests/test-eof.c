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
#include <push/eof.h>


/*-----------------------------------------------------------------------
 * EOF callback implementation
 */

typedef struct _int_callback
{
    push_callback_t  base;
    uint32_t  value;
} int_callback_t;


static ssize_t
int_callback_process_bytes(push_parser_t *parser,
                           push_callback_t *pcallback,
                           const void *buf,
                           size_t bytes_available)
{
    int_callback_t  *callback = (int_callback_t *) pcallback;

    PUSH_DEBUG_MSG("Processing %zu bytes at %p.\n",
                   bytes_available, buf);

    if (bytes_available >= sizeof(uint32_t))
    {
        const uint32_t  *next_int = (const uint32_t *) buf;
        buf += sizeof(uint32_t);
        bytes_available -= sizeof(uint32_t);

        PUSH_DEBUG_MSG("Parsed value %"PRIu32".\n", *next_int);

        callback->value = *next_int;

        /*
         * Pass off to the next callback.
         */

        push_parser_set_callback(parser, pcallback->next_callback);
    }

    return bytes_available;
}


static int_callback_t *
int_callback_new()
{
    push_callback_t  *eof;
    int_callback_t  *result;

    eof = push_eof_new();
    if (eof == NULL)
        return NULL;

    result = (int_callback_t *) malloc(sizeof(int_callback_t));
    if (result == NULL)
        return NULL;

    push_callback_init(&result->base,
                       sizeof(uint32_t),
                       sizeof(uint32_t),
                       NULL,
                       int_callback_process_bytes,
                       push_eof_not_allowed,
                       NULL,
                       eof);

    result->value = 0;

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


START_TEST(test_eof_01)
{
    push_parser_t  *parser;
    int_callback_t  *callback;

    /*
     * Here, we only present one integer, so it should pass.
     */

    callback = int_callback_new();
    fail_if(callback == NULL,
            "Could not allocate a new int callback");

    parser = push_parser_new(&callback->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, 1 * sizeof(uint32_t))
                == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_SUCCESS,
                "Shouldn't get parse error at EOF");

    fail_unless(callback->value == 1,
                "Int doesn't match (got %"PRIu32
                ", expected %"PRIu32")",
                callback->value, 1);

    push_parser_free(parser);
}
END_TEST


START_TEST(test_parse_error_01)
{
    push_parser_t  *parser;
    int_callback_t  *callback;
    size_t  FIRST_CHUNK_SIZE = 3; /* something not divisible by 4 */

    /*
     * Our callback processes ints on nice 32-bit boundaries.  If we
     * send the parser data that doesn't align with these boundaries,
     * and then reach EOF, we should get a parse error.
     */

    callback = int_callback_new();
    fail_if(callback == NULL,
            "Could not allocate a new int callback");

    parser = push_parser_new(&callback->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

    fail_unless(push_parser_submit_data
                (parser, &DATA_01, FIRST_CHUNK_SIZE) == PUSH_SUCCESS,
                "Could not parse data");

    fail_unless(push_parser_eof(parser) == PUSH_PARSE_ERROR,
                "Should get parse error at EOF");

    push_parser_free(parser);
}
END_TEST


START_TEST(test_parse_error_02)
{
    push_parser_t  *parser;
    int_callback_t  *callback;

    /*
     * Here, we present two integer, so we should get a parse error.
     */

    callback = int_callback_new();
    fail_if(callback == NULL,
            "Could not allocate a new int callback");

    parser = push_parser_new(&callback->base);
    fail_if(parser == NULL,
            "Could not allocate a new push parser");

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
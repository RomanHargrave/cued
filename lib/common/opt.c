//
// opt.c:
//
// Copyright (C) 2008 Robert William Fuller <hydrologiccycle@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifdef HAVE_CONFIG_H
#include "cued_config.h" // CUED_HAVE_CONST_OPTION
#endif

#include "unix.h"
#include "macros.h"
#include "opt.h"
#include "util.h"

#include <getopt.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>


static void opt_default_error_handler(const char format[], ...);
static opt_err_fn_t errFn = opt_default_error_handler;

static opt_param_t *shrtOpts, *longOpts;
static int numShrtOpts, numLongOpts;
static int shrtOptAlloc, longOptAlloc;


void opt_register_params(opt_param_t opts[], int numOpts, int longOptHint, int shrtOptHint)
{
    opt_param_t *shrtOpt, *longOpt;
    int i, newShrtOpts, newLongOpts;
    void *vShrtOpts, *vLongOpts;

    // count the number of short and long options
    for (i = newShrtOpts = newLongOpts = 0;  i < numOpts;  ++i) {

        switch (strlen(opts[i].opt)) {
            case 0:
                errFn("zero length option (internal error)");
                return;

            case 1:
                ++newShrtOpts;
                break;

            default:
                ++newLongOpts;
                break;
        }
    }

    // deal with aliasing in ISO C
    vShrtOpts = shrtOpts;
    vLongOpts = longOpts;

    // expand the arrays to hold the new options
    if (   util_realloc_items(&vShrtOpts, sizeof(opt_param_t), &shrtOptAlloc, numShrtOpts, newShrtOpts, shrtOptHint)
        || util_realloc_items(&vLongOpts, sizeof(opt_param_t), &longOptAlloc, numLongOpts, newLongOpts, longOptHint))
    {
        errFn("out of memory allocating memory for options");
        return;
    }

    shrtOpts = (opt_param_t *) vShrtOpts;
    longOpts = (opt_param_t *) vLongOpts;

    // copy the new options to the arrays
    shrtOpt = &shrtOpts[numShrtOpts];
    longOpt = &longOpts[numLongOpts];
    for (i = 0;  i < numOpts;  ++i) {
        if (1 == strlen(opts[i].opt)) {
            *shrtOpt++ = opts[i];
        } else {
            *longOpt++ = opts[i];
        }
    }

    // update the number of options stored
    numShrtOpts += newShrtOpts;
    numLongOpts += newLongOpts;
}


#define opt2_return(n) { rc = n;  goto cleanup; }
#define opt2_error(n) { errFn(n);  opt2_return(OPT_FAILURE); }


opt_result_t opt_parse_args(int argc, char *const argv[])
{
    struct option *opts = NULL;
    char *optstring = NULL;
    PIT(char, shortStr);
    opt_result_t rc;
    int i, option, longIndex;

    // calloc ensures option array is NULL terminated and irrelevant fields are zeroed
    opts = (struct option *) calloc(numLongOpts + 1, sizeof(struct option));
    if (!opts) {
        opt2_error("out of memory allocating memory for long options");
    }

    // set options array for getopt_long
    for (i = 0;  i < numLongOpts;  ++i) {
#ifdef CUED_HAVE_CONST_OPTION
        opts[i].name =          longOpts[i].opt;
#else
        opts[i].name = (char *) longOpts[i].opt;
#endif
        switch (longOpts[i].mode) {

            case OPT_NONE:
            case OPT_SET_FLAG:
            case OPT_CLR_FLAG:
                opts[i].has_arg = no_argument;
                break;

            case OPT_REQUIRED:
                opts[i].has_arg = required_argument;
                break;

            case OPT_OPTIONAL:
                opts[i].has_arg = optional_argument;
                break;

            default:
                opt2_error("bad option mode (internal error)");
                break;
        }
        //printf("loaded %s with %d\n", opts[i].name, opts[i].has_arg);
    }

    // the worst case is a:: and we need a null terminator
    optstring = (char *) malloc(3 * numShrtOpts + 1);
    if (!optstring) {
        opt2_error("out of memory allocating memory for short options");
    }

    // set optstring for getopt_long
    shortStr = optstring;
    for (i = 0;  i < numShrtOpts;  ++i) {

        *shortStr++ = shrtOpts[i].opt[0];

        switch (shrtOpts[i].mode) {

            case OPT_NONE:
            case OPT_SET_FLAG:
            case OPT_CLR_FLAG:
                break;

            case OPT_REQUIRED:
                *shortStr++ = ':';
                break;

            case OPT_OPTIONAL:
                *shortStr++ = ':';
                *shortStr++ = ':';
                break;

            default:
                opt2_error("bad option mode (internal error)");
                break;
        }
    }
    *shortStr = 0;

    while (-1 != (option = getopt_long(argc, argv, optstring, opts, &longIndex))) {

        switch (option) {

            case 0:
                if (longIndex < 0 || longIndex >= numLongOpts) {
                    opt2_error("getopt_long returned illegal index (broken libc?)");
                }
                switch (longOpts[longIndex].mode) {

                    case OPT_SET_FLAG:
                        SETF(longOpts[longIndex].flag, *(int *) longOpts[longIndex].context);
                        break;

                    case OPT_CLR_FLAG:
                        CLRF(longOpts[longIndex].flag, *(int *) longOpts[longIndex].context);
                        break;

                    default:
                        longOpts[longIndex].fn(longOpts[longIndex].context, optarg, longOpts[longIndex].opt);
                        break;
                }
                break;

            case '?':
                opt2_return(OPT_INVALID);

            default:
                for (i = 0;  i < numShrtOpts;  ++i) {
                    if (option == shrtOpts[i].opt[0]) {
                        switch (shrtOpts[i].mode) {

                            case OPT_SET_FLAG:
                                SETF(shrtOpts[i].flag, *(int *) shrtOpts[i].context);
                                break;

                            case OPT_CLR_FLAG:
                                CLRF(shrtOpts[i].flag, *(int *) shrtOpts[i].context);
                                break;

                            default:
                                shrtOpts[i].fn(shrtOpts[i].context, optarg, shrtOpts[i].opt);
                                break;
                        }
                        goto next;
                    }
                }
                opt2_return(OPT_NOT_FOUND);
        }

    next:
        ;
    }

    opt2_return(OPT_SUCCESS);

cleanup:

    free(opts);
    free(optstring);

    free(shrtOpts);
    free(longOpts);
    shrtOpts = longOpts = NULL;
    numShrtOpts = numLongOpts = 0;
    shrtOptAlloc = longOptAlloc = 0;

    return rc;
}


static void opt_default_error_handler(const char format[], ...)
{
    va_list args;

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

void opt_set_error_handler(opt_err_fn_t fn)
{
    errFn = fn;
}


void opt_set_port(void *context, char *portno, const char *optName)
{
    ssize_t sst;

    if (strtol2(portno, NULL, 10, &sst) || sst < 0 || sst > 65535) {
        errFn("invalid port of \"%s\" specified for option \"%s\"; port must be between 0 and 65535 inclusively", portno, optName);
    } else {
        *((int *) context) = (int) sst;
    }
}


void opt_set_string(void *context, char *opt, const char *optName)
{
    *((char **) context) = opt;
}


void opt_set_int(void *context, char *opt, const char *optName)
{
    ssize_t sst;

    if (strtol2(opt, NULL, 10, &sst)) {
        errFn("invalid integer of \"%s\" specified for option \"%s\"", opt, optName);
    } else {
        *((int *) context) = (int) sst;
    }

}


void opt_set_nat_no(void *context, char *opt, const char *optName)
{
    ssize_t sst;

    if (strtol2(opt, NULL, 10, &sst) || sst < 1) {
        errFn("invalid natural number of \"%s\" specified for option \"%s\"; please specify a number greater than 0", opt, optName);
    } else {
        *((int *) context) = (int) sst;
    }

}


void opt_set_whole_no(void *context, char *opt, const char *optName)
{
    ssize_t sst;

    if (strtol2(opt, NULL, 10, &sst) || sst < 0) {
        errFn("invalid whole number of \"%s\" specified for option \"%s\"; please specify a non-negative number", opt, optName);
    } else {
        *((int *) context) = (int) sst;
    }

}


void opt_set_flag(void *context, char *opt, const char *optName)
{
    *((int *) context) = 1;
}

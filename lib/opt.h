//
// opt.h
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

#ifndef OPT_H_INCLUDED
#define OPT_H_INCLUDED


typedef void (*opt_fn_t)(void *context, char *optionArg, const char *optionName);

typedef enum _opt_mode_t {

    OPT_NONE,
    OPT_REQUIRED,
    OPT_OPTIONAL

} opt_mode_t;

typedef enum _opt_result_t {

    OPT_SUCCESS,
    OPT_INVALID,
    OPT_NOT_FOUND,
    OPT_FAILURE

} opt_result_t;

typedef struct _opt_param_t {

    const char *opt;
    void *context;
    opt_fn_t fn;

    opt_mode_t mode;
    
} opt_param_t;

typedef void (*opt_err_fn_t)(const char format[], ...);


extern void opt_set_error_handler(opt_err_fn_t fn);
extern void opt_register_params(opt_param_t options[], int numOptions, int longOptHint, int shortOptHint);
extern opt_result_t opt_parse_args(int argc, char *const argv[]);

extern void opt_set_string  (void *context, char *option, const char *optionName);
extern void opt_set_int     (void *context, char *option, const char *optionName);
extern void opt_set_nat_no  (void *context, char *option, const char *optionName);
extern void opt_set_whole_no(void *context, char *option, const char *optionName);
extern void opt_set_port    (void *context, char *option, const char *optionName);
extern void opt_set_flag    (void *context, char *option, const char *optionName);


#endif // OPT_H_INCLUDED

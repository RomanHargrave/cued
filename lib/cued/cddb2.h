//
// cddb2.h
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

#ifndef CDDB2_H_INCLUDED
#define CDDB2_H_INCLUDED

#include <cddb/cddb.h>
#include <getopt.h>


#define CDDB2_OPTIONS \
    "\t--no-cddb              disable cddb access\n" \
    "\t--no-cddb-cache        disable cddb cache\n" \
    "\t--cddb-http            alternate cddb access method\n" \
    "\t--cddb-match=number    choose cddb entry to use for disc\n" \
    "\t--cddb-server=hostname alternate cddb server\n" \
    "\t--cddb-port=number     alternate cddb port\n" \
    "\t--cddb-email=user@host alternate cddb email address\n" \
    "\t--cddb-timeout=seconds alternate cddb timeout\n" \
    "\t--cddb-cache=dir       alternate cddb cache directory\n"


extern void cddb2_init();
extern void cddb2_cleanup();
extern void cddb2_set_log_handler();
extern void cddb2_opt_register_params();

extern cddb_disc_t *cddb2_get_disc(CdIo_t *cdObj, int verbose);
extern cddb_disc_t *cddb2_get_match      (CdIo_t *cdObj, cddb_conn_t **dbObj, int *matches, int matchIndex);
extern cddb_disc_t *cddb2_get_first_match(CdIo_t *cdObj, cddb_conn_t **dbObj, int *matches);
extern cddb_disc_t *cddb2_get_next_match(cddb_conn_t *dbObj);

extern char *cddb2_get_category(cddb_disc_t *cddbObj);

extern cddb_track_t *cddb2_get_track(cddb_disc_t *cddbObj, track_t track);


#endif // CDDB2_H_INCLUDED

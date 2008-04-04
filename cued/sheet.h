//
// sheet.h
//
// Copyright (C) 2007,2008 Robert William Fuller <hydrologiccycle@gmail.com>
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

#ifndef SHEET_H_INCLUDED
#define SHEET_H_INCLUDED

#include <stdio.h>


extern void cued_write_cuefile(
    FILE *cueFile,
    CdIo_t *cdObj,
    const char *devName,
    track_t firstTrack, track_t lastTrack
    );


#endif // SHEET_H_INCLUDED

//
// qsc_types.h
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

#ifndef QSC_TYPES_H_INCLUDED
#define QSC_TYPES_H_INCLUDED

#include <sys/types.h>
#include <stdint.h>


typedef unsigned short crc16_t;

#ifndef __CDIO_H__
typedef struct msf_s msf_t;
typedef int32_t lsn_t;
#endif // __CDIO_H__


#endif // QSC_TYPES_H_INCLUDED

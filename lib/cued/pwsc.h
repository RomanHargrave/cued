//
// pwsc.h
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

#ifndef PWSC_H_INCLUDED
#define PWSC_H_INCLUDED

#include "qsc.h"


typedef struct _mmc_raw_pwsc_t {

    uint8_t subcodeData[96];

} mmc_raw_pwsc_t;


extern void pwsc_get_qsc(mmc_raw_pwsc_t *rawPWsc, qsc_buffer_t *formattedQsc);


#endif // PWSC_H_INCLUDED

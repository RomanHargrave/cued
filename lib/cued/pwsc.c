//
// pwsc.c:
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

#include <cued/pwsc.h>
#include <caulk/macros.h>
#include <string.h>


#define QSC_SHIFT 6

#define QSC_MASK(c) (( 1  << QSC_SHIFT) & (c))
#define QSC_BIT(c)  (((c) >> QSC_SHIFT) & 0x01)


static inline
void pwsc_set_bit(uint8_t *v, int n)
{
    int b, c;

    b = n % 8;
    c = n / 8;

    v[c] |= 1 << (7 - b);
}


void pwsc_get_qsc(mmc_raw_pwsc_t *rawPWsc, qsc_buffer_t *formattedQsc)
{
    int i;

    memset(formattedQsc, 0x00, sizeof(qsc_buffer_t));

    for (i = 0;  i < ssizeof(rawPWsc->subcodeData);  ++i) {
        if (QSC_MASK(rawPWsc->subcodeData[i])) {
            pwsc_set_bit(formattedQsc->subcodeData, i);
        }
    }
}

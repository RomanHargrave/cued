#
# dir.mk
#
# Copyright (C) 2008 Robert William Fuller <hydrologiccycle@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.	If not, see <http://www.gnu.org/licenses/>.
#

.PHONY : dirs $(DIRS)

#
# either :: or : can be specified here;  :: prevents the rule from being
# run unless the pre-requisites exist, which means that no implict rules
# are considered to create intermediates;  the other option is to specify
# : which means that this rule will not be considered for file patterns
# for which an implict rule is defined
#
# N.B. in this "match anything" rule, : and :: have different meanings
# than in other rules:
#
# if other rules use :, only the last given set of commands is run,
# although all the pre-requisites are aggregated;  the :: case allows
# multiple rules to be run independently, when their individual
# pre-requisites are out of date
#
% :: dirs ;

dirs : $(DIRS)

$(DIRS) :
	$(MAKE) -C $@ $(MAKECMDGOALS)

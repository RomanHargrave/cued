#
# GNUmakefile
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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

DIRS= \
	cued \
	lib \
	make \
	qdump \
    classc \
    firstcls \
    sfcat \
    analyzer \
    sfcmp \
    test

include make/dir.mk

cued qdump : lib

lib : classc

#
# seems to work with either : or :: with or without the .PHONY directive,
# and still invoke the other makefiles!
#

.PHONY : distclean

distclean ::
	@-rm -f *~

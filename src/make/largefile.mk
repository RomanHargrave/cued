#
# largefile.mk
#
# Copyright (C) 2005,2008 Robert William Fuller <hydrologiccycle@gmail.com>
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

ifeq (Linux, $(UNAME_SYSTEM))
	CC_DEFINES += -D__USE_FILE_OFFSET64 -D__USE_LARGEFILE64
else ifeq (SunOS, $(UNAME_SYSTEM))
	CC_DEFINES += -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
endif

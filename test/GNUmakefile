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

SRC = test
SRC_M4 = ob.h

BIN = object

INCLUDE = ../classc ../firstcls
LIBDIRS = ../classc ../firstcls
LIBS = classc firstcls

C_FLAGS=-pedantic

include ../make/system.mk
#include ../make/largefile.mk

ob.h : ../classc/inherit.m4

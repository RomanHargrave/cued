#
# lint.mk
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

ifeq (SunOS, $(UNAME_SYSTEM))
	LINT_BIN		?= /opt/SUNWspro/bin/lint

	LINT_FLAGS       = -XCC -Xa $(CC_DEFINES) -Nlevel=4 -errtags=yes -erroff=E_FUNC_ARG_UNUSED,E_FUNC_VAR_UNUSED,E_FUNC_SET_NOT_USED,E_EXPR_NULL_EFFECT,E_ASSIGN_UINT_TO_SIGNED_INT,E_PASS_UINT_TO_SIGNED_INT -Ncheck=%all -errhdr=%user -n
	ifeq (64, $(BITS))
		LINT_FLAGS  += -Xarch=v9 -errchk=longptr64,sizematch
	else
		LINT_FLAGS  += -errchk=sizematch
	endif
endif

.SUFFIXES: .ln

vpath %.ln $(OBJ_DIR)

%.ln : %.c
	$(LINT_BIN) $(LINT_FLAGS) -c -dirout=$(OBJ_DIR)/ $^

.PHONY: lint lint_2nd_pass lintclean

lint : $(OBJ_DIR) lint_2nd_pass lintclean

lint_2nd_pass : $(addsuffix .ln, $(SRC))
	$(LINT_BIN) $(LINT_FLAGS) $(addprefix $(OBJ_DIR)/, $(addsuffix .ln, $(SRC)))

lintclean : 
	@-rm $(addprefix $(OBJ_DIR)/, $(addsuffix .ln, $(SRC)))
	@-rmdir -p $(OBJ_DIR) 2>/dev/null

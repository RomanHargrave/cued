#
# flac.mk
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

FLAC_BIN    ?= flac
META_BIN    ?= metaflac
FLAC_FLAGS  ?= -V --best -s

# for testing
#FLAC_FLAGS  ?= --fast -s

.PHONY : all *.wav

all : *.wav

*.wav :
	$(FLAC_BIN) $(FLAC_FLAGS) "$@"
	@$(META_BIN) --import-tags-from="$(subst .wav,.tag,$@)" "$(subst .wav,.flac,$@)" 
	@rm "$@"

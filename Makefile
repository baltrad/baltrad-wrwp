###########################################################################
# Copyright (C) 2013 Swedish Meteorological and Hydrological Institute, SMHI,
#
# This file is part of baltrad-wrwp.
#
# baltrad-wrwp is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# baltrad-wrwp is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public License
# along with baltrad-wrwp.  If not, see <http://www.gnu.org/licenses/>.
# ------------------------------------------------------------------------
# 
# Main build file
# @file
# @author Anders Henja (Swedish Meteorological and Hydrological Institute, SMHI)
# @date 2013-09-16
###########################################################################

.PHONY:all
all: build

def.mk:
	+[ -f $@ ] || $(error You need to run ./configure)

.PHONY:build 
build: def.mk
	$(MAKE) -C lib
	$(MAKE) -C pywrwp
	$(MAKE) -C bin

.PHONY:install
install: def.mk
	$(MAKE) -C lib install
	$(MAKE) -C pywrwp install
	$(MAKE) -C bin install
	@echo "################################################################"
	@echo "To run the binaries you will need to setup your library path to"
	@echo "LD_LIBRARY_PATH="`cat def.mk | grep LD_PRINTOUT | sed -e"s/LD_PRINTOUT=//"`
	@echo "################################################################"

.PHONY:doc
doc:
	$(MAKE) -C doxygen doc

.PHONY:test
test: def.mk
	@chmod +x ./tools/test_wrwp.sh
	@./tools/test_wrwp.sh

.PHONY:clean
clean:
	$(MAKE) -C lib clean
	$(MAKE) -C pywrwp clean
	#$(MAKE) -C doxygen clean
	$(MAKE) -C bin clean

.PHONY:distclean
distclean:
	$(MAKE) -C lib distclean
	$(MAKE) -C pywrwp distclean
	#$(MAKE) -C doxygen distclean
	$(MAKE) -C bin distclean
	$(MAKE) -C test/pytest distclean
	@\rm -f *~ config.log config.status def.mk

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
# baltrad-wrwp make file
# @file
# @author Anders Henja (Swedish Meteorological and Hydrological Institute, SMHI)
# @date 2013-09-17
###########################################################################
-include ../def.mk

# c flags, use rave suggested ones.
#
CFLAGS= -I../lib -I. $(LAPACKE_INCLUDE_DIR) $(CBLAS_INCLUDE_DIR) $(RAVE_MODULE_PYCFLAGS)

# Linker flags
LDFLAGS= -L../lib -L. $(BLAS_LIB_DIR) $(CBLAS_LIB_DIR) $(LAPACK_LIB_DIR) $(LAPACKE_LIB_DIR) $(RAVE_MODULE_LDFLAGS) 

LIBRARIES= -lwrwp $(RAVE_MODULE_PYLIBRARIES) -llapacke -llapack -lcblas -lblas $(FORTRAN_CLINK_LIBS) -lm

# --------------------------------------------------------------------
# Fixed definitions

SOURCE= pywrwp.c
OBJECTS= $(SOURCE:.c=.o)
TARGET= _wrwp.so

MAKECDEPEND=$(CC) -MM $(CFLAGS) -MT '$(@D)/$(@F)' -o $(DF).d $<

DEPDIR=.dep
DF=$(DEPDIR)/$(*F)

# Ensures that the .dep directory exists
.PHONY=$(DEPDIR)
$(DEPDIR):
	+@[ -d $@ ] || mkdir -p $@

# And the rest of the make file targets
#
.PHONY=all
all:		$(TARGET)

$(TARGET): $(DEPDIR) $(OBJECTS) ../lib/libwrwp.so
	$(LDSHARED) -o $@ $(OBJECTS) $(LDFLAGS) $(LIBRARIES)
	
.PHONY=install
install:
	@mkdir -p "${prefix}/share/wrwp/pywrwp"
	@cp -v -f $(TARGET) "${prefix}/share/wrwp/pywrwp/"
	@cp -v -f *.py "${prefix}/share/wrwp/pywrwp/"
	@-echo "$(prefix)/share/wrwp/pywrwp" > "$(SITEPACK_PYTHON)/pywrwp.pth"

.PHONY=clean
clean:
	@\rm -f *.o core *~
	@\rm -fr $(DEPDIR)

.PHONY=distclean		 
distclean:	clean
	@\rm -f $(TARGET)

# --------------------------------------------------------------------
# Rules

# Contains dependency generation as well, so if you are not using
# gcc, comment out everything until the $(CC) statement.
%.o : %.c
	@$(MAKECDEPEND); \
	cp $(DF).d $(DF).P; \
	sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
		-e '/^$$/ d' -e 's/$$/ :/' < $(DF).d >> $(DF).P; \
	\rm -f $(DF).d
	$(CC) -c $(CFLAGS) $< -o $@

# NOTE! This ensures that the dependencies are setup at the right time so this should not be moved
-include $(SOURCE:%.c=$(DEPDIR)/%.P)

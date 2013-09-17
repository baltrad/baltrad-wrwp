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
# @date 2013-09-16
###########################################################################
-include ../def.mk

# --------------------------------------------------------------------
# Fixed definitions
CFLAGS=-I../lib $(LAPACKE_INCLUDE_DIR) $(CBLAS_INCLUDE_DIR) $(RAVE_MODULE_CFLAGS)
LDFLAGS=-L../lib $(BLAS_LIB_DIR) $(CBLAS_LIB_DIR) $(LAPACK_LIB_DIR) $(LAPACKE_LIB_DIR) $(RAVE_MODULE_LDFLAGS) $(FORTRAN_CLINK_LDFLAG)
LIBS= -lwrwp $(RAVE_MODULE_LIBRARIES) -llapacke -llapack -lcblas -lblas $(FORTRAN_CLINK_LIBS) -lm

SOURCES= wrwp_main.c
OBJECTS= $(SOURCES:.c=.o)
TARGET=	wrwp

MAKEDEPEND=gcc -MM $(CFLAGS) -o $(DF).d $<
DEPDIR=.dep
DF=$(DEPDIR)/$(*F)

# --------------------------------------------------------------------
# Rules

# Contains dependency generation as well, so if you are not using
# gcc, comment out everything until the $(CC) statement.
%.o : %.c
	@$(MAKEDEPEND); \
	cp $(DF).d $(DF).P; \
	sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
		-e '/^$$/ d' -e 's/$$/ :/' < $(DF).d >> $(DF).P; \
	\rm -f $(DF).d
	$(CC) -c $(CFLAGS) $<

# Ensures that the .dep directory exists
.PHONY=$(DEPDIR)
$(DEPDIR):
	+@[ -d $@ ] || mkdir -p $@

.PHONY=all
all:		$(TARGET)

$(TARGET): $(DEPDIR) $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS)

.PHONY=install
install:
	@"$(HLHDF_INSTALL_BIN)" -f -o -C $(TARGET) "$(prefix)/bin/$(TARGET)"

.PHONY=clean
clean:
		@\rm -f *.o core *~ $(TARGET)
		@\rm -fr $(DEPDIR)

.PHONY=distclean		 
distclean:	clean
		@\rm -f *.so

# NOTE! This ensures that the dependencies are setup at the right time so this should not be moved
-include $(SOURCES:%.c=$(DEPDIR)/%.P)

#.PHONY=install
#install:
#	@mkdir -p ${prefix}/bin/
#	@cp -v -f beamb ${prefix}/bin/

#.PHONY=clean
#clean:
#	@\rm -f *.o core *~
#	@\rm -fr $(DEPDIR)

#.PHONY=distclean		 
#distclean:	clean

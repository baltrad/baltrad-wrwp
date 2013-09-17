'''
Copyright (C) 2009 Swedish Meteorological and Hydrological Institute, SMHI,

This file is part of baltrad-wrwp.

baltrad-wrwp is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

baltrad-wrwp is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with baltrad-wrwp.  If not, see <http://www.gnu.org/licenses/>.
------------------------------------------------------------------------*/

Test suite for baltrad-wrwp that generates junit-like reports.

@file
@author Anders Henja (Swedish Meteorological and Hydrological Institute, SMHI)
@date 2013-09-17
'''
import xmlrunner

from WrwpFullTestSuite import *

if __name__=="__main__":
  xmlrunner.main()

'''
Copyright (C) 2013 Swedish Meteorological and Hydrological Institute, SMHI,

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

Tests the wrwp module.

@file
@author Anders Henja (Swedish Meteorological and Hydrological Institute, SMHI)
@date 2013-09-17
'''

import unittest
import string
import _wrwp
import _helpers

class WrwpTest(unittest.TestCase):
  def setUp(self):
    _helpers.triggerMemoryStatus()

  def tearDown(self):
    pass

  def test_new(self):
    obj = _wrwp.new()
    self.assertNotEqual(-1, string.find(`type(obj)`, "WrwpCore"))

  def test_dz(self):
    obj = _wrwp.new()
    self.assertEquals(200, obj.dz)
    obj.dz = 100
    self.assertEquals(100, obj.dz)
    try:
      obj.dz = 200.0
      self.fail("Expected TypeError")
    except TypeError, e:
      pass
    self.assertEquals(100, obj.dz)

  def test_hmax(self):
    obj = _wrwp.new()
    self.assertEquals(12000, obj.hmax)
    obj.hmax = 100
    self.assertEquals(100, obj.hmax)
    try:
      obj.hmax = 200.0
      self.fail("Expected TypeError")
    except TypeError, e:
      pass
    self.assertEquals(100, obj.hmax)

  def test_dmin(self):
    obj = _wrwp.new()
    self.assertEquals(4000, obj.dmin)
    obj.dmin = 100
    self.assertEquals(100, obj.dmin)
    try:
      obj.dmin = 200.0
      self.fail("Expected TypeError")
    except TypeError, e:
      pass
    self.assertEquals(100, obj.dmin)
    
  def test_dmax(self):
    obj = _wrwp.new()
    self.assertEquals(40000, obj.dmax)
    obj.dmax = 100
    self.assertEquals(100, obj.dmax)
    try:
      obj.dmax = 200.0
      self.fail("Expected TypeError")
    except TypeError, e:
      pass
    self.assertEquals(100, obj.dmax)

  def test_emin(self):
    obj = _wrwp.new()
    self.assertAlmostEquals(2.5, obj.emin, 4)
    obj.emin = 3.5
    self.assertAlmostEquals(3.5, obj.emin, 4)
    obj.emin = 4
    self.assertAlmostEquals(4.0, obj.emin, 4)

  def test_vmin(self):
    obj = _wrwp.new()
    self.assertAlmostEquals(2.0, obj.vmin, 4)
    obj.vmin = 3.5
    self.assertAlmostEquals(3.5, obj.vmin, 4)
    obj.vmin = 4
    self.assertAlmostEquals(4.0, obj.vmin, 4)
    
if __name__ == "__main__":
  unittest.main()
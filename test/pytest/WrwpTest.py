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
import _raveio, _rave

class WrwpTest(unittest.TestCase):
  FIXTURE = "fixtures/pvol_seang_20090501T120000Z.h5"
  FIXTURE2 = "fixtures/selul_pvol_20151114T1615Z.h5"
  
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
  
  def test_generate(self):
    pvol = _raveio.open(self.FIXTURE).object
    generator = _wrwp.new()
    generator.hmax = 2000
    generator.dz = 200
    
    vp = generator.generate(pvol)
    
    ff = vp.getFF()
    ff_dev = vp.getFFDev()
    dd = vp.getDD()
    dbz = vp.getDBZ()
    dbz_dev = vp.getDBZDev()
    
    self.assertEquals(1, ff.xsize)
    self.assertEquals(10, ff.ysize)
    self.assertEquals("ff", ff.getAttribute("what/quantity"))

    self.assertEquals(1, ff_dev.xsize)
    self.assertEquals(10, ff_dev.ysize)
    self.assertEquals("ff_dev", ff_dev.getAttribute("what/quantity"))

    self.assertEquals(1, dd.xsize)
    self.assertEquals(10, dd.ysize)
    self.assertEquals("dd", dd.getAttribute("what/quantity"))
    
    self.assertEquals(1, dbz.xsize)
    self.assertEquals(10, dbz.ysize)
    self.assertEquals("dbz", dbz.getAttribute("what/quantity"))

    self.assertEquals(1, dbz_dev.xsize)
    self.assertEquals(10, dbz_dev.ysize)
    self.assertEquals("dbz_dev", dbz_dev.getAttribute("what/quantity"))

    self.assertEquals(10, vp.getLevels())
    self.assertEquals(200, vp.interval)
    self.assertEquals(100, vp.minheight)
    self.assertEquals(2000, vp.maxheight)
    self.assertEquals(pvol.source, vp.source)
    self.assertEquals(pvol.date, vp.date)
    self.assertEquals(pvol.time, vp.time)
    
  def X_test_generate_2(self):
    pvol = _raveio.open(self.FIXTURE).object
    generator = _wrwp.new()
    generator.hmax = 2000
    generator.dz = 200
    
    vp = generator.generate(pvol)

    robj = _raveio.new()
    robj.object = vp
    robj.save("slask.h5")

  def test_generate_with_several_howattributes(self):
    pvol = _raveio.open(self.FIXTURE2).object
    generator = _wrwp.new()
    generator.hmax = 2000
    generator.dz = 200
    
    vp = generator.generate(pvol)
    
    ff = vp.getFF()
    ff_dev = vp.getFFDev()
    dd = vp.getDD()
    dbz = vp.getDBZ()
    dbz_dev = vp.getDBZDev()
    
    self.assertEquals(1, ff.xsize)
    self.assertEquals(10, ff.ysize)
    self.assertEquals("ff", ff.getAttribute("what/quantity"))

    self.assertEquals(1, ff_dev.xsize)
    self.assertEquals(10, ff_dev.ysize)
    self.assertEquals("ff_dev", ff_dev.getAttribute("what/quantity"))

    self.assertEquals(1, dd.xsize)
    self.assertEquals(10, dd.ysize)
    self.assertEquals("dd", dd.getAttribute("what/quantity"))
    
    self.assertEquals(1, dbz.xsize)
    self.assertEquals(10, dbz.ysize)
    self.assertEquals("dbz", dbz.getAttribute("what/quantity"))

    self.assertEquals(1, dbz_dev.xsize)
    self.assertEquals(10, dbz_dev.ysize)
    self.assertEquals("dbz_dev", dbz_dev.getAttribute("what/quantity"))

    self.assertEquals(10, vp.getLevels())
    self.assertEquals(200, vp.interval)
    self.assertEquals(100, vp.minheight)
    self.assertEquals(2000, vp.maxheight)
    self.assertEquals(pvol.source, vp.source)
    self.assertEquals(pvol.date, vp.date)
    self.assertEquals(pvol.time, vp.time)
    
    self.assertEquals(900, vp.getAttribute("how/lowprf"))
    self.assertEquals(1200, vp.getAttribute("how/highprf"))
    self.assertAlmostEqual(0.61, vp.getAttribute("how/pulsewidth"), 4)
    self.assertAlmostEqual(5.35, vp.getAttribute("how/wavelength"), 4)
    self.assertAlmostEqual(0.8, vp.getAttribute("how/RXbandwidth"), 4)
    self.assertAlmostEqual(3.1, vp.getAttribute("how/RXlossH"), 4)
    self.assertAlmostEqual(1.9, vp.getAttribute("how/TXlossH"), 4)
    self.assertAlmostEqual(44.9, vp.getAttribute("how/antgainH"), 4)
    self.assertEquals("AVERAGE", vp.getAttribute("how/azmethod"))
    self.assertEquals("AVERAGE", vp.getAttribute("how/binmethod"))
    self.assertEquals("False", vp.getAttribute("how/malfunc"))
    self.assertAlmostEqual(277.4, vp.getAttribute("how/nomTXpower"), 4)
    self.assertEquals("b94 3dd 000 000 000:", vp.getAttribute("how/radar_msg"), 4)
    self.assertAlmostEqual(73.101, vp.getAttribute("how/radconstH"), 4)
    self.assertAlmostEqual(0.2, vp.getAttribute("how/radomelossH"), 4)
    self.assertAlmostEqual(2.0, vp.getAttribute("how/rpm"), 4)
    self.assertEquals("PARTEC2", vp.getAttribute("how/software"))
    self.assertEquals("ERIC", vp.getAttribute("how/system"))
    self.assertEquals(4000, vp.getAttribute("how/minrange"))
    self.assertEquals(40000, vp.getAttribute("how/maxrange"))
    
if __name__ == "__main__":
  unittest.main()
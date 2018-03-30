'''
Copyright (C) 2013 Swedish Meteorological and Hydrological Institute, SMHI,

This file is part of baltrad-wrwp.

baltrad-wrwp is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

baltrad-wrwp is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
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
  FIXTURE3 = "fixtures/seang_zdr_qcvol_one_scan_and_elangle_lower_than_wrwp_threshold.h5"
  
  def setUp(self):
    _helpers.triggerMemoryStatus()

  def tearDown(self):
    pass

  def test_new(self):
    obj = _wrwp.new()
    self.assertNotEqual(-1, str(type(obj)).find("WrwpCore"))

  def test_dz(self):
    obj = _wrwp.new()
    self.assertEqual(200, obj.dz)
    obj.dz = 100
    self.assertEqual(100, obj.dz)
    try:
      obj.dz = 200.0
      self.fail("Expected TypeError")
    except TypeError:
      pass
    self.assertEqual(100, obj.dz)

  def test_hmax(self):
    obj = _wrwp.new()
    self.assertEqual(12000, obj.hmax)
    obj.hmax = 100
    self.assertEqual(100, obj.hmax)
    try:
      obj.hmax = 200.0
      self.fail("Expected TypeError")
    except TypeError:
      pass
    self.assertEqual(100, obj.hmax)

  def test_dmin(self):
    obj = _wrwp.new()
    self.assertEqual(4000, obj.dmin)
    obj.dmin = 100
    self.assertEqual(100, obj.dmin)
    try:
      obj.dmin = 200.0
      self.fail("Expected TypeError")
    except TypeError:
      pass
    self.assertEqual(100, obj.dmin)
    
  def test_dmax(self):
    obj = _wrwp.new()
    self.assertEqual(40000, obj.dmax)
    obj.dmax = 100
    self.assertEqual(100, obj.dmax)
    try:
      obj.dmax = 200.0
      self.fail("Expected TypeError")
    except TypeError:
      pass
    self.assertEqual(100, obj.dmax)

  def test_emin(self):
    obj = _wrwp.new()
    self.assertAlmostEqual(2.5, obj.emin, 4)
    obj.emin = 3.5
    self.assertAlmostEqual(3.5, obj.emin, 4)
    obj.emin = 4
    self.assertAlmostEqual(4.0, obj.emin, 4)

  def test_vmin(self):
    obj = _wrwp.new()
    self.assertAlmostEqual(2.0, obj.vmin, 4)
    obj.vmin = 3.5
    self.assertAlmostEqual(3.5, obj.vmin, 4)
    obj.vmin = 4
    self.assertAlmostEqual(4.0, obj.vmin, 4)
  
  def test_generate(self):
    pvol = _raveio.open(self.FIXTURE).object
    generator = _wrwp.new()
    generator.hmax = 2000
    generator.dz = 200
    
    vp = generator.generate(pvol, "UWND,VWND,HGHT,NV")
    
    uwnd = vp.getUWND()
    vwnd = vp.getVWND()
    hght = vp.getHGHT()
    nv = vp.getNV()
    
    self.assertEqual(1, uwnd.xsize)
    self.assertEqual(10, uwnd.ysize)
    self.assertEqual("UWND", uwnd.getAttribute("what/quantity"))

    self.assertEqual(1, vwnd.xsize)
    self.assertEqual(10, vwnd.ysize)
    self.assertEqual("VWND", vwnd.getAttribute("what/quantity"))

    self.assertEqual(1, hght.xsize)
    self.assertEqual(10, hght.ysize)
    self.assertEqual("HGHT", hght.getAttribute("what/quantity"))
    
    self.assertEqual(1, nv.xsize)
    self.assertEqual(10, nv.ysize)
    self.assertEqual("n", nv.getAttribute("what/quantity"))

    self.assertEqual(10,vp.getLevels())
    self.assertEqual(200, vp.interval)
    self.assertEqual(0, vp.minheight)
    self.assertEqual(2000, vp.maxheight)
    self.assertEqual(pvol.source, vp.source)
    self.assertEqual(pvol.date, vp.date)
    self.assertEqual(pvol.time, vp.time)
    
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
    
    vp = generator.generate(pvol, "UWND,VWND,HGHT,NV")
    
    uwnd = vp.getUWND()
    vwnd = vp.getVWND()
    hght = vp.getHGHT()
    nv = vp.getNV()

    self.assertEqual(1, uwnd.xsize)
    self.assertEqual(10, uwnd.ysize)
    self.assertEqual("UWND", uwnd.getAttribute("what/quantity"))

    self.assertEqual(1, vwnd.xsize)
    self.assertEqual(10, vwnd.ysize)
    self.assertEqual("VWND", vwnd.getAttribute("what/quantity"))

    self.assertEqual(1, hght.xsize)
    self.assertEqual(10, hght.ysize)
    self.assertEqual("HGHT", hght.getAttribute("what/quantity"))

    self.assertEqual(1, nv.xsize)
    self.assertEqual(10, nv.ysize)
    self.assertEqual("n", nv.getAttribute("what/quantity"))

    self.assertEqual(10, vp.getLevels())
    self.assertEqual(200, vp.interval)
    self.assertEqual(0, vp.minheight)
    self.assertEqual(2000, vp.maxheight)
    self.assertEqual(pvol.source, vp.source)
    self.assertEqual(pvol.date, vp.date)
    self.assertEqual(pvol.time, vp.time)
    
    self.assertEqual(900, vp.getAttribute("how/lowprf"))
    self.assertEqual(1200, vp.getAttribute("how/highprf"))
    self.assertAlmostEqual(0.61, vp.getAttribute("how/pulsewidth"), 4)
    self.assertAlmostEqual(5.35, vp.getAttribute("how/wavelength"), 4)
    self.assertAlmostEqual(0.8, vp.getAttribute("how/RXbandwidth"), 4)
    self.assertAlmostEqual(3.1, vp.getAttribute("how/RXlossH"), 4)
    self.assertAlmostEqual(1.9, vp.getAttribute("how/TXlossH"), 4)
    self.assertAlmostEqual(44.9, vp.getAttribute("how/antgainH"), 4)
    self.assertEqual("AVERAGE", vp.getAttribute("how/azmethod"))
    self.assertEqual("AVERAGE", vp.getAttribute("how/binmethod"))
    self.assertEqual("False", vp.getAttribute("how/malfunc"))
    self.assertAlmostEqual(277.4, vp.getAttribute("how/nomTXpower"), 4)
    self.assertEqual("b94 3dd 000 000 000:", vp.getAttribute("how/radar_msg"), 4)
    self.assertAlmostEqual(73.101, vp.getAttribute("how/radconstH"), 4)
    self.assertAlmostEqual(0.2, vp.getAttribute("how/radomelossH"), 4)
    self.assertAlmostEqual(2.0, vp.getAttribute("how/rpm"), 4)
    self.assertEqual("PARTEC2", vp.getAttribute("how/software"))
    self.assertEqual("ERIC", vp.getAttribute("how/system"))
    self.assertEqual(4.0, vp.getAttribute("how/minrange"))
    self.assertEqual(40.0, vp.getAttribute("how/maxrange"))

    robj = _raveio.new()
    robj.object = vp
    robj.save("slask1.h5")

  def test_second_generate_with_several_howattributes(self):
    pvol = _raveio.open(self.FIXTURE3).object
    generator = _wrwp.new()
    generator.hmax = 12000
    generator.dz = 200
    generator.emin = 4.0

    exceptionTest = False

    try:
      vp = generator.generate(pvol, "UWND,VWND,HGHT,NV")

      uwnd = vp.getUWND()
      vwnd = vp.getVWND()
      hght = vp.getHGHT()
      nv = vp.getNV()

      self.assertEqual(1, uwnd.xsize)
      self.assertEqual(60, uwnd.ysize)
      self.assertEqual("UWND", uwnd.getAttribute("what/quantity"))

      self.assertEqual(1, vwnd.xsize)
      self.assertEqual(60, vwnd.ysize)
      self.assertEqual("VWND", vwnd.getAttribute("what/quantity"))

      self.assertEqual(1, hght.xsize)
      self.assertEqual(60, hght.ysize)
      self.assertEqual("HGHT", hght.getAttribute("what/quantity"))

      self.assertEqual(1, nv.xsize)
      self.assertEqual(60, nv.ysize)
      self.assertEqual("n", nv.getAttribute("what/quantity"))

      self.assertEqual(60, vp.getLevels())
      self.assertEqual(200, vp.interval)
      self.assertEqual(0, vp.minheight)
      self.assertEqual(12000, vp.maxheight)
      self.assertEqual(pvol.source, vp.source)
      self.assertEqual(pvol.date, vp.date)
      self.assertEqual(pvol.time, vp.time)
    
      self.assertEqual(450, vp.getAttribute("how/lowprf"))
      self.assertEqual(600, vp.getAttribute("how/highprf"))
      self.assertAlmostEqual(0.5, vp.getAttribute("how/pulsewidth"), 4)
      self.assertAlmostEqual(5.348660945892334, vp.getAttribute("how/wavelength"), 4)
      self.assertAlmostEqual(2.5, vp.getAttribute("how/RXbandwidth"), 4)
      self.assertAlmostEqual(1.600000023841858, vp.getAttribute("how/RXlossH"), 4)
      self.assertAlmostEqual(2.3000001907348633, vp.getAttribute("how/TXlossH"), 4)
      self.assertAlmostEqual(44.290000915527344, vp.getAttribute("how/antgainH"), 4)
      self.assertEqual("AVERAGE", vp.getAttribute("how/azmethod"))
      self.assertEqual("AVERAGE", vp.getAttribute("how/binmethod"))
      self.assertEqual("False", vp.getAttribute("how/malfunc"))
      self.assertAlmostEqual(270.0, vp.getAttribute("how/nomTXpower"), 4)
      self.assertEqual("", vp.getAttribute("how/radar_msg"), 4)
      self.assertAlmostEqual(71.4227294921875, vp.getAttribute("how/radconstH"), 4)
      self.assertAlmostEqual(3.0, vp.getAttribute("how/rpm"), 4)
      self.assertEqual("EDGE", vp.getAttribute("how/software"))
      self.assertEqual("EECDWSR-2501C-SDP", vp.getAttribute("how/system"))
      self.assertEqual(4.0, vp.getAttribute("how/minrange"))
      self.assertEqual(40.0, vp.getAttribute("how/maxrange"))

      robj = _raveio.new()
      robj.object = vp
      robj.save("slask2.h5")
    except:
      exceptionTest = True

    self.assertEqual(True, exceptionTest)

if __name__ == "__main__":
  unittest.main()

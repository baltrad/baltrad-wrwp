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
import xml.etree.cElementTree as ET
import sys
import os

sys.path.append(os.path.realpath(__file__))

def strToNumber(strval):
  # Converts a string into a number, either int or float
  # strval: the string to translate
  # return:the translated value
  # throws ValueError if value not could be translated
  if type(strval) is not str: # Avoid doing anything if it is not a string as input
    return strval
  else:
    try:
      return int(strval)
    except ValueError:
      return float(strval)

# Parse the xml-file to get the default values, the file is placed in the directory below i.e. fixtures
root = ET.parse('./fixtures/wrwp_config.xml').getroot()
for param in root.findall('param'):
  if param.get('name') == 'DMIN':
    DMIN = strToNumber(param.find('value').text)
  if param.get('name') == 'DMAX':
    DMAX = strToNumber(param.find('value').text)
  if param.get('name') == 'NMIN_WND':
    NMIN_WND = strToNumber(param.find('value').text)
  if param.get('name') == 'NMIN_REF':
    NMIN_REF = strToNumber(param.find('value').text)
  if param.get('name') == 'EMIN':
    EMIN = strToNumber(param.find('value').text)
  if param.get('name') == 'EMAX':
    EMAX = strToNumber(param.find('value').text)
  if param.get('name') == 'ECONDMAX':
    ECONDMAX = strToNumber(param.find('value').text)
  if param.get('name') == 'HTHR':
    HTHR = strToNumber(param.find('value').text)
  if param.get('name') == 'NIMIN':
    NIMIN = strToNumber(param.find('value').text)
  if param.get('name') == 'NGAPBIN':
    NGAPBIN = strToNumber(param.find('value').text)
  if param.get('name') == 'NGAPMIN':
    NGAPMIN = strToNumber(param.find('value').text)
  if param.get('name') == 'MAXNSTD':
    MAXNSTD = strToNumber(param.find('value').text)
  if param.get('name') == 'MAXVDIFF':
    MAXVDIFF = strToNumber(param.find('value').text)
  if param.get('name') == 'VMIN':
    VMIN = strToNumber(param.find('value').text)
  if param.get('name') == 'FF_MAX':
    FF_MAX = strToNumber(param.find('value').text)
  if param.get('name') == 'DZ':
    DZ = strToNumber(param.find('value').text)
  if param.get('name') == 'HMAX':
    HMAX = strToNumber(param.find('value').text)
  if param.get('name') == 'NODATA_VP':
    NODATA_VP = strToNumber(param.find('value').text)
  if param.get('name') == 'UNDETECT_VP':
    UNDETECT_VP = strToNumber(param.find('value').text)
  if param.get('name') == 'GAIN_VP':
    GAIN_VP = strToNumber(param.find('value').text)
  if param.get('name') == 'OFFSET_VP':
    OFFSET_VP = strToNumber(param.find('value').text)
  if param.get('name') == 'METHOD':
    WRWPMETHOD = param.find('value').text
  if param.get('name') == 'QUANTITIES':
    QUANTITIES = param.find('value').text


def load_wrwp_defaults_to_obj():
  wrwp = _wrwp.new()
  wrwp.hmax = HMAX
  wrwp.dz = DZ
  wrwp.emin = EMIN
  wrwp.dmin = DMIN
  wrwp.dmax = DMAX
  wrwp.nmin_wnd = NMIN_WND
  wrwp.nmin_ref = NMIN_REF
  wrwp.emax = EMAX
  wrwp.econdmax = ECONDMAX
  wrwp.hthr = HTHR
  wrwp.nimin = NIMIN
  wrwp.ngapbin = NGAPBIN
  wrwp.ngapmin = NGAPMIN
  wrwp.maxnstd = MAXNSTD
  wrwp.maxvdiff= MAXVDIFF
  wrwp.vmin = VMIN
  wrwp.ff_max = FF_MAX
  wrwp.nodata_VP = NODATA_VP
  wrwp.undetect_VP = UNDETECT_VP
  wrwp.gain_VP = GAIN_VP
  wrwp.offset_VP = OFFSET_VP

  return wrwp

class WrwpTest(unittest.TestCase):
  FIXTURE = "fixtures/pvol_seang_20090501T120000Z.h5"
  FIXTURE2 = "fixtures/selul_pvol_20151114T1615Z.h5"
  FIXTURE3 = "fixtures/seang_zdr_qcvol_one_scan_and_elangle_lower_than_wrwp_threshold.h5"
  FIXTURE4 = "fixtures/seosd_qcvol_zdrvol_different_task.h5"
  
  def setUp(self):
    _helpers.triggerMemoryStatus()

  def tearDown(self):
    pass

  def test_new(self):
    obj = _wrwp.new()
    self.assertNotEqual(-1, str(type(obj)).find("WrwpCore"))

  def test_load_wrwp_defaults_to_obj(self):
    obj = load_wrwp_defaults_to_obj()
    self.assertEqual(200, obj.dz)
    self.assertEqual(12000, obj.hmax)
    self.assertEqual(5000, obj.dmin)
    self.assertEqual(25000, obj.dmax)
    self.assertAlmostEqual(0.5, obj.emin, 4)
    self.assertAlmostEqual(45.0, obj.emax, 4)
    self.assertAlmostEqual(9.5, obj.econdmax, 4)
    self.assertAlmostEqual(2000.0, obj.hthr, 4)
    self.assertAlmostEqual(10.0, obj.nimin, 4)
    self.assertEqual(8, obj.ngapbin)
    self.assertEqual(5, obj.ngapmin)
    self.assertEqual(0, obj.maxnstd)
    self.assertAlmostEqual(10.0, obj.maxvdiff, 4)
    self.assertAlmostEqual(2.0, obj.vmin, 4)
    self.assertAlmostEqual(60.0, obj.ff_max, 4)
    self.assertEqual(40, obj.nmin_wnd, 4)
    self.assertEqual(40, obj.nmin_ref, 4)
    
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
    self.assertEqual(5000, obj.dmin)
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
    self.assertEqual(25000, obj.dmax)
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
    self.assertAlmostEqual(0.5, obj.emin, 4)
    obj.emin = 3.5
    self.assertAlmostEqual(3.5, obj.emin, 4)
    obj.emin = 4
    self.assertAlmostEqual(4.0, obj.emin, 4)

  def test_emax(self):
    obj = _wrwp.new()
    self.assertAlmostEqual(45.0, obj.emax, 4)
    obj.emax = 35.0
    self.assertAlmostEqual(35.0, obj.emax, 4)
    obj.emax = 4
    self.assertAlmostEqual(4.0, obj.emax, 4)

  def test_econdmax(self):
    obj = _wrwp.new()
    self.assertAlmostEqual(9.5, obj.econdmax, 4)
    obj.econdmax = 45.0
    self.assertAlmostEqual(45.0, obj.econdmax, 4)
    obj.econdmax = 4
    self.assertAlmostEqual(4.0, obj.econdmax, 4)

  def test_hthr(self):
    obj = _wrwp.new()
    self.assertAlmostEqual(2000.0, obj.hthr, 4)
    obj.hthr = 1000.0
    self.assertAlmostEqual(1000.0, obj.hthr, 4)
    obj.hthr = 50
    self.assertAlmostEqual(50.0, obj.hthr, 4)

  def test_nimin(self):
    obj = _wrwp.new()
    self.assertAlmostEqual(10.0, obj.nimin, 4)
    obj.nimin = 5.0
    self.assertAlmostEqual(5.0, obj.nimin, 4)
    obj.nimin = 1
    self.assertAlmostEqual(1.0, obj.nimin, 4)
    
  def test_ngapbin(self):
    obj = _wrwp.new()
    self.assertEqual(8, obj.ngapbin)
    obj.ngapbin = 100
    self.assertEqual(100, obj.ngapbin)
    try:
      obj.ngapbin = 200.0
      self.fail("Expected TypeError")
    except TypeError:
      pass
    self.assertEqual(100, obj.ngapbin)
    
  def test_ngapmin(self):
    obj = _wrwp.new()
    self.assertEqual(8, obj.ngapmin)
    obj.ngapmin = 100
    self.assertEqual(100, obj.ngapmin)
    try:
      obj.ngapmin = 200.0
      self.fail("Expected TypeError")
    except TypeError:
      pass
    self.assertEqual(100, obj.ngapmin)
    
  def test_maxnstd(self):
    obj = _wrwp.new()
    self.assertEqual(0, obj.maxnstd)
    obj.maxnstd = 100
    self.assertEqual(100, obj.maxnstd)
    try:
      obj.maxnstd = 200.0
      self.fail("Expected TypeError")
    except TypeError:
      pass
    self.assertEqual(100, obj.maxnstd)

  def test_maxvdiff(self):
    obj = _wrwp.new()
    self.assertAlmostEqual(10.0, obj.maxvdiff, 4)
    obj.maxvdiff = 5.0
    self.assertAlmostEqual(5.0, obj.maxvdiff, 4)
    obj.maxvdiff = 1
    self.assertAlmostEqual(1.0, obj.maxvdiff, 4)

  def test_vmin(self):
    obj = _wrwp.new()
    self.assertAlmostEqual(2.0, obj.vmin, 4)
    obj.vmin = 3.5
    self.assertAlmostEqual(3.5, obj.vmin, 4)
    obj.vmin = 4
    self.assertAlmostEqual(4.0, obj.vmin, 4)

  def test_ff_max(self):
    obj = _wrwp.new()
    self.assertAlmostEqual(60.0, obj.ff_max, 4)
    obj.ff_max = 3.5
    self.assertAlmostEqual(3.5, obj.ff_max, 4)
    obj.ff_max = 90.0
    self.assertAlmostEqual(90.0, obj.ff_max, 4)

  def test_nmin_wnd(self):
    obj = _wrwp.new()
    self.assertEqual(40, obj.nmin_wnd, 4)
    obj.nmin_wnd = 70
    self.assertEqual(70, obj.nmin_wnd, 4)
    obj.nmin_wnd = 20
    self.assertEqual(20, obj.nmin_wnd, 4)

  def test_nmin_ref(self):
    obj = _wrwp.new()
    self.assertEqual(40, obj.nmin_ref, 4)
    obj.nmin_ref = 70
    self.assertEqual(70, obj.nmin_ref, 4)
    obj.nmin_ref = 20
    self.assertEqual(20, obj.nmin_ref, 4)

  def test_generate(self):
    pvol = _raveio.open(self.FIXTURE).object
    wrwp = load_wrwp_defaults_to_obj()
    wrwp.hmax = 2000
    wrwp.dz = 200
    fields = None
    
    fields = QUANTITIES
    method = WRWPMETHOD
    vp = wrwp.generate(pvol, method, fields)
    
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
        
    method = 'KNMI'
    vp_KNMI = wrwp.generate(pvol, method, fields)
    
    uwnd_KNMI = vp_KNMI.getUWND()
    vwnd_KNMI = vp_KNMI.getVWND()
    hght_KNMI = vp_KNMI.getHGHT()
    nv_KNMI = vp_KNMI.getNV()
    
    self.assertEqual(1, uwnd_KNMI.xsize)
    self.assertEqual(10, uwnd_KNMI.ysize)
    self.assertEqual("UWND", uwnd_KNMI.getAttribute("what/quantity"))

    self.assertEqual(1, vwnd_KNMI.xsize)
    self.assertEqual(10, vwnd_KNMI.ysize)
    self.assertEqual("VWND", vwnd_KNMI.getAttribute("what/quantity"))

    self.assertEqual(1, hght_KNMI.xsize)
    self.assertEqual(10, hght_KNMI.ysize)
    self.assertEqual("HGHT", hght_KNMI.getAttribute("what/quantity"))
    
    self.assertEqual(1, nv_KNMI.xsize)
    self.assertEqual(10, nv_KNMI.ysize)
    self.assertEqual("n", nv_KNMI.getAttribute("what/quantity"))

    self.assertEqual(10,vp_KNMI.getLevels())
    self.assertEqual(200, vp_KNMI.interval)
    self.assertEqual(0, vp_KNMI.minheight)
    self.assertEqual(2000, vp_KNMI.maxheight)
    self.assertEqual(pvol.source, vp_KNMI.source)
    self.assertEqual(pvol.date, vp_KNMI.date)
    self.assertEqual(pvol.time, vp_KNMI.time)
    

  def test_generate_from_default(self):
    pvol = _raveio.open(self.FIXTURE).object
    wrwp = load_wrwp_defaults_to_obj()
    fields = None

    exceptionTest = False
    
    method = WRWPMETHOD
    vp = wrwp.generate(pvol, method, fields)

    ff = vp.getFF()
    dd = vp.getDD()
    dbzh = vp.getDBZ()
    dbzh_dev = vp.getDBZDev()
    NZ = vp.getNZ()
    uwnd = vp.getUWND()

    self.assertEqual(1, ff.xsize)
    self.assertEqual(60, ff.ysize)
    self.assertEqual("ff", ff.getAttribute("what/quantity"))

    self.assertEqual(1, dd.xsize)
    self.assertEqual(60, dd.ysize)
    self.assertEqual("dd", dd.getAttribute("what/quantity"))

    self.assertEqual(1, dbzh.xsize)
    self.assertEqual(60, dbzh.ysize)
    self.assertEqual("DBZH", dbzh.getAttribute("what/quantity"))

    self.assertEqual(1, dbzh_dev.xsize)
    self.assertEqual(60, dbzh_dev.ysize)
    self.assertEqual("DBZH_dev", dbzh_dev.getAttribute("what/quantity"))

    self.assertEqual(1, NZ.xsize)
    self.assertEqual(60, NZ.ysize)
    self.assertEqual("nz", NZ.getAttribute("what/quantity"))

    self.assertEqual(60,vp.getLevels())
    self.assertEqual(200, vp.interval)
    self.assertEqual(0, vp.minheight)
    self.assertEqual(12000, vp.maxheight)
    self.assertEqual(pvol.source, vp.source)
    self.assertEqual(pvol.date, vp.date)
    self.assertEqual(pvol.time, vp.time)

    try:
      self.assertEqual(1, uwnd.xsize)
      self.assertEqual(60, uwnd.ysize)
      self.assertEqual("UWND", uwnd.getAttribute("what/quantity"))
    except:
      exceptionTest = True

    self.assertEqual(True, exceptionTest)

    
  def X_test_generate_2(self):
    pvol = _raveio.open(self.FIXTURE).object
    wrwp = load_wrwp_defaults_to_obj()
    wrwp.hmax = 2000
    wrwp.dz = 200
    fields = None
        
    method = WRWPMETHOD
    vp = wrwp.generate(pvol, method, fields)

    robj = _raveio.new()
    robj.object = vp
    robj.save("slasktest.h5")

  def test_generate_with_several_howattributes(self):
    pvol = _raveio.open(self.FIXTURE2).object
    wrwp = load_wrwp_defaults_to_obj()
    wrwp.hmax = 2000
    wrwp.dz = 200
    fields = None
    
    fields = QUANTITIES
    method = WRWPMETHOD
    vp = wrwp.generate(pvol, method, fields)
    
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
    
    '''self.assertEqual(900, vp.getAttribute("how/lowprf"))
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
    self.assertEqual("ERIC", vp.getAttribute("how/system"))'''
    self.assertEqual(5.0, vp.getAttribute("how/minrange"))
    self.assertEqual(25.0, vp.getAttribute("how/maxrange"))

    robj = _raveio.new()
    robj.object = vp
    robj.save("slask1.h5")

  def test_second_generate_with_several_howattributes(self):
    pvol = _raveio.open(self.FIXTURE3).object
    wrwp = load_wrwp_defaults_to_obj()
    wrwp.emin = 4.0
    fields = None

    exceptionTest = False

    try:
      fields = QUANTITIES
      method = WRWPMETHOD
      vp = wrwp.generate(pvol, method, fields)

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
    
      '''self.assertEqual(450, vp.getAttribute("how/lowprf"))
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
      self.assertEqual("EECDWSR-2501C-SDP", vp.getAttribute("how/system"))'''
      self.assertEqual(5.0, vp.getAttribute("how/minrange"))
      self.assertEqual(25.0, vp.getAttribute("how/maxrange"))

      robj = _raveio.new()
      robj.object = vp
      robj.save("slask2.h5")
    except:
      exceptionTest = True

    self.assertEqual(True, exceptionTest)

  def test_third_generate_with_several_howattributes(self):
    pvol = _raveio.open(self.FIXTURE4).object
    wrwp = load_wrwp_defaults_to_obj()
    wrwp.emin = 4.0
    fields = None

    exceptionTest = False

    try:
      fields = QUANTITIES
      method = WRWPMETHOD
      vp = wrwp.generate(pvol, method, fields)

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

      self.assertEqual(5.0, vp.getAttribute("how/minrange"))
      self.assertEqual(25.0, vp.getAttribute("how/maxrange"))
      self.assertEqual("4.0,8.0,14.0,24.0,40.0", vp.getAttribute("how/angles"))
      self.assertEqual("osd_zdr,osd_zdr_fastshort,osd_ldr_fastshort,osd_zdr_longshort", vp.getAttribute("how/task"))

      robj = _raveio.new()
      robj.object = vp
      robj.save("slask3.h5")
    except:
      exceptionTest = True

    self.assertEqual(False, exceptionTest)


if __name__ == "__main__":
  unittest.main()

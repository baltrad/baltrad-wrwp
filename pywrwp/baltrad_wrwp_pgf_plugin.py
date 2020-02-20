'''
Copyright (C) 2010- Swedish Meteorological and Hydrological Institute (SMHI)

This file is part of baltrad-wrwp.

baltrad-wrwp is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

baltrad-wrwp is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with baltrad-wrwp.  If not, see <http://www.gnu.org/licenses/>.

'''
## Plugin for generating the wrwp product generation that is initiated from the beast
## framework.
## Register in pgf with
## --name=eu.baltrad.beast.generatewrwp
## --floats=minelevationangle,velocitythreshold --ints=interval,maxheight,mindistance,maxdistance --strings=fields -m baltrad_wrwp_pgf_plugin -f generate
##
## The WRWP generation is executed by providing a polar volume that a wind profile is calculated on
##
## wrwp = _wrwp.new()
## wrwp.interval = ...
## wrwp.maxheight = ...
## ...
## result = wrwp.generate(pvol)
## 
## @file
## @author Anders Henja, SMHI
## @date 2013-09-25
##
## @co-autor Ulf E. Nordh, SMHI
## @date 2018-02-08
##
## Slight adjustment done to the call to wrwp.generate in function generate.
## The call is done with try-except to deal with the situation that
## the returned result is NULL. In addition, logging is inserted.
##
## @co-autor Ulf E. Nordh, SMHI
## @date 2019-10-08
##
## From additional requirements, some of the parameters given in wrwp.h is given in a configuration
## file instead for greater ease of adjustments without having to re-compile the code. The selected format
## is xml.

import _wrwp
import _rave
import _raveio
import _polarvolume
import string
import logging
import rave_pgf_logger
import rave_tempfile
import odim_source
import math
import os
import sys
import fnmatch
import xml.etree.cElementTree as ET    
from rave_defines import CENTER_ID, GAIN, OFFSET

logger = rave_pgf_logger.create_logger()


ravebdb = None
try:
  import rave_bdb
  ravebdb = rave_bdb.rave_bdb()
except:
  pass

# Locates a file (pattern) in a directory tree (path)
def find(pattern, path):
  result = []
  for root, dirs, files in os.walk(path):
    for name in files:
      if fnmatch.fnmatch(name, pattern):
        result.append(os.path.join(root, name))
  return result

## Creates a dictionary from a rave argument list
#@param arglist the argument list
#@return a dictionary
def arglist2dict(arglist):
  result={}
  for i in range(0, len(arglist), 2):
    result[arglist[i]] = arglist[i+1]
  return result

##
# Converts a string into a number, either int or float
# @param sval the string to translate
# @return the translated value
# @throws ValueError if value not could be translated
#
    
def strToNumber(sval):
  if type(sval) is not str: # Avoid doing anything if it is not a string as input
    return sval
  else:
    try:
      return int(sval)
    except ValueError:
      return float(sval)

## Creates a vertical profile
#@param files the list of files to be used for generating the vertical profile
#@param arguments the arguments defining the vertical profile
#@return a temporary h5 file with the vertical profile
def generate(files, arguments):

  args = arglist2dict(arguments)
  wrwp = _wrwp.new()
  fields = None

  # Finds the wrwp_config.xml under /local_disk/baltrad/blt_sys/ and sets parameters necessary for wrwp generation.
  # Usually the installation paths in the node-installer is not changed relative to default, the use of find locates the wrwp_config.xml
  # should another path be set in the node-installer, the only requirement is that the wrwp_config.xml is somewhere under /local_disk/baltrad/blt_sys.

  path2config = find('wrwp_config.xml', '/local_disk/baltrad/blt_sys')
  root = ET.parse(str(path2config[0])).getroot()

  for param in root.findall('param'):

    if param.get('name') == 'EMAX':
      wrwp.emax = strToNumber(param.find('value').text)
    if param.get('name') == 'EMIN':
      wrwp.emin = strToNumber(param.find('value').text)
    if param.get('name') == 'DMAX':
      wrwp.dmax = strToNumber(param.find('value').text)
    if param.get('name') == 'DMIN':
      wrwp.dmin = strToNumber(param.find('value').text)
    if param.get('name') == 'VMIN':
      wrwp.vmin = strToNumber(param.find('value').text)
    if param.get('name') == 'NMIN_WND':
      wrwp.nmin_wnd = strToNumber(param.find('value').text)
    if param.get('name') == 'NMIN_REF':
      wrwp.nmin_ref = strToNumber(param.find('value').text)
    if param.get('name') == 'FF-MAX':
      wrwp.ff_max = strToNumber(param.find('value').text)
    if param.get('name') == 'DZ':
      wrwp.dz = strToNumber(param.find('value').text)
    if param.get('name') == 'HMAX':
      wrwp.hmax = strToNumber(param.find('value').text)
    if param.get('name') == 'NODATA_VP':
      wrwp.nodata_VP = strToNumber(param.find('value').text)
    if param.get('name') == 'UNDETECT_VP':
      wrwp.undetect_VP = strToNumber(param.find('value').text)
    if param.get('name') == 'GAIN_VP':
      wrwp.gain_VP = strToNumber(param.find('value').text)
    if param.get('name') == 'OFFSET_VP':
      wrwp.offset_VP = strToNumber(param.find('value').text)

    if param.get('name') == 'QUANTITIES':
      QUANTITIES = param.find('value').text

    # Parameter values from the web-GUI, they overwright the ones above.
    if "interval" in args.keys():
      wrwp.dz = strToNumber(args["interval"])
    if "maxheight" in args.keys():
      wrwp.hmax = strToNumber(args["maxheight"])
    if "mindistance" in args.keys():
      wrwp.dmin = strToNumber(args["mindistance"])
    if "maxdistance" in args.keys():
      wrwp.dmax = strToNumber(args["maxdistance"]) 
    if "minelevationangle" in args.keys():
      wrwp.emin = strToNumber(args["minelevationangle"])
    if "velocitythreshold" in args.keys():
      wrwp.vmin = strToNumber(args["velocitythreshold"])
    if "fields" in args.keys():
      fields = args["fields"]

    if fields == None:
      fields = QUANTITIES # If no fields are given in the web-GUI, we build wrwp with all the supported quantities
    
  if len(files) != 1:
    raise AttributeError("Must call plugin with _one_ polar volume")
  
  logger.debug("Start generating vertical profile from polar volume %s"%files[0])

  obj = None
  if ravebdb != None:
    obj = ravebdb.get_rave_object(files[0])
  else:
    rio = _raveio.open(files[0])
    obj = rio.object

  if not _polarvolume.isPolarVolume(obj):
    raise AttributeError("Must call plugin with a polar volume")

  try:
    profile = wrwp.generate(obj, fields)  
    fileno, outfile = rave_tempfile.mktemp(suffix='.h5', close="True")
    ios = _raveio.new()
    ios.object = profile
    ios.filename = outfile
    ios.save()
    logger.debug("Finished generating vertical profile from polar volume %s"%files[0])
    return outfile
  except:
    logger.info("No vertical profile could be generated from polar volume %s"%files[0])
    return None

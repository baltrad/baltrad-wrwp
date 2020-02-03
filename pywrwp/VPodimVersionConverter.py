'''
Copyright (C) 2015- Swedish Meteorological and Hydrological Institute (SMHI)

Converts WRWP files in ODIM-H5 ver2.2 to WRWP files in ODIM-H5 ver 2.1 files.

@file
@author Ulf E. Nordh, SMHI
@date 2019-10-23'''

import _pyhl
import numpy
import re, sys, traceback, time

DATATYPES = {"char":"char",
             "int8":"char",
             "uint8":"uchar",
             "int16":"short",
             "uint16":"ushort",
             "int32":"int",
             "int64":"long",
             "float32":"float",
             "float64":"double"}

# Main class for the version converter.
class VPodimVersionConverter(object):
  def __init__(self, filename, quantities):
    self._filename = filename
    if not _pyhl.is_file_hdf5(filename):
      raise Exception("Not a HDF5 file")
    self._nodelist = _pyhl.read_nodelist(self._filename)
    self._nodelist.selectAll()
    self._nodelist.fetch()    
    self._nodenames = self._nodelist.getNodeNames().keys()
    self._converted_files = []  # Contains tuples of (nodelist, suggested name)

  def convert(self,quantities, filename):
    if self.isVerticalProfile():
      self._convertVP(quantities, filename)
    else:
      raise Exception("No Support for file type: %s"%self.getWhatObject())
    return self._nodelist, self._converted_files
  
  def isVerticalProfile(self):
    return self.getWhatObject() in ["VP"]
    
  def isSupported(self):
    return self.isVerticalProfile()
    
  def getWhatObject(self):
    return self._nodelist.getNode("/what/object").data()
  
  def _getDatasetAndDataIndexForQuantity(self, quantity):
    dsetidx = 1
    dsetloop = True
    while dsetloop:
      dloop = True
      dsetname = "/dataset%i"%dsetidx
      if dsetname not in self._nodenames:
        dsetloop = False
        break
      didx = 1
      while dloop:
        try:
          name = "/dataset%i/data%i"%(dsetidx,didx)
          if name in self._nodenames:
            dataname = "/dataset%i/data%i/what/quantity"%(dsetidx,didx)
            if dataname in self._nodenames:
              if self._nodelist.getNode(dataname).data() == quantity:
                return (dsetidx, didx)
          else:
            dloop = False
        except Exception:
          traceback.print_exc(file=sys.stdout)
          dloop = False
        didx = didx + 1
      dsetidx = dsetidx + 1
    return None
    
  def _getDatasetByQuantity(self, quantity):
    indices = self._getDatasetAndDataIndexForQuantity(quantity)
    if indices != None:
      name = "/dataset%i/data%i/data"%(indices[0], indices[1])
      return name
    return None
    
  def _convertValueTypeToStr(self, value):
    if type(value) is str:
      result = "string"
    elif type(value) is float:
      result = "float"
    elif type(value) is int:
      result = "int"
    else:
      raise Exception("Unsupported data type")
    return result

  def _copyData(self, nodelist, name, oname=None):
    d = self._nodelist.getNode(name).data()
    datatype = str(d.dtype)
    if datatype in DATATYPES:
      translDatatype= DATATYPES[datatype]
    else:
      raise Exception("Unsupported datatype %s"%datatype)

    c = _pyhl.compression(_pyhl.COMPRESSION_ZLIB)
    c.level = 6
    if oname:
      n = _pyhl.node(_pyhl.DATASET_ID, oname, c)
    else:
      n = _pyhl.node(_pyhl.DATASET_ID, name, c)
    n.setArrayValue(-1, list(d.shape), d, translDatatype, -1)
    nodelist.addNode(n)

  def _addGroup(self, nodelist, name):
    node = _pyhl.node(_pyhl.GROUP_ID, name)
    nodelist.addNode(node)

  def _addAttribute(self, nodelist, name, value):
    node = _pyhl.node(_pyhl.ATTRIBUTE_ID, name)
    node.setScalarValue(-1, value, self._convertValueTypeToStr(value), -1)
    nodelist.addNode(node)

  def _copyAttribute(self, nodelist, name, ntype=None, oname=None):
    d = self._nodelist.getNode(name).data()
    if oname:
      n = _pyhl.node(_pyhl.ATTRIBUTE_ID, oname)
    else:
      n = _pyhl.node(_pyhl.ATTRIBUTE_ID, name)
      
    if ntype:
      n.setScalarValue(-1, d, ntype, -1)
    else:
      n.setScalarValue(-1, d, self._convertValueTypeToStr(d), -1)
    
    nodelist.addNode(n)

  def _copyAttributeIfExistsToGroup(self, nodelist, name, toname=None):
    if name in self._nodenames:
      if toname == None:
        toname = name
      self._copyAttribute(nodelist, name, None, toname)
      return True
    return False

  def _copyWhatObject(self, nodelist):
    d = self._nodelist.getNode("/what/object").data()
    self._addAttribute(nodelist, "/what/object", d)
    
  def _populateNodelistWithDataAndAttributes(self, nodelist, quantity):

    if quantity == "NV":
      quantity = "n"
    if quantity == "NZ":
      quantity = "nz"

    datasetLocation = self._getDatasetByQuantity(quantity)

    # Add the datsets and their attributes to the nodelist
    data = self._nodelist.getNode(datasetLocation).data()
    self._copyData(nodelist, datasetLocation, datasetLocation)
    self._copyAttributeIfExistsToGroup(nodelist, datasetLocation[:-4] + "what/gain", datasetLocation[:-4] + "what/gain")
    self._copyAttributeIfExistsToGroup(nodelist, datasetLocation[:-4] + "what/offset", datasetLocation[:-4] + "what/offset")
    self._copyAttributeIfExistsToGroup(nodelist, datasetLocation[:-4] + "what/nodata", datasetLocation[:-4] + "what/nodata")

    if quantity == "DBZH":
      self._addAttribute(nodelist, datasetLocation[:-4] + "what/quantity", "dbz")
    elif quantity == "DBZH_dev":
      self._addAttribute(nodelist, datasetLocation[:-4] + "what/quantity", "dbz_dev")
    else:
      self._copyAttributeIfExistsToGroup(nodelist, datasetLocation[:-4] + "what/quantity", datasetLocation[:-4] + "what/quantity")

    self._copyAttributeIfExistsToGroup(nodelist, datasetLocation[:-4] + "what/undetect", datasetLocation[:-4] + "what/undetect")   
    
  def _addVPInformation(self, nodelist, quantities):
    # Prepare the nodelist
    self._addGroup(nodelist, "/how")
    self._addGroup(nodelist, "/what")
    self._addGroup(nodelist, "/where")
    self._addGroup(nodelist, "/dataset1")
    self._addGroup(nodelist, "/dataset1/what")

    for i in range(1, len(quantities) + 1):

      self._addGroup(nodelist, "/dataset1/data%d"%i)
      self._addGroup(nodelist, "/dataset1/data%d/what"%i)
    
    # Root attribute
    self._addAttribute(nodelist, "/Conventions", "ODIM_H5/V2_1")

    # Attributes under /what
    self._copyAttributeIfExistsToGroup(nodelist, "/what/date", "/what/date")
    self._copyAttributeIfExistsToGroup(nodelist, "/what/object", "/what/object")
    self._copyAttributeIfExistsToGroup(nodelist, "/what/time", "/what/time")
    self._addAttribute(nodelist, "/what/version", "H5rad 2.1")
    self._copyAttributeIfExistsToGroup(nodelist, "/what/source", "/what/source")

    # Attributes under /where
    self._copyAttributeIfExistsToGroup(nodelist, "/where/height", "/where/height")
    self._copyAttributeIfExistsToGroup(nodelist, "/where/interval", "/where/interval")
    self._copyAttributeIfExistsToGroup(nodelist, "/where/lat", "/where/lat")
    self._copyAttributeIfExistsToGroup(nodelist, "/where/levels", "/where/levels")
    self._copyAttributeIfExistsToGroup(nodelist, "/where/lon", "/where/lon")
    self._copyAttributeIfExistsToGroup(nodelist, "/where/maxheight", "/where/maxheight")
    self._copyAttributeIfExistsToGroup(nodelist, "/where/minheight", "/where/minheight")

    # Attribute under /dataset1/what
    self._copyAttributeIfExistsToGroup(nodelist, "/dataset1/what/product", "/dataset1/what/product")
    self._copyAttributeIfExistsToGroup(nodelist, "/dataset1/what/enddate", "/dataset1/what/enddate")
    self._copyAttributeIfExistsToGroup(nodelist, "/dataset1/what/endtime", "/dataset1/what/endtime")
    self._copyAttributeIfExistsToGroup(nodelist, "/dataset1/what/startdate", "/dataset1/what/startdate")
    self._copyAttributeIfExistsToGroup(nodelist, "/dataset1/what/starttime", "/dataset1/what/starttime")
        
    # Attributes under /how
    self._copyAttributeIfExistsToGroup(nodelist, "/how/angles", "/how/angles")
    self._copyAttributeIfExistsToGroup(nodelist, "/how/maxrange", "/how/maxrange")
    self._copyAttributeIfExistsToGroup(nodelist, "/how/minrange", "/how/minrange")
    self._copyAttributeIfExistsToGroup(nodelist, "/how/task", "/how/task")
    
    for QUANTITY in quantities:
      self._populateNodelistWithDataAndAttributes(nodelist, QUANTITY)

  def _convertVP(self, quantities, filename):
    nodelist = _pyhl.nodelist()
    quantities = [str(x) for x in quantities.split(",")]

    # Populate the nodelist with attributes and datasets needed for DIANA
    self._addVPInformation(nodelist, quantities)

    # Create the filename and send the modified file back to b2d
    self._converted_files.append((nodelist, filename))


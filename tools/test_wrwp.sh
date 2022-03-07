#!/bin/sh
############################################################
# Description: Script that performs the actual unit test of
#   baltrad-wrwp.
#
# Author(s):   Anders Henja
#
# Copyright:   Swedish Meteorological and Hydrological Institute, 2009
#
# History:  2013-09-17 Created by Anders Henja
############################################################
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

RES=255

if [ $# -gt 0 -a "$1" = "alltest" ]; then
  "$SCRIPTPATH/run_python_script.sh" "${SCRIPTPATH}/../test/pytest/WrwpFullTestSuite.py" "${SCRIPTPATH}/../test/pytest"
  RES=$?
else
  "$SCRIPTPATH/run_python_script.sh" "${SCRIPTPATH}/../test/pytest/WrwpTestSuite.py" "${SCRIPTPATH}/../test/pytest"
  RES=$?
fi

exit $RES

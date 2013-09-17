#!/bin/sh
############################################################
# Description: Script that should be executed from a continuous
# integration runner. It is necessary to point out the proper
# paths to baltrad-wrwp since this test sequence should be run whenever
# baltrad-wrwp is changed.
#
# Author(s):   Anders Henja
#
# Copyright:   Swedish Meteorological and Hydrological Institute, 2011
#
# History:  2013-09-17 Created by Anders Henja
############################################################
SCRFILE=`python -c "import os;print os.path.abspath(\"$0\")"`
SCRIPTPATH=`dirname "$SCRFILE"`

"$SCRIPTPATH/run_python_script.sh" "${SCRIPTPATH}/../test/pytest/WrwpXmlTestSuite.py" "${SCRIPTPATH}/../test/pytest"
exit $?

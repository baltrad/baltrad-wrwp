/* --------------------------------------------------------------------
Copyright (C) 2011 Swedish Meteorological and Hydrological Institute, SMHI

This is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This software is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with baltrad-wrwp.  If not, see <http://www.gnu.org/licenses/>.
------------------------------------------------------------------------*/
#include "wrwp.h"
#include "rave_debug.h"
#include <getopt.h>
#include <libgen.h>

static void PrintUsage(char* name, int full)
{
  char* namecopy = RAVE_STRDUP(name);

  printf("Usage: %s [options] <input volume.h5> <output verticalprofile.h5>\n", basename(namecopy));

  if (full) {
    printf("--help             - Prints this output\n");
    printf("--verbose          - Produces some information about the generated product\n");
    printf("--debug            - Produces some debug information during the generation\n");
    printf("--dz=<value>       - Height interval for deriving a profile [m] (default: %d)\n", DZ);
    printf("--nodata=<value>   - Nodata value (default: %d)\n", NODATA_VP);
    printf("--undetect=<value> - Undetect value (default: %d)\n", UNDETECT_VP);
    printf("--gain=<value>     - Gain value (default: %f)\n", GAIN_VP);
    printf("--offset=<value>   - Offset value (default: %f)\n", OFFSET_VP);
    printf("--hmax=<value>     - Maximum height of the profile [m] (default: %d)\n", HMAX);
    printf("--dmin=<value>     - Minimum distance for deriving a profile [m] (default: %d)\n", DMIN);
    printf("--dmax=<value>     - Maximum distance for deriving a profile [m] (default: %d)\n", DMAX);
    printf("--emin=<value>     - Minimum elevation angle [deg] (default: %f)\n", EMIN);
    printf("--vmin=<value>     - Radial velocity threshold [m/s] (default: %f)\n", VMIN);
    printf("\n");
    printf("<input volume.h>  must be a polar volume in ODIM H5 format\n");
    printf("<output verticalprofile.h5> will be a vertical profile in ODIM H5 format");
  }

  RAVE_FREE(namecopy);
}

/**
 * Tries to translate an string into an int value.
 * @param[in] arg - the argument
 * @param[in,out] pInt - the outvalue
 * @return 1 on success otherwise 0
 */
int ParseInt(char* arg, int* pInt)
{
  if (arg != NULL) {
    int i = 0;
    while (arg[i] != '\0') {
      if (arg[i] < '0' || arg[i] > '9') {
        return 0;
      }
      i++;
    }
  }
  if (arg != NULL && pInt != NULL && sscanf(arg, "%d",pInt) == 1) {
    return 1;
  }
  return 0;
}

/**
 * Tries to translate an string into an int value.
 * @param[in] arg - the argument
 * @param[in,out] pDouble - the outvalue
 * @return 1 on success otherwise 0
 */
int ParseDouble(char* arg, double* pDouble)
{
  if (arg != NULL) {
    int i = 0;
    int nrDots = 0;
    while (arg[i] != '\0') {
      if (arg[i] < '0' || arg[i] > '9') {
        if (arg[i] == '.' && nrDots==0) {
          nrDots++;
        } else {
          return 0;
        }
      }
      i++;
    }
  }
  if (arg != NULL && pDouble != NULL && sscanf(arg, "%lf",pDouble) == 1) {
    return 1;
  }
  return 0;
}

/** Main function for deriving weather radar wind and reflectivity profiles
 * @file
 * @author G�nther Haase, SMHI
 * @date 2011-11-29
 */
int main (int argc,char *argv[]) {
	RaveIO_t* raveio = NULL;
	PolarVolume_t* inobj = NULL;
	VerticalProfile_t* result = NULL;
	Wrwp_t* wrwp = NULL;
	int exitcode = 127;
	char* inputfile = NULL; /* do not delete */
	char* outputfile = NULL; /* do not delete */

	int verbose_flag=0;
	int debug_flag=0;
	int help_flag=0;
	int dz_value = DZ;
	int nodata_vp = NODATA_VP;
	int undetect_vp = UNDETECT_VP;
	double gain_vp = GAIN_VP;
	double offset_vp = OFFSET_VP;
	int hmax = HMAX;
	int dmin = DMIN;
	int dmax = DMAX;
	double emin = EMIN;
	double vmin = VMIN;

	int getopt_ret, option_index;

	struct option long_options[] = {
	    {"help", no_argument, &help_flag, 1},
	    {"verbose", no_argument, &verbose_flag, 1},
      {"debug", no_argument, &debug_flag, 1},
	    {"dz", optional_argument, 0, 'z'},
	    {"nodata", optional_argument, 0, 'n'},
	    {"undetect", optional_argument, 0, 'u'},
      {"gain", optional_argument, 0, 'g'},
      {"offset", optional_argument, 0, 'o'},
      {"hmax", optional_argument, 0, 'h'},
      {"dmin", optional_argument, 0, 'd'},
      {"dmax", optional_argument, 0, 'D'},
      {"emin", optional_argument, 0, 'e'},
      {"vmin", optional_argument, 0, 'v'},
	    {0, 0, 0, 0}
	};


	Rave_initializeDebugger();
	Rave_setDebugLevel(RAVE_INFO);

  while (1) {
      getopt_ret = getopt_long( argc, argv, "",
                                long_options,  &option_index);
      if (getopt_ret == -1) break;
      if (help_flag) {
        PrintUsage(argv[0], 1);
        exitcode=1;
        goto done;
      }
      if (verbose_flag) {
        Rave_setDebugLevel(RAVE_DEBUG);
      }

      switch(getopt_ret) {
      case 0:
        /* Here all non named options will arrive */
        /*printf("option %s\n", long_options[option_index].name);*/
        break;
      case 'z':
        if (!ParseInt(optarg, &dz_value)) {
          fprintf(stderr, "--dz=<value> must be an integer value\n");
          PrintUsage(argv[0], 0);
          goto done;
        }
        break;
      case 'n':
        if (!ParseInt(optarg, &nodata_vp)) {
          fprintf(stderr, "--nodata=<value> must be an integer value\n");
          PrintUsage(argv[0], 0);
          goto done;
        }
        break;
      case 'u':
        if (!ParseInt(optarg, &undetect_vp)) {
          fprintf(stderr, "--undetect=<value> must be an integer value\n");
          PrintUsage(argv[0], 0);
          goto done;
        }
        break;
      case 'g':
        if (!ParseDouble(optarg, &gain_vp)) {
          fprintf(stderr, "--gain=<value> must be a double value\n");
          PrintUsage(argv[0], 0);
          goto done;
        }
        break;
      case 'o':
        if (!ParseDouble(optarg, &offset_vp)) {
          fprintf(stderr, "--offset=<value> must be a double value\n");
          PrintUsage(argv[0], 0);
          goto done;
        }
        break;
      case 'h':
        if (!ParseInt(optarg, &hmax)) {
          fprintf(stderr, "--hmax=<value> must be an integer value\n");
          PrintUsage(argv[0], 0);
          goto done;
        }
        break;
      case 'd':
        if (!ParseInt(optarg, &dmin)) {
          fprintf(stderr, "--dmin=<value> must be an integer value\n");
          PrintUsage(argv[0], 0);
          goto done;
        }
        break;
      case 'D':
        if (!ParseInt(optarg, &dmax)) {
          fprintf(stderr, "--dmax=<value> must be an integer value\n");
          PrintUsage(argv[0], 0);
          goto done;
        }
        break;
      case 'e':
        if (!ParseDouble(optarg, &emin)) {
          fprintf(stderr, "--emin=<value> must be a double value\n");
          PrintUsage(argv[0], 0);
          goto done;
        }
        break;
      case 'v':
        if (!ParseDouble(optarg, &vmin)) {
          fprintf(stderr, "--vmin=<value> must be a double value\n");
          PrintUsage(argv[0], 0);
          goto done;
        }
        break;
      case '?':
      default:
        fprintf(stderr, "Unknown argument\n");
        PrintUsage(argv[0], 0);
        break;
      }
  }

  if (argc - optind != 2) {
    PrintUsage(argv[0], 0);
    goto done;
  }

  inputfile=argv[optind++];
  outputfile=argv[optind++];

  if (argc<3) {
		printf ("Usage: %s <input ODIM_H5 polar volume> <output ODIM_H5 polar volume> \n",argv[0]);
		exit (1);
	}

	raveio = RaveIO_open(inputfile);
	if (raveio == NULL) {
	  fprintf(stderr, "Failed to open file = %s\n", inputfile);
	  goto done;
	}

	wrwp = RAVE_OBJECT_NEW(&Wrwp_TYPE);
	if (wrwp == NULL) {
	  fprintf(stderr, "Failed to create wrwp object\n");
	  goto done;
	}

	Wrwp_setDZ(wrwp, dz_value);
	Wrwp_setNODATA_VP(wrwp, nodata_vp);
	Wrwp_setUNDETECT_VP(wrwp, undetect_vp);
	Wrwp_setOFFSET_VP(wrwp, offset_vp);
	Wrwp_setGAIN_VP(wrwp, gain_vp);
	Wrwp_setHMAX(wrwp, hmax);
	Wrwp_setDMIN(wrwp, dmin);
	Wrwp_setDMAX(wrwp, dmax);
	Wrwp_setEMIN(wrwp, emin);
	Wrwp_setVMIN(wrwp, vmin);

	/*Opening of HDF5 radar input file.*/
	if (RaveIO_getObjectType(raveio)== Rave_ObjectType_PVOL) {
		inobj = (PolarVolume_t*)RaveIO_getObject(raveio);
	}
	else {
		printf ("Input file is not a polar volume. Giving up ...\n");
		goto done;
	}
	RaveIO_close (raveio);

	result = Wrwp_generate(wrwp, inobj, NULL);
	if (inobj == NULL) {
		printf ("Could not derive wind profile %s, exiting ...\n", argv[1]);
		goto done;
	}

	RaveIO_setObject(raveio, (RaveCoreObject*)result);

	if (!RaveIO_save(raveio, outputfile))
	  goto done;

  if (debug_flag) {
    printf("Generated vertical profile...\n");
    printf("Input file: %s\n", inputfile);
    printf("Output file: %s\n", outputfile);
    printf("DZ         = %d\n", Wrwp_getDZ(wrwp));
    printf("NODATA     = %d\n", Wrwp_getNODATA_VP(wrwp));
    printf("UNDETECT   = %d\n", Wrwp_getUNDETECT_VP(wrwp));
    printf("GAIN       = %lf\n", Wrwp_getGAIN_VP(wrwp));
    printf("OFFSET     = %lf\n", Wrwp_getOFFSET_VP(wrwp));
    printf("HMAX       = %d\n", Wrwp_getHMAX(wrwp));
    printf("DMIN       = %d\n", Wrwp_getDMIN(wrwp));
    printf("DMAX       = %d\n", Wrwp_getDMAX(wrwp));
    printf("EMIN       = %lf\n", Wrwp_getEMIN(wrwp));
    printf("VMIN       = %lf\n", Wrwp_getVMIN(wrwp));
  }

	exitcode = 0;
done:
	RAVE_OBJECT_RELEASE(raveio);
	RAVE_OBJECT_RELEASE(inobj);
	RAVE_OBJECT_RELEASE(result);

	return exitcode;
}

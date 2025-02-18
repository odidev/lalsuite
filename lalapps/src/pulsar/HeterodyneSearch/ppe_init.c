/*
*  Copyright (C) 2014 Matthew Pitkin
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with with program; see the file COPYING. If not, write to the
*  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
*  MA  02110-1301  USA
*/

/******************************************************************************/
/*                      INITIALISATION FUNCTIONS                              */
/******************************************************************************/

#include "config.h"
#include "ppe_init.h"

/** Array for conversion from lowercase to uppercase */
static const CHAR a2A[256] = {
  ['a'] = 'A', ['b'] = 'B', ['c'] = 'C', ['d'] = 'D', ['e'] = 'E', ['f'] = 'F', ['g'] = 'G', ['h'] = 'H',
  ['i'] = 'I', ['j'] = 'J', ['k'] = 'K', ['l'] = 'L', ['m'] = 'M', ['n'] = 'N', ['o'] = 'O', ['p'] = 'P',
  ['q'] = 'Q', ['r'] = 'R', ['s'] = 'S', ['t'] = 'T', ['u'] = 'U', ['v'] = 'V', ['w'] = 'W', ['x'] = 'X',
  ['y'] = 'Y', ['z'] = 'Z' };


/** \brief Convert string to uppercase */
static void strtoupper(CHAR *s) {
  /* convert everything to uppercase */
  CHAR c;
  for ( ; *s; ++s ) {
    if ( (c = a2A[(int)(*s)]) ) { *s = c; }
  }
}


/**
 * \brief A wrapper around \c LALInferenceNestedSamplingAlgorithm
 *
 * This function just calls \c LALInferenceNestedSamplingAlgorithm, but will time the algorithm
 * if required.
 *
 * \param runState [] A pointer to the \c LALInferenceRunState
 */
void nested_sampling_algorithm_wrapper( LALInferenceRunState *runState ){
  /* timing values */
  struct timeval time1, time2;
  REAL8 tottime;

  if ( LALInferenceCheckVariable( runState->algorithmParams, "timefile" ) ){ gettimeofday(&time1, NULL); }

  LALInferenceNestedSamplingAlgorithm(runState);

  if ( LALInferenceCheckVariable( runState->algorithmParams, "timefile" ) ){
    gettimeofday(&time2, NULL);

    FILE *timefile = *(FILE **)LALInferenceGetVariable( runState->algorithmParams, "timefile" );
    UINT4 timenum = *(UINT4 *)LALInferenceGetVariable( runState->algorithmParams, "timenum" );
    tottime = (REAL8)((time2.tv_sec + time2.tv_usec*1.e-6) - (time1.tv_sec + time1.tv_usec*1.e-6));
    fprintf(timefile, "[%d] %s: %.9le secs\n", timenum, __func__, tottime);
    timenum++;
    /* get number of likelihood evaluations */
    UINT4 nlike = 0, ndata = 0;
    LALInferenceIFOData *tmpdata = runState->data;
    while ( tmpdata ){
      nlike += tmpdata->likeli_counter;
      ndata++;
      tmpdata = tmpdata->next;
    }
    fprintf(timefile, "[%d] Number of likelihood evaluations: %d\n", timenum, nlike/ndata);
    timenum++;
    check_and_add_fixed_variable( runState->algorithmParams, "timenum", &timenum, LALINFERENCE_UINT4_t );
  }
}


/**
 * \brief A wrapper around \c LALInferenceSetupLivePointsArray
 *
 * This function just calls \c LALInferenceSetupLivePointsArray, but will time the algorithm
 * if required.
 *
 * \param runState [] A pointer to the \c LALInferenceRunState
 */
void setup_live_points_array_wrapper( LALInferenceRunState *runState ){
  /* timing values */
  struct timeval time1, time2;
  REAL8 tottime;

  if ( LALInferenceCheckVariable( runState->algorithmParams, "timefile" ) ){ gettimeofday(&time1, NULL); }

  LALInferenceSetupLivePointsArray( runState );

  /* note that this time divided by the number of live points will give the approximate time per likelihood evaluation */
  if ( LALInferenceCheckVariable( runState->algorithmParams, "timefile" ) ){
    gettimeofday(&time2, NULL);

    FILE *timefile = *(FILE **)LALInferenceGetVariable( runState->algorithmParams, "timefile" );
    UINT4 timenum = *(UINT4 *)LALInferenceGetVariable( runState->algorithmParams, "timenum" );
    tottime = (REAL8)((time2.tv_sec + time2.tv_usec*1.e-6) - (time1.tv_sec + time1.tv_usec*1.e-6));
    fprintf(timefile, "[%d] %s: %.9le secs\n", timenum, __func__, tottime);
    timenum++;
    check_and_add_fixed_variable( runState->algorithmParams, "timenum", &timenum, LALINFERENCE_UINT4_t );
  }
}


/**
 * \brief Initialises the nested sampling algorithm control
 *
 * Memory is allocated for the parameters, priors and proposals. The nested sampling control parameters are set: the
 * number of live points \c Nlive, the number of points for each MCMC \c Nmcmc, the number of independent runs within
 * the algorithm \c Nruns, and the stopping criterion \c tolerance.
 *
 * The random number generator is initialise (the GSL Mersenne Twister algorithm \c gsl_rng_mt19937) using either a user
 * defined seed \c randomseed, the system defined \c /dev/random file, or the system clock time.
 *
 * \param runState [in] A pointer to the \c LALInferenceRunState
 */
void initialise_algorithm( LALInferenceRunState *runState )
{
  ProcessParamsTable *ppt = NULL;
  ProcessParamsTable *commandLine = runState->commandLine;
  REAL8 tmp;
  INT4 tmpi;
  INT4 randomseed;
  UINT4 verbose = 0;

  FILE *devrandom = NULL;
  struct timeval tv;

  /* print out help message */
  ppt = LALInferenceGetProcParamVal( commandLine, "--help" );
  if(ppt){
    fprintf(stderr, USAGE, commandLine->program);
    exit(0);
  }

  /* Initialise parameters structure */
  runState->algorithmParams = XLALCalloc( 1, sizeof(LALInferenceVariables) );
  runState->priorArgs = XLALCalloc( 1, sizeof(LALInferenceVariables) );
  runState->proposalArgs = XLALCalloc( 1, sizeof(LALInferenceVariables) );
  /* Initialise threads - single thread */
  runState->threads = LALInferenceInitThreads(1);

  ppt = LALInferenceGetProcParamVal( commandLine, "--verbose" );
  if( ppt ) {
    LALInferenceAddVariable( runState->algorithmParams, "verbose", &verbose , LALINFERENCE_UINT4_t, LALINFERENCE_PARAM_FIXED );
  }

  /* Number of live points */
  ppt = LALInferenceGetProcParamVal( commandLine, "--Nlive" );
  if( ppt ){
    tmpi = atoi( LALInferenceGetProcParamVal(commandLine, "--Nlive")->value );
    LALInferenceAddVariable( runState->algorithmParams,"Nlive", &tmpi, LALINFERENCE_INT4_t, LALINFERENCE_PARAM_FIXED );
  }
  else{
    if ( !LALInferenceGetProcParamVal( commandLine, "--inject-only" ) ){
      XLAL_ERROR_VOID(XLAL_EIO, "Error... Number of live point must be specified.");
    }
  }

  /* Number of points in MCMC chain */
  ppt = LALInferenceGetProcParamVal( commandLine, "--Nmcmc" );
  if( ppt ){
    tmpi = atoi( LALInferenceGetProcParamVal(commandLine, "--Nmcmc")->value );
    LALInferenceAddVariable( runState->algorithmParams, "Nmcmc", &tmpi, LALINFERENCE_INT4_t, LALINFERENCE_PARAM_FIXED );
  }

  /* set sloppiness! */
  ppt = LALInferenceGetProcParamVal(commandLine,"--sloppyfraction");
  if( ppt ) { tmp = atof(ppt->value); }
  else { tmp = 0.0; }
  LALInferenceAddVariable( runState->algorithmParams, "sloppyfraction", &tmp, LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_OUTPUT );

  /* Optionally specify number of parallel runs */
  ppt = LALInferenceGetProcParamVal( commandLine, "--Nruns" );
  if( ppt ) {
    tmpi = atoi( ppt->value );
    LALInferenceAddVariable( runState->algorithmParams, "Nruns", &tmpi, LALINFERENCE_INT4_t, LALINFERENCE_PARAM_FIXED );
  }

  /* Tolerance of the Nested sampling integrator */
  ppt = LALInferenceGetProcParamVal( commandLine, "--tolerance" );
  if( ppt ){
    tmp = strtod( ppt->value, (char **)NULL );
    LALInferenceAddVariable( runState->algorithmParams, "tolerance", &tmp, LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_FIXED );
  }

  /* Set cpu_time variable */
  REAL8 zero = 0.0;
  LALInferenceAddVariable( runState->algorithmParams, "cpu_time", &zero, LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_OUTPUT );

  /* Set up the random number generator */
  gsl_rng_env_setup();
  runState->GSLrandom = gsl_rng_alloc( gsl_rng_mt19937 );

  /* (try to) get random seed from command line: */
  ppt = LALInferenceGetProcParamVal( commandLine, "--randomseed" );
  if ( ppt != NULL ) { randomseed = atoi( ppt->value ); }
  else { /* otherwise generate "random" random seed: */
    if ( (devrandom = fopen("/dev/random","r")) == NULL ) {
      gettimeofday( &tv, 0 );
      randomseed = tv.tv_sec + tv.tv_usec;
    }
    else {
      if( fread(&randomseed, sizeof(randomseed), 1, devrandom) != 1 ){
        fprintf(stderr, "Error... could not read random seed\n");
        exit(3);
      }
      fclose( devrandom );
    }
  }

  gsl_rng_set( runState->GSLrandom, randomseed );

  /* check if we want to time the program */
  ppt = LALInferenceGetProcParamVal( commandLine, "--time-it" );
  if ( ppt != NULL ){
    FILE *timefile = NULL;
    UINT4 timenum = 1;

    ppt = LALInferenceGetProcParamVal( commandLine, "--outfile" );
    if ( !ppt ){
      XLAL_ERROR_VOID(XLAL_EFUNC, "Error... no output file is specified!");
    }

    CHAR *outtimefile = NULL;
    outtimefile = XLALStringDuplicate( ppt->value );
    /* strip the file extension */
    CHAR *dotloc = strrchr(outtimefile, '.');
    CHAR *slashloc = strrchr(outtimefile, '/');
    if ( dotloc != NULL ){
      if ( slashloc != NULL ){ /* check dot is after any filename seperator */
        if( slashloc < dotloc ){ *dotloc = '\0'; }
      }
      else{ *dotloc = '\0'; }
    }
    outtimefile = XLALStringAppend( outtimefile, "_timings" );

    if ( ( timefile = fopen(outtimefile, "w") ) == NULL ){
      fprintf(stderr, "Warning... cannot create a timing file, so proceeding without timings\n");
    }
    else{
      LALInferenceAddVariable( runState->algorithmParams, "timefile", &timefile, LALINFERENCE_void_ptr_t, LALINFERENCE_PARAM_FIXED );
      LALInferenceAddVariable( runState->algorithmParams, "timenum", &timenum, LALINFERENCE_UINT4_t, LALINFERENCE_PARAM_FIXED );
    }
    XLALFree( outtimefile );
  }

  /* log samples */
  runState->logsample = LALInferenceLogSampleToArray;

  return;
}


/**
 * \brief Sets the time angle antenna response lookup table
 *
 * This function sets up an antenna response lookup table in time for each detector from which data
 * exists (either real or fake). The time ranges over one sidereal day. The number of bins for the grid
 * over time can be specified on the command line via \c time-bins, but if this are not given then default
 * values are used. The data times as a fraction of a sidereal day from the start time will also be
 * calculated.
 *
 * \param runState [in] A pointer to the LALInferenceRunState
 * \param source [in] A pointer to a LALSource variable containing the source location
 */
void setup_lookup_tables( LALInferenceRunState *runState, LALSource *source ){
  /* Set up lookup tables */
  /* Using psi bins, time bins */
  ProcessParamsTable *ppt;
  ProcessParamsTable *commandLine = runState->commandLine;
  /* Single thread here */
  LALInferenceThreadState *threadState = &runState->threads[0];

  LALInferenceIFOData *data = runState->data;
  LALInferenceIFOModel *ifo_model = threadState->model->ifo;

  REAL8 t0;
  LALDetAndSource detAndSource;

  ppt = LALInferenceGetProcParamVal( commandLine, "--time-bins" );
  INT4 timeBins;
  if( ppt ) { timeBins = atoi( ppt->value ); }
  else { timeBins = TIMEBINS; } /* default time bins */

  while( ifo_model ){
    REAL8Vector *arespT = NULL, *brespT = NULL;
    REAL8Vector *arespV = NULL, *brespV = NULL;
    REAL8Vector *arespS = NULL, *brespS = NULL;

    REAL8Vector *sidDayFrac = NULL;
    REAL8 dt = 0;
    UINT4 i = 0;

    LALInferenceAddVariable( ifo_model->params, "timeSteps", &timeBins, LALINFERENCE_INT4_t, LALINFERENCE_PARAM_FIXED );

    t0 = XLALGPSGetREAL8( &data->compTimeData->epoch );

    sidDayFrac = XLALCreateREAL8Vector( ifo_model->times->length );

    /* set the time in sidereal days since the first data point (mod 1 sidereal day) */
    for( i = 0; i < ifo_model->times->length; i++ ){
      sidDayFrac->data[i] = fmod( XLALGPSGetREAL8( &ifo_model->times->data[i] ) - t0, LAL_DAYSID_SI );
    }

    LALInferenceAddVariable( ifo_model->params, "siderealDay", &sidDayFrac, LALINFERENCE_REAL8Vector_t, LALINFERENCE_PARAM_FIXED );

    detAndSource.pDetector = data->detector;
    detAndSource.pSource = source;

    arespT = XLALCreateREAL8Vector( timeBins );
    brespT = XLALCreateREAL8Vector( timeBins );
    arespV = XLALCreateREAL8Vector( timeBins );
    brespV = XLALCreateREAL8Vector( timeBins );
    arespS = XLALCreateREAL8Vector( timeBins );
    brespS = XLALCreateREAL8Vector( timeBins );

    dt = LALInferenceGetREAL8Variable( ifo_model->params, "dt" );

    response_lookup_table( t0, detAndSource, timeBins, dt, arespT, brespT, arespV, brespV, arespS, brespS );

    LALInferenceAddVariable( ifo_model->params, "a_response_tensor", &arespT, LALINFERENCE_REAL8Vector_t, LALINFERENCE_PARAM_FIXED );
    LALInferenceAddVariable( ifo_model->params, "b_response_tensor", &brespT, LALINFERENCE_REAL8Vector_t, LALINFERENCE_PARAM_FIXED );

    if ( LALInferenceCheckVariable( ifo_model->params, "nonGR" ) ){
      LALInferenceAddVariable( ifo_model->params, "a_response_vector", &arespV, LALINFERENCE_REAL8Vector_t, LALINFERENCE_PARAM_FIXED );
      LALInferenceAddVariable( ifo_model->params, "b_response_vector", &brespV, LALINFERENCE_REAL8Vector_t, LALINFERENCE_PARAM_FIXED );
      LALInferenceAddVariable( ifo_model->params, "a_response_scalar", &arespS, LALINFERENCE_REAL8Vector_t, LALINFERENCE_PARAM_FIXED );
      LALInferenceAddVariable( ifo_model->params, "b_response_scalar", &brespS, LALINFERENCE_REAL8Vector_t, LALINFERENCE_PARAM_FIXED );
    }
    else{
      XLALDestroyREAL8Vector( arespV );
      XLALDestroyREAL8Vector( brespV );
      XLALDestroyREAL8Vector( arespS );
      XLALDestroyREAL8Vector( brespS );
    }

    data = data->next;
    ifo_model = ifo_model->next;
  }

  return;
}


/**
 * \brief Set up all the allowed variables for a known pulsar search
 * This functions sets up all possible variables that are possible in a known pulsar search. Parameter values read in
 * from a .par file and passed in via the \c pars variable will be set.
 *
 * \param ini [in] A pointer to a \c LALInferenceVariables type that will be filled in with pulsar parameters
 * \param pars [in] A \c PulsarParameters type containing pulsar parameters read in from a TEMPO-style .par file
 */
void add_initial_variables( LALInferenceVariables *ini, PulsarParameters *pars ){
  /* amplitude model parameters for l=m=2 harmonic emission */
  add_variable_parameter( pars, ini, "H0", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "PHI0", LALINFERENCE_PARAM_FIXED ); /* note that this is rotational phase */
  add_variable_parameter( pars, ini, "COSIOTA", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "IOTA", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "PSI", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "Q22", LALINFERENCE_PARAM_FIXED ); /* mass quadrupole, Q22 */

  /* amplitude model parameters for l=2, m=1 and 2 harmonic emission from Jones (2010) */
  add_variable_parameter( pars, ini, "I21", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "I31", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "LAMBDA", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "COSTHETA", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "THETA", LALINFERENCE_PARAM_FIXED );

  /* amplitude model parameters in phase and amplitude form */
  add_variable_parameter( pars, ini, "C22", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "C21", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "PHI22", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "PHI21", LALINFERENCE_PARAM_FIXED );

  /***** phase model parameters ******/
  if ( PulsarCheckParam(pars, "F") ){ /* frequency and frequency derivative parameters */
    UINT4 i = 0;
    const REAL8Vector *freqs = PulsarGetREAL8VectorParam( pars, "F" );
    /* add each frequency and derivative value as a seperate parameter (also set a value that is the FIXED value to be used for calculating phase differences) */
    for ( i = 0; i < freqs->length; i++ ){
      CHAR varname[256];
      snprintf(varname, sizeof(varname), "F%u", i);
      REAL8 fval = PulsarGetREAL8VectorParamIndividual( pars, varname );
      LALInferenceAddVariable( ini, varname, &fval, LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_FIXED );
      snprintf(varname, sizeof(varname), "F%u_FIXED", i);
      LALInferenceAddVariable( ini, varname, &fval, LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_FIXED ); /* add FIXED value */
    }
    /* add value with the number of FB parameters given */
    LALInferenceAddVariable( ini, "FREQNUM", &i, LALINFERENCE_UINT4_t, LALINFERENCE_PARAM_FIXED );
  }
  add_variable_parameter( pars, ini, "PEPOCH", LALINFERENCE_PARAM_FIXED );

  /* add non-GR parameters */
  add_variable_parameter( pars, ini, "CGW", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "HPLUS", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "HCROSS", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "PSITENSOR", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "PHI0TENSOR", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "HSCALARB", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "HSCALARL", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "PSISCALAR", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "PHI0SCALAR", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "HVECTORX", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "HVECTORY", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "PSIVECTOR", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "PHI0VECTOR", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "HPLUS_F", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "HCROSS_F", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "PSITENSOR_F", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "PHI0TENSOR_F", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "HSCALARB_F", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "HSCALARL_F", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "PSISCALAR_F", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "PHI0SCALAR_F", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "HVECTORX_F", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "HVECTORY_F", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "PSIVECTOR_F", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "PHI0VECTOR_F", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "H0_F", LALINFERENCE_PARAM_FIXED );

  /* sky position */
  REAL8 ra = 0.;
  if ( PulsarCheckParam( pars, "RA" ) ) { ra = PulsarGetREAL8Param( pars, "RA" ); }
  else if ( PulsarCheckParam( pars, "RAJ" ) ) { ra = PulsarGetREAL8Param( pars, "RAJ" ); }
  else {
    XLAL_ERROR_VOID( XLAL_EINVAL, "No source right ascension specified!" );
  }
  REAL8 dec = 0.;
  if ( PulsarCheckParam( pars, "DEC" ) ) { dec = PulsarGetREAL8Param( pars, "DEC" ); }
  else if ( PulsarCheckParam( pars, "DECJ" ) ) { dec = PulsarGetREAL8Param( pars, "DECJ" ); }
  else {
    XLAL_ERROR_VOID( XLAL_EINVAL, "No source declination specified!" );
  }
  LALInferenceAddVariable( ini, "RA", &ra, LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_FIXED );
  LALInferenceAddVariable( ini, "DEC", &dec, LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_FIXED );

  add_variable_parameter( pars, ini, "PMRA", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "PMDEC", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "POSEPOCH", LALINFERENCE_PARAM_FIXED );

  /* source distance and parallax */
  add_variable_parameter( pars, ini, "DIST", LALINFERENCE_PARAM_FIXED );
  add_variable_parameter( pars, ini, "PX", LALINFERENCE_PARAM_FIXED );

  /* only add binary system parameters if required */
  if ( PulsarCheckParam( pars, "BINARY" ) ){
    CHAR *binary = XLALStringDuplicate(PulsarGetStringParam(pars, "BINARY"));
    LALInferenceAddVariable( ini, "BINARY", &binary, LALINFERENCE_string_t, LALINFERENCE_PARAM_FIXED );

    add_variable_parameter( pars, ini, "PB", LALINFERENCE_PARAM_FIXED );
    add_variable_parameter( pars, ini, "ECC", LALINFERENCE_PARAM_FIXED );
    add_variable_parameter( pars, ini, "EPS1", LALINFERENCE_PARAM_FIXED );
    add_variable_parameter( pars, ini, "EPS2", LALINFERENCE_PARAM_FIXED );
    add_variable_parameter( pars, ini, "T0", LALINFERENCE_PARAM_FIXED );
    add_variable_parameter( pars, ini, "TASC", LALINFERENCE_PARAM_FIXED );
    add_variable_parameter( pars, ini, "A1", LALINFERENCE_PARAM_FIXED );
    add_variable_parameter( pars, ini, "OM", LALINFERENCE_PARAM_FIXED );

    add_variable_parameter( pars, ini, "PB_2", LALINFERENCE_PARAM_FIXED );
    add_variable_parameter( pars, ini, "ECC_2", LALINFERENCE_PARAM_FIXED );
    add_variable_parameter( pars, ini, "T0_2", LALINFERENCE_PARAM_FIXED );
    add_variable_parameter( pars, ini, "A1_2", LALINFERENCE_PARAM_FIXED );
    add_variable_parameter( pars, ini, "OM_2", LALINFERENCE_PARAM_FIXED );

    add_variable_parameter( pars, ini, "PB_3", LALINFERENCE_PARAM_FIXED );
    add_variable_parameter( pars, ini, "ECC_3", LALINFERENCE_PARAM_FIXED );
    add_variable_parameter( pars, ini, "T0_3", LALINFERENCE_PARAM_FIXED );
    add_variable_parameter( pars, ini, "A1_3", LALINFERENCE_PARAM_FIXED );
    add_variable_parameter( pars, ini, "OM_3", LALINFERENCE_PARAM_FIXED );

    add_variable_parameter( pars, ini, "XPBDOT", LALINFERENCE_PARAM_FIXED );
    add_variable_parameter( pars, ini, "EPS1DOT", LALINFERENCE_PARAM_FIXED );
    add_variable_parameter( pars, ini, "EPS2DOT", LALINFERENCE_PARAM_FIXED );
    add_variable_parameter( pars, ini, "OMDOT", LALINFERENCE_PARAM_FIXED );
    add_variable_parameter( pars, ini, "GAMMA", LALINFERENCE_PARAM_FIXED );
    add_variable_parameter( pars, ini, "PBDOT", LALINFERENCE_PARAM_FIXED );
    add_variable_parameter( pars, ini, "XDOT", LALINFERENCE_PARAM_FIXED );
    add_variable_parameter( pars, ini, "EDOT", LALINFERENCE_PARAM_FIXED );

    add_variable_parameter( pars, ini, "SINI", LALINFERENCE_PARAM_FIXED );
    add_variable_parameter( pars, ini, "DR", LALINFERENCE_PARAM_FIXED );
    add_variable_parameter( pars, ini, "DTHETA", LALINFERENCE_PARAM_FIXED );
    add_variable_parameter( pars, ini, "A0", LALINFERENCE_PARAM_FIXED );
    add_variable_parameter( pars, ini, "B0", LALINFERENCE_PARAM_FIXED );
    add_variable_parameter( pars, ini, "MTOT", LALINFERENCE_PARAM_FIXED );
    add_variable_parameter( pars, ini, "M2", LALINFERENCE_PARAM_FIXED );

    if ( PulsarCheckParam(pars, "FB") ){
      UINT4 i = 0;
      const REAL8Vector *fb = PulsarGetREAL8VectorParam( pars, "FB" );
      /* add each FB value as a seperate parameter */
      for ( i = 0; i < fb->length; i++ ){
        CHAR varname[256];
        snprintf(varname, sizeof(varname), "FB%u", i);
        REAL8 fbval = PulsarGetREAL8VectorParamIndividual( pars, varname );
        LALInferenceAddVariable( ini, varname, &fbval, LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_FIXED );
      }
      /* add value with the number of FB parameters given */
      LALInferenceAddVariable( ini, "FBNUM", &i, LALINFERENCE_UINT4_t, LALINFERENCE_PARAM_FIXED );
    }
  }

  /* check for glitches (searching on glitch epochs GLEP) */
  if ( PulsarCheckParam(pars, "GLEP") ){
    UINT4 i = 0, j = 0, glnum = 0;
    for ( i = 0; i < NUMGLITCHPARS; i++ ){
      if ( PulsarCheckParam( pars, glitchpars[i] ) ){
        const REAL8Vector *glv = PulsarGetREAL8VectorParam( pars, glitchpars[i] );
        for ( j = 0; j < glv->length; j++ ){
          CHAR varname[300];
          snprintf(varname, sizeof(varname), "%s_%u", glitchpars[i], j+1);
          REAL8 glval = PulsarGetREAL8VectorParamIndividual( pars, varname );
          LALInferenceAddVariable( ini, varname, &glval, LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_FIXED );
        }
        if ( glv->length > glnum ) { glnum = glv->length; } /* find max number of glitch parameters */
      }
    }
    /* add value with the number of glitch parameters given */
    LALInferenceAddVariable( ini, "GLNUM", &glnum, LALINFERENCE_UINT4_t, LALINFERENCE_PARAM_FIXED );
  }
}


/**
 * \brief Sets up the parameters to be searched over and their prior ranges
 *
 * This function sets up any parameters that you require the code to search over and specifies the prior range and type
 * for each. This information is contained in a prior file specified by the command line argument \c prior-file. There
 * are currently five different allowed prior distributions:
 *   - "uniform": A flat distribution between a minimum and maximum range (with zero probabiility outside that range);
 *   - "gaussian": A Gaussian/Normal distribution defined by a mean and standard devaition;
 *   - "fermidirac": A Fermi-Dirac-like distribution defined by a knee and attenuation width;
 *   - "gmm": A one- or more-dimensional Gaussian Mixture model defined by the means, standard deviations and weights of each mode.
 *   - "loguniform": A flat distribution in log-space (proportional to the inverse of the value in non-log-space)
 * For all priors the first two columns of the prior definition should be:
 *   -# the name of a parameter to be searched over, and
 *   -# the prior type (e.g. "uniform", "gaussian", "fermidirac", "gmm" or "loguniform".
 * For the "uniform, "loguniform", "gaussian", "fermidirac" priors the prior file should contain the a further two columns containing:
 *   -# the lower limit ("uniform" and "loguniform"), mean ("gaussian"), or sigma ("fermidirac") of the distribution;
 *   -# the upper limit ("uniform" and "loguniform"), standard deviation ("gaussian"), or r ("fermidirac") of the distribution.
 * For the "gmm" prior the file should contain at least a further four columns. The third column gives the number of modes
 * for the mixture. The fourth columns is a list of means for each mode (with sub-lists of means
 * for each parameter). The fifth column is a list of covariance matrices for each mode. The sixth
 * column is a list of weights for each mode (weights can be relative weights as they will be
 * normalised when parsed by the code). Finally, there can be additional lists containing pairs
 * of values for each parameter giving lower and upper limits of the distribution (note that the 
 * weights given are weights assuming that there are no bounds on the distributions).
 *
 * Some examples of files are:
 * \code
 * H0 uniform 0 1e-21
 * PHI0 uniform 0 3.14159265359
 * COSIOTA uniform -1 1
 * PSI uniform -0.785398163397448 0.785398163397448
 * \endcode
 * or
 * \code
 * H0 fermidirac 3.3e-26 9.16
 * PHI0 uniform 0 3.14159265359
 * IOTA uniform 0 3.14159265359
 * PSI uniform -0.785398163397448 0.785398163397448
 * \endcode
 * or
 * \code
 * H0 gmm 3 [[1e-25],[2e-25],[1e-26]] [[[2e-24]],[[3e-24]],[[1e-25]]] [1.5,2.5,0.2] [0.0,1e-20]
 * PHI0 uniform 0 3.14159265359
 * COSIOTA uniform -1 1
 * PSI uniform -0.785398163397448 0.785398163397448
 * \endcode
 *
 * Any parameter specified in the file will have its vary type set to \c LALINFERENCE_PARAM_LINEAR.
 *
 * If a parameter correlation matrix is given by the \c cor-file command then this is used to construct a multi-variate
 * Gaussian prior for the given parameters (it is assumed that this file is created using TEMPO and the parameters it
 * contains are the same as those for which a standard deviation is defined in the par file). This overrules the
 * Gaussian priors that will have been set for these parameters.
 *
 * \param runState [in] A pointer to the LALInferenceRunState
 */
void initialise_prior( LALInferenceRunState *runState )
{
  CHAR *propfile = NULL;
  /* Single thread here */
  LALInferenceThreadState *threadState = &runState->threads[0];
  ProcessParamsTable *ppt;
  ProcessParamsTable *commandLine = runState->commandLine;

  CHAR *tempPar = NULL, *tempPrior = NULL;
  REAL8 low, high;

  LALInferenceIFOModel *ifo = NULL;
  if ( !LALInferenceGetProcParamVal(commandLine, "--test-gaussian-likelihood" ) ){
    ifo = threadState->model->ifo;
  }

  CHAR *filebuf =  NULL; /* buffer to store prior file */
  TokenList *tlist = NULL;
  UINT4 k = 0, nvals = 0;

  /* parameters in correlation matrix */
  LALStringVector *corParams = NULL;
  REAL8Array *corMat = NULL;

  INT4 varyphase = 0, varyskypos = 0, varybinary = 0, varyglitch = 0;

  ppt = LALInferenceGetProcParamVal( commandLine, "--prior-file" );
  if( ppt ) { propfile = XLALStringDuplicate( LALInferenceGetProcParamVal(commandLine,"--prior-file")->value ); }
  else{
    fprintf(stderr, "Error... --prior-file is required.\n");
    fprintf(stderr, USAGE, commandLine->program);
    exit(0);
  }

  /* read in prior file and separate lines */
  filebuf = XLALFileLoad( propfile );
  if ( XLALCreateTokenList( &tlist, filebuf, "\n" ) != XLAL_SUCCESS ){
    XLAL_ERROR_VOID(XLAL_EINVAL, "Error... could not convert data into separate lines.\n");
  }

  /* parse through priors */
  for ( k = 0; k < tlist->nTokens; k++ ){
    UINT4 isthere = 0, i = 0;
    LALInferenceParamVaryType varyType;

    /* check for comment line starting with '#' or '%' */
    if ( tlist->tokens[k][0] == '#' || tlist->tokens[k][0] == '%' ){ continue; }

    /* check the number of values in the line by counting the whitespace separated values */
    nvals = 0;
    TokenList *tline = NULL;
    XLALCreateTokenList( &tline, tlist->tokens[k], " \t" );
    nvals = tline->nTokens;

    if ( nvals < 2 ){
      fprintf(stderr, "Warning... number of values ('%d') on line '%d' in prior file is different than expected:\n\t'%s'", nvals, k+1, tlist->tokens[k]);
      XLALDestroyTokenList( tline );
      continue;
    }

    tempPar = XLALStringDuplicate( tline->tokens[0] );
    strtoupper( tempPar ); /* convert tempPar to all uppercase letters */
    tempPrior = XLALStringDuplicate( tline->tokens[1] );

    /* check if there is more than one parameter in tempPar, separated by a ':', for us in GMM prior */
    TokenList *parnames = NULL;
    XLALCreateTokenList( &parnames, tempPar, ":" ); /* find number of parameters used by GMM (parameters should be ':' separated */
    UINT4 npars = parnames->nTokens;

    /* gaussian, uniform, loguniform and fermi-dirac priors should all have four values to a line */
    if ( !strcmp(tempPrior, "uniform") || !strcmp(tempPrior, "loguniform") || !strcmp(tempPrior, "gaussian") || !strcmp(tempPrior, "fermidirac") ){
      if ( npars > 1 ){
        XLAL_ERROR_VOID( XLAL_EINVAL, "Error... 'uniform', 'loguniform', 'gaussian', or 'fermidirac' priors must only be given for single parameters." );
      }

      if ( nvals != 4 ){
        XLAL_ERROR_VOID( XLAL_EINVAL, "Error... 'uniform', 'loguniform', 'gaussian', or 'fermidirac' priors must specify four values." );
      }

      low = atof(tline->tokens[2]);
      high = atof(tline->tokens[3]);

      if ( !strcmp(tempPrior, "uniform") || !strcmp(tempPrior, "loguniform") ){
        if( high < low ){
          XLAL_ERROR_VOID( XLAL_EINVAL, "Error... In %s the %s parameters ranges are wrongly set.", propfile, tempPar );
        }
      }

      if ( strcmp(tempPrior, "uniform") && strcmp(tempPrior, "loguniform") && strcmp(tempPrior, "gaussian") && strcmp(tempPrior, "fermidirac") ){
        XLAL_ERROR_VOID( XLAL_EINVAL, "Error... prior type '%s' not recognised", tempPrior );
      }

      /* Add the prior variables */
      if ( !strcmp(tempPrior, "uniform") ){
        LALInferenceAddMinMaxPrior( runState->priorArgs, tempPar, &low, &high, LALINFERENCE_REAL8_t );
      }
      else if ( !strcmp(tempPrior, "loguniform") ){
        LALInferenceAddLogUniformPrior( runState->priorArgs, tempPar, &low, &high, LALINFERENCE_REAL8_t );
      }
      else if( !strcmp(tempPrior, "gaussian") ){
        LALInferenceAddGaussianPrior( runState->priorArgs, tempPar, &low, &high, LALINFERENCE_REAL8_t );
      }
      else if( !strcmp(tempPrior, "fermidirac") ){
        LALInferenceAddFermiDiracPrior( runState->priorArgs, tempPar, &low, &high, LALINFERENCE_REAL8_t );
      }
    }
    else if ( !strcmp(tempPrior, "gmm") ){
      /* check if using a 1D/multi-dimensional Gaussian Mixture Model prior e.g.:
       * H0:COSIOTA gmm 2 [[1e-23,0.3],[2e-23,0.4]] [[[1e-45,2e-25],[2e-25,0.02]],[[4e-46,2e-24],[2e-24,0.1]]] [0.2,0.4] [0.,1e-22] [-1.,1.]
       * where the third value is the number of modes followed by: a list of means for each parameter for each mode; the covariance
       * matrices for each mode; the weights for each mode; and if required sets of pairs of minimum and maximum prior ranges for each
       * parameter.
       */
      if ( nvals < 6 ){
        fprintf(stderr, "Warning... number of values ('%d') on line '%d' in prior file is different than expected:\n\t'%s'", nvals, k+1, tlist->tokens[k]);
        XLALDestroyTokenList( tline );
        continue;
      }

      UINT4 nmodes = (UINT4)atoi( tline->tokens[2] ); /* get the number of modes */

      /* get means of modes for each parameter */
      REAL8Vector **gmmmus;
      gmmmus = parse_gmm_means(tline->tokens[3], npars, nmodes);
      if ( !gmmmus ){
        XLAL_ERROR_VOID(XLAL_EINVAL, "Error... problem parsing GMM prior mean values for '%s'.", tempPar);
      }

      /* get the covariance matrices for the modes */
      gsl_matrix **gmmcovs;
      gmmcovs = parse_gmm_covs(tline->tokens[4], npars, nmodes);
      if ( !gmmcovs ){
        XLAL_ERROR_VOID(XLAL_EINVAL, "Error... problem parsing GMM prior covariance matrix values for '%s'.", tempPar);
      }

      /* get weights for the modes */
      REAL8Vector *gmmweights = NULL;
      gmmweights = XLALCreateREAL8Vector( nmodes );

      CHAR strpart[8192];
      CHAR UNUSED *nextpart;
      nextpart = get_bracketed_string(strpart, tline->tokens[5], '[', ']');
      if ( !strpart[0] ){
        XLAL_ERROR_VOID(XLAL_EINVAL, "Error... problem parsing GMM prior weights values for '%s'.", tempPar);
      }

      /* parse comma separated weights */
      TokenList *weightvals = NULL;
      XLALCreateTokenList( &weightvals, strpart, "," );
      if ( weightvals->nTokens != nmodes ){
        XLAL_ERROR_VOID(XLAL_EINVAL, "Error... problem parsing GMM prior weights values for '%s'.", tempPar);
      }
      for ( UINT4 j=0; j < nmodes; j++ ){ gmmweights->data[j] = atof(weightvals->tokens[j]); }
      XLALDestroyTokenList( weightvals );

      REAL8 minval = -INFINITY, maxval = INFINITY;
      REAL8Vector *minvals = XLALCreateREAL8Vector( npars ), *maxvals = XLALCreateREAL8Vector( npars );

      /* check if minimum and maximum bounds are specified (otherwise set to +/- infinity) */
      /* there are minimum and maximum values, e.g. [h0min,h0max] [cosiotamin,cosiotamax] */
      for ( UINT4 j=0; j < npars; j++ ){
        REAL8 thismin = minval, thismax = maxval;
        if ( tline->nTokens > 6+j ){
          nextpart = get_bracketed_string(strpart, tline->tokens[6+j], '[', ']');
          if ( !strpart[0] ){
            XLAL_ERROR_VOID(XLAL_EINVAL, "Error... problem parsing GMM prior limit values for '%s'.", tempPar);
          }
          TokenList *minmaxvals = NULL;
          XLALCreateTokenList( &minmaxvals, strpart, "," );
          if ( minmaxvals->nTokens == 2 ){
            if ( isfinite(atof(minmaxvals->tokens[0])) ){
              thismin = atof(minmaxvals->tokens[0]);
            }
            if ( isfinite(atof(minmaxvals->tokens[1])) ){
	      thismax = atof(minmaxvals->tokens[1]);
	    }
          }
          XLALDestroyTokenList( minmaxvals );
        }
        minvals->data[j] = thismin;
        maxvals->data[j] = thismax;
      }

      LALInferenceAddGMMPrior( runState->priorArgs, tempPar, &gmmmus, &gmmcovs, &gmmweights, &minvals, &maxvals );
    }
    else{
      XLAL_ERROR_VOID( XLAL_EINVAL, "Error... prior type '%s' not recognised", tempPrior );
    }

    /* if there is a phase parameter defined in the proposal then set varyphase to 1 */
    for ( UINT4 j = 0; j < npars; j++ ){
      for ( i = 0; i < NUMAMPPARS; i++ ){
        if ( !strcmp(parnames->tokens[j], amppars[i]) ){
          isthere = 1;
          break;
        }
      }
      if ( !isthere ) { varyphase = 1; }

      /* check if there are sky position parameters that will be searched over */
      for ( i = 0; i < NUMSKYPARS; i++ ){
        if ( !strcmp(parnames->tokens[j], skypars[i]) ){
          varyskypos = 1;
          break;
        }
      }

      /* check if there are any binary parameters that will be searched over */
      for ( i = 0; i < NUMBINPARS; i++ ){
        if ( !strcmp(parnames->tokens[j], binpars[i]) ){
          varybinary = 1;
          break;
        }
      }

      /* check is there are any glitch parameters that will be searched over */
      for ( i = 0; i < NUMGLITCHPARS; i++ ){
        if ( !strncmp(parnames->tokens[j], glitchpars[i], strlen(glitchpars[i]) * sizeof(CHAR)) ){
          varyglitch = 1;
          break;
        }
      }

      /* set variable type to LINEAR (as they are initialised as FIXED) */
      varyType = LALINFERENCE_PARAM_LINEAR;
      LALInferenceSetParamVaryType( threadState->currentParams, parnames->tokens[j], varyType );
    }

    XLALDestroyTokenList( parnames );
    XLALDestroyTokenList( tline );
  }

  XLALDestroyTokenList( tlist );
  XLALFree( filebuf );

  LALInferenceIFOModel *ifotemp = ifo;
  while( ifotemp ){
    /* add in variables to say whether phase, sky position and binary parameter are varying */
    if( varyphase ) { LALInferenceAddVariable( ifotemp->params, "varyphase", &varyphase, LALINFERENCE_INT4_t, LALINFERENCE_PARAM_FIXED ); }
    if( varyskypos ) { LALInferenceAddVariable( ifotemp->params, "varyskypos", &varyskypos, LALINFERENCE_INT4_t, LALINFERENCE_PARAM_FIXED ); }
    if( varybinary ) { LALInferenceAddVariable( ifotemp->params, "varybinary", &varybinary, LALINFERENCE_INT4_t, LALINFERENCE_PARAM_FIXED ); }
    if( varyglitch ) { LALInferenceAddVariable( ifotemp->params, "varyglitch", &varyglitch, LALINFERENCE_INT4_t, LALINFERENCE_PARAM_FIXED ); }

    ifotemp = ifotemp->next;
  }

  /* now check for a parameter correlation coefficient matrix file */
  ppt = LALInferenceGetProcParamVal( runState->commandLine, "--cor-file" );
  if( ppt ){
    CHAR *corFile = XLALStringDuplicate( ppt->value );
    UINT4Vector *dims = XLALCreateUINT4Vector( 2 );
    dims->data[0] = 1;
    dims->data[1] = 1;

    corMat = XLALCreateREAL8Array( dims );

    corParams = XLALReadTEMPOCorFile( corMat, corFile );

    /* if the correlation matrix is given then add it as the prior for values with Gaussian errors specified in the par file */
    add_correlation_matrix( threadState->currentParams, runState->priorArgs, corMat, corParams );

    XLALDestroyUINT4Vector( dims );
  }

  /* check if using a previous nested sampling file as a prior */
  samples_prior( runState );

  return;
}


/**
 * \brief Initialise the MCMC proposal distribution for sampling new points
 *
 * There are various proposal distributions that can be used to sample new live points via an MCMC. A combination of
 * different ones can be used to help efficiency for awkward posterior distributions. Here the proposals that can be
 * used are:
 * \c diffev Drawing a new point by differential evolution of two randomly chosen live points. All parameters are
 * evolved during a single draw.
 * \c freqBinJump Jumps that are the size of the Fourier frequency bins (can be used if searching over frequency).
 * \c ensembleStretch Ensemble stretch moves (WARNING: These can lead to long autocorrelation lengths).
 * \c ensembleWalk Ensemble walk moves. These are used as the default proposal.
 * \c uniformprop Points for any parameters with uniform priors are drawn from those priors
 *
 * This function sets up the relative weights with which each of above distributions is used.
 *
 * \param runState [in] A pointer to the run state
 */
void initialise_proposal( LALInferenceRunState *runState ){
  ProcessParamsTable *ppt = NULL;
  UINT4 defrac = 0, freqfrac = 0, esfrac = 0, ewfrac = 0, flatfrac = 0;

  ppt = LALInferenceGetProcParamVal( runState->commandLine, "--diffev" );
  if( ppt ) { defrac = atoi( ppt->value ); }
  else { defrac = 0; } /* default value */

  ppt = LALInferenceGetProcParamVal( runState->commandLine, "--freqBinJump" );
  if( ppt ) { freqfrac = atoi( ppt->value ); }

  ppt = LALInferenceGetProcParamVal(runState->commandLine, "--ensembleStretch" );
  if ( ppt ) { esfrac = atoi( ppt->value ); }
  else { esfrac = 0; }

  ppt = LALInferenceGetProcParamVal(runState->commandLine, "--ensembleWalk" );
  if ( ppt ) { ewfrac = atoi( ppt->value ); }
  else { ewfrac = 3; }

  ppt = LALInferenceGetProcParamVal(runState->commandLine, "--uniformprop" );
  if ( ppt ) { flatfrac = atoi( ppt->value ); }
  else { flatfrac = 1; }

  if( !defrac && !freqfrac && !ewfrac && !esfrac ){
    XLAL_ERROR_VOID(XLAL_EFAILED, "All proposal weights are zero!");
  }

  /* Single thread here */
  LALInferenceThreadState *threadState = &runState->threads[0];
  threadState->cycle = LALInferenceInitProposalCycle();
  LALInferenceProposalCycle *cycle=threadState->cycle;
  /* add proposals */
  if( defrac ){
    LALInferenceAddProposalToCycle(cycle,
                                   LALInferenceInitProposal(&LALInferenceDifferentialEvolutionFull,differentialEvolutionFullName),
                                   defrac);
  }

  if ( freqfrac ){
    LALInferenceAddProposalToCycle(cycle,
                                   LALInferenceInitProposal(&LALInferenceFrequencyBinJump,frequencyBinJumpName),
                                   freqfrac);
  }

  /* Use ensemble moves */
  if ( esfrac ){
    LALInferenceAddProposalToCycle(cycle,
                                   LALInferenceInitProposal(&LALInferenceEnsembleStretchFull,ensembleStretchFullName),
                                   esfrac);
  }

  if ( ewfrac ){
    LALInferenceAddProposalToCycle(cycle,
                                   LALInferenceInitProposal(&LALInferenceEnsembleWalkFull,ensembleWalkFullName),
                                   ewfrac);
  }

  if ( flatfrac ){
    LALInferenceAddProposalToCycle(cycle,
                                   LALInferenceInitProposal(&LALInferenceDrawFlatPrior,drawFlatPriorName),
                                   flatfrac);
  }

  LALInferenceRandomizeProposalCycle( cycle, runState->GSLrandom );

  /* set proposal */
  threadState->proposal = LALInferenceCyclicProposal;
  LALInferenceZeroProposalStats(threadState->cycle);
}


/**
 * \brief Adds a correlation matrix for a multi-variate Gaussian prior
 *
 * If a TEMPO-style parameter correlation coefficient file has been given, then this function will use it to set the
 * prior distribution for the given parameters. It is assumed that the equivalent par file contained standard
 * deviations for all parameters given in the correlation matrix file, but if  the correlation matrix contains more
 * parameters they will be ignored.
 */
void add_correlation_matrix( LALInferenceVariables *ini, LALInferenceVariables *priors, REAL8Array *corMat,
                             LALStringVector *parMat ){
  UINT4 i = 0, j = 0, k = 0;
  LALStringVector *newPars = NULL;
  gsl_matrix *corMatg = NULL;
  UINT4Vector *dims = XLALCreateUINT4Vector( 2 );
  UINT4 corsize = corMat->dimLength->data[0];

  /* loop through parameters and find ones that have Gaussian priors set - these should match with parameters in the
   * correlation coefficient matrix */
  for ( i = 0; i < parMat->length; i++ ){
    UINT4 incor = 0;
    LALInferenceVariableItem *checkPrior = ini->head;

    for( ; checkPrior ; checkPrior = checkPrior->next ){
      if( LALInferenceCheckGaussianPrior(priors, checkPrior->name) ){
        /* ignore parameter name case */
        if( !XLALStringCaseCompare(parMat->data[i], checkPrior->name) ){
          incor = 1;

          /* add parameter to new parameter string vector */
          newPars = XLALAppendString2Vector( newPars, parMat->data[i] );
          break;
        }
      }
    }

    /* if parameter in the corMat did not match one with a Gaussian defined prior, then remove it from the matrix */
    if ( incor == 0 ){
      /* remove the ith row and column from corMat, and the ith name from parMat */
      /* shift rows up */
      for ( j = i+1; j < corsize; j++ )
        for ( k = 0; k < corsize; k++ )
          corMat->data[(j-1)*corsize + k] = corMat->data[j*corsize + k];

      /* shift columns left */
      for ( k = i+1; k < corsize; k++ )
        for ( j = 0; j < corsize; j++ )
          corMat->data[j*corsize + k-1] = corMat->data[j*corsize + k];
    }
  }

  XLALDestroyUINT4Vector( dims );

  /* return new parameter string vector as old one */
  XLALDestroyStringVector( parMat );
  parMat = newPars;

  /* copy the corMat into a gsl_matrix */
  corMatg = gsl_matrix_alloc( parMat->length, parMat->length );
  for ( i = 0; i < parMat->length; i++ )
    for ( j = 0; j < parMat->length; j++ )
      gsl_matrix_set( corMatg, i, j, corMat->data[i*corsize + j] );

  /* re-loop over parameters removing Gaussian priors on those in the parMat and replacing with a correlation matrix */
  for ( i = 0; i < parMat->length; i++ ){
    LALInferenceVariableItem *checkPrior = ini->head;

    /* allocate global variable giving the list of the correlation matrix parameters */
    corlist = XLALAppendString2Vector( corlist, parMat->data[i] );

    for( ; checkPrior ; checkPrior = checkPrior->next ){
      if( LALInferenceCheckGaussianPrior(priors, checkPrior->name) ){
        if( !XLALStringCaseCompare(parMat->data[i], checkPrior->name) ){
          /* get the mean and standard deviation from the Gaussian prior */
          REAL8 mu, sigma;
          LALInferenceGetGaussianPrior( priors, checkPrior->name, &mu, &sigma );

          /* replace it with the correlation matrix as a gsl_matrix */
          LALInferenceAddCorrelatedPrior( priors, checkPrior->name, &corMatg, &mu, &sigma, &i );

          /* remove the Gaussian prior */
          LALInferenceRemoveGaussianPrior( priors, checkPrior->name );

          break;
        }
      }
    }
  }
}


/**
 * \brief Calculates the sum of the square of the data and model terms
 *
 * This function calculates the sum of the square of the data and model terms:
 * \f[
 * \sum_i^N \Re{d_i}^2 + \Im{d_i}^2, \sum_i^N \Re{h_i}^2, \sum_i^N \Im{h_i}^2, \sum_i^N \Re{d_i}\Re{h_i}, \sum_i^N Im{d_i}\Im{h_i}
 * \f]
 * for each stationary segment given in the \c chunkLength vector. These value are used in the likelihood calculation in
 * \c pulsar_log_likelihood and are precomputed here to speed that calculation up.
 *
 * \param runState [in] The analysis information structure
 */
void sum_data( LALInferenceRunState *runState ){
  LALInferenceIFOData *data = runState->data;
  LALInferenceIFOModel *ifomodel = runState->threads[0].model->ifo;

  UINT4 gaussianLike = 0, roq = 0, nonGR = 0;

  if ( LALInferenceGetProcParamVal( runState->commandLine, "--gaussian-like" ) ){ gaussianLike = 1; }
  if ( LALInferenceGetProcParamVal( runState->commandLine, "--roq" ) ){ roq = 1; }
  if ( LALInferenceGetProcParamVal( runState->commandLine, "--nonGR" ) ){ nonGR = 1; }

  while( data ){
    REAL8Vector *sumdat = NULL;

    /* sums of the antenna pattern functions with themeselves and the data.
     * These won't be needed if searching over phase parameters, but there's
     * no harm in computing them anyway. */

    /* also have "explicitly" whitened (i.e. divided by variance) versions of these for use
     * by the function to calculate the signal-to-noise ratio (if using the Gaussian
     * likelihood the standard versions of these vectors will also be whitened too) */
    REAL8Vector *sumP = NULL; /* sum of tensor antenna pattern function a(t)^2 */
    REAL8Vector *sumC = NULL; /* sum of tensor antenna pattern function b(t)^2 */
    REAL8Vector *sumPWhite = NULL; /* sum of antenna pattern function a(t)^2 */
    REAL8Vector *sumCWhite = NULL; /* sum of antenna pattern function b(t)^2 */

    /* non-GR values */
    REAL8Vector *sumX = NULL; /* sum of vector antenna pattern function a(t)^2 */
    REAL8Vector *sumY = NULL; /* sum of vector antenna pattern function b(t)^2 */
    REAL8Vector *sumXWhite = NULL; /* whitened version */
    REAL8Vector *sumYWhite = NULL; /* whitened version */

    REAL8Vector *sumB = NULL; /* sum of scalar antenna pattern function a(t)^2 */
    REAL8Vector *sumL = NULL; /* sum of scalar antenna pattern function b(t)^2 */
    REAL8Vector *sumBWhite = NULL; /* whitened version */
    REAL8Vector *sumLWhite = NULL; /* whitened version */

    COMPLEX16Vector *sumDataP = NULL; /* sum of the data * a(t) */
    COMPLEX16Vector *sumDataC = NULL; /* sum of the data * b(t) */
    COMPLEX16Vector *sumDataX = NULL; /* sum of the data * a(t) */
    COMPLEX16Vector *sumDataY = NULL; /* sum of the data * b(t) */
    COMPLEX16Vector *sumDataB = NULL; /* sum of the data * a(t) */
    COMPLEX16Vector *sumDataL = NULL; /* sum of the data * b(t) */

    /* cross terms */
    REAL8Vector *sumPC = NULL, *sumPX = NULL, *sumPY = NULL, *sumPB = NULL, *sumPL = NULL;
    REAL8Vector *sumCX = NULL, *sumCY = NULL, *sumCB = NULL, *sumCL = NULL;
    REAL8Vector *sumXY = NULL, *sumXB = NULL, *sumXL = NULL;
    REAL8Vector *sumYB = NULL, *sumYL = NULL;
    REAL8Vector *sumBL = NULL;

    /* whitened versions cross terms */
    REAL8Vector *sumPCWhite = NULL, *sumPXWhite = NULL, *sumPYWhite = NULL, *sumPBWhite = NULL, *sumPLWhite = NULL;
    REAL8Vector *sumCXWhite = NULL, *sumCYWhite = NULL, *sumCBWhite = NULL, *sumCLWhite = NULL;
    REAL8Vector *sumXYWhite = NULL, *sumXBWhite = NULL, *sumXLWhite = NULL;
    REAL8Vector *sumYBWhite = NULL, *sumYLWhite = NULL;
    REAL8Vector *sumBLWhite = NULL;

    /* get antenna patterns */
    REAL8Vector *arespT = *(REAL8Vector **)LALInferenceGetVariable( ifomodel->params, "a_response_tensor" );
    REAL8Vector *brespT = *(REAL8Vector **)LALInferenceGetVariable( ifomodel->params, "b_response_tensor" );
    REAL8Vector *arespV = NULL, *brespV = NULL, *arespS = NULL, *brespS = NULL;

    if ( nonGR ){
      arespV = *(REAL8Vector **)LALInferenceGetVariable( ifomodel->params, "a_response_vector" );
      brespV = *(REAL8Vector **)LALInferenceGetVariable( ifomodel->params, "b_response_vector" );
      arespS = *(REAL8Vector **)LALInferenceGetVariable( ifomodel->params, "a_response_scalar" );
      brespS = *(REAL8Vector **)LALInferenceGetVariable( ifomodel->params, "b_response_scalar" );
    }

    INT4 tsteps = *(INT4 *)LALInferenceGetVariable( ifomodel->params, "timeSteps" );

    INT4 chunkLength = 0, length = 0, i = 0, j = 0, count = 0;
    COMPLEX16 B;
    REAL8 aT = 0., bT = 0., aV = 0., bV = 0., aS = 0., bS = 0.;

    UINT4Vector *chunkLengths;

    REAL8Vector *sidDayFrac = *(REAL8Vector**)LALInferenceGetVariable( ifomodel->params, "siderealDay" );

    chunkLengths = *(UINT4Vector **)LALInferenceGetVariable( ifomodel->params, "chunkLength" );

    length = ifomodel->times->length + 1 - chunkLengths->data[chunkLengths->length - 1];

    sumdat = XLALCreateREAL8Vector( chunkLengths->length );

    if ( !roq ){
      /* allocate memory */
      sumP = XLALCreateREAL8Vector( chunkLengths->length );
      sumC = XLALCreateREAL8Vector( chunkLengths->length );
      sumPC = XLALCreateREAL8Vector( chunkLengths->length );
      sumDataP = XLALCreateCOMPLEX16Vector( chunkLengths->length );
      sumDataC = XLALCreateCOMPLEX16Vector( chunkLengths->length );

      sumPWhite = XLALCreateREAL8Vector( chunkLengths->length );
      sumCWhite = XLALCreateREAL8Vector( chunkLengths->length );
      sumPCWhite = XLALCreateREAL8Vector( chunkLengths->length );

      if ( nonGR ){
        sumX = XLALCreateREAL8Vector( chunkLengths->length );
        sumY = XLALCreateREAL8Vector( chunkLengths->length );
        sumB = XLALCreateREAL8Vector( chunkLengths->length );
        sumL = XLALCreateREAL8Vector( chunkLengths->length );

        sumPX = XLALCreateREAL8Vector( chunkLengths->length );
        sumPY = XLALCreateREAL8Vector( chunkLengths->length );
        sumPB = XLALCreateREAL8Vector( chunkLengths->length );
        sumPL = XLALCreateREAL8Vector( chunkLengths->length );
        sumCX = XLALCreateREAL8Vector( chunkLengths->length );
        sumCY = XLALCreateREAL8Vector( chunkLengths->length );
        sumCB = XLALCreateREAL8Vector( chunkLengths->length );
        sumCL = XLALCreateREAL8Vector( chunkLengths->length );
        sumXY = XLALCreateREAL8Vector( chunkLengths->length );
        sumXB = XLALCreateREAL8Vector( chunkLengths->length );
        sumXL = XLALCreateREAL8Vector( chunkLengths->length );
        sumYB = XLALCreateREAL8Vector( chunkLengths->length );
        sumYL = XLALCreateREAL8Vector( chunkLengths->length );
        sumBL = XLALCreateREAL8Vector( chunkLengths->length );

        sumXWhite = XLALCreateREAL8Vector( chunkLengths->length );
        sumYWhite = XLALCreateREAL8Vector( chunkLengths->length );
        sumBWhite = XLALCreateREAL8Vector( chunkLengths->length );
        sumLWhite = XLALCreateREAL8Vector( chunkLengths->length );

        sumPXWhite = XLALCreateREAL8Vector( chunkLengths->length );
        sumPYWhite = XLALCreateREAL8Vector( chunkLengths->length );
        sumPBWhite = XLALCreateREAL8Vector( chunkLengths->length );
        sumPLWhite = XLALCreateREAL8Vector( chunkLengths->length );
        sumCXWhite = XLALCreateREAL8Vector( chunkLengths->length );
        sumCYWhite = XLALCreateREAL8Vector( chunkLengths->length );
        sumCBWhite = XLALCreateREAL8Vector( chunkLengths->length );
        sumCLWhite = XLALCreateREAL8Vector( chunkLengths->length );
        sumXYWhite = XLALCreateREAL8Vector( chunkLengths->length );
        sumXBWhite = XLALCreateREAL8Vector( chunkLengths->length );
        sumXLWhite = XLALCreateREAL8Vector( chunkLengths->length );
        sumYBWhite = XLALCreateREAL8Vector( chunkLengths->length );
        sumYLWhite = XLALCreateREAL8Vector( chunkLengths->length );
        sumBLWhite = XLALCreateREAL8Vector( chunkLengths->length );

        sumDataX = XLALCreateCOMPLEX16Vector( chunkLengths->length );
        sumDataY = XLALCreateCOMPLEX16Vector( chunkLengths->length );
        sumDataB = XLALCreateCOMPLEX16Vector( chunkLengths->length );
        sumDataL = XLALCreateCOMPLEX16Vector( chunkLengths->length );
      }
    }

    REAL8 tsv = LAL_DAYSID_SI / tsteps, T = 0., timeMin = 0., timeMax = 0.;
    REAL8 logGaussianNorm = 0.; /* normalisation constant for Gaussian distribution */

    for( i = 0, count = 0 ; i < length ; i+= chunkLength, count++ ){
      chunkLength = chunkLengths->data[count];

      sumdat->data[count] = 0.;

      if ( !roq ){
        sumP->data[count] = 0.;
        sumC->data[count] = 0.;
        sumPC->data[count] = 0.;
        sumDataP->data[count] = 0.;
        sumDataC->data[count] = 0.;

        sumPWhite->data[count] = 0.;
        sumCWhite->data[count] = 0.;
        sumPCWhite->data[count] = 0.;

        if ( nonGR ){
          sumX->data[count] = 0.;
          sumY->data[count] = 0.;
          sumB->data[count] = 0.;
          sumL->data[count] = 0.;

          sumPX->data[count] = 0.;
          sumPY->data[count] = 0.;
          sumPB->data[count] = 0.;
          sumPL->data[count] = 0.;
          sumCX->data[count] = 0.;
          sumCY->data[count] = 0.;
          sumCB->data[count] = 0.;
          sumCL->data[count] = 0.;
          sumXY->data[count] = 0.;
          sumXB->data[count] = 0.;
          sumXL->data[count] = 0.;
          sumYB->data[count] = 0.;
          sumYL->data[count] = 0.;
          sumBL->data[count] = 0.;

          sumXWhite->data[count] = 0.;
          sumYWhite->data[count] = 0.;
          sumBWhite->data[count] = 0.;
          sumLWhite->data[count] = 0.;

          sumPXWhite->data[count] = 0.;
          sumPYWhite->data[count] = 0.;
          sumPBWhite->data[count] = 0.;
          sumPLWhite->data[count] = 0.;
          sumCXWhite->data[count] = 0.;
          sumCYWhite->data[count] = 0.;
          sumCBWhite->data[count] = 0.;
          sumCLWhite->data[count] = 0.;
          sumXYWhite->data[count] = 0.;
          sumXBWhite->data[count] = 0.;
          sumXLWhite->data[count] = 0.;
          sumYBWhite->data[count] = 0.;
          sumYLWhite->data[count] = 0.;
          sumBLWhite->data[count] = 0.;

          sumDataX->data[count] = 0.;
          sumDataY->data[count] = 0.;
          sumDataB->data[count] = 0.;
          sumDataL->data[count] = 0.;
        }
      }

      for( j = i ; j < i + chunkLength ; j++){
        REAL8 vari = 1., a0 = 0., a1 = 0., b0 = 0., b1 = 0., timeScaled = 0.;
        INT4 timebinMin = 0, timebinMax = 0;

        B = data->compTimeData->data->data[j];

        /* if using a Gaussian likelihood divide all these values by the variance */
        if ( gaussianLike ) {
          vari = data->varTimeData->data->data[j];
          logGaussianNorm -= log(LAL_TWOPI*vari);
        }

        /* sum up the data */
        sumdat->data[count] += (creal(B)*creal(B) + cimag(B)*cimag(B))/vari;

        if ( !roq ){
          /* set the time bin for the lookup table and interpolate between bins */
          T = sidDayFrac->data[j];
          timebinMin = (INT4)fmod( floor(T / tsv), tsteps );
          timeMin = timebinMin*tsv;
          timebinMax = (INT4)fmod( timebinMin + 1, tsteps );
          timeMax = timeMin + tsv;

          /* get values of vector for linear interpolation */
          a0 = arespT->data[timebinMin];
          a1 = arespT->data[timebinMax];
          b0 = brespT->data[timebinMin];
          b1 = brespT->data[timebinMax];

          /* rescale time for linear interpolation on a unit square */
          timeScaled = (T - timeMin)/(timeMax - timeMin);

          aT = a0 + (a1-a0)*timeScaled;
          bT = b0 + (b1-b0)*timeScaled;

          /* sum up the other terms */
          sumP->data[count] += aT*aT/vari;
          sumC->data[count] += bT*bT/vari;
          sumPC->data[count] += aT*bT/vari;
          sumDataP->data[count] += B*aT/vari;
          sumDataC->data[count] += B*bT/vari;

          /* non-GR values */
          if ( nonGR ){
            a0 = arespV->data[timebinMin];
            a1 = arespV->data[timebinMax];
            b0 = brespV->data[timebinMin];
            b1 = brespV->data[timebinMax];

            aV = a0 + (a1-a0)*timeScaled;
            bV = b0 + (b1-b0)*timeScaled;

            a0 = arespS->data[timebinMin];
            a1 = arespS->data[timebinMax];
            b0 = brespS->data[timebinMin];
            b1 = brespS->data[timebinMax];

            aS = a0 + (a1-a0)*timeScaled;
            bS = b0 + (b1-b0)*timeScaled;

            sumX->data[count] += aV*aV/vari;
            sumY->data[count] += bV*bV/vari;
            sumB->data[count] += aS*aS/vari;
            sumL->data[count] += bS*bS/vari;

            sumPX->data[count] += aT*aV/vari;
            sumPY->data[count] += aT*bV/vari;
            sumPB->data[count] += aT*aS/vari;
            sumPL->data[count] += aT*bS/vari;
            sumCX->data[count] += bT*aV/vari;
            sumCY->data[count] += bT*bV/vari;
            sumCB->data[count] += bT*aS/vari;
            sumCL->data[count] += bT*bS/vari;
            sumXY->data[count] += aV*bV/vari;
            sumXB->data[count] += aV*aS/vari;
            sumXL->data[count] += aV*bS/vari;
            sumYB->data[count] += bV*aS/vari;
            sumYL->data[count] += bV*bS/vari;
            sumBL->data[count] += aS*bS/vari;

            sumDataX->data[count] += B*aV/vari;
            sumDataY->data[count] += B*bV/vari;
            sumDataB->data[count] += B*aS/vari;
            sumDataL->data[count] += B*bS/vari;
          }

          /* get "explicitly whitened" versions, i.e. for use in signal-to-noise ratio
           * calculations even when not using a Gaussian likelihood */
          vari = data->varTimeData->data->data[j];
          sumPWhite->data[count] += aT*aT/vari;
          sumCWhite->data[count] += bT*bT/vari;
          sumPCWhite->data[count] += aT*bT/vari;

          /* non-GR values */
          if ( nonGR ){
            sumXWhite->data[count] += aV*aV/vari;
            sumYWhite->data[count] += bV*bV/vari;
            sumBWhite->data[count] += aS*aS/vari;
            sumLWhite->data[count] += bS*bS/vari;

            sumPXWhite->data[count] += aT*aV/vari;
            sumPYWhite->data[count] += aT*bV/vari;
            sumPBWhite->data[count] += aT*aS/vari;
            sumPLWhite->data[count] += aT*bS/vari;
            sumCXWhite->data[count] += bT*aV/vari;
            sumCYWhite->data[count] += bT*bV/vari;
            sumCBWhite->data[count] += bT*aS/vari;
            sumCLWhite->data[count] += bT*bS/vari;
            sumXYWhite->data[count] += aV*bV/vari;
            sumXBWhite->data[count] += aV*aS/vari;
            sumXLWhite->data[count] += aV*bS/vari;
            sumYBWhite->data[count] += bV*aS/vari;
            sumYLWhite->data[count] += bV*bS/vari;
            sumBLWhite->data[count] += aS*bS/vari;
          }
        }
      }
    }

    /* add all the summed data values - remove if already there, so that sum_data can be called more
     * than once if required e.g. if needed in the injection functions */

    check_and_add_fixed_variable( ifomodel->params, "sumData", &sumdat, LALINFERENCE_REAL8Vector_t );

    if ( !roq ){
      check_and_add_fixed_variable( ifomodel->params, "sumP", &sumP, LALINFERENCE_REAL8Vector_t );
      check_and_add_fixed_variable( ifomodel->params, "sumC", &sumC, LALINFERENCE_REAL8Vector_t );
      check_and_add_fixed_variable( ifomodel->params, "sumPC", &sumPC, LALINFERENCE_REAL8Vector_t );
      check_and_add_fixed_variable( ifomodel->params, "sumDataP", &sumDataP, LALINFERENCE_COMPLEX16Vector_t );
      check_and_add_fixed_variable( ifomodel->params, "sumDataC", &sumDataC, LALINFERENCE_COMPLEX16Vector_t );
      check_and_add_fixed_variable( ifomodel->params, "sumPWhite", &sumPWhite, LALINFERENCE_REAL8Vector_t );
      check_and_add_fixed_variable( ifomodel->params, "sumCWhite", &sumCWhite, LALINFERENCE_REAL8Vector_t );
      check_and_add_fixed_variable( ifomodel->params, "sumPCWhite", &sumPCWhite, LALINFERENCE_REAL8Vector_t );

      if ( nonGR ){
        check_and_add_fixed_variable( ifomodel->params, "sumX", &sumX, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumY", &sumY, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumB", &sumB, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumL", &sumL, LALINFERENCE_REAL8Vector_t );

        check_and_add_fixed_variable( ifomodel->params, "sumPX", &sumPX, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumPY", &sumPY, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumPB", &sumPB, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumPL", &sumPL, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumCX", &sumCX, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumCY", &sumCY, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumCB", &sumCB, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumCL", &sumCL, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumXY", &sumXY, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumXB", &sumXB, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumXL", &sumXL, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumYB", &sumYB, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumYL", &sumYL, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumBL", &sumBL, LALINFERENCE_REAL8Vector_t );

        check_and_add_fixed_variable( ifomodel->params, "sumDataX", &sumDataX, LALINFERENCE_COMPLEX16Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumDataY", &sumDataY, LALINFERENCE_COMPLEX16Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumDataB", &sumDataB, LALINFERENCE_COMPLEX16Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumDataL", &sumDataL, LALINFERENCE_COMPLEX16Vector_t );

        check_and_add_fixed_variable( ifomodel->params, "sumXWhite", &sumXWhite, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumYWhite", &sumYWhite, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumBWhite", &sumBWhite, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumLWhite", &sumLWhite, LALINFERENCE_REAL8Vector_t );

        check_and_add_fixed_variable( ifomodel->params, "sumPXWhite", &sumPXWhite, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumPYWhite", &sumPYWhite, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumPBWhite", &sumPBWhite, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumPLWhite", &sumPLWhite, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumCXWhite", &sumCXWhite, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumCYWhite", &sumCYWhite, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumCBWhite", &sumCBWhite, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumCLWhite", &sumCLWhite, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumXYWhite", &sumXYWhite, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumXBWhite", &sumXBWhite, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumXLWhite", &sumXLWhite, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumYBWhite", &sumYBWhite, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumYLWhite", &sumYLWhite, LALINFERENCE_REAL8Vector_t );
        check_and_add_fixed_variable( ifomodel->params, "sumBLWhite", &sumBLWhite, LALINFERENCE_REAL8Vector_t );
      }
    }
    else{ /* add parameter defining the usage of RQO here (as this is after any injection generation, which would fail if this was set */
      LALInferenceAddVariable( ifomodel->params, "roq", &roq, LALINFERENCE_UINT4_t, LALINFERENCE_PARAM_FIXED );
    }

    LALInferenceAddVariable( ifomodel->params, "logGaussianNorm", &logGaussianNorm, LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_FIXED );

    data = data->next;
    ifomodel = ifomodel->next;
  }

  return;
}

/**
 * \brief Parse data from a prior file containing Gaussian Mixture Model mean values
 *
 * If a Gaussian Mixture Model prior has been specified then this function will parse
 * the means for each parameter for each mode given. E.g. if the GMM provides multivariate
 * Gaussian modes for two parameters, x and y, then the means would be specified in a string
 * of the form "[[mux1,muy1],[mux2,muy2],....]". The string should have no whitespace between
 * values, and mean values for a given mode must be separated by a comma.
 *
 * These values are returned in an vector of REAL8Vectors. If an error occurred then NULL will be returned.
 *
 * \param meanstr [in] A string containing the mean values
 * \param npars [in] The number of parameters
 * \param nmodes [in] The number of modes
 */
REAL8Vector** parse_gmm_means(CHAR *meanstr, UINT4 npars, UINT4 nmodes){
  UINT4 modecount = 0;

  /* parse mean string */
  CHAR *startloc = strchr(meanstr, '['); /* find location of first '[' */
  if ( !startloc ){ return NULL; }

  CHAR strpart[16384]; /* string to hold elements */

  /* allocate memory for returned value */
  REAL8Vector **meanmat;
  meanmat =  XLALCalloc(nmodes, sizeof(REAL8Vector *));

  while( 1 ){
    CHAR *closeloc = get_bracketed_string(strpart, startloc+1, '[', ']');
    if ( !strpart[0] ){ break; } /* break when no more bracketed items are found */

    /* get mean values (separated by commas) */
    TokenList *meantoc = NULL;
    XLALCreateTokenList( &meantoc, strpart, "," );
    if ( meantoc->nTokens != npars ){
      XLAL_PRINT_WARNING("Warning... number of means parameters specified for GMM is not consistent with number of parameters.\n");
      for ( INT4 k=modecount-1; k > -1; k-- ){ XLALDestroyREAL8Vector(meanmat[k]); }
      XLALFree(meanmat);
      return NULL;
    }
    meanmat[modecount] = XLALCreateREAL8Vector( npars );
    for( UINT4 j = 0; j < meantoc->nTokens; j++ ){ meanmat[modecount]->data[j] = atof(meantoc->tokens[j]); }

    startloc = closeloc;
    modecount++;
    XLALDestroyTokenList( meantoc );
  }

  if ( modecount != nmodes ){
    XLAL_PRINT_WARNING("Warning... number of means values specified for GMM is not consistent with number of modes.\n");
    for ( INT4 k=modecount-1; k > -1; k-- ){ XLALDestroyREAL8Vector(meanmat[k]); }
    XLALFree(meanmat);
    meanmat = NULL;
  }

  return meanmat;
}

/**
 * \brief Parse data from a prior file containing Gaussian Mixture Model covariance matrix values
 *
 * If a Gaussian Mixture Model prior has been specified then this function will parse
 * the covariance matrices for each mode given. E.g. if the GMM provides multivariate
 * Gaussian modes for two parameters, x and y, then the covariances for each mode would
 * be specified in a string of the form "[[[covxx1,covxy1][covyx1,covyy1]],[[covxx2,covxy2][covyx2,covyy2]],...]".
 * The string should have no whitespace between values, and covariance values for a given mode must be
 * separated by a comma.
 *
 * These values are returned in an array of GSL matrices. If an error occurred then NULL will be returned.
 *
 * \param covstr [in] A string containing the covariance matrix values
 * \param npars [in] The number of parameters
 * \param nmodes [in] The number of modes
 */
gsl_matrix** parse_gmm_covs(CHAR *covstr, UINT4 npars, UINT4 nmodes){
  UINT4 modecount = 0;

  /* parse covariance string */
  CHAR *startloc = strchr(covstr, '['); /* find location of first '[' */
  if ( !startloc ){ return NULL; }

  CHAR strpart[16384]; /* string to hold elements */

  /* allocate memory for returned value */
  gsl_matrix **covmat;
  covmat = XLALCalloc(nmodes, sizeof(gsl_matrix *));

  while( 1 ){
    CHAR *openloc = strstr(startloc+1, "[["); /* find next "[[" */
    /* break if there are no more opening brackets */
    if ( !openloc ){ break; }

    CHAR *closeloc = strstr(openloc+1, "]]"); /* find next "]]" */
    if ( !closeloc ){ break; }

    strncpy(strpart, openloc+1, (closeloc-openloc)); /* copy string */
    strpart[(closeloc-openloc)] = '\0'; /* add null terminating character */

    CHAR *newstartloc = strpart;

    UINT4 parcount = 0;

    covmat[modecount] = gsl_matrix_alloc(npars, npars);

    while ( 1 ){
      CHAR newstrpart[8192];
      CHAR *newcloseloc = get_bracketed_string(newstrpart, newstartloc, '[', ']');

      if ( !newstrpart[0] ){ break; } /* read all of covariance matrix for this mode */

      if ( parcount > npars ){
        XLAL_PRINT_WARNING("Warning... number of covariance parameters specified for GMM is not consistent with number of parameters.\n");
        for ( INT4 k=modecount; k > -1; k-- ){ gsl_matrix_free(covmat[k]); }
        XLALFree(covmat);
        return NULL;
      }

      newstartloc = newcloseloc;

      /* get covariance values (separated by commas) */
      TokenList *covtoc = NULL;
      XLALCreateTokenList( &covtoc, newstrpart, "," );
      if ( covtoc->nTokens != npars ){
        XLAL_PRINT_WARNING("Warning... number of means parameters specified for GMM is not consistent with number of parameters.\n");
        for ( INT4 k=modecount; k > -1; k-- ){ gsl_matrix_free(covmat[k]); }
        XLALFree(covmat);
        return NULL;
      }

      for( UINT4 j = 0; j < covtoc->nTokens; j++ ){ gsl_matrix_set(covmat[modecount], parcount, j, atof(covtoc->tokens[j])); }
      XLALDestroyTokenList( covtoc );

      parcount++;
    }
    startloc = closeloc;
    modecount++;
  }

  if ( modecount != nmodes ){
    XLAL_PRINT_WARNING("Warning... number of means values specified for GMM is not consistent with number of modes.\n");
    for ( INT4 k=modecount; k > -1; k-- ){ gsl_matrix_free(covmat[k]); }
    XLALFree(covmat);
    covmat = NULL;
  }

  return covmat;
}


CHAR* get_bracketed_string(CHAR *dest, const CHAR *bstr, int openbracket, int closebracket){
  /* get positions of opening and closing brackets */
  CHAR *openpar = strchr(bstr, openbracket);
  CHAR *closepar = strchr(bstr+1, closebracket);

  if ( !openpar || !closepar ){
    dest[0] = 0;
    return NULL;
  }

  strncpy(dest, openpar+1, (closepar-openpar)-1);
  dest[(closepar-openpar)-1] = '\0';

  /* return pointer the the location after the closing brackets */
  return closepar+1;
}


void initialise_threads(LALInferenceRunState *state, INT4 nthreads){
  INT4 i,randomseed;
  LALInferenceThreadState *thread;
  for (i=0; i<nthreads; i++){
    thread=&state->threads[i];
    //LALInferenceCopyVariables(thread->model->params, thread->currentParams);
    LALInferenceCopyVariables(state->priorArgs, thread->priorArgs);
    LALInferenceCopyVariables(state->proposalArgs, thread->proposalArgs);
    thread->GSLrandom = gsl_rng_alloc(gsl_rng_mt19937);
    randomseed = gsl_rng_get(state->GSLrandom);
    gsl_rng_set(thread->GSLrandom, randomseed);

    /* Explicitly zero out DE, in case it's not used later */
    thread->differentialPoints = NULL;
    thread->differentialPointsLength = 0;
    thread->differentialPointsSize = 0;
  }
}

/*
 * Copyright (C) 2005 Reinhard Prix
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
 *  Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *  MA  02111-1307  USA
 */

/**
 * \author Reinhard Prix
 * \date 2005
 * \ingroup pulsarCoherent
 * \file
 * \brief Header-file defining the API for the F-statistic functions.
 *
 * This code is (partly) a descendant of an earlier implementation found in
 * LALDemod.[ch] by Jolien Creighton, Maria Alessandra Papa, Reinhard Prix, Steve Berukoff, Xavier Siemens, Bruce Allen
 * ComputSky.[ch] by Jolien Creighton, Reinhard Prix, Steve Berukoff
 * LALComputeAM.[ch] by Jolien Creighton, Maria Alessandra Papa, Reinhard Prix, Steve Berukoff, Xavier Siemens
 *
 * $Id$
 *
 */

#ifndef _COMPUTEFSTAT_H  /* Double-include protection. */
#define _COMPUTEFSTAT_H

/* C++ protection. */
#ifdef  __cplusplus
extern "C" {
#endif

#include <lal/LALRCSID.h>
NRCSID( COMPUTEFSTATH, "$Id$" );

/*---------- exported INCLUDES ----------*/
#include <lal/LALComputeAM.h>
#include <lal/ComplexAM.h>

#include <lal/PulsarDataTypes.h>
#include <lal/DetectorStates.h>
#include <gsl/gsl_vector.h>

/*---------- exported DEFINES ----------*/

/** \name Error codes */
/*@{*/
#define COMPUTEFSTATC_ENULL 		1
#define COMPUTEFSTATC_ENONULL 		2
#define COMPUTEFSTATC_EINPUT   		3
#define COMPUTEFSTATC_EMEM   		4
#define COMPUTEFSTATC_EXLAL		5
#define COMPUTEFSTATC_EIEEE		6

#define COMPUTEFSTATC_MSGENULL 		"Arguments contained an unexpected null pointer"
#define COMPUTEFSTATC_MSGENONULL 	"Output pointer is non-NULL"
#define COMPUTEFSTATC_MSGEINPUT   	"Invalid input"
#define COMPUTEFSTATC_MSGEMEM   	"Out of memory. Bad."
#define COMPUTEFSTATC_MSGEXLAL		"XLAL function call failed"
#define COMPUTEFSTATC_MSGEIEEE		"Floating point failure"
/*@}*/

/*---------- exported types ----------*/

/** Simple container for two REAL8-vectors, namely the SSB-timings DeltaT_alpha  and Tdot_alpha,
 * with one entry per SFT-timestamp. These are required input for XLALNewDemod().
 * We also store the SSB reference-time tau0.
 */
typedef struct {
  LIGOTimeGPS refTime;
  REAL8Vector *DeltaT;		/**< Time-difference of SFT-alpha - tau0 in SSB-frame */
  REAL8Vector *Tdot;		/**< dT/dt : time-derivative of SSB-time wrt local time for SFT-alpha */
} SSBtimes;

/** Multi-IFO container for SSB timings */
typedef struct {
  UINT4 length;		/**< number of IFOs */
  SSBtimes **data;	/**< array of SSBtimes (pointers) */
} MultiSSBtimes;

/** one F-statistic 'atom', ie the elementary per-SFT quantities required to compute F, for one detector X */
typedef struct {
  UINT4 timestamp;		/**< SFT GPS timestamp t_i in seconds */
  REAL8 a2_alpha;		/**< antenna-pattern factor a^2(X,t_i) */
  REAL8 b2_alpha;		/**< antenna-pattern factor b^2(X,t_i) */
  REAL8 ab_alpha;		/**< antenna-pattern factor a*b(X,t_i) */
  COMPLEX8 Fa_alpha;		/**< Fa^X(t_i) */
  COMPLEX8 Fb_alpha;		/**< Fb^X(t_i) */
} FstatAtom;

/** vector of F-statistic 'atoms', ie all per-SFT quantities required to compute F, for one detector X */
typedef struct {
  UINT4 length;			/**< number of per-SFT 'atoms' */
  FstatAtom *data;		/** FstatAtoms array of given length */
  UINT4 TAtom;			/**< time-baseline of F-stat atoms (typically Tsft) */
} FstatAtomVector;

/** multi-detector version of FstatAtoms type */
typedef struct {
  UINT4 length;			/**< number of detectors */
  FstatAtomVector **data;	/**< array of FstatAtom (pointers), one for each detector X */
} MultiFstatAtomVector;


/** Type containing F-statistic proper plus the two complex amplitudes Fa and Fb (for ML-estimators) */
typedef struct {
  REAL8 F;				/**< F-statistic value */
  COMPLEX16 Fa;				/**< complex amplitude Fa */
  COMPLEX16 Fb;				/**< complex amplitude Fb */
  MultiFstatAtomVector *multiFstatAtoms;/**< per-IFO, per-SFT arrays of F-stat 'atoms', ie quantities required to compute F-stat */
} Fcomponents;

/** The precision in calculating the barycentric transformation */
typedef enum {
  SSBPREC_NEWTONIAN,		/**< simple Newtonian: \f$\tau = t + \vec{r}\cdot\vec{n}/c\f$ */
  SSBPREC_RELATIVISTIC,		/**< detailed relativistic: \f$\tau=\tau(t; \vec{n}, \vec{r})\f$ */
  SSBPREC_LAST			/**< end marker */
} SSBprecision;

/** [opaque] type holding a ComputeFBuffer for use in the resampling F-stat codes */
typedef struct tag_ComputeFBuffer_RS ComputeFBuffer_RS;

/** Extra parameters controlling the actual computation of F */
typedef struct {
  UINT4 Dterms;		/**< how many terms to keep in the Dirichlet kernel (~16 is usually fine) */
  REAL8 upsampling;	/**< frequency-upsampling applied to SFTs ==> dFreq != 1/Tsft ... */
  SSBprecision SSBprec; /**< whether to use full relativist SSB-timing, or just simple Newtonian */
  BOOLEAN useRAA;        /**< whether to use the frequency- and sky-position-dependent rigid adiabatic response tensor and not just the long-wavelength approximation */
  BOOLEAN bufferedRAA;	/**< approximate RAA by assuming constant response over (small) frequency band */
  ComputeFBuffer_RS *buffer; /**< buffer for storing pre-resampled timeseries (used for resampling implementation) */
  EphemerisData *edat;   /**< ephemeris data for re-computing multidetector states */
  BOOLEAN returnAtoms;	/**< whether or not to return the 'FstatAtoms' used to compute the F-statistic */
} ComputeFParams;


/** Struct holding buffered ComputeFStat()-internal quantities to avoid unnecessarily
 * recomputing things that depend ONLY on the skyposition and detector-state series (but not on the spins).
 * For the first call of ComputeFStat() the pointer-entries should all be NULL.
 */
typedef struct {
  const MultiDetectorStateSeries *multiDetStates;/**< buffer for each detStates (store pointer) and skypos */
  REAL8 Alpha, Delta;				/**< skyposition of candidate */
  MultiSSBtimes *multiSSB;
  MultiSSBtimes *multiBinary;
  MultiAMCoeffs *multiAMcoef;
  MultiCmplxAMCoeffs *multiCmplxAMcoef;
} ComputeFBuffer;


/*---------- exported Global variables ----------*/
/* empty init-structs for the types defined in here */
extern const SSBtimes empty_SSBtimes;
extern const MultiSSBtimes empty_MultiSSBtimes;
extern const Fcomponents empty_Fcomponents;
extern const ComputeFParams empty_ComputeFParams;
extern const ComputeFBuffer empty_ComputeFBuffer;

/*---------- exported prototypes [API] ----------*/
int
XLALComputeFaFb ( Fcomponents *FaFb,
		  const SFTVector *sfts,
		  constPulsarSpins fkdot,
		  const SSBtimes *tSSB,
		  const AMCoeffs *amcoe,
		  const ComputeFParams *params);

int
XLALComputeFaFbXavie ( Fcomponents *FaFb,
		       const SFTVector *sfts,
		       constPulsarSpins fkdot,
		       const SSBtimes *tSSB,
		       const AMCoeffs *amcoe,
		       const ComputeFParams *params);
int
XLALComputeFaFbCmplx ( Fcomponents *FaFb,
		       const SFTVector *sfts,
		       constPulsarSpins fkdot,
		       const SSBtimes *tSSB,
		       const CmplxAMCoeffs *amcoe,
		       const ComputeFParams *params);

void
LALGetBinarytimes (LALStatus *,
		   SSBtimes *tBinary,
		   const SSBtimes *tSSB,
		   const DetectorStateSeries *DetectorStates,
		   const BinaryOrbitParams *binaryparams,
		   LIGOTimeGPS refTime);

void
LALGetMultiBinarytimes (LALStatus *status,
			MultiSSBtimes **multiBinary,
			const MultiSSBtimes *multiSSB,
			const MultiDetectorStateSeries *multiDetStates,
			const BinaryOrbitParams *binaryparams,
			LIGOTimeGPS refTime);

void
LALGetSSBtimes (LALStatus *,
		SSBtimes *tSSB,
		const DetectorStateSeries *DetectorStates,
		SkyPosition pos,
		LIGOTimeGPS refTime,
		SSBprecision precision);

void
LALGetMultiSSBtimes (LALStatus *,
		     MultiSSBtimes **multiSSB,
		     const MultiDetectorStateSeries *multiDetStates,
		     SkyPosition pos,
		     LIGOTimeGPS refTime,
		     SSBprecision precision );

void ComputeFStat ( LALStatus *, Fcomponents *Fstat,
		    const PulsarDopplerParams *doppler,
		    const MultiSFTVector *multiSFTs,
		    const MultiNoiseWeights *multiWeights,
		    const MultiDetectorStateSeries *multiDetStates,
		    const ComputeFParams *params,
		    ComputeFBuffer *cfBuffer );

void ComputeFStatFreqBand ( LALStatus *status,
			    REAL4FrequencySeries *FstatVector,
			    const PulsarDopplerParams *doppler,
			    const MultiSFTVector *multiSFTs,
			    const MultiNoiseWeights *multiWeights,
			    const MultiDetectorStateSeries *multiDetStates,
			    const ComputeFParams *params);

void
LALEstimatePulsarAmplitudeParams (LALStatus * status,
				  PulsarCandidate *pulsarParams,
				  const Fcomponents *Fstat,
				  const LIGOTimeGPS *FstatRefTime,
				  const CmplxAntennaPatternMatrix *Mmunu
				  );

FstatAtomVector * XLALCreateFstatAtomVector ( UINT4 num );

int XLALAmplitudeParams2Vect ( PulsarAmplitudeVect A_Mu, const PulsarAmplitudeParams Amp );
int XLALAmplitudeVect2Params ( PulsarAmplitudeParams *Amp, constPulsarAmplitudeVect A_Mu );

/* destructors */
void XLALDestroyMultiSSBtimes ( MultiSSBtimes *multiSSB );
void XLALEmptyComputeFBuffer ( ComputeFBuffer *cfb );

void XLALDestroyFstatAtomVector ( FstatAtomVector *atoms );
void XLALDestroyMultiFstatAtomVector ( MultiFstatAtomVector *multiAtoms );

/* helpers */
int sin_cos_LUT (REAL4 *sinx, REAL4 *cosx, REAL8 x);
int sin_cos_2PI_LUT (REAL4 *sin2pix, REAL4 *cos2pix, REAL8 x);


#ifdef  __cplusplus
}
#endif
/* C++ protection. */

#endif  /* Double-include protection. */

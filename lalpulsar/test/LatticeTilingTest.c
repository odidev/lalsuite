//
// Copyright (C) 2014, 2015 Karl Wette
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with with program; see the file COPYING. If not, write to the
// Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
// MA 02111-1307 USA
//

// Tests of the lattice-based template generation code in LatticeTiling.[ch].

#include <stdio.h>

#include <lal/LatticeTiling.h>
#include <lal/LALStdlib.h>
#include <lal/Factorial.h>
#include <lal/DopplerFullScan.h>
#include <lal/SuperskyMetrics.h>
#include <lal/LALInitBarycenter.h>

#include "../src/GSLHelpers.h"

#define MISM_HIST_BINS 20

// The reference mismatch histograms were generated in Octave
// using the LatticeMismatchHist() function available in OctApps.

const double Z1_mism_hist[MISM_HIST_BINS] = {
  4.531107, 1.870257, 1.430467, 1.202537, 1.057047, 0.953084, 0.875050, 0.813050, 0.762368, 0.719968,
  0.683877, 0.652659, 0.625394, 0.601300, 0.579724, 0.560515, 0.542944, 0.527142, 0.512487, 0.499022
};

const double Z2_mism_hist[MISM_HIST_BINS] = {
  1.570963, 1.571131, 1.571074, 1.571102, 1.570808, 1.570789, 1.570617, 1.570716, 1.570671, 1.570867,
  1.157132, 0.835785, 0.645424, 0.503305, 0.389690, 0.295014, 0.214022, 0.143584, 0.081427, 0.025878
};

const double Z3_mism_hist[MISM_HIST_BINS] = {
  0.608404, 1.112392, 1.440652, 1.705502, 1.934785, 2.139464, 2.296868, 2.071379, 1.748278, 1.443955,
  1.155064, 0.879719, 0.616210, 0.375368, 0.223752, 0.131196, 0.071216, 0.033130, 0.011178, 0.001489
};

#define A1s_mism_hist Z1_mism_hist

const double A2s_mism_hist[MISM_HIST_BINS] = {
  1.210152, 1.210142, 1.209837, 1.209697, 1.209368, 1.209214, 1.209399, 1.209170, 1.208805, 1.208681,
  1.208631, 1.208914, 1.208775, 1.209021, 1.208797, 0.816672, 0.505394, 0.315665, 0.170942, 0.052727
};

const double A3s_mism_hist[MISM_HIST_BINS] = {
  0.327328, 0.598545, 0.774909, 0.917710, 1.040699, 1.150991, 1.250963, 1.344026, 1.431020, 1.512883,
  1.590473, 1.664510, 1.595423, 1.391209, 1.194340, 1.004085, 0.729054, 0.371869, 0.098727, 0.011236
};

static int BasicTest(
  size_t n,
  const TilingLattice lattice,
  const UINT8 total_ref_0,
  const UINT8 total_ref_1,
  const UINT8 total_ref_2,
  const UINT8 total_ref_3
  )
{

  const UINT8 total_ref[4] = {total_ref_0, total_ref_1, total_ref_2, total_ref_3};

  // 'n == 0' denotes a single-point template bank
  const bool single_point = (n == 0);
  if (single_point) {
    n = 4;
  }

  // Create lattice tiling
  printf("Number of dimensions: %zu%s\n", n, single_point ? " (single point)" : "");
  LatticeTiling *tiling = XLALCreateLatticeTiling(n);
  XLAL_CHECK(tiling != NULL, XLAL_EFUNC);

  // Add bounds
  for (size_t i = 0; i < n; ++i) {
    XLAL_CHECK(XLALSetLatticeTilingConstantBound(tiling, i, 0.0, single_point ? 0.0 : pow(100.0, 1.0/n)) == XLAL_SUCCESS, XLAL_EFUNC);
  }

  // Set metric to the Lehmer matrix
  const double max_mismatch = 0.3;
  {
    gsl_matrix *GAMAT(metric, n, n);
    for (size_t i = 0; i < n; ++i) {
      for (size_t j = 0; j < n; ++j) {
        const double ii = i+1, jj = j+1;
        gsl_matrix_set(metric, i, j, jj >= ii ? ii/jj : jj/ii);
      }
    }
    printf("  Lattice type: %u\n", lattice);
    XLAL_CHECK(XLALSetTilingLatticeAndMetric(tiling, lattice, metric, max_mismatch) == XLAL_SUCCESS, XLAL_EFUNC);
    GFMAT(metric);
  }

  // Create lattice tiling locator
  LatticeTilingLocator *loc = XLALCreateLatticeTilingLocator(tiling);
  XLAL_CHECK(loc != NULL, XLAL_EFUNC);
  if (lalDebugLevel & LALINFOBIT) {
    printf("  Index trie:\n");
    XLAL_CHECK(XLALPrintLatticeTilingIndexTrie(loc, stdout) == XLAL_SUCCESS, XLAL_EFUNC);
  }

  for (size_t i = 0; i < n; ++i) {

    // Create lattice tiling iterator and locator over 'i+1' dimensions
    LatticeTilingIterator *itr = XLALCreateLatticeTilingIterator(tiling, i+1);
    XLAL_CHECK(itr != NULL, XLAL_EFUNC);

    // Count number of points
    const UINT8 total = XLALTotalLatticeTilingPoints(itr);
    printf("Number of lattice points in %zu dimensions: %" LAL_UINT8_FORMAT "\n", i+1, total);
    XLAL_CHECK(total == total_ref[i], XLAL_EFUNC, "ERROR: total = %" LAL_UINT8_FORMAT " != %" LAL_UINT8_FORMAT " = total_ref", total, total_ref[i]);
    for (UINT8 k = 0; XLALNextLatticeTilingPoint(itr, NULL) > 0; ++k) {
      const UINT8 itr_index = XLALCurrentLatticeTilingIndex(itr);
      XLAL_CHECK(k == itr_index, XLAL_EFUNC, "ERROR: k = %" LAL_UINT8_FORMAT " != %" LAL_UINT8_FORMAT " = itr_index", k, itr_index);
    }
    XLAL_CHECK(XLALResetLatticeTilingIterator(itr) == XLAL_SUCCESS, XLAL_EFUNC);

    // Check tiling statistics
    printf("  Check tiling statistics ...");
    for (size_t j = 0; j < n; ++j) {
      const LatticeTilingStats *stats = XLALLatticeTilingStatistics(tiling, j);
      XLAL_CHECK(stats != NULL, XLAL_EFUNC);
      XLAL_CHECK(stats->total_points == total_ref[j], XLAL_EFAILED, "\n  "
                 "ERROR: total = %" LAL_UINT8_FORMAT " != %" LAL_UINT8_FORMAT " = total_ref", stats->total_points, total_ref[j]);
      XLAL_CHECK(stats->min_points_pass <= stats->avg_points_pass, XLAL_EFAILED, "\n  "
                 "ERROR: min_points_pass = %" LAL_INT4_FORMAT " > %g = avg_points_pass", stats->min_points_pass, stats->avg_points_pass);
      XLAL_CHECK(stats->max_points_pass >= stats->avg_points_pass, XLAL_EFAILED, "\n  "
                 "ERROR: max_points_pass = %" LAL_INT4_FORMAT " < %g = avg_points_pass", stats->max_points_pass, stats->avg_points_pass);
    }
    printf(" done\n");

    // Get all points
    gsl_matrix *GAMAT(points, n, total);
    XLAL_CHECK(XLALNextLatticeTilingPoints(itr, &points) == (int)total, XLAL_EFUNC);
    XLAL_CHECK(XLALNextLatticeTilingPoint(itr, NULL) == 0, XLAL_EFUNC);

    // Get nearest points to each template, check for consistency
    printf("  Testing XLALNearestLatticeTiling{Point|Pass}() ...");
    gsl_vector *GAVEC(nearest, n);
    UINT8Vector *nearest_seq_idxs = XLALCreateUINT8Vector(n);
    XLAL_CHECK(nearest_seq_idxs != NULL, XLAL_ENOMEM);
    UINT4Vector *nearest_pass_idxs = XLALCreateUINT4Vector(n);
    XLAL_CHECK(nearest_pass_idxs != NULL, XLAL_ENOMEM);
    UINT4Vector *nearest_pass_lens = XLALCreateUINT4Vector(n);
    XLAL_CHECK(nearest_pass_lens != NULL, XLAL_ENOMEM);
    for (UINT8 k = 0; k < total; ++k) {
      gsl_vector_const_view point_view = gsl_matrix_const_column(points, k);
      const gsl_vector *point = &point_view.vector;
      XLAL_CHECK(XLALNearestLatticeTilingPoint(loc, point, nearest, nearest_seq_idxs, nearest_pass_idxs, nearest_pass_lens) == XLAL_SUCCESS, XLAL_EFUNC);
      gsl_vector_sub(nearest, point);
      double err = gsl_blas_dasum(nearest) / n;
      XLAL_CHECK(err < 1e-6, XLAL_EFAILED, "\n  "
                 "ERROR: err = %e < 1e-6", err);
      XLAL_CHECK(nearest_seq_idxs->data[i] == k, XLAL_EFAILED, "\n  "
                 "ERROR: nearest_seq_idxs[%zu] = %" LAL_UINT8_FORMAT " != %" LAL_UINT8_FORMAT "\n", i, nearest_seq_idxs->data[i], k);
      for (size_t j = 0; j < n; ++j) {
        XLAL_CHECK(nearest_pass_idxs->data[j] < nearest_pass_lens->data[j], XLAL_EFAILED, "\n  "
                   "ERROR: nearest_pass_idxs[%zu] = %" LAL_UINT4_FORMAT " >= %" LAL_UINT4_FORMAT "\n", j, nearest_pass_idxs->data[i], nearest_pass_lens->data[j]);
      }
      if (0 < i) {
        UINT8 nearest_seq_idx = 0;
        XLAL_CHECK(XLALNearestLatticeTilingPass(loc, point, i, NULL, &nearest_seq_idx, NULL, NULL) == XLAL_SUCCESS, XLAL_EFUNC);
        XLAL_CHECK(nearest_seq_idx == nearest_seq_idxs->data[i-1], XLAL_EFAILED, "\n  "
                   "ERROR: nearest_seq_idx = %" LAL_UINT8_FORMAT " != %" LAL_UINT8_FORMAT "\n", nearest_seq_idx, nearest_seq_idxs->data[i-1]);
      }
      if (i+1 < n) {
        UINT8 nearest_seq_idx = 0;
        XLAL_CHECK(XLALNearestLatticeTilingPass(loc, point, i+1, NULL, &nearest_seq_idx, NULL, NULL) == XLAL_SUCCESS, XLAL_EFUNC);
        XLAL_CHECK(nearest_seq_idx == nearest_seq_idxs->data[i], XLAL_EFAILED, "\n  "
                   "ERROR: nearest_seq_idx = %" LAL_UINT8_FORMAT " != %" LAL_UINT8_FORMAT "\n", nearest_seq_idx, nearest_seq_idxs->data[i]);
      }
    }
    printf(" done\n");

    // Cleanup
    XLALDestroyLatticeTilingIterator(itr);
    GFMAT(points);
    GFVEC(nearest);
    XLALDestroyUINT8Vector(nearest_seq_idxs);
    XLALDestroyUINT4Vector(nearest_pass_idxs);
    XLALDestroyUINT4Vector(nearest_pass_lens);

  }

  for (size_t i = 0; i < n; ++i) {

    // Create alternating lattice tiling iterator over 'i+1' dimensions
    LatticeTilingIterator *itr_alt = XLALCreateLatticeTilingIterator(tiling, i+1);
    XLAL_CHECK(itr_alt != NULL, XLAL_EFUNC);
    XLAL_CHECK(XLALSetLatticeTilingAlternatingIterator(itr_alt, true) == XLAL_SUCCESS, XLAL_EFUNC);

    // Count number of points, check for consistency with non-alternating count
    UINT8 total = 0;
    while (XLALNextLatticeTilingPoint(itr_alt, NULL) > 0) ++total;
    XLAL_CHECK(total == total_ref[i], XLAL_EFUNC, "ERROR: alternating total = %" LAL_UINT8_FORMAT " != %" LAL_UINT8_FORMAT " = total_ref", total, total_ref[i]);

    // Cleanup
    XLALDestroyLatticeTilingIterator(itr_alt);

  }

  // Cleanup
  XLALDestroyLatticeTiling(tiling);
  XLALDestroyLatticeTilingLocator(loc);
  LALCheckMemoryLeaks();
  printf("\n");
  fflush(stdout);

  return XLAL_SUCCESS;

}

static int MismatchTest(
  LatticeTiling *tiling,
  gsl_matrix *metric,
  const double max_mismatch,
  const UINT8 total_ref,
  const double mism_hist_ref[MISM_HIST_BINS]
  )
{

  const size_t n = XLALTotalLatticeTilingDimensions(tiling);

  // Create lattice tiling iterator and locator
  LatticeTilingIterator *itr = XLALCreateLatticeTilingIterator(tiling, n);
  XLAL_CHECK(itr != NULL, XLAL_EFUNC);
  LatticeTilingLocator *loc = XLALCreateLatticeTilingLocator(tiling);
  XLAL_CHECK(loc != NULL, XLAL_EFUNC);

  // Count number of points
  const UINT8 total = XLALTotalLatticeTilingPoints(itr);
  printf("Number of lattice points: %" LAL_UINT8_FORMAT "\n", total);
  XLAL_CHECK(total == total_ref, XLAL_EFUNC, "ERROR: total = %" LAL_UINT8_FORMAT " != %" LAL_UINT8_FORMAT " = total_ref", total, total_ref);

  // Get all points
  gsl_matrix *GAMAT(points, n, total);
  XLAL_CHECK(XLALNextLatticeTilingPoints(itr, &points) == (int)total, XLAL_EFUNC);
  XLAL_CHECK(XLALNextLatticeTilingPoint(itr, NULL) == 0, XLAL_EFUNC);

  // Initialise mismatch histogram counts
  double mism_hist[MISM_HIST_BINS] = {0};
  double mism_hist_total = 0, mism_hist_out_of_range = 0;

  // Perform 10 injections for every template
  {
    gsl_matrix *GAMAT(injections, 3, total);
    gsl_matrix *GAMAT(nearest, 3, total);
    gsl_matrix *GAMAT(temp, 3, total);
    RandomParams *rng = XLALCreateRandomParams(total);
    XLAL_CHECK(rng != NULL, XLAL_EFUNC);
    for (size_t i = 0; i < 10; ++i) {

      // Generate random injection points
      XLAL_CHECK(XLALRandomLatticeTilingPoints(tiling, 0.0, rng, injections) == XLAL_SUCCESS, XLAL_EFUNC);

      // Find nearest lattice template points
      XLAL_CHECK(XLALNearestLatticeTilingPoints(loc, injections, &nearest, NULL, NULL, NULL) == XLAL_SUCCESS, XLAL_EFUNC);

      // Compute mismatch between injections
      gsl_matrix_sub(nearest, injections);
      gsl_blas_dsymm(CblasLeft, CblasUpper, 1.0, metric, nearest, 0.0, temp);
      for (size_t j = 0; j < temp->size2; ++j) {
        gsl_vector_view temp_j = gsl_matrix_column(temp, j);
        gsl_vector_view nearest_j = gsl_matrix_column(nearest, j);
        double mismatch = 0.0;
        gsl_blas_ddot(&nearest_j.vector, &temp_j.vector, &mismatch);
        mismatch /= max_mismatch;

        // Increment mismatch histogram counts
        ++mism_hist_total;
        if (mismatch < 0.0 || mismatch > 1.0) {
          ++mism_hist_out_of_range;
        } else {
          ++mism_hist[lround(floor(mismatch * MISM_HIST_BINS))];
        }

      }

    }

    // Cleanup
    GFMAT(injections, nearest, temp);
    XLALDestroyRandomParams(rng);

  }

  // Normalise histogram
  for (size_t i = 0; i < MISM_HIST_BINS; ++i) {
    mism_hist[i] *= MISM_HIST_BINS / mism_hist_total;
  }

  // Print mismatch histogram and its reference
  printf("Mismatch histogram: ");
  for (size_t i = 0; i < MISM_HIST_BINS; ++i) {
    printf(" %0.3f", mism_hist[i]);
  }
  printf("\n");
  printf("Reference histogram:");
  for (size_t i = 0; i < MISM_HIST_BINS; ++i) {
    printf(" %0.3f", mism_hist_ref[i]);
  }
  printf("\n");

  // Determine error between mismatch histogram and its reference
  double mism_hist_error = 0.0;
  for (size_t i = 0; i < MISM_HIST_BINS; ++i) {
    mism_hist_error += fabs(mism_hist[i] - mism_hist_ref[i]);
  }
  mism_hist_error /= MISM_HIST_BINS;
  printf("Mismatch histogram error: %0.3e\n", mism_hist_error);
  const double mism_hist_error_tol = 5e-2;
  if (mism_hist_error >= mism_hist_error_tol) {
    XLAL_ERROR(XLAL_EFAILED, "ERROR: mismatch histogram error exceeds %0.3e\n", mism_hist_error_tol);
  }

  // Check fraction of injections out of histogram range
  const double mism_out_of_range = mism_hist_out_of_range / mism_hist_total;
  printf("Fraction of points out of histogram range: %0.3e\n", mism_out_of_range);
  const double mism_out_of_range_tol = 2e-3;
  if (mism_out_of_range > mism_out_of_range_tol) {
    XLAL_ERROR(XLAL_EFAILED, "ERROR: fraction of points out of histogram range exceeds %0.3e\n", mism_out_of_range_tol);
  }

  // Perform 10 injections outside parameter space
  {
    gsl_matrix *GAMAT(injections, 3, 10);
    gsl_matrix *GAMAT(nearest, n, total);
    RandomParams *rng = XLALCreateRandomParams(total);
    XLAL_CHECK(rng != NULL, XLAL_EFUNC);

    // Generate random injection points outside parameter space
    XLAL_CHECK(XLALRandomLatticeTilingPoints(tiling, 5.0, rng, injections) == XLAL_SUCCESS, XLAL_EFUNC);

    // Find nearest lattice template points
    XLAL_CHECK(XLALNearestLatticeTilingPoints(loc, injections, &nearest, NULL, NULL, NULL) == XLAL_SUCCESS, XLAL_EFUNC);

    // Cleanup
    GFMAT(injections, nearest);
    XLALDestroyRandomParams(rng);

  }

  // Cleanup
  XLALDestroyLatticeTiling(tiling);
  XLALDestroyLatticeTilingIterator(itr);
  XLALDestroyLatticeTilingLocator(loc);
  GFMAT(metric, points);
  LALCheckMemoryLeaks();
  printf("\n");
  fflush(stdout);

  return XLAL_SUCCESS;

}

static int MismatchSquareTest(
  const TilingLattice lattice,
  const double freqband,
  const double f1dotband,
  const double f2dotband,
  const UINT8 total_ref,
  const double mism_hist_ref[MISM_HIST_BINS]
  )
{

  // Create lattice tiling
  LatticeTiling *tiling = XLALCreateLatticeTiling(3);
  XLAL_CHECK(tiling != NULL, XLAL_EFUNC);

  // Add bounds
  const double fndot[3] = {100, 0, 0};
  const double fndotband[3] = {freqband, f1dotband, f2dotband};
  for (size_t i = 0; i < 3; ++i) {
    printf("Bounds: f%zudot=%0.3g, f%zudotband=%0.3g\n", i, fndot[i], i, fndotband[i]);
    XLAL_CHECK(XLALSetLatticeTilingConstantBound(tiling, i, fndot[i], fndot[i] + fndotband[i]) == XLAL_SUCCESS, XLAL_EFUNC);
  }

  // Set metric to the spindown metric
  const double max_mismatch = 0.3;
  gsl_matrix *GAMAT(metric, 3, 3);
  for (size_t i = 0; i < metric->size1; ++i) {
    for (size_t j = i; j < metric->size2; ++j) {
      const double Tspan = 432000;
      const double metric_i_j_num = 4.0 * LAL_PI * LAL_PI * pow(Tspan, i + j + 2) * (i + 1) * (j + 1);
      const double metric_i_j_denom = LAL_FACT[i + 1] * LAL_FACT[j + 1] * (i + 2) * (j + 2) * (i + j + 3);
      gsl_matrix_set(metric, i, j, metric_i_j_num / metric_i_j_denom);
      gsl_matrix_set(metric, j, i, gsl_matrix_get(metric, i, j));
    }
  }
  printf("Lattice type: %u\n", lattice);
  XLAL_CHECK(XLALSetTilingLatticeAndMetric(tiling, lattice, metric, max_mismatch) == XLAL_SUCCESS, XLAL_EFUNC);

  // Perform mismatch test
  XLAL_CHECK(MismatchTest(tiling, metric, max_mismatch, total_ref, mism_hist_ref) == XLAL_SUCCESS, XLAL_EFUNC);

  return XLAL_SUCCESS;

}

static int MismatchAgeBrakeTest(
  const TilingLattice lattice,
  const double freq,
  const double freqband,
  const UINT8 total_ref,
  const double mism_hist_ref[MISM_HIST_BINS]
  )
{

  // Create lattice tiling
  LatticeTiling *tiling = XLALCreateLatticeTiling(3);
  XLAL_CHECK(tiling != NULL, XLAL_EFUNC);

  // Add bounds
  printf("Bounds: freq=%0.3g, freqband=%0.3g\n", freq, freqband);
  XLAL_CHECK(XLALSetLatticeTilingConstantBound(tiling, 0, freq, freq + freqband) == XLAL_SUCCESS, XLAL_EFUNC);
  XLAL_CHECK(XLALSetLatticeTilingF1DotAgeBrakingBound(tiling, 0, 1, 1e11, 2, 5) == XLAL_SUCCESS, XLAL_EFUNC);
  XLAL_CHECK(XLALSetLatticeTilingF2DotBrakingBound(tiling, 0, 1, 2, 2, 5) == XLAL_SUCCESS, XLAL_EFUNC);

  // Set metric to the spindown metric
  const double max_mismatch = 0.3;
  gsl_matrix *GAMAT(metric, 3, 3);
  for (size_t i = 0; i < metric->size1; ++i) {
    for (size_t j = i; j < metric->size2; ++j) {
      const double Tspan = 1036800;
      const double metric_i_j_num = 4.0 * LAL_PI * LAL_PI * pow(Tspan, i + j + 2) * (i + 1) * (j + 1);
      const double metric_i_j_denom = LAL_FACT[i + 1] * LAL_FACT[j + 1] * (i + 2) * (j + 2) * (i + j + 3);
      gsl_matrix_set(metric, i, j, metric_i_j_num / metric_i_j_denom);
      gsl_matrix_set(metric, j, i, gsl_matrix_get(metric, i, j));
    }
  }
  printf("Lattice type: %u\n", lattice);
  XLAL_CHECK(XLALSetTilingLatticeAndMetric(tiling, lattice, metric, max_mismatch) == XLAL_SUCCESS, XLAL_EFUNC);

  // Perform mismatch test
  XLAL_CHECK(MismatchTest(tiling, metric, max_mismatch, total_ref, mism_hist_ref) == XLAL_SUCCESS, XLAL_EFUNC);

  return XLAL_SUCCESS;

}

static int SuperskyTest(
  const double T,
  const double max_mismatch,
  const TilingLattice lattice,
  const UINT8 patch_count,
  const double freq,
  const double freqband,
  const UINT8 total_ref,
  const double mism_hist_ref[MISM_HIST_BINS]
  )
{

  // Create lattice tiling
  LatticeTiling *tiling = XLALCreateLatticeTiling(3);
  XLAL_CHECK(tiling != NULL, XLAL_EFUNC);

  // Compute reduced supersky metric
  const double Tspan = T * 86400;
  LIGOTimeGPS ref_time;
  XLALGPSSetREAL8(&ref_time, 900100100);
  LALSegList segments;
  {
    XLAL_CHECK(XLALSegListInit(&segments) == XLAL_SUCCESS, XLAL_EFUNC);
    LALSeg segment;
    LIGOTimeGPS start_time = ref_time, end_time = ref_time;
    XLALGPSAdd(&start_time, -0.5 * Tspan);
    XLALGPSAdd(&end_time, 0.5 * Tspan);
    XLAL_CHECK(XLALSegSet(&segment, &start_time, &end_time, 0) == XLAL_SUCCESS, XLAL_EFUNC);
    XLAL_CHECK(XLALSegListAppend(&segments, &segment) == XLAL_SUCCESS, XLAL_EFUNC);
  }
  MultiLALDetector detectors = {
    .length = 1,
    .sites = { lalCachedDetectors[LAL_LLO_4K_DETECTOR] }
  };
  EphemerisData *edat =  XLALInitBarycenter(TEST_DATA_DIR "earth00-19-DE405.dat.gz",
                                            TEST_DATA_DIR "sun00-19-DE405.dat.gz");
  XLAL_CHECK(edat != NULL, XLAL_EFUNC);
  SuperskyMetrics *metrics = XLALComputeSuperskyMetrics(0, &ref_time, &segments, freq, &detectors, NULL, DETMOTION_SPIN | DETMOTION_PTOLEORBIT, edat);
  XLAL_CHECK(metrics != NULL, XLAL_EFUNC);
  gsl_matrix *rssky_metric = metrics->semi_rssky_metric, *rssky_transf = metrics->semi_rssky_transf;
  metrics->semi_rssky_metric = metrics->semi_rssky_transf = NULL;
  XLALDestroySuperskyMetrics(metrics);
  XLALSegListClear(&segments);
  XLALDestroyEphemerisData(edat);

  // Add bounds
  printf("Bounds: supersky, sky patch 0/%" LAL_UINT8_FORMAT ", freq=%0.3g, freqband=%0.3g\n", patch_count, freq, freqband);
  XLAL_CHECK(XLALSetSuperskyLatticeTilingPhysicalSkyPatch(tiling, rssky_metric, rssky_transf, patch_count, 0) == XLAL_SUCCESS, XLAL_EFUNC);
  XLAL_CHECK(XLALSetSuperskyLatticeTilingPhysicalSpinBound(tiling, rssky_transf, 0, freq, freq + freqband) == XLAL_SUCCESS, XLAL_EFUNC);
  GFMAT(rssky_transf);

  // Set metric
  printf("Lattice type: %u\n", lattice);
  XLAL_CHECK(XLALSetTilingLatticeAndMetric(tiling, lattice, rssky_metric, max_mismatch) == XLAL_SUCCESS, XLAL_EFUNC);

  // Perform mismatch test
  XLAL_CHECK(MismatchTest(tiling, rssky_metric, max_mismatch, total_ref, mism_hist_ref) == XLAL_SUCCESS, XLAL_EFUNC);

  return XLAL_SUCCESS;

}

static int MultiSegSuperskyTest(void)
{
  printf("Performing multiple-segment tests ...\n");

  // Compute reduced supersky metrics
  const double Tspan = 86400;
  LIGOTimeGPS ref_time;
  XLALGPSSetREAL8(&ref_time, 900100100);
  LALSegList segments;
  {
    XLAL_CHECK(XLALSegListInit(&segments) == XLAL_SUCCESS, XLAL_EFUNC);
    LALSeg segment;
    {
      LIGOTimeGPS start_time = ref_time, end_time = ref_time;
      XLALGPSAdd(&start_time, -4 * Tspan);
      XLALGPSAdd(&end_time, -3 * Tspan);
      XLAL_CHECK(XLALSegSet(&segment, &start_time, &end_time, 0) == XLAL_SUCCESS, XLAL_EFUNC);
      XLAL_CHECK(XLALSegListAppend(&segments, &segment) == XLAL_SUCCESS, XLAL_EFUNC);
    }
    {
      LIGOTimeGPS start_time = ref_time, end_time = ref_time;
      XLALGPSAdd(&start_time, -0.5 * Tspan);
      XLALGPSAdd(&end_time, 0.5 * Tspan);
      XLAL_CHECK(XLALSegSet(&segment, &start_time, &end_time, 0) == XLAL_SUCCESS, XLAL_EFUNC);
      XLAL_CHECK(XLALSegListAppend(&segments, &segment) == XLAL_SUCCESS, XLAL_EFUNC);
    }
    {
      LIGOTimeGPS start_time = ref_time, end_time = ref_time;
      XLALGPSAdd(&start_time, 3.5 * Tspan);
      XLALGPSAdd(&end_time, 4.5 * Tspan);
      XLAL_CHECK(XLALSegSet(&segment, &start_time, &end_time, 0) == XLAL_SUCCESS, XLAL_EFUNC);
      XLAL_CHECK(XLALSegListAppend(&segments, &segment) == XLAL_SUCCESS, XLAL_EFUNC);
    }
  }
  MultiLALDetector detectors = {
    .length = 1,
    .sites = { lalCachedDetectors[LAL_LLO_4K_DETECTOR] }
  };
  EphemerisData *edat =  XLALInitBarycenter(TEST_DATA_DIR "earth00-19-DE405.dat.gz",
                                            TEST_DATA_DIR "sun00-19-DE405.dat.gz");
  XLAL_CHECK(edat != NULL, XLAL_EFUNC);
  SuperskyMetrics *metrics = XLALComputeSuperskyMetrics(1, &ref_time, &segments, 50, &detectors, NULL, DETMOTION_SPIN | DETMOTION_PTOLEORBIT, edat);
  XLAL_CHECK(metrics != NULL, XLAL_EFUNC);

  // Project and rescale semicoherent metric to give equal frequency spacings
  const double coh_max_mismatch = 0.2, semi_max_mismatch = 0.4;
  XLAL_CHECK(XLALEqualizeReducedSuperskyMetricsFreqSpacing(metrics, coh_max_mismatch, semi_max_mismatch) == XLAL_SUCCESS, XLAL_EFUNC);

  // Create lattice tilings
  LatticeTiling *coh_tiling[metrics->num_segments];
  for (size_t n = 0; n < metrics->num_segments; ++n) {
    coh_tiling[n] = XLALCreateLatticeTiling(4);
    XLAL_CHECK(coh_tiling[n] != NULL, XLAL_EFUNC);
  }
  LatticeTiling *semi_tiling = XLALCreateLatticeTiling(4);
  XLAL_CHECK(semi_tiling != NULL, XLAL_EFUNC);

  // Add bounds
  for (size_t n = 0; n < metrics->num_segments; ++n) {
    XLAL_CHECK(XLALSetSuperskyLatticeTilingPhysicalSkyPatch(coh_tiling[n], metrics->coh_rssky_metric[n], metrics->coh_rssky_transf[n], 1, 0) == XLAL_SUCCESS, XLAL_EFUNC);
    XLAL_CHECK(XLALSetSuperskyLatticeTilingPhysicalSpinBound(coh_tiling[n], metrics->coh_rssky_transf[n], 0, 50, 50 + 1e-4) == XLAL_SUCCESS, XLAL_EFUNC);
    XLAL_CHECK(XLALSetSuperskyLatticeTilingPhysicalSpinBound(coh_tiling[n], metrics->coh_rssky_transf[n], 1, 0, -5e-10) == XLAL_SUCCESS, XLAL_EFUNC);
  }
  XLAL_CHECK(XLALSetSuperskyLatticeTilingPhysicalSkyPatch(semi_tiling, metrics->semi_rssky_metric, metrics->semi_rssky_transf, 1, 0) == XLAL_SUCCESS, XLAL_EFUNC);
  XLAL_CHECK(XLALSetSuperskyLatticeTilingPhysicalSpinBound(semi_tiling, metrics->semi_rssky_transf, 0, 50, 50 + 1e-4) == XLAL_SUCCESS, XLAL_EFUNC);
  XLAL_CHECK(XLALSetSuperskyLatticeTilingPhysicalSpinBound(semi_tiling, metrics->semi_rssky_transf, 1, 0, -5e-10) == XLAL_SUCCESS, XLAL_EFUNC);

  // Set metric
  for (size_t n = 0; n < metrics->num_segments; ++n) {
    XLAL_CHECK(XLALSetTilingLatticeAndMetric(coh_tiling[n], TILING_LATTICE_ANSTAR, metrics->coh_rssky_metric[n], coh_max_mismatch) == XLAL_SUCCESS, XLAL_EFUNC);
  }
  XLAL_CHECK(XLALSetTilingLatticeAndMetric(semi_tiling, TILING_LATTICE_ANSTAR, metrics->semi_rssky_metric, semi_max_mismatch) == XLAL_SUCCESS, XLAL_EFUNC);

  // Check lattice step sizes in frequency
  const size_t ifreq = 3;
  const double semi_dfreq = XLALLatticeTilingStepSizes(semi_tiling, ifreq);
  for (size_t n = 0; n < metrics->num_segments; ++n) {
    const double coh_dfreq = XLALLatticeTilingStepSizes(coh_tiling[n], ifreq);
    const double tol = 1e-8;
    XLAL_CHECK(fabs(coh_dfreq - semi_dfreq) < tol * semi_dfreq, XLAL_EFAILED,
               "  ERROR: semi_dfreq=%0.15e, coh_dfreq[%zu]=%0.15e, |coh_dfreq - semi_dfreq| >= %g * semi_dfreq", semi_dfreq, n, coh_dfreq, tol);
  }

  // Cleanup
  for (size_t n = 0; n < metrics->num_segments; ++n) {
    XLALDestroyLatticeTiling(coh_tiling[n]);
  }
  XLALDestroyLatticeTiling(semi_tiling);
  XLALDestroySuperskyMetrics(metrics);
  XLALSegListClear(&segments);
  XLALDestroyEphemerisData(edat);
  LALCheckMemoryLeaks();
  printf("\n");
  fflush(stdout);

  return XLAL_SUCCESS;

}

int main(void)
{

  // Perform basic tests
  XLAL_CHECK_MAIN(BasicTest(0, TILING_LATTICE_ANSTAR,  1,   1,   1,    1) == XLAL_SUCCESS, XLAL_EFUNC);
  XLAL_CHECK_MAIN(BasicTest(1, TILING_LATTICE_CUBIC,  93,   0,   0,    0) == XLAL_SUCCESS, XLAL_EFUNC);
  XLAL_CHECK_MAIN(BasicTest(1, TILING_LATTICE_ANSTAR, 93,   0,   0,    0) == XLAL_SUCCESS, XLAL_EFUNC);
  XLAL_CHECK_MAIN(BasicTest(2, TILING_LATTICE_CUBIC,  13, 190,   0,    0) == XLAL_SUCCESS, XLAL_EFUNC);
  XLAL_CHECK_MAIN(BasicTest(2, TILING_LATTICE_ANSTAR, 12, 144,   0,    0) == XLAL_SUCCESS, XLAL_EFUNC);
  XLAL_CHECK_MAIN(BasicTest(3, TILING_LATTICE_CUBIC,   8,  60, 583,    0) == XLAL_SUCCESS, XLAL_EFUNC);
  XLAL_CHECK_MAIN(BasicTest(3, TILING_LATTICE_ANSTAR,  8,  46, 332,    0) == XLAL_SUCCESS, XLAL_EFUNC);
  XLAL_CHECK_MAIN(BasicTest(4, TILING_LATTICE_CUBIC,   7,  46, 287, 2543) == XLAL_SUCCESS, XLAL_EFUNC);
  XLAL_CHECK_MAIN(BasicTest(4, TILING_LATTICE_ANSTAR,  6,  30, 145,  897) == XLAL_SUCCESS, XLAL_EFUNC);

  // Perform mismatch tests with a square parameter space
  XLAL_CHECK_MAIN(MismatchSquareTest(TILING_LATTICE_CUBIC,  0.03,     0,     0, 21460,  Z1_mism_hist) == XLAL_SUCCESS, XLAL_EFUNC);
  XLAL_CHECK_MAIN(MismatchSquareTest(TILING_LATTICE_CUBIC,  2e-4, -2e-9,     0, 23763,  Z2_mism_hist) == XLAL_SUCCESS, XLAL_EFUNC);
  XLAL_CHECK_MAIN(MismatchSquareTest(TILING_LATTICE_CUBIC,  1e-4, -1e-9, 1e-17, 19550,  Z3_mism_hist) == XLAL_SUCCESS, XLAL_EFUNC);
  XLAL_CHECK_MAIN(MismatchSquareTest(TILING_LATTICE_ANSTAR, 0.03,     0,     0, 21460, A1s_mism_hist) == XLAL_SUCCESS, XLAL_EFUNC);
  XLAL_CHECK_MAIN(MismatchSquareTest(TILING_LATTICE_ANSTAR, 2e-4, -2e-9,     0, 18283, A2s_mism_hist) == XLAL_SUCCESS, XLAL_EFUNC);
  XLAL_CHECK_MAIN(MismatchSquareTest(TILING_LATTICE_ANSTAR, 1e-4, -2e-9, 2e-17, 20268, A3s_mism_hist) == XLAL_SUCCESS, XLAL_EFUNC);

  // Perform mismatch tests with an age--braking index parameter space
  XLAL_CHECK_MAIN(MismatchAgeBrakeTest(TILING_LATTICE_ANSTAR, 100, 4.0e-5, 37872, A3s_mism_hist) == XLAL_SUCCESS, XLAL_EFUNC);
  XLAL_CHECK_MAIN(MismatchAgeBrakeTest(TILING_LATTICE_ANSTAR, 200, 1.5e-5, 37232, A3s_mism_hist) == XLAL_SUCCESS, XLAL_EFUNC);
  XLAL_CHECK_MAIN(MismatchAgeBrakeTest(TILING_LATTICE_ANSTAR, 300, 1.0e-5, 37022, A3s_mism_hist) == XLAL_SUCCESS, XLAL_EFUNC);

  // Perform mismatch tests with the reduced supersky parameter space and metric
  XLAL_CHECK_MAIN(SuperskyTest(1.1, 0.8, TILING_LATTICE_ANSTAR,  1, 50, 2.0e-5, 20548, A3s_mism_hist) == XLAL_SUCCESS, XLAL_EFUNC);
  XLAL_CHECK_MAIN(SuperskyTest(1.5, 0.8, TILING_LATTICE_ANSTAR,  3, 50, 2.0e-5, 20202, A3s_mism_hist) == XLAL_SUCCESS, XLAL_EFUNC);
  XLAL_CHECK_MAIN(SuperskyTest(2.5, 0.8, TILING_LATTICE_ANSTAR, 17, 50, 2.0e-5, 29147, A3s_mism_hist) == XLAL_SUCCESS, XLAL_EFUNC);

  // Perform tests with the reduced supersky parameter space metric and multiple segments
  XLAL_CHECK_MAIN(MultiSegSuperskyTest() == XLAL_SUCCESS, XLAL_EFUNC);

  return EXIT_SUCCESS;

}

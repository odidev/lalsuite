# Perform a fully-coherent search of a single segment, and compare F-statistics to reference results

export LAL_FSTAT_FFT_PLAN_MODE=ESTIMATE

echo "=== Create single-segment search setup ==="
set -x
lalapps_WeaveSetup --ephem-earth=earth00-40-DE405.dat.gz --ephem-sun=sun00-40-DE405.dat.gz --first-segment=1122332211/90000 --detectors=H1,L1 --output-file=WeaveSetup.fits
lalapps_fits_overview WeaveSetup.fits
set +x
echo

echo "=== Restrict timestamps to segment list in WeaveSetup.fits ==="
set -x
lalapps_fits_table_list 'WeaveSetup.fits[segments][col c1=start_s; col2=end_s]' \
    | awk 'BEGIN { print "/^#/ { print }" } /^#/ { next } { printf "%i <= $1 && $1 <= %i { print }\n", $1, $2 + 1 }' > timestamp-filter.awk
awk -f timestamp-filter.awk all-timestamps-1.txt > timestamps-1.txt
awk -f timestamp-filter.awk all-timestamps-2.txt > timestamps-2.txt
set +x
echo

echo "=== Extract reference time from WeaveSetup.fits ==="
set -x
ref_time=`lalapps_fits_header_getval "WeaveSetup.fits[0]" 'DATE-OBS GPS' | tr '\n\r' '  ' | awk 'NF == 1 {printf "%.9f", $1}'`
set +x
echo

echo "=== Generate SFTs with injected signal ==="
set -x
inject_params="Alpha=5.4; Delta=1.1; Freq=55.5; f1dot=-1e-9"
lalapps_Makefakedata_v5 --randSeed=1234 --fmin=55.0 --Band=1.0 \
    --injectionSources="{refTime=${ref_time}; h0=0.5; cosi=0.2; psi=0.4; phi0=0.1; ${inject_params}}" \
    --Tsft=1800 --outSingleSFT --outSFTdir=. --IFOs=H1,L1 --sqrtSX=1,1 \
    --timestampsFiles=timestamps-1.txt,timestamps-2.txt
set +x
echo

echo "=== Perform fully-coherent search ==="
set -x
lalapps_Weave --output-file=WeaveOut.fits \
    --toplists=mean2F --toplist-limit=0 --extra-statistics="mean2F_det" \
    --toplist-tmpl-idx --segment-info \
    --setup-file=WeaveSetup.fits --sft-files='*.sft' \
    --Fstat-method=ResampGeneric --Fstat-run-med-window=50 \
    --freq=55.5~0.005 --f1dot=-2e-9,0 \
    --semi-max-mismatch=9
lalapps_fits_overview WeaveOut.fits
set +x
echo

echo "=== Check for non-singular semicoherent dimensions ==="
set -x
semi_ntmpl_prev=1
for dim in SSKYA SSKYB NU1DOT NU0DOT; do
    semi_ntmpl=`lalapps_fits_header_getval "WeaveOut.fits[0]" "NSEMITMPL ${dim}" | tr '\n\r' '  ' | awk 'NF == 1 {printf "%d", $1}'`
    expr ${semi_ntmpl} '>' ${semi_ntmpl_prev}
    semi_ntmpl_prev=${semi_ntmpl}
done
set +x
echo

echo "=== Check that no results were recomputed ==="
set -x
coh_nres=`lalapps_fits_header_getval "WeaveOut.fits[0]" 'NCOHRES' | tr '\n\r' '  ' | awk 'NF == 1 {printf "%d", $1}'`
coh_ntmpl=`lalapps_fits_header_getval "WeaveOut.fits[0]" 'NCOHTPL' | tr '\n\r' '  ' | awk 'NF == 1 {printf "%d", $1}'`
expr ${coh_nres} '=' ${coh_ntmpl}
set +x
echo

echo "=== Check computed number of coherent and semicoherent templates ==="
set -x
coh_ntmpl=`lalapps_fits_header_getval "WeaveOut.fits[0]" 'NCOHTPL' | tr '\n\r' '  ' | awk 'NF == 1 {printf "%d", $1}'`
semi_ntmpl=`lalapps_fits_header_getval "WeaveOut.fits[0]" 'NSEMITPL' | tr '\n\r' '  ' | awk 'NF == 1 {printf "%d", $1}'`
expr ${coh_ntmpl} '=' ${semi_ntmpl}
set +x
echo

### Make updating reference results a little easier ###
mkdir newtarball/
cd newtarball/
cp ../all-timestamps-1.txt ../all-timestamps-2.txt .
cp ../RefExact.txt .
cp ../WeaveOut.fits RefWeaveOut.fits
tar zcf ../new_testWeave_single_segment.tar.gz *
cd ..
rm -rf newtarball/

echo "=== Compare F-statistics from lalapps_Weave to reference results ==="
set -x
if lalapps_WeaveCompare --setup-file=WeaveSetup.fits --result-file-1=WeaveOut.fits --result-file-2=RefWeaveOut.fits; then
    exitcode=0
else
    exitcode=77
    lalapps_WeaveCompare --setup-file=WeaveSetup.fits --result-file-1=WeaveOut.fits --result-file-2=RefWeaveOut.fits --param-tol-mism=0
fi
set +x
echo

echo "=== Compare F-statistic at exact injected signal parameters with loudest F-statistic found by lalapps_Weave ==="
set -x
coh2F_exact=`cat RefExact.txt | sed -n '/^[^%]/p' | awk '{print $7}'`
lalapps_fits_table_list "WeaveOut.fits[mean2F_toplist][col c1=mean2F][#row == 1]" > tmp
coh2F_loud=`cat tmp | sed "/^#/d" | xargs printf "%.16g"`
# Value of 'mean_mu' was calculated by:
#   octapps_run WeaveFstatMismatch --setup-file=WeaveSetup.fits --spindowns=1 --semi-max-mismatch=9 --coh-max-mismatch=0 --printarg=meanOfHist
mean_mu=0.58305
awk "BEGIN { print mu = ( ${coh2F_exact} - ${coh2F_loud} ) / ${coh2F_exact}; exit ( mu < ${mean_mu} ? 0 : 1 ) }"
set +x
echo

echo "=== Check that lalapps_Weave can be run at a single point ==="
set -x
lalapps_fits_table_list "WeaveOut.fits[mean2F_toplist][col c1=alpha; c2=delta; c3=freq; c4=f1dot; c5=mean2F][#row == 1]" > tmp
alpha_loud=`cat tmp | sed "/^#/d" | awk '{print $1}' | xargs printf "%.16g"`
delta_loud=`cat tmp | sed "/^#/d" | awk '{print $2}' | xargs printf "%.16g"`
freq_loud=` cat tmp | sed "/^#/d" | awk '{print $3}' | xargs printf "%.16g"`
f1dot_loud=`cat tmp | sed "/^#/d" | awk '{print $4}' | xargs printf "%.16g"`
coh2F_loud=`cat tmp | sed "/^#/d" | awk '{print $5}' | xargs printf "%.16g"`
lalapps_Weave --output-file=WeaveOutSingle.fits \
    --toplists=mean2F --toplist-limit=0 --extra-statistics="mean2F_det" --segment-info \
    --setup-file=WeaveSetup.fits --sft-files='*.sft' \
    --alpha=${alpha_loud}~0 --delta=${delta_loud}~0 --freq=${freq_loud}~0 --f1dot=${f1dot_loud}~0 \
    --semi-max-mismatch=9
lalapps_fits_overview WeaveOutSingle.fits
lalapps_fits_table_list "WeaveOutSingle.fits[mean2F_toplist][col c1=mean2F][#row == 1]" > tmp
coh2F_loud_single=`cat tmp | sed "/^#/d" | xargs printf "%.16g"`
mean_mu=0.05
awk "BEGIN { print mu = ( ${coh2F_loud} - ${coh2F_loud_single} ) / ${coh2F_loud}; exit ( mu < ${mean_mu} ? 0 : 1 ) }"
set +x
echo

exit ${exitcode}

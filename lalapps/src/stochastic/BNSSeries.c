/*
 *  BNSSeries.c
 *  
 *  Created by Tania Regimbau on 8/8/09.
 *
 */

/*
* simulate times series from the extragalactic population of BNS
* gcc -o BNS BNSSeries.c -I${LAL_PREFIX}/include -L/usr/local/lib -L${LAL_PREFIX}/lib  -lm -llal -llalsupport
*/


#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <lal/LALStdio.h>
#include <lal/LALStdlib.h>
#include <lal/LALConfig.h>
#include <lal/Units.h>
#include <lal/LALConstants.h>
#include <lal/LALDatatypes.h>
#include <lal/LALRCSID.h>
#include <lal/LALStatusMacros.h>
#include <lal/AVFactories.h>
#include <lal/PrintFTSeries.h>
#include <lal/ReadFTSeries.h>
#include <lal/StreamInput.h>
#include <lal/PrintVector.h>
#include <lal/VectorOps.h>
#include <lal/FileIO.h> 
#include <lal/Random.h>
#include <lal/Interpolate.h>


#define Pi2 2.*LAL_PI
#define year  3.156e7
#define Msolar 1.989e33
#define day 86164.1

NRCSID (BNSSeriesC, "$Id: BNSSeries.c,v 1.1 2009/08/04 11:07:52 tania Exp $");
RCSID ("$Id: BNSSeries.c,v 1.1 2009/08/04 11:07:52 tania Exp $");

/* cvs info */
#define CVS_ID "$Id: BNSSeries.c,v 1.1 2009/08/04 11:07:52 tania Exp $"
#define CVS_REVISION "$Revision: 1.1 $"
#define CVS_DATE "$Date: 2009/08/04 11:07:52 $"
#define PROGRAM_NAME "BNSSeries"

/* variables for getopt options parsing */
extern char *optarg;
extern int optind;

/* flag for getopt_long */
static int verbose_flag = 0;
static int condor_flag = 0;

UINT4 seed=10;
UINT4 job=1;//always 1 if not running on a cluster
REAL8 Tobs=604800.; //1 week
REAL8 deltaT=1.; //sampling
//REAL8 zMax=6.;
//REAL8 mu=1.37;
REAL8 zMax=2.;
REAL8 mu=943;
REAL8 fMin=10.;

/*Virgo localization*/
REAL8 lambda=43.63*(LAL_PI/180.);
REAL8 gama=116.5*(LAL_PI/180.);
REAL8 ksi=60.*(LAL_PI/180.);
REAL8 wr=Pi2/day*3600.;/*rotation in radians/h*/ 

void parseOptions(INT4 argc, CHAR *argv[]);
void displayUsage(INT4 exitcode);

INT4 main (INT4 argc, CHAR *argv[])
 {
   static LALStatus status;
   int i, j, k;
   UINT4 N, Nobs, Ntot;
   REAL8 TwaveMax; //duration of the waveform
   REAL4 alea1, alea2, reject, sign;
   REAL8 lambda2, clambda, slambda, clambda2, slambda2;
   REAL8 gama2, cgama, sgama, cgama2, sgama2;
   REAL8 phi, phi2, cphi, sphi, cphi2, sphi2;
   REAL8 theta, theta2, ctheta, stheta, ctheta2, stheta2;
   REAL8 psi, psi2, cpsi2, spsi2;
   REAL8 alphat, alphat2, calphat, salphat, calphat2, salphat2;
   REAL8 cinc, inc, sksi;
   REAL8 a11, a12, a21, a31, a41, a51, a1, a2, a3, a4, a5, a;
   REAL8 b11, b21, b31, b41, b1, b2, b3, b4, b;
   REAL8 m1, m2, Mz, Mcz, Mcz1,Mcz2;
   REAL8 z, dL;
   REAL8 nu1, nu, numax,numax1,tau;
   REAL8 phasec, phase1, phasez, phase, cphase, sphase;
   REAL8 A, Ap, Ac, hp, hc, h;
   REAL8 p;
   REAL8 t, dt,TcMax;
   
   REAL8Vector *tc, *to, *serie;
   REAL8 time[25], Fplus[25], Fcross[25]; 
   
   /*pointers to input/output files*/
   FILE *pfin,*pfout1,*pfout2;

   /* parse command line options */
   parseOptions(argc, argv);
   
   /* output file name */
   CHAR fileName1[LALNameLength], fileName2[LALNameLength];
   LALSnprintf(fileName1, LALNameLength,"catalog_%d.dat",job);
   pfout1 = fopen(fileName1,"w");
   LALSnprintf(fileName2, LALNameLength,"serie_%d.dat",job);
   pfout2 = fopen(fileName2,"w");
   
   /* initialize status pointer */
   status.statusPtr = NULL;
   
   /* random generator */
   RandomParams *randParams=NULL;
   LALCreateRandomParams(&status, &randParams, seed);

   /* interpolation structures */
   DInterpolatePar intparp = {25, time, Fplus};
   DInterpolatePar intparc = {25, time, Fcross};
   DInterpolateOut Fp, Fc;   
   
   /* observation times */
   Nobs=floor(Tobs/deltaT)+1;
   to = NULL;
   LALDCreateVector(&status, &to, Nobs);
   for(i=1;i<Nobs;i++)
   to->data[i]=to->data[i-1]+deltaT+(double)(j-1)*Tobs;
   
   /* initialize output time serie */
   serie = NULL;
   LALDCreateVector(&status, &serie, Nobs);
   
   /* calculate some constants */
   sksi=sin(ksi);sksi=sin(ksi);	
   gama2=2.*gama;cgama=cos(gama);sgama=sin(gama);cgama2=cos(gama2);sgama2=sin(gama2);
   lambda2=2.*lambda;clambda=cos(lambda);slambda=sin(lambda);clambda2=cos(lambda2);slambda2=sin(lambda2);
   a11=0.0625*sgama2*(3.-clambda2);
   a21=0.25*cgama2*slambda;
   a31=0.25*sgama2*slambda2;
   a41=0.5*cgama2*clambda;
   a51=0.75*sgama2*clambda*clambda;
   b11=cgama2*slambda;
   b21=0.25*sgama2*(3.-clambda2);
   b31=cgama2*clambda;
   b41=0.5*sgama2*slambda2;

   if (verbose_flag)
	 printf("generate coalescence times\n"); 
   /*coalescence times (Poisson statistic)*/
   if(condor_flag){
    /* read coalescence time from file generated by poisson.c */    
    pfin = fopen("times.dat","r"); 
    LALDReadVector(&status, &tc, pfin, 0);
    N = tc->length;
   }   
   else{ 
    TwaveMax=815132.*pow(fMin,-8./3.)*(zMax+1.); //for a 1-1 binary at z=6
    N=floor(1.3*(Tobs+TwaveMax)/mu);
    tc = NULL;
    LALDCreateVector(&status, &tc, N);
    i=0; 
    tc->data[0]=0.;
    do {
     i++;
	 LALUniformDeviate(&status, &alea1, randParams); 
	 dt=-mu*log(alea1);
	 tc->data[i]=tc->data[i-1]+dt;
	 }
    while (tc->data[i]<=Tobs+TwaveMax);
    N=i;
   }
   //LALDPrintVector(tc);
   
   if (verbose_flag)
	 printf("generate %d sources\n", N);
   Ntot=0;	 
   /* generate sources */
   for (i=0;i<N;i++)
    {
	 if((i%1000)==0){
	  if (verbose_flag){
	   printf("source %d... select parameters...\t",i);}}
	   
     /*select masses between 1-3, gaussianly distributed with mu=1.4 and sigma=0.5*/
	 do
      {
       LALUniformDeviate(&status, &alea1, randParams); 
       LALUniformDeviate(&status, &alea2, randParams);
	   m1=alea1*2.+1.;
       reject=(double)alea2;
       p=exp(-2.*(m1-1.4)*(m1-1.4));
      }
     while (reject>p);
     
	 do
	  {
	   LALUniformDeviate(&status, &alea1, randParams); 
	   LALUniformDeviate(&status, &alea2, randParams);
	   m2=alea1*2.+1.;
	   reject=(double)alea2;
	   p=exp(-2.*(m1-1.4)*(m1-1.4));
	  }
	 while (reject>p);
	
	 /* select redshift */
     do
      {
       LALUniformDeviate(&status, &alea1, randParams); 
       LALUniformDeviate(&status, &alea2, randParams);
       z=(double)alea1*zMax;
       reject=0.43*(double)alea2;
       p=-0.000429072589677+(z*(-0.036349728568888+(z*(0.860892111762314+(z*(-0.740935488674010+z*(0.265848831356864+z*(-0.050041573542298+z*(0.005184554232421+z*(-0.000281450045300+z*0.000006400690921))))))))));
      }
     while (reject>p);

     /* calculate duration of the waveform and check if it is in our observation window */
	 /* redshifted chirp mass */
	 Mcz=pow(pow((m1*m2),3.)/(m1+m2),1./5.)*(1.+z);
	 Mcz2=pow(Mcz,5./3.);
	 tau = 646972./Mcz2*pow(fMin,-8./3.)*(z+1.);
	 
     if(tau>tc->data[i]-Tobs){
	 Ntot++;
	 /* select sky position in celestial coordinates */
	 LALUniformDeviate( &status, &alea1, randParams); 
	 phi=(double)alea1*Pi2;
	 
	 LALUniformDeviate(&status, &alea1, randParams); 
	 LALUniformDeviate(&status, &alea2, randParams);
	 if (alea2<0.5){ctheta=-(double)alea1;}
	  else{ctheta=(double)alea1;}
     theta=acos(ctheta);

	 /* select inclination */
	 LALUniformDeviate(&status, &alea1, randParams); 
	 LALUniformDeviate(&status, &alea2, randParams);
	 if (alea2<0.5){cinc=-(double)alea1;}
	  else {cinc=(double)alea1;}
     inc=acos(cinc);
	 
	 /* select polarization */
	 LALUniformDeviate(&status, &alea1, randParams); 
	 psi=(double)alea1*Pi2;
	
	 /* select phase at coalescence */
	 LALUniformDeviate(&status, &alea1, randParams); 
	 phase=(double)alea1*Pi2;
	 
	 /* write parameters to file */
	 fprintf(pfout1,"%d %f %f %f %f %f %f %f %f %f\n",i,tc->data[i],z,phi,theta,inc,psi,phase,m1,m2);

     /* calculate some constants */
     phi2=2.*phi;cphi=cos(phi);sphi=sin(phi);cphi2=cos(phi2);sphi2=sin(phi2);
     theta2=2.*theta;stheta=sin(theta);ctheta2=cos(theta2);stheta2=sin(theta2);
     psi2=2.*psi;cpsi2=cos(psi2);spsi2=sin(psi2);
     a12=3.-ctheta2;
     a1=a11*a12;
     a2=a21*a12;
     a3=a31*stheta2;
     a4=a41*stheta2;
     a5=a51*ctheta*ctheta;
     b1=b11*stheta;
     b2=b21*stheta;
     b3=b31*ctheta;
     b4=b41*ctheta;
	 
	 /* fit of luminosity distance in Mpc for h0=0.7, omega_m=0.3, omega_v=0.7 */
	 dL=-2.89287707063171+(z*(4324.33492012756+(z*(3249.74193862773+(z*(-1246.66339928289+z*(335.354613407693+z*(-56.1194965448065+z*(5.20261234121263+z*(-0.203151569744028))))))))));
   
     /* redshifted mass and chirp mass */
	 Mz=(m1+m2)*(1+z);
	 Mcz1=pow(Mcz,-5./8.);
	 
	 /* maximal observed frequency */
	 numax=4397./Mz;
	 numax1=pow(numax,-3./8.);
	 
	 /* quantities needed to calculate the waveform */
	 nu1=151.*Mcz1; //constant in the expression of the frequency
	 phase1=-1518.38*Mcz1; //constant in the expression of the phase
	 A=2.73531e-22*Mcz2/dL;
	 Ap=A*(1.0+(cinc*cinc));
	 Ac=-2.*A*cinc;
	 
	 /* calculate beam pattern function over a day */
	 for(k=0;k<=24;k++){
	  time[k]=(double)k;
      alphat=(phi-wr*time[k]);alphat2=2.*alphat;
      calphat=cos(alphat);salphat=sin(alphat);
      calphat2=cos(alphat2);salphat2=sin(alphat2);
      a=a1*calphat2-a2*salphat2+a3*calphat-a4*salphat+a5;
      b=b1*calphat2+b2*salphat2+b3*calphat+b4*salphat;
      
	  Fplus[k]=sksi*(a*cpsi2+b*spsi2);
      Fcross[k]=sksi*(b*cpsi2-a*spsi2);
	 }

	 if((i%1000)==0){
	  if (verbose_flag){
	   printf("calculate waveform...\n",i);}}
	   
	 /* calculate contribution of the source to the time serie */
	 if(tc->data[i]>Tobs) {j=Tobs/deltaT;}
	 else {j=floor(tc->data[i]/deltaT);}
	 
	 do
	  {
	   tau=tc->data[i]-to->data[j];//time to coalescence
	   t=fmod(to->data[j],day)/3600.;//time in the day
	   nu=(pow(numax1+1.54566e-6*Mcz2*tau,-3./8.));//frequency
	   phase=phase1*pow(tau,5./3.)+phasec;
	   cphase=cos(phase);
	   sphase=sin(phase);
	   hp=Ap*cphase;
	   hc=Ac*sphase;
	   
	   /*interpolcate to find Fp and Fc at time t*/
	   LALDPolynomialInterpolation(&status, &Fp, t, &intparp);
	   LALDPolynomialInterpolation(&status, &Fc, t, &intparc);
	   
	   /*observed strain*/
	   h=hp*Fp.y+hc*Fc.y;	   
	   serie->data[j]=serie->data[j]+h;
	   j--;
	   if(j<0){break;}
	   }  
	 while (nu>fMin);//exit loop when leave frequency band
   }}

  if (verbose_flag)
	printf("%d sources... write time serie to file\n",Ntot);
  /* write to file */ 
  for(j=0;j<Nobs;j++)
   fprintf(pfout2,"%f %e\n",(double)j*deltaT,serie->data[j]);
   
  if (verbose_flag)
	printf("clean up and exit\n");
  /* clean up */
  LALDDestroyVector(&status, &to);
  LALDDestroyVector(&status, &tc);
  LALDDestroyVector(&status, &serie);
  fclose(pfout1);  fclose(pfout2);  
 }
 
 
/* parse command line options */
void parseOptions(INT4 argc, CHAR *argv[])
 {
  int c = -1;
  
  while(1)
   {
    static struct option long_options[] =
     {
	  /* options that set a flag */
      {"verbose", no_argument, &verbose_flag, 1},
	  {"condor", no_argument, &condor_flag, 1},
      /* options that don't set a flag */
      {"help", no_argument, 0, 'h'},
	  {"seed", required_argument, 0, 's'},
	  {"job-number", required_argument, 0, 'j'},
	  {"duration", required_argument, 0, 'T'},
	  {"sampling-time", required_argument, 0, 't'},
      {"poisson-parameter", required_argument, 0, 'p'},
	  {"frequency-min", required_argument, 0, 'f'},
      {"z-max", required_argument, 0, 'z'},
      {"version", no_argument, 0, 'v'},
      {0, 0, 0, 0}
     };

    /* getopt_long stores the option here */
    int option_index = 0;

    c = getopt_long(argc, argv, 
                  "hs:j:T:t:p:f:z:v:",
 		   long_options, &option_index);

    if (c == -1)
     {
      /* end of options, break loop */
      break;
     }

    switch(c)
     {
      case 0:
             /* If this option set a flag, do nothing else now. */
             if (long_options[option_index].flag != 0)
              break;
             printf ("option %s", long_options[option_index].name);
             if (optarg)
              printf (" with arg %s", optarg);
             printf ("\n");
             break;

      case 'h':
               /* HELP!!! */
               displayUsage(0);
               break;
			   
	  case 's':
			   /* seed */
	           seed = atoi(optarg);
	           break;
			   	   
      case 'j':
			   /* job number */
	           job = atoi(optarg);
	           break;
			   
      case 'T':
			   /* duration */
	           Tobs = atof(optarg);
	           break;
			   
	  case 't':
			   /* sampling time */
	           deltaT = atof(optarg);
	           break;
			   
	  case 'p':
			   /* poisson parameter */
	           mu = atof(optarg);
	           break;

      case 'f':
			   /* minimal frequency */
	           fMin = atof(optarg);
	           break;
			   
	  case 'z':
			   /* maximal redshift */
	           zMax = atof(optarg);
	           break;
			   
      case 'v':
	     /* display version info and exit */
	     fprintf(stdout, "simulation of extragalactic BNS\n" CVS_ID "\n");
	     exit(0);
	     break;

     default:
     displayUsage(1);
     }
    }

   if (optind < argc)
    {
     displayUsage(1);
    }

  return;
}

/* display program usage */
void displayUsage(INT4 exitcode)
 {
  fprintf(stderr, "Usage: pipeline [options]\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, " -h                    print this message\n");
  fprintf(stderr, " -v                    display version\n");
  fprintf(stderr, " --verbose             verbose mode\n");
  fprintf(stderr, " --condor              run on cluster\n");
  fprintf(stderr, " -s                    seed for random generator\n");
  fprintf(stderr, " -j                    job number\n");
  fprintf(stderr, " -T                    length of the time serie\n");
  fprintf(stderr, " -t                    sampling time of the time serie\n");
  fprintf(stderr, " -p                    time interval between successive coalescences\n");
  fprintf(stderr, " -f                    minimal frequency\n");
  fprintf(stderr, " -z                    maximal redshift\n");
  exit(exitcode);
}

  

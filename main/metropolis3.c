#include<math.h>
#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include"random.h"
#include"plot.h"
#include"libutil.h"

#define N 32
#define PRINT

double M = 1;
double W = 1;
double D = 3;

double V(double x){ return M*W*W*x*x/2; }

double dS(double *x,double y,int i){ return M*((x[i]-y)*(x[(i+1)%N]+x[(i-1+N)%N])+(y*y-x[i]*x[i]))+V(y)-V(x[i]); }

double action(double *x){
    double s = 0;
    int i; for(i = 0; i < N-1; i++)
        s += M*(x[i+1]-x[i])*(x[i+1]-x[i])/2+(V(x[i])+V(x[i+1]))/2;
    return s;
}

double metropolis(double *x){
    double DS = 0, y, ds, r[2*N];
    ranlxd(r,2*N);
    int i; for(i = 0; i < N; i++){
        y = x[i]+D*(2*r[i]-1);
        ds = dS(x,y,i);
        if(r[N+i] < exp(-ds)){
            x[i] = y;
            DS += ds;
        }
    }
    return DS;
}

double correlation(double* x,int dt){
    double odt = 0;
    int i; for(i = 0; i < N; i++)
        odt += x[i]*x[(i+dt)%N];
    return odt/N;
}

double autoCorrelation(int i,int k,int n,double* data){
    double mean,var,Rk;
    mean = var = Rk = 0;
    int t; for(t = 0; t < n-k; t++){
        double tmp = data[N*t+i];
        Rk += tmp*data[N*(t+k)+i];
        mean += tmp;
        var += tmp*tmp;
    }
    mean /= n-k;
    var = var/(n-k)-mean*mean;
    return (Rk/(n-k)-mean*mean)/var;
}

int main(int argc,char* argv[]){
    /*init ranlux*/
    rlxd_init(2,time(NULL));
    /*init stuff*/
    int cycles = 1e6; if(argc == 2) cycles = atoi(argv[1]);
    int wid = 100; if(argc == 3) wid = atoi(argv[2]);
    int bin = cycles/wid;
    int i,j,k;
    FILE* f;
    /*init mem*/
    double* data = malloc(N*wid*bin*sizeof(double));
    double* dtcl = malloc(N*bin*sizeof(double));
    /*init variables*/
    double x[N],c[N],var[N],tmp;
    for(i = 0; i < N; i++){
        x[i] = 100;
        c[i] = var[i] = 0;
    }

    /*action*/
    double S = action(x);
    f = fopen("action.dat","w");
    for(i = 0; i < 1000; i++)
        fprintf(f,"%d\t%lf\n",(i+1),S += metropolis(x));
    fclose(f); plot_action(); system("rm action.dat");

    /*metropolis loop*/
    for(i = 0; i < bin; i++)
        for(j = 0; j < wid; j++){
            metropolis(x);
            for(k = 0; k < N; k++){
                tmp = correlation(x,k);
                data[(i*wid+j)*N+k] = tmp;
                dtcl[k*bin+i] += tmp/wid;
            }
#ifdef PRINT
            loading(i*wid+j,bin*wid);
#endif
        }

    /*autocorrelation*/
    f = fopen("autocorrelation.dat","w");
    for(i = 0; i < 30; i++)
        fprintf(f,"%d\t%lf\n",i,autoCorrelation(1,i,bin*wid,data));
    fclose(f); plot_autocorrelation(); system("rm autocorrelation.dat");
    free(data);

    /*correlation*/
    f = fopen("correlation.dat","w");
    for(k = 0; k < N; k++){
        for(i = 0; i < bin; i++){
            c[k] += tmp = dtcl[k*bin+i];
            var[k] += tmp*tmp;
        }
        c[k] /= bin;
        var[k] = var[k]/bin-c[k]*c[k];
        fprintf(f,"%d\t%lf\t%lf\n",k,fabs(c[k]),var[k]);
    }
    fclose(f); plot_correlation(); system("rm correlation.dat");

    /*delta E*/
    double dE = 0;
    for(i = 2; i < 5; i++)
        dE += acosh((c[i+1]+c[i-1])/(2*c[i]));
    dE /= 3;

    /*jackknife*/
    double mcl[6],dEm = 0,var_dE = 0;
    for(i = 1; i < 6; i++){
        mcl[i] = 0;
        for(j = 0; j < bin; j++)
            mcl[i] += dtcl[i*bin+j];
        mcl[i] /= bin;
        for(j = 0; j < bin; j++)
            dtcl[i*bin+j] = mcl[i]+(mcl[i]-dtcl[i*bin+j])/(bin-1);
    }
    for(i = 2; i < 5; i++)
        dEm += acosh((mcl[i+1]+mcl[i-1])/(2*mcl[i]));
    for(j = 0; j < bin; j++){
        double tmp = 0;
        for(i = 2; i < 5; i++)
            tmp += acosh((dtcl[(i+1)*bin+j]+dtcl[(i-1)*bin+j])/(2*dtcl[i*bin+j]));
        var_dE += (tmp-dEm)*(tmp-dEm)/9;
    }
    var_dE = (var_dE*(bin-1))/bin;
    printf("\n\n dE  = %lf\n\n σ = %e\n\n",dE,sqrt(var_dE));
    f = fopen("dE.dat","a");
    fprintf(f,"%lf\n",dE);
    fclose(f);
    plot_histogram();

    free(dtcl);
    return 0;
}


/**************************************************************************

    Matrix solvers. The band matrix solver is from Numerical Recipes.

**************************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/10/03 16:10:21 $
 * $Id: vdeal_banbks.c,v 1.6 2014/10/03 16:10:21 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <vdeal.h>
#include <infr.h>

#define SWAP(a,b) {dum=(a);(a)=(b);(b)=dum;}
#define TINY 1.0e-20f

static int Time_out_secs = 0, Time_out_start = 0;

static void banbks (Banbks_t **a, int n, int m1, int m2, 
			Banbks_t **al, int indx[], Banbks_t b[]);
static int bandec (Banbks_t **a, int n, int m1, int m2, 
			Banbks_t **al, int indx[], Banbks_t *d);
static int lubksb (Banbks_t **a, int n, int *indx, Banbks_t *b);
static int ludcmp(Banbks_t **a, int n, int *indx, Banbks_t *d);
static int Solve_non_band_matrix (int n, int bw, Banbks_t *a, Banbks_t *b);
static void Mv_multiply (int n, Sp_matrix *a, Spmcg_t *x, Spmcg_t *y);
static Spmcg_t V_norm (int n, Spmcg_t *u);
static void Vv_add (int n, Spmcg_t su, Spmcg_t *u, 
				Spmcg_t sv, Spmcg_t *v, Spmcg_t *out);
static Spmcg_t Vv_dot_mul (int n, Spmcg_t *u, Spmcg_t *v);
static int Cg_solve (int n, Sp_matrix *a, Spmcg_t *b, Spmcg_t *x);
static int Solve_by_cg (int n, Sp_matrix *a, Banbks_t *b);
static int Parallelize_cg (int n, Sp_matrix *a);
static void *T_cg_process (void *arg);
static void Cg_proc_sec (int t_ind);
static void Cg_process ();


/**************************************************************************

    Sets/checks timeout. 

***************************************************************************/

void VDB_check_timeout (int seconds) {

    if (seconds == 0)		/* reset/cancel timedout check */
	Time_out_secs = 0;
    else {			/* start timedout check */
	Time_out_secs = seconds;
	Time_out_start = MISC_systime (NULL);
    }
}

/**************************************************************************

    Fits n values of "y" at location "x" to a linear function y = a * x + b.
    Returns a and b. Returns 0 on success of -1 on failure. 

***************************************************************************/

int VDB_linear_fit (int n, double *x, double *y, double *ap, double *bp) {
    double x2, x1, x0, y1, y0;
    double d, a, b, ad;
    int i;

    if (n <= 0)
	return (-1);

    x2 = x1 = 0.;
    y1 = y0 = 0.;
    for (i = 0; i < n; i++) {
	double xx;
	xx = x[i];
	x2 += xx * xx;
	x1 += xx;
	y1 += xx * y[i];
	y0 += y[i];
    }
    x0 = (double)n;
    d = x2 * x0 - x1 * x1;
    ad = d;
    if (ad < 0.)
	ad = -ad;
    if (ad >= 1.e-20) {
	d = 1. / d;
	a = (y1 * x0 - x1 * y0) * d;
	b = (x2 * y0 - y1 * x1) * d;
    }
    else {
	a = 0.;
	b = y0 / n;
    }
    *ap = a;
    *bp = b;
    return (0);
}

/**************************************************************************

    This solves the equation A x = b. "n" is the number of unknowns. If
    the band width of A is large, the CG iterative solver is called (A
    must be symmetric, positive definite). Otherwise if A is band limited,
    the banbks program is called. Otherwise, A generic solver is called.
    To call banbks, aa, in the band limited format, is formed. aa is an 
    "n" by "2 * bw + 1" array. Its center column contains the diagonal
    elements. "b" returns the result. Return values: 0 on success; -1 for
    malloc failed; -3 for singular matrix A; -2 for other errors.

***************************************************************************/

int VDB_solve (int n, Sp_matrix *a, Banbks_t *b) {
    static __thread int pn = 0, pbw = 0, pbma_s = 0;
    static __thread Banbks_t **aa = NULL, **al = NULL, *bma = NULL;
    static __thread int *indx = NULL;
    int bw, rs, i;
    Banbks_t d;
    int *c_ind, *ne;
    Spmcg_t *ev;
    extern int Max_partition_size;

    if (n <= 0)
	return (-2);

    bw = 0;		/* find band width */
    ne = a->ne;
    c_ind = a->c_ind;
    for (i = 0; i < n; i++) {
	int nei, k;
	nei = ne[i];
	for (k = 0; k < nei; k++) {
	    int ind = *c_ind;
	    c_ind++;
	    if (ind < 0 || ind >= n)
		return (-2);
	    ind -= i;
	    if (ind < 0)
		ind = -ind;
	    if (ind > bw)
		bw = ind;
	}
    }

    if (bw > Max_partition_size + 2 * OVERLAP_SIZE)
	return (Solve_by_cg (n, a, b));

    rs = 2 * bw + 1;
    if (rs * n > pbma_s) {
	if (bma != NULL)
	    free (bma);
	pbma_s = 0;
	bma = (Banbks_t *)MISC_malloc (rs * n * sizeof (Banbks_t));
	if (bma == NULL)
	    return (-1);
	pbma_s = rs * n;
    }
    ne = a->ne;
    c_ind = a->c_ind;
    ev = a->ev;
    for (i = 0; i < n; i++) {
	Banbks_t *p;
	int k, nei;

	p = bma + (i * rs);
	for (k = 0; k < rs; k++)
	    p[k] = 0.;
	nei = ne[i];
	for (k = 0; k < nei; k++) {
	    int ind = *c_ind;
	    c_ind++;
	    p[ind + bw - i] = *ev;
	    ev++;
	}
    }

    if (n < rs)
	return (Solve_non_band_matrix (n, bw, bma, b));

    if (n > pn || bw > pbw) {
	if (aa != NULL) {
	    free (aa);
	    pn = 0;
	}
	aa = (Banbks_t **)MISC_malloc (2 * (n + 1) * sizeof (Banbks_t *) +
			n * bw * sizeof (Banbks_t) +
			(n + 1) * sizeof (int));
	if (aa == NULL)
	    return (-1);
	al = aa + (n + 1);
	al[0] = (Banbks_t *)(aa + 2 * (n + 1));
	indx = (int *)((char *)al[0] + n * bw * sizeof (Banbks_t));

	pn = n;
	pbw = bw;
	for (i = 1; i <= n; i++)
	    al[i] = al[0] + (i - 1) * bw - 1;
    }

    for (i = 1; i <= n; i++) {
	aa[i] = bma + (i - 1) * (2 * bw + 1) - 1;
    }

    if (bandec (aa, n, bw, bw, al, indx, &d) != 0)
	return (-3);
    banbks (aa, n, bw, bw, al, indx, b - 1);

    if (rs * n > 50000) {
	if (bma != NULL)
	    free (bma);
	bma = NULL;
	pbma_s = 0;
	if (aa != NULL)
	    free (aa);
	aa = NULL;
	pn = 0;
    }

    return (0);
}

/**************************************************************************

    Solve linear system using the CG solver.

***************************************************************************/

static int Solve_by_cg (int n, Sp_matrix *a, Banbks_t *b) {
    static __thread int b_sz = 0;
    static __thread char *buf = NULL;
    Spmcg_t *bb, *xx;
    int i, ret;

    if (n > b_sz) {
	if (buf != NULL)
	    free (buf);
	b_sz = 0;
	buf = MISC_malloc (n * 2 * sizeof (Spmcg_t));
	if (buf == NULL)
	    return (-1);
	b_sz = n;
    }
    bb = (Spmcg_t *)buf;
    xx = bb + n;

    for (i = 0; i < n; i++) {
	bb[i] = b[i];
	xx[i] = 0.;
    }

    ret = Cg_solve (n, a, bb, xx);
    if (ret < 0)
	return (ret);
    for (i = 0; i < n; i++)
	b[i] = xx[i];
    return (0);
}

/**************************************************************************

    Solve linear system using the non-band functions.

***************************************************************************/

static int Solve_non_band_matrix (int n, int bw, Banbks_t *a, Banbks_t *b) {
    static __thread int pn = 0;
    static __thread Banbks_t **aa = NULL;
    static __thread int *indx = NULL;
    Banbks_t d;
    int i, ret;

    if (n > pn) {
	if (aa != NULL) {
	    free (aa);
	    free (indx);
	    pn = 0;
	}
	aa = (Banbks_t **)MISC_malloc ((n + 1) * sizeof (Banbks_t *));
	if (aa == NULL)
	    return (-1);
	indx = (int *)MISC_malloc ((n + 1) * sizeof (int));
	if (indx == NULL) {
	    free (aa);
	    return (-1);
	}
	pn = n;
    }

    for (i = 1; i <= n; i++) {
	aa[i] = a + (i - 1) * (2 * bw + 1) + (bw + 1 - i) - 1;
    }

    ret = ludcmp (aa, n, indx, &d);
    if (ret == -2)	/* malloc failed */
	return (-1);
    else if (ret < 0)	/* singular matrix */
	return (-3);
    lubksb (aa, n, indx, b - 1);

    return (0);
}

/**************************************************************************

    Given an n n band diagonal matrix A with m1 subdiagonal rows and m2
    superdiagonal rows, compactly stored in the array
    a[1..n][1..m1+m2+1] as described in the comment for routine banmul,
    this routine constructs an LU decomposition of a rowwise permutation
    of A. The upper triangular matrix replaces a, while the lower
    triangular matrix is returned in al[1..n][1..m1]. indx[1..n] is an
    output vector which records the row permutation eected by the
    partial pivoting; d is output as 1 depending on whether the number
    of row interchanges was even or odd, respectively. This routine is
    used in combination with banbks to solve band-diagonal sets of
    equations. Some intermediate variables are used for improving 
    performance.

**************************************************************************/

static int bandec (Banbks_t **a, int n, int m1, int m2, 
			Banbks_t **al, int indx[], Banbks_t *d) {
    int i,j,k,l, singular;
    int mm;
    Banbks_t dum;

    singular = 0;
    mm=m1+m2+1;
    l=m1;
    /* Rearrange the storage a bit. */
    for (i=1;i<=m1;i++) {
	for (j=m1+2-i;j<=mm;j++) a[i][j-l]=a[i][j];
	l--;
	for (j=mm-l;j<=mm;j++) a[i][j]=0.0;
    }

    *d=1.0f;
    l=m1;
    for (k=1;k<=n;k++) {	/* For each row... */
	Banbks_t *ak = a[k];
	dum=ak[1];
	i=k;
	if (l < n) l++;
	for (j=k+1;j<=l;j++) {	/* Find the pivot element. */
	    Banbks_t aj1 = a[j][1];
	    if (fabs(aj1) > fabs(dum)) {
		dum=aj1;
		i=j;
	    }
	}

	indx[k]=i;
	if (dum == 0.0f) {	/* Matrix is algorithmically singular */
	    a[k][1]=TINY;	/* but proceed anyway with TINY pivot 
				   (desirable in some applications). */
	    singular = 1;
	}
   
	if (i != k) {		/* Interchange rows. */
	    Banbks_t *ai = a[i];
	    *d = -(*d);
	    for (j=1;j<=mm;j++) SWAP(ak[j],ai[j])
	}

	for (i=k+1;i<=l;i++) {	/* Do the elimination. */
	    Banbks_t *ai = a[i];
	    dum=ai[1]/ak[1];
	    al[k][i-k]=dum;
	    for (j=2;j<=mm;j++) ai[j-1]=ai[j]-dum*ak[j];
	    ai[mm]=0.0f;
	}
    }
    if (singular)
	VDD_log ("Singular system - bandec\n");
    return (0);
}

/**************************************************************************

    Given the arrays a, al, and indx as returned from bandec, and given
    a right-hand side vector b[1..n], solves the band diagonal linear
    equations A x = b. The solution vector x overwrites b[1..n]. The
    other input arrays are not modied, and can be left in place for
    successive calls with dierent right-hand sides.

**************************************************************************/

static void banbks (Banbks_t **a, int n, int m1, int m2, 
			Banbks_t **al, int indx[], Banbks_t b[]) {
    int i,k,l;
    int mm;
    Banbks_t dum;

    mm=m1+m2+1;
    l=m1;
    /* Forward substitution, unscrambling the permuted rows as we go. */
    for (k=1;k<=n;k++) { 
	i=indx[k];
	if (i != k) SWAP(b[k],b[i])
	if (l < n) l++;
	for (i=k+1;i<=l;i++) b[i] -= al[k][i-k]*b[k];
    }

    l=1;
    /* Backsubstitution. */
    for (i=n;i>=1;i--) { 
	dum=b[i];
	for (k=2;k<=l;k++) dum -= a[i][k]*b[k+i-1];
	b[i]=dum/a[i][1];
	if (l < mm) l++;
    }
}

/**************************************************************************

    Given a matrix a[1..n][1..n], this routine replaces it by the LU
    decomposition of a row wise permutation of itself. a and n are
    input. a is output, arranged as in equation (2.3.14) above;
    indx[1..n] is an output vector that records the row permutation
    effected by the partial pivoting; d is output as +-1 dependeing on
    whether the number of raw interchanges was even or odd,
    respectively. This routine is used in combination with lubksb
    to solve linear equations or invert a matrix. Returns 0 on success,
    -1 on singular matrix, or -2 on malloc failure.

**************************************************************************/

static int ludcmp(Banbks_t **a, int n, int *indx, Banbks_t *d) {
    static __thread Banbks_t *vv = NULL;
			/* stores the implicit scaling of each row */
    static __thread int vv_s = 0;
    int i, imax, j, k, singular;
    Banbks_t big, dum, sum, temp;

    singular = 0;
    if (n > vv_s) {
	if (vv != NULL)
	    free (vv);
	vv_s = 0;
	vv = (Banbks_t *)MISC_malloc ((n + 1) * sizeof (Banbks_t));
	if (vv == NULL)
	    return (-2);
	vv_s = n;
    }
    *d=1.0;			/* No row interchanges yet */
    for (i=1;i<=n;i++) {	/* Loop over rows to get implicit scaling */
	big=0.0;
	for (j=1;j<=n;j++) {
	    if ((temp=fabs(a[i][j])) > big)
		big=temp;
	}
	if (big == 0.0) {
	    VDD_log ("Singular system - ludcmp\n");
	    return (-1);
	}
	vv[i]=1.0/big;	/* save the scaling */
    }
    imax = 0;			/* to eliminate a compiler warning */
    for (j=1;j<=n;j++) {	/* the loop over columns of Crout's method */
	for (i=1;i<j;i++) {	/* this is equation (2.3.12), i != j */
	    sum=a[i][j];
	    for (k=1;k<i;k++) sum -= a[i][k]*a[k][j];
	    a[i][j]=sum;
	}
	big=0.0;	/* search for the largest pivot element */
	for (i=j;i<=n;i++) {	/* equation (2.3.12), (2.3.13) */
	    Banbks_t abs;
	    sum=a[i][j];
	    for (k=1;k<j;k++)
		sum -= a[i][k]*a[k][j];
	    a[i][j]=sum;
	    abs = sum;
	    if (abs < 0.)
		abs = -abs;
	    if ( (dum=vv[i]*abs) >= big) {
		/* is the figure of merit for the pivot better than the best 
		   so far? */
		big=dum;
		imax=i;
	    }
	}
	if (j != imax) {	/* do we need to interchange rows? */
	    for (k=1;k<=n;k++) {	/* yes. do so */
		dum=a[imax][k];
		a[imax][k]=a[j][k];
		a[j][k]=dum;
	    }
	    *d = -(*d);		/* and change the parity of d */
	    vv[imax]=vv[j];	/* also interchange the scale factor */ 
	}
	indx[j]=imax;
	if (a[j][j] == 0.0) {	/* a[j][j]=TINY; */
	    a[j][j]=TINY;
	    singular = 1;
	}
	if (j != n) {	/* finally, divide by the pivot element */
	    dum=1.0/(a[j][j]);
	    for (i=j+1;i<=n;i++) a[i][j] *= dum;
	}
    }			/* go back to the next column in the reduction */
    if (singular)
	VDD_log ("Near singular system - ludcmp\n");
    return (0);
}

/**************************************************************************

    Solves the set of n linear euations A.X = B. Here a[1..n][1..n] is
    input, not as the matrix A but rather as its LU decomposition,
    determined by the routine ludcmp. indx[1..n] is input as the
    premutation vector returned by ludcmp. b[1..n] is input as the
    right-hand side vector B. and returns with the solution vector X. a,
    n, and indx are not modified by this routine and can be left in
    place for successive calls with different right-hand sides b. This
    routine takes into account the possibility that b will begin with
    many zero elements, so it is efficient for use in matrix inversion.

**************************************************************************/

static int lubksb (Banbks_t **a, int n, int *indx, Banbks_t *b) {
    int i, ii = 0, ip, j;
    Banbks_t sum;

    for (i=1;i<=n;i++) {	/* forward substitution */
	ip=indx[i];
	sum=b[ip];
	b[ip]=b[i];
	if (ii) {  /* ii is the index of the first non-zero element of b */
	    for (j=ii;j<=i-1;j++) sum -= a[i][j]*b[j];
	}
	else if (sum)
	    ii=i;
	b[i]=sum;
    }
    for (i=n;i>=1;i--) {	/* back substitution */
	sum=b[i];
	for (j=i+1;j<=n;j++) sum -= a[i][j]*b[j];
	b[i]=sum/a[i][i];
    }
    return (0);
}

/**************************************************************************

    Prepares for threading the matrix multiplication function Mv_multiply.

**************************************************************************/

#include <pthread.h>
#include <errno.h>
static int N_threads = 1;
int VDB_threads = 1;
static int T_buf_size = 0;

static int *T_cind_st = NULL, *T_st_ind, *T_n;
static Sp_matrix *Ta;
static Spmcg_t *Tw, *Tr, *Tz, *Tx, T_sec, Talfa;
static Spmcg_t *Tds = NULL, *Talfas, *Tnorms, *Tbetas;

static pthread_t *T_threads = NULL;
static pthread_mutex_t Sync_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t Sync_st = PTHREAD_COND_INITIALIZER;
static pthread_cond_t Sync_end = PTHREAD_COND_INITIALIZER;
static int Sync_cnt = 0;

static int Parallelize_cg (int n, Sp_matrix *a) {
    int i, j, k, inc;
    int *ne, *cind;

    if (VDB_threads > T_buf_size) {
	if (T_cind_st != NULL)
	    free (T_cind_st);
	if (Tds != NULL)
	    free (Tds);
	T_buf_size = 0;
	T_cind_st = MISC_malloc (VDB_threads * (sizeof (int) * 3));
	if (T_cind_st == NULL)
	    return (-1);
	T_st_ind = T_cind_st + VDB_threads;
	T_n = T_st_ind + VDB_threads;
	Tds = MISC_malloc (VDB_threads * sizeof (Spmcg_t) * 4);
	if (Tds == NULL)
	    return (-1);
	T_buf_size = VDB_threads;
	Talfas = Tds + T_buf_size;
	Tnorms = Talfas + T_buf_size;
	Tbetas = Tnorms + T_buf_size;
    }

    N_threads = VDB_threads;
    if (n < 6000 || N_threads <= 1) {
	N_threads = 1;
	T_cind_st[0] = T_st_ind[0] = 0;
	T_n[0] = n;
	return (0);
    }

    if (T_threads == NULL) {
	T_threads = (pthread_t *)MISC_malloc ((VDB_threads - 1) * sizeof (pthread_t));
	if (T_threads == NULL)
	    return (-1);
	for (i = 1; i < VDB_threads; i++) {
	    int t_ind, err;

	    t_ind = i;
	    if ((err = pthread_create 
		(T_threads + i - 1, NULL, T_cg_process, (void *)t_ind)) != 0) {
		VDD_log ("pthread_create 0 ret %d, errno %d\n", err, errno);
		exit (1);
	    }
	}
    }

    inc = n / N_threads;
    ne = a->ne;
    cind = a->c_ind;
    k = 0;
    for (i = 0; i < n; i++) {
	if (i == k * inc) {
	    T_st_ind[k] = i;
	    T_n[k] = inc + i;
	    T_cind_st[k] = cind - a->c_ind;
	    k++;
	    if (k >= N_threads) {
		T_n[k - 1] = n;
		break;
	    }
	}
	for (j = 0; j < ne[i]; j++)
	    cind++;
    }
    return (0);
}

/**************************************************************************

    Solves system a * x = b of n unknowns. a is a sparse, symmetric and 
    positive definite matrix. x contains the initial solution vector.

**************************************************************************/

static int Cg_solve (int n, Sp_matrix *a, Spmcg_t *b, Spmcg_t *x) {
    static __thread int b_size = 0;
    static __thread char *buf = NULL;
    Spmcg_t alfa, beta, d, test, one, zero;
    Spmcg_t *t, *r, *w, *z;
    int i, cnt, cf;
    time_t st_t;
    extern int VDD_in_thread;

    N_threads = 1;
    if (!VDD_in_thread &&
	Parallelize_cg (n, a) < 0)
	return (-1);	/* malloc failed */

    if (n > b_size) {
	if (buf != NULL)
	    free (buf);
	buf = MISC_malloc (n * sizeof (Spmcg_t) * 4);
	if (buf == NULL)
	    return (-1);
	b_size = n;
    }

    t = (Spmcg_t *)buf;
    r = t + n;
    w = r + n;
    z = w + n;
    if (N_threads > 1) {
	Tr = r;
	Tw = w;
	Ta = a;
	Tz = z;
	Tx = x;
    }

    one = 1.;
    zero = 0.;
    Mv_multiply (n, a, x, t);
    Vv_add (n, one, b, -one, t, r);
    Vv_add (n, -one, r, zero, t, w);
    Mv_multiply (n, a, w, z);
    if ((d = Vv_dot_mul (n, w, z)) == zero) {
	VDD_log ("Singular system (intial) - Cg_solve\n");
	return (-1);
    }
    alfa = Vv_dot_mul (n, r, w) / d;
    beta = zero;

    test = .00001; /* 1.e-10; */
    cnt = 0;
    st_t = MISC_systime (NULL);
    if (n < 10000)
	cf = 1000;	/* time check frequency */
    else
	cf = (int)(100. * 30000. * 30000. / ((double)n * n)) + 1;
    for (i = 0; i < n; i++) {
	int k;

	if (N_threads == 1) {
	    Vv_add (n, one, x, alfa, w, x);
	    Vv_add (n, one, r, -alfa, z, r);
	    if (V_norm (n, r) < test)
		break;
	    beta = Vv_dot_mul (n, r, z) / d;
	}
	else {
	    Spmcg_t norm;
	    Talfa = alfa;
	    T_sec = 1;
	    Cg_process ();
    
	    norm = 0.;
	    for (k = 0; k < N_threads; k++)
		norm += Tnorms[k];
	    norm = sqrt (norm);
	    if (norm < test)
		break;
	    beta = 0.;
	    for (k = 0; k < N_threads; k++)
		beta += Tbetas[k];
	    beta /= d;
	}

	Vv_add (n, -one, r, beta, w, w);

	if (N_threads == 1) {
	    Mv_multiply (n, a, w, z);
	    if ((d = Vv_dot_mul (n, w, z)) == zero) {
		VDD_log ("Singular system - Cg_solve\n");
		return (-3);
	    }
	    alfa = Vv_dot_mul (n, r, w) / d;
	}
	else {
	    T_sec = 2;
	    Cg_process ();
    
	    d = 0.;
	    for (k = 0; k < N_threads; k++)
		d += Tds[k];
	    if ((d = Vv_dot_mul (n, w, z)) == zero) {
		VDD_log ("Singular system - Cg_solve\n");
		return (-3);
	    }
	    alfa = 0.;
	    for (k = 0; k < N_threads; k++)
		alfa += Talfas[k];
	    alfa /= d;
	}
	cnt++;
	if ((cnt % cf) == 0) {
	    if (Time_out_secs > 0 && MISC_systime (NULL) > Time_out_start + Time_out_secs) {
		VDD_log ("User timeout (solution not reached) - Cg_solve\n");
		return (-1);
	    }
	}
    }
    return (cnt);
}

/**************************************************************************

    Performs a section (T_sec == 1 or 2) of CG solving.

**************************************************************************/

static void Cg_proc_sec (int t_ind) {
    int i, j, st, end;
    int *ne, *cind;
    Spmcg_t *ev, s, d;

    st = T_st_ind[t_ind];
    end = T_n[t_ind];

    if (T_sec == 1) {
	Spmcg_t norm, beta, t;
	norm = beta = 0.;
	for (i = st; i < end; i++) {
	    Tx[i] = Tx[i] + Talfa * Tw[i];
	    t = Tr[i] - Talfa * Tz[i];
	    Tr[i] = t;
	    norm += t * t;
	    beta += t * Tz[i];
	}
	Tnorms[t_ind] = norm;
	Tbetas[t_ind] = beta;
	return;
    }

    ne = Ta->ne;
    cind = Ta->c_ind + T_cind_st[t_ind];
    ev = Ta->ev + T_cind_st[t_ind];
    for (i = st; i < end; i++) {
	s = 0.;
	for (j = 0; j < ne[i]; j++) {
	    s += *ev * Tw[*cind];
	    ev++;
	    cind++;
	}
	Tz[i] = s;
    }

    d = 0.;
    s = 0.;
    for (i = st; i < end; i++) {
	Spmcg_t t;
	t = Tw[i];
	d += t * Tz[i];
	s += Tr[i] * t;
    }
    Tds[t_ind] = d;
    Talfas[t_ind] = s;
}

/**************************************************************************

    Computes a section of CG processing - The main thread.

**************************************************************************/

static void Cg_process () {

    if (N_threads > 1) {
	pthread_mutex_lock (&Sync_mutex);
	while (Sync_cnt < N_threads - 1)
	    pthread_cond_wait (&Sync_end, &Sync_mutex);
	pthread_cond_broadcast (&Sync_st);
	Sync_cnt = 0;
	pthread_mutex_unlock (&Sync_mutex);
    }

    Cg_proc_sec (0);

    if (N_threads > 1) {
	pthread_mutex_lock (&Sync_mutex);
	while (Sync_cnt < N_threads - 1)
	    pthread_cond_wait (&Sync_end, &Sync_mutex);
	pthread_mutex_unlock (&Sync_mutex);
    }
}

/**************************************************************************

    Parallel-processing threads for Cg_process.

**************************************************************************/

static void *T_cg_process (void *arg) {
    int t_ind;

    t_ind = (int)arg;

    while (1) {

	pthread_mutex_lock (&Sync_mutex);
	Sync_cnt++;
	if (Sync_cnt == N_threads - 1)
	    pthread_cond_signal (&Sync_end);
	pthread_cond_wait (&Sync_st, &Sync_mutex);
	pthread_mutex_unlock (&Sync_mutex);

	Cg_proc_sec (t_ind);
    }
}

/**************************************************************************

    Computes t = a * x where a is n-by-n sparse matrix, x and t are 
    n-vectors.

**************************************************************************/

static void Mv_multiply (int n, Sp_matrix *a, Spmcg_t *x, Spmcg_t *y) {
    int i, j;
    int *ne, *cind;
    Spmcg_t *ev;

    ne = a->ne;
    cind = a->c_ind;
    ev = a->ev;
    for (i = 0; i < n; i++) {
	Spmcg_t s = 0.;
	for (j = 0; j < ne[i]; j++) {
	    s += *ev * x[*cind];
	    ev++;
	    cind++;
	}
	y[i] = s;
    }
}

/**************************************************************************

    Computes the norm of n-vector u.

**************************************************************************/

static Spmcg_t V_norm (int n, Spmcg_t *u) {
    int i;
    Spmcg_t s;

    s = 0.;
    for (i = 0; i < n; i++)
	s += u[i] * u[i];
    return (sqrt (s));
}

/**************************************************************************

    Computes the dot multiplcation of two n-vectos of u and v.

**************************************************************************/

static Spmcg_t Vv_dot_mul (int n, Spmcg_t *u, Spmcg_t *v) {
    int i;
    Spmcg_t s;

    s = 0.;
    for (i = 0; i < n; i++)
	s += u[i] * v[i];
    return (s);
}

/**************************************************************************

    Computes out = su * u + sv * v where u, v, out are n-vector and su
    and sv are scaling factors.

**************************************************************************/

static void Vv_add (int n, Spmcg_t su, Spmcg_t *u, 
				Spmcg_t sv, Spmcg_t *v, Spmcg_t *out) {
    int i;
    Spmcg_t test;

    test = 1.;
    if (su == test) {
	for (i = 0; i < n; i++)
	    out[i] = sv * v[i] + u[i];
    }
    else if (su == -test) {
	for (i = 0; i < n; i++)
	    out[i] = sv * v[i] - u[i];
    }
    else {
	for (i = 0; i < n; i++)
	    out[i] = sv * v[i] + su * u[i];
    }
}




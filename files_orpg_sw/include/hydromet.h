/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2004/10/15 14:09:04 $
 * $Id: hydromet.h,v 1.7 2004/10/15 14:09:04 ryans Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */
/*	This header file defines the adaptation data structure for the	*
 *	hydromet subseries algorithms.  It corresponds to common blocks	*
 *	HYDROMET_PREP, HYDROMET_RATE, HYDROMET_ACC, HYDROMET_ADJ  in	*
 *	legacy code.							*/


#ifndef HYDROMET_H
#define	HYDROMET_H

#include <orpgctype.h>
#include <hydromet_prep.h>
#include <hydromet_rate.h>
#include <hydromet_acc.h>
#include <hydromet_adj.h>


/*	Hydromet (all components)					*/

#ifdef __cplusplus
class Hydromet:  public OCPropertyObject
{
    protected:
	HydroPreprocessing preprocessing;
	HydroRate rate;
	HydroAccumulation accumulation;
	HydroAdjustment adjustment;

    public:
	Hydromet();
	Hydromet(const Hydromet& accum_obj);
	virtual OCObject* Clone() const;
	virtual void Assign(const OCObject& obj);
	virtual ~Hydromet() {};

	DECLARE_DICTIONARY_CLASS(Hydromet)
	IMPLEMENT_SIMPLE_REFCOUNT
};
#endif

typedef	struct	{

	hydromet_prep_t	prep;	/*  Preprocessing data			*/
	hydromet_rate_t	rate;	/*  Rate data				*/
	hydromet_acc_t	acc;	/*  Accumulation data			*/
	hydromet_adj_t	adj;	/*  Adjustment data			*/

} hydromet_t;

#endif

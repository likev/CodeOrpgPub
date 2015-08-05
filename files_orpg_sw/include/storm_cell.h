/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2003/12/10 23:19:14 $
 * $Id: storm_cell.h,v 1.8 2003/12/10 23:19:14 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

/*	This header file defines the structure for the storm cell	*
 *	Id and tracking algorithms.  It corresponds to common blocks	*
 *	STORM_CELL_SEG, STORM_CELL_COMP, STORM_CELL_TRACK in the	*
 *	 legacy code.							*/


#ifndef STORM_CELL_H
#define	STORM_CELL_H

#include <storm_cell_seg.h>
#include <storm_cell_component.h>
#include <storm_cell_track.h>

#ifdef __cplusplus
class StormCell:  public OCPropertyObject
{
    protected:
	StormCellSegments segments;		
	StormCellComponents components;		
	StormCellTracking tracking;
	
    public:
	StormCell();
	StormCell(const StormCell& accum_obj);
	virtual OCObject* Clone() const;
	virtual void Assign(const OCObject& obj);
	virtual ~StormCell() {};

	DECLARE_DICTIONARY_CLASS(StormCell)
	IMPLEMENT_SIMPLE_REFCOUNT
};
#endif

/*	Storm Cell Data (all components)		*/

typedef	struct {

	storm_cell_seg_t	seg;
	storm_cell_comp_t	comp;
	storm_cell_track_t	track;

} storm_cell_t;

#endif

/*   Implement a catch-throw-like error mechanism for gtt
 *   Copyright (C) 2001 Linas Vepstas
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <assert.h>

#include "err-throw.h"
 

static GttErrCode err = GTT_NO_ERR;

void 
gtt_err_set_code (GttErrCode code)
{
	/* clear the error if requested. */
	if (GTT_NO_ERR == code) 
	{
		err = GTT_NO_ERR;
		return;
	}
	
	/* if the error code is already set, don't over-write it */
	if (GTT_NO_ERR != err) return;

	/* Otherwise set it. */
	err = code;
}

GttErrCode 
gtt_err_get_code (void)
{
	return err;
}

/* =========================== END OF FILE ======================== */

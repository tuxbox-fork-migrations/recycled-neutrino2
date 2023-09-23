/*
 * $Id: scan.h 23.09.2023 mohousch Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __scan_h__
#define __scan_h__

#include <linux/dvb/frontend.h>

#include <inttypes.h>

#include <map>
#include <string>


class CScan
{
	public:
		uint32_t  actual_freq;
		uint32_t actual_polarisation;
		
	private:
		CScan(){actual_freq = 0; actual_polarisation = 0;};
		~CScan(){};
		
	public:
		//
		static CScan *getInstance()
		{
			static CScan * scan = NULL;

			if(!scan) 
			{
				scan = new CScan();
			} 

			return scan;
		};
		
		//
		bool tuneFrequency(FrontendParameters *feparams, t_satellite_position satellitePosition, int feindex);
		int addToScan(transponder_id_t TsidOnid, FrontendParameters *feparams, bool fromnit = 0, int feindex = 0);
		bool getSDTS(t_satellite_position satellitePosition, int feindex);
		bool scanTransponder(xmlNodePtr transponder, uint8_t diseqc_pos, t_satellite_position satellitePosition, int feindex);
		bool scanProvider(xmlNodePtr search, t_satellite_position satellitePosition, uint8_t diseqc_pos, bool satfeed, int feindex);
};

#endif /* __scan_h__ */


/*
  Neutrino-GUI  -   DBoxII-Project

  Copyright (C) 2001 Steffen Hehn 'McClean'
  Homepage: http://dbox.cyberphoria.org/

  License: GPL

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __AUDIOFILE_H__
#define __AUDIOFILE_H__

#include <string>
#include <vector>

#include <driver/audiometadata.h>
#include <driver/file.h>


class CAudiofile
{
	public:
		/* constructors */
		CAudiofile();
		CAudiofile( std::string name, CFile::FileExtension extension );
		CAudiofile( const CAudiofile& src );

		void operator=( const CAudiofile& src );
		void clear();

		CAudioMetaData MetaData;
		std::string Filename;
		CFile::FileExtension FileExtension;
};

typedef std::vector<CAudiofile> CAudioPlayList;

#endif /* __AUDIOFILE_H__ */


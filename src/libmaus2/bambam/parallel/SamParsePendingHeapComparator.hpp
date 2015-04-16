/*
    libmaus2
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_SAMPARSEPENDINGHEAPCOMPARATOR_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_SAMPARSEPENDINGHEAPCOMPARATOR_HPP

#include <libmaus2/bambam/parallel/SamParsePending.hpp>
			
namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct SamParsePendingHeapComparator
			{
				bool operator()(SamParsePending const & A, SamParsePending const & B) const
				{
					if ( A.block->meta.blockid != B.block->meta.blockid )
						return A.block->meta.blockid > B.block->meta.blockid;
					else
						return A.subid > B.subid;
				}
			};
		}
	}
}
#endif

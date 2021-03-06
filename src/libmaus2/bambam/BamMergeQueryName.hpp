/*
    libmaus2
    Copyright (C) 2009-2013 German Tischler
    Copyright (C) 2011-2013 Genome Research Limited

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
#if ! defined(LIBMAUS2_BAMBAM_BAMMERGEQUERYNAME_HPP)
#define LIBMAUS2_BAMBAM_BAMMERGEQUERYNAME_HPP

#include <libmaus2/bambam/BamCatHeader.hpp>
#include <libmaus2/bambam/BamMergeTemplate.hpp>
#include <libmaus2/bambam/BamAlignmentNameComparator.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct BamMergeQueyNameHeapComparator
		{
			libmaus2::bambam::BamAlignment ** algns;

			BamMergeQueyNameHeapComparator(libmaus2::bambam::BamAlignment ** ralgns) : algns(ralgns) {}

			bool operator()(uint64_t const a, uint64_t const b) const
			{
				return libmaus2::bambam::BamAlignmentNameComparator::compareInt(*algns[a],*algns[b]) > 0;
			}
		};

		typedef BamMergeTemplate<BamMergeQueyNameHeapComparator,BamCatHeader::IsQueryNameSorted> BamMergeQueryName;

		struct BamMergeQueryNameWrapper
		{
			BamMergeQueryName object;

			BamMergeQueryNameWrapper(std::vector<std::string> const & filenames, bool const putrank = false)
			: object(filenames,putrank) {}
			BamMergeQueryNameWrapper(libmaus2::util::ArgInfo const & arginfo, std::vector<std::string> const & filenames, bool const putrank = false)
			: object(arginfo,filenames,putrank) {}
		};
	}
}
#endif

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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_FILECHECKSUMBLOCKWORKPACKAGE_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_FILECHECKSUMBLOCKWORKPACKAGE_HPP

#include <libmaus2/parallel/SimpleThreadWorkPackage.hpp>
#include <libmaus2/bambam/parallel/GenericInputControlCompressionPending.hpp>
#include <libmaus2/digest/DigestInterface.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct FileChecksumBlockWorkPackage : public libmaus2::parallel::SimpleThreadWorkPackage
			{
				typedef FileChecksumBlockWorkPackage this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				libmaus2::bambam::parallel::GenericInputControlCompressionPending GICCP;
				libmaus2::digest::DigestInterface * checksum;

				FileChecksumBlockWorkPackage() : libmaus2::parallel::SimpleThreadWorkPackage(), GICCP(), checksum(0) {}
				FileChecksumBlockWorkPackage(
					uint64_t const rpriority,
					uint64_t const rdispatcherid,
					libmaus2::bambam::parallel::GenericInputControlCompressionPending rGICCP,
					libmaus2::digest::DigestInterface * rchecksum
				)
				: libmaus2::parallel::SimpleThreadWorkPackage(rpriority,rdispatcherid), GICCP(rGICCP), checksum(rchecksum)
				{
				}

				virtual char const * getPackageName() const { return "FileChecksumBlockWorkPackage"; }
			};
		}
	}
}
#endif

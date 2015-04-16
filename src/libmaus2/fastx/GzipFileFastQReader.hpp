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

#if ! defined(LIBMAUS_FASTX_GZIPFILEFASTQREADER_HPP)
#define LIBMAUS_FASTX_GZIPFILEFASTQREADER_HPP

#include <libmaus2/fastx/GzipStreamFastQReader.hpp>
#include <libmaus2/aio/CheckedInputStreamAtOffset.hpp>

namespace libmaus2
{
	namespace fastx
	{
		struct GzipFileFastQReader : public libmaus2::aio::CheckedInputStreamAtOffsetWrapper, GzipStreamFastQReader
		{
                	typedef GzipFileFastQReader this_type;
                	typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			GzipFileFastQReader(std::string const & filename)
			: libmaus2::aio::CheckedInputStreamAtOffsetWrapper(filename), GzipStreamFastQReader(libmaus2::aio::CheckedInputStreamAtOffsetWrapper::object)
			{
			
			}
			GzipFileFastQReader(std::string const & filename, int const qualityOffset)
			: libmaus2::aio::CheckedInputStreamAtOffsetWrapper(filename), GzipStreamFastQReader(libmaus2::aio::CheckedInputStreamAtOffsetWrapper::object,qualityOffset)
			{
			
			}
			GzipFileFastQReader(std::string const & filename, libmaus2::fastx::FastInterval const & FI)
			: libmaus2::aio::CheckedInputStreamAtOffsetWrapper(filename,FI.fileoffset), GzipStreamFastQReader(libmaus2::aio::CheckedInputStreamAtOffsetWrapper::object,FI)
			{
			
			}
			GzipFileFastQReader(std::string const & filename, int const qualityOffset, libmaus2::fastx::FastInterval const & FI)
			: libmaus2::aio::CheckedInputStreamAtOffsetWrapper(filename,FI.fileoffset), GzipStreamFastQReader(libmaus2::aio::CheckedInputStreamAtOffsetWrapper::object,qualityOffset,FI)
			{
			
			}
		};
	}
}
#endif

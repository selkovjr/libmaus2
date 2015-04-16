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

#if ! defined(LIBMAUS_UTIL_UTF8DECODERBUFFER_HPP)
#define LIBMAUS_UTIL_UTF8DECODERBUFFER_HPP

#include <istream>
#include <fstream>
#include <ios>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/util/GetFileSize.hpp>
#include <libmaus2/aio/CheckedInputStream.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <libmaus2/bitio/Ctz.hpp>
#include <libmaus2/bitio/CompactArray.hpp>
#include <libmaus2/bitio/ArrayDecode.hpp>
#include <libmaus2/util/Utf8BlockIndex.hpp>

namespace libmaus2
{
	namespace util
	{
		struct Utf8DecoderBuffer : public ::std::wstreambuf
		{
			protected:
			typedef ::std::wstreambuf::char_type char_type;
			
			static uint64_t const headersize = 4*sizeof(uint64_t);
			
			::libmaus2::util::Utf8BlockIndexDecoder indexdecoder;
			uint64_t const blocksize;
                        uint64_t const lastblocksize;
                        uint64_t const maxblockbytes;
                        uint64_t const numblocks;
                        
			::libmaus2::aio::CheckedInputStream stream;
			uint64_t const n;
			uint64_t const buffersize;

			::libmaus2::autoarray::AutoArray<uint8_t> inbuffer;
			::libmaus2::autoarray::AutoArray<wchar_t> buffer;
			
			uint64_t symsread;

			Utf8DecoderBuffer(Utf8DecoderBuffer const &);
			Utf8DecoderBuffer & operator=(Utf8DecoderBuffer&);
			
			public:
			Utf8DecoderBuffer(
				std::string const & filename, 
				::std::size_t rbuffersize
			);

			private:
			::std::streampos seekpos(::std::streampos sp, ::std::ios_base::openmode which = ::std::ios_base::in | ::std::ios_base::out);
			::std::streampos seekoff(::std::streamoff off, ::std::ios_base::seekdir way, ::std::ios_base::openmode which = ::std::ios_base::in | ::std::ios_base::out);
			int_type underflow();
		};
	}
}
#endif

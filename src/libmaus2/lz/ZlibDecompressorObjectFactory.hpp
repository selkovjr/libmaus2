/*
    libmaus2
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS2_LZ_ZLIBDECOMPRESSOROBJECTFACTORY_HPP)
#define LIBMAUS2_LZ_ZLIBDECOMPRESSOROBJECTFACTORY_HPP

#include <libmaus2/lz/DecompressorObjectFactory.hpp>
#include <libmaus2/lz/ZlibDecompressorObject.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct ZlibDecompressorObjectFactory : public DecompressorObjectFactory
		{
			typedef ZlibDecompressorObjectFactory this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			
			virtual ~ZlibDecompressorObjectFactory() {}
			virtual DecompressorObject::unique_ptr_type operator()()
			{
				DecompressorObject::unique_ptr_type ptr(new ZlibDecompressorObject);
				return UNIQUE_PTR_MOVE(ptr);
			}
			virtual std::string getDescription() const
			{
				return "ZlibDecompressorObjectFactory";
			}
		};
	}
}
#endif

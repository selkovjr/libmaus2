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
#if ! defined(LIBMAUS2_GAMMA_GAMMARLENCODER_HPP)
#define LIBMAUS2_GAMMA_GAMMARLENCODER_HPP

#include <libmaus2/aio/CheckedOutputStream.hpp>
#include <libmaus2/gamma/GammaEncoder.hpp>
#include <libmaus2/huffman/IndexEntry.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <libmaus2/aio/SynchronousGenericOutput.hpp>
#include <libmaus2/bitio/FastWriteBitWriter.hpp>
#include <libmaus2/math/bitsPerNum.hpp>
#include <libmaus2/gamma/GammaRLDecoder.hpp>

namespace libmaus2
{
	namespace gamma
	{
		struct GammaRLEncoder
		{
			typedef GammaRLEncoder this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::aio::SynchronousGenericOutput<uint64_t> sgo_type;
			
			uint64_t const blocksize;
			
			::libmaus2::aio::CheckedOutputStream COS;
			sgo_type SGO;
			::libmaus2::gamma::GammaEncoder < sgo_type > GE;
						
			::std::vector< ::libmaus2::huffman::IndexEntry > index;
			typedef std::pair<int64_t,uint64_t> ptype;
			::libmaus2::autoarray::AutoArray< ptype > A;
			ptype * const pa;
			ptype * pc;
			ptype * const pe;

			uint64_t cursym;
			uint64_t curcnt;
			
			bool indexwritten;
			
			unsigned int const albits;

			
			GammaRLEncoder(std::string const & filename, unsigned int const ralbits, uint64_t const n, uint64_t const rblocksize, uint64_t const rbufsize = 64*1024)
			: 
			  blocksize(rblocksize),
			  COS(filename), SGO(COS,rbufsize), GE(SGO), 
			  A(blocksize), pa(A.begin()), pc(pa), pe(A.end()), 
			  cursym(0), curcnt(0), indexwritten(false), albits(ralbits)
			{
				SGO.put(n);
				SGO.put(albits);
			}
			~GammaRLEncoder()
			{
				flush();
			}
			
			void implicitFlush()
			{
				uint64_t const bs = pc-pa;
				
				if ( bs )
				{
					uint64_t acc = 0;
					uint64_t const pos = SGO.getWrittenBytes();

					GE.encode(bs);
					for ( uint64_t i = 0; i < bs; ++i )
					{
						acc += pa[i].second;
						GE.encodeWord(pa[i].first,albits);
						GE.encode(pa[i].second);
					}
					GE.flush();
					
					::libmaus2::huffman::IndexEntry const entry(pos,bs,acc);
					index.push_back(entry);
					
					pc = pa;
				}
			}
			
			void encode(uint64_t const sym)
			{
				if ( sym == cursym )
				{
					curcnt++;
				}
				else if ( curcnt )
				{
					*(pc++) = ptype(cursym,curcnt);
					
					if ( pc == pe )
						implicitFlush();
					
					cursym = sym;
					curcnt = 1;
				}
				else
				{
					assert ( sym != cursym );
					cursym = sym;
					curcnt = 1;
				}
			}
			
			template<typename iterator>
			void encode(iterator a, iterator e)
			{
				for ( ; a != e; ++a )
					encode(*a);
			}

			void flush()
			{
				if ( curcnt )
				{
					*(pc++) = ptype(cursym,curcnt);
					implicitFlush();
					curcnt = 0;
				}

				SGO.flush();

				uint64_t const indexpos = SGO.getWrittenBytes();
				writeIndex(indexpos);				
			}
			
			static void writeIndex(
				::std::vector< ::libmaus2::huffman::IndexEntry > const & index,
				::libmaus2::bitio::FastWriteBitWriterStream8Std & gapHEF, 
				uint64_t const indexpos
			)
			{
				uint64_t const maxpos = index.size() ? index[index.size()-1].pos : 0;
				unsigned int const posbits = ::libmaus2::math::bitsPerNum(maxpos);
				
				uint64_t const kacc = std::accumulate(index.begin(),index.end(),0ull,::libmaus2::huffman::IndexEntryKeyAdd());
				unsigned int const kbits = ::libmaus2::math::bitsPerNum(kacc);

				uint64_t const vacc = std::accumulate(index.begin(),index.end(),0ull,::libmaus2::huffman::IndexEntryValueAdd());
				unsigned int const vbits = ::libmaus2::math::bitsPerNum(vacc);

				// write number of entries in index
				gapHEF.writeElias2(index.size());
				// write number of bits per file position
				gapHEF.writeElias2(posbits);
				
				// write number of bits per sym acc
				gapHEF.writeElias2(kbits);
				// write symacc
				gapHEF.writeElias2(kacc);

				// write number of bits per sym acc
				gapHEF.writeElias2(vbits);
				// write symacc
				gapHEF.writeElias2(vacc);
				
				// align
				gapHEF.flush();
				
				uint64_t tkacc = 0, tvacc = 0;
				for ( uint64_t i = 0; i < index.size(); ++i )
				{
					gapHEF.write(index[i].pos,posbits); // position of block
					gapHEF.write(tkacc,kbits); // sum of values inblock
					gapHEF.write(tvacc,vbits); // sum of values inblock
					tkacc += index[i].kcnt;
					tvacc += index[i].vcnt;
				}
				gapHEF.write(0,posbits); // position of block
				gapHEF.write(tkacc,kbits); // sum of values inblock
				gapHEF.write(tvacc,vbits); // sum of values inblock
				gapHEF.flush();
			
				// write position of index in last 64 bits of file	
				for ( uint64_t i = 0; i < 64; ++i )
					gapHEF.writeBit( (indexpos & (1ull<<(63-i))) != 0 );

				gapHEF.flush();				
			}

			void writeIndex(uint64_t const indexpos)
			{
				if ( ! indexwritten )
				{
					::libmaus2::aio::SynchronousGenericOutput<uint8_t> SGO(COS,64*1024);
					::libmaus2::aio::SynchronousGenericOutput<uint8_t>::iterator_type it(SGO);
					::libmaus2::bitio::FastWriteBitWriterStream8Std FWBWS(it);
					writeIndex(index,FWBWS,indexpos);

					FWBWS.flush();
					SGO.flush();
					COS.flush();
					
					indexwritten = true;
				}
			}
			
			static void concatenate(std::vector<std::string> const & infilenames, std::string const & outfilename, bool const removeinput = false)
			{
				uint64_t const n = ::libmaus2::gamma::GammaRLDecoder::getLength(infilenames);
				unsigned int const albits = infilenames.size() ? ::libmaus2::gamma::GammaRLDecoder::getAlBits(infilenames[0]) : 0;
				
				::libmaus2::aio::CheckedOutputStream COS(outfilename);
				::libmaus2::aio::SynchronousGenericOutput<uint64_t> SGO(COS,64);
				SGO.put(n);
				SGO.put(albits);
				SGO.flush();
				uint64_t const headerlen = 2*sizeof(uint64_t);
				
				std::vector < ::libmaus2::huffman::IndexEntry > index;
				uint64_t ioff = headerlen;
				
				for ( uint64_t i = 0; i < infilenames.size(); ++i )
				{
					uint64_t const indexpos = ::libmaus2::huffman::IndexLoaderBase::getIndexPos(infilenames[i]);
					uint64_t const datalen = indexpos-headerlen;
					
					// copy data
					::libmaus2::aio::CheckedInputStream CIS(infilenames[i]);
					CIS.seekg(headerlen);
					::libmaus2::util::GetFileSize::copy(CIS,COS,datalen);
					
					// add entries to index
					::libmaus2::huffman::IndexLoaderSequential indexdata(infilenames[i]);
					::libmaus2::huffman::IndexEntry ij = indexdata.getNext();
					
					// ::libmaus2::huffman::IndexDecoderData indexdata(infilenames[i]);
					for ( uint64_t j = 0; j < indexdata.numentries; ++j )
					{
						::libmaus2::huffman::IndexEntry ij1 = indexdata.getNext();
						/*
						::libmaus2::huffman::IndexEntry const ij  = indexdata.readEntry(j);
						::libmaus2::huffman::IndexEntry const ij1 = indexdata.readEntry(j+1);						
						*/
						index.push_back(::libmaus2::huffman::IndexEntry((ij.pos - headerlen) + ioff, ij1.kcnt - ij.kcnt, ij1.vcnt - ij.vcnt));
						
						ij = ij1;
					}
					
					// update position pointer
					ioff += datalen;
					
					if ( removeinput )
						remove(infilenames[i].c_str());
				}

				// write index
				::libmaus2::aio::SynchronousGenericOutput<uint8_t> SGO8(COS,64*1024);
				::libmaus2::aio::SynchronousGenericOutput<uint8_t>::iterator_type it(SGO8);
				::libmaus2::bitio::FastWriteBitWriterStream8Std FWBWS(it);
				writeIndex(index,FWBWS,ioff);

				FWBWS.flush();
				SGO8.flush();
				COS.flush();
			}
		};
	}
}
#endif

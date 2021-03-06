/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_HUFFMAN_LFRANKPOSDECODER_HPP)
#define LIBMAUS2_HUFFMAN_LFRANKPOSDECODER_HPP

#include <libmaus2/huffman/LFSupportBitDecoder.hpp>
#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/aio/FileRemoval.hpp>
#include <libmaus2/aio/SynchronousGenericInput.hpp>
#include <libmaus2/gamma/GammaDecoder.hpp>
#include <libmaus2/util/PrefixSums.hpp>
#include <libmaus2/util/iterator.hpp>
#include <libmaus2/gamma/GammaPDIndexDecoder.hpp>
#include <libmaus2/huffman/LFRankPos.hpp>
#include <libmaus2/math/lowbits.hpp>
#include <libmaus2/bitio/readElias.hpp>
#include <libmaus2/huffman/CanonicalEncoder.hpp>
#include <libmaus2/util/Histogram.hpp>
#include <libmaus2/util/GetFileSize.hpp>

namespace libmaus2
{
	namespace huffman
	{
		struct LFRankPosIndexDecoder
		{
			struct FileBlockOffset
			{
				uint64_t file;
				uint64_t block;
				uint64_t blockoffset;
				uint64_t offset;

				FileBlockOffset(
					uint64_t const rfile = 0,
					uint64_t const rblock = 0,
					uint64_t const rblockoffset = 0,
					uint64_t const roffset = 0
				) : file(rfile), block(rblock), blockoffset(rblockoffset), offset(roffset) {}
			};

			static uint64_t getIndexOffset(::std::istream & in)
			{
				in.clear();
				in.seekg(-8,std::ios_base::end);
				uint64_t const offset = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				return offset;
			}

			static uint64_t getWordsPerEntry()
			{
				return 4ull;
			}

			static uint64_t getBytesPerEntry()
			{
				return getWordsPerEntry()*sizeof(uint64_t);
			}

			static uint64_t getNumIndexEntries(::std::istream & in)
			{
				uint64_t const indexpos = getIndexOffset(in);
				uint64_t const fs = libmaus2::util::GetFileSize::getFileSize(in);
				assert ( indexpos <= fs );
				uint64_t const sub = fs - indexpos - sizeof(uint64_t);
				assert ( sub % getBytesPerEntry() == 0 );
				return sub / getBytesPerEntry();
			}

			static uint64_t getNumBlocks(::std::istream & in)
			{
				uint64_t const numindexentries = getNumIndexEntries(in);
				assert ( numindexentries );
				return numindexentries - 1;
			}

			std::vector<std::string> Vfn;
			std::vector<uint64_t> valuesPerFile;
			std::vector<uint64_t> blocksPerFile;
			std::vector<uint64_t> indexEntriesPerFile;
			std::vector<uint64_t> indexOffset;
			std::vector< std::pair<uint64_t,uint64_t> > Vranklowhigh;

			uint64_t getLowRank() const
			{
				if ( Vranklowhigh.size() )
					return Vranklowhigh.front().first;
				else
					return 0;
			}
			uint64_t getHighRank() const
			{
				if ( Vranklowhigh.size() )
					return Vranklowhigh.back().second;
				else
					return 0;
			}

			struct IndexEntry
			{
				uint64_t offset;
				uint64_t valuesperblock;
				uint64_t ranklow;
				uint64_t rankhigh;

				IndexEntry() {}
				IndexEntry(
					uint64_t const roffset,
					uint64_t const rvaluesperblock,
					uint64_t const rranklow,
					uint64_t const rrankhigh
				) : offset(roffset), valuesperblock(rvaluesperblock), ranklow(rranklow), rankhigh(rrankhigh)
				{

				}
			};

			struct IndexAccessor
			{
				typedef libmaus2::util::ConstIterator< IndexAccessor, IndexEntry > const_iterator;

				mutable libmaus2::aio::InputStreamInstance ISI;
				uint64_t const indexoffset;
				uint64_t const indexentries;

				IndexAccessor(std::string const & fn, uint64_t const rindexoffset, uint64_t const rindexentries)
				: ISI(fn), indexoffset(rindexoffset), indexentries(rindexentries)
				{

				}

				static IndexEntry get(std::istream & ISI, uint64_t const indexoffset, uint64_t const i)
				{
					ISI.clear();
					ISI.seekg(indexoffset + i*getBytesPerEntry());
					IndexEntry E;
					E.offset = libmaus2::util::NumberSerialisation::deserialiseNumber(ISI);
					E.valuesperblock = libmaus2::util::NumberSerialisation::deserialiseNumber(ISI);
					E.ranklow = libmaus2::util::NumberSerialisation::deserialiseNumber(ISI);
					E.rankhigh = libmaus2::util::NumberSerialisation::deserialiseNumber(ISI);
					return E;
				}

				IndexEntry get(uint64_t const i) const
				{
					return get(ISI,indexoffset,i);
				}

				IndexEntry operator[](uint64_t const i) const
				{
					return get(i);
				}

				const_iterator begin() const
				{
					return const_iterator(this,0);
				}

				const_iterator end() const
				{
					return const_iterator(this,indexentries);
				}
			};

			struct IndexEntryValuesPerBlockComp
			{
				bool operator()(IndexEntry const & A, IndexEntry const & B) const
				{
					return A.valuesperblock < B.valuesperblock;
				}
			};

			struct IndexEntryRankHighComp
			{
				bool operator()(IndexEntry const & A, IndexEntry const & B) const
				{
					return A.rankhigh < B.rankhigh;
				}
			};

			static uint64_t getNumValues(std::istream & in)
			{
				in.clear();
				in.seekg(-4*static_cast<int64_t>(sizeof(uint64_t)),std::ios_base::end);
				return libmaus2::util::NumberSerialisation::deserialiseNumber(in);
			}

			static uint64_t getNumValues(std::string const & fn)
			{
				libmaus2::aio::InputStreamInstance ISI(fn);
				return getNumValues(ISI);
			}

			LFRankPosIndexDecoder(std::vector<std::string> const & rVfn)
			: Vfn(rVfn), valuesPerFile(0), blocksPerFile(0), indexEntriesPerFile(0)
			{
				uint64_t o = 0;
				for ( uint64_t i = 0; i < Vfn.size(); ++i )
				{
					libmaus2::aio::InputStreamInstance ISI(Vfn[i]);
					uint64_t const vpf = getNumValues(ISI);

					if ( vpf )
					{
						valuesPerFile.push_back(vpf);
						blocksPerFile.push_back(getNumBlocks(ISI));
						indexEntriesPerFile.push_back(blocksPerFile.back()+1);
						indexOffset.push_back(getIndexOffset(ISI));
						Vfn[o++] = Vfn[i];

						IndexEntry const firstentry = IndexAccessor::get(ISI,indexOffset.back(),0);
						IndexEntry const lastentry = IndexAccessor::get(ISI,indexOffset.back(),blocksPerFile.back()-1);
						Vranklowhigh.push_back(std::pair<uint64_t,uint64_t>(firstentry.ranklow,lastentry.rankhigh));
					}
				}
				// for prefix sum
				valuesPerFile.push_back(0);
				Vfn.resize(o);

				libmaus2::util::PrefixSums::prefixSums(valuesPerFile.begin(),valuesPerFile.end());
			}

			uint64_t size() const
			{
				return valuesPerFile.back();
			}

			struct PairSecondComp
			{
				bool operator()(std::pair<uint64_t,uint64_t> const & A, std::pair<uint64_t,uint64_t> const & B) const
				{
					return A.second < B.second;
				}
			};

			FileBlockOffset lookupRank(uint64_t rank)
			{
				if ( ! Vranklowhigh.size() || rank > getHighRank() )
					return FileBlockOffset(Vfn.size(),0,0);

				std::vector<std::pair<uint64_t,uint64_t> >::const_iterator it = ::std::lower_bound(Vranklowhigh.begin(),Vranklowhigh.end(),std::pair<uint64_t,uint64_t>(0,rank),PairSecondComp());
				assert ( it != Vranklowhigh.end() );
				assert ( it->second >= rank );

				uint64_t const fileid = it - Vranklowhigh.begin();

				IndexAccessor IA(Vfn[fileid],indexOffset[fileid],indexEntriesPerFile[fileid]);
				IndexAccessor::const_iterator fit = std::lower_bound(IA.begin(),IA.end(),IndexEntry(0,0,0,rank),IndexEntryRankHighComp());
				assert ( fit != IA.end() );

				uint64_t const blockid = fit - IA.begin();
				IndexEntry E = IA.get(blockid);

				assert ( E.rankhigh >= rank );

				if ( blockid )
				{
					assert ( rank > IA.get(blockid-1).rankhigh );
				}
				else if ( fileid )
				{
					assert ( rank > Vranklowhigh[fileid-1].second );
				}

				return FileBlockOffset(fileid,blockid,E.offset);
			}

			FileBlockOffset lookup(uint64_t offset)
			{
				if ( offset >= valuesPerFile.back() )
					return FileBlockOffset(Vfn.size(),0,0);

				std::vector<uint64_t>::const_iterator it = ::std::lower_bound(valuesPerFile.begin(),valuesPerFile.end(),offset);
				assert ( it != valuesPerFile.end() );

				if ( *it != offset )
				{
					assert ( it != valuesPerFile.begin() );
					it -= 1;
				}

				assert ( offset >= *it );

				uint64_t const fileid = it - valuesPerFile.begin();
				offset -= *it;

				assert ( offset < valuesPerFile[fileid+1]-valuesPerFile[fileid] );

				IndexAccessor IA(Vfn[fileid],indexOffset[fileid],indexEntriesPerFile[fileid]);
				IndexAccessor::const_iterator fit = std::lower_bound(IA.begin(),IA.end(),IndexEntry(0,offset,0,0),IndexEntryValuesPerBlockComp());
				assert ( fit != IA.end() );

				if ( fit[0].valuesperblock != offset )
					fit -= 1;

				assert ( offset >= fit[0].valuesperblock );

				uint64_t const blockid = fit - IA.begin();
				uint64_t const blockoffset = fit[0].offset;

				offset -= fit[0].valuesperblock;

				// std::cerr << "fileid=" << fileid << " blockid=" << blockid << " blockoffset=" << blockoffset << " offset=" << offset << std::endl;

				return FileBlockOffset(fileid,blockid,blockoffset,offset);
			}
		};


		struct LFRankPosDecoder // : public libmaus2::gamma::GammaPDIndexDecoderBase
		{
			typedef LFRankPosDecoder this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			typedef LFRankPos value_type;

			libmaus2::huffman::LFRankPosIndexDecoder index;
			libmaus2::huffman::LFRankPosIndexDecoder::FileBlockOffset FBO;

			libmaus2::aio::InputStreamInstance::unique_ptr_type PISI;
			libmaus2::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type PSGI;

			libmaus2::autoarray::AutoArray<LFRankPos> B;

			LFRankPos * pa;
			LFRankPos * pc;
			LFRankPos * pe;

			libmaus2::autoarray::AutoArray<uint64_t> V;

			typedef std::pair<int64_t,uint64_t> rl_pair;
			::libmaus2::autoarray::AutoArray < rl_pair > rlbuffer;

			void openFile()
			{
				if ( FBO.file < index.Vfn.size() )
				{
					libmaus2::aio::InputStreamInstance::unique_ptr_type TISI(new libmaus2::aio::InputStreamInstance(index.Vfn[FBO.file]));
					PISI = UNIQUE_PTR_MOVE(TISI);
					PISI->clear();
					PISI->seekg(FBO.blockoffset);

					libmaus2::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type TSGI(new libmaus2::aio::SynchronousGenericInput<uint64_t>(*PISI,4096,std::numeric_limits<uint64_t>::max(),false /* check mod */));
					PSGI = UNIQUE_PTR_MOVE(TSGI);
				}
				else
				{
					PSGI.reset();
					PISI.reset();
				}
			}

			uint64_t decodeRL()
			{
				LFSSupportBitDecoder SBIS(*PSGI);

				// read block size
				uint64_t const bs = ::libmaus2::bitio::readElias2(SBIS);

				bool const cntescape = SBIS.readBit();

				// read huffman code maps
				::libmaus2::autoarray::AutoArray< std::pair<int64_t, uint64_t> > symmap = ::libmaus2::huffman::CanonicalEncoder::deserialise(SBIS);
				::libmaus2::autoarray::AutoArray< std::pair<int64_t, uint64_t> > cntmap = ::libmaus2::huffman::CanonicalEncoder::deserialise(SBIS);

				// construct decoder for symbols
				::libmaus2::huffman::CanonicalEncoder symdec(symmap);

				// construct decoder for runlengths
				::libmaus2::huffman::EscapeCanonicalEncoder::unique_ptr_type esccntdec;
				::libmaus2::huffman::CanonicalEncoder::unique_ptr_type cntdec;
				if ( cntescape )
				{
					::libmaus2::huffman::EscapeCanonicalEncoder::unique_ptr_type tesccntdec(new ::libmaus2::huffman::EscapeCanonicalEncoder(cntmap));
					esccntdec = UNIQUE_PTR_MOVE(tesccntdec);
				}
				else
				{
					::libmaus2::huffman::CanonicalEncoder::unique_ptr_type tcntdec(new ::libmaus2::huffman::CanonicalEncoder(cntmap));
					cntdec = UNIQUE_PTR_MOVE(tcntdec);
				}

				// increase buffersize if necessary
				rlbuffer.ensureSize(bs);

				// byte align input stream
				//SBIS.flush();

				// decode symbols
				for ( uint64_t i = 0; i < bs; ++i )
				{
					uint64_t const sym = symdec.fastDecode(SBIS);
					rlbuffer[i].first = sym;
				}

				// byte align
				// SBIS.flush();

				// decode runlengths
				if ( cntescape )
					for ( uint64_t i = 0; i < bs; ++i )
					{
						uint64_t const cnt = esccntdec->fastDecode(SBIS);
						rlbuffer[i].second = cnt;
					}
				else
					for ( uint64_t i = 0; i < bs; ++i )
					{
						uint64_t const cnt = cntdec->fastDecode(SBIS);
						rlbuffer[i].second = cnt;
					}

				// byte align
				SBIS.flush();

				return bs;
			}

			void decodeR(uint64_t const n)
			{
				#if 0
				libmaus2::gamma::GammaDecoder< libmaus2::aio::SynchronousGenericInput<uint64_t> > GD(*PSGI);

				for ( uint64_t i = 0; i < n; ++i )
					B[i].p = GD.decode();
				GD.flush();
				#endif

				for ( uint64_t i = 0; i < n; ++i )
					PSGI->getNext(B[i].r);
			}

			void decodeP(uint64_t const n)
			{
				#if 0
				libmaus2::gamma::GammaDecoder< libmaus2::aio::SynchronousGenericInput<uint64_t> > GD(*PSGI);

				for ( uint64_t i = 0; i < n; ++i )
					B[i].p = GD.decode();
				GD.flush();
				#endif

				for ( uint64_t i = 0; i < n; ++i )
					PSGI->getNext(B[i].p);
			}

			void decodeV(uint64_t const numobj)
			{
				uint64_t numv = 0;
				for ( uint64_t i = 0; i < numobj; ++i )
					numv += B[i].n;
				V.ensureSize(numv);

				libmaus2::gamma::GammaDecoder< libmaus2::aio::SynchronousGenericInput<uint64_t> > GD(*PSGI);
				numv = 0;
				for ( uint64_t i = 0; i < numobj; ++i )
				{
					B[i].v = V.begin() + numv;

					for ( uint64_t j = 0; j < B[i].n; ++j )
						V[numv++] = GD.decode();
				}
				GD.flush();
			}

			void decodeActive(uint64_t const numobj)
			{
				LFSSupportBitDecoder SBIS(*PSGI);

				for ( uint64_t i = 0; i < numobj; ++i )
					B[i].active = SBIS.readBit();

				SBIS.flush();
			}

			bool decodeBlock()
			{
				while ( FBO.file < index.Vfn.size() && FBO.block >= index.blocksPerFile[FBO.file] )
				{
					FBO.file++;
					FBO.block = 0;
					FBO.blockoffset = 0; // check this if we change the file format
					FBO.offset = 0;
					openFile();
				}
				if ( FBO.file == index.Vfn.size() )
				{
					PSGI.reset();
					PISI.reset();
					return false;
				}

				uint64_t numobj;
				PSGI->getNext(numobj);

				B.ensureSize(numobj);

				decodeR(numobj);
				decodeP(numobj);

				uint64_t const numnruns = decodeRL();
				uint64_t o = 0;
				for ( uint64_t i = 0; i < numnruns; ++i )
					for ( uint64_t j = 0; j < rlbuffer[i].second; ++j )
						B[o++].n = rlbuffer[i].first;

				decodeV(numobj);
				decodeActive(numobj);

				pa = B.begin();
				pc = B.begin();
				pe = B.begin()+numobj;

				FBO.block += 1;

				return true;
			}

			void init(uint64_t const offset)
			{
				FBO = index.lookup(offset);

				if ( FBO.file < index.Vfn.size() )
				{
					openFile();
					decodeBlock();

					LFRankPos v;
					while ( FBO.offset )
					{
						bool const ok = decode(v);
						assert ( ok );
						FBO.offset--;
					}
				}
			}

			void setup(uint64_t const v)
			{
				FBO = index.lookupRank(v);

				if ( FBO.file < index.Vfn.size() )
				{
					openFile();
					decodeBlock();

					while ( pc != pe && pc->r < v )
						++pc;
					assert ( pc != pe );
					assert ( pc->r >= v );
				}
				else
				{
					pa = pc = pe = 0;
				}

				#if 0
				uint64_t low = 0;
				uint64_t high = index.size();

				while ( high - low > 2 )
				{
					//std::cerr << "low=" << low << " high=" << high << std::endl;

					uint64_t m = (high+low)/2;

					init(m);

					LFRankPos P;
					bool const ok = decode(P);
					assert ( ok );

					//std::cerr << "m=" << m << " v=" << v << " P.r=" << P.r << std::endl;

					if ( P.r >= v ) // P.r is valid
					{
						high = m+1;
					}
					else // P.r < v is too small, i.e. invalid
					{
						low = m+1;
					}
				}

				while ( low < index.size() )
				{
					init(low);

					LFRankPos P;
					bool const ok = decode(P);
					assert ( ok );

					if ( P.r < v )
						++low;
					else
						break;
				}

				if ( low != index.size() )
				{
					init(low);

					LFRankPos P;
					bool const ok = decode(P);
					assert ( ok );
					assert ( P.r >= v );
				}

				init(low);
				#endif
			}

			enum init_type {
				init_type_offset,
				init_type_rank
			};

			LFRankPosDecoder(std::vector<std::string> const & rVfn, uint64_t const offset, init_type itype = init_type_offset)
			: index(rVfn), FBO(), PISI(), PSGI(), B(), pa(0), pc(0), pe(0), V(), rlbuffer()
			{
				switch ( itype )
				{
					case init_type_rank:
						setup(offset);
						break;
					default:
						init(offset);
						break;
				}
			}

			bool decode(LFRankPos & v)
			{
				while ( pc == pe )
				{
					bool const ok = decodeBlock();
					if ( ! ok )
						return false;
				}

				v = *(pc++);
				return true;
			}

			LFRankPos decode()
			{
				LFRankPos v;
				bool const ok = decode(v);
				assert ( ok );
				return v;
			}

			static uint64_t getLength(std::string const & fn, uint64_t const /* numthreads */)
			{
				return libmaus2::huffman::LFRankPosIndexDecoder::getNumValues(fn);
			}

			static uint64_t getLength(
				std::vector<std::string> const & Vfn,
				uint64_t const
					#if defined(_OPENMP)
					numthreads
					#endif
			)
			{
				uint64_t volatile s = 0;
				libmaus2::parallel::PosixSpinLock lock;

				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( uint64_t i = 0; i < Vfn.size(); ++i )
				{
					uint64_t const ls = getLength(Vfn[i],1);
					libmaus2::parallel::ScopePosixSpinLock slock(lock);
					s += ls;
				}
				return s;
			}
		};
	}
}

#endif

/*
 * Reader.hh
 *
 *  Created on: Jun 17, 2016
 *      Author: teng
 */

#ifndef READER_HH_
#define READER_HH_

#include "orc_proto.pb.h"
#include "MemoryPool.h"

namespace orc{


enum Stream_Kind {
  Stream_Kind_PRESENT = 0,
  Stream_Kind_DATA = 1,
  Stream_Kind_LENGTH = 2,
  Stream_Kind_DICTIONARY_DATA = 3,
  Stream_Kind_DICTIONARY_COUNT = 4,
  Stream_Kind_SECONDARY = 5,
  Stream_Kind_ROW_INDEX = 6,
  Stream_Kind_BLOOM_FILTER = 7
};

class ReaderOptions{

};


class FileInputStream{
 private:
   std::string filename ;
   int file;
   uint64_t totalLength;

 public:
   FileInputStream(std::string _filename);

   ~FileInputStream();

   uint64_t getLength();

   uint64_t getNaturalReadSize();

   void read(void* buf,uint64_t length,uint64_t offset);

   const std::string& getName();
 };

class Reader{
	std::unique_ptr<proto::PostScript> ps;
	std::unique_ptr<proto::Footer> footer;
	std::unique_ptr<FileInputStream> stream;

public:
	Reader(std::unique_ptr<FileInputStream> stream,std::unique_ptr<proto::PostScript> ps, std::unique_ptr<proto::Footer> footer);
	~Reader();
	void readdata(int stripe, int column, Stream_Kind kind, tengdb::DataBuffer<unsigned char> &, uint64_t &);
	uint64_t rowsize();
	int getNumofRows(int stripe_num);
	size_t getStripeSize();

};

/**
 * Create a stream to a local file.
 * @param path the name of the file in the local file system
 */

std::unique_ptr<FileInputStream> readLocalFile(const std::string& path);

/**
 * Create a reader to the for the ORC file.
 * @param stream the stream to read
 * @param options the options for reading the file
 */
Reader *createReader(std::unique_ptr<FileInputStream> stream,const ReaderOptions& options);

}
#endif /* READER_HH_ */

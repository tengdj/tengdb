/*
 * Reader.hh
 *
 *  Created on: Jun 17, 2016
 *      Author: teng
 */

#ifndef READER_HH_
#define READER_HH_

#include "OrcFile.hh"
#include "orc_proto.pb.h"

namespace orc{

class ReaderOptions{

};
class Reader{
	std::unique_ptr<orc::proto::PostScript> ps;
	std::unique_ptr<proto::Footer> footer;
	std::unique_ptr<FileInputStream> stream;

public:
	Reader(std::unique_ptr<FileInputStream> stream,std::unique_ptr<orc::proto::PostScript> ps, std::unique_ptr<orc::proto::Footer> footer);
	~Reader();
	void readdata(int stripe, int column, proto::Stream_Kind kind, char **data, uint64_t *datasize);
	uint64_t rowsize();
	proto::StripeInformation getStrips(int stripe);
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
std::unique_ptr<Reader> createReader(std::unique_ptr<FileInputStream> stream,const ReaderOptions& options);

uint64_t getfunc();
}
#endif /* READER_HH_ */


#include "../include/Reader.h"

#include <memory>
#include <iostream>

#include "../include/config.h"
#include "../include/OrcFile.h"
#include "../include/RLE2.h"
#include "../include/util.h"
#include "../include/orc_proto.pb.h"

using namespace std;

namespace orc{
static const uint64_t DIRECTORY_SIZE_GUESS = 16 * 1024;

Reader::Reader(std::unique_ptr<FileInputStream> stream,
		std::unique_ptr<orc::proto::PostScript> ps, std::unique_ptr<orc::proto::Footer> footer):
		stream(std::move(stream)),
		ps(std::move(ps)),
		footer(std::move(footer)){

}

Reader::~Reader(){

}

uint64_t Reader::rowsize(){

	return footer->numberofrows();

}

void Reader::readdata(int stripe, int column, proto::Stream_Kind kind, DataBuffer<unsigned char> &buf, uint64_t &datasize){

	proto::StripeInformation sinfo = footer->stripes(stripe);
	uint64_t footerStart = sinfo.offset()+sinfo.indexlength()+sinfo.datalength();
	uint64_t footerLength = sinfo.footerlength();
	proto::StripeFooter stripefooter;
	char footerdata[footerLength];
	stream->read(footerdata,footerLength,footerStart);
	stripefooter.ParseFromArray(footerdata,footerLength);
	uint64_t offset = sinfo.offset();

	for(int i = 0; i < stripefooter.streams_size(); ++i) {
		const proto::Stream& stream = stripefooter.streams(i);
		if (stream.has_kind() &&
				stream.kind() == kind &&
				stream.column() == static_cast<uint64_t>(column)) {
			datasize = stream.length();
			if(buf.capacity()<datasize){
				buf.resize(datasize);
			}
			this->stream->read(buf.data(),datasize,offset);
			break;
		}
		offset += stream.length();
	}//end stream
}

proto::StripeInformation Reader::getStrips(int stripe){
	return footer->stripes(stripe);
}

size_t Reader::getStripeSize(){
	return footer->stripes_size();
}
Reader* createReader(std::unique_ptr<FileInputStream> stream,
                                       const ReaderOptions& options) {

    uint64_t fileLength = static_cast<uint64_t>(stream->getLength());
    uint64_t postscriptLength;

    //read last bytes into buffer to get PostScript
    uint64_t readSize = std::min(fileLength, DIRECTORY_SIZE_GUESS);
    if (readSize < 4) {
      std::cerr<<"File size too small"<<std::endl;;
    }
    char *data = new char[readSize];
    stream->read(data, readSize, fileLength - readSize);

    postscriptLength = data[readSize - 1] & 0xff;
    std::unique_ptr<proto::PostScript> ps = std::unique_ptr<proto::PostScript>(new proto::PostScript());
    if (!ps->ParseFromArray(data + readSize - 1 - postscriptLength, static_cast<int>(postscriptLength))) {
        std::cerr<<"Failed to parse the postscript from " + stream->getName()<<std::endl;
    }
	uint64_t footerSize = ps->footerlength();
	uint64_t tailSize = 1 + postscriptLength + footerSize;//1 byte for postscriptLength
	uint64_t footerOffset;

	footerOffset = readSize - tailSize;
	char *footerPtr = data + footerOffset;
	std::unique_ptr<proto::Footer> footer = std::unique_ptr<proto::Footer>(new proto::Footer());
	if (!footer->ParseFromArray(footerPtr,footerSize)) {
	 	std::cerr<<"Failed to parse the footer from " + stream->getName()<<std::endl;
	}
    delete data;

    return new Reader(std::move(stream),std::move(ps),std::move(footer));
  }

}

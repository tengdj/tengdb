/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "OrcFile.hh"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>

using namespace std;

namespace orc {

FileInputStream::FileInputStream(std::string _filename) {
      filename = _filename ;
      file = open(filename.c_str(), O_RDONLY);
      if (file == -1) {
        std::cerr<<"Can't open " + filename +"\n";
      }
      struct stat fileStat;
      if (fstat(file, &fileStat) == -1) {
        std::cerr<<"Can't stat " + filename<<std::endl;
      }
      totalLength = static_cast<uint64_t>(fileStat.st_size);
    }

	FileInputStream::~FileInputStream(){
    	close(file);
    }

    uint64_t FileInputStream::getLength(){
      return totalLength;
    }

    uint64_t FileInputStream::getNaturalReadSize(){
      return 128 * 1024;
    }

    void FileInputStream::read(void* buf,
              uint64_t length,
              uint64_t offset) {
      if (!buf) {
        std::cerr<<"Buffer is null"<<std::endl;
      }
      ssize_t bytesRead = pread(file, buf, length, static_cast<off_t>(offset));

      if (bytesRead == -1) {
        std::cerr<<"Bad read of " + filename<<std::endl;
      }
      if (static_cast<uint64_t>(bytesRead) != length) {
        std::cerr<<" Short read of " + filename<<std::endl;
      }
    }

    const std::string& FileInputStream::getName(){
      return filename;
    }


  std::unique_ptr<FileInputStream> readLocalFile(const std::string& path) {
    return std::unique_ptr<FileInputStream>(new FileInputStream(path));
  }
}


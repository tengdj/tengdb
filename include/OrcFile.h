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

#ifndef ORC_FILE_HH
#define ORC_FILE_HH

#include <string>
#include <memory>

/** /file orc/OrcFile.hh
    @brief The top level interface to ORC.
*/

namespace orc {

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
}

#endif

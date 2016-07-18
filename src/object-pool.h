// Copyright 2012 Cloudera Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#ifndef IMPALA_COMMON_OBJECT_POOL_H
#define IMPALA_COMMON_OBJECT_POOL_H

#include <vector>

namespace orc {

/// An ObjectPool maintains a list of C++ objects which are deallocated by destroying or
/// clearing the pool.
/// Thread-safe.
class ObjectPool {
 public:
  ObjectPool(): objects_() {}

  ~ObjectPool() { Clear(); }

  template <class T>
  T* Add(T* t) {
    // Create the object to be pushed to the shared vector outside the critical section.
    // TODO: Consider using a lock-free structure.
    SpecificElement<T>* obj = new SpecificElement<T>(t);
    objects_.push_back(obj);
    return t;
  }

  void Clear() {
    for (ElementVector::iterator i = objects_.begin();
         i != objects_.end(); ++i) {
      delete *i;
    }
    objects_.clear();
  }

 private:
  struct GenericElement {
    virtual ~GenericElement() {}
  };

  template <class T>
  struct SpecificElement : GenericElement {
    SpecificElement(T* t): t(t) {}
    ~SpecificElement() {
      delete t;
    }

    T* t;
  };

  typedef std::vector<GenericElement*> ElementVector;
  ElementVector objects_;
};

}

#endif

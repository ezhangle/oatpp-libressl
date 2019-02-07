/***************************************************************************
 *
 * Project         _____    __   ____   _      _
 *                (  _  )  /__\ (_  _)_| |_  _| |_
 *                 )(_)(  /(__)\  )( (_   _)(_   _)
 *                (_____)(__)(__)(__)  |_|    |_|
 *
 *
 * Copyright 2018-present, Leonid Stryzhevskyi, <lganzzzo@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ***************************************************************************/

#ifndef oatpp_libressl_Connection_hpp
#define oatpp_libressl_Connection_hpp

#include "oatpp/core/base/memory/ObjectPool.hpp"
#include "oatpp/core/data/stream/Stream.hpp"

#include <tls.h>

namespace oatpp { namespace libressl {
    
class Connection : public oatpp::base::Controllable, public oatpp::data::stream::IOStream {
public:
  typedef struct tls* TLSHandle;
public:
  OBJECT_POOL(libressl_Connection_Pool, Connection, 32);
  SHARED_OBJECT_POOL(libressl_Shared_Connection_Pool, Connection, 32);
private:
  TLSHandle m_tlsHandle;
  data::v_io_handle m_handle;
public:
  Connection(TLSHandle tlsHandle, data::v_io_handle handle);
public:
  
  static std::shared_ptr<Connection> createShared(TLSHandle tlsHandle, data::v_io_handle handle){
    return libressl_Shared_Connection_Pool::allocateShared(tlsHandle, handle);
  }
  
  ~Connection();
  
  data::v_io_size write(const void *buff, data::v_io_size count) override;
  data::v_io_size read(void *buff, data::v_io_size count) override;
  
  void close();
  
  TLSHandle getTlsHandle() {
    return m_tlsHandle;
  }
  
  data::v_io_handle getHandle() {
    return m_handle;
  }
  
};
  
}}

#endif /* oatpp_libressl_Connection_hpp */

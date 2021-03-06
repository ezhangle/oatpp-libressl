/***************************************************************************
 *
 * Project         _____    __   ____   _      _
 *                (  _  )  /__\ (_  _)_| |_  _| |_
 *                 )(_)(  /(__)\  )( (_   _)(_   _)
 *                (_____)(__)(__)(__)  |_|    |_|
 *
 *
 * Copyright 2018-present, Leonid Stryzhevskyi <lganzzzo@gmail.com>
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

#include "ConnectionProvider.hpp"

#include "oatpp/core/utils/ConversionUtils.hpp"

#include <fcntl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

#include <openssl/crypto.h>

#include <unistd.h>

namespace oatpp { namespace libressl { namespace server {
  
ConnectionProvider::ConnectionProvider(const std::shared_ptr<Config>& config,
                                       v_word16 port,
                                       bool nonBlocking)
  : m_config(config)
  , m_port(port)
  , m_nonBlocking(nonBlocking)
  , m_closed(false)
{
  
  setProperty(PROPERTY_HOST, "localhost");
  setProperty(PROPERTY_PORT, oatpp::utils::conversion::int32ToStr(port));
  
  auto calback = CRYPTO_get_locking_callback();
  if(!calback) {
    OATPP_LOGD("[oatpp::libressl::server::ConnectionProvider::ConnectionProvider()]",
               "WARNING. libressl. CRYPTO_set_locking_callback is NOT set. "
               "This can cause problems using libressl in multithreaded environment! "
               "Please call oatpp::libressl::Callbacks::setDefaultCallbacks() or "
               "consider setting custom locking_callback.");
  }
  
  m_serverHandle = instantiateServer();
  m_tlsServerHandle = instantiateTLSServer();
}

std::shared_ptr<ConnectionProvider> ConnectionProvider::createShared(const std::shared_ptr<Config>& config,
                                                                     v_word16 port,
                                                                     bool nonBlocking){
  return std::shared_ptr<ConnectionProvider>(new ConnectionProvider(config, port, nonBlocking));
}

ConnectionProvider::~ConnectionProvider() {
  close();
}
  
data::v_io_handle ConnectionProvider::instantiateServer(){
  
  data::v_io_handle serverHandle;
  v_int32 ret;
  int yes = 1;
  
  struct sockaddr_in6 addr;
  
  addr.sin6_family = AF_INET6;
  addr.sin6_port = htons(m_port);
  addr.sin6_addr = in6addr_any;
  
  serverHandle = socket(AF_INET6, SOCK_STREAM, 0);
  
  if(serverHandle < 0){
    return -1;
  }
  
  ret = setsockopt(serverHandle, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  if(ret < 0) {
    OATPP_LOGD("[oatpp::libressl::server::ConnectionProvider::instantiateServer()]", "Warning failed to set %s for accepting socket", "SO_REUSEADDR");
  }
  
  ret = bind(serverHandle, (struct sockaddr *)&addr, sizeof(addr));
  
  if(ret != 0) {
    ::close(serverHandle);
    throw std::runtime_error("[oatpp::libressl::server::ConnectionProvider::instantiateServer()]: Can't bind to address");
    return -1 ;
  }
  
  ret = listen(serverHandle, 10000);
  if(ret < 0) {
    ::close(serverHandle);
    throw std::runtime_error("[oatpp::libressl::server::ConnectionProvider::instantiateServer()]: Failed to listen");
    return -1 ;
  }
  
  fcntl(serverHandle, F_SETFL, 0);//O_NONBLOCK);
  
  return serverHandle;
  
}
  
Connection::TLSHandle ConnectionProvider::instantiateTLSServer() {
  
  Connection::TLSHandle handle = tls_server();
  
  if(handle == NULL) {
    throw std::runtime_error("[oatpp::libressl::server::ConnectionProvider::instantiateTLSServer()]: Failed to create tls_server");
  }
  
  if(tls_configure(handle, m_config->getTLSConfig()) < 0) {
    OATPP_LOGD("[oatpp::libressl::server::ConnectionProvider::instantiateTLSServer()]", "Error on call to 'tls_configure'. %s", tls_error(handle));
    throw std::runtime_error("[oatpp::libressl::server::ConnectionProvider::instantiateTLSServer()]: Failed to configure tls_server");
  }
  
  return handle;
  
}

void ConnectionProvider::close() {
  if(!m_closed) {
    m_closed = true;
    tls_close(m_tlsServerHandle);
    tls_free(m_tlsServerHandle);
    ::close(m_serverHandle);
  }
}

std::shared_ptr<oatpp::data::stream::IOStream> ConnectionProvider::getConnection(){
  
  data::v_io_handle handle = accept(m_serverHandle, nullptr, nullptr);
  
  if (handle < 0) {
    v_int32 error = errno;
    if(error == EAGAIN || error == EWOULDBLOCK){
      return nullptr;
    } else {
      OATPP_LOGD("[oatpp::libressl::server::ConnectionProvider::getConnection()]", "Error: %d", error);
      return nullptr;
    }
  }
  
#ifdef SO_NOSIGPIPE
  int yes = 1;
  v_int32 ret = setsockopt(handle, SOL_SOCKET, SO_NOSIGPIPE, &yes, sizeof(int));
  if(ret < 0) {
    OATPP_LOGD("[oatpp::libressl::server::ConnectionProvider::getConnection()]", "Warning failed to set %s for socket", "SO_NOSIGPIPE");
  }
#endif
  
  int flags = 0;
  if(m_nonBlocking) {
    flags |= O_NONBLOCK;
  }
  
  fcntl(handle, F_SETFL, flags);
  
  Connection::TLSHandle tlsHandle;
  
  if(tls_accept_socket(m_tlsServerHandle, &tlsHandle, handle) < 0) {
    OATPP_LOGD("[oatpp::libressl::server::ConnectionProvider::getConnection()]", "Error on call to 'tls_accept_socket'");
    ::close(handle);
  }
  
  return Connection::createShared(tlsHandle, handle);
  
}
  
}}}

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

#include "Config.hpp"

namespace oatpp { namespace libressl {
  
std::shared_ptr<Config> Config::createDefaultServerConfig(const oatpp::String& keyFile,
                                                          const oatpp::String& certFile) {
  
  unsigned int protocols = TLS_PROTOCOLS_ALL;
  const char *ciphers = "secure";
  
  auto config = createShared();
  
  tls_config_set_protocols(config->getTLSConfig(), protocols);
  
  if(tls_config_set_ciphers(config->getTLSConfig(), ciphers) < 0) {
    throw std::runtime_error("[oatpp::libressl::Config::createDefaultServerConfig]: failed call to tls_config_set_ciphers()");
  }
  
  if(tls_config_set_key_file(config->getTLSConfig(), keyFile->c_str()) < 0) {
    throw std::runtime_error("[oatpp::libressl::Config::createDefaultServerConfig]: failed call to tls_config_set_key_file()");
  }
  
  if(tls_config_set_cert_file(config->getTLSConfig(), certFile->c_str()) < 0) {
    throw std::runtime_error("[oatpp::libressl::Config::createDefaultServerConfig]: failed call to tls_config_set_cert_file()");
  }
  
  return config;
  
}
  
}}

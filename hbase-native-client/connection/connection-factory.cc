/*
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
 *
 */

#include "connection/connection-factory.h"

#include <chrono>

#include "connection/client-dispatcher.h"
#include "connection/pipeline.h"
#include "connection/service.h"

using namespace folly;
using namespace hbase;
using std::chrono::milliseconds;
using std::chrono::nanoseconds;

ConnectionFactory::ConnectionFactory(std::shared_ptr<wangle::IOThreadPoolExecutor> io_pool,
                                     std::shared_ptr<Codec> codec, nanoseconds connect_timeout)
    : connect_timeout_(connect_timeout),
      io_pool_(io_pool),
      pipeline_factory_(std::make_shared<RpcPipelineFactory>(codec)) {}

std::shared_ptr<wangle::ClientBootstrap<SerializePipeline>> ConnectionFactory::MakeBootstrap() {
  auto client = std::make_shared<wangle::ClientBootstrap<SerializePipeline>>();
  client->group(io_pool_);
  client->pipelineFactory(pipeline_factory_);

  // TODO: Opened https://github.com/facebook/wangle/issues/85 in wangle so that we can set socket
  //  options like TCP_NODELAY, SO_KEEPALIVE, CONNECT_TIMEOUT_MILLIS, etc.

  return client;
}
std::shared_ptr<HBaseService> ConnectionFactory::Connect(
    std::shared_ptr<wangle::ClientBootstrap<SerializePipeline>> client, const std::string &hostname,
    int port) {
  // Yes this will block however it makes dealing with connection pool soooooo
  // much nicer.
  // TODO see about using shared promise for this.
  auto pipeline = client
                      ->connect(SocketAddress(hostname, port, true),
                                std::chrono::duration_cast<milliseconds>(connect_timeout_))
                      .get();
  auto dispatcher = std::make_shared<ClientDispatcher>();
  dispatcher->setPipeline(pipeline);
  return dispatcher;
}

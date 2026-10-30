/* Copyright 2013-2019 Homegear GmbH
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#ifndef MYNODE_H_
#define MYNODE_H_

#include <homegear-base/Managers/TranslationManager.h>

#include <homegear-node/INode.h>
#include <homegear-node/RpcDecoder.h>
#include <unordered_map>
#include <string>
#include <atomic>
#include <thread>
#include <homegear-base/Encoding/RpcEncoder.h>

namespace Cloud {

class CloudDevice : public Flows::INode {
 public:
  CloudDevice(const std::string &path, const std::string &type, const std::atomic_bool *frontendConnected);
  ~CloudDevice() override;

  bool init(const Flows::PNodeInfo &info) override;
  bool start() override;
  void startUpComplete() override;
  void stop() override;
  void waitForStop() override;
 private:
  enum class Priority {
    kCritical = 1,
    kError = 2,
    kWarning = 3,
    kInfo = 4,
    kOffline = 0x100,
  };

  struct DataPointInfo {
    std::string id;
    std::string label;
    std::string type;
    int64_t role = 0;
    int64_t service_role = 0;
    std::string au;
    std::string unit;
    int64_t unit_code;
    Priority priority = Priority::kInfo;

    //Dynamic info
    bool service_message_active = false;
  };

  std::string name_;
  std::string description_;
  std::string manufacturer_;
  std::string product_name_;
  std::string serial_number_;
  std::string software_version_;
  std::string equipment_id_;
  std::vector<std::string> tags_;
  std::string section_;
  std::string au_;
  int64_t offline_timeout_ = 176400;
  Flows::PVariable metadata_;
  std::unordered_map<uint32_t, std::shared_ptr<DataPointInfo>> data_points_;

  std::atomic_bool is_offline_{false};
  std::atomic<int64_t> last_input_{0};
  std::atomic_bool stop_worker_thread_{true};
  std::thread worker_thread_;

  std::unique_ptr<BaseLib::Rpc::RpcEncoder> base_lib_rpc_encoder_ = std::make_unique<BaseLib::Rpc::RpcEncoder>(true, true);
  std::unique_ptr<Flows::RpcDecoder> node_rpc_decoder_ = std::make_unique<Flows::RpcDecoder>();


  void SetOffline(bool value);
  bool PublishServiceMessage(const std::string &variable, bool active, Priority priority);
  void input(const Flows::PNodeInfo &info, uint32_t index, const Flows::PVariable &message) override;
  void Worker();
};

}

#endif

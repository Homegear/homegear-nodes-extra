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

#include "CloudDevice.h"

#include <homegear-node/JsonDecoder.h>

namespace Cloud {

CloudDevice::CloudDevice(const std::string &path, const std::string &type, const std::atomic_bool *frontendConnected) : Flows::INode(path, type, frontendConnected) {
}

CloudDevice::~CloudDevice() = default;

bool CloudDevice::init(const Flows::PNodeInfo &info) {
  try {
    int32_t inputIndex = -1;

    auto settingsIterator = info->info->structValue->find("name");
    if (settingsIterator != info->info->structValue->end()) { name_ = settingsIterator->second->stringValue; }

    settingsIterator = info->info->structValue->find("description");
    if (settingsIterator != info->info->structValue->end()) { description_ = settingsIterator->second->stringValue; }

    settingsIterator = info->info->structValue->find("manufacturer");
    if (settingsIterator != info->info->structValue->end()) { manufacturer_ = settingsIterator->second->stringValue; }

    settingsIterator = info->info->structValue->find("product-name");
    if (settingsIterator != info->info->structValue->end()) { product_name_ = settingsIterator->second->stringValue; }

    settingsIterator = info->info->structValue->find("serial-number");
    if (settingsIterator != info->info->structValue->end()) { serial_number_ = settingsIterator->second->stringValue; }

    settingsIterator = info->info->structValue->find("software-version");
    if (settingsIterator != info->info->structValue->end()) { software_version_ = settingsIterator->second->stringValue; }

    settingsIterator = info->info->structValue->find("equipment-id");
    if (settingsIterator != info->info->structValue->end()) { equipment_id_ = settingsIterator->second->stringValue; }

    settingsIterator = info->info->structValue->find("tags");
    if (settingsIterator != info->info->structValue->end()) {
      std::stringstream stringStream(settingsIterator->second->stringValue);
      std::string element;
      while (std::getline(stringStream, element, ',')) {
        tags_.emplace_back(Flows::HelperFunctions::toLower(Flows::HelperFunctions::trim(element)));
      }
    }

    settingsIterator = info->info->structValue->find("section");
    if (settingsIterator != info->info->structValue->end()) { section_ = settingsIterator->second->stringValue; }

    settingsIterator = info->info->structValue->find("au");
    if (settingsIterator != info->info->structValue->end()) { au_ = settingsIterator->second->stringValue; }

    settingsIterator = info->info->structValue->find("offline-timeout");
    if (settingsIterator != info->info->structValue->end()) { offline_timeout_ = settingsIterator->second->integerValue64; }
    if (offline_timeout_ != 0 && offline_timeout_ < 600) { offline_timeout_ = 176400; }

    settingsIterator = info->info->structValue->find("metadata");
    if (settingsIterator != info->info->structValue->end()) {
      try {
        metadata_ = Flows::JsonDecoder::decode(settingsIterator->second->stringValue);
      } catch (const Flows::JsonDecoderException &ex) {
        _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
      }
    }

    settingsIterator = info->info->structValue->find("datapoints");
    if (settingsIterator != info->info->structValue->end()) {
      for (auto &element : *settingsIterator->second->arrayValue) {
        inputIndex++;

        auto data_point_info = std::make_shared<DataPointInfo>();

        auto data_point_iterator = element->structValue->find("id");
        if (data_point_iterator == element->structValue->end() || data_point_iterator->second->stringValue.empty()) { continue; }
        data_point_info->id = data_point_iterator->second->stringValue;

        data_point_iterator = element->structValue->find("l");
        if (data_point_iterator != element->structValue->end()) {
          data_point_info->label = data_point_iterator->second->stringValue;
        }

        data_point_iterator = element->structValue->find("t");
        if (data_point_iterator == element->structValue->end()) { continue; }
        if (data_point_iterator->second->stringValue == "int") { data_point_info->type = "integer"; }
        else if (data_point_iterator->second->stringValue == "float") { data_point_info->type = "double"; }
        else if (data_point_iterator->second->stringValue == "bool") { data_point_info->type = "boolean"; }
        else if (data_point_iterator->second->stringValue == "action") { data_point_info->type = "action"; }
        else if (data_point_iterator->second->stringValue == "string") { data_point_info->type = "string"; }

        data_point_iterator = element->structValue->find("r");
        if (data_point_iterator == element->structValue->end()) { continue; }
        data_point_info->role = (int64_t) Flows::Math::getUnsignedNumber64(data_point_iterator->second->stringValue);

        data_point_iterator = element->structValue->find("sr");
        if (data_point_iterator != element->structValue->end()) {
          data_point_info->service_role = (int64_t) Flows::Math::getUnsignedNumber64(data_point_iterator->second->stringValue);
        }

        data_point_iterator = element->structValue->find("au");
        if (data_point_iterator != element->structValue->end()) { data_point_info->au = data_point_iterator->second->stringValue; }
        if (!au_.empty() && data_point_info->au.empty()) { data_point_info->au = au_; }

        data_point_iterator = element->structValue->find("u");
        if (data_point_iterator != element->structValue->end()) { data_point_info->unit = data_point_iterator->second->stringValue; }

        data_point_iterator = element->structValue->find("uc");
        if (data_point_iterator != element->structValue->end()) { data_point_info->unit_code = Flows::Math::getNumber64(data_point_iterator->second->stringValue); }

        data_point_iterator = element->structValue->find("p");
        if (data_point_iterator != element->structValue->end()) {
          if (data_point_iterator->second->stringValue == "critical") { data_point_info->priority = Priority::kCritical; }
          else if (data_point_iterator->second->stringValue == "error") { data_point_info->priority = Priority::kError; }
          else if (data_point_iterator->second->stringValue == "warning") { data_point_info->priority = Priority::kWarning; }
          else if (data_point_iterator->second->stringValue == "info") { data_point_info->priority = Priority::kInfo; }
        }

        data_point_info->service_message_active = getNodeData(data_point_info->id + ".serviceMessage")->booleanValue;

        data_points_.emplace(inputIndex, data_point_info);
      }
    }

    if (name_.empty() || product_name_.empty() || serial_number_.empty() || section_.empty()) { return false; }

    is_offline_ = getNodeData("offline")->booleanValue;
    last_input_ = BaseLib::HelperFunctions::getTimeSeconds();

    return true;
  }
  catch (const std::exception &ex) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return false;
}

bool CloudDevice::start() {
  try {
    {
      auto device_info = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
      device_info->structValue->emplace("name", std::make_shared<Flows::Variable>(name_));
      device_info->structValue->emplace("description", std::make_shared<Flows::Variable>(description_));
      device_info->structValue->emplace("manufacturer", std::make_shared<Flows::Variable>(manufacturer_));
      device_info->structValue->emplace("productName", std::make_shared<Flows::Variable>(product_name_));
      device_info->structValue->emplace("serialNumber", std::make_shared<Flows::Variable>(serial_number_));
      device_info->structValue->emplace("softwareVersion", std::make_shared<Flows::Variable>(software_version_));
      device_info->structValue->emplace("equipmentId", std::make_shared<Flows::Variable>(equipment_id_));
      device_info->structValue->emplace("tags", std::make_shared<Flows::Variable>(tags_));
      device_info->structValue->emplace("section", std::make_shared<Flows::Variable>(section_));
      if (metadata_) { device_info->structValue->emplace("metadata", metadata_); }

      auto data_points_array = std::make_shared<Flows::Variable>(Flows::VariableType::tArray);
      data_points_array->arrayValue->reserve(data_points_.size() + 1);

      {
        //Offline data point
        auto data_point_info = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
        data_point_info->structValue->emplace("id", std::make_shared<Flows::Variable>("offline"));
        data_point_info->structValue->emplace("type", std::make_shared<Flows::Variable>("boolean"));
        data_point_info->structValue->emplace("serviceRole", std::make_shared<Flows::Variable>(800002));
        data_point_info->structValue->emplace("au", std::make_shared<Flows::Variable>(au_));
        data_point_info->structValue->emplace("priority", std::make_shared<Flows::Variable>(3));
        data_points_array->arrayValue->emplace_back(data_point_info);
      }

      for (auto &data_point : data_points_) {
        auto data_point_info = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
        data_point_info->structValue->emplace("id", std::make_shared<Flows::Variable>(data_point.second->id));
        data_point_info->structValue->emplace("label", std::make_shared<Flows::Variable>(data_point.second->label));
        data_point_info->structValue->emplace("type", std::make_shared<Flows::Variable>(data_point.second->type));
        data_point_info->structValue->emplace("role", std::make_shared<Flows::Variable>(data_point.second->role));
        if (data_point.second->service_role != 0) { data_point_info->structValue->emplace("serviceRole", std::make_shared<Flows::Variable>(data_point.second->service_role)); }
        data_point_info->structValue->emplace("au", std::make_shared<Flows::Variable>(data_point.second->au));
        data_point_info->structValue->emplace("unit", std::make_shared<Flows::Variable>(data_point.second->unit));
        data_point_info->structValue->emplace("unitCode", std::make_shared<Flows::Variable>(data_point.second->unit_code));
        data_point_info->structValue->emplace("priority", std::make_shared<Flows::Variable>((int32_t) data_point.second->priority));
        data_points_array->arrayValue->emplace_back(data_point_info);
      }
      device_info->structValue->emplace("dataPoints", data_points_array);

      auto parameters = std::make_shared<Flows::Array>();
      parameters->reserve(3);
      parameters->emplace_back(std::make_shared<Flows::Variable>("homegear-cloudconnect-devices"));
      parameters->emplace_back(std::make_shared<Flows::Variable>("nb_" + _id));
      parameters->emplace_back(device_info);

      invoke("setData", parameters);
    }

    return true;
  }
  catch (const std::exception &ex) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return false;
}

void CloudDevice::startUpComplete() {
  stop_worker_thread_ = false;
  if (worker_thread_.joinable()) { worker_thread_.join(); }
  worker_thread_ = std::thread(&CloudDevice::Worker, this);
}

void CloudDevice::stop() {
  stop_worker_thread_ = true;
}

void CloudDevice::waitForStop() {
  stop_worker_thread_ = true;
  if (worker_thread_.joinable()) { worker_thread_.join(); }
}

void CloudDevice::SetOffline(bool value) {
  {
    auto parameters = std::make_shared<Flows::Array>();
    parameters->reserve(3);
    parameters->emplace_back(std::make_shared<Flows::Variable>("nb_" + _id));
    parameters->emplace_back(std::make_shared<Flows::Variable>("offline"));
    parameters->emplace_back(std::make_shared<Flows::Variable>(value));

    if (invoke("cloudconnectEvent", parameters)->errorStruct) {
      return;
    }
  }

  if (PublishServiceMessage("offline", value, Priority::kOffline)) {
    //Set is_offline_ on success only
    if (value != is_offline_) {
      is_offline_ = value;
      setNodeData("offline", std::make_shared<Flows::Variable>(value));
    }
  }
}

bool CloudDevice::PublishServiceMessage(const std::string &variable, bool active, Priority priority) {
  try {
    auto service_message_struct = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);

    service_message_struct->structValue->emplace("TYPE", std::make_shared<Flows::Variable>(2));
    service_message_struct->structValue->emplace("VALUE", std::make_shared<Flows::Variable>((int32_t) active));
    service_message_struct->structValue->emplace("PRIORITY", std::make_shared<Flows::Variable>((int32_t) priority));
    service_message_struct->structValue->emplace("TIMESTAMP", std::make_shared<Flows::Variable>(Flows::HelperFunctions::getTimeSeconds()));
    service_message_struct->structValue->emplace("PEER_ID", std::make_shared<Flows::Variable>("nb_" + _id));
    service_message_struct->structValue->emplace("CHANNEL", std::make_shared<Flows::Variable>(1));
    service_message_struct->structValue->emplace("VARIABLE", std::make_shared<Flows::Variable>(variable));

    auto translations = BaseLib::TranslationManager::getTranslations("l10n.common.serviceMessage." + variable);

    //{{{ Set name to variable if there are no translations
    auto translations_iterator = translations->structValue->find("en");
    if (translations_iterator != translations->structValue->end() && translations_iterator->second->stringValue.compare(0, 5, "l10n.") == 0) {
      translations_iterator->second->stringValue = variable;
    }
    //}}}

    std::vector<uint8_t> encoded_data;
    base_lib_rpc_encoder_->encodeResponse(translations, encoded_data);
    service_message_struct->structValue->emplace("MESSAGE", node_rpc_decoder_->decodeResponse(encoded_data));

    auto parameters = std::make_shared<Flows::Array>();
    parameters->emplace_back(service_message_struct);

    return !invoke("cloudconnectServiceMessage", parameters)->errorStruct;
  }
  catch (const std::exception &ex) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return false;
}

void CloudDevice::Worker() {
  int64_t last_offline_event = Flows::HelperFunctions::getTime();
  int64_t hour_counter = 0;
  auto seconds_to_wait = BaseLib::HelperFunctions::getRandomNumber(60, 180);
  for (int32_t i = 0; i < seconds_to_wait; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    if (stop_worker_thread_) { return; }
  }
  while (!stop_worker_thread_) {
    try {
      for (int32_t i = 0; i < 60; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        if (stop_worker_thread_) { return; }
      }
      hour_counter++;

      if (offline_timeout_ != 0 && Flows::HelperFunctions::getTimeSeconds() - last_input_ >= offline_timeout_ && !is_offline_) {
        last_offline_event = Flows::HelperFunctions::getTime();
        SetOffline(true);
      } else if (Flows::HelperFunctions::getTimeSeconds() - last_input_ < offline_timeout_ && is_offline_) {
        last_offline_event = Flows::HelperFunctions::getTime();
        SetOffline(false);
      } else if ((is_offline_ && Flows::HelperFunctions::getTime() - last_offline_event >= 3600000) || Flows::HelperFunctions::getTime() - last_offline_event >= 86400000) {
        last_offline_event = Flows::HelperFunctions::getTime();
        SetOffline(is_offline_);
      }

      if (hour_counter % 60 == 0) {
        bool day_event = hour_counter % 1440 == 0;
        for (auto &data_point_iterator : data_points_) {
          auto &data_point = data_point_iterator.second;
          if ((data_point->service_message_active || day_event) && data_point->service_role != 0) {
            //Publish active service message once per hour and inactive ones once per day.
            if (!PublishServiceMessage(data_point->id, data_point->service_message_active, data_point->priority)) {
              hour_counter -= 10; //Retry in 10 minutes
            }
          }
        }
      }
    }
    catch (const std::exception &ex) {
      _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
  }
}

void CloudDevice::input(const Flows::PNodeInfo &info, uint32_t index, const Flows::PVariable &message) {
  try {
    auto &value = message->structValue->at("payload");
    auto &data_point = data_points_.at(index);

    auto parameters = std::make_shared<Flows::Array>();
    parameters->reserve(3);
    parameters->emplace_back(std::make_shared<Flows::Variable>("nb_" + _id));
    parameters->emplace_back(std::make_shared<Flows::Variable>(data_point->id));
    parameters->emplace_back(message->structValue->at("payload"));

    invoke("cloudconnectEvent", parameters);

    if (data_point->service_role >= 800000 && data_point->service_role < 810000) {
      auto state = (bool) (*value);
      if ((state && !data_point->service_message_active) || (!state && data_point->service_message_active)) {
        if (PublishServiceMessage(data_point->id, state, data_point->priority)) {
          data_point->service_message_active = state;
          setNodeData(data_point->id + ".serviceMessage", std::make_shared<Flows::Variable>(state));
        }
      }
    }

    last_input_ = Flows::HelperFunctions::getTimeSeconds();
  }
  catch (const std::exception &ex) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

}

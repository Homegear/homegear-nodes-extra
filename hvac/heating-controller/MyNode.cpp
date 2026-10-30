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

#include "MyNode.h"

namespace MyNode {

MyNode::MyNode(const std::string &path, const std::string &type, const std::atomic_bool *frontendConnected) : Flows::INode(path, type, frontendConnected) {
  try {
    _heatingController = std::unique_ptr<HeatingController>(new HeatingController(_out));
    _heatingController->setSetUValve(std::function<void(int32_t position)>(std::bind(&MyNode::setUHeating, this, std::placeholders::_1)));
  }
  catch (const std::exception &ex) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

MyNode::~MyNode() = default;

bool MyNode::init(const Flows::PNodeInfo &info) {
  try {
    std::string heatingType;
    auto settingsIterator = info->info->structValue->find("heatingtype");
    if (settingsIterator != info->info->structValue->end()) heatingType = settingsIterator->second->stringValue;

    settingsIterator = info->info->structValue->find("decalcificationday");
    if (settingsIterator != info->info->structValue->end()) _decalcificationDay = Flows::Math::getNumber(settingsIterator->second->stringValue);

    settingsIterator = info->info->structValue->find("decalcificationhour");
    if (settingsIterator != info->info->structValue->end()) _decalcificationHour = Flows::Math::getNumber(settingsIterator->second->stringValue);

    if (_decalcificationDay < 0 || _decalcificationDay > 6) _decalcificationDay = 5;
    if (_decalcificationHour < 0 || _decalcificationHour > 23) _decalcificationHour = 11;

    HeatingController::ControllerSettings controllerSettings;
    if (heatingType == "radiator") {
      controllerSettings.Kp = 50.0; // P gain in % valve state per °C
      controllerSettings.TN = 3600.0; // I time in seconds (kind of time constant)
      controllerSettings.Td = 0.0; // D time in seconds (duration of "compensation shot" in seconds)
      controllerSettings.RC_w_LP = 60.0; // LP time constant for w in seconds
      controllerSettings.y_LP_RC = 60.0; // LP time constant for y in seconds (should be <180 to avoid instability)
      controllerSettings.uvalve_LP_RC = 60.0; // LP time constant for uvalve in seconds (should be <180 to avoid instability)
    } else if (heatingType == "castironradiator") {
      controllerSettings.Kp = 50.0;
      controllerSettings.TN = 7200.0;
      controllerSettings.Td = 0.0;
      controllerSettings.RC_w_LP = 600.0;
      controllerSettings.y_LP_RC = 60.0;
      controllerSettings.uvalve_LP_RC = 60.0;
    } else if (heatingType == "floorheating") {
      controllerSettings.Kp = 50.0;
      controllerSettings.TN = 14400.0;
      controllerSettings.Td = 0.0;
      controllerSettings.RC_w_LP = 1200.0;
      controllerSettings.y_LP_RC = 60.0;
      controllerSettings.uvalve_LP_RC = 60.0;
    } else //manual
    {
      double value = 0;
      settingsIterator = info->info->structValue->find("kp");
      if (settingsIterator != info->info->structValue->end()) value = Flows::Math::getDouble(settingsIterator->second->stringValue);
      if (value < 0.1) value = 0.1;
      controllerSettings.Kp = value;

      value = 0;
      settingsIterator = info->info->structValue->find("tn");
      if (settingsIterator != info->info->structValue->end()) value = Flows::Math::getDouble(settingsIterator->second->stringValue);
      if (value < 0.1) value = 0.1;
      controllerSettings.TN = value;

      value = 0;
      settingsIterator = info->info->structValue->find("td");
      if (settingsIterator != info->info->structValue->end()) value = Flows::Math::getDouble(settingsIterator->second->stringValue);
      if (value < 0.1) value = 0.1;
      controllerSettings.Td = value;

      value = 0;
      settingsIterator = info->info->structValue->find("rcwlp");
      if (settingsIterator != info->info->structValue->end()) value = Flows::Math::getDouble(settingsIterator->second->stringValue);
      if (value < 0.1) value = 0.1;
      controllerSettings.RC_w_LP = value;

      value = 0;
      settingsIterator = info->info->structValue->find("ylprc");
      if (settingsIterator != info->info->structValue->end()) value = Flows::Math::getDouble(settingsIterator->second->stringValue);
      if (value < 0.1) value = 0.1;
      controllerSettings.y_LP_RC = value;

      value = 0;
      settingsIterator = info->info->structValue->find("uvalvelprc");
      if (settingsIterator != info->info->structValue->end()) value = Flows::Math::getDouble(settingsIterator->second->stringValue);
      if (value < 0.1) value = 0.1;
      controllerSettings.uvalve_LP_RC = value;
    }

    _heatingController->unserialize(getNodeData("heatingcontroller"));

    auto setting = getNodeData("enabled");
    if (setting->type == Flows::VariableType::tBoolean) _enabled = setting->booleanValue;

    setting = getNodeData("w");
    if (setting->type == Flows::VariableType::tFloat) _heatingController->setSetPointTemperature(setting->floatValue);

    setting = getNodeData("y");
    if (setting->type == Flows::VariableType::tFloat) _heatingController->setCurrentTemperature(setting->floatValue);

    _heatingController->setControllerSettings(controllerSettings);

    return true;
  }
  catch (const std::exception &ex) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return false;
}

bool MyNode::start() {
  return true;
}

void MyNode::startUpComplete() {
  try {
    std::lock_guard<std::mutex> timerGuard(_timerMutex);
    _stopThread = false;
    if (_timerThread.joinable()) _timerThread.join();
    _timerThread = std::thread(&MyNode::tick, this);
  }
  catch (const std::exception &ex) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void MyNode::stop() {
  try {
    _stopThread = true;
    std::lock_guard<std::mutex> variableGuard(_variableMutex);
    setNodeData("y", std::make_shared<Flows::Variable>(_heatingController->getCurrentTemperature()));
    setNodeData("heatingcontroller", _heatingController->serialize());
  }
  catch (const std::exception &ex) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void MyNode::waitForStop() {
  try {
    std::lock_guard<std::mutex> timerGuard(_timerMutex);
    _stopThread = true;
    if (_timerThread.joinable()) _timerThread.join();
  }
  catch (const std::exception &ex) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void MyNode::getTimeStruct(std::tm &timeStruct) {
  const auto timePoint = std::chrono::system_clock::now();
  std::time_t t = std::chrono::system_clock::to_time_t(timePoint);

  localtime_r(&t, &timeStruct);
}

void MyNode::tick() {
  int32_t i = 0;
  {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 60);
    int32_t randomInterval = dis(gen);
    for (i = 0; i < randomInterval; i++) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      if (_stopThread) break;
    }
  }

  int32_t sleepingTime = 1000;
  int64_t startTime = Flows::HelperFunctions::getTime();
  std::tm timeStruct{};
  while (!_stopThread) {
    try {
      for (i = 0; i < 60; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepingTime));
        if (_stopThread) break;
      }
      if (_stopThread) break;

      getTimeStruct(timeStruct);
      if (timeStruct.tm_wday == _decalcificationDay && timeStruct.tm_hour == _decalcificationHour && timeStruct.tm_min >= 0 && timeStruct.tm_min <= 10) {
        setUHeating(100);
      } else if (_enabled) {
        std::lock_guard<std::mutex> variableGuard(_variableMutex);
        _heatingController->tick();
      } else setUHeating(0);

      sleepingTime = (Flows::HelperFunctions::getTime() - startTime) / 60;
      if (sleepingTime < 500) sleepingTime = 500;
      else if (sleepingTime > 1000) sleepingTime = 1000;
      startTime = Flows::HelperFunctions::getTime();
    }
    catch (const std::exception &ex) {
      _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch (...) {
      _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
  }
}

void MyNode::setUHeating(int32_t position) {
  try {
    if (_lastValveState == position && Flows::HelperFunctions::getTime() - _lastValveStateTime < 600000) return;
    _lastValveState = position;
    _lastValveStateTime = Flows::HelperFunctions::getTime();
    Flows::PVariable message = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
    message->structValue->emplace("payload", std::make_shared<Flows::Variable>(position));
    output(0, message);
  }
  catch (const std::exception &ex) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void MyNode::input(const Flows::PNodeInfo &info, uint32_t index, const Flows::PVariable &message) {
  try {
    if (index == 0) //EN
    {
      auto enabled = (bool)*message->structValue->at("payload");
      if (enabled != _enabled) {
        _enabled = (bool)*message->structValue->at("payload");
        setNodeData("enabled", std::make_shared<Flows::Variable>(_enabled));
      }
    } else if (index == 1) //y = current temperature
    {
      _heatingController->setCurrentTemperature(message->structValue->at("payload")->floatValue);
    } else if (index == 2) //w = setpoint temperature
    {
      _heatingController->setSetPointTemperature(message->structValue->at("payload")->floatValue);
      setNodeData("w", std::make_shared<Flows::Variable>(message->structValue->at("payload")->floatValue));
    }
  }
  catch (const std::exception &ex) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

}

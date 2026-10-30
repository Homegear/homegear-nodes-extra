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
    _heatingController = std::unique_ptr<HeatingCoolingController>(new HeatingCoolingController(_out));
    _heatingController->setSetUValve(std::function<void(int32_t position)>(std::bind(&MyNode::setUHeating, this, std::placeholders::_1)));

    _coolingController = std::unique_ptr<HeatingCoolingController>(new HeatingCoolingController(_out));
    _coolingController->setSetUValve(std::function<void(int32_t position)>(std::bind(&MyNode::setUCooling, this, std::placeholders::_1)));
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

    auto settingsIterator = info->info->structValue->find("hysteresis");
    if (settingsIterator != info->info->structValue->end()) {
      auto hysteresis = settingsIterator->second;
      if (hysteresis->type == Flows::VariableType::tFloat || hysteresis->type == Flows::VariableType::tInteger || hysteresis->type == Flows::VariableType::tInteger64 || hysteresis->type == Flows::VariableType::tString) {
        if (hysteresis->type == Flows::VariableType::tFloat)
          _hysteresis = hysteresis->floatValue;
        else if (hysteresis->type == Flows::VariableType::tString)
          _hysteresis = std::stod(hysteresis->stringValue);
        else
          _hysteresis = hysteresis->integerValue64;
      }
    }

    settingsIterator = info->info->structValue->find("inputtype");
    if (settingsIterator != info->info->structValue->end()) {
      auto inputType = settingsIterator->second;
      if (inputType->type == Flows::VariableType::tBoolean || inputType->type == Flows::VariableType::tInteger || inputType->type == Flows::VariableType::tInteger64 || inputType->type == Flows::VariableType::tString) {
        if (inputType->type == Flows::VariableType::tBoolean)
          _inputType = inputType->booleanValue;
        else if (inputType->type == Flows::VariableType::tString)
          _inputType = std::stoi(inputType->stringValue);
        else
          _inputType = inputType->integerValue64;

      }
    }

    std::string heatingType;
    settingsIterator = info->info->structValue->find("heatingtype");
    if (settingsIterator != info->info->structValue->end()) heatingType = settingsIterator->second->stringValue;

    settingsIterator = info->info->structValue->find("decalcificationday");
    if (settingsIterator != info->info->structValue->end()) _decalcificationDay = Flows::Math::getNumber(settingsIterator->second->stringValue);

    settingsIterator = info->info->structValue->find("decalcificationhour");
    if (settingsIterator != info->info->structValue->end()) _decalcificationHour = Flows::Math::getNumber(settingsIterator->second->stringValue);

    if (_decalcificationDay < 0 || _decalcificationDay > 6) _decalcificationDay = 5;
    if (_decalcificationHour < 0 || _decalcificationHour > 23) _decalcificationHour = 11;

    HeatingCoolingController::ControllerSettings heatingControllerSettings;
    if (heatingType == "radiator") {
      heatingControllerSettings.Kp = 50.0; // P gain in % valve state per °C
      heatingControllerSettings.TN = 3600.0; // I time in seconds (kind of time constant)
      heatingControllerSettings.Td = 0.0; // D time in seconds (duration of "compensation shot" in seconds)
      heatingControllerSettings.RC_w_LP = 60.0; // LP time constant for w in seconds
      heatingControllerSettings.y_LP_RC = 60.0; // LP time constant for y in seconds (should be <180 to avoid instability)
      heatingControllerSettings.uvalve_LP_RC = 60.0; // LP time constant for uvalve in seconds (should be <180 to avoid instability)
    } else if (heatingType == "castironradiator") {
      heatingControllerSettings.Kp = 50.0;
      heatingControllerSettings.TN = 7200.0;
      heatingControllerSettings.Td = 0.0;
      heatingControllerSettings.RC_w_LP = 600.0;
      heatingControllerSettings.y_LP_RC = 60.0;
      heatingControllerSettings.uvalve_LP_RC = 60.0;
    } else if (heatingType == "floorheating") {
      heatingControllerSettings.Kp = 50.0;
      heatingControllerSettings.TN = 14400.0;
      heatingControllerSettings.Td = 0.0;
      heatingControllerSettings.RC_w_LP = 1200.0;
      heatingControllerSettings.y_LP_RC = 60.0;
      heatingControllerSettings.uvalve_LP_RC = 60.0;
    } else //manual
    {
      double value = 0;
      settingsIterator = info->info->structValue->find("heatingkp");
      if (settingsIterator != info->info->structValue->end()) value = Flows::Math::getDouble(settingsIterator->second->stringValue);
      if (value < 0.1) value = 0.1;
      heatingControllerSettings.Kp = value;

      value = 0;
      settingsIterator = info->info->structValue->find("heatingtn");
      if (settingsIterator != info->info->structValue->end()) value = Flows::Math::getDouble(settingsIterator->second->stringValue);
      if (value < 0.1) value = 0.1;
      heatingControllerSettings.TN = value;

      value = 0;
      settingsIterator = info->info->structValue->find("heatingtd");
      if (settingsIterator != info->info->structValue->end()) value = Flows::Math::getDouble(settingsIterator->second->stringValue);
      if (value < 0.1) value = 0.1;
      heatingControllerSettings.Td = value;

      value = 0;
      settingsIterator = info->info->structValue->find("heatingrcwlp");
      if (settingsIterator != info->info->structValue->end()) value = Flows::Math::getDouble(settingsIterator->second->stringValue);
      if (value < 0.1) value = 0.1;
      heatingControllerSettings.RC_w_LP = value;

      value = 0;
      settingsIterator = info->info->structValue->find("heatingylprc");
      if (settingsIterator != info->info->structValue->end()) value = Flows::Math::getDouble(settingsIterator->second->stringValue);
      if (value < 0.1) value = 0.1;
      heatingControllerSettings.y_LP_RC = value;

      value = 0;
      settingsIterator = info->info->structValue->find("heatingulprc");
      if (settingsIterator != info->info->structValue->end()) value = Flows::Math::getDouble(settingsIterator->second->stringValue);
      if (value < 0.1) value = 0.1;
      heatingControllerSettings.uvalve_LP_RC = value;
    }

    HeatingCoolingController::ControllerSettings coolingControllerSettings;
    if (heatingType == "ac") {
      coolingControllerSettings.Kp = 50.0; // P gain in % valve state per °C
      coolingControllerSettings.TN = 3600.0; // I time in seconds (kind of time constant)
      coolingControllerSettings.Td = 0.0; // D time in seconds (duration of "compensation shot" in seconds)
      coolingControllerSettings.RC_w_LP = 60.0; // LP time constant for w in seconds
      coolingControllerSettings.y_LP_RC = 60.0; // LP time constant for y in seconds (should be <180 to avoid instability)
      coolingControllerSettings.uvalve_LP_RC = 60.0; // LP time constant for uvalve in seconds (should be <180 to avoid instability)
    } else if (heatingType == "floorcooling") {
      coolingControllerSettings.Kp = 50.0;
      coolingControllerSettings.TN = 14400.0;
      coolingControllerSettings.Td = 0.0;
      coolingControllerSettings.RC_w_LP = 1200.0;
      coolingControllerSettings.y_LP_RC = 60.0;
      coolingControllerSettings.uvalve_LP_RC = 60.0;
    } else //manual
    {
      double value = 0;
      settingsIterator = info->info->structValue->find("coolingkp");
      if (settingsIterator != info->info->structValue->end()) value = Flows::Math::getDouble(settingsIterator->second->stringValue);
      if (value < 0.1) value = 0.1;
      coolingControllerSettings.Kp = value;

      value = 0;
      settingsIterator = info->info->structValue->find("coolingtn");
      if (settingsIterator != info->info->structValue->end()) value = Flows::Math::getDouble(settingsIterator->second->stringValue);
      if (value < 0.1) value = 0.1;
      coolingControllerSettings.TN = value;

      value = 0;
      settingsIterator = info->info->structValue->find("coolingtd");
      if (settingsIterator != info->info->structValue->end()) value = Flows::Math::getDouble(settingsIterator->second->stringValue);
      if (value < 0.1) value = 0.1;
      coolingControllerSettings.Td = value;

      value = 0;
      settingsIterator = info->info->structValue->find("coolingrcwlp");
      if (settingsIterator != info->info->structValue->end()) value = Flows::Math::getDouble(settingsIterator->second->stringValue);
      if (value < 0.1) value = 0.1;
      coolingControllerSettings.RC_w_LP = value;

      value = 0;
      settingsIterator = info->info->structValue->find("coolingylprc");
      if (settingsIterator != info->info->structValue->end()) value = Flows::Math::getDouble(settingsIterator->second->stringValue);
      if (value < 0.1) value = 0.1;
      coolingControllerSettings.y_LP_RC = value;

      value = 0;
      settingsIterator = info->info->structValue->find("coolingulprc");
      if (settingsIterator != info->info->structValue->end()) value = Flows::Math::getDouble(settingsIterator->second->stringValue);
      if (value < 0.1) value = 0.1;
      coolingControllerSettings.uvalve_LP_RC = value;
    }

    _heatingController->unserialize(getNodeData("heatingcontroller"));
    _coolingController->unserialize(getNodeData("coolingcontroller"));

    auto setting = getNodeData("enabled");
    if (setting->type == Flows::VariableType::tBoolean) _enabled = setting->booleanValue;

    setting = getNodeData("w");
    if (setting->type == Flows::VariableType::tFloat) {
      _heatingController->setSetPointTemperature(setting->floatValue);
      _coolingController->setSetPointTemperature(setting->floatValue);
    }

    setting = getNodeData("y");
    if (setting->type == Flows::VariableType::tFloat) {
      _heatingController->setCurrentTemperature(setting->floatValue);
      _coolingController->setCurrentTemperature(setting->floatValue);
    }

    _heatingController->setControllerSettings(heatingControllerSettings);
    _coolingController->setControllerSettings(heatingControllerSettings);

    auto type = getNodeData("hvactype");
    if (type->type == Flows::VariableType::tInteger || type->type == Flows::VariableType::tInteger64 || type->type == Flows::VariableType::tString) {
      if (type->type == Flows::VariableType::tString)
        _type = std::stoi(type->stringValue);
      else
        _type = type->integerValue64;
    }

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
    setNodeData("coolingcontroller", _coolingController->serialize());
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
        setUCooling(0);
      } else if (_enabled) {
        std::lock_guard<std::mutex> variableGuard(_variableMutex);
        if (_type == 0 || _type == 1) {
          _heatingController->tick();
        } else {
          setUHeating(0);
        }
        if (_type == 0 || _type == 2) {
          _coolingController->tick();
        } else {
          setUCooling(100);
        }
      } else {
        setUHeating(0);
        setUCooling(100);
      }

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
    if (_type == 2) position = 0;
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

void MyNode::setUCooling(int32_t position) {
  try {
    position = 100 - position;
    if (_type == 1) position = 0;
    if (_lastCoolingState == position && Flows::HelperFunctions::getTime() - _lastCoolingStateTime < 600000) return;
    _lastCoolingState = position;
    _lastCoolingStateTime = Flows::HelperFunctions::getTime();
    Flows::PVariable message = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
    message->structValue->emplace("payload", std::make_shared<Flows::Variable>(position));
    output(1, message);
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

        if (_enabled && !_inputType) {
          setHvacType(_heatingController->getCurrentTemperature(), _heatingController->getSetPointTemperature());
        }
      }
    } else if (index == 1) //y = current temperature
    {
      const double curTemperature = message->structValue->at("payload")->floatValue;
      _heatingController->setCurrentTemperature(curTemperature);
      _coolingController->setCurrentTemperature(curTemperature);

      // check against setpoint temperature and hysteresis
      if (_enabled && !_inputType) // can switch only if the type is set to 'node'
      {
        setHvacType(curTemperature, _heatingController->getSetPointTemperature());
      }
    } else if (index == 2) //w = setpoint temperature
    {
      const double setpointTemperature = message->structValue->at("payload")->floatValue;
      _heatingController->setSetPointTemperature(setpointTemperature);
      _coolingController->setSetPointTemperature(setpointTemperature);
      setNodeData("w", std::make_shared<Flows::Variable>(setpointTemperature));

      // check against current temperature and hysteresis
      if (_enabled && !_inputType) // can switch only if the type is set to 'node'
      {
        setHvacType(_heatingController->getCurrentTemperature(), setpointTemperature);
      }
    } else if (index == 3) // tI = hvac type
    {
      if (_inputType) {
        setHvacType(message->structValue->at("payload")->integerValue64);
      }
    }
  }
  catch (const std::exception &ex) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void MyNode::setHvacType(int type) {
  if (type < 0 || type > 2) return;
  _type = type;
  setNodeData("hvactype", std::make_shared<Flows::Variable>(_type));

  auto typeMessage = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
  typeMessage->structValue->emplace("payload", std::make_shared<Flows::Variable>(_type));

  output(2, typeMessage);
}

void MyNode::setHvacType(double curTemperature, double setpointTemperature) {
  if (curTemperature > setpointTemperature + (_hysteresis / 2)) // the temperature is too big
  {
    // don't switch again if already switched
    if (_type == 2) return;

    // switch to cooling
    setHvacType(2);
  } else if (curTemperature < setpointTemperature - (_hysteresis / 2)) {
    // don't switch again if already switched
    if (_type == 1) return;

    // switch to warming
    setHvacType(1);
  }
}

}

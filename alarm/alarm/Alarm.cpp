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

#include "Alarm.h"

namespace Alarm {

Alarm::Alarm(const std::string &path, const std::string &type, const std::atomic_bool *frontendConnected) : Flows::INode(path, type, frontendConnected) {
}

Alarm::~Alarm() = default;

bool Alarm::init(const Flows::PNodeInfo &info) {
  return true;
}

bool Alarm::start() {
  try {
    auto currentMode = getNodeData("currentMode");
    if (currentMode->type == Flows::VariableType::tInteger || currentMode->type == Flows::VariableType::tInteger64) {
      _currentMode = currentMode->integerValue64;
    }

    auto previousMode = getNodeData("previousMode");
    if (previousMode->type == Flows::VariableType::tInteger || previousMode->type == Flows::VariableType::tInteger64) {
      _previousMode = previousMode->integerValue64;
    }

    auto windowOpen = getNodeData("windowOpen");
    if (currentMode->type == Flows::VariableType::tBoolean) {
      _windowOpen = currentMode->booleanValue;
    }

    auto setpointTemperatures = getNodeData("setpointTemperatures")->arrayValue;
    for (int32_t i = 0; i < (signed)setpointTemperatures->size(); i++) {
      if (i >= (signed)_setpointTemperatures.size()) break;
      _setpointTemperatures.at(i) = setpointTemperatures->at(i)->floatValue;
    }

    return true;
  }
  catch (const std::exception &ex) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return false;
}

void Alarm::startUpComplete() {
}

void Alarm::stop() {
}

void Alarm::waitForStop() {
}

void Alarm::input(const Flows::PNodeInfo &info, uint32_t index, const Flows::PVariable &message) {
  try {
    if (index == 0) //mI = mode input
    {
      auto mode = message->structValue->at("payload")->integerValue64;
      if (mode >= 0 && mode <= 10) {
        _previousMode = mode; //Keep mode settable even if window is open (i. e. for central functions)
        setNodeData("previousMode", message->structValue->at("payload"));
        if (!_windowOpen) {
          _currentMode = mode;
          auto currentMode = std::make_shared<Flows::Variable>((int64_t)_currentMode);
          auto modeMessage = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
          modeMessage->structValue->emplace("payload", currentMode);
          setNodeData("currentMode", currentMode);

          auto temperatureMessage = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
          temperatureMessage->structValue->emplace("payload", std::make_shared<Flows::Variable>(_setpointTemperatures.at(_currentMode)));

          output(0, temperatureMessage);
          output(1, modeMessage);
        }
      }
    } else if (index == 1) //wI = setpoint temperature
    {
      _setpointTemperatures.at(_currentMode) = message->structValue->at("payload")->floatValue;

      auto setpointTemperatures = std::make_shared<Flows::Variable>(Flows::VariableType::tArray);
      setpointTemperatures->arrayValue->reserve(_setpointTemperatures.size());
      for (double &setpointTemperature : _setpointTemperatures) {
        setpointTemperatures->arrayValue->emplace_back(std::move(std::make_shared<Flows::Variable>(setpointTemperature)));
      }
      setNodeData("setpointTemperatures", setpointTemperatures);

      auto temperatureMessage = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
      temperatureMessage->structValue->emplace("payload", std::make_shared<Flows::Variable>(_setpointTemperatures.at(_currentMode)));

      output(0, temperatureMessage);
    } else if (index == 2) //WIN = window
    {
      _windowOpen = (bool)(*message->structValue->at("payload"));
      setNodeData("windowOpen", std::make_shared<Flows::Variable>(_windowOpen));

      _currentMode = _windowOpen ? 0 : _previousMode;
      auto currentMode = std::make_shared<Flows::Variable>((int64_t)_currentMode);
      setNodeData("currentMode", currentMode);
      auto modeMessage = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
      modeMessage->structValue->emplace("payload", currentMode);

      auto temperatureMessage = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
      temperatureMessage->structValue->emplace("payload", std::make_shared<Flows::Variable>(_setpointTemperatures.at(_currentMode)));

      output(0, temperatureMessage);
      output(1, modeMessage);
    }
  }
  catch (const std::exception &ex) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

}

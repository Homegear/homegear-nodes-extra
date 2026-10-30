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

namespace MyNode
{

MyNode::MyNode(const std::string &path, const std::string &type, const std::atomic_bool *frontendConnected)
: Flows::INode(path, type, frontendConnected)
{
}

MyNode::~MyNode()
{
    delete controller;
}

bool MyNode::init(const Flows::PNodeInfo &info)
{

    try
    {

        auto settingsIterator = info->info->structValue->find("shadingType");
        if (settingsIterator != info->info->structValue->end())
        {
            auto shadingType = settingsIterator->second;
            if (shadingType->type == Flows::VariableType::tInteger || shadingType->type == Flows::VariableType::tInteger64 || shadingType->type == Flows::VariableType::tString)
            {
                if (shadingType->type == Flows::VariableType::tString)
                    _shadingType = std::stoi(shadingType->stringValue);
                else
                    _shadingType = shadingType->integerValue64;
            }
        }

        settingsIterator = info->info->structValue->find("totalTimeMovingUp");
        if (settingsIterator != info->info->structValue->end())
        {
            auto totalTimeMovingUp = settingsIterator->second;
            if (totalTimeMovingUp->type == Flows::VariableType::tFloat || totalTimeMovingUp->type == Flows::VariableType::tInteger || totalTimeMovingUp->type == Flows::VariableType::tInteger64 || totalTimeMovingUp->type == Flows::VariableType::tString)
            {
                if (totalTimeMovingUp->type == Flows::VariableType::tFloat)
                    _totalTimeMovingUp = totalTimeMovingUp->floatValue;
                else if (totalTimeMovingUp->type == Flows::VariableType::tString)
                    _totalTimeMovingUp = std::stod(totalTimeMovingUp->stringValue);
                else
                    _totalTimeMovingUp = totalTimeMovingUp->integerValue64;
            }
        }

        settingsIterator = info->info->structValue->find("totalTimeMovingDown");
        if (settingsIterator != info->info->structValue->end())
        {
            auto totalTimeMovingDown = settingsIterator->second;
            if (totalTimeMovingDown->type == Flows::VariableType::tFloat || totalTimeMovingDown->type == Flows::VariableType::tInteger || totalTimeMovingDown->type == Flows::VariableType::tInteger64 || totalTimeMovingDown->type == Flows::VariableType::tString)
            {
                if (totalTimeMovingDown->type == Flows::VariableType::tFloat)
                    _totalTimeMovingDown = totalTimeMovingDown->floatValue;
                else if (totalTimeMovingDown->type == Flows::VariableType::tString)
                    _totalTimeMovingDown = std::stod(totalTimeMovingDown->stringValue);
                else
                    _totalTimeMovingDown = totalTimeMovingDown->integerValue64;
            }
        }

        settingsIterator = info->info->structValue->find("deadTime");
        if (settingsIterator != info->info->structValue->end())
        {
            auto deadTime = settingsIterator->second;
            if (deadTime->type == Flows::VariableType::tFloat || deadTime->type == Flows::VariableType::tInteger || deadTime->type == Flows::VariableType::tInteger64 || deadTime->type == Flows::VariableType::tString)
            {
                if (deadTime->type == Flows::VariableType::tFloat)
                    _deadTime = deadTime->floatValue;
                else if (deadTime->type == Flows::VariableType::tString)
                    _deadTime = std::stod(deadTime->stringValue);
                else
                    _deadTime = deadTime->integerValue64;
            }
        }

        settingsIterator = info->info->structValue->find("startTime");
        if (settingsIterator != info->info->structValue->end())
        {
            auto startTime = settingsIterator->second;
            if (startTime->type == Flows::VariableType::tFloat || startTime->type == Flows::VariableType::tInteger || startTime->type == Flows::VariableType::tInteger64 || startTime->type == Flows::VariableType::tString)
            {
                if (startTime->type == Flows::VariableType::tFloat)
                    _startTime = startTime->floatValue;
                else if (startTime->type == Flows::VariableType::tString)
                    _startTime = std::stod(startTime->stringValue);
                else
                    _startTime = startTime->integerValue64;
            }
        }


        settingsIterator = info->info->structValue->find("slatAdjustTime");
        if (settingsIterator != info->info->structValue->end())
        {
            auto slatAdjustTime = settingsIterator->second;
            if (slatAdjustTime->type == Flows::VariableType::tFloat || slatAdjustTime->type == Flows::VariableType::tInteger || slatAdjustTime->type == Flows::VariableType::tInteger64 || slatAdjustTime->type == Flows::VariableType::tString)
            {
                if (slatAdjustTime->type == Flows::VariableType::tFloat)
                    _slatAdjustTime = slatAdjustTime->floatValue;
                else if (slatAdjustTime->type == Flows::VariableType::tString)
                    _slatAdjustTime = std::stod(slatAdjustTime->stringValue);
                else
                    _slatAdjustTime = slatAdjustTime->integerValue64;
            }
        }

        settingsIterator = info->info->structValue->find("stepsNumber");
        if (settingsIterator != info->info->structValue->end())
        {
            auto stepsNumber = settingsIterator->second;
            if (stepsNumber->type == Flows::VariableType::tInteger || stepsNumber->type == Flows::VariableType::tInteger64 || stepsNumber->type == Flows::VariableType::tString)
            {
                if (stepsNumber->type == Flows::VariableType::tString)
                    _stepsNumber = std::stoi(stepsNumber->stringValue);
                else
                    _stepsNumber = stepsNumber->integerValue64;
            }
        }


        settingsIterator = info->info->structValue->find("actionAfterSecurity");
        if (settingsIterator != info->info->structValue->end())
        {
            auto actionAfterSecurity = settingsIterator->second;
            if (actionAfterSecurity->type == Flows::VariableType::tInteger || actionAfterSecurity->type == Flows::VariableType::tInteger64 || actionAfterSecurity->type == Flows::VariableType::tString)
            {
                if (actionAfterSecurity->type == Flows::VariableType::tString)
                    _actionAfterSecurity = std::stoi(actionAfterSecurity->stringValue);
                else
                    _actionAfterSecurity = actionAfterSecurity->integerValue64;
            }
        }




        auto position = getNodeData("position");
        if (position->type == Flows::VariableType::tInteger || position->type == Flows::VariableType::tInteger64)
        {
            _position = position->integerValue64;
        }

        auto securityPosition = getNodeData("securityPosition");
        if (securityPosition->type == Flows::VariableType::tBoolean)
        {
            _securityPosition = securityPosition->booleanValue;
        }



        auto deactivate = getNodeData("deactivate");
        if (deactivate->type == Flows::VariableType::tBoolean)
        {
            _deactivate = deactivate->booleanValue;
        }

        auto slatsPosition = getNodeData("slatsPosition");
        if (slatsPosition->type == Flows::VariableType::tInteger || slatsPosition->type == Flows::VariableType::tInteger64)
        {
            _slatsPosition = slatsPosition->integerValue64;
        }


        delete controller;
        if (_shadingType == 0)
        {
            controller = new RollerShutterController(this);
        }
        else
        {
            BlindController *newController = new BlindController(this);

            newController->_slatAdjustTime = _slatAdjustTime;
            newController->_stepsNumber = _stepsNumber;

            controller = newController;
        }

        controller->_totalTimeMovingUp = _totalTimeMovingUp;
        controller->_totalTimeMovingDown = _totalTimeMovingDown;
        controller->_deadTime = _deadTime;
        controller->_startTime = _startTime;

        controller->_actionAfterSecurity = _actionAfterSecurity;

        return true;
    }
    catch (const std::exception &ex)
    {
        _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return false;
}

bool MyNode::start()
{
    if (controller)
        controller->StartThread();

    return true;
}

void MyNode::startUpComplete()
{
}

void MyNode::stop()
{
    if (controller)
    {
        controller->SignalToStopThread();
    }
}

void MyNode::waitForStop()
{
    if (controller)
    {
        controller->WaitForStopThread();

    }
}

void MyNode::input(const Flows::PNodeInfo &info, uint32_t index, const Flows::PVariable &message)
{
    try
    {
        // output is 0 - up, 1 - down, 2 - stop, 3 - position, 4 - slats up, 5 - slats down, 6 - slats position
        if (index == 0 && !_securityPosition) // up
        {
            if (!_deactivate)
            {
                const bool pressed = (bool)*message->structValue->at("payload");
                if (controller)
                {
                    controller->SignalEvent(pressed ? Event::PressUp : Event::ReleaseUp);
                }
            }
        }
        else if (index == 1 && !_securityPosition) // down
        {
            if (!_deactivate)
            {
                const bool pressed = (bool)*message->structValue->at("payload");
                if (controller)
                {
                    controller->SignalEvent(pressed ? Event::PressDown : Event::ReleaseDown);
                }
            }
        }
        else if (index == 2 && !_securityPosition) // stop
        {
            if (!_deactivate)
            {
                const bool pressed = (bool)*message->structValue->at("payload");

                if (controller)
                {
                    controller->SignalEvent(pressed ? Event::PressStop : Event::ReleaseStop);
                }
            }
        }
        else if (index == 3 && !_securityPosition) // set position
        {
            if (!_deactivate)
            {
                const int position = message->structValue->at("payload")->integerValue64;
                _position = position;
                setNodeData("position", std::make_shared<Flows::Variable>(position));

                if (controller)
                    controller->SignalEvent(Event::PositionChange);

            }
        }
        else if (index == 4) // security position
        {
            // security goes through even if deactivated
            const bool pressed = (bool)*message->structValue->at("payload");
            _securityPosition = pressed;
            setNodeData("securityPosition", std::make_shared<Flows::Variable>(pressed));

            if (controller)
            {
                controller->SignalEvent(pressed ? Event::SecurityUp : Event::SecurityDown);
            }
        }
        else if (index == 5) // deactivate
        {
            const bool pressed = (bool)*message->structValue->at("payload");
            _deactivate = pressed;
            setNodeData("deactivate", std::make_shared<Flows::Variable>(pressed));

            // do nothing
        }
        else if (index == 6 && !_securityPosition) // slats up
        {
            if (!_deactivate)
            {
                const bool pressed = (bool)*message->structValue->at("payload");
                if (controller)
                {
                    controller->SignalEvent(pressed ? Event::PressSlatsUp : Event::ReleaseSlatsUp);
                }
            }
        }
        else if (index == 7 && !_securityPosition) // slats down
        {
            if (!_deactivate)
            {
                const bool pressed = (bool)*message->structValue->at("payload");
                if (controller)
                {
                    controller->SignalEvent(pressed ? Event::PressSlatsDown : Event::ReleaseSlatsDown);
                }
            }
        }
        else if (index == 8 && !_securityPosition) // set position of slats
        {
            if (!_deactivate && _shadingType != 0)
            {
                const int position = message->structValue->at("payload")->integerValue64;
                _slatsPosition = position;
                setNodeData("slatsPosition", std::make_shared<Flows::Variable>(position));

                if (controller)
                    controller->SignalEvent(Event::PositionSlatsChange);
            }
        }
    }
    catch (const std::exception &ex)
    {
        _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

}

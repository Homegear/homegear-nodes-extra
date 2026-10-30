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

#include <homegear-node/INode.h>

#include <mutex>
#include <thread>
#include <random>
#include <atomic>
#include <chrono>

#include "ShadingController.h"

namespace MyNode
{

class MyNode: public Flows::INode
{
public:
	MyNode(const std::string &path, const std::string &type, const std::atomic_bool* frontendConnected);
	~MyNode() override;

	bool init(const Flows::PNodeInfo &info);
	bool start() override;
	void startUpComplete() override;
	void stop() override;
	void waitForStop() override;

//#define DEBUG_LOG 1

#ifdef DEBUG_LOG
    std::atomic_int logCnt{0};
#endif

    void log(int32_t logLevel, const std::string &message)
    {
#ifdef DEBUG_LOG
        Flows::INode::log(logLevel, "Nr: " + std::to_string(++logCnt) + " " + message);
#endif
    }

    void frontendEventLog(const std::string &message)
    {
#ifdef DEBUG_LOG
        Flows::INode::frontendEventLog(message);
#endif
    }
private:
    // parameters
    int _shadingType = 0; // 0 - roller shutter, 1 - blinds (can there be more?)

    double _totalTimeMovingUp = 0.;
    double _totalTimeMovingDown = 0.;
    double _deadTime = 0.;
    double _startTime = 0.;

    // only for blinds:
    double _slatAdjustTime = 0.;
    int _stepsNumber = 1;
    // end only for blinds

    int _actionAfterSecurity = 0; // 0 = last position, 1 = move down, 2 = do nothing

    // inputs / outputs
    bool _deactivate = false;

public:
    std::atomic<int> _position{0};

    std::atomic<int> _slatsPosition{0};

    // just to be able to call it from the controller
    void output(int outIndex, const Flows::PVariable& msg, bool sync = false)
    {
        Flows::INode::output(outIndex, msg, sync);
    }

private:
    std::atomic_bool _securityPosition{false};

	void input(const Flows::PNodeInfo &info, uint32_t index, const Flows::PVariable &message) override;

	ShadingController* controller = nullptr;
};

}

#endif

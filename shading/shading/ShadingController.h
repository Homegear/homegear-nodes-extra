/* Copyright 2013-2021 Homegear GmbH
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


#ifndef SHADING_CONTROLLER_H_
#define SHADING_CONTROLLER_H_

#include <thread>
#include <array>
#include <atomic>
#include <chrono>

#include "InputEvent.h"

#include "StateMachine.h"


namespace MyNode
{

class MyNode;


// TODO: Common functionality in shading controller, the different ones in the derived classes

class ShadingController
{
public:
    ShadingController(MyNode *node);
    virtual ~ShadingController();

    virtual void StartThread();
    virtual void SignalToStopThread();
    virtual void WaitForStopThread();

    void SignalEvent(const Event& e);

    double _totalTimeMovingUp = 0.;
    double _totalTimeMovingDown = 0.;
    double _deadTime = 0.;
    double _startTime = 0.;

    int _actionAfterSecurity = 0; // 0 = last position, 1 = move down, 2 = do nothing

    // TODO: make up different events for the state machine, more detailed than the ones incoming from the input?

    enum class State : unsigned int
    {
        // the simplest state, just wait for some event
        Idle = 0,

        // from those it can transition out either through timeout events (if move time is specified) or when the input is released otherwise
        MovingUp, // a command was sent (as in Up, Down) and it's currently waiting for completion, while updating position
        MovingDown,
        MovingUnknownDirection, // do not know the direction (there can be such cases) so next time better wait deadtime until next move

        SlatsClosing,
        SlatsOpening,
        SlatsClosingOrOpening // do not know the direction (there can be such cases) so next time better wait deadtime until next move
    };

    enum class WaitingState : unsigned int
    {
        NotWaiting = 0,
        WaitingForDeadtime,
        WaitingForStarttime,
        WaitingForMoving
    };

    enum class Direction : unsigned int
    {
        Unknown = 0,
        Up,
        Down
    };


protected:
    virtual void Tick();

    virtual bool NeedsToWaitForAnotherEvent(const Event& e) = 0;
    virtual WaitingState NeedsToWaitForATimeout(const Event& e, const std::chrono::milliseconds& timeDif, Direction lastDir, bool wasDeadtime = false);
    virtual std::chrono::milliseconds HowMuchTimeToWait(const Event& e, const std::chrono::milliseconds& timeDif);

    void SetStopOutputFalse();

    virtual void Forward(const Event& e) {} // for forwarding to derived objects

    MyNode* m_node;
    bool hasSlats = false;
    std::atomic<WaitingState> waitingState{WaitingState::NotWaiting};

private:
    void ThreadLoop();

    void TickThreadLoop();

    bool EventCommandingToMove(const Event& e) const;

    void SetTargetPositionTotallyUp();
    void SetTargetPositionTotallyDown();

    bool IsTotallyUp() const
    {
        return lastPosition == 0;
    }

    bool IsTotallyDown() const
    {
        return lastPosition == 100;
    }

    int ComputeCurrentPosition(const std::chrono::milliseconds& timeDif, Direction dir) const
    {
        if (dir == Direction::Unknown || lastPosition == -1) return -1;
        else if (timeDif.count() == 0) return lastPosition; // no time to move

        int pos = 0;

        if (dir == Direction::Up)
        {
            if (_totalTimeMovingUp == 0) return -1; // can't compute position, so return unknown

            const double fractionMoved = timeDif.count() / (_totalTimeMovingUp * 1000.);

            pos = lastPosition - fractionMoved * 100.;
        }
        else // down
        {
            if (_totalTimeMovingDown == 0) return -1; // can't compute position, so return unknown

            const double fractionMoved = timeDif.count() / (_totalTimeMovingDown * 1000.);

            pos = lastPosition + fractionMoved * 100.;
        }

        if (pos > 100) return 100;
        else if (pos < 0) return 0;

        return pos;
    }

    // negative means 'I don't know'
    double TimeToMove() const
    {
        if (lastPosition == -1 && targetPosition == -1) return -1;
        else if (lastPosition == -1)
        {
            if (targetPosition == 0)
            {
                // up
                if (_totalTimeMovingUp == 0) return -1;

                return _totalTimeMovingUp;
            }
            else if (targetPosition == 100)
            {
                // down
                if (_totalTimeMovingDown == 0) return -1;

                return _totalTimeMovingDown;
            }
        }

        const int dif = targetPosition - lastPosition;

        double res = 0;
        if (dif > 0) // moving down
        {
            if (_totalTimeMovingDown == 0) return -1;

            res = _totalTimeMovingDown * dif / 100.;
        }
        else // moving up
        {
            if (_totalTimeMovingUp == 0) return -1;

            res = -_totalTimeMovingUp * dif / 100.;
        }

        return res;
    }

    Direction lastDirection() const
    {
        if (targetPosition == 0) return Direction::Up;
        else if (targetPosition == 100) return Direction::Down;
        else if (targetPosition == -1 || lastPosition == -1) return Direction::Unknown;

        const int dif = targetPosition - lastPosition;

        if (0 == dif) return prevDirection;

        return dif < 0 ? Direction::Up : Direction::Down;
    }

    Direction eventDirection(const Event& e)
    {
        switch(e)
        {
            case Event::PressUp:
            case Event::ReleaseUp:
            case Event::SecurityUp:
                return Direction::Up;

            case Event::PressDown:
            case Event::ReleaseDown:
            case Event::SecurityDown:
                return Direction::Down;

            case Event::PositionChange:
                return lastDirection();

            default:
                break;
        }

        return Direction::Unknown;
    }

    void OutputPosition(int pos);



    InputEvent inputEvent;

    std::array<bool, 3> outputs{false, false, true}; // stop is true

    std::atomic_int lastPositionBeforeSecurity{-1};
    std::atomic_int lastPosition{-1}; // 0 - totally opened (up), 100 totally closed (down), -1 unknown ?
    std::atomic_int targetPosition{0}; // set target position at the beginning of the command, maybe it's stopped before ending it

    std::atomic<std::chrono::system_clock::time_point> lastEventTime;

    bool inSecurity = false;

    std::thread m_thread;

    std::atomic_bool m_stopTickThread{false};
    std::thread m_tickThread; // the only purpose of this is to update output for position(s)

    StateMachine<State, Event> stateMachine; // TODO: set different state machines depending on _totalTimeMovingUp and _totalTimeMovingDown settings? Also if _deadTime is set to zero it can get much easier

    int oldPosition = -1; // for tick thread

    std::atomic<Direction> prevDirection{Direction::Unknown};
};


class RollerShutterController : public ShadingController
{
public:
    RollerShutterController(MyNode *node);

protected:
    virtual bool NeedsToWaitForAnotherEvent(const Event& e) override;
};


class BlindController : public ShadingController
{
public:
    BlindController(MyNode *node);

    // only for blinds:
    double _slatAdjustTime = 0.;
    int _stepsNumber = 1;
    double _longTimeClick = 0.5;

    virtual void StartThread() override;
    virtual void SignalToStopThread() override;
    virtual void WaitForStopThread() override;

    enum class SlatsWaitingState : unsigned int
    {
        SlatsNotWaiting = 0,
        SlatsWaitingForStarttime,
        SlatsWaitingForMoving
    };

    enum class SlatsDirection : unsigned int
    {
        UnknownSlatsDir = 0,
        Closing,
        Opening
    };



protected:
    virtual bool NeedsToWaitForAnotherEvent(const Event& e) override;
    virtual WaitingState NeedsToWaitForATimeout(const Event& e, const std::chrono::milliseconds& timeDif, Direction lastDir, bool wasDeadtime = false) override;
    virtual std::chrono::milliseconds HowMuchTimeToWait(const Event& e, const std::chrono::milliseconds& timeDif) override;
    virtual void Tick() override;

    void OutputSlatsPosition(int pos);

    virtual void Forward(const Event& e) override;

    void SlatsThreadLoop();

    static int TotallyClosed()
    {
        return 100;
    }

    static int TotallyOpened()
    {
        return 0;
    }

    bool IsTotallyClosed() const
    {
        return lastSlatsPosition == TotallyClosed();
    }

    bool IsTotallyOpened() const
    {
        return lastSlatsPosition == TotallyOpened();
    }

    double SlatsStepLength() const
    {
        const int stepsNumber = _stepsNumber == 0 ? 1 : _stepsNumber;

        return _slatAdjustTime / stepsNumber;
    }

    SlatsDirection eventSlatsDirection(const Event& e)
    {
        switch(e)
        {
            case Event::PressUp:
            case Event::ReleaseUp:
                return SlatsDirection::Opening;

            case Event::PressDown:
            case Event::ReleaseDown:
                return SlatsDirection::Closing;

            case Event::PositionChange:
                return lastSlatsDirection();

            default:
                break;
        }

        return SlatsDirection::UnknownSlatsDir;
    }

    SlatsDirection lastSlatsDirection() const
    {
        if (targetSlatsPosition == 0) return SlatsDirection::Opening;
        else if (targetSlatsPosition == 100) return SlatsDirection::Closing;
        else if (targetSlatsPosition == -1 || lastSlatsPosition == -1) return SlatsDirection::UnknownSlatsDir;

        const int dif = targetSlatsPosition - lastSlatsPosition;

        if (0 == dif) return prevSlatsDirection;

        return dif < 0 ? SlatsDirection::Opening : SlatsDirection::Closing;
    }

    int ComputeCurrentSlatsPosition(const std::chrono::milliseconds& timeDif, SlatsDirection dir) const
    {
        if (dir == SlatsDirection::UnknownSlatsDir || lastSlatsPosition == -1) return -1;
        else if (timeDif.count() == 0) return lastSlatsPosition; // no time to move

        int pos = 0;

        if (dir == SlatsDirection::Opening) // decreasing
        {
            if (_slatAdjustTime == 0) return -1; // can't compute position, so return unknown

            const double fractionMoved = timeDif.count() / (_slatAdjustTime * 1000.);

            pos = lastSlatsPosition - fractionMoved * 100.;
        }
        else // closing - so growing
        {
            if (_slatAdjustTime == 0) return -1; // can't compute position, so return unknown

            const double fractionMoved = timeDif.count() / (_slatAdjustTime * 1000.);

            pos = lastSlatsPosition + fractionMoved * 100.;
        }

        if (pos > 100) return 100;
        else if (pos < 0) return 0;

        return pos;
    }

    // negative means 'I don't know'
    double TimeToMoveSlats() const
    {
        if (lastSlatsPosition == -1 && targetSlatsPosition == -1) return -1;
        else if (lastSlatsPosition == -1)
        {
            if (targetSlatsPosition == 0 || targetSlatsPosition == 100)
            {
                // up
                if (_slatAdjustTime == 0) return -1;

                return _slatAdjustTime;
            }
        }

        const int dif = targetSlatsPosition - lastSlatsPosition;

        double res = 0;
        if (dif > 0) // closing
        {
            if (_slatAdjustTime == 0) return -1;

            res = _slatAdjustTime * dif / 100.;
        }
        else // opening
        {
            if (_slatAdjustTime == 0) return -1;

            res = -_slatAdjustTime * dif / 100.;
        }

        return res;
    }

    std::atomic<std::chrono::system_clock::time_point> lastSlatsEventTime;

    InputEvent slatsInputEvent;

    std::thread m_slatsThread;

    std::atomic<SlatsWaitingState> waitingSlatsState{SlatsWaitingState::SlatsNotWaiting};

    int oldSlatsPosition = -1; // for tick thread

    std::atomic_int lastSlatsPosition{100}; // 0 - totally opened, 100 totally closed, -1 unknown ?
    std::atomic_int targetSlatsPosition{100}; // set target position at the beginning of the command, maybe it's stopped before ending it

    std::atomic<SlatsDirection> prevSlatsDirection{SlatsDirection::UnknownSlatsDir};
};

}

#endif // SHADING_CONTROLLER_H_

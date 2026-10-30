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


#include "ShadingController.h"
#include "MyNode.h"


namespace MyNode
{


ShadingController::ShadingController(MyNode *node)
    : m_node(node), lastEventTime(std::chrono::system_clock::now())
{
    stateMachine.SetTransitionTable(
    {
        //NoEvent - remain in the old state
        {
            State::Idle, State::MovingUp, State::MovingDown, State::MovingUnknownDirection
        },
        //PressUp - transition to 'move up' state
        {
            State::MovingUp, State::MovingUp, State::MovingUp, State::MovingUp
        },
        //PressDown - transition to 'move down' state
        {
            State::MovingDown, State::MovingDown, State::MovingDown, State::MovingDown
        },
        //ReleaseUp - end the movement - if the movement is ended by a timeout, the state machine should not receive this as sent by the node, but sent instead when the timeout occurs
        {
            State::Idle, State::Idle, State::Idle, State::Idle
        },
        //ReleaseDown - end the movement - if the movement is ended by a timeout, the state machine should not receive this as sent by the node, but sent instead when the timeout occurs
        {
            State::Idle, State::Idle, State::Idle, State::Idle
        },
        //PressStop - end the movement - if the movement is ended by a timeout, the state machine should not receive this as sent by the node, but sent instead when the timeout occurs
        {
            State::Idle, State::Idle, State::Idle, State::Idle
        },
        //ReleaseStop - end the movement - if the movement is ended by a timeout, the state machine should not receive this as sent by the node, but sent instead when the timeout occurs
        {
            State::Idle, State::Idle, State::Idle, State::Idle
        },
        //PositionChange,
        {
            State::MovingUnknownDirection, State::MovingUnknownDirection, State::MovingUnknownDirection, State::MovingUnknownDirection
        }
    });

    stateMachine.SetFalseOutputs(
    {
        // Idle
        {
            0, 1 // up, down
        },
        // MovingUp
        {
            1, 2 // down, stop
        },
        // MovingDown
        {
            0, 2 // up, stop
        },
        // MovingUnknownDirection
        {
            2 // have no clue which one should be true/false from up/down
        }
    });

    stateMachine.SetTrueOutputs(
    {
        // Idle
        {
            2 // stop
        },
        // MovingUp
        {
            0
        },
        // MovingDown
        {
            1
        },
        // MovingUnknownDirection
        {
            // have no clue which one should be true/false from up/down
        }
    });
}

ShadingController::~ShadingController()
{
}


void ShadingController::StartThread()
{
    WaitForStopThread();

    inputEvent.ResetEvent();

    m_thread = std::thread(&ShadingController::ThreadLoop, this);

    m_stopTickThread = false;
    m_tickThread = std::thread(&ShadingController::TickThreadLoop, this);
}

void ShadingController::SignalToStopThread()
{
    SignalEvent(Event::StopThread);
}

void ShadingController::WaitForStopThread()
{
    if (m_thread.joinable())
    {
        SignalToStopThread();
        m_thread.join();
    }

    m_stopTickThread = true;
    if (m_tickThread.joinable())
    {
        m_tickThread.join();
    }
}


void ShadingController::SetTargetPositionTotallyUp()
{
    if (lastPosition == -1) lastPosition = 100;

    targetPosition = 0;

    m_node->log(1, "Setting target position to: " + std::to_string((int)targetPosition.load()) + " from moving up");
}

void ShadingController::SetTargetPositionTotallyDown()
{
    if (lastPosition == -1) lastPosition = 0;

    targetPosition = 100;

    m_node->log(1, "Setting target position to: " + std::to_string((int)targetPosition.load()) + " from moving down");
}

void ShadingController::Tick()
{
    if (waitingState == WaitingState::WaitingForMoving)
    {
        std::chrono::system_clock::time_point tickTime = std::chrono::system_clock::now();
        std::chrono::milliseconds timeDif = std::chrono::duration_cast<std::chrono::milliseconds>(tickTime - lastEventTime.load());

        const Direction lastDir = lastDirection();
        const int newPosition = ComputeCurrentPosition(timeDif, lastDir);
        if (oldPosition != newPosition && newPosition != -1)
        {
            /*
            if (lastDir == Direction::Down)
                m_node->log(1, "From tick thread, moving down...");
            else if (lastDir == Direction::Up)
                m_node->log(1, "From tick thread, moving up...");
            else
                m_node->log(1, "From tick thread, moving in unknown direction...");
            */

            OutputPosition(newPosition);
            oldPosition = newPosition;
        }
    }
}

void ShadingController::TickThreadLoop()
{
    const int TICK_MS = 25;

    while (!m_stopTickThread)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(TICK_MS));

        Tick();
    }
}


void ShadingController::SetStopOutputFalse()
{
    if (outputs[2])
    {
        auto msgFalse = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
        msgFalse->structValue->emplace("payload", std::make_shared<Flows::Variable>(false));

        m_node->output(2, msgFalse);
        outputs[2] = false;
    }
}


void ShadingController::ThreadLoop()
{
    if (!m_node) return;

    auto msgTrue = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
    msgTrue->structValue->emplace("payload", std::make_shared<Flows::Variable>(true));

    auto msgFalse = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
    msgFalse->structValue->emplace("payload", std::make_shared<Flows::Variable>(false));


    lastEventTime = std::chrono::system_clock::now();

    Event eventBeforeDeadtime = Event::NoEvent;
    Event lastEvent = Event::NoEvent;

    Event e = Event::NoEvent;
    std::chrono::system_clock::time_point eventTime = lastEventTime;

    std::chrono::milliseconds timeDif = std::chrono::milliseconds(0);
    std::chrono::milliseconds waitTime = std::chrono::milliseconds(0);

    bool firstTime = true;

    for(;;)
    {
        if (WaitingState::NotWaiting != waitingState)
        {
            waitTime = HowMuchTimeToWait(e, timeDif);

            m_node->log(1, "Will wait for " + std::to_string((int)waitTime.count()) + " ms");

            e = inputEvent.WaitMilliseconds(waitTime.count());
        }
        else e = inputEvent.Wait();

        if (e == Event::StopThread)
        {
            Forward(Event::StopThread);
            break;
        }

        if (e != Event::Timeout)
        {
            if (e == Event::PositionSlatsChange || e == Event::PressSlatsDown || e == Event::PressSlatsUp || e == Event::ReleaseSlatsUp || e == Event::ReleaseSlatsDown)
            {
                // this event is specific to slats position only, so forward it and ignore it
                // it's handled in the blind controller
                Forward(e);
                continue;
            }
            else if (e == Event::PressStop) Forward(Event::PressStop); // forward it to slats, too

            // some events can be canceled by previous events
            // so we ignore those
            if ((e == Event::ReleaseUp && lastEvent != Event::PressUp) || (e == Event::ReleaseDown && lastEvent != Event::PressDown) ||
                (e == Event::ReleaseStop && lastEvent != Event::PressStop))
                continue;

            // those are ignored if a time is set, a waiting is used instead to generate them
            if (((e == Event::ReleaseDown && _totalTimeMovingDown > 0) || (e == Event::ReleaseUp && _totalTimeMovingUp > 0)))
                continue;
        }

        if (firstTime)
        {
            lastEventTime = std::chrono::system_clock::now();
            firstTime = false;
        }

        eventTime = std::chrono::system_clock::now();
        timeDif = std::chrono::duration_cast<std::chrono::milliseconds>(eventTime - lastEventTime.load());

        const WaitingState oldWaitingState = waitingState;
        const State oldState = stateMachine.GetCurrentState();
        const Direction lastDir = lastDirection();


        // a state machine that also deals with waiting time intervals gets too complex,
        // so handle waiting separately


        bool wasATimeout = false;
        bool wasADeadtimeTimeout = false;

        if (e == Event::Timeout)
        {
            wasATimeout = true;
            // either was waiting for a deadtime, or for moving, or for determining if it's a short or long press
            switch(waitingState)
            {
            case WaitingState::WaitingForDeadtime:
                m_node->log(1, "Timeout for waiting for deadtime...");
                timeDif = waitTime;
                e = eventBeforeDeadtime;
                wasADeadtimeTimeout = true;

                break;
            case WaitingState::WaitingForMoving:
            {
                waitingState = WaitingState::NotWaiting;

                m_node->log(1, "Timeout for waiting for moving...");

                prevDirection = lastDir;
                lastPosition = targetPosition.load();
                m_node->log(1, "Setting last position to: " + std::to_string((int)lastPosition.load()));
                timeDif = waitTime;
                if (lastPosition != -1) OutputPosition(lastPosition);
            }
            break;
            case WaitingState::NotWaiting:
            default:
                m_node->log(1, "Timeout while not in a waiting state, should not happen...");
                break;
            }

            if (!wasADeadtimeTimeout) // otherwise eventBeforeDeadtime was already used to set it back
            {
                e = lastEvent;
                m_node->log(1, "Timeout, last event was: " + std::to_string((int)e));
            }
        }
        else if (waitingState != WaitingState::NotWaiting)
        {
            m_node->log(1, "Event arrived while waiting: " + std::to_string((int)e));

            if (waitingState == WaitingState::WaitingForMoving)
            {
                waitingState = WaitingState::NotWaiting;

                prevDirection = lastDir;

                lastPosition = ComputeCurrentPosition(timeDif, lastDir);
                m_node->log(1, "Setting last position to: " + std::to_string((int)lastPosition.load()));

                timeDif = std::chrono::milliseconds(0);
            }
        }
        else if (oldState != State::Idle) // not waiting, but it was left moving
        {
            prevDirection = lastDir;
            lastPosition = ComputeCurrentPosition(timeDif, lastDir);
            m_node->log(1, "Setting last position to: " + std::to_string((int)lastPosition.load()));
            timeDif = std::chrono::milliseconds(0);
        }

        WaitingState waitingStateNew;

        if (wasATimeout)
            waitingStateNew = ShadingController::NeedsToWaitForATimeout(e, timeDif, lastDir, wasADeadtimeTimeout);
        else
            waitingStateNew = NeedsToWaitForATimeout(e, timeDif, lastDir, wasADeadtimeTimeout);

        // events that are handled the same for roller shutter and blinds


        // also set target position for events


        {
            m_node->log(1, "Handling event: " + std::to_string((int)e));

            if (e == Event::PressDown)
            {
                SetTargetPositionTotallyDown();
            }
            else if (e == Event::PressUp)
            {
                SetTargetPositionTotallyUp();
            }
            else if (e == Event::PositionChange)
            {
                targetPosition = m_node->_position.load();
                m_node->log(1, "Setting target position to: " + std::to_string((int)targetPosition.load()) + " from position change");
            }
            // the security events can be converted to up / down / position events, to simplify more the state maching
            else if (e == Event::SecurityUp)
            {
                inSecurity = true;

                Forward(Event::PressStop); // fake a 'stop' for slats

                SetTargetPositionTotallyUp();

                lastPositionBeforeSecurity = lastPosition.load();
                // for security, there are several cases:

                if (lastPosition == 0 && oldState == State::Idle)
                {
                    // the position is already correct for security or state is during moving up

                    e = Event::NoEvent;
                }
                else
                {
                    e = Event::PressUp;
                }
            }
            else if (e == Event::SecurityDown)
            {
                inSecurity = false;
                // this is either 'no action' or a 'move down', which might need waiting for dead time completion
                if (_actionAfterSecurity != 2) // 2 is 'do nothing'
                {
                    // in any move case, there is a reversion of the move, so check if 'dead time' passed since it was driven up

                    if (_actionAfterSecurity == 1)
                    {
                        // move down
                        SetTargetPositionTotallyDown();

                        e = Event::PressDown;
                    }
                    else
                    {
                        // last position
                        targetPosition = lastPositionBeforeSecurity.load();
                        m_node->log(1, "Setting target position to: " + std::to_string((int)targetPosition.load()) + " from security action");

                        e = Event::PositionChange;
                    }
                }
                else e = Event::NoEvent;
            }

            if (wasATimeout)
            {
                waitingStateNew = ShadingController::NeedsToWaitForATimeout(e, timeDif, lastDir, wasADeadtimeTimeout);

                if (waitingStateNew != WaitingState::WaitingForDeadtime)
                {
                    // timeout for 'long press' but not waiting for dead time
                    timeDif = std::chrono::milliseconds(0);
                    eventTime = std::chrono::system_clock::now();
                }
            }
            else
                waitingStateNew = NeedsToWaitForATimeout(e, timeDif, lastDir);


            if (waitingStateNew == WaitingState::NotWaiting)
            {
                m_node->log(1, "No need for waiting, will not wait...");

                if (wasATimeout)
                {
                    if (oldWaitingState == WaitingState::WaitingForMoving)
                    {
                        // just finished waiting for moving

                        m_node->log(1, "Ending timed command...");

                        if (e == Event::PositionChange && lastPosition != targetPosition)
                        {
                            // finished waiting from position change
                            SetStopOutputFalse();
                            OutputPosition(targetPosition);

                            lastPosition = -1;
                        }

                        if (e == Event::PressDown)
                            lastEvent = e = Event::ReleaseDown;
                        else if (e == Event::PressUp)
                            lastEvent = e = Event::ReleaseUp;
                        else if (e == Event::PositionChange)
                            lastEvent = e = Event::PressStop;

                        lastEventTime = eventTime;
                    }
                    else if (oldWaitingState == WaitingState::WaitingForDeadtime)
                    {
                        // just finished waiting for dead time - it should be stopped by now but it doesn't hurt to check again
                        if (e == Event::PositionChange && lastPosition != targetPosition)
                        {
                            SetStopOutputFalse();
                            OutputPosition(targetPosition);

                            lastPosition = -1;

                            e = Event::NoEvent;
                            lastEventTime = eventTime;
                        }
                    }
                }
                else
                {
                    // it enters here only if it was waiting for a long press - interrupted by a position change command

                    if (e == Event::PositionChange && lastPosition != targetPosition)
                    {
                        SetStopOutputFalse();
                        OutputPosition(targetPosition);

                        lastPosition = -1;

                        e = Event::NoEvent;
                        lastEventTime = eventTime;
                    }
                }
            }
            else m_node->log(1, "Waiting state: " + std::to_string((int)waitingStateNew));


            if ((e == Event::PressUp && IsTotallyUp()) || (e == Event::PressDown && IsTotallyDown()))
                continue;

            if (waitingStateNew == WaitingState::WaitingForDeadtime && oldWaitingState != WaitingState::WaitingForDeadtime && e != Event::NoEvent)
            {
                eventBeforeDeadtime = e;

                m_node->log(1, "Event before dead time: " + std::to_string((int)e));
            }
            else
                eventBeforeDeadtime = Event::NoEvent;

            // this is done only for the state machine, it doesn't know about time and position
            const Event saveEvent = e;
            const Direction dir = lastDirection();
            if (e == Event::PositionChange)
            {
                if (dir != Direction::Unknown)
                {
                    if (lastPosition == targetPosition)
                    {
                        e = Event::NoEvent;
                        // maybe in this case just output position?
                    }
                    else if (dir == Direction::Down)
                        e = Event::PressDown;
                    else
                        e = Event::PressUp;
                }
            }

            const State newState = stateMachine.Event(e);

            e = saveEvent; // restore the event

            m_node->log(1, "Old state: " + std::to_string((int)oldState) + ", new state: " + std::to_string((int)newState));

            {
                // only output them if they change

                const std::vector<unsigned int>& trueOutputs = stateMachine.GetTrueOutputs();
                const std::vector<unsigned int>& falseOutputs = stateMachine.GetFalseOutputs();

                for (const unsigned int o : falseOutputs)
                {
                    // will wait, don't set stop on false even if set in the state machine new state
                    // it will be set next time after (if) dead time passes
                    if (2 == o && waitingStateNew == WaitingState::WaitingForDeadtime)
                        continue;

                    if (outputs[o])
                    {
                        // avoid setting it to false it will be set back to true immediately - should not have that case
                        //if (waitingStateNew != WaitingState::WaitingForDeadtime && trueOutputs[o]) continue;

                        m_node->output(o, msgFalse);
                        outputs[o] = false;
                    }
                }


                if (waitingStateNew != WaitingState::WaitingForDeadtime)
                {
                    for (const unsigned int o : trueOutputs)
                    {
                        if (!outputs[o])
                        {
                            m_node->output(o, msgTrue);
                            outputs[o] = true;
                        }
                    }
                }
                else if (!outputs[2])
                {
                    // any output (except stop) that's on true is set to false:
                    for (unsigned int o = 0; o < 2; ++o)
                    {
                        if (outputs[o])
                        {
                            m_node->output(o, msgFalse);
                            outputs[o] = false;
                        }
                    }

                    // send stop if it's on false
                    m_node->output(2, msgTrue);
                    outputs[2] = true;
                }

            }


            if (e != Event::NoEvent)
            {
                if (!wasATimeout)
                {
                    m_node->log(1, "Setting last event to: " + std::to_string((int)e));

                    lastEvent = e;
                }
                lastEventTime = eventTime;
            }
            waitingState = waitingStateNew;
        }

    }

}


bool ShadingController::EventCommandingToMove(const Event& e) const
{
    if (e == Event::PressUp || e == Event::PressDown || e == Event::PositionChange || e == Event::SecurityUp || e == Event::PositionSlatsChange)
        return true;
    else if (e == Event::SecurityDown)
    {
        if (_actionAfterSecurity == 0) // last position
        {
            return lastPosition > 0;
        }
        else if (_actionAfterSecurity == 1) // move down
        {
            return true;
        }
    }

    return false;
}


void ShadingController::OutputPosition(int pos)
{
    if (!m_node) return;

    auto msgPosition = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
    msgPosition->structValue->emplace("payload", std::make_shared<Flows::Variable>(pos));

    m_node->output(3, msgPosition);
}



void ShadingController::SignalEvent(const Event& e)
{
    inputEvent.SetEvent(e);
    inputEvent.NotifyOne();
}

// timeDif contains the difference between the 'current' event (last one, can be a timeout) and the previous one - the recorded ones are always 'action' ones (that is, no timeout)
// if it was a timeout, it's replaced with the previous action event - that one probably waited for some time interval (either deadtime or moving time)
ShadingController::WaitingState ShadingController::NeedsToWaitForATimeout(const Event& e, const std::chrono::milliseconds& timeDif, Direction lastDir, bool wasDeadtime)
{
    // if the direction is reversed and from last command time less than 'dead time' has passed
    if (EventCommandingToMove(e))
    {
        if (!wasDeadtime && _deadTime > 0 && timeDif.count() < _deadTime * 1000)
        {
            m_node->log(1, "Command time less than dead time...");

            if (lastDir == Direction::Unknown)
                return ShadingController::WaitingState::WaitingForDeadtime;

            const Direction dir = lastDirection();
            if (dir != lastDir)
                return ShadingController::WaitingState::WaitingForDeadtime;
        }

        // no need for deadtime waiting

        // if a command was given, wait for its completion (or some other incoming event)

        const double mt = TimeToMove() * 1000.;

        m_node->log(1, "Checking how much time is needed for moving, computed: " + std::to_string(mt));

        if (mt > 0)
        {
            m_node->log(1, "Needs to wait...");
            return ShadingController::WaitingState::WaitingForMoving;
        }
        m_node->log(1, "No need to wait...");
    }

    return ShadingController::WaitingState::NotWaiting;
}

std::chrono::milliseconds ShadingController::HowMuchTimeToWait(const Event& e, const std::chrono::milliseconds& timeDif)
{
    if (waitingState == WaitingState::WaitingForDeadtime)
    {
        const double dif = _deadTime * 1000 - timeDif.count();

        if (dif > 0)
            return std::chrono::milliseconds(static_cast<int>(dif));


        return std::chrono::milliseconds(0);
    }

    const double neededTime = TimeToMove() * 1000.;
    if (neededTime < 0) return std::chrono::milliseconds(0);

    return std::chrono::milliseconds(static_cast<int>(neededTime));
}


RollerShutterController::RollerShutterController(MyNode *node) : ShadingController(node)
{

}


bool RollerShutterController::NeedsToWaitForAnotherEvent(const Event& e)
{
    return false; // not really for shutter
}



BlindController::BlindController(MyNode *node)
    : ShadingController(node), lastSlatsEventTime(std::chrono::system_clock::now())
{
    hasSlats = true;
}

void BlindController::StartThread()
{
    ShadingController::StartThread();

    //WaitForStopThread(); // is virtual, already called in the base class call above

    slatsInputEvent.ResetEvent();

    m_slatsThread = std::thread(&BlindController::SlatsThreadLoop, this);

}

void BlindController::SignalToStopThread()
{
    ShadingController::SignalToStopThread();

    // nothing really to be done here for the slats thread, the event for the base class is forwarded to this one as well
}

void BlindController::WaitForStopThread()
{
    ShadingController::WaitForStopThread();

    if (m_slatsThread.joinable())
    {
        //SignalToStopThread(); // signaling was done in the base class, the event should be forwarded to this one as well
        m_slatsThread.join();
    }
}

bool BlindController::NeedsToWaitForAnotherEvent(const Event& e)
{
    if (e == Event::PressDown || e == Event::PressUp) // needs to wait for release event (or at least the length of the click from long time click), to compute the length of the click
        return true;

    return false;
}

ShadingController::WaitingState BlindController::NeedsToWaitForATimeout(const Event& e, const std::chrono::milliseconds& timeDif, Direction lastDir, bool wasDeadtime)
{
    return ShadingController::NeedsToWaitForATimeout(e, timeDif, lastDir, wasDeadtime);
}

std::chrono::milliseconds BlindController::HowMuchTimeToWait(const Event& e, const std::chrono::milliseconds& timeDif)
{

    return ShadingController::HowMuchTimeToWait(e, timeDif);
}

void BlindController::Forward(const Event& e)
{
    // TODO: Filter them out, pass through only events that concern the functionality of the blind controller (slats, terminate thread and so on)
    slatsInputEvent.SetEvent(e);
    slatsInputEvent.NotifyOne();
}

void BlindController::Tick()
{
    ShadingController::Tick();

    // do the updating for the slats position
    if (waitingSlatsState == SlatsWaitingState::SlatsWaitingForMoving)
    {
        std::chrono::system_clock::time_point tickTime = std::chrono::system_clock::now();
        std::chrono::milliseconds timeDif = std::chrono::duration_cast<std::chrono::milliseconds>(tickTime - lastSlatsEventTime.load());

        const SlatsDirection lastDir = lastSlatsDirection();

        const int newPosition = ComputeCurrentSlatsPosition(timeDif, lastDir);
        if (oldSlatsPosition != newPosition && newPosition != -1)
        {
            OutputSlatsPosition(newPosition);
            oldSlatsPosition = newPosition;
        }
    }
}


void BlindController::SlatsThreadLoop()
{
    Event e = Event::NoEvent;

    lastSlatsEventTime = std::chrono::system_clock::now();

    std::chrono::system_clock::time_point eventTime = lastSlatsEventTime;

    std::chrono::milliseconds timeDif = std::chrono::milliseconds(0);
    std::chrono::milliseconds waitTime = std::chrono::milliseconds(0);

    bool firstTime = true;

    for (;;)
    {
        if (waitingSlatsState == SlatsWaitingState::SlatsNotWaiting)
            e = slatsInputEvent.Wait();
        else
        {
            double ms = 0;
            if (e == Event::PressSlatsDown || e == Event::PressSlatsUp)
                // wait for a step
                ms = SlatsStepLength();
            else
                ms = TimeToMoveSlats();

            if (ms == 0)
                e = slatsInputEvent.Wait();
            else
            {
                ms *= 1000.;

                e = slatsInputEvent.WaitMilliseconds(ms);
            }
        }

        if (e == Event::StopThread)
            break;

        // basically only four kind of events can come here
        // press up or down from a short press (filtered out at the base class level)
        // set slats position
        // and timeout for waiting for slats moving

        if (firstTime)
        {
            lastSlatsEventTime = std::chrono::system_clock::now();
            firstTime = false;
        }

        eventTime = std::chrono::system_clock::now();
        timeDif = std::chrono::duration_cast<std::chrono::milliseconds>(eventTime - lastSlatsEventTime.load());

        const SlatsDirection lastDir = lastSlatsDirection();


        m_node->log(1, "Slats event received...");

        if (_slatAdjustTime == 0)
        {
            if (e == Event::PositionSlatsChange)
            {
                prevSlatsDirection = lastDir;
                lastSlatsPosition = targetSlatsPosition.load(); // this is not ok, but there is no info for how much time it takes to get there, so...
                targetSlatsPosition = m_node->_slatsPosition.load();

                // not much can be done about it, except copy the value to output
                OutputSlatsPosition(m_node->_slatsPosition);

                lastSlatsEventTime = eventTime;
                waitingSlatsState = SlatsWaitingState::SlatsNotWaiting;
            }
            else if (e == Event::PressSlatsDown || e == Event::PressSlatsUp)
            {
                prevSlatsDirection = lastDir;
                lastSlatsPosition = targetSlatsPosition.load(); // this is not ok, but there is no info for how much time it takes to get there, so...

                const int stepsNumber = _stepsNumber == 0 ? 1 : _stepsNumber;

                const double percStep = 100. / stepsNumber;

                targetSlatsPosition += (e == Event::PressSlatsDown ? 1 : -1) * percStep;

                if (targetSlatsPosition > 100) targetSlatsPosition = 100;
                else if (targetSlatsPosition < 0) targetSlatsPosition = 0;

                lastSlatsEventTime = eventTime;
                waitingSlatsState = SlatsWaitingState::SlatsNotWaiting;
            }
            else if (e == Event::Timeout)
            {
                // shouldn't happen
                m_node->log(1, "Slats timeout with no adjustment time set");
            }
        }
        else
        {
            // TODO: should I count up/down presses to see when it reaches max to be either totally closed or opened if the starting position is unknown?

            // another trick that could be used for both slats and roller shutter is to first move it to a max/min position if the position is unknown then move it from there, as the final position is known

            if (waitingSlatsState == SlatsWaitingState::SlatsNotWaiting)
            {
                if (e == Event::PositionSlatsChange)
                {
                    targetSlatsPosition = m_node->_slatsPosition.load();

                    // put it in waiting state
                    lastSlatsEventTime = eventTime;
                    waitingSlatsState = SlatsWaitingState::SlatsWaitingForMoving;
                }
                else if (e == Event::PressSlatsDown || e == Event::PressSlatsUp)
                {
                    const int stepsNumber = _stepsNumber == 0 ? 1 : _stepsNumber;

                    const double percStep = 100. / stepsNumber;

                    targetSlatsPosition += (e == Event::PressSlatsDown ? 1 : -1) * percStep;

                    if (targetSlatsPosition > 100) targetSlatsPosition = 100;
                    else if (targetSlatsPosition < 0) targetSlatsPosition = 0;

                    lastSlatsEventTime = eventTime;
                    waitingSlatsState = SlatsWaitingState::SlatsWaitingForMoving;
                }
                else if (e == Event::Timeout)
                {
                    // shouldn't happen
                    m_node->log(1, "Slats timeout while not waiting");
                }
            }
            else
            {
                // interrupted while waiting for moving

                if (e == Event::PositionSlatsChange)
                {
                    waitingSlatsState = SlatsWaitingState::SlatsNotWaiting;

                    // not finished moving, compute the slats position since waiting started
                    const double fracMoved = timeDif.count() / (1000. * _slatAdjustTime);

                    if (lastSlatsPosition != -1)
                    {
                        if (lastDir == SlatsDirection::Closing)
                            lastSlatsPosition += fracMoved * 100.;
                        else if (lastDir == SlatsDirection::Opening)
                            lastSlatsPosition -= fracMoved * 100.;

                        if (lastSlatsPosition > 100) lastSlatsPosition = 100;
                        else if (lastSlatsPosition < 0) lastSlatsPosition = 0;
                    }

                    prevSlatsDirection = lastDir;
                    targetSlatsPosition = m_node->_slatsPosition.load();

                    // put it in waiting state
                    lastSlatsEventTime = eventTime;
                    waitingSlatsState = SlatsWaitingState::SlatsWaitingForMoving;
                }
                else if (e == Event::PressSlatsDown || e == Event::PressSlatsUp)
                {
                    waitingSlatsState = SlatsWaitingState::SlatsNotWaiting;

                    // not finished moving, compute the slats position since waiting started
                    const double fracMoved = timeDif.count() / (1000. * _slatAdjustTime);

                    if (lastSlatsPosition != -1)
                    {
                        if (lastDir == SlatsDirection::Closing)
                            lastSlatsPosition += fracMoved * 100.;
                        else if (lastDir == SlatsDirection::Opening)
                            lastSlatsPosition -= fracMoved * 100.;

                        if (lastSlatsPosition > 100) lastSlatsPosition = 100;
                        else if (lastSlatsPosition < 0) lastSlatsPosition = 0;
                    }

                    const int stepsNumber = _stepsNumber == 0 ? 1 : _stepsNumber;

                    const double percStep = 100. / stepsNumber;

                    targetSlatsPosition += (e == Event::PressSlatsDown ? 1 : -1) * percStep;

                    if (targetSlatsPosition > 100) targetSlatsPosition = 100;
                    else if (targetSlatsPosition < 0) targetSlatsPosition = 0;

                    prevSlatsDirection = lastDir;
                    lastSlatsEventTime = eventTime;
                    waitingSlatsState = SlatsWaitingState::SlatsWaitingForMoving;
                }
                else if (e == Event::Timeout)
                {
                    // movement finished
                    waitingSlatsState = SlatsWaitingState::SlatsNotWaiting;

                    prevSlatsDirection = lastDir;
                    lastSlatsPosition = targetSlatsPosition.load();
                    OutputSlatsPosition(lastSlatsPosition);
                    lastSlatsEventTime = eventTime;
                }
            }
        }
    }
}


void BlindController::OutputSlatsPosition(int pos)
{
    if (!m_node) return;

    auto msgPosition = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
    msgPosition->structValue->emplace("payload", std::make_shared<Flows::Variable>(pos));

    m_node->output(6, msgPosition);
}



}

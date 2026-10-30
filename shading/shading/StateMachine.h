#ifndef _STATE_MACHINE_H_
#define _STATE_MACHINE_H_


#include <vector>
#include <atomic>

namespace MyNode
{

// looks like the situation is complex enough to benefit from a separate state machine
// states and events/inputs are here ints, but probably they will be enums, convertible to/from int


// the same functionality can be implemented in many ways
// basically we could either have many states and less events
// or many events and less states

// some events might need to be differentiated
// for example a timeout can occur due of waiting for a movement
// or waiting to pass 'dead time', or if it's a blind controller, waiting for the 'long time click'
// also 'security off' event could be three types: off and do nothing, off and move to last position, off and move down

template<typename StateType, typename EventType> class StateMachine
{
public:
    StateMachine() {};
    StateMachine(const std::vector<std::vector<StateType>>& tt)
    : transitionTable(tt)
    {
    }

    void SetTransitionTable(const std::vector<std::vector<StateType>>& tt)
    {
        transitionTable = tt;
    }

    void SetTrueOutputs(const std::vector<std::vector<unsigned int>>& o)
    {
        stateOutputsTrue = o;
    }

    void SetFalseOutputs(const std::vector<std::vector<unsigned int>>& o)
    {
        stateOutputsFalse = o;
    }

    void SetCurrentState(const StateType& state)
    {
        curState = state;
    }

    StateType GetCurrentState() const
    {
        return curState;
    }

    StateType FutureState(const EventType& e) const
    {
        const unsigned int eventIndex = static_cast<unsigned int>(e);
        const unsigned int stateIndex = static_cast<unsigned int>(GetCurrentState());

        if (eventIndex >= transitionTable.size() || stateIndex >= transitionTable[eventIndex].size()) return curState;

        return transitionTable[eventIndex][stateIndex];
    }

    StateType Event(const EventType& e)
    {
        curState = FutureState(e);

        return curState;
    }

    const std::vector<unsigned int>& GetTrueOutputs() const
    {
        const unsigned int state = static_cast<unsigned int>(GetCurrentState());

        if (state >= stateOutputsTrue.size()) return noOutputs;

        return stateOutputsTrue[state];
    }

    const std::vector<unsigned int>& GetFalseOutputs() const
    {
        const unsigned int state = static_cast<unsigned int>(GetCurrentState());

        if (state >= stateOutputsFalse.size()) return noOutputs;

        return stateOutputsFalse[state];
    }

protected:
    std::vector<std::vector<StateType>> transitionTable;

    std::vector<std::vector<unsigned int>> stateOutputsTrue;
    std::vector<std::vector<unsigned int>> stateOutputsFalse;

    std::vector<unsigned int> noOutputs; // an empty array

    std::atomic<StateType> curState{static_cast<StateType>(0)};
};

}

#endif // _STATE_MACHINE_H_

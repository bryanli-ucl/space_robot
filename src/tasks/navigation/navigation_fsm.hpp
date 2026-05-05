#pragma once

#include "tinyfsm.hpp"

#include "navigation_types.hpp"

namespace navigation {

// Forward declare states
struct Idle;
struct LineFollow;
struct WallFollow;

// ------------------------------------------------------------------
// FSM Base
// ------------------------------------------------------------------

struct NavigationFSM : tinyfsm::Fsm<NavigationFSM> {

    static NavContext ctx;

    virtual void entry() {}
    virtual void exit() {}

    virtual void react(IntoLineFollow const&) {}
    virtual void react(IntoWallFollow const&) {}
    virtual void react(IntoIdle const&) {}
    virtual void react(RfidDone const&) {}

    virtual void update() {}

    static void update_current() {
        current_state_ptr->update();
    }
};

// ------------------------------------------------------------------
// States
// ------------------------------------------------------------------

struct Idle : NavigationFSM {
    void react(IntoLineFollow const& e);
    void react(IntoWallFollow const& e);
    void react(RfidDone const& e);
};

struct LineFollow : NavigationFSM {
    void entry();
    void react(IntoLineFollow const& e);
    void react(IntoWallFollow const& e);
    void react(IntoIdle const&);
    void react(RfidDone const& e);
    void update();
};

struct WallFollow : NavigationFSM {
    void entry();
    void react(IntoWallFollow const& e);
    void react(IntoLineFollow const& e);
    void react(IntoIdle const&);
    void update();
};

} // namespace navigation

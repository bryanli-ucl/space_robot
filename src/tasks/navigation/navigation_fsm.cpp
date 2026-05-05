#include "navigation_fsm.hpp"

#include <cstdio>

namespace navigation {

// ------------------------------------------------------------------
// Context definition
// ------------------------------------------------------------------

NavContext NavigationFSM::ctx{};

// ------------------------------------------------------------------
// Idle
// ------------------------------------------------------------------

void Idle::react(IntoLineFollow const& e) {
    transit<LineFollow>([=] {
        ctx.line_cfg = e.cfg;
    });
}

void Idle::react(IntoWallFollow const& e) {
    transit<WallFollow>([=] {
        ctx.wall_cfg = e.cfg;
    });
}

void Idle::react(RfidDone const& e) {
    printf("[Nav/Idle] Unexpected RfidDone tag=%lu fertile=%d\n", static_cast<unsigned long>(e.tag_id), e.fertile);
}

// ------------------------------------------------------------------
// LineFollow
// ------------------------------------------------------------------

void LineFollow::entry() {
    // 初始化，比如 PID reset
}

void LineFollow::react(IntoLineFollow const& e) {
    // 同状态更新参数
    ctx.line_cfg = e.cfg;
}

void LineFollow::react(IntoWallFollow const& e) {
    transit<WallFollow>([=] {
        ctx.wall_cfg = e.cfg;
    });
}

void LineFollow::react(IntoIdle const&) {
    transit<Idle>();
}

void LineFollow::react(RfidDone const& e) {
    printf("[Nav/LineFollow] RfidDone tag=%lu fertile=%d\n", static_cast<unsigned long>(e.tag_id), e.fertile);
    // TODO: transition to Plant or continue based on e.fertile
    transit<Idle>();
}

void LineFollow::update() {
    // TODO: 实现你的 line follow 控制
    // line_follow_update(ctx.line_cfg);
}

// ------------------------------------------------------------------
// WallFollow
// ------------------------------------------------------------------

void WallFollow::entry() {
    // 初始化
}

void WallFollow::react(IntoWallFollow const& e) {
    ctx.wall_cfg = e.cfg;
}

void WallFollow::react(IntoLineFollow const& e) {
    transit<LineFollow>([=] {
        ctx.line_cfg = e.cfg;
    });
}

void WallFollow::react(IntoIdle const&) {
    transit<Idle>();
}

void WallFollow::update() {
    // TODO: 实现你的 wall follow 控制
    // wall_follow_update(ctx.wall_cfg);
}

} // namespace navigation

#!/usr/bin/env python3
"""
PlantUML State Machine → TinyFSM C++ Generator

Parses docs/robot_fsm.puml and generates:
  - src/tasks/robot_fsm_gen.hpp   (skeleton declarations)
  - src/tasks/robot_fsm_impl.cpp  (stub implementations)

Usage:
    python tools/generate_fsm.py
"""

import re
import os
from pathlib import Path

PUML_PATH = Path("docs/robot_fsm.puml")
OUT_HPP = Path("src/tasks/robot_fsm_gen.hpp")
OUT_CPP = Path("src/tasks/robot_fsm_impl.cpp")


class StateNode:
    def __init__(self, name: str, parent: "StateNode | None" = None):
        self.name = name
        self.parent = parent
        self.children: list[StateNode] = []
        self.is_composite = False
        self.initial_child: str | None = None

    def full_name(self) -> str:
        if self.parent and self.parent.name:
            return f"{self.parent.full_name()}_{self.name}"
        return self.name

    def leaf_states(self) -> list["StateNode"]:
        if not self.children:
            return [self]
        leaves = []
        for c in self.children:
            leaves.extend(c.leaf_states())
        return leaves


def parse_puml(path: Path) -> tuple[StateNode, list[tuple[str, str, str]]]:
    root = StateNode("")
    stack: list[StateNode] = [root]
    transitions: list[tuple[str, str, str]] = []

    with open(path, "r", encoding="utf-8") as f:
        for raw in f:
            line = raw.strip()
            if not line or line.startswith("'") or line.startswith("legend"):
                continue
            if line.startswith("skinparam") or line.startswith("title") or line.startswith("@") or line.startswith("endlegend"):
                continue

            # Composite start: state Name {  or  state Name <<stereo>> {
            m = re.match(r'state\s+(\w+)(?:\s*<<[^>]+>>)?\s*\{', line)
            if m:
                name = m.group(1)
                node = StateNode(name, stack[-1])
                node.is_composite = True
                stack[-1].children.append(node)
                stack.append(node)
                continue

            # End of composite
            if line == "}" and len(stack) > 1:
                stack.pop()
                continue

            # Simple state: state Name or state Name : desc
            m = re.match(r'state\s+(\w+)(?:\s*:\s*(.*))?$', line)
            if m and not line.endswith("{"):
                name = m.group(1)
                node = StateNode(name, stack[-1])
                stack[-1].children.append(node)
                continue

            # Initial state inside composite
            m = re.match(r'\[\*\]\s*-->\s*(\w+)', line)
            if m:
                stack[-1].initial_child = m.group(1)
                continue

            # Transition
            m = re.match(r'(\w+)\s*-->\s*(\w+)\s*:\s*(.*)', line)
            if m:
                src, dst, label = m.group(1), m.group(2), m.group(3).strip()
                transitions.append((src, dst, label))
                continue

    # ------------------------------------------------------------------
    # Pass 2: ensure every state referenced in a transition exists
    # as a simple-state child inside the current composite context.
    # This handles PlantUML implicit state declarations.
    # ------------------------------------------------------------------
    def find_or_create(parent: StateNode, name: str) -> StateNode:
        for c in parent.children:
            if c.name == name:
                return c
        node = StateNode(name, parent)
        parent.children.append(node)
        return node

    # Re-walk the file to place implicit states in the correct composite
    stack2 = [root]
    with open(path, "r", encoding="utf-8") as f:
        for raw in f:
            line = raw.strip()
            if not line or line.startswith("'"):
                continue
            if line.startswith("skinparam") or line.startswith("title") or line.startswith("@") or line.startswith("endlegend"):
                continue

            # Composite start
            m = re.match(r'state\s+(\w+)(?:\s*<<[^>]+>>)?\s*\{', line)
            if m:
                name = m.group(1)
                for c in stack2[-1].children:
                    if c.name == name and c.is_composite:
                        stack2.append(c)
                        break
                continue

            if line == "}" and len(stack2) > 1:
                stack2.pop()
                continue

            # Transition: ensure both src and dst exist in current scope
            m = re.match(r'(\w+)\s*-->\s*(\w+)', line)
            if m:
                src_name, dst_name = m.group(1), m.group(2)
                find_or_create(stack2[-1], src_name)
                find_or_create(stack2[-1], dst_name)
                continue

            # Initial state
            m = re.match(r'\[\*\]\s*-->\s*(\w+)', line)
            if m:
                find_or_create(stack2[-1], m.group(1))
                continue

    return root, transitions


def extract_events(label: str) -> list[str]:
    """Extract event names from a PlantUML transition label."""
    # Remove guard conditions like [was charging]
    label = re.sub(r'\[.*?\]', '', label)
    # Remove stereotypes like <<Multiple>>
    label = re.sub(r'<<.*?>>', '', label)
    # Remove parenthetical notes like (1min)
    label = re.sub(r'\(.*?\)', '', label)
    # Remove entry/ action prefix
    label = re.sub(r'^entry\s*/', '', label)
    # Replace literal \n with space
    label = label.replace('\\n', ' ')
    # Split by '/'  → events / actions
    parts = label.split('/')
    events = []
    for part in parts:
        part = part.strip()
        if not part:
            continue
        # Clean up remaining whitespace
        part = re.sub(r'\s+', '_', part)
        # Skip obvious action descriptions
        if part.lower() in ('manualdeploy', 'send_letmein', 'led_redtogreen', 'auto_power_boost'):
            continue
        events.append(part)
    return events


def flatten_states(root: StateNode) -> dict[str, StateNode]:
    """Map full flattened name → leaf StateNode."""
    result = {}
    for leaf in root.leaf_states():
        result[leaf.full_name()] = leaf
    return result


def build_state_tree() -> tuple[StateNode, dict[str, StateNode]]:
    root, raw_transitions = parse_puml(PUML_PATH)
    flat = flatten_states(root)
    return root, flat, raw_transitions


def resolve_state(name: str, context: StateNode | None, flat: dict[str, StateNode]) -> str | None:
    """Resolve a short state name to its full flattened name."""
    # Direct match in flat map (already full name)
    if name in flat:
        return name
    # Try prefixed by context
    if context:
        candidates = [k for k in flat if k.endswith(f"_{name}")]
        if candidates:
            return candidates[0]
    # Try prefixed by any parent
    for k in flat:
        if k.endswith(f"_{name}"):
            return k
    return None


def find_initial_leaf(composite: StateNode, flat: dict[str, StateNode]) -> str | None:
    """Find the flattened initial leaf state of a composite."""
    if not composite.initial_child:
        return None
    child = next((c for c in composite.children if c.name == composite.initial_child), None)
    if not child:
        return None
    if child.is_composite:
        return find_initial_leaf(child, flat)
    return child.full_name()


def collect_all_events(raw_transitions: list[tuple[str, str, str]]) -> set[str]:
    events = set()
    for src, dst, label in raw_transitions:
        for ev in extract_events(label):
            events.add(ev)
    return events


def generate(root: StateNode, flat: dict[str, StateNode], raw_transitions: list[tuple[str, str, str]]):
    events = sorted(collect_all_events(raw_transitions))

    # Build composite map for fast lookup
    composites: dict[str, StateNode] = {}
    def walk(node: StateNode):
        if node.is_composite:
            composites[node.name] = node
            composites[node.full_name()] = node
        for c in node.children:
            walk(c)
    walk(root)

    # Identify history states (pseudo-states, should not become real State classes)
    history_states: set[str] = set()
    for name in list(flat.keys()):
        if "History" in name:
            history_states.add(name)

    # Resolve transitions into flattened form
    # Key: (src_flat, event) → list of (dst_flat, guard, action)
    trans_map: dict[tuple[str, str], list[tuple[str, str, str]]] = {}

    def add_transition(src_flat: str, event: str, dst_flat: str, guard: str = "", action: str = ""):
        key = (src_flat, event)
        trans_map.setdefault(key, []).append((dst_flat, guard, action))

    for src, dst, label in raw_transitions:
        evs = extract_events(label)
        guard = ""
        action = ""
        # Extract guard [xxx]
        g = re.search(r'\[(.*?)\]', label)
        if g:
            guard = g.group(1).strip()
        # Extract action after /
        if '/' in label:
            action = label.split('/', 1)[1].strip().split('\n')[0]

        src_node = composites.get(src)
        dst_node = composites.get(dst)

        if src_node and dst_node:
            # Composite → Composite: expand to all inner leaves
            src_leaves = src_node.leaf_states()
            dst_initial = find_initial_leaf(dst_node, flat)
            for ev in evs:
                for leaf in src_leaves:
                    add_transition(leaf.full_name(), ev, dst_initial or dst, guard, action)
        elif src_node and not dst_node:
            # Composite → Simple: expand source
            src_leaves = src_node.leaf_states()
            dst_full = resolve_state(dst, None, flat)
            for ev in evs:
                for leaf in src_leaves:
                    add_transition(leaf.full_name(), ev, dst_full or dst, guard, action)
        elif not src_node and dst_node:
            # Simple → Composite: resolve target initial
            src_full = resolve_state(src, None, flat)
            dst_initial = find_initial_leaf(dst_node, flat)
            for ev in evs:
                add_transition(src_full or src, ev, dst_initial or dst, guard, action)
        else:
            # Simple → Simple
            src_full = resolve_state(src, None, flat)
            dst_full = resolve_state(dst, None, flat)
            for ev in evs:
                add_transition(src_full or src, ev, dst_full or dst, guard, action)

    # Special: History state handling
    # ObstacleAvoidance --> History : ObstacleCleared
    # We emit a comment in the generated code for the user to handle manually
    history_comment = ""
    for src, dst, label in raw_transitions:
        if dst == "History" or "History" in label:
            history_comment = f"// NOTE: History state transition detected: {src} --> {dst} : {label}\n"

    # ============================================================
    # Generate header
    # ============================================================
    hpp = []
    hpp.append('#pragma once')
    hpp.append('')
    hpp.append('#include "tinyfsm.hpp"')
    hpp.append('#include <cstdint>')
    hpp.append('')
    hpp.append('// ============================================================')
    hpp.append('// Auto-generated from docs/robot_fsm.puml')
    hpp.append('// Do NOT edit this file manually; re-run tools/generate_fsm.py')
    hpp.append('// ============================================================')
    hpp.append('')

    # Event struct
    hpp.append('struct RobotEvent : tinyfsm::Event {')
    hpp.append('    enum class Type {')
    for ev in events:
        hpp.append(f'        {ev},')
    hpp.append('    } type;')
    hpp.append('')
    hpp.append('    uint32_t uid = 0;')
    hpp.append('    bool was_charging = false;')
    hpp.append('};')
    hpp.append('')

    # FSM base
    hpp.append('struct FSM_Robot : tinyfsm::Fsm<FSM_Robot> {')
    hpp.append('    // History emulation for Navigating sub-states')
    hpp.append('    enum class NavMode { LineFollowing, GridTraversing };')
    hpp.append('    NavMode previous_nav_mode_ = NavMode::LineFollowing;')
    hpp.append('')
    hpp.append('    virtual void react(RobotEvent const & e) = 0;')
    hpp.append('    virtual void entry() = 0;')
    hpp.append('    virtual void exit() = 0;')
    hpp.append('};')
    hpp.append('')

    # State declarations (skip history pseudo-states)
    for name in sorted(flat.keys()):
        if not name or name in history_states:
            continue
        hpp.append(f'struct State_{name} : FSM_Robot {{')
        hpp.append('    void entry() override;')
        hpp.append('    void exit() override;')
        hpp.append('    void react(RobotEvent const & e) override;')
        hpp.append('};')
        hpp.append('')

    # Initial state macro must go into a single .cpp file to avoid ODR violations
    initial = root.initial_child
    initial_state_macro = ""
    if initial:
        initial_full = resolve_state(initial, root, flat)
        if initial_full:
            initial_state_macro = f'FSM_INITIAL_STATE(FSM_Robot, State_{initial_full});'
        else:
            init_node = next((c for c in root.children if c.name == initial), None)
            if init_node and init_node.is_composite:
                init_leaf = find_initial_leaf(init_node, flat)
                if init_leaf:
                    initial_state_macro = f'FSM_INITIAL_STATE(FSM_Robot, State_{init_leaf});'

    # ============================================================
    # Generate implementation stub
    # ============================================================
    cpp = []
    cpp.append('#include "robot_fsm_gen.hpp"')
    cpp.append('#include <stdio.h>')
    if initial_state_macro:
        cpp.append('')
        cpp.append(initial_state_macro)
    cpp.append('')
    cpp.append('// ============================================================')
    cpp.append('// Auto-generated stub implementations')
    cpp.append('// Fill in entry(), exit(), and react() bodies with real logic')
    cpp.append('// ============================================================')
    cpp.append('')

    for name in sorted(flat.keys()):
        if not name or name in history_states:
            continue
        cpp.append(f'// ----- State: {name} -----')
        cpp.append(f'void State_{name}::entry() {{')
        cpp.append(f'    printf("[FSM] Enter {name}\\n");')
        cpp.append('}')
        cpp.append('')
        cpp.append(f'void State_{name}::exit() {{')
        cpp.append(f'    printf("[FSM] Exit {name}\\n");')
        cpp.append('}')
        cpp.append('')

        # Build react() switch body
        body_lines = []
        handled_events = set()
        for ev in events:
            key = (name, ev)
            if key not in trans_map:
                continue
            handled_events.add(ev)
            targets = trans_map[key]
            if len(targets) == 1:
                dst_full, guard, action = targets[0]
                body_lines.append(f'        case RobotEvent::Type::{ev}:')

                # Special: save nav mode before entering ObstacleAvoidance
                if 'ObstacleAvoidance' in dst_full and ('LineFollowing' in name or 'GridTraversing' in name):
                    mode = 'LineFollowing' if 'LineFollowing' in name else 'GridTraversing'
                    body_lines.append(f'            previous_nav_mode_ = NavMode::{mode};')

                if guard:
                    if guard == 'was charging':
                        body_lines.append(f'            if (!e.was_charging) break;')
                    elif guard == 'was standby':
                        body_lines.append(f'            if (e.was_charging) break;')
                    else:
                        body_lines.append(f'            // Guard: [{guard}] — evaluate manually')
                        body_lines.append(f'            if (false) break;')
                if action:
                    body_lines.append(f'            // Action: {action}')

                # Special: History state emulation
                if dst_full in history_states:
                    body_lines.append(f'            switch (previous_nav_mode_) {{')
                    body_lines.append(f'                case NavMode::LineFollowing:')
                    body_lines.append(f'                    transit<State_{name.rsplit("_", 1)[0]}_LineFollowing>();')
                    body_lines.append(f'                    break;')
                    body_lines.append(f'                case NavMode::GridTraversing:')
                    body_lines.append(f'                    transit<State_{name.rsplit("_", 1)[0]}_GridTraversing>();')
                    body_lines.append(f'                    break;')
                    body_lines.append(f'            }}')
                else:
                    body_lines.append(f'            transit<State_{dst_full}>();')
                body_lines.append(f'            break;')
            else:
                body_lines.append(f'        case RobotEvent::Type::{ev}:')
                body_lines.append(f'            // Multiple targets (first wins in stub)')
                for i, (dst_full, guard, action) in enumerate(targets):
                    prefix = 'if' if i == 0 else 'else if'
                    if guard == 'was charging':
                        body_lines.append(f'            {prefix} (e.was_charging) {{')
                    elif guard == 'was standby':
                        body_lines.append(f'            {prefix} (!e.was_charging) {{')
                    elif guard:
                        body_lines.append(f'            {prefix} (/* TODO: [{guard}] */) {{')
                    else:
                        body_lines.append(f'            {prefix} (true) {{')
                    body_lines.append(f'                transit<State_{dst_full}>();')
                    body_lines.append(f'            }}')
                body_lines.append(f'            break;')

        if body_lines:
            cpp.append(f'void State_{name}::react(RobotEvent const & e) {{')
            cpp.append('    switch (e.type) {')
            for line in body_lines:
                cpp.append(line)
            cpp.append('        default:')
            cpp.append('            break;')
            cpp.append('    }')
            cpp.append('}')
        else:
            cpp.append(f'void State_{name}::react(RobotEvent const & e) {{')
            cpp.append('    // No outgoing transitions defined')
            cpp.append('}')
        cpp.append('')

    if history_comment:
        cpp.append(history_comment)

    # Write files
    OUT_HPP.write_text('\n'.join(hpp), encoding='utf-8')
    OUT_CPP.write_text('\n'.join(cpp), encoding='utf-8')
    print(f"Generated: {OUT_HPP}")
    print(f"Generated: {OUT_CPP}")


if __name__ == '__main__':
    root, flat, raw_transitions = build_state_tree()
    generate(root, flat, raw_transitions)

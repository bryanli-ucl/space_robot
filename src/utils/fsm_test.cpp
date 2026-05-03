#include <Arduino.h>
#include <mbed.h>

#include "../tasks/robot_fsm_gen.hpp"

#include <stdio.h>
#include <string.h>

using namespace rtos;
using namespace std::chrono_literals;

// Map user-typed command strings to RobotEvent::Type
static auto parse_event(const char* cmd) -> RobotEvent::Type {
    if (strcmp(cmd, "launch") == 0)               return RobotEvent::Type::Launch;
    if (strcmp(cmd, "rfid_b_read") == 0)          return RobotEvent::Type::RFID_B_Read;
    if (strcmp(cmd, "exit_clearance") == 0)       return RobotEvent::Type::ExitClearanceGranted;
    if (strcmp(cmd, "door_opened") == 0)          return RobotEvent::Type::DoorOpened;
    if (strcmp(cmd, "tunnel_exited") == 0)        return RobotEvent::Type::TunnelExited;
    if (strcmp(cmd, "line_lost") == 0)            return RobotEvent::Type::LineLost;
    if (strcmp(cmd, "line_detected") == 0)        return RobotEvent::Type::LineDetected;
    if (strcmp(cmd, "obstacle_detected") == 0)    return RobotEvent::Type::ObstacleDetected;
    if (strcmp(cmd, "obstacle_cleared") == 0)     return RobotEvent::Type::ObstacleCleared;
    if (strcmp(cmd, "rfid_detected") == 0)        return RobotEvent::Type::RFIDDetected;
    if (strcmp(cmd, "read_success") == 0)         return RobotEvent::Type::ReadSuccess;
    if (strcmp(cmd, "response_fertile") == 0)     return RobotEvent::Type::Response_Fertile;
    if (strcmp(cmd, "response_infertile") == 0)   return RobotEvent::Type::Response_Infertile;
    if (strcmp(cmd, "response_covered") == 0)     return RobotEvent::Type::Response_Covered;
    if (strcmp(cmd, "seed_planted") == 0)         return RobotEvent::Type::SeedPlanted;
    if (strcmp(cmd, "light_scan") == 0)           return RobotEvent::Type::PeriodicLightScan;
    if (strcmp(cmd, "light_recorded") == 0)       return RobotEvent::Type::LightRecorded;
    if (strcmp(cmd, "mission_complete") == 0)     return RobotEvent::Type::MissionComplete;
    if (strcmp(cmd, "all_seeds") == 0)            return RobotEvent::Type::AllSeedsPlanted;
    if (strcmp(cmd, "low_battery") == 0)          return RobotEvent::Type::LowBattery;
    if (strcmp(cmd, "emergency") == 0)            return RobotEvent::Type::EmergencyWarning;
    if (strcmp(cmd, "airlock_a") == 0)            return RobotEvent::Type::AirlockA_Reached;
    if (strcmp(cmd, "timer_expired") == 0)        return RobotEvent::Type::TimerExpired;
    if (strcmp(cmd, "battery_depleted") == 0)     return RobotEvent::Type::BatteryDepleted;
    if (strcmp(cmd, "emergency_timer") == 0)      return RobotEvent::Type::EmergencyTimerExpired;
    if (strcmp(cmd, "parking") == 0)              return RobotEvent::Type::ParkingReached;
    if (strcmp(cmd, "charge_complete") == 0)      return RobotEvent::Type::ChargeComplete;
    if (strcmp(cmd, "revive") == 0)               return RobotEvent::Type::ReviveButtonPressed_LED_RedToGreen;
    if (strcmp(cmd, "wifi_enabled") == 0)         return RobotEvent::Type::WiFiReenabled;
    if (strcmp(cmd, "open_door_req") == 0)        return RobotEvent::Type::OpenDoorRequest;
    if (strcmp(cmd, "assistance_done") == 0)      return RobotEvent::Type::AssistanceComplete;
    if (strcmp(cmd, "tag_a_reached") == 0)        return RobotEvent::Type::TagA_Reached;
    if (strcmp(cmd, "tag_a_pressed") == 0)        return RobotEvent::Type::TagA_Pressed;
    return RobotEvent::Type::Launch; // fallback
}

static void print_current_state() {
    // TinyFSM does not expose current state name at runtime.
    // We use a set of is_in_state<> checks.
    if      (FSM_Robot::is_in_state<State_Standby>())                                    printf("[STATE] Standby\n");
    else if (FSM_Robot::is_in_state<State_ExitingBase_ApproachingTagB>())               printf("[STATE] ExitingBase::ApproachingTagB\n");
    else if (FSM_Robot::is_in_state<State_ExitingBase_RequestingClearance>())           printf("[STATE] ExitingBase::RequestingClearance\n");
    else if (FSM_Robot::is_in_state<State_ExitingBase_WaitingForDoorB>())               printf("[STATE] ExitingBase::WaitingForDoorB\n");
    else if (FSM_Robot::is_in_state<State_ExitingBase_TraversingTunnelB>())             printf("[STATE] ExitingBase::TraversingTunnelB\n");
    else if (FSM_Robot::is_in_state<State_SurfaceExploration_Navigating_LineFollowing>()) printf("[STATE] SurfaceExploration::Navigating::LineFollowing\n");
    else if (FSM_Robot::is_in_state<State_SurfaceExploration_Navigating_GridTraversing>()) printf("[STATE] SurfaceExploration::Navigating::GridTraversing\n");
    else if (FSM_Robot::is_in_state<State_SurfaceExploration_Navigating_ObstacleAvoidance>()) printf("[STATE] SurfaceExploration::Navigating::ObstacleAvoidance\n");
    else if (FSM_Robot::is_in_state<State_SurfaceExploration_ReadingRFID>())            printf("[STATE] SurfaceExploration::ReadingRFID\n");
    else if (FSM_Robot::is_in_state<State_SurfaceExploration_QueryingServer>())         printf("[STATE] SurfaceExploration::QueryingServer\n");
    else if (FSM_Robot::is_in_state<State_SurfaceExploration_Planting>())               printf("[STATE] SurfaceExploration::Planting\n");
    else if (FSM_Robot::is_in_state<State_SurfaceExploration_ScanningForLight>())       printf("[STATE] SurfaceExploration::ScanningForLight\n");
    else if (FSM_Robot::is_in_state<State_ReturningToBase>())                           printf("[STATE] ReturningToBase\n");

    else if (FSM_Robot::is_in_state<State_EnteringBase_WaitingForDoor>())               printf("[STATE] EnteringBase::WaitingForDoor\n");
    else if (FSM_Robot::is_in_state<State_EnteringBase_TraversingTunnelA>())            printf("[STATE] EnteringBase::TraversingTunnelA\n");
    else if (FSM_Robot::is_in_state<State_Charging>())                                  printf("[STATE] Charging\n");
    else if (FSM_Robot::is_in_state<State_Stranded>())                                  printf("[STATE] Stranded\n");
    else if (FSM_Robot::is_in_state<State_BeingRevived>())                              printf("[STATE] BeingRevived\n");
    else if (FSM_Robot::is_in_state<State_AssistingEntry_MovingToTagA>())               printf("[STATE] AssistingEntry::MovingToTagA\n");
    else if (FSM_Robot::is_in_state<State_AssistingEntry_PressingTagA>())               printf("[STATE] AssistingEntry::PressingTagA\n");
    else if (FSM_Robot::is_in_state<State_AssistingEntry_ReturningToParking>())        printf("[STATE] AssistingEntry::ReturningToParking\n");
    else                                                                                printf("[STATE] UNKNOWN\n");
}

static void print_help() {
    printf("\n========== FSM Test Commands ==========\n");
    printf("Type an event name and press Enter to dispatch it.\n");
    printf("Common flow:\n");
    printf("  launch → rfid_b_read → exit_clearance → door_opened → tunnel_exited\n");
    printf("  → rfid_detected → read_success → response_fertile → seed_planted\n");
    printf("  → mission_complete → airlock_a → door_opened → parking → charge_complete\n");
    printf("Other events:\n");
    printf("  line_lost, line_detected, obstacle_detected, obstacle_cleared\n");
    printf("  response_infertile, response_covered, light_scan, light_recorded\n");
    printf("  emergency, low_battery, all_seeds, timer_expired, battery_depleted\n");
    printf("  emergency_timer, revive, wifi_enabled\n");
    printf("  open_door_req, tag_a_reached, tag_a_pressed, assistance_done\n");
    printf("Special:\n");
    printf("  state   → print current state\n");
    printf("  help    → show this help\n");
    printf("========================================\n\n");
}

void setup() {
    Serial3.begin(115200);
    while (!Serial3) delay(100);
    printf("\n========== FSM Test Start ==========\n\n");

    FSM_Robot::start();
    print_current_state();
    print_help();
}

void loop() {
    if (Serial3.available()) {
        String line = Serial3.readStringUntil('\n');
        line.trim();
        line.toLowerCase();

        if (line.length() == 0) return;

        if (line == "help" || line == "h") {
            print_help();
            return;
        }
        if (line == "state" || line == "s") {
            print_current_state();
            return;
        }

        RobotEvent ev;
        ev.type = parse_event(line.c_str());
        ev.was_charging = (line == "assistance_done"); // simple test default

        printf(">>> Dispatch: %s\n", line.c_str());
        FSM_Robot::dispatch(ev);
        print_current_state();
    }
}

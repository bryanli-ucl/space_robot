#include "robot_fsm_gen.hpp"
#include <stdio.h>

FSM_INITIAL_STATE(FSM_Robot, State_Standby);

// ============================================================
// Auto-generated stub implementations
// Fill in entry(), exit(), and react() bodies with real logic
// ============================================================

// ----- State: AssistingEntry_MovingToTagA -----
void State_AssistingEntry_MovingToTagA::entry() {
    printf("[FSM] Enter AssistingEntry_MovingToTagA\n");
}

void State_AssistingEntry_MovingToTagA::exit() {
    printf("[FSM] Exit AssistingEntry_MovingToTagA\n");
}

void State_AssistingEntry_MovingToTagA::react(RobotEvent const & e) {
    switch (e.type) {
        case RobotEvent::Type::AssistanceComplete:
            // Multiple targets (first wins in stub)
            if (e.was_charging) {
                transit<State_Charging>();
            }
            else if (!e.was_charging) {
                transit<State_Standby>();
            }
            break;
        case RobotEvent::Type::TagA_Reached:
            transit<State_AssistingEntry_PressingTagA>();
            break;
        default:
            break;
    }
}

// ----- State: AssistingEntry_PressingTagA -----
void State_AssistingEntry_PressingTagA::entry() {
    printf("[FSM] Enter AssistingEntry_PressingTagA\n");
}

void State_AssistingEntry_PressingTagA::exit() {
    printf("[FSM] Exit AssistingEntry_PressingTagA\n");
}

void State_AssistingEntry_PressingTagA::react(RobotEvent const & e) {
    switch (e.type) {
        case RobotEvent::Type::AssistanceComplete:
            // Multiple targets (first wins in stub)
            if (e.was_charging) {
                transit<State_Charging>();
            }
            else if (!e.was_charging) {
                transit<State_Standby>();
            }
            break;
        case RobotEvent::Type::TagA_Pressed:
            transit<State_AssistingEntry_ReturningToParking>();
            break;
        default:
            break;
    }
}

// ----- State: AssistingEntry_ReturningToParking -----
void State_AssistingEntry_ReturningToParking::entry() {
    printf("[FSM] Enter AssistingEntry_ReturningToParking\n");
}

void State_AssistingEntry_ReturningToParking::exit() {
    printf("[FSM] Exit AssistingEntry_ReturningToParking\n");
}

void State_AssistingEntry_ReturningToParking::react(RobotEvent const & e) {
    switch (e.type) {
        case RobotEvent::Type::AssistanceComplete:
            // Multiple targets (first wins in stub)
            if (e.was_charging) {
                transit<State_Charging>();
            }
            else if (!e.was_charging) {
                transit<State_Standby>();
            }
            break;
        default:
            break;
    }
}

// ----- State: BeingRevived -----
void State_BeingRevived::entry() {
    printf("[FSM] Enter BeingRevived\n");
}

void State_BeingRevived::exit() {
    printf("[FSM] Exit BeingRevived\n");
}

void State_BeingRevived::react(RobotEvent const & e) {
    switch (e.type) {
        case RobotEvent::Type::WiFiReenabled:
            transit<State_EmergencyReturn>();
            break;
        default:
            break;
    }
}

// ----- State: Charging -----
void State_Charging::entry() {
    printf("[FSM] Enter Charging\n");
}

void State_Charging::exit() {
    printf("[FSM] Exit Charging\n");
}

void State_Charging::react(RobotEvent const & e) {
    switch (e.type) {
        case RobotEvent::Type::ChargeComplete:
            transit<State_Standby>();
            break;
        case RobotEvent::Type::OpenDoorRequest:
            // Guard: [from server] — evaluate manually
            if (false) break;
            transit<State_AssistingEntry_MovingToTagA>();
            break;
        default:
            break;
    }
}

// ----- State: EmergencyReturn -----
void State_EmergencyReturn::entry() {
    printf("[FSM] Enter EmergencyReturn\n");
}

void State_EmergencyReturn::exit() {
    printf("[FSM] Exit EmergencyReturn\n");
}

void State_EmergencyReturn::react(RobotEvent const & e) {
    // No outgoing transitions defined
}

// ----- State: EnteringBase_TraversingTunnelA -----
void State_EnteringBase_TraversingTunnelA::entry() {
    printf("[FSM] Enter EnteringBase_TraversingTunnelA\n");
}

void State_EnteringBase_TraversingTunnelA::exit() {
    printf("[FSM] Exit EnteringBase_TraversingTunnelA\n");
}

void State_EnteringBase_TraversingTunnelA::react(RobotEvent const & e) {
    switch (e.type) {
        case RobotEvent::Type::ParkingReached:
            transit<State_Charging>();
            break;
        default:
            break;
    }
}

// ----- State: EnteringBase_WaitingForDoor -----
void State_EnteringBase_WaitingForDoor::entry() {
    printf("[FSM] Enter EnteringBase_WaitingForDoor\n");
}

void State_EnteringBase_WaitingForDoor::exit() {
    printf("[FSM] Exit EnteringBase_WaitingForDoor\n");
}

void State_EnteringBase_WaitingForDoor::react(RobotEvent const & e) {
    switch (e.type) {
        case RobotEvent::Type::DoorOpened:
            transit<State_EnteringBase_TraversingTunnelA>();
            break;
        case RobotEvent::Type::ParkingReached:
            transit<State_Charging>();
            break;
        default:
            break;
    }
}

// ----- State: ExitingBase_ApproachingTagB -----
void State_ExitingBase_ApproachingTagB::entry() {
    printf("[FSM] Enter ExitingBase_ApproachingTagB\n");
}

void State_ExitingBase_ApproachingTagB::exit() {
    printf("[FSM] Exit ExitingBase_ApproachingTagB\n");
}

void State_ExitingBase_ApproachingTagB::react(RobotEvent const & e) {
    switch (e.type) {
        case RobotEvent::Type::RFID_B_Read:
            transit<State_ExitingBase_RequestingClearance>();
            break;
        case RobotEvent::Type::TunnelExited:
            transit<State_SurfaceExploration_Navigating_LineFollowing>();
            break;
        default:
            break;
    }
}

// ----- State: ExitingBase_RequestingClearance -----
void State_ExitingBase_RequestingClearance::entry() {
    printf("[FSM] Enter ExitingBase_RequestingClearance\n");
}

void State_ExitingBase_RequestingClearance::exit() {
    printf("[FSM] Exit ExitingBase_RequestingClearance\n");
}

void State_ExitingBase_RequestingClearance::react(RobotEvent const & e) {
    switch (e.type) {
        case RobotEvent::Type::ExitClearanceGranted:
            transit<State_ExitingBase_WaitingForDoorB>();
            break;
        case RobotEvent::Type::TunnelExited:
            transit<State_SurfaceExploration_Navigating_LineFollowing>();
            break;
        default:
            break;
    }
}

// ----- State: ExitingBase_TraversingTunnelB -----
void State_ExitingBase_TraversingTunnelB::entry() {
    printf("[FSM] Enter ExitingBase_TraversingTunnelB\n");
}

void State_ExitingBase_TraversingTunnelB::exit() {
    printf("[FSM] Exit ExitingBase_TraversingTunnelB\n");
}

void State_ExitingBase_TraversingTunnelB::react(RobotEvent const & e) {
    switch (e.type) {
        case RobotEvent::Type::TunnelExited:
            transit<State_SurfaceExploration_Navigating_LineFollowing>();
            break;
        default:
            break;
    }
}

// ----- State: ExitingBase_WaitingForDoorB -----
void State_ExitingBase_WaitingForDoorB::entry() {
    printf("[FSM] Enter ExitingBase_WaitingForDoorB\n");
}

void State_ExitingBase_WaitingForDoorB::exit() {
    printf("[FSM] Exit ExitingBase_WaitingForDoorB\n");
}

void State_ExitingBase_WaitingForDoorB::react(RobotEvent const & e) {
    switch (e.type) {
        case RobotEvent::Type::DoorOpened:
            transit<State_ExitingBase_TraversingTunnelB>();
            break;
        case RobotEvent::Type::TunnelExited:
            transit<State_SurfaceExploration_Navigating_LineFollowing>();
            break;
        default:
            break;
    }
}

// ----- State: ReturningToBase -----
void State_ReturningToBase::entry() {
    printf("[FSM] Enter ReturningToBase\n");
}

void State_ReturningToBase::exit() {
    printf("[FSM] Exit ReturningToBase\n");
}

void State_ReturningToBase::react(RobotEvent const & e) {
    switch (e.type) {
        case RobotEvent::Type::AirlockA_Reached:
            transit<State_EnteringBase_WaitingForDoor>();
            break;
        case RobotEvent::Type::BatteryDepleted:
            // Action: BatteryDepleted\n/ EmergencyTimerExpired
            transit<State_Stranded>();
            break;
        case RobotEvent::Type::EmergencyTimerExpired:
            // Action: BatteryDepleted\n/ EmergencyTimerExpired
            transit<State_Stranded>();
            break;
        case RobotEvent::Type::TimerExpired:
            // Action: BatteryDepleted\n/ EmergencyTimerExpired
            transit<State_Stranded>();
            break;
        default:
            break;
    }
}

// ----- State: Standby -----
void State_Standby::entry() {
    printf("[FSM] Enter Standby\n");
}

void State_Standby::exit() {
    printf("[FSM] Exit Standby\n");
}

void State_Standby::react(RobotEvent const & e) {
    switch (e.type) {
        case RobotEvent::Type::Launch:
            // Action: ManualDeploy
            transit<State_ExitingBase_ApproachingTagB>();
            break;
        case RobotEvent::Type::OpenDoorRequest:
            // Guard: [from server] — evaluate manually
            if (false) break;
            transit<State_AssistingEntry_MovingToTagA>();
            break;
        default:
            break;
    }
}

// ----- State: Stranded -----
void State_Stranded::entry() {
    printf("[FSM] Enter Stranded\n");
}

void State_Stranded::exit() {
    printf("[FSM] Exit Stranded\n");
}

void State_Stranded::react(RobotEvent const & e) {
    switch (e.type) {
        case RobotEvent::Type::ReviveButtonPressed_LED_RedToGreen:
            transit<State_BeingRevived>();
            break;
        default:
            break;
    }
}

// ----- State: SurfaceExploration_Navigating_GridTraversing -----
void State_SurfaceExploration_Navigating_GridTraversing::entry() {
    printf("[FSM] Enter SurfaceExploration_Navigating_GridTraversing\n");
}

void State_SurfaceExploration_Navigating_GridTraversing::exit() {
    printf("[FSM] Exit SurfaceExploration_Navigating_GridTraversing\n");
}

void State_SurfaceExploration_Navigating_GridTraversing::react(RobotEvent const & e) {
    switch (e.type) {
        case RobotEvent::Type::AllSeedsPlanted:
            // Action: AllSeedsPlanted\n/ LowBattery
            transit<State_ReturningToBase>();
            break;
        case RobotEvent::Type::BonusTrigger:
            // Action: BonusTrigger
            transit<State_SurfaceExploration_ScanningForLight>();
            break;
        case RobotEvent::Type::EmergencyWarning:
            transit<State_ReturningToBase>();
            break;
        case RobotEvent::Type::LineDetected:
            transit<State_SurfaceExploration_Navigating_LineFollowing>();
            break;
        case RobotEvent::Type::LowBattery:
            // Action: AllSeedsPlanted\n/ LowBattery
            transit<State_ReturningToBase>();
            break;
        case RobotEvent::Type::MissionComplete:
            // Action: AllSeedsPlanted\n/ LowBattery
            transit<State_ReturningToBase>();
            break;
        case RobotEvent::Type::ObstacleDetected:
            previous_nav_mode_ = NavMode::GridTraversing;
            transit<State_SurfaceExploration_Navigating_ObstacleAvoidance>();
            break;
        case RobotEvent::Type::PeriodicLightScan:
            // Action: BonusTrigger
            transit<State_SurfaceExploration_ScanningForLight>();
            break;
        case RobotEvent::Type::RFIDDetected:
            transit<State_SurfaceExploration_ReadingRFID>();
            break;
        default:
            break;
    }
}

// ----- State: SurfaceExploration_Navigating_LineFollowing -----
void State_SurfaceExploration_Navigating_LineFollowing::entry() {
    printf("[FSM] Enter SurfaceExploration_Navigating_LineFollowing\n");
}

void State_SurfaceExploration_Navigating_LineFollowing::exit() {
    printf("[FSM] Exit SurfaceExploration_Navigating_LineFollowing\n");
}

void State_SurfaceExploration_Navigating_LineFollowing::react(RobotEvent const & e) {
    switch (e.type) {
        case RobotEvent::Type::AllSeedsPlanted:
            // Action: AllSeedsPlanted\n/ LowBattery
            transit<State_ReturningToBase>();
            break;
        case RobotEvent::Type::BonusTrigger:
            // Action: BonusTrigger
            transit<State_SurfaceExploration_ScanningForLight>();
            break;
        case RobotEvent::Type::EmergencyWarning:
            transit<State_ReturningToBase>();
            break;
        case RobotEvent::Type::LineLost:
            transit<State_SurfaceExploration_Navigating_GridTraversing>();
            break;
        case RobotEvent::Type::LowBattery:
            // Action: AllSeedsPlanted\n/ LowBattery
            transit<State_ReturningToBase>();
            break;
        case RobotEvent::Type::MissionComplete:
            // Action: AllSeedsPlanted\n/ LowBattery
            transit<State_ReturningToBase>();
            break;
        case RobotEvent::Type::ObstacleDetected:
            previous_nav_mode_ = NavMode::LineFollowing;
            transit<State_SurfaceExploration_Navigating_ObstacleAvoidance>();
            break;
        case RobotEvent::Type::PeriodicLightScan:
            // Action: BonusTrigger
            transit<State_SurfaceExploration_ScanningForLight>();
            break;
        case RobotEvent::Type::RFIDDetected:
            transit<State_SurfaceExploration_ReadingRFID>();
            break;
        default:
            break;
    }
}

// ----- State: SurfaceExploration_Navigating_ObstacleAvoidance -----
void State_SurfaceExploration_Navigating_ObstacleAvoidance::entry() {
    printf("[FSM] Enter SurfaceExploration_Navigating_ObstacleAvoidance\n");
}

void State_SurfaceExploration_Navigating_ObstacleAvoidance::exit() {
    printf("[FSM] Exit SurfaceExploration_Navigating_ObstacleAvoidance\n");
}

void State_SurfaceExploration_Navigating_ObstacleAvoidance::react(RobotEvent const & e) {
    switch (e.type) {
        case RobotEvent::Type::AllSeedsPlanted:
            // Action: AllSeedsPlanted\n/ LowBattery
            transit<State_ReturningToBase>();
            break;
        case RobotEvent::Type::BonusTrigger:
            // Action: BonusTrigger
            transit<State_SurfaceExploration_ScanningForLight>();
            break;
        case RobotEvent::Type::EmergencyWarning:
            transit<State_ReturningToBase>();
            break;
        case RobotEvent::Type::LowBattery:
            // Action: AllSeedsPlanted\n/ LowBattery
            transit<State_ReturningToBase>();
            break;
        case RobotEvent::Type::MissionComplete:
            // Action: AllSeedsPlanted\n/ LowBattery
            transit<State_ReturningToBase>();
            break;
        case RobotEvent::Type::ObstacleCleared:
            switch (previous_nav_mode_) {
                case NavMode::LineFollowing:
                    transit<State_SurfaceExploration_Navigating_LineFollowing>();
                    break;
                case NavMode::GridTraversing:
                    transit<State_SurfaceExploration_Navigating_GridTraversing>();
                    break;
            }
            break;
        case RobotEvent::Type::PeriodicLightScan:
            // Action: BonusTrigger
            transit<State_SurfaceExploration_ScanningForLight>();
            break;
        case RobotEvent::Type::RFIDDetected:
            transit<State_SurfaceExploration_ReadingRFID>();
            break;
        default:
            break;
    }
}

// ----- State: SurfaceExploration_Planting -----
void State_SurfaceExploration_Planting::entry() {
    printf("[FSM] Enter SurfaceExploration_Planting\n");
}

void State_SurfaceExploration_Planting::exit() {
    printf("[FSM] Exit SurfaceExploration_Planting\n");
}

void State_SurfaceExploration_Planting::react(RobotEvent const & e) {
    switch (e.type) {
        case RobotEvent::Type::AllSeedsPlanted:
            // Action: AllSeedsPlanted\n/ LowBattery
            transit<State_ReturningToBase>();
            break;
        case RobotEvent::Type::EmergencyWarning:
            transit<State_ReturningToBase>();
            break;
        case RobotEvent::Type::LowBattery:
            // Action: AllSeedsPlanted\n/ LowBattery
            transit<State_ReturningToBase>();
            break;
        case RobotEvent::Type::MissionComplete:
            // Action: AllSeedsPlanted\n/ LowBattery
            transit<State_ReturningToBase>();
            break;
        case RobotEvent::Type::SeedPlanted:
            transit<State_SurfaceExploration_Navigating_LineFollowing>();
            break;
        default:
            break;
    }
}

// ----- State: SurfaceExploration_QueryingServer -----
void State_SurfaceExploration_QueryingServer::entry() {
    printf("[FSM] Enter SurfaceExploration_QueryingServer\n");
}

void State_SurfaceExploration_QueryingServer::exit() {
    printf("[FSM] Exit SurfaceExploration_QueryingServer\n");
}

void State_SurfaceExploration_QueryingServer::react(RobotEvent const & e) {
    switch (e.type) {
        case RobotEvent::Type::AllSeedsPlanted:
            // Action: AllSeedsPlanted\n/ LowBattery
            transit<State_ReturningToBase>();
            break;
        case RobotEvent::Type::EmergencyWarning:
            transit<State_ReturningToBase>();
            break;
        case RobotEvent::Type::LowBattery:
            // Action: AllSeedsPlanted\n/ LowBattery
            transit<State_ReturningToBase>();
            break;
        case RobotEvent::Type::MissionComplete:
            // Action: AllSeedsPlanted\n/ LowBattery
            transit<State_ReturningToBase>();
            break;
        case RobotEvent::Type::Response_Covered:
            // Guard: [mountain range] — evaluate manually
            if (false) break;
            transit<State_SurfaceExploration_Navigating_LineFollowing>();
            break;
        case RobotEvent::Type::Response_Fertile:
            transit<State_SurfaceExploration_Planting>();
            break;
        case RobotEvent::Type::Response_Infertile:
            transit<State_SurfaceExploration_Navigating_LineFollowing>();
            break;
        default:
            break;
    }
}

// ----- State: SurfaceExploration_ReadingRFID -----
void State_SurfaceExploration_ReadingRFID::entry() {
    printf("[FSM] Enter SurfaceExploration_ReadingRFID\n");
}

void State_SurfaceExploration_ReadingRFID::exit() {
    printf("[FSM] Exit SurfaceExploration_ReadingRFID\n");
}

void State_SurfaceExploration_ReadingRFID::react(RobotEvent const & e) {
    switch (e.type) {
        case RobotEvent::Type::AllSeedsPlanted:
            // Action: AllSeedsPlanted\n/ LowBattery
            transit<State_ReturningToBase>();
            break;
        case RobotEvent::Type::EmergencyWarning:
            transit<State_ReturningToBase>();
            break;
        case RobotEvent::Type::LowBattery:
            // Action: AllSeedsPlanted\n/ LowBattery
            transit<State_ReturningToBase>();
            break;
        case RobotEvent::Type::MissionComplete:
            // Action: AllSeedsPlanted\n/ LowBattery
            transit<State_ReturningToBase>();
            break;
        case RobotEvent::Type::ReadSuccess:
            transit<State_SurfaceExploration_QueryingServer>();
            break;
        default:
            break;
    }
}

// ----- State: SurfaceExploration_ScanningForLight -----
void State_SurfaceExploration_ScanningForLight::entry() {
    printf("[FSM] Enter SurfaceExploration_ScanningForLight\n");
}

void State_SurfaceExploration_ScanningForLight::exit() {
    printf("[FSM] Exit SurfaceExploration_ScanningForLight\n");
}

void State_SurfaceExploration_ScanningForLight::react(RobotEvent const & e) {
    switch (e.type) {
        case RobotEvent::Type::AllSeedsPlanted:
            // Action: AllSeedsPlanted\n/ LowBattery
            transit<State_ReturningToBase>();
            break;
        case RobotEvent::Type::EmergencyWarning:
            transit<State_ReturningToBase>();
            break;
        case RobotEvent::Type::LightRecorded:
            transit<State_SurfaceExploration_Navigating_LineFollowing>();
            break;
        case RobotEvent::Type::LowBattery:
            // Action: AllSeedsPlanted\n/ LowBattery
            transit<State_ReturningToBase>();
            break;
        case RobotEvent::Type::MissionComplete:
            // Action: AllSeedsPlanted\n/ LowBattery
            transit<State_ReturningToBase>();
            break;
        default:
            break;
    }
}

// NOTE: History state transition detected: ObstacleAvoidance --> History : ObstacleCleared

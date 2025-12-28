#include "FSM/CtrlFSM.h"
#include "FSM/State_Passive.h"
#include "FSM/State_FixStand.h"
#include "FSM/State_RLBase.h"
#include "State_Mimic.h"

std::unique_ptr<LowCmd_t> FSMState::lowcmd = nullptr;
std::shared_ptr<LowState_t> FSMState::lowstate = nullptr;
std::shared_ptr<Keyboard> FSMState::keyboard = std::make_shared<Keyboard>();

void init_fsm_state()
{
    auto lowcmd_sub = std::make_shared<unitree::robot::g1::subscription::LowCmd>();
    usleep(0.2 * 1e6);
    if(!lowcmd_sub->isTimeout())
    {
        spdlog::critical("The other process is using the lowcmd channel, please close it first.");
        unitree::robot::go2::shutdown();
        // exit(0);
    }
    FSMState::lowcmd = std::make_unique<LowCmd_t>();
    FSMState::lowstate = std::make_shared<LowState_t>();
    spdlog::info("Waiting for connection to robot...");
    FSMState::lowstate->wait_for_connection();
    spdlog::info("Connected to robot.");
}

int main(int argc, char** argv)
{
    // Load parameters
    auto vm = param::helper(argc, argv);

    std::cout << " --- Unitree Robotics --- \n";
    std::cout << "     G1-29dof Controller \n";

    // Unitree DDS Config
    unitree::robot::ChannelFactory::Instance()->Init(0, vm["network"].as<std::string>());

    init_fsm_state();

    FSMState::lowcmd->msg_.mode_machine() = 5; // 29dof
    if(!FSMState::lowcmd->check_mode_machine(FSMState::lowstate)) {
        spdlog::critical("Unmatched robot type.");
        exit(-1);
    }
    
    // Initialize FSM
    auto fsm = std::make_unique<CtrlFSM>(param::config["FSM"]);
    fsm->start();

    std::cout << "Use keyboard to control the robot.\n";
    std::cout << "W, A, S, D -> Direction buttons and left joystick (lx, ly)\n";
    std::cout << "J -> A button, K -> B button, U -> X button, I -> Y button\n";
    std::cout << "Q -> LB, E -> RB, Z -> LT, C -> RT\n";
    std::cout << "Space -> Start, N -> Back, F -> F1, G -> F2\n";

    std::cout << "state transition: ";
    std::cout << "Passive -> FixStand -> Velocity to Mimic_Dance_102 \n";
    std::cout << "                   OR  Velocity to Mimic_Gangnam_Style\n";
    std::cout << "tips:click mujoco window and use 7 8 to hang up or put down the robot.\n";
    std::cout << "and use 9 to enable or disable the elastic_band\n";
    std::cout << "and click this terminal and press keys to control the robot based on the following tips.\n";

    std::cout << "GUIDE: firstly click this terminal, let robot convert to fixstand mode and then to velocity mode.secondly click mujoco, press 8 to put down the robot, make sure that robot is standing stably.thirdly click this terminal, press keys to control the robot based on the following tips.\n";

    std::cout << "\n==================first==========================\n";
    std::cout << "Press [Z + J] to enter FixStand mode.\n";
    std::cout << "==================second==========================\n";
    std::cout << "And then press [E + U] to start controlling the robot(Velocity mode).\n";
    std::cout << "\n";
    std::cout << "==================third==========================\n";
    std::cout << "Press [C + K] to enter Dance 102 mode.\n Or Press [C + J] to enter Gangnam Style mode.\n";


    while (true)
    {
        sleep(1);
    }
    
    return 0;
}


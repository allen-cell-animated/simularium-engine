#ifndef AICS_NET_MESSAGE_IDS
#define AICS_NET_MESSAGE_IDS

#include <string>
#include <unordered_map>
#include <vector>

namespace aics {
namespace simularium {

    enum WebRequestTypes {
        id_undefined_web_request = 0,
        id_vis_data_arrive = 1,
        id_vis_data_request,
        id_vis_data_finish,
        id_vis_data_pause,
        id_vis_data_resume,
        id_vis_data_abort,
        id_update_time_step,
        id_update_rate_param,
        id_model_definition,
        id_heartbeat_ping,
        id_heartbeat_pong,
        id_play_cache,
        id_trajectory_file_info,
        id_goto_simulation_time,
        id_init_trajectory_file
    };

    //
    static std::unordered_map<std::size_t, std::string> WebRequestNames {
        { id_undefined_web_request, "undefined" },
        { id_vis_data_arrive, "stream data" },
        { id_vis_data_request, "stream request" },
        { id_vis_data_finish, "stream finish" },
        { id_vis_data_pause, "pause" },
        { id_vis_data_resume, "resume" },
        { id_vis_data_abort, "abort" },
        { id_update_time_step, "update time step" },
        { id_update_rate_param, "update rate param" },
        { id_model_definition, "model definition" },
        { id_heartbeat_ping, "heartbeat ping" },
        { id_heartbeat_pong, "heartbeat pong" },
        { id_play_cache, "play cache" },
        { id_trajectory_file_info, "trajectory file info" },
        { id_goto_simulation_time, "go to simulation time" },
        { id_init_trajectory_file, "init trajectory file" },
    };

    enum SimulationMode {
        id_live_simulation = 0,
        id_pre_run_simulation = 1,
        id_traj_file_playback = 2
    };

} // namespace simularium
} // namespace aics

#endif // AICS_NET_MESSAGE_IDS

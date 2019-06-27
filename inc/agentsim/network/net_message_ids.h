#ifndef AICS_NET_MESSAGE_IDS
#define AICS_NET_MESSAGE_IDS

#include <vector>
#include <string>

enum {
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
    id_play_cache
};

std::vector<std::string> webRequestNames {
    "undefined",
    "stream data",
    "stream request",
    "stream finish",
    "pause",
    "resume",
    "abort",
    "update time step",
    "update rate param",
    "model definition",
    "heartbeat ping",
    "heartbeat pong",
    "play cache"
};


enum {
    id_live_simulation = 0,
    id_pre_run_simulation = 1,
    id_traj_file_playback = 2
};

#endif // AICS_NET_MESSAGE_IDS

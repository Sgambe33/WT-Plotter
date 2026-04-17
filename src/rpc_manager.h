#ifndef RPC_MANAGER_H
#define RPC_MANAGER_H

#include <string>

#include "app_state.h"
#include "telemetry_thread.h"

void LoadRpcImage(const std::string& imageKey);

void UpdateRPC(const TelemetryUpdate *telemetry_update);

#endif // RPC_MANAGER_H

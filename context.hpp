/**
 * @file context.hpp The Batsim context
 */

#include "network.hpp"
#include "machines.hpp"
#include "jobs.hpp"
#include "profiles.hpp"
#include "export.hpp"

struct BatsimContext
{
    UnixDomainSocket socket;
    Machines machines;
    Jobs jobs;
    Profiles profiles;

    long long microseconds_used_by_scheduler;
};

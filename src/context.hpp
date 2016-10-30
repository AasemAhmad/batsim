/**
 * @file context.hpp
 * @brief The Batsim context
 */

#pragma once

#include <vector>
#include <chrono>

#include "network.hpp"
#include "machines.hpp"
#include "jobs.hpp"
#include "profiles.hpp"
#include "export.hpp"
#include "pstate.hpp"
#include "storage.hpp"
#include "workload.hpp"
#include "workflow.hpp"

/**
 * @brief Stores a high-resolution timestamp
 */
typedef std::chrono::time_point<std::chrono::high_resolution_clock> my_timestamp;

/**
 * @brief The Batsim context
 */
struct BatsimContext
{
    UnixDomainSocket socket;                        //!< The UnixDomainSocket
    Machines machines;                              //!< The machines
    Workloads workloads;                            //!< The workloads
    Workflows workflows;                            //!< The workflows
    PajeTracer paje_tracer;                         //!< The PajeTracer
    PStateChangeTracer pstate_tracer;               //!< The PStateChangeTracer
    EnergyConsumptionTracer energy_tracer;          //!< The EnergyConsumptionTracer
    CurrentSwitches current_switches;               //!< The current switches
    RedisStorage storage;                           //!< The RedisStorage

    bool terminate_with_last_workflow = false;      //!< If true, allows to ignore the jobs submitted after the last workflow termination

    long double energy_first_job_submission = -1;   //!< The amount of consumed energy (J) when the first job is submitted
    long double energy_last_job_completion;         //!< The amount of consumed energy (J) when the last job is completed

    long long microseconds_used_by_scheduler = 0;   //!< The number of microseconds used by the scheduler
    my_timestamp simulation_start_time;             //!< The moment in time at which the simulation has started
    my_timestamp simulation_end_time;               //!< The moment in time at which the simulation has ended

    bool energy_used;                               //!< Stores whether the energy part of Batsim should be used
    bool smpi_used;                                 //!< Stores whether SMPI should be used
    bool allow_space_sharing;                       //!< Stores whether space sharing (using the same machines to compute different jobs) should be allowed
    bool trace_schedule;                            //!< Stores whether the resulting schedule should be outputted
    std::string platform_filename;                  //!< The name of the platform file
    std::string export_prefix;                      //!< The output export prefix
};

#pragma once

/**
 * @brief Batsim verbosity level
 */
enum class VerbosityLevel
{
    QUIET           //!< Almost nothing should be displayed
    ,NETWORK_ONLY   //!< Only network messages should be displayed
    ,INFORMATION    //!< Informations should be displayed (default)
    ,DEBUG          //!< Debug informations should be displayed too
};

/**
 * @brief Stores Batsim's arguments, a.k.a. the main function arguments
 */
struct MainArguments
{
    /**
     * @brief Stores the command-line description of a workflow
     */
    struct WorkflowDescription
    {
        std::string workflow_filename;  //!< The name of the workflow file
        double workflow_start_time;     //!< The moment in time at which the workflow should be started
    };

    std::string platform_filename;                          //!< The SimGrid platform filename
    std::list<std::string> workload_filenames;              //!< The JSON workload filename
    std::list<WorkflowDescription> workflow_descriptions;   //!< The workflow descriptions

    std::string socket_filename = "/tmp/bat_socket";        //!< The Unix Domain Socket filename

    std::string master_host_name = "master_host";           //!< The name of the SimGrid host which runs scheduler processes and not user tasks
    std::string export_prefix;                              //!< The filename prefix used to export simulation information

    std::string redis_hostname = "127.0.0.1";               //!< The Redis (data storage) server host name
    int redis_port = 6379;                                  //!< The Redis (data storage) server port
    std::string redis_prefix;                               //!< The Redis (data storage) instance prefix

    int limit_machines_count = -1;                          //!< The number of machines to use to compute jobs. -1 : no limit. >= 1 : the number of computation machines
    bool limit_machines_count_by_workload = false;          //!< If set to true, the number of computing machiens to use should be limited by the workload description

    bool energy_used = false;                               //!< True if and only if the SimGrid energy plugin should be used.
    VerbosityLevel verbosity = VerbosityLevel::INFORMATION; //!< Sets the Batsim verbosity
    bool allow_space_sharing = false;                       //!< Allows/forbids space sharing. Two jobs can run on the same machine if and only if space sharing is allowed.
    bool enable_simgrid_process_tracing = false;            //!< If set to true, this options enables the tracing of SimGrid processes
    bool enable_schedule_tracing = true;                    //!< If set to true, the schedule is exported to a Pajé trace file

    bool abort = false;                                     //!< A boolean value. If set to yet, the launching should be aborted for reason abortReason
    std::string abortReason;                                //!< Human readable reasons which explains why the launch should be aborted
};


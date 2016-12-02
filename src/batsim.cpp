/**
 * @file batsim.cpp
 * @brief Batsim's entry point
 */

#include <sys/types.h>
#include <stdio.h>
#include <argp.h>
#include <unistd.h>

#include <string>

#include <simgrid/msg.h>
#include <smpi/smpi.h>
#include <simgrid/plugins/energy.h>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>

#include <openssl/sha.h>

#include "batsim.hpp"
#include "context.hpp"
#include "export.hpp"
#include "ipp.hpp"
#include "job_submitter.hpp"
#include "jobs.hpp"
#include "jobs_execution.hpp"
#include "machines.hpp"
#include "network.hpp"
#include "profiles.hpp"
#include "server.hpp"
#include "workload.hpp"
#include "workflow.hpp"

using namespace std;

XBT_LOG_NEW_DEFAULT_CATEGORY(batsim, "batsim"); //!< Logging

string generate_sha1_string(std::string string_to_hash, int output_length)
{
    static_assert(sizeof(unsigned char) == sizeof(char), "sizeof(unsigned char) should equals to sizeof(char)");
    xbt_assert(output_length > 0);
    xbt_assert(output_length < SHA_DIGEST_LENGTH);

    unsigned char sha1_buf[SHA_DIGEST_LENGTH];
    SHA1((const unsigned char *)string_to_hash.c_str(), string_to_hash.size(), sha1_buf);

    char * output_buf = (char *) calloc(SHA_DIGEST_LENGTH * 2 + 1, sizeof(char));
    xbt_assert(output_buf != 0, "Couldn't allocate memory");

    for (int i = 0; i < SHA_DIGEST_LENGTH; ++i)
    {
        int nb_printed_char = snprintf(output_buf + 2*i, 3, "%02x", sha1_buf[i]);
        xbt_assert(nb_printed_char == 2, "Fix me :(");
    }

    XBT_DEBUG("SHA-1 of string '%s' is '%s'\n", string_to_hash.c_str(), output_buf);

    string result(output_buf, output_length);
    xbt_assert((int)result.size() == output_length);

    free(output_buf);
    return result;
}

int parse_opt (int key, char *arg, struct argp_state *state)
{
    MainArguments * main_args = (MainArguments *) state->input;

    switch (key)
    {
    // Mandatory parameters
    case 'p':
    {
        main_args->platform_filename = arg;
        if (access(main_args->platform_filename.c_str(), R_OK) == -1)
        {
            main_args->abort = true;
            main_args->abortReason += "\n  invalid PLATFORM_FILE argument: file '" + string(main_args->platform_filename) + "' cannot be read";
        }
        break;
    }
    case 'w':
    {
        if (access(((string) arg).c_str(), R_OK) == -1)
        {
            main_args->abort = true;
            main_args->abortReason += "\n  invalid WORKLOAD_FILE argument: file '" + string(arg) + "' cannot be read";
        }
        else
        {
            MainArguments::WorkloadDescription desc;
            desc.filename = absolute_filename(arg);
            desc.name = generate_sha1_string(arg);

            XBT_INFO("Workload '%s' corresponds to workload file '%s'.", desc.name.c_str(), desc.filename.c_str());
            main_args->workload_descriptions.push_back(desc);
        }
        break;
    }
    case 'W':
    {
        // format:   FILENAME[:start_time]
        vector<string> parts;
        boost::split(parts, (const std::string)std::string(arg),
                     boost::is_any_of(":"), boost::token_compress_on);

        if (access(parts.at(0).c_str(), R_OK) == -1)
        {
            main_args->abort = true;
            main_args->abortReason += "\n  invalid WORKFLOW_FILE argument: file '" + parts.at(0) + "' cannot be read";
        }
        else
        {
            string workflow_filename = parts.at(0).c_str();
            double workflow_start_time = 0.0;

            if (parts.size() == 2)
                workflow_start_time = std::stod(parts.at(1));

            MainArguments::WorkflowDescription desc;
            desc.filename = absolute_filename(workflow_filename);
            desc.name = generate_sha1_string(desc.filename);
            desc.workload_name = desc.name;
            desc.start_time = workflow_start_time;

            XBT_INFO("Workflow '%s' corresponds to workflow file '%s'.", desc.name.c_str(), desc.filename.c_str());
            main_args->workflow_descriptions.push_back(desc);
        }
        break;
    }
    // Optional parameters:
    case 'c':
        main_args->program_type = ProgramType::BATEXEC;
    case 'e':
        main_args->export_prefix = arg;
        break;
    case 'E':
        main_args->energy_used = true;
        break;
    case 'h':
        main_args->allow_space_sharing = true;
        break;
    case 'H':
        main_args->redis_hostname = arg;
        break;
    case 'J':
        main_args->workflow_limit = std::stod(arg);
        break;
    case 'k':
    {
        main_args->terminate_with_last_workflow = true;
        break;
    }
    case 'l':
    {
        int ivalue = stoi(arg);

        if ((ivalue < -1) || (ivalue == 0))
        {
            main_args->abort = true;
            main_args->abortReason += "\n  invalid M positional argument (" + to_string(ivalue) + "): it should either be -1 or a strictly positive value";
        }

        main_args->limit_machines_count = ivalue;
        break;
    }
    case 'L':
    {
        main_args->limit_machines_count_by_workload = true;
        break;
    }
    case 'm':
        main_args->master_host_name = arg;
        break;
    case 'f':
        main_args->pfs_host_name = arg;
        break;
    case 'P':
    {
        int ivalue = stoi(arg);

        if ((ivalue <= 0) || (ivalue > 65535))
        {
            main_args->abort = true;
            main_args->abortReason += "\n  invalid PORT positional argument (" + to_string(ivalue) +
                                     "): it should be a valid port: integer in range [1,65535].";
        }

        main_args->redis_port = ivalue;
        break;
    }
    case 'q':
        main_args->verbosity = VerbosityLevel::QUIET;
        break;
    case 's':
        main_args->socket_filename = arg;
        break;
    case 't':
        main_args->enable_simgrid_process_tracing = true;
        break;
    case 'T':
        main_args->enable_schedule_tracing = false;
        break;
    case 'U':
        main_args->enable_machine_state_tracing = false;
        break;
    case 'v':
    {
        string sArg = arg;
        boost::to_lower(sArg);
        if (sArg == "quiet")
            main_args->verbosity = VerbosityLevel::QUIET;
        else if (sArg == "network-only")
            main_args->verbosity = VerbosityLevel::NETWORK_ONLY;
        else if (sArg == "information")
            main_args->verbosity = VerbosityLevel::INFORMATION;
        else if (sArg == "debug")
            main_args->verbosity = VerbosityLevel::DEBUG;
        else
        {
            main_args->abort = true;
            main_args->abortReason += "\n  invalid VERBOSITY_LEVEL argument: '" + string(sArg) + "' is not in {quiet, network-only, information, debug}.";
        }
        break;
    }
    }

    return 0;
}

bool parse_main_args(int argc, char * argv[], MainArguments & main_args)
{
    struct argp_option options[] =
    {
        // Required parameters (p is always required, |w| + |W] must be >= 1)
        {"platform", 'p', "FILENAME", 0, "Platform to be simulated by SimGrid. Required.", 0},
        {"workload", 'w', "FILENAME", 0, "Workload to be submitted to the platform. Required unless a workflow is submitted.", 0},
        {"workflow", 'W', "FILENAME[:start_time]", 0, "Workflow to be submitted to the platform, with an optional start time. Required unless a workload is submitted.", 0},

        // Optional parameters
        {"batexec", 'c', 0, 0, "If set, a simpler execution of the workloads is made (this was formerly known as the batexec program)", 0},
        {"export", 'e', "FILENAME_PREFIX", 0, "The export filename prefix used to generate simulation output. Default value: 'out'", 0},
        {"energy-plugin", 'E', 0, 0, "Enables energy-aware experiments", 0},
        {"allow-space-sharing", 'h', 0, 0, "Allows space sharing: the same resource can compute several jobs at the same time", 0},
        {"redis-hostname", 'H', "HOSTNAME", 0, "Sets the host name of the remote Redis (data storage) server.", 0},
        {"jobs_limit", 'J', "NUMBER", 0, "Number of possible concurrent jobs for workflows", 0},
        {"ignore-workflow-beyond-last-workflow", 'k', 0, 0, "Ignore workload jobs that occur after all workflows have completed, if any, as completed", 0},
        {"limit-machine-count", 'l', "M", 0, "Allows to limit the number of computing machines to use. If M == -1 (default), all the machines described in PLATFORM_FILE are used (but the master_host). If M >= 1, only the first M machines will be used to comupte jobs.", 0},
        {"limit-machine-count-by-worload", 'L', 0, 0, "If set, allows to limit the number of computing machines to use. This number is read from the workload file. If both limit-machine-count and limit-machine-count-by-worload are set, the minimum of the two will be used.", 0},
        {"master-host", 'm', "NAME", 0, "The name of the host in PLATFORM_FILE which will run SimGrid scheduling processes and won't be used to compute tasks. Default value: 'master_host'", 0},
        {"pfs-host", 'f', "NAME", 0, "The name of the host in PLATFORM_FILE which will be the parallel filesystem target as data sink/source. Default value: 'pfs_host'", 0},
        {"redis-port", 'P', "PORT", 0, "Sets the port of the remote Redis (data storage) server.", 0},
        {"quiet", 'q', 0, 0, "Shortcut for --verbosity=quiet", 0},
        {"socket", 's', "FILENAME", 0, "Unix Domain Socket filename. Default value: '/tmp/bat_socket'", 0},
        {"process-tracing", 't', 0, 0, "Enables SimGrid process tracing (shortcut for SimGrid options ----cfg=tracing:1 --cfg=tracing/msg/process:1)", 0},
        {"disable-schedule-tracing", 'T', 0, 0, "If set, the tracing of the schedule is disabled (the output file _schedule.trace will not be exported)", 0},
        {"disable-machine-state-tracing", 'U', 0, 0, "If set, the tracing of the machine states is disabled (the output file _machine_states.csv will not be exported)", 0},
        {"verbosity", 'v', "VERBOSITY_LEVEL", 0, "Sets the Batsim verbosity level. Available values are : quiet, network-only, information (default), debug.", 0},
        {0, '\0', 0, 0, 0, 0} // The options array must be NULL-terminated
    };

    struct argp argp = {options, parse_opt, 0, "A tool to simulate (via SimGrid) the behaviour of scheduling algorithms.", 0, 0, 0};
    argp_parse(&argp, argc, argv, 0, 0, &main_args);

    if (access(main_args.platform_filename.c_str(), R_OK) == -1)
    {
        main_args.abort = true;
        main_args.abortReason += "\n  invalid PLATFORM_FILE argument: file '" + string(main_args.platform_filename) + "' cannot be read";
    }

    if (main_args.abort)
    {
        fprintf(stderr, "Impossible to run batsim:%s\n", main_args.abortReason.c_str());
        return false;
    }

    return true;
}

void configure_batsim_logging_output(const MainArguments & main_args)
{
    vector<string> log_categories_to_set = {"workload", "job_submitter", "redis", "jobs", "machines", "pstate",
                                            "workflow", "jobs_execution", "server", "export", "profiles", "machine_range",
                                            "network", "ipp"};
    string log_threshold_to_set = "critical";

    if (main_args.verbosity == VerbosityLevel::QUIET || main_args.verbosity == VerbosityLevel::NETWORK_ONLY)
        log_threshold_to_set = "error";
    else if (main_args.verbosity == VerbosityLevel::DEBUG)
        log_threshold_to_set = "debug";
    else if (main_args.verbosity == VerbosityLevel::INFORMATION)
        log_threshold_to_set = "info";
    else
        xbt_assert(false, "FIXME!");

    for (const auto & log_cat : log_categories_to_set)
    {
        const string final_str = log_cat + ".thresh:" + log_threshold_to_set;
        xbt_log_control_set(final_str.c_str());
    }

    // In network-only, we add a rule to display the network info
    if (main_args.verbosity == VerbosityLevel::NETWORK_ONLY)
        xbt_log_control_set("network.thresh:info");

    // Batsim is always set to info, to allow to trace Batsim's input easily
    xbt_log_control_set("batsim.thresh:info");

    // Simgrid-related log control
    xbt_log_control_set("surf_energy.thresh:critical");
}

void initialize_msg(const MainArguments & main_args, int argc, char * argv[])
{
    // Must be initialized before MSG_init
    if (main_args.energy_used)
        sg_energy_plugin_init();

    MSG_init(&argc, argv);

    // Setting SimGrid configuration if the SimGrid process tracing is enabled
    if (main_args.enable_simgrid_process_tracing)
    {
        string sg_trace_filename = main_args.export_prefix + "_sg_processes.trace";

        MSG_config("tracing", "1");
        MSG_config("tracing/msg/process", "1");
        MSG_config("tracing/filename", sg_trace_filename.c_str());
    }
}

void load_workloads_and_workflows(const MainArguments & main_args, BatsimContext * context, int & max_nb_machines_to_use)
{
    int max_nb_machines_in_workloads = -1;

    // Let's create the workloads
    for (const MainArguments::WorkloadDescription & desc : main_args.workload_descriptions)
    {
        Workload * workload = new Workload(desc.name);

        int nb_machines_in_workload = -1;
        workload->load_from_json(desc.filename, nb_machines_in_workload);
        max_nb_machines_in_workloads = max(max_nb_machines_in_workloads, nb_machines_in_workload);

        context->workloads.insert_workload(desc.name, workload);
    }

    // Let's create the workflows
    for (const MainArguments::WorkflowDescription & desc : main_args.workflow_descriptions)
    {
        Workload * workload = new Workload(desc.workload_name); // Is creating the Workload now necessary? Workloads::add_job_if_not_exists may be enough
        workload->jobs = new Jobs;
        workload->profiles = new Profiles;
        context->workloads.insert_workload(desc.workload_name, workload);

        Workflow * workflow = new Workflow(desc.name);
        workflow->start_time = desc.start_time;
        workflow->load_from_xml(desc.filename);
        context->workflows.insert_workflow(desc.name, workflow);
    }

    // Let's compute how the number of machines to use should be limited
    max_nb_machines_to_use = -1;
    if ((main_args.limit_machines_count_by_workload) && (main_args.limit_machines_count > 0))
        max_nb_machines_to_use = min(main_args.limit_machines_count, max_nb_machines_in_workloads);
    else if (main_args.limit_machines_count_by_workload)
        max_nb_machines_to_use = max_nb_machines_in_workloads;
    else if (main_args.limit_machines_count > 0)
        max_nb_machines_to_use = main_args.limit_machines_count;

    if (max_nb_machines_to_use != -1)
        XBT_INFO("The maximum number of machines to use is %d.", max_nb_machines_to_use);
}

void start_initial_simulation_processes(const MainArguments & main_args,
                                        BatsimContext * context,
                                        bool is_batexec)
{
    const Machine * master_machine = context->machines.master_machine();

    // Let's run a static_job_submitter process for each workload
    for (const MainArguments::WorkloadDescription & desc : main_args.workload_descriptions)
    {
        XBT_DEBUG("Creating a workload_submitter process...");
        JobSubmitterProcessArguments * submitter_args = new JobSubmitterProcessArguments;
        submitter_args->context = context;
        submitter_args->workload_name = desc.name;

        string submitter_instance_name = "workload_submitter_" + desc.name;
        if (!is_batexec)
            MSG_process_create(submitter_instance_name.c_str(), static_job_submitter_process,
                               (void*) submitter_args, master_machine->host);
        else
            MSG_process_create(submitter_instance_name.c_str(), job_launcher_process,
                               (void*) submitter_args, master_machine->host);
        XBT_INFO("The process '%s' has been created.", submitter_instance_name.c_str());
    }

    // Let's run a workflow_submitter process for each workflow
    for (const MainArguments::WorkflowDescription & desc : main_args.workflow_descriptions)
    {
        XBT_DEBUG("Creating a workflow_submitter process...");

        WorkflowSubmitterProcessArguments * submitter_args = new WorkflowSubmitterProcessArguments;
        submitter_args->context = context;
        submitter_args->workflow_name = desc.name;

        string submitter_instance_name = "workflow_submitter_" + desc.name;
        MSG_process_create(submitter_instance_name.c_str(), workflow_submitter_process, (void*)submitter_args, master_machine->host);
        XBT_INFO("The process '%s' has been created.", submitter_instance_name.c_str());
    }


    if (!is_batexec)
    {
        XBT_DEBUG("Creating the 'server' process...");
        ServerProcessArguments * server_args = new ServerProcessArguments;
        server_args->context = context;
        MSG_process_create("server", uds_server_process, (void*)server_args, master_machine->host);
        XBT_INFO("The process 'server' has been created.");
    }
}

/**
 * @brief Main function
 * @param[in] argc The number of arguments
 * @param[in] argv The arguments' values
 * @return 0 on success, something else otherwise
 */
int main(int argc, char * argv[])
{
    // Let's parse command-line arguments
    MainArguments main_args;
    if (!parse_main_args(argc, argv, main_args))
        return 1;

    // Let's configure how Batsim should be logged
    configure_batsim_logging_output(main_args);

    // Let's initialize SimGrid
    initialize_msg(main_args, argc, argv);

    // Let's create the BatsimContext, which stores information about the current instance
    BatsimContext context;
    context.platform_filename = main_args.platform_filename;
    context.export_prefix = main_args.export_prefix;
    context.workflow_limit = main_args.workflow_limit;
    context.energy_used = main_args.energy_used;
    context.allow_space_sharing = main_args.allow_space_sharing;
    context.trace_schedule = main_args.enable_schedule_tracing;
    context.trace_machine_states = main_args.enable_machine_state_tracing;
    context.simulation_start_time = chrono::high_resolution_clock::now();

    context.terminate_with_last_workflow = main_args.terminate_with_last_workflow;

    // Let's load the workloads and workflows
    int max_nb_machines_to_use = -1;
    load_workloads_and_workflows(main_args, &context, max_nb_machines_to_use);

    // Let's choose which SimGrid computing model should be used
    XBT_INFO("Checking whether SMPI is used or not...");
    context.smpi_used = context.workloads.contains_smpi_job(); // todo: SMPI workflows
    if (!context.smpi_used)
    {
        XBT_INFO("SMPI will NOT be used.");
        MSG_config("host/model", "ptask_L07");
    }
    else
    {
        XBT_INFO("SMPI will be used.");
        context.workloads.register_smpi_applications(); // todo: SMPI workflows
        SMPI_init();
    }

    // Let's create the machines
    create_machines(main_args, &context, max_nb_machines_to_use);

    // Let's prepare Batsim's outputs
    XBT_INFO("Batsim's export prefix is '%s'.", context.export_prefix.c_str());
    prepare_batsim_outputs(&context);

    if (main_args.program_type == ProgramType::BATSIM)
    {
        // Let's prepare Redis's connection
        context.storage.set_instance_key_prefix(absolute_filename(main_args.socket_filename)); // TODO: main argument?
        context.storage.connect_to_server(main_args.redis_hostname, main_args.redis_port);

        // Let's create the socket
        // TODO: check that the socket is not currently being used
        context.socket.create_socket(main_args.socket_filename);
        context.socket.accept_pending_connection();

        // Let's store some metadata about the current instance in the data storage
        context.storage.set("nb_res", std::to_string(context.machines.nb_machines()));

        // Let's execute the initial processes
        start_initial_simulation_processes(main_args, &context);
    }
    else if (main_args.program_type == ProgramType::BATEXEC)
    {
        // Let's execute the initial processes
        start_initial_simulation_processes(main_args, &context, true);
    }

    // Simulation main loop, handled by MSG
    msg_error_t res = MSG_main();

    // If SMPI had been used, it should be finalized
    if (context.smpi_used)
        SMPI_finalize();

    // Let's finalize Batsim's outputs
    finalize_batsim_outputs(&context);

    if (res == MSG_OK)
        return 0;
    else
        return 1;
}

/**
 * @file task_execution.cpp
 * @brief Contains functions related to the execution of the MSG profile tasks
 */

#include <simgrid/msg.h>
#include "jobs.hpp"
#include "profiles.hpp"
#include "ipp.hpp"
#include "context.hpp"
#include "jobs_execution.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(task_execution, "task_execution"); //!< Logging

using namespace std;

/**
 * @brief Generate the communication and computaion matrix for the msg
 * parallel task profile. It also set the prefix name of the task.
 * @param[out] computation_amount the computation matrix to be simulated by the msg task
 * @param[out] communication_amount the communication matrix to be simulated by the msg task
 * @param[in] nb_res the number of resources the task have to run on
 * @param[in] profile_data the profile data
 */
void generate_msg_parallel_task(double *& computation_amount,
                                double *& communication_amount,
                                unsigned int nb_res,
                                void * profile_data)
{
    MsgParallelProfileData* data = (MsgParallelProfileData*)profile_data;
    // These amounts are deallocated by SG
    computation_amount = xbt_new(double, nb_res);
    communication_amount = xbt_new(double, nb_res* nb_res);

    // Retrieve the matrices from the profile
    memcpy(computation_amount, data->cpu, sizeof(double) * nb_res);
    memcpy(communication_amount, data->com, sizeof(double) * nb_res * nb_res);
}

/**
 * @brief Generates the communication and computaion matrix for the msg
 *        parallel homogeneous task profile. It also set the prefix name of the task.
 * @param[out] computation_amount the computation matrix to be simulated by the msg task
 * @param[out] communication_amount the communication matrix to be simulated by the msg task
 * @param[in] nb_res the number of resources the task have to run on
 * @param[in] profile_data the profile data
 */
void generate_msg_parallel_homogeneous(double *& computation_amount,
                                       double *& communication_amount,
                                       unsigned int nb_res,
                                       void * profile_data)
{
    MsgParallelHomogeneousProfileData* data = (MsgParallelHomogeneousProfileData*)profile_data;

    double cpu = data->cpu;
    double com = data->com;

    // These amounts are deallocated by SG
    computation_amount = xbt_new(double, nb_res);
    communication_amount = nullptr;
    if (com > 0)
    {
        communication_amount = xbt_new(double, nb_res* nb_res);
    }

    // Let us fill the local computation and communication matrices
    int k = 0;
    for (unsigned int y = 0; y < nb_res; ++y)
    {
        computation_amount[y] = cpu;
        if (communication_amount != nullptr)
        {
            for (unsigned int x = 0; x < nb_res; ++x)
            {
                if (x == y)
                {
                    communication_amount[k] = 0;
                }
                else
                {
                    communication_amount[k] = com;
                }
                k++;
            }
        }
    }
}

/**
 * @brief Generates the communication vector and computation matrix for the msg
 *        parallel homogeneous total amount task profile.
 *
 * @param[out] computation_amount the computation matrix to be simulated by the msg task
 * @param[out] communication_amount the communication matrix to be simulated by the msg task
 * @param[in] nb_res the number of resources the task have to run on
 * @param[in] profile_data the profile data
 *
 * @details It is like homogeneous profile but instead of giving what has
 *          to be done per host, the user gives the total amounts that should be spread
 *          homogeneously across the hosts.
 */
void generate_msg_parallel_homogeneous_total_amount(double *& computation_amount,
                                                    double *& communication_amount,
                                                    unsigned int nb_res,
                                                    void * profile_data)
{
    MsgParallelHomogeneousTotalAmountProfileData* data = (MsgParallelHomogeneousTotalAmountProfileData*)profile_data;

    const double spread_cpu = data->cpu / nb_res;
    const double spread_com = data->com / nb_res;

    // These amounts are deallocated by SG
    computation_amount = xbt_new(double, nb_res);
    communication_amount = nullptr;
    if (spread_com > 0)
    {
        communication_amount = xbt_new(double, nb_res * nb_res);
    }

    // Fill the local computation and communication matrices
    int k = 0;
    for (unsigned int y = 0; y < nb_res; ++y)
    {
        computation_amount[y] = spread_cpu;
        if (communication_amount != nullptr)
        {
            for (unsigned int x = 0; x < nb_res; ++x)
            {
                if (x == y)
                {
                    communication_amount[k] = 0;
                }
                else
                {
                    communication_amount[k] = spread_com;
                }
                k++;
            }
        }
    }
}

/**
 * @brief Generate the communication and computaion matrix for the msg
 *        parallel homogeneous task profile with pfs.
 *
 * @param[out] computation_amount the computation matrix to be simulated by the msg task
 * @param[out] communication_amount the communication matrix to be simulated by the msg task
 * @param[in,out] hosts_to_use the list of host to be used by the task
 * @param[in] storage_mapping mapping from label given in the profile and machine id
 * @param[in] profile_data the profile data
 * @param[in] context the batsim context
 *
 * @details Note that the number of resource is also altered because of the
 *          pfs node that is addded. It also set the prefix name of the
 *          task.
 */
void generate_msg_parallel_homogeneous_with_pfs(double *& computation_amount,
                                                double *& communication_amount,
                                                std::vector<msg_host_t> & hosts_to_use,
                                                std::map<std::string, int> storage_mapping,
                                                void * profile_data,
                                                BatsimContext * context)
{
    MsgParallelHomogeneousPFSMultipleTiersProfileData* data =
            (MsgParallelHomogeneousPFSMultipleTiersProfileData*)profile_data;

    double cpu = 0;
    double size = data->size;

    // The PFS machine will also be used
    unsigned int nb_res = host_to_use.size() + 1;
    unsigned int pfs_id = nb_res - 1;

    // Add the pfs_machine
    int pfs_machine_id;
    if (storage_mapping.empty())
    {
        if (context->machines.storage_machines().size() == 1)
        {
            // No label given: Use the only storage available
            pfs_machine_id = context->machines.storage_machines().at(0)->id;
        }
        else
        {
            xbt_assert(false, "No storage/host mapping given and there is no (ore more than one) storage node available");
        }
    }
    else
    {
        pfs_machine_id = storage_mapping[data->storage_label];
    }
    hosts_to_use.push_back(context->machines[pfs_machine_id]->host);

    // These amounts are deallocated by SG
    computation_amount = xbt_new(double, nb_res);
    communication_amount = nullptr;
    if (size > 0)
    {
        communication_amount = xbt_new(double, nb_res* nb_res);
    }

    // Let us fill the local computation and communication matrices
    int k = 0;
    for (unsigned int y = 0; y < nb_res; ++y)
    {
        computation_amount[y] = cpu;
        if (communication_amount != nullptr)
        {
            for (unsigned int x = 0; x < nb_res; ++x)
            {
                switch (data->direction)
                {
                case MsgParallelHomogeneousPFSMultipleTiersProfileData::Direction::FROM_NODES_TO_STORAGE:
                {
                    // Communications are done towards the PFS host,
                    // which is the last resource (to the storage)
                    if (x != pfs_id)
                    {
                        communication_amount[k] = 0;
                    }
                    else
                    {
                        communication_amount[k] = size;
                    }
                    k++;
                } break;
                case MsgParallelHomogeneousPFSMultipleTiersProfileData::Direction::FROM_STORAGE_TO_NODES:
                {
                    // Communications are done towards the job
                    // allocation (from the storage)
                    if (x != pfs_id)
                    {
                        communication_amount[k] = size;
                    }
                    else
                    {
                        communication_amount[k] = 0;
                    }
                    k++;
                } break;
                default:
                    xbt_die("Should not be reached");
                }
            }
        }
    }
}

/**
 * @brief Generate the communication and computaion matrix for the msg
 * data staging task profile.
 * @details Note that the number of resource is also altered because only
 * the pfs and the hpst are involved in the transfer. It also set the prefix
 * name of the task.
 * @param[out] computation_amount the computation matrix to be simulated by the msg task
 * @param[out] communication_amount the communication matrix to be simulated by the msg task
 * @param[in,out] hosts_to_use the list of host to be used by the task
 * @param[in] storage_mapping mapping from label given in the profile and machine id
 * @param[in] profile_data the profile data
 * @param[in] context the batsim context
 */
void generate_msg_data_staginig_task(double *&  computation_amount,
                                     double *& communication_amount,
                                     std::vector<msg_host_t> & hosts_to_use,
                                     std::map<std::string, int> storage_mapping,
                                     void * profile_data,
                                     BatsimContext * context)
{
    MsgDataStagingProfileData* data = (MsgDataStagingProfileData*)profile_data;

    double cpu = 0;
    double size = data->size;

    // The PFS machine will also be used
    unsigned int nb_res = 2;
    unsigned int pfs_id = nb_res - 1;

    // reset the alloc to use only IO nodes
    hosts_to_use = std::vector<msg_host_t>();

    // Add the pfs_machine
    int from_machine_id = storage_mapping[data->from_storage_label];
    int to_machine_id = storage_mapping[data->to_storage_label];
    hosts_to_use.push_back(context->machines[from_machine_id]->host);
    hosts_to_use.push_back(context->machines[to_machine_id]->host);

    // These amounts are deallocated by SG
    computation_amount = xbt_new(double, nb_res);
    communication_amount = nullptr;
    if (size > 0)
    {
        communication_amount = xbt_new(double, nb_res* nb_res);
    }

    // Let us fill the local computation and communication matrices
    int k = 0;
    for (unsigned int y = 0; y < nb_res; ++y)
    {
        computation_amount[y] = cpu;
        if (communication_amount != nullptr)
        {
            for (unsigned int x = 0; x < nb_res; ++x)
            {
                // Communications are done towards the last resource
                if (x != pfs_id)
                {
                    communication_amount[k] = 0;
                }
                else
                {
                    communication_amount[k] = size;
                }
                k++;
            }
        }
    }
}

/**
 * @brief Generate communication and computation matrix depending on the profile
 *
 * @param[out] computation_amount the computation matrix to be simulated by the msg task
 * @param[out] communication_amount the communication matrix to be simulated by the msg task
 * @param[in,out] nb_res the number of resources the task have to run on
 * @param[in,out] hosts_to_use the list of host to be used by the task
 * @param[in] the job allocation
 * @param[in] profile_data the profile data
 * @param[in] context the batsim context
 */
void generate_matices_from_profile(double *& computation_matrix,
                                   double *& communication_matrix,
                                   std::vector<msg_host_t> hosts_to_use,
                                   BatsimContext * context)
{

    unsigned int nb_res = host_to_use.size();

    switch(profile->type)
    {
    case ProfileType::MSG_PARALLEL:
        generate_msg_parallel_task(computation_matrix,
                                   communication_matrix,
                                   nb_res,
                                   profile->data);
        break;
    case ProfileType::MSG_PARALLEL_HOMOGENEOUS:
        generate_msg_parallel_homogeneous(computation_matrix,
                                          communication_matrix,
                                          nb_res,
                                          profile->data);
        break;
    case ProfileType::MSG_PARALLEL_HOMOGENEOUS_TOTAL_AMOUNT:
        generate_msg_parallel_homogeneous_total_amount(computation_matrix,
                                                       communication_matrix,
                                                       nb_res,
                                                       profile->data);
        break;
    case ProfileType::MSG_PARALLEL_HOMOGENEOUS_PFS_MULTIPLE_TIERS:
        generate_msg_parallel_homogeneous_with_pfs(computation_matrix,
                                                   communication_matrix,
                                                   hosts_to_use,
                                                   allocation->storage_mapping,
                                                   profile->data,
                                                   context);
        break;
    case ProfileType::MSG_DATA_STAGING:
        generate_msg_data_staginig_task(computation_matrix,
                                        communication_matrix,
                                        hosts_to_use,
                                        allocation->storage_mapping,
                                        profile->data,
                                        context);
        break;
    default:
        xbt_die("Should not be reached.");
    }
}

int execute_msg_task(BatTask * btask,
                     const SchedulingAllocation* allocation,
                     double * remaining_time,
                     BatsimContext * context,
                     CleanExecuteTaskData * cleanup_data)
{
    Profile * profile = btask->profile;
    std::vector<msg_host_t> hosts_to_use = allocation->hosts;

    double* computation_matrix = nullptr;
    double* communication_matrix = nullptr;

    generate_matices_from_profile(computation_matrix,
                                  communication_matrix,
                                  allocation,
                                  profile,
                                  context)

    // Manage additional io job
    if (job->additional_io_profile != nullptr)
    {
        Profile * io_profile = btask->io_profile;
        double* io_computation_matrix = nullptr;
        double* io_communication_matrix = nullptr;

        generate_matices_from_profile(io_computation_matrix,
                                      io_communication_matrix,
                                      allocation->io_hosts_to_use,
                                      profile,
                                      context)
        // merge the two profiles
        // First get part of the allocation that do change or not in the job
        immut_job_alloc = allocation->machine_ids - allocation->io_allocation;
        immut_io_alloc = allocation->io_allocation - allocation->machine_ids;
        to_merge_alloc = allocation->machine_ids & allocation->io_allocation;
        MachineRange new_alloc;
        new_alloc += immut_job_alloc;
        new_alloc += immut_io_alloc;
        new_alloc += to_merge_alloc;

        // FIXME this does not work for profiles that changes the number of hosts: where the allocation and the host to use
        // are different

        //Generate the new list of hosts
        vector<msg_host_t> new_host_to_use;
        for (int & machine_id : new_alloc)
        {
            new_hosts_to_use.push_back(context->machines[machine_id]->host);
        }

        // Generate the new matrices
        unsigned int nb_res = new_host_to_use.size();

        // These amounts are deallocated by SG
        double * new_computation_amount = xbt_new(double, nb_res);
        double * new_communication_amount = xbt_new(double, nb_res* nb_res);

        // Fill the computation and communication matrices
        int k = 0;
        for (unsigned int y = 0; y < nb_res; ++y)
        {
            if (immut_job_alloc.contains(new_alloc[y]))
            {
                // FIXME Need a mapping between matrix index and in job and machine id
                new_computation_amount[y] = communication_amount[y];
            }
            for (unsigned int x = 0; x < nb_res; ++x)
            {
                // Communications are done towards the last resource
                if (x != pfs_id)
                {
                    new_communication_amount[k] = 0;
                }
                else
                {
                    new_communication_amount[k] = size;
                }
                k++;
            }
        }
    }

    // Create the MSG task
    string task_name = profile_type_to_string(profile) + '_' + btask->parent_job->id.to_string() +
                       "_" + btask->profile->name + "_";
    XBT_DEBUG("Creating MSG task '%s' on %d resources", task_name.c_str(), nb_res);
    msg_task_t ptask = MSG_parallel_task_create(task_name.c_str(), host_to_use.size(),
                                                hosts_to_use.data(), computation_matrix,
                                                communication_matrix, NULL);

    // If the process gets killed, the following data may need to be freed
    cleanup_data->task = ptask;

    // Keep track of the task to get information on kill
    btask->ptask = ptask;

    // Execute the MSG task (blocking)
    msg_error_t err;
    if (*remaining_time < 0)
    {
        XBT_INFO("Executing task '%s' without walltime", MSG_task_get_name(ptask));
        err = MSG_parallel_task_execute(ptask);
    }
    else
    {
        double time_before_execute = MSG_get_clock();
        XBT_INFO("Executing task '%s' with walltime of %g", MSG_task_get_name(ptask), *remaining_time);
        err = MSG_parallel_task_execute_with_timeout(ptask, *remaining_time);
        *remaining_time = *remaining_time - (MSG_get_clock() - time_before_execute);
    }

    int ret;
    if (err == MSG_OK)
    {
        ret = profile->return_code;
    }
    else if (err == MSG_TIMEOUT)
    {
        ret = -1;
    }
    else
    {
        xbt_die("A task execution had been stopped by an unhandled way (err = %d)", err);
    }

    XBT_INFO("Task '%s' finished", MSG_task_get_name(ptask));
    MSG_task_destroy(ptask);

    // The task has been executed, the data does need to be freed in the cleanup function anymore
    cleanup_data->task = nullptr;

    return ret;
}



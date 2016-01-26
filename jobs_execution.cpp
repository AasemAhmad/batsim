#include "jobs_execution.hpp"
#include "jobs.hpp"

#include <simgrid/plugins/energy.h>

#include <simgrid/msg.h>
#include <smpi/smpi.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(jobs_execution, "jobs_execution");

using namespace std;

int killer_process(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    KillerProcessArguments * args = (KillerProcessArguments *) MSG_process_get_data(MSG_process_self());

    /* The sleep can either stop normally (res=MSG_OK) or be cancelled when the task execution
     * completed (res=MSG_TASK_CANCELED) */
    msg_error_t res = MSG_process_sleep(args->walltime);

    if (res == MSG_OK)
    {
        // If we had time to sleep until walltime (res=MSG_OK), the task execution is not over and must be cancelled
        XBT_INFO("Cancelling task '%s'", MSG_task_get_name(args->task));
        MSG_task_cancel(args->task);
    }

    delete args;
    return 0;
}

int smpi_replay_process(int argc, char *argv[])
{
    //just to verify given argv
    /*for(int index = 0; index < argc; index++)
        printf("The %d is %s\n",index,argv[index]);*/

    smpi_replay_run(&argc, &argv);
    return 0;
}

int execute_profile(BatsimContext *context,
                    const string & profile_name,
                    SchedulingAllocation *allocation,
                    double *remaining_time)
{
    Job * job = context->jobs[allocation->job_id];
    Profile * profile = context->profiles[profile_name];
    int nb_res = job->required_nb_res;

    if (profile->type == ProfileType::MSG_PARALLEL_HOMOGENEOUS)
    {
        MsgParallelHomogeneousProfileData * data = (MsgParallelHomogeneousProfileData *)profile->data;

        // These amounts are deallocated by SG
        double * computation_amount = xbt_new(double, nb_res);
        double * communication_amount = xbt_new(double, nb_res*nb_res);

        double cpu = data->cpu;
        double com = data->com;

        // Let us fill the local computation and communication matrices
        int k = 0;
        for (int y = 0; y < nb_res; ++y)
        {
            computation_amount[y] = cpu;
            for (int x = 0; x < nb_res; ++x)
            {
                if (x == y)
                    communication_amount[k++] = 0;
                else
                    communication_amount[k++] = com;
            }
        }

        string task_name = "phg " + to_string(job->id) + "'" + job->profile + "'";
        XBT_INFO("Creating task '%s'", task_name.c_str());

        msg_task_t ptask = MSG_parallel_task_create(task_name.c_str(),
                                         nb_res, allocation->hosts.data(),
                                         computation_amount,
                                         communication_amount, NULL);

        // Let's spawn a process which will wait until walltime and cancel the task if needed
        KillerProcessArguments * killer_args = new KillerProcessArguments;
        killer_args->task = ptask;
        killer_args->walltime = *remaining_time;

        msg_process_t kill_process = MSG_process_create("killer", killer_process, killer_args, MSG_host_self());

        double timeBeforeExecute = MSG_get_clock();
        XBT_INFO("Executing task '%s'", MSG_task_get_name(ptask));
        msg_error_t err = MSG_parallel_task_execute(ptask);
        *remaining_time = *remaining_time - (MSG_get_clock() - timeBeforeExecute);

        int ret = 1;
        if (err == MSG_OK)
            SIMIX_process_throw(kill_process, cancel_error, 0, "wake up");
        else if (err == MSG_TASK_CANCELED)
            ret = 0;
        else
            xbt_die("A task execution had been stopped by an unhandled way (err = %d)", err);

        XBT_INFO("Task '%s' finished", MSG_task_get_name(ptask));
        MSG_task_destroy(ptask);
        return ret;
    }
    else if (profile->type == ProfileType::MSG_PARALLEL)
    {
        MsgParallelProfileData * data = (MsgParallelProfileData *)profile->data;

        // These amounts are deallocated by SG
        double * computation_amount = xbt_new(double, nb_res);
        double * communication_amount = xbt_new(double, nb_res*nb_res);

        // Let us retrieve the matrices from the profile
        memcpy(computation_amount, data->cpu, sizeof(double) * nb_res);
        memcpy(communication_amount, data->com, sizeof(double) * nb_res * nb_res);

        string task_name = "p " + to_string(job->id) + "'" + job->profile + "'";
        XBT_INFO("Creating task '%s'", task_name.c_str());

        msg_task_t ptask = MSG_parallel_task_create(task_name.c_str(),
                                         nb_res, allocation->hosts.data(),
                                         computation_amount,
                                         communication_amount, NULL);

        // Let's spawn a process which will wait until walltime and cancel the task if needed
        KillerProcessArguments * killer_args = new KillerProcessArguments;
        killer_args->task = ptask;
        killer_args->walltime = *remaining_time;

        msg_process_t kill_process = MSG_process_create("killer", killer_process, killer_args, MSG_host_self());

        double timeBeforeExecute = MSG_get_clock();
        XBT_INFO("Executing task '%s'", MSG_task_get_name(ptask));
        msg_error_t err = MSG_parallel_task_execute(ptask);
        *remaining_time = *remaining_time - (MSG_get_clock() - timeBeforeExecute);

        int ret = 1;
        if (err == MSG_OK)
            SIMIX_process_throw(kill_process, cancel_error, 0, "wake up");
        else if (err == MSG_TASK_CANCELED)
            ret = 0;
        else
            xbt_die("A task execution had been stopped by an unhandled way (err = %d)", err);

        XBT_INFO("Task '%s' finished", MSG_task_get_name(ptask));
        MSG_task_destroy(ptask);
        return ret;
    }
    else if (profile->type == ProfileType::SEQUENCE)
    {
        xbt_die("Unhandled sequence profile type");
        SequenceProfileData * data = (SequenceProfileData *) profile->data;

        for (int i = 0; i < data->repeat; i++)
        {
            for (unsigned int j = 0; j < data->sequence.size(); j++)
            {
                if (execute_profile(context, data->sequence[j], allocation, remaining_time) == 0)
                    return 0;
            }
        }
        return 1;
    }
    else if (profile->type == ProfileType::DELAY)
    {
        DelayProfileData * data = (DelayProfileData *) profile->data;

        if (data->delay < *remaining_time)
        {
            XBT_INFO("Sleeping the whole task length");
            MSG_process_sleep(data->delay);
            XBT_INFO("Sleeping done");
            *remaining_time = *remaining_time - data->delay;
            return 1;
        }
        else
        {
            XBT_INFO("Sleeping until walltime");
            MSG_process_sleep(*remaining_time);
            XBT_INFO("Walltime reached");
            *remaining_time = 0;
            return 0;
        }
    }
    else if (profile->type == ProfileType::SMPI)
    {
        SmpiProfileData * data = (SmpiProfileData *) profile->data;

        for (int i = 0; i < nb_res; ++i)
        {
            char *str_instance_id = NULL;
            int ret = asprintf(&str_instance_id, "%d", job->id);
            xbt_assert(ret != -1, "asprintf failed (not enough memory?)");

            char *str_rank_id  = NULL;
            ret = asprintf(&str_rank_id, "%d", i);
            xbt_assert(ret != -1, "asprintf failed (not enough memory?)");

            char *str_pname = NULL;
            ret = asprintf(&str_pname, "%d_%d", job->id, i);
            xbt_assert(ret != -1, "asprintf failed (not enough memory?)");

            char **argv = xbt_new(char*, 5);
            argv[0] = xbt_strdup("1"); // Fonction_replay_label (can be ignored, for log only),
            argv[1] = str_instance_id; // Instance Id (application) job_id is used
            argv[2] = str_rank_id;     // Rank Id
            argv[3] = (char*) data->trace_filenames[i].c_str();
            argv[4] = xbt_strdup("0"); //

            MSG_process_create_with_arguments(str_pname, smpi_replay_process, NULL, allocation->hosts[i], 5, argv );

            free(str_pname);
        }
    }
    else
        xbt_die("Cannot execute job %d: the profile type '%s' is unknown", job->id, job->profile.c_str());

    return 0;
}

int execute_job_process(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    // Retrieving input parameters
    ExecuteJobProcessArguments * args = (ExecuteJobProcessArguments *) MSG_process_get_data(MSG_process_self());

    Job * job = args->context->jobs[args->allocation->job_id];
    job->starting_time = MSG_get_clock();
    job->allocation = args->allocation->machine_ids;
    double remaining_time = job->walltime;

    // If energy is enabled, let us compute the energy used by the machines before running the job
    if (args->context->energy_used)
    {
        job->consumed_energy = 0;

        for (auto it = job->allocation.elements_begin(); it != job->allocation.elements_end(); ++it)
        {
            int machine_id = *it;
            Machine * machine = args->context->machines[machine_id];
            job->consumed_energy += sg_host_get_consumed_energy(machine->host);
        }
    }

    // Job computation
    args->context->machines.updateMachinesOnJobRun(job->id, args->allocation->machine_ids);
    if (execute_profile(args->context, job->profile, args->allocation, &remaining_time) == 1)
    {
        XBT_INFO("Job %d finished in time", job->id);
        job->state = JobState::JOB_STATE_COMPLETED_SUCCESSFULLY;
    }
    else
    {
        XBT_INFO("Job %d had been killed (walltime %lf reached", job->id, job->walltime);
        job->state = JobState::JOB_STATE_COMPLETED_KILLED;
        if (args->context->trace_schedule)
            args->context->paje_tracer.addJobKill(job->id, args->allocation->machine_ids, MSG_get_clock(), true);
    }

    args->context->machines.updateMachinesOnJobEnd(job->id, args->allocation->machine_ids);
    job->runtime = MSG_get_clock() - job->starting_time;

    // If energy is enabled, let us compute the energy used by the machines after running the job
    if (args->context->energy_used)
    {
        long double consumed_energy_before = job->consumed_energy;
        job->consumed_energy = 0;


        for (auto it = job->allocation.elements_begin(); it != job->allocation.elements_end(); ++it)
        {
            int machine_id = *it;
            Machine * machine = args->context->machines[machine_id];
            job->consumed_energy += sg_host_get_consumed_energy(machine->host);
        }

        // The consumed energy is the difference (consumed_energy_after_job - consumed_energy_before_job)
        job->consumed_energy = job->consumed_energy - consumed_energy_before;
    }

    // Let us tell the server that the job completed
    JobCompletedMessage * message = new JobCompletedMessage;
    message->job_id = job->id;

    send_message("server", IPMessageType::JOB_COMPLETED, (void*)message);
    delete args->allocation;
    delete args;

    return 0;
}

int waiter_process(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    WaiterProcessArguments * args = (WaiterProcessArguments *) MSG_process_get_data(MSG_process_self());

    double curr_time = MSG_get_clock();

    if (curr_time < args->target_time)
    {
        double time_to_wait = args->target_time - curr_time;
        XBT_INFO("Sleeping %g seconds to reach time %g", time_to_wait, args->target_time);
        MSG_process_sleep(time_to_wait);
        XBT_INFO("Sleeping done");
    }
    else
        XBT_INFO("Time %g is already reached, skipping sleep", args->target_time);

    send_message("server", IPMessageType::WAITING_DONE);
    delete args;

    return 0;
}

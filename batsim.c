/* Copyright (c) 2007-2014. The SimGrid Team and OAR Team
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or motask_data_tdify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "batsim.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>

#include <msg/msg.h>

#include <xbt/sysdep.h>         /* calloc, printf */
/* Create a log channel to have nice outputs. */
#include <xbt/log.h>
#include <xbt/asserts.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(batsim, "Batsim");

#include "job.h"
#include "utils.h"
#include "export.h"

#define BAT_SOCK_NAME "/tmp/bat_socket"

#define COMM_SIZE 0.000001 //#define COMM_SIZE 0.0 // => raise core dump 
#define COMP_SIZE 0.0

#define true 1
#define false 0

char *task_type2str[] =
{
    "ECHO",
    "FINALIZE",
    "LAUNCH_JOB",
    "JOB_SUMITTED",
    "JOB_COMPLETED",
    "KILL_JOB",
    "SUSPEND_JOB",
    "SCHED_READY"
};

int nb_nodes = 0;
msg_host_t *nodes;

int uds_fd = -1;

PajeTracer * tracer;

// process functions
static int node(int argc, char *argv[]);
static int server(int argc, char *argv[]);
static int launch_job(int argc, char *argv[]);
static int kill_job(int argc, char *argv[]);
static void send_message(const char *dst, e_task_type_t type, int job_id, void *data);

/**
 * \brief Send message through Unix Domain Socket
 */
static int send_uds(char *msg)
{
    int32_t lg = strlen(msg);
    XBT_INFO("send_uds, lg: %d", lg);
    write(uds_fd, &lg, 4);
    write(uds_fd, msg, lg);
    free(msg);
}

/**
 * \brief Receive message through Unix Domain Socket
 */

static char *recv_uds()
{
    int32_t lg;
    char *msg;
    read(uds_fd, &lg, 4);
    printf("msg size to recv %d\n", lg);
    msg = (char *) malloc(sizeof(char)*(lg+1)); /* +1 for null terminator */
    read(uds_fd, msg, lg);
    msg[lg] = 0;
    printf("msg: %s\n", msg);

    return msg;
}

/**
 * \brief Open Unix Domain Socket for communication with scheduler
 */
static void open_uds()
{
    struct sockaddr_un address;

    uds_fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if(uds_fd < 0)
    {
        XBT_ERROR("socket() failed\n");
        exit(1);
    }
    /* start with a clean address structure */
    memset(&address, 0, sizeof(struct sockaddr_un));

    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, 255, BAT_SOCK_NAME);

    if(connect(uds_fd,
               (struct sockaddr *) &address,
               sizeof(struct sockaddr_un)) != 0)
    {
        XBT_ERROR("connect() failed\n");
        exit(1);
    }
}

/**
 * \brief
 */
static int send_sched(int argc, char *argv[])
{
    double t_send = MSG_get_clock();
    double t_answer = 0;

    char *message_recv = NULL;
    char *message_send = MSG_process_get_data(MSG_process_self());
    char *core_message = NULL;
    xbt_dynar_t * core_message_dynar = malloc(sizeof(xbt_dynar_t));

    int size_m;
    xbt_dynar_t answer_dynar;

    //add current time to message
    size_m = asprintf(&message_send, "0:%f%s", MSG_get_clock(), message_send );
    send_uds(message_send);

    message_recv = recv_uds();
    answer_dynar = xbt_str_split(message_recv, "|");

    t_answer = atof(*(char **)xbt_dynar_get_ptr(answer_dynar, 0) + 2 );//2 left shift to skip "0:"

    //waiting before consider the sched's answer
    MSG_process_sleep(t_answer - t_send);

    //XBT_INFO("send_sched, msg type %p", (char **)xbt_dynar_get_ptr(answer_dynar, 1));
    //signal
    core_message = *(char **)xbt_dynar_get_ptr(answer_dynar, 1);
    *core_message_dynar = xbt_str_split(core_message, ":");
    send_message("server", SCHED_READY, 0, core_message_dynar);

    xbt_dynar_free(&answer_dynar);
    free(message_recv);
    return 0;
}

/**
 * \brief
 */
void send_message(const char *dst, e_task_type_t type, int job_idx, void *data)
{
    msg_task_t task_sent;
    task_data_t req_data = xbt_new0(s_task_data_t,1);
    req_data->type = type;
    req_data->job_idx = job_idx;
    req_data->data = data;
    req_data->src = MSG_host_get_name(MSG_host_self());
    task_sent = MSG_task_create(NULL, COMP_SIZE, COMM_SIZE, req_data);

    XBT_INFO("send_message data pointer %p", req_data->data);

    MSG_task_send(task_sent, dst);
}

/**
 * \brief
 */
static void task_free(struct msg_task ** task)
{
    if(*task != NULL)
    {
        //xbt_free(MSG_task_get_data(*task));
        MSG_task_destroy(*task);
        *task = NULL;
    }
}


/**
 * @brief The function in charge of job launching
 * @return 0
 */
static int launch_job(int argc, char *argv[])
{
    // Let's read a first time the input to get the barrier
    s_launch_data_t * data = MSG_process_get_data(MSG_process_self());

    // Let's wait on the barrier to be sure we can read our arguments
    MSG_barrier_wait(*(data->barrier));

    // After the second read, we know who the killerProcess is and its associated data
    data = MSG_process_get_data(MSG_process_self());
    s_kill_data_t * kdata = data->killerData;
    int jobID = data->jobID;

    XBT_INFO("job launcher %d ready\t(killp=%p, launchp=%p)", jobID, data->killerProcess, MSG_process_self());

    // Let's run the job
    pajeTracer_addJobLaunching(tracer, MSG_get_clock(), jobID, data->reservedNodeCount, data->reservedNodesIDs);
    pajeTracer_addJobRunning(tracer, MSG_get_clock(), jobID, data->reservedNodeCount, data->reservedNodesIDs);

    job_exec(jobID, data->reservedNodeCount, data->reservedNodesIDs, nodes, &(data->dataToRelease));
    
    //       .
    //      / \
    //     / | \     
    //    /  |  \    The remaining code of this function is executed ONLY IF
    //   /   |   \   the job finished in time (its execution time was smaller than its walltime)
    //  /         \  
    // /     *     \
    // ‾‾‾‾‾‾‾‾‾‾‾‾‾

    XBT_INFO("job launcher %d completed", jobID);
    int dbg = 0;

    pajeTracer_addJobEnding(tracer, MSG_get_clock(), jobID, data->reservedNodeCount, data->reservedNodesIDs);

    // Let's kill the killer, destroy its associated data then our data
    XBT_INFO("job launcher %d killing killer", jobID);
    MSG_process_kill(data->killerProcess);
    free(kdata);
    xbt_dict_free(&(data->dataToRelease));
    free(data);

    XBT_INFO("job launcher %d freeing done", jobID);

    send_message("server", JOB_COMPLETED, jobID, NULL);

    return 0;
}

/**
 * @brief The function in charge of job killing
 * @return 0
 */
static int kill_job(int argc, char *argv[])
{
    // Let's read a first time the input to get the barrier
    s_kill_data_t * data = MSG_process_get_data(MSG_process_self());

    // Let's wait on the barrier to be sure we can read our arguments
    MSG_barrier_wait(*(data->barrier));

    // After the second read, we know who the launcherProcess is and its associated data
    data = MSG_process_get_data(MSG_process_self());
    s_launch_data_t * ldata = data->launcherData;
    int jobID = ldata->jobID;

    XBT_INFO("job killer %d ready", jobID);

    // Let's sleep until the walltime is reached
    MSG_process_sleep(jobs[jobID].walltime);

    //       .
    //      / \
    //     / | \     
    //    /  |  \    The remaining code of this function is executed ONLY IF
    //   /   |   \   the killer finished its wait before the launcher completion
    //  /         \
    // /     *     \
    // ‾‾‾‾‾‾‾‾‾‾‾‾‾

    XBT_INFO("job killer %d completed", jobID);

    pajeTracer_addJobEnding(tracer, MSG_get_clock(), jobID, ldata->reservedNodeCount, ldata->reservedNodesIDs);
    pajeTracer_addJobKill(tracer, MSG_get_clock(), jobID, ldata->reservedNodeCount, ldata->reservedNodesIDs);

    // Let's kill the launcher, destroy its associated data then our data
    XBT_INFO("job killer %d killing launcher", jobID);
    MSG_process_kill(data->launcherProcess);
    xbt_dict_free(&(ldata->dataToRelease));
    free(ldata);
    free(data);

    XBT_INFO("job killer %d freeing done", jobID);

    // Let's say to the server that the job execution finished
    send_message("server", JOB_COMPLETED, jobID, NULL);
}

/**
 * \brief
 */
static int node(int argc, char *argv[])
{
    const char *node_id = MSG_host_get_name(MSG_host_self());

    msg_task_t task_received = NULL;
    task_data_t task_data;
    int type = -1;

    XBT_INFO("I am %s", node_id);

    while(1)
    {
        MSG_task_receive(&(task_received), node_id);

        task_data = (task_data_t) MSG_task_get_data(task_received);
        type = task_data->type;

        XBT_INFO("MSG_Task received %s, type %s", node_id, task_type2str[type]);

        if (type == FINALIZE) break;

        switch (type)
        {
            case LAUNCH_JOB:
            {
                // Let's retrieve the launch data and create a kill data
                s_launch_data_t * launchData = (s_launch_data_t *) task_data->data;
                s_kill_data_t * killData = (s_kill_data_t *) malloc(sizeof(s_kill_data_t));

                // Since the launcher needs to know the killer and vice versa, let's synchronize this knowledge by a barrier
                msg_bar_t barrier = MSG_barrier_init(3);
                launchData->barrier = killData->barrier = &barrier;

                // MSG process launching. These processes wait on the given barrier on their beginning
                msg_process_t launcher = MSG_process_create("launch_job", launch_job, launchData, MSG_host_self());
                msg_process_t killer = MSG_process_create("kill_job", kill_job, killData, MSG_host_self());

                // The processes can now know each other
                launchData->killerProcess = killer;
                launchData->killerData = killData;
                killData->launcherProcess = launcher;
                killData->launcherData = launchData;

                XBT_INFO("job %d master: processes and data set", task_data->job_idx);

                // Let's wake the processes then destroy the barrier
                MSG_barrier_wait(barrier);
                MSG_barrier_destroy(barrier);

                break;
            }
        }
        task_free(&task_received);
    }

}

/**
 * \brief Unroll jobs submission
 *
 * Jobs' submission is managed by a dedicate MSG_process
 */
static int jobs_submitter(int argc, char *argv[])
{

    double submission_time = 0.0;
    int job2submit_idx = 0;
    xbt_dynar_t jobs2sub_dynar;
    double t = MSG_get_clock();
    int job_idx, i;

    while (job2submit_idx < nb_jobs)
    {
        submission_time = jobs[job2submit_idx].submission_time;

        jobs2sub_dynar = xbt_dynar_new(sizeof(int), NULL);
        while(submission_time == jobs[job2submit_idx].submission_time)
        {
            xbt_dynar_push_as(jobs2sub_dynar, int, job2submit_idx);
            XBT_INFO("job2submit_idx %d, job_id %d", job2submit_idx, jobs[job2submit_idx].id);
            job2submit_idx++;
            if (job2submit_idx == nb_jobs) break;
        }

        MSG_process_sleep(submission_time - t);
        t = MSG_get_clock();

        xbt_dynar_foreach(jobs2sub_dynar, i, job_idx)
        {
            send_message("server", JOB_SUBMITTED, job_idx, NULL);
        }
        xbt_dynar_free(&jobs2sub_dynar);
    }
    return 0;
}


/**
 * \brief The central process which manage the global orchestration
 */
int server(int argc, char *argv[])
{
    msg_host_t node;

    msg_task_t task_received = NULL;
    task_data_t task_data;

    int nb_completed_jobs = 0;
    int nb_submitted_jobs = 0;
    int sched_ready = true;
    char *sched_message = "";
    int size_m;
    char *tmp;
    char *job_ready_str;
    char *jobid_ready;
    char *job_id_str;
    char *res_str;
    double t = 0;
    int i, j;

    while ((nb_completed_jobs < nb_jobs) || !sched_ready)
    {
        // wait message node, submitter, scheduler...
        MSG_task_receive(&(task_received), "server");

        task_data = (task_data_t) MSG_task_get_data(task_received);

        XBT_INFO("Server receive Task/message type %s:", task_type2str[task_data->type]);

        switch (task_data->type)
        {
        case JOB_COMPLETED:
        {
            nb_completed_jobs++;
            job_id_str= jobs[task_data->job_idx].id_str;
            XBT_INFO("Job id %s COMPLETED, %d jobs completed", job_id_str, nb_completed_jobs);
            size_m = asprintf(&sched_message, "%s|%f:C:%s", sched_message, MSG_get_clock(), job_id_str);
            XBT_INFO("sched_message: %s", sched_message);

            //TODO add job_id + msg to send
            break;
        } // end of case JOB_COMPLETED
        case JOB_SUBMITTED:
        {
            nb_submitted_jobs++;
            job_id_str = jobs[task_data->job_idx].id_str;
            XBT_INFO("Job id %s SUBMITTED, %d jobs submitted", job_id_str, nb_submitted_jobs);
            size_m = asprintf(&sched_message, "%s|%f:S:%s", sched_message, MSG_get_clock(), job_id_str);
            XBT_INFO("sched_message: %s", sched_message);

            break;
        } // end of case JOB_SUBMITTED
        case SCHED_READY:
        {
            xbt_dynar_t * input = (xbt_dynar_t *)task_data->data;

            tmp = *(char **)xbt_dynar_get_ptr(*input, 1);

            XBT_INFO("type of receive message from scheduler: %c", *tmp);
            switch (*tmp)
            {
            case 'J': // "timestampJ"
            {
                tmp = *(char **)xbt_dynar_get_ptr(*input, 2); // to skip the timestamp and the 'J'
                xbt_dynar_t jobs_ready_dynar = xbt_str_split(tmp, ";");

                xbt_dynar_foreach(jobs_ready_dynar, i, job_ready_str)
                {
                    if (job_ready_str != NULL)
                    {
                        XBT_INFO("job_ready: %s", job_ready_str);
                        xbt_dynar_t job_id_res = xbt_str_split(tmp, "=");
                        job_id_str = *(char **)xbt_dynar_get_ptr(job_id_res, 0);
                        char * job_reservs_str = *(char **)xbt_dynar_get_ptr(job_id_res, 1);

                        int *job_idx =  xbt_dict_get(jobs_idx2id, job_id_str);

                        xbt_dynar_t res_dynar = xbt_str_split(job_reservs_str, ",");

                        int head_node = atoi(*(char **)xbt_dynar_get_ptr(res_dynar, 0));
                        XBT_INFO("head node: %s, id: %d", MSG_host_get_name(nodes[head_node]), head_node);

                        // Let's create a launch data structure, which will be given to the head_node then used to launch the job
                        s_launch_data_t * launchData = (s_launch_data_t*) malloc(sizeof(s_launch_data_t));
                        launchData->jobID = *job_idx;
                        launchData->reservedNodeCount = xbt_dynar_length(res_dynar);

                        xbt_assert(launchData->reservedNodeCount == jobs[launchData->jobID].nb_res,
                                   "Invalid scheduling algorithm decision: allocation of job %d is done on %d nodes (instead of %d)",
                                   launchData->jobID, launchData->reservedNodeCount, jobs[launchData->jobID].nb_res);

                        launchData->reservedNodesIDs = (int*) malloc(launchData->reservedNodeCount * sizeof(int));
                        launchData->dataToRelease = xbt_dict_new();
                        xbt_dict_set(launchData->dataToRelease, "reservedNodesIDs", launchData->reservedNodesIDs, free);

                        // Let's fill the reserved node IDs from the XBT dynar
                        xbt_dynar_foreach(res_dynar, j, res_str)
                        {
                            launchData->reservedNodesIDs[j] = atoi(res_str);
                        }

                        send_message(MSG_host_get_name(nodes[head_node]), LAUNCH_JOB, *job_idx, launchData);

                        xbt_dynar_free(&res_dynar);
                        xbt_dynar_free(&job_id_res);
                    } // end of if
                } // end of xbt_dynar_foreach

                xbt_dynar_free(&jobs_ready_dynar);
                break;
            } // end of case J
            case 'N':
            {
                XBT_DEBUG("Nothing to do");
                break;
            } // end of case N
            default:
            {
                XBT_ERROR("Command %s is not supported", tmp);
                break;
            } // end of default
            } // end of switch (inner)

            XBT_INFO("before freeing dynar");
            xbt_dynar_free(input);
            free(input);
            XBT_INFO("after freeing dynar");

            sched_ready = true;

            break;
        } // end of case SCHED_READY
        } // end of switch (outer)

        task_free(&task_received);

        if (sched_ready && (strcmp(sched_message, "") !=0))
        {
            //add current time to sched_message
            //size_m = asprintf(&sched_message, "%s0:%f:T", sched_message, MSG_get_clock());
            MSG_process_create("send message to sched", send_sched, sched_message, MSG_host_self() );
            sched_message = "";
            sched_ready = false;
        }

    } // end of while

    // send finalize to node
    XBT_INFO("All jobs completed, time to finalize");

    for(i = 0; i < nb_nodes; i++)
        send_message(MSG_host_get_name(nodes[i]), FINALIZE, 0, NULL);
}

/**
 * \brief
 */
msg_error_t deploy_all(const char *platform_file)
{
    msg_error_t res = MSG_OK;
    xbt_dynar_t all_hosts;
    msg_host_t first_host;
    msg_host_t host;
    int i;

    MSG_config("workstation/model", "ptask_L07");
    MSG_create_environment(platform_file);

    all_hosts = MSG_hosts_as_dynar();
    xbt_dynar_get_cpy(all_hosts, 0, &first_host); // todo: maybe use a specific host (with a given name) rather than using the first host
    //first_host = xbt_dynar_getfirst_as(all_hosts,msg_host_t);
    xbt_dynar_remove_at(all_hosts, 0, NULL);

    xbt_dynar_foreach(all_hosts, i, host)
    {
        XBT_INFO("Create node process %d !", i);
        MSG_process_create("node", node, NULL, host);
    }


    nb_nodes = xbt_dynar_length(all_hosts);
    nodes = xbt_dynar_to_array(all_hosts);
    //xbt_dynar_free(&all_hosts);

    XBT_INFO("Nb nodes: %d", nb_nodes);

    MSG_process_create("server", server, NULL, first_host);
    MSG_process_create("jobs_submitter", jobs_submitter, NULL, first_host);

    tracer = pajeTracer_create("schedule.trace", 0, 32);
    pajeTracer_initialize(tracer, MSG_get_clock(), nb_nodes, nodes);

    res = MSG_main();

    pajeTracer_finalize(tracer, MSG_get_clock(), nb_nodes, nodes);
    pajeTracer_destroy(&tracer);

    XBT_INFO("Simulation time %g", MSG_get_clock());
    return res;
}

int main(int argc, char *argv[])
{
    msg_error_t res = MSG_OK;
    int i;

    json_t *json_workload_profile;

    //Comment to remove debug message
    xbt_log_control_set("batsim.threshold:debug");

    if (argc < 2)
    {
        printf("Batsim: Batch System Simulator.\n");
        printf("\n");
        printf("Usage: %s platform_file workload_file\n", argv[0]);
        printf("example: %s platforms/small_platform.xml workload_profiles/test_workload_profile.json\n", argv[0]);
        exit(1);
    }

    json_workload_profile = load_json_workload_profile(argv[2]);
    retrieve_jobs(json_workload_profile);
    retrieve_profiles(json_workload_profile);

    open_uds();

    MSG_init(&argc, argv);

    res = deploy_all(argv[1]);

    // Let's clear global allocated data
    xbt_dict_free(&jobs_idx2id);

    xbt_dict_cursor_t cursor=NULL;
    char *key;
    s_profile_t * data;

    xbt_dict_foreach(profiles,cursor,key,data)
    {
        free(data->data);
        data->data = NULL;
    }
    xbt_dict_free(&profiles);

    for (int i = 0; i < nb_jobs; ++i)
        free(jobs[i].id_str);
    //xbt_dynar_free(&jobs_dynar);

    if (res == MSG_OK)
        return 0;
    else
        return 1;
}

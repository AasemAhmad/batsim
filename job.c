/* Copyright (c) 2015. The OAR Team.
 * All rights reserved.                       */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "job.h"
#include "utils.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(job, "job");

void profile_exec(const char *profile_str, int job_id, int nb_res, msg_host_t *job_res, xbt_dict_t * allocatedStuff)
{
    double *computation_amount;
    double *communication_amount;
    msg_task_t ptask;

    profile_t profile = xbt_dict_get(profiles, profile_str);

    if (strcmp(profile->type, "msg_par") == 0)
    {
        // These amounts are deallocated by SG
        computation_amount = malloc(nb_res * sizeof(double));
        communication_amount = malloc(nb_res * nb_res * sizeof(double));

        double *cpu = ((msg_par_t)(profile->data))->cpu;
        double *com = ((msg_par_t)(profile->data))->com;

        memcpy(computation_amount , cpu, nb_res * sizeof(double));
        memcpy(communication_amount, com, nb_res * nb_res * sizeof(double));

        char * tname = NULL;
        asprintf(&tname, "p %d", job_id);
        XBT_INFO("Creating task '%s'", tname);

        //ptask = malloc(sizeof(msg_task_t *));
        ptask = MSG_parallel_task_create(tname,
                                         nb_res, job_res,
                                         computation_amount,
                                         communication_amount, NULL);
        xbt_dict_set(*allocatedStuff, tname, (void*)ptask, freeTask);
        MSG_parallel_task_execute(ptask);
    }
    else if (strcmp(profile->type, "msg_par_hg") == 0)
    {
        double cpu = ((msg_par_hg_t)(profile->data))->cpu;
        double com = ((msg_par_hg_t)(profile->data))->com;

        // These amounts are deallocated by SG
        computation_amount = malloc(nb_res * sizeof(double));
        communication_amount = malloc(nb_res * nb_res * sizeof(double));

        XBT_DEBUG("msg_par_hg: nb_res: %d , cpu: %f , com: %f", nb_res, cpu, com);

        for (int i = 0; i < nb_res; i++)
            computation_amount[i] = cpu;

        for (int j = 0; j < nb_res; j++)
            for (int i = 0; i < nb_res; i++)
                communication_amount[nb_res * j + i] = com;

        char * tname = NULL;
        asprintf(&tname, "hg %d", job_id);
        XBT_INFO("Creating task '%s'", tname);

        //ptask = malloc(sizeof(msg_task_t *));
        ptask = MSG_parallel_task_create(tname,
                                         nb_res, job_res,
                                         computation_amount,
                                         communication_amount, NULL);
        xbt_dict_set(*allocatedStuff, tname, (void*)ptask, freeTask);
        MSG_parallel_task_execute(ptask);
    }
    else if (strcmp(profile->type, "composed") == 0)
    {
        char buffer[20];

        int nb = ((composed_prof_t)(profile->data))->nb;
        int lg_seq = ((composed_prof_t)(profile->data))->lg_seq;
        int *seq = ((composed_prof_t)(profile->data))->seq;

        XBT_DEBUG("composed: nb: %d, lg_seq: %d", nb, lg_seq);

        for (int j = 0; j < lg_seq; j++)
        {
            for (int i = 0; i < nb; i++)
            {
                sprintf(buffer, "%d", seq[j]);
                profile_exec(buffer, job_id, nb_res, job_res, allocatedStuff);
            }
        }
    }
    else if (strcmp(profile->type, "delay") == 0)
    {
        double delay = ((delay_t)(profile->data))->delay;
        MSG_process_sleep(delay);

    }
    else if (strcmp(profile->type, "smpi") == 0)
    {
        xbt_die("Profile with type %s is not yet implemented", profile->type);
    }
    else
    {
        xbt_die("Profile with type %s is not supported", profile->type);
    }
}

/**
 * \brief Load workload with jobs' profiles file
 */

void job_exec(int job_idx, int nb_res, int *res_idxs, msg_host_t *nodes, xbt_dict_t * allocatedStuff)
{
    int dictCreatedHere = 0;

    if (allocatedStuff == NULL)
    {
        allocatedStuff = malloc(sizeof(xbt_dict_t));
        *allocatedStuff = xbt_dict_new();
        dictCreatedHere = 1;
    }

    s_job_t job = jobs[job_idx];
    XBT_DEBUG("Launch_job: idx %d, id %s profile %s", job_idx, jobs[job_idx].id_str, job.profile);

    msg_host_t * job_res = (msg_host_t *) malloc(nb_res * sizeof(s_msg_host_t));
    xbt_dict_set(*allocatedStuff, "hosts", job_res, free);

    for(int i = 0; i < nb_res; i++)
        job_res[i] = nodes[res_idxs[i]];

    profile_exec(job.profile, job_idx, nb_res, job_res, allocatedStuff);

    if (dictCreatedHere)
    {
        xbt_dict_free(allocatedStuff);
        free(allocatedStuff);
    }
}

void freeTask(void * task)
{
    msg_task_t t = (msg_task_t) task;
    char * tname = (char *) MSG_task_get_name(t);

    XBT_INFO("freeing task '%s' (with memory leak)", tname);

    //MSG_task_destroy(t); // why does this make the next MSG_task_create crash?
    // todo : solve this (memory leak: task won't be freed on KILL)
    
    //free(t);
    XBT_INFO("freeing task '%s' (with memory leak) done", tname);
    free(tname);
}
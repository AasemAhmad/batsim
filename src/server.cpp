/**
 * @file server.cpp
 * @brief Contains functions related to the general orchestration of the simulation
 */

#include "server.hpp"

#include <string>

#include <boost/algorithm/string.hpp>

#include <simgrid/msg.h>

#include "context.hpp"
#include "ipp.hpp"
#include "network.hpp"
#include "jobs_execution.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(server, "server"); //!< Logging

using namespace std;

int uds_server_process(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    ServerProcessArguments * args = (ServerProcessArguments *) MSG_process_get_data(MSG_process_self());
    BatsimContext * context = args->context;

    int nb_completed_jobs = 0;
    int nb_submitted_jobs = 0;
    int nb_scheduled_jobs = 0;
    int nb_submitters = 0;
    int nb_submitters_finished = 0;
    int nb_running_jobs = 0;
    int nb_switching_machines = 0;
    int nb_waiters = 0;
    const int protocol_version = 2;
    bool sched_ready = true;

    // Let's store some information about the submitters
    struct Submitter
    {
        string mailbox;
        bool should_be_called_back;
    };

    map<string, Submitter*> submitters;

    // Let's store the origin or some jobs
    map<JobIdentifier, Submitter*> origin_of_jobs;

    string send_buffer;

    while ((nb_submitters == 0) || (nb_submitters_finished < nb_submitters) ||
           (nb_completed_jobs < nb_submitted_jobs) || (!sched_ready) ||
           (nb_switching_machines > 0) || (nb_waiters > 0))
    {
        // Let's wait a message from a node or the request-reply process...
        msg_task_t task_received = NULL;
        IPMessage * task_data;
        MSG_task_receive(&(task_received), "server");
        task_data = (IPMessage *) MSG_task_get_data(task_received);

        XBT_INFO( "Server received a message of type %s:", ipMessageTypeToString(task_data->type).c_str());

        switch (task_data->type)
        {
        case IPMessageType::SUBMITTER_HELLO:
        {
            xbt_assert(task_data->data != nullptr);
            SubmitterHelloMessage * message = (SubmitterHelloMessage *) task_data->data;

            xbt_assert(submitters.count(message->submitter_name) == 0,
                       "Invalid new submitter '%s': a submitter with the same name already exists!",
                       message->submitter_name.c_str());

            nb_submitters++;

            Submitter * submitter = new Submitter;
            submitter->mailbox = message->submitter_name;
            submitter->should_be_called_back = message->enable_callback_on_job_completion;

            submitters[message->submitter_name] = submitter;

            XBT_INFO("New submitter said hello. Number of polite submitters: %d", nb_submitters);

        } break; // end of case SUBMITTER_HELLO

        case IPMessageType::SUBMITTER_BYE:
        {
            xbt_assert(task_data->data != nullptr);
            SubmitterByeMessage * message = (SubmitterByeMessage *) task_data->data;

            xbt_assert(submitters.count(message->submitter_name) == 1);
            submitters.erase(message->submitter_name);

            nb_submitters_finished++;
            XBT_INFO( "A submitted said goodbye. Number of finished submitters: %d", nb_submitters_finished);

        } break; // end of case SUBMITTER_BYE

        case IPMessageType::JOB_COMPLETED:
        {
            xbt_assert(task_data->data != nullptr);
            JobCompletedMessage * message = (JobCompletedMessage *) task_data->data;

            if (origin_of_jobs.count(message->job_id) == 1)
            {
                // Let's call the submitter which submitted the job back
                SubmitterJobCompletionCallbackMessage * msg = new SubmitterJobCompletionCallbackMessage;
                msg->job_id = message->job_id;

                Submitter * submitter = origin_of_jobs.at(message->job_id);
                send_message(submitter->mailbox, IPMessageType::SUBMITTER_CALLBACK, (void*) msg);

                origin_of_jobs.erase(message->job_id);
            }

            nb_running_jobs--;
            xbt_assert(nb_running_jobs >= 0);
            nb_completed_jobs++;
            Job * job = context->workloads.job_at(message->job_id);

            XBT_INFO( "Job %s-%d COMPLETED. %d jobs completed so far", job->workload->name.c_str(), job->number, nb_completed_jobs);

            send_buffer += '|' + std::to_string(MSG_get_clock()) + ":C:" + message->job_id.to_string();
            XBT_DEBUG( "Message to send to scheduler: %s", send_buffer.c_str());

        } break; // end of case JOB_COMPLETED

        case IPMessageType::JOB_SUBMITTED:
        {
            xbt_assert(task_data->data != nullptr);
            JobSubmittedMessage * message = (JobSubmittedMessage *) task_data->data;

            xbt_assert(submitters.count(message->submitter_name) == 1);

            Submitter * submitter = submitters.at(message->submitter_name);
            if (submitter->should_be_called_back)
            {
                xbt_assert(origin_of_jobs.count(message->job_id) == 0);
                origin_of_jobs[message->job_id] = submitter;
            }

            // Let's retrieve the Job from memory (or add it into memory if it is dynamic)
            Job * job = context->workloads.add_job_if_not_exists(message->job_id, context);

            // Update control information
            job->state = JobState::JOB_STATE_SUBMITTED;
            nb_submitted_jobs++;
            XBT_INFO("Job %s SUBMITTED. %d jobs submitted so far",
                     message->job_id.to_string().c_str(), nb_submitted_jobs);

            send_buffer += "|" + std::to_string(MSG_get_clock()) + ":S:" + message->job_id.to_string();
            XBT_DEBUG( "Message to send to scheduler: '%s'", send_buffer.c_str());

        } break; // end of case JOB_SUBMITTED

        case IPMessageType::SCHED_REJECTION:
        {
            xbt_assert(task_data->data != nullptr);
            JobRejectedMessage * message = (JobRejectedMessage *) task_data->data;

            Job * job = context->workloads.job_at(message->job_id);
            job->state = JobState::JOB_STATE_REJECTED;
            nb_completed_jobs++;

            XBT_INFO( "Job %d has been rejected", job->number);
        } break; // end of case SCHED_REJECTION

        case IPMessageType::SCHED_NOP_ME_LATER:
        {
            xbt_assert(task_data->data != nullptr);
            NOPMeLaterMessage * message = (NOPMeLaterMessage *) task_data->data;

            xbt_assert(message->target_time > MSG_get_clock(), "You asked to be awaken in the past! (you ask: %f, it is: %f)", message->target_time, MSG_get_clock());

            WaiterProcessArguments * args = new WaiterProcessArguments;
            args->target_time = message->target_time;

            string pname = "waiter " + to_string(message->target_time);
            MSG_process_create(pname.c_str(), waiter_process, (void*) args, context->machines.masterMachine()->host);
            ++nb_waiters;
        } break; // end of case SCHED_NOP_ME_LATER

        case IPMessageType::PSTATE_MODIFICATION:
        {
            xbt_assert(task_data->data != nullptr);
            PStateModificationMessage * message = (PStateModificationMessage *) task_data->data;

            context->current_switches.add_switch(message->machine_ids, message->new_pstate);

            for (auto machine_it = message->machine_ids.elements_begin(); machine_it != message->machine_ids.elements_end(); ++machine_it)
            {
                const int machine_id = *machine_it;
                Machine * machine = context->machines[machine_id];
                int curr_pstate = MSG_host_get_pstate(machine->host);

                if (machine->pstates[curr_pstate] == PStateType::COMPUTATION_PSTATE)
                {
                    if (machine->pstates[message->new_pstate] == PStateType::COMPUTATION_PSTATE)
                    {
                        XBT_INFO("Switching machine %d ('%s') pstate : %d -> %d.", machine->id,
                                 machine->name.c_str(), curr_pstate, message->new_pstate);
                        MSG_host_set_pstate(machine->host, message->new_pstate);
                        xbt_assert(MSG_host_get_pstate(machine->host) == message->new_pstate);

                        string reply_message_content;
                        if (context->current_switches.mark_switch_as_done(machine->id, message->new_pstate,
                                                                          reply_message_content, context))
                        {
                            send_buffer += "|" + std::to_string(MSG_get_clock()) + ":p:" + reply_message_content;
                            XBT_DEBUG("Message to send to scheduler : '%s'", send_buffer.c_str());
                        }
                    }
                    else if (machine->pstates[message->new_pstate] == PStateType::SLEEP_PSTATE)
                    {
                        machine->state = MachineState::TRANSITING_FROM_COMPUTING_TO_SLEEPING;
                        SwitchPStateProcessArguments * args = new SwitchPStateProcessArguments;
                        args->context = context;
                        args->machine_id = machine_id;
                        args->new_pstate = message->new_pstate;

                        string pname = "switch ON " + to_string(machine_id);
                        MSG_process_create(pname.c_str(), switch_off_machine_process, (void*)args, machine->host);

                        ++nb_switching_machines;
                    }
                    else
                        XBT_ERROR("Switching from a communication pstate to an invalid pstate on machine %d ('%s') : %d -> %d",
                                  machine->id, machine->name.c_str(), curr_pstate, message->new_pstate);
                }
                else if (machine->pstates[curr_pstate] == PStateType::SLEEP_PSTATE)
                {
                    xbt_assert(machine->pstates[message->new_pstate] == PStateType::COMPUTATION_PSTATE,
                            "Switching from a sleep pstate to a non-computation pstate on machine %d ('%s') : %d -> %d, which is forbidden",
                            machine->id, machine->name.c_str(), curr_pstate, message->new_pstate);

                    machine->state = MachineState::TRANSITING_FROM_SLEEPING_TO_COMPUTING;
                    SwitchPStateProcessArguments * args = new SwitchPStateProcessArguments;
                    args->context = context;
                    args->machine_id = machine_id;
                    args->new_pstate = message->new_pstate;

                    string pname = "switch OFF " + to_string(machine_id);
                    MSG_process_create(pname.c_str(), switch_on_machine_process, (void*)args, machine->host);

                    ++nb_switching_machines;
                }
                else
                    XBT_ERROR("Machine %d ('%s') has an invalid pstate : %d", machine->id, machine->name.c_str(), curr_pstate);
            }

        } break; // end of case PSTATE_MODIFICATION

        case IPMessageType::SCHED_NOP:
        {
            XBT_INFO("Nothing to do received.");
            if ((nb_running_jobs == 0) && (nb_scheduled_jobs < nb_submitted_jobs) && (nb_switching_machines == 0) && (nb_waiters == 0))
            {
                XBT_INFO("Nothing to do received while nothing is currently happening (no job is running, no machine is switching state, no wake-up timer is active) and some jobs are waiting to be scheduled... This might cause a deadlock!");

                // Let us display the available jobs (to help in the scheduler debugging)
                vector<string> submittedJobs;

                for (auto const workload_mit : context->workloads.workloads())
                {
                    const string & workload_name = workload_mit.first;
                    Workload * workload = workload_mit.second;
		    if(workload->jobs)
		      {
			for (auto & job_mit : workload->jobs->jobs())
			  {
			    const Job * job = job_mit.second;
			    if (job->state == JobState::JOB_STATE_SUBMITTED)
			      submittedJobs.push_back(workload_name + '!' + std::to_string(job->number));
			  }
		      }
                }

                string submittedJobsString = boost::algorithm::join(submittedJobs, ", ");

                XBT_INFO( "The available jobs are [%s]", submittedJobsString.c_str());
            }

        } break; // end of case SCHED_NOP

        case IPMessageType::SCHED_ALLOCATION:
        {
            xbt_assert(task_data->data != nullptr);
            SchedulingAllocationMessage * message = (SchedulingAllocationMessage *) task_data->data;

            for (SchedulingAllocation * allocation : message->allocations)
            {
                Job * job = context->workloads.job_at(allocation->job_id);
                job->state = JobState::JOB_STATE_RUNNING;

                nb_running_jobs++;
                xbt_assert(nb_running_jobs <= nb_submitted_jobs);
                nb_scheduled_jobs++;
                xbt_assert(nb_scheduled_jobs <= nb_submitted_jobs);

                if (!context->allow_space_sharing)
                {
                    for (auto machine_id_it = allocation->machine_ids.elements_begin(); machine_id_it != allocation->machine_ids.elements_end(); ++machine_id_it)
                    {
                        int machine_id = *machine_id_it;
                        const Machine * machine = context->machines[machine_id];
                        (void) machine; // Avoids a warning if assertions are ignored
                        xbt_assert(machine->jobs_being_computed.empty(),
                                   "Invalid job allocation: machine %d ('%s') is currently computing jobs (these ones:"
                                   " {%s}) whereas space sharing is forbidden. Space sharing can be enabled via an option,"
                                   " try --help to display the available options", machine->id, machine->name.c_str(),
                                   machine->jobs_being_computed_as_string().c_str());
                    }
                }

                if (context->energy_used)
                {
                    // Check that every machine is in a computation pstate
                    for (auto machine_id_it = allocation->machine_ids.elements_begin(); machine_id_it != allocation->machine_ids.elements_end(); ++machine_id_it)
                    {
                        int machine_id = *machine_id_it;
                        Machine * machine = context->machines[machine_id];
                        int ps = MSG_host_get_pstate(machine->host);
                        (void) ps; // Avoids a warning if assertions are ignored
                        xbt_assert(machine->has_pstate(ps));
                        xbt_assert(machine->pstates[ps] == PStateType::COMPUTATION_PSTATE,
                                   "Invalid job allocation: machine %d ('%s') is not in a computation pstate (ps=%d)",
                                   machine->id, machine->name.c_str(), ps);
                        xbt_assert(machine->state == MachineState::COMPUTING || machine->state == MachineState::IDLE,
                                   "Invalid job allocation: machine %d ('%s') cannot compute jobs now (the machine is"
                                   " neither computing nor being idle)", machine->id, machine->name.c_str());
                    }

                }

                ExecuteJobProcessArguments * exec_args = new ExecuteJobProcessArguments;
                exec_args->context = context;
                exec_args->allocation = allocation;
                string pname = "job" + to_string(job->number);
                MSG_process_create(pname.c_str(), execute_job_process, (void*)exec_args, context->machines[allocation->machine_ids.first_element()]->host);
            }

        } break; // end of case SCHED_ALLOCATION

        case IPMessageType::WAITING_DONE:
        {
            send_buffer += "|" + std::to_string(MSG_get_clock()) + ":N";
            XBT_DEBUG("Message to send to scheduler: '%s'", send_buffer.c_str());
            --nb_waiters;
        } break; // end of case WAITING_DONE

        case IPMessageType::SCHED_READY:
        {
            sched_ready = true;

        } break; // end of case SCHED_READY

        case IPMessageType::SWITCHED_ON:
        {
            xbt_assert(task_data->data != nullptr);
            SwitchONMessage * message = (SwitchONMessage *) task_data->data;

            xbt_assert(context->machines.exists(message->machine_id));
            Machine * machine = context->machines[message->machine_id];
            (void) machine; // Avoids a warning if assertions are ignored
            xbt_assert(MSG_host_get_pstate(machine->host) == message->new_pstate);

            string reply_message_content;
            if (context->current_switches.mark_switch_as_done(message->machine_id, message->new_pstate,
                                                              reply_message_content, context))
            {
                send_buffer += "|" + std::to_string(MSG_get_clock()) + ":p:" + reply_message_content;
                XBT_DEBUG("Message to send to scheduler : '%s'", send_buffer.c_str());
            }

            --nb_switching_machines;
        } break; // end of case SWITCHED_ON

        case IPMessageType::SWITCHED_OFF:
        {
            xbt_assert(task_data->data != nullptr);
            SwitchOFFMessage * message = (SwitchOFFMessage *) task_data->data;

            xbt_assert(context->machines.exists(message->machine_id));
            Machine * machine = context->machines[message->machine_id];
            (void) machine; // Avoids a warning if assertions are ignored
            xbt_assert(MSG_host_get_pstate(machine->host) == message->new_pstate);

            string reply_message_content;
            if (context->current_switches.mark_switch_as_done(message->machine_id, message->new_pstate,
                                                              reply_message_content, context))
            {
                send_buffer += "|" + std::to_string(MSG_get_clock()) + ":p:" + reply_message_content;
                XBT_DEBUG("Message to send to scheduler : '%s'", send_buffer.c_str());
            }

            --nb_switching_machines;
        } break; // end of case SWITCHED_ON

        case IPMessageType::SCHED_TELL_ME_ENERGY:
        {
            long double total_consumed_energy = context->machines.total_consumed_energy(context);

            send_buffer += "|" + std::to_string(MSG_get_clock()) + ":e:" +
                           std::to_string(total_consumed_energy);
            XBT_DEBUG("Message to send to scheduler : '%s'", send_buffer.c_str());
        } break; // end of case SCHED_TELL_ME_ENERGY

        case IPMessageType::SUBMITTER_CALLBACK:
        {
            xbt_assert(false, "The server received a SUBMITTER_CALLBACK message, which should not happen");
        } break; // end of case SUBMITTER_CALLBACK
        } // end of switch

        delete task_data;
        MSG_task_destroy(task_received);

        if (sched_ready && !send_buffer.empty())
        {
            RequestReplyProcessArguments * req_rep_args = new RequestReplyProcessArguments;
            req_rep_args->context = context;
            req_rep_args->send_buffer = to_string(protocol_version) + ":" + to_string(MSG_get_clock()) + send_buffer;
            send_buffer.clear();

            MSG_process_create("Scheduler REQ-REP", request_reply_scheduler_process, (void*)req_rep_args, MSG_host_self());
            sched_ready = false;
        }

    } // end of while

    XBT_INFO( "All jobs completed!");

    delete args;
    return 0;
}

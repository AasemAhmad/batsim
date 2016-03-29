#pragma once

/**
 * @file ipp.hpp Inter-Process Protocol
 */

#include <vector>
#include <string>

#include <simgrid/msg.h>

#include "machine_range.hpp"

struct BatsimContext;

enum class IPMessageType
{
    JOB_SUBMITTED           //!< Submitter -> Server. The submitter tells the server a new job has been submitted.
    ,JOB_COMPLETED          //!< Launcher/killer -> Server. The launcher tells the server a job has been completed.
    ,PSTATE_MODIFICATION    //!< SchedulerHandler -> Server. The scheduler handler tells the server a scheduling event occured (a pstate modification).
    ,SCHED_ALLOCATION       //!< SchedulerHandler -> Server. The scheduler handler tells the server a scheduling event occured (a job allocation).
    ,SCHED_REJECTION        //!< SchedulerHandler -> Server. The scheduler handler tells the server a scheduling event occured (a job rejection).
    ,SCHED_NOP              //!< SchedulerHandler -> Server. The scheduler handler tells the server a scheduling event occured (a NOP message).
    ,SCHED_NOP_ME_LATER     //!< SchedulerHandler -> Server. The scheduler handler tells the server a scheduling event occured (a NOP_ME_LATTER message).
    ,SCHED_TELL_ME_ENERGY   //!< SchedulerHandler -> Server. The scheduler handler tells the server a scheduling event occured (a TELL_ME_CONSUMED_ENERGY message).
    ,SCHED_READY            //!< SchedulerHandler -> Server. The scheduler handler tells the server that the scheduler is ready (messages can be sent to it).
    ,WAITING_DONE           //!< Waiter -> server. The waiter tells the server that the target time has been reached.
    ,SUBMITTER_HELLO        //!< Submitter -> Server. The submitter tells it starts submitting to the server.
    ,SUBMITTER_BYE          //!< Submitter -> Server. The submitter tells it stops submitting to the server.
    ,SWITCHED_ON            //!< SwitcherON -> Server. The switcherON process tells the server the machine pstate has been changed
    ,SWITCHED_OFF           //!< SwitcherOFF -> Server. The switcherOFF process tells the server the machine pstate has been changed.
};

struct JobSubmittedMessage
{
    int job_id; //! The job ID
};

struct JobCompletedMessage
{
    int job_id; //! The job ID
};

struct JobRejectedMessage
{
    int job_id; //! The job ID
};

struct SchedulingAllocation
{
    int job_id;
    MachineRange machine_ids; //! The IDs of the machines on which the job should be allocated
    std::vector<msg_host_t> hosts;  //! The corresponding SimGrid hosts
};

struct SchedulingAllocationMessage
{
    std::vector<SchedulingAllocation *> allocations;  //! Possibly several allocations
};

struct PStateModificationMessage
{
    MachineRange machine_ids; //! The IDs of the machines on which the pstate should be changed
    int new_pstate; //! The pstate the machines should be put into
};

struct NOPMeLaterMessage
{
    double target_time;
};

struct SwitchONMessage
{
    int machine_id;
    int new_pstate;
};

struct SwitchOFFMessage
{
    int machine_id;
    int new_pstate;
};

struct IPMessage
{
    ~IPMessage();
    IPMessageType type; //! The message type
    void * data;        //! The message data (can be NULL if type is in [SCHED_NOP, SUBMITTER_HELLO, SUBMITTER_BYE, SUBMITTER_READY]). Otherwise, it is either a JobSubmittedMessage*, a JobCompletedMessage* or a SchedulingAllocationMessage* according to type.
};

struct RequestReplyProcessArguments
{
    BatsimContext * context;
    std::string send_buffer;
};

struct ServerProcessArguments
{
    BatsimContext * context;
};

struct ExecuteJobProcessArguments
{
    BatsimContext * context;
    SchedulingAllocation * allocation;
};

struct KillerProcessArguments
{
    msg_task_t task; //! The task that will be cancelled if the walltime is reached
    double walltime; //! The number of seconds to wait before cancelling the task
};

struct SwitchPStateProcessArguments
{
    BatsimContext * context;
    int machine_id;
    int new_pstate;
};

struct JobSubmitterProcessArguments
{
    BatsimContext * context;
};

struct WaiterProcessArguments
{
    double target_time;
};

/**
 * @brief Sends a message from the given process to the given mailbox
 * @param[in] dst The destination mailbox
 * @param[in] type The type of message to send
 * @param[in] job_id The job the message is about
 * @param[in] data The data associated to the message
 */
void send_message(const std::string & destination_mailbox, IPMessageType type, void * data = nullptr);
void send_message(const char * destination_mailbox, IPMessageType type, void * data = nullptr);

std::string ipMessageTypeToString(IPMessageType type);

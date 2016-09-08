/**
 * @file ipp.hpp
 * @brief Inter-Process Protocol (within Batsim, not with the Decision real process)
 */

#pragma once

#include <vector>
#include <string>

#include <simgrid/msg.h>

#include "machine_range.hpp"

struct BatsimContext;

/**
 * @brief A simple structure used to identify one job
 */
struct JobIdentifier
{
    /**
     * @brief Creates a JobIdentifier
     * @param[in] workload_name The workload name
     * @param[in] job_number The job number
     */
    JobIdentifier(const std::string & workload_name = "", int job_number = -1);

    std::string workload_name; //!< The name of the workload the job belongs to
    int job_number; //!< The job unique number inside its workload

    /**
     * @brief Returns a string representation of the JobIdentifier.
     * @details Output format is WORKLOAD_NAME!JOB_NUMBER
     * @return A string representation of the JobIdentifier.
     */
    std::string to_string() const;
};

/**
 * @brief Compares two JobIdentifier thanks to their string representations
 * @param[in] ji1 The first JobIdentifier
 * @param[in] ji2 The second JobIdentifier
 * @return ji1.to_string() < ji2.to_string()
 */
bool operator<(const JobIdentifier & ji1, const JobIdentifier & ji2);

/**
 * @brief Stores the different types of inter-process messages
 */
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
    ,SUBMITTER_CALLBACK     //!< Server -> Submitter. The server sends a message to the Submitter. This message is initiated when a Job which has been submitted by the submitter has completed. The submitter must have said that it wanted to be called back when he said hello.
    ,SUBMITTER_BYE          //!< Submitter -> Server. The submitter tells it stops submitting to the server.
    ,SWITCHED_ON            //!< SwitcherON -> Server. The switcherON process tells the server the machine pstate has been changed
    ,SWITCHED_OFF           //!< SwitcherOFF -> Server. The switcherOFF process tells the server the machine pstate has been changed.
};

/**
 * @brief The content of the SUBMITTER_HELLO message
 */
struct SubmitterHelloMessage
{
    std::string submitter_name; //!< The name of the submitter. Must be unique. Is also used as a mailbox.
    bool enable_callback_on_job_completion; //!< If set to true, the submitter should be called back when its jobs complete.
};

/**
 * @brief The content of the SUBMITTER_BYE message
 */
struct SubmitterByeMessage
{
    std::string submitter_name; //!< The name of the submitter.
};

/**
 * @brief The content of the SUBMITTER_CALLBACK message
 */
struct SubmitterJobCompletionCallbackMessage
{
    JobIdentifier job_id; //!< The JobIdentifier
};

/**
 * @brief The content of the JobSubmitted message
 */
struct JobSubmittedMessage
{
    std::string submitter_name; //!< The name of the submitter which submitted the job.
    JobIdentifier job_id; //!< The JobIdentifier
};

/**
 * @brief The content of the JobCompleted message
 */
struct JobCompletedMessage
{
    JobIdentifier job_id; //!< The JobIdentifier
};

/**
 * @brief The content of the JobRejected message
 */
struct JobRejectedMessage
{
    JobIdentifier job_id; //!< The JobIdentifier
};

/**
 * @brief A subpart of the SchedulingAllocation message
 */
struct SchedulingAllocation
{
    JobIdentifier job_id; //!< The JobIdentifier
    MachineRange machine_ids; //!< The IDs of the machines on which the job should be allocated
    std::vector<msg_host_t> hosts;  //!< The corresponding SimGrid hosts
};

/**
 * @brief The content of the JobSubmitted message
 */
struct SchedulingAllocationMessage
{
    std::vector<SchedulingAllocation *> allocations;  //!< Possibly several allocations
};

/**
 * @brief The content of the PstateModification message
 */
struct PStateModificationMessage
{
    MachineRange machine_ids; //!< The IDs of the machines on which the pstate should be changed
    int new_pstate; //!< The power state into which the machines should be put
};

/**
 * @brief The content of the NOPMeLater message
 */
struct NOPMeLaterMessage
{
    double target_time; //!< The time at which Batsim should send a NOP message to the decision real process
};

/**
 * @brief The content of the SwitchON message
 */
struct SwitchONMessage
{
    int machine_id; //!< The unique number of the machine which should be switched ON
    int new_pstate; //!< The power state the machine should be put into
};

/**
 * @brief The content of the SwitchOFF message
 */
struct SwitchOFFMessage
{
    int machine_id; //!< The unique number of the machine which should be switched OFF
    int new_pstate; //!< The power state the machine should be put into
};

/**
 * @brief The base struct sent in inter-process messages
 */
struct IPMessage
{
    /**
     * @brief Destroys a IPMessage
     * @details This method deletes the message data according to its type
     */
    ~IPMessage();
    IPMessageType type; //!< The message type
    void * data;        //!< The message data (can be NULL if type is in [SCHED_NOP, SUBMITTER_HELLO, SUBMITTER_BYE, SUBMITTER_READY]). Otherwise, it is either a JobSubmittedMessage*, a JobCompletedMessage* or a SchedulingAllocationMessage* according to type.
};

/**
 * @brief The arguments of the request_reply_scheduler_process process
 */
struct RequestReplyProcessArguments
{
    BatsimContext * context;    //!< The BatsimContext
    std::string send_buffer;    //!< The message to send to the Decision real process
};

/**
 * @brief The arguments of the uds_server_process process
 */
struct ServerProcessArguments
{
    BatsimContext * context;    //!< The BatsimContext
};

/**
 * @brief The arguments of the execute_job_process process
 */
struct ExecuteJobProcessArguments
{
    BatsimContext * context;            //!< The BatsimContext
    SchedulingAllocation * allocation;  //!< The SchedulingAllocation
};

/**
 * @brief The arguments of the killer_process process
 */
struct KillerProcessArguments
{
    msg_task_t task; //!< The task that will be cancelled if the walltime is reached
    double walltime; //!< The number of seconds to wait before cancelling the task
};

/**
 * @brief The arguments of the switch_on_machine_process and switch_off_machine_process processes
 */
struct SwitchPStateProcessArguments
{
    BatsimContext * context;    //!< The BatsimContext
    int machine_id;             //!< The unique number of the machine whose power state should be switched
    int new_pstate;             //!< The power state into which the machine should be put
};

/**
 * @brief The arguments of the static_job_submitter_process process
 */
struct JobSubmitterProcessArguments
{
    BatsimContext * context;    //!< The BatsimContext
    std::string workload_name; //!< The name of the workload the submitter should use
};

/**
 * @brief The arguments of the workflow_submitter_process process
 */
struct WorkflowSubmitterProcessArguments
{
    BatsimContext * context;       //!< The BatsimContext
    std::string workflow_name; //!< The name of the workflow the submitter should use
};


/**
 * @brief The arguments of the waiter_process process
 */
struct WaiterProcessArguments
{
    double target_time; //!< The time at which the waiter should stop waiting
};

/**
 * @brief Sends a message from the given process to the given mailbox
 * @param[in] destination_mailbox The destination mailbox
 * @param[in] type The type of message to send
 * @param[in] data The data associated to the message
 */
void send_message(const std::string & destination_mailbox, IPMessageType type, void * data = nullptr);

/**
 * @brief Sends a message from the given process to the given mailbox
 * @param[in] destination_mailbox The destination mailbox
 * @param[in] type The type of message to send
 * @param[in] data The data associated to the message
 */
void send_message(const char * destination_mailbox, IPMessageType type, void * data = nullptr);

/**
 * @brief Sends a message from the given process to the given mailbox
 * @param[in] destination_mailbox The destination mailbox
 * @param[in] type The type of message to send
 * @param[in] data The data associated to the message
 */
void dsend_message(const std::string & destination_mailbox, IPMessageType type, void * data = nullptr);

/**
 * @brief Sends a message from the given process to the given mailbox
 * @param[in] destination_mailbox The destination mailbox
 * @param[in] type The type of message to send
 * @param[in] data The data associated to the message
 */
void dsend_message(const char * destination_mailbox, IPMessageType type, void * data = nullptr);

/**
 * @brief Transforms a IPMessageType into a std::string
 * @param[in] type The IPMessageType
 * @return The std::string corresponding to the type
 */
std::string ipMessageTypeToString(IPMessageType type);

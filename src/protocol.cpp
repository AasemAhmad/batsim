#include "protocol.hpp"

#include <xbt.h>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "context.hpp"
#include "jobs.hpp"
#include "network.hpp"

using namespace rapidjson;
using namespace std;

XBT_LOG_NEW_DEFAULT_CATEGORY(protocol, "protocol"); //!< Logging

JsonProtocolWriter::JsonProtocolWriter() :
    _alloc(_doc.GetAllocator())
{
    _doc.SetObject();
}

JsonProtocolWriter::~JsonProtocolWriter()
{

}

void JsonProtocolWriter::append_nop(double date)
{
    xbt_assert(date >= _last_date, "Date inconsistency");
   _last_date = date;
   _is_empty = false;
}

void JsonProtocolWriter::append_submit_job(const string &job_id,
                                           double date,
                                           const string &job_description,
                                           const string &profile_description,
                                           bool acknowledge_submission)
{
    xbt_assert(false, "Unimplemented!");
    (void) job_id;
    (void) date;
    (void) job_description;
    (void) profile_description;
    (void) acknowledge_submission;
}

void JsonProtocolWriter::append_execute_job(const string &job_id,
                                            const MachineRange &allocated_resources,
                                            double date,
                                            const string &executor_to_allocated_resource_mapping)
{
    xbt_assert(false, "Unimplemented!");
    (void) job_id;
    (void) allocated_resources;
    (void) date;
    (void) executor_to_allocated_resource_mapping;
}

void JsonProtocolWriter::append_reject_job(const string &job_id,
                                           double date)
{
    xbt_assert(false, "Unimplemented!");
    (void) job_id;
    (void) date;
}

void JsonProtocolWriter::append_kill_job(const vector<string> &job_ids,
                                         double date)
{
    xbt_assert(false, "Unimplemented!");
    (void) job_ids;
    (void) date;
}

void JsonProtocolWriter::append_set_resource_state(MachineRange resources,
                                                   const string &new_state,
                                                   double date)
{
    xbt_assert(false, "Unimplemented!");
    (void) resources;
    (void) new_state;
    (void) date;
}

void JsonProtocolWriter::append_call_me_later(double future_date,
                                              double date)
{
    xbt_assert(false, "Unimplemented!");
    (void) future_date;
    (void) date;
}

void JsonProtocolWriter::append_submitter_may_submit_jobs(double date)
{
    xbt_assert(false, "Unimplemented!");
    (void) date;
}

void JsonProtocolWriter::append_scheduler_finished_submitting_jobs(double date)
{
    xbt_assert(false, "Unimplemented!");
    (void) date;
}

void JsonProtocolWriter::append_query_request(void *anything,
                                              double date)
{
    xbt_assert(false, "Unimplemented!");
    (void) anything;
    (void) date;
}



void JsonProtocolWriter::append_simulation_begins(double date)
{
    /* {
      "timestamp": 0.0,
      "type": "SIMULATION_BEGINS",
      "data": {}
    } */

    xbt_assert(date >= _last_date, "Date inconsistency");
    _last_date = date;

    Value event(rapidjson::kObjectType);
    event.AddMember("timestamp", Value().SetDouble(date), _alloc);
    event.AddMember("type", Value().SetString("SIMULATION_BEGINS"), _alloc);
    event.AddMember("data", Value().SetObject(), _alloc);

    _events.PushBack(event, _alloc);
}

void JsonProtocolWriter::append_simulation_ends(double date)
{
    /* {
      "timestamp": 0.0,
      "type": "SIMULATION_ENDS",
      "data": {}
    } */

    xbt_assert(date >= _last_date, "Date inconsistency");
    _last_date = date;

    Value event(rapidjson::kObjectType);
    event.AddMember("timestamp", Value().SetDouble(date), _alloc);
    event.AddMember("type", Value().SetString("SIMULATION_ENDS"), _alloc);
    event.AddMember("data", Value().SetObject(), _alloc);

    _events.PushBack(event, _alloc);
}

void JsonProtocolWriter::append_job_submitted(const vector<string> & job_ids,
                                              double date)
{
    /* {
      "timestamp": 10.0,
      "type": "JOB_SUBMITTED",
      "data": {
        "job_ids": ["w0!1", "w0!2"]
      }
    } */

    xbt_assert(date >= _last_date, "Date inconsistency");
    _last_date = date;

    Value event(rapidjson::kObjectType);
    event.AddMember("timestamp", Value().SetDouble(date), _alloc);
    event.AddMember("type", Value().SetString("JOB_SUBMITTED"), _alloc);

    Value jobs(rapidjson::kArrayType);
    jobs.Reserve(job_ids.size(), _alloc);
    for (const string & job_id : job_ids)
        jobs.PushBack(Value().SetString(job_id.c_str(), _alloc), _alloc);

    event.AddMember("data", Value().SetObject().AddMember("job_ids", jobs, _alloc), _alloc);

    _events.PushBack(event, _alloc);
}

void JsonProtocolWriter::append_job_completed(const string & job_id,
                                              const string & job_status,
                                              double date)
{
    /* {
      "timestamp": 10.0,
      "type": "JOB_COMPLETED",
      "data": {"job_id": "w0!1", "status": "SUCCESS"}
    } */

    xbt_assert(date >= _last_date, "Date inconsistency");
    xbt_assert(std::find(accepted_completion_statuses.begin(), accepted_completion_statuses.end(), job_status) != accepted_completion_statuses.end(),
               "Unsupported job status '%s'!", job_status.c_str());
    _last_date = date;

    Value data(rapidjson::kObjectType);
    data.AddMember("job_id", Value().SetString(job_id.c_str(), _alloc), _alloc);
    data.AddMember("status", Value().SetString(job_status.c_str(), _alloc), _alloc);

    Value event(rapidjson::kObjectType);
    event.AddMember("timestamp", Value().SetDouble(date), _alloc);
    event.AddMember("type", Value().SetString("JOB_COMPLETED"), _alloc);
    event.AddMember("data", data, _alloc);

    _events.PushBack(event, _alloc);
}

void JsonProtocolWriter::append_job_killed(const vector<string> & job_ids,
                                           double date)
{
    /* {
      "timestamp": 10.0,
      "type": "JOB_KILLED",
      "data": {"job_ids": ["w0!1", "w0!2"]}
    } */

    xbt_assert(date >= _last_date, "Date inconsistency");
    _last_date = date;

    Value event(rapidjson::kObjectType);
    event.AddMember("timestamp", Value().SetDouble(date), _alloc);
    event.AddMember("type", Value().SetString("JOB_KILLED"), _alloc);

    Value jobs(rapidjson::kArrayType);
    jobs.Reserve(job_ids.size(), _alloc);
    for (const string & job_id : job_ids)
        jobs.PushBack(Value().SetString(job_id.c_str(), _alloc), _alloc);

    event.AddMember("data", Value().SetObject().AddMember("job_ids", jobs, _alloc), _alloc);

    _events.PushBack(event, _alloc);
}

void JsonProtocolWriter::append_resource_state_changed(const MachineRange & resources,
                                                       const string & new_state,
                                                       double date)
{
    /* {
      "timestamp": 10.0,
      "type": "RESOURCE_STATE_CHANGED",
      "data": {"resources": "1 2 3-5", "state": "42"}
    } */

    xbt_assert(date >= _last_date, "Date inconsistency");
    _last_date = date;

    Value data(rapidjson::kObjectType);
    data.AddMember("resources",
                   Value().SetString(resources.to_string_hyphen(" ", "-").c_str(), _alloc), _alloc);
    data.AddMember("state", Value().SetString(new_state.c_str(), _alloc), _alloc);

    Value event(rapidjson::kObjectType);
    event.AddMember("timestamp", Value().SetDouble(date), _alloc);
    event.AddMember("type", Value().SetString("RESOURCE_STATE_CHANGED"), _alloc);
    event.AddMember("data", data, _alloc);

    _events.PushBack(event, _alloc);
}

void JsonProtocolWriter::append_query_reply_energy(double consumed_energy,
                                                   double date)
{
    /* {
      "timestamp": 10.0,
      "type": "QUERY_REPLY",
      "data": {"energy_consumed": "12500" }
    } */

    xbt_assert(date >= _last_date, "Date inconsistency");
    _last_date = date;

    Value event(rapidjson::kObjectType);
    event.AddMember("timestamp", Value().SetDouble(date), _alloc);
    event.AddMember("type", Value().SetString("QUERY_REPLY"), _alloc);
    event.AddMember("data", Value().SetObject().AddMember("energy_consumed", Value().SetDouble(consumed_energy), _alloc), _alloc);

    _events.PushBack(event, _alloc);
}

void JsonProtocolWriter::clear()
{
    _is_empty = true;

    _doc.RemoveAllMembers();
    _events.SetArray();
}

string JsonProtocolWriter::generate_current_message(double date)
{
    xbt_assert(date >= _last_date, "Date inconsistency");
    xbt_assert(_events.IsArray(),
               "Successive calls to JsonProtocolWriter::generate_current_message without calling "
               "the clear() method is not supported");

    // Generating the content
    _doc.AddMember("now", Value().SetDouble(date), _alloc);
    _doc.AddMember("events", _events, _alloc);

    // Dumping the content to a buffer
    StringBuffer buffer;
    Writer<rapidjson::StringBuffer> writer(buffer);
    _doc.Accept(writer);

    // Returning the buffer as a string
    return string(buffer.GetString());
}

bool test_json_writer()
{
    AbstractProtocolWriter * proto_writer = new JsonProtocolWriter;
    printf("EMPTY content:\n%s\n", proto_writer->generate_current_message(0).c_str());
    proto_writer->clear();

    proto_writer->append_nop(0);
    printf("NOP content:\n%s\n", proto_writer->generate_current_message(42).c_str());
    proto_writer->clear();

    proto_writer->append_simulation_begins(10);
    printf("SIM_BEGINS content:\n%s\n", proto_writer->generate_current_message(42).c_str());
    proto_writer->clear();

    proto_writer->append_simulation_ends(10);
    printf("SIM_ENDS content:\n%s\n", proto_writer->generate_current_message(42).c_str());
    proto_writer->clear();

    proto_writer->append_job_submitted({"w0!j0", "w0!j1"}, 10);
    printf("JOB_SUBMITTED content:\n%s\n", proto_writer->generate_current_message(42).c_str());
    proto_writer->clear();

    proto_writer->append_job_completed("w0!j0", "SUCCESS", 10);
    printf("JOB_COMPLETED content:\n%s\n", proto_writer->generate_current_message(42).c_str());
    proto_writer->clear();

    proto_writer->append_job_killed({"w0!j0", "w0!j1"}, 10);
    printf("JOB_KILLED content:\n%s\n", proto_writer->generate_current_message(42).c_str());
    proto_writer->clear();

    proto_writer->append_resource_state_changed(MachineRange::from_string_hyphen("1,3-5"), "42", 10);
    printf("RESOURCE_STATE_CHANGED content:\n%s\n", proto_writer->generate_current_message(42).c_str());
    proto_writer->clear();

    proto_writer->append_query_reply_energy(65535, 10);
    printf("QUERY_REPLY (energy) content:\n%s\n", proto_writer->generate_current_message(42).c_str());
    proto_writer->clear();

    delete proto_writer;

    return true;
}



JsonProtocolReader::JsonProtocolReader(BatsimContext *context) :
    context(context)
{
    _type_to_handler_map["QUERY_REQUEST"] = &JsonProtocolReader::handle_query_request;
    _type_to_handler_map["REJECT_JOB"] = &JsonProtocolReader::handle_reject_job;
    _type_to_handler_map["EXECUTE_JOB"] = &JsonProtocolReader::handle_execute_job;
    _type_to_handler_map["CALL_ME_LATER"] = &JsonProtocolReader::handle_call_me_later;
    _type_to_handler_map["KILL_JOB"] = &JsonProtocolReader::handle_kill_job;
    _type_to_handler_map["SUBMIT_JOB"] = &JsonProtocolReader::handle_submit_job;
    _type_to_handler_map["SET_RESOURCE_STATE"] = &JsonProtocolReader::handle_set_resource_state;
    _type_to_handler_map["NOTIFY"] = &JsonProtocolReader::handle_notify;
}

JsonProtocolReader::~JsonProtocolReader()
{
}

void JsonProtocolReader::parse_and_apply_message(const string &message)
{
    rapidjson::Document doc;
    doc.Parse(message.c_str());

    xbt_assert(doc.IsObject(), "Invalid JSON message: not a JSON object");

    xbt_assert(doc.HasMember("now"), "Invalid JSON message: no 'now' key");
    xbt_assert(doc["now"].IsDouble(), "Invalid JSON message: 'now' value should be a double.");
    double now = doc["now"].GetDouble();

    xbt_assert(doc.HasMember("events"), "Invalid JSON message: no 'events' key");
    const auto & events_array = doc["events"];
    xbt_assert(events_array.IsArray(), "Invalid JSON message: 'events' value should be an array.");

    for (unsigned int i = 0; i < events_array.Size(); ++i)
    {
        const auto & event_object = events_array[i];
        parse_and_apply_event(event_object, i, now);
    }
}

void JsonProtocolReader::parse_and_apply_event(const Value & event_object,
                                               int event_number,
                                               double now)
{
    xbt_assert(event_object.IsObject(), "Invalid JSON message: event %d should be an object.", event_number);

    xbt_assert(event_object.HasMember("timestamp"), "Invalid JSON message: event %d should have a 'timestamp' key.", event_number);
    xbt_assert(event_object["timestamp"].IsDouble(), "Invalid JSON message: timestamp of event %d should be a double", event_number);
    double timestamp = event_object["timestamp"].GetDouble();
    xbt_assert(timestamp <= now, "Invalid JSON message: timestamp %g of event %d should be lower than or equal to now=%g.", timestamp, event_number, now);

    xbt_assert(event_object.HasMember("type"), "Invalid JSON message: event %d should have a 'type' key.", event_number);
    xbt_assert(event_object["type"].IsString(), "Invalid JSON message: event %d 'type' value should be a String", event_number);
    string type = event_object["type"].GetString();
    xbt_assert(_type_to_handler_map.find(type) != _type_to_handler_map.end(), "Invalid JSON message: event %d has an unknown 'type' value '%s'", event_number, type.c_str());

    xbt_assert(event_object.HasMember("data"), "Invalid JSON message: event %d should have a 'data' key.", event_number);
    const Value & data_object = event_object["data"];

    auto handler_function = _type_to_handler_map[type];
    handler_function(this, event_number, timestamp, data_object);
}

void JsonProtocolReader::handle_query_request(int event_number, double timestamp, const Value &data_object)
{
    /* {
      "timestamp": 10.0,
      "type": "QUERY_REQUEST",
      "data": {
        "requests": {"consumed_energy": {}}
      }
    } */

    xbt_assert(data_object.IsObject(), "Invalid JSON message: the 'data' value of event %d (QUERY_REQUEST) should be an object", event_number);
    xbt_assert(data_object.MemberCount() > 0, "Invalid JSON message: the 'data' value of event %d (QUERY_REQUEST) cannot be empty (size=%d)", event_number, (int)data_object.MemberCount());

    for (auto it = data_object.MemberBegin(); it != data_object.MemberEnd(); ++it)
    {
        const Value & key_value = it->name;
        const Value & value_object = it->value;

        xbt_assert(key_value.IsString(), "Invalid JSON message: a key within the 'data' object of event %d (QUERY_REQUEST) is not a string", event_number);
        string key = key_value.GetString();
        xbt_assert(std::find(accepted_requests.begin(), accepted_requests.end(), key) != accepted_requests.end(), "Invalid JSON message: Unknown QUERY_REQUEST '%s' of event %d", key.c_str(), event_number);

        xbt_assert(value_object.IsObject(), "Invalid JSON message: the value of '%s' inside 'data' object of event %d (QUERY_REQUEST) is not an object", key.c_str(), event_number);

        if (key == "consumed_energy")
        {
            xbt_assert(value_object.ObjectEmpty(), "Invalid JSON message: the value of '%s' inside 'data' object of event %d (QUERY_REQUEST) should be empty", key.c_str(), event_number);
            dsend_message(timestamp, "server", IPMessageType::SCHED_TELL_ME_ENERGY);
        }
    }
}

void JsonProtocolReader::handle_reject_job(int event_number, double timestamp, const Value &data_object)
{
    /* {
      "timestamp": 10.0,
      "type": "REJECT_JOB",
      "data": { "job_id": "w12!45" }
    } */

    xbt_assert(data_object.IsObject(), "Invalid JSON message: the 'data' value of event %d (REJECT_JOB) should be an object", event_number);
    xbt_assert(data_object.MemberCount() == 1, "Invalid JSON message: the 'data' value of event %d (REJECT_JOB) should be of size 1 (size=%d)", event_number, (int)data_object.MemberCount());

    xbt_assert(data_object.HasMember("job_id"), "Invalid JSON message: the 'data' value of event %d (REJECT_JOB) should contain a 'job_id' key.", event_number);
    const Value & job_id_value = data_object["job_id"];
    xbt_assert(job_id_value.IsString(), "Invalid JSON message: the 'job_id' value in the 'data' value of event %d (REJECT_JOB) should be a string.", event_number);
    string job_id = job_id_value.GetString();

    JobRejectedMessage * message = new JobRejectedMessage;
    if (!identify_job_from_string(context, job_id, message->job_id))
    {
        xbt_assert(false, "Invalid JSON message: "
                          "Invalid job rejection received: The job identifier '%s' is not valid. "
                          "Job identifiers must be of the form [WORKLOAD_NAME!]JOB_ID. "
                          "If WORKLOAD_NAME! is omitted, WORKLOAD_NAME='static' is used. "
                          "Furthermore, the corresponding job must exist.", job_id.c_str());
    }

    Job * job = context->workloads.job_at(message->job_id);
    xbt_assert(job->state == JobState::JOB_STATE_SUBMITTED,
               "Invalid JSON message: "
               "Invalid rejection received: job %s cannot be rejected at the present time."
               "For being rejected, a job must be submitted and not allocated yet.",
               job->id.c_str());

    dsend_message(timestamp, "server", IPMessageType::SCHED_REJECTION, (void*) message);
}

void JsonProtocolReader::handle_execute_job(int event_number, double timestamp, const Value &data_object)
{
    /* {
      "timestamp": 10.0,
      "type": "EXECUTE_JOB",
      "data": {
        "job_id": "w12!45",
        "alloc": "2-3",
        "mapping": {"0": "0", "1": "0", "2": "1", "3": "1"}
      }
    } */

    SchedulingAllocationMessage * message = new SchedulingAllocationMessage;
    SchedulingAllocation * sched_alloc = new SchedulingAllocation;
    message->allocations.push_back(sched_alloc);

    xbt_assert(data_object.IsObject(), "Invalid JSON message: the 'data' value of event %d (EXECUTE_JOB) should be an object", event_number);

    // *************************
    // Job identifier management
    // *************************
    // Let's read it from the JSON message
    xbt_assert(data_object.HasMember("job_id"), "Invalid JSON message: the 'data' value of event %d (EXECUTE_JOB) should contain a 'job_id' key.", event_number);
    const Value & job_id_value = data_object["job_id"];
    xbt_assert(job_id_value.IsString(), "Invalid JSON message: the 'job_id' value in the 'data' value of event %d (EXECUTE_JOB) should be a string.", event_number);
    string job_id = job_id_value.GetString();

    // Let's retrieve the job identifier
    if (!identify_job_from_string(context, job_id, sched_alloc->job_id))
    {
        xbt_assert(false, "Invalid JSON message: in event %d (EXECUTE_JOB): "
                          "The job identifier '%s' is not valid. "
                          "Job identifiers must be of the form [WORKLOAD_NAME!]JOB_ID. "
                          "If WORKLOAD_NAME! is omitted, WORKLOAD_NAME='static' is used. "
                          "Furthermore, the corresponding job must exist.",
                   event_number, job_id.c_str());
    }

    // Let's make sure the job is submitted
    Job * job = context->workloads.job_at(sched_alloc->job_id);
    xbt_assert(job->state == JobState::JOB_STATE_SUBMITTED,
               "Invalid JSON message: in event %d (EXECUTE_JOB): "
               "Invalid state of job %s: It cannot be executed now",
               event_number, job->id.c_str());

    // *********************
    // Allocation management
    // *********************
    // Let's read it from the JSON message
    xbt_assert(data_object.HasMember("alloc"), "Invalid JSON message: the 'data' value of event %d (EXECUTE_JOB) should contain a 'alloc' key.", event_number);
    const Value & alloc_value = data_object["alloc"];
    xbt_assert(alloc_value.IsString(), "Invalid JSON message: the 'alloc' value in the 'data' value of event %d (EXECUTE_JOB) should be a string.", event_number);
    string alloc = alloc_value.GetString();

    sched_alloc->machine_ids = MachineRange::from_string_hyphen(alloc, " ", "-", "Invalid JSON message: ");
    int nb_allocated_resources = sched_alloc->machine_ids.size();
    xbt_assert(nb_allocated_resources > 0, "Invalid JSON message: in event %d (EXECUTE_JOB): the number of allocated resources should be strictly positive (got %d).", event_number, nb_allocated_resources);

    // *****************************
    // Mapping management (optional)
    // *****************************
    if (data_object.HasMember("mapping"))
    {
        const Value & mapping_value = data_object["mapping"];
        xbt_assert(mapping_value.IsObject(), "Invalid JSON message: the 'mapping' value in the 'data' value of event %d (EXECUTE_JOB) should be a string.", event_number);
        xbt_assert(mapping_value.MemberCount() > 0, "Invalid JSON: the 'mapping' value in the 'data' value of event %d (EXECUTE_JOB) must be a non-empty object", event_number);
        map<int,int> mapping_map;

        // Let's fill the map from the JSON description
        for (auto it = mapping_value.MemberBegin(); it != mapping_value.MemberEnd(); ++it)
        {
            const Value & key_value = it->name;
            const Value & value_value = it->value;

            xbt_assert(key_value.IsInt() || key_value.IsString(), "Invalid JSON message: Invalid 'mapping' of event %d (EXECUTE_JOB): a key is not an integer nor a string", event_number);
            xbt_assert(value_value.IsInt() || value_value.IsString(), "Invalid JSON message: Invalid 'mapping' of event %d (EXECUTE_JOB): a value is not an integer nor a string", event_number);

            int executor;
            int resource;

            try
            {
                if (key_value.IsInt())
                    executor = key_value.GetInt();
                else
                    executor = std::stoi(key_value.GetString());

                if (value_value.IsInt())
                    resource = value_value.GetInt();
                else
                    resource = std::stoi(key_value.GetString());
            }
            catch (const exception & e)
            {
                xbt_assert(false, "Invalid JSON message: Invalid 'mapping' object of event %d (EXECUTE_JOB): all keys and values must be integers (or strings representing integers)", event_number);
            }

            mapping_map[executor] = resource;
        }

        // Let's write the mapping as a vector (keys will be implicit between 0 and nb_executor-1)
        sched_alloc->mapping.reserve(mapping_map.size());
        auto mit = mapping_map.begin();
        int nb_inserted = 0;

        xbt_assert(mit->first == nb_inserted, "Invalid JSON message: Invalid 'mapping' object of event %d (EXECUTE_JOB): no resource associated to executor %d.", event_number, nb_inserted);
        xbt_assert(mit->second >= 0 && mit->second < nb_allocated_resources, "Invalid JSON message: Invalid 'mapping' object of evend %d (EXECUTE_JOB): the %d-th resource within the allocation is told to be used, but there are only %d allocated resources.", event_number, mit->second, nb_allocated_resources);
        sched_alloc->mapping.push_back(mit->second);

        for (++mit, ++nb_inserted; mit != mapping_map.end(); ++mit, ++nb_inserted)
        {
            xbt_assert(mit->first == nb_inserted, "Invalid JSON message: Invalid 'mapping' object of event %d (EXECUTE_JOB): no resource associated to executor %d.", event_number, nb_inserted);
            xbt_assert(mit->second >= 0 && mit->second < nb_allocated_resources, "Invalid JSON message: Invalid 'mapping' object of evend %d (EXECUTE_JOB): the %d-th resource within the allocation is told to be used, but there are only %d allocated resources.", event_number, mit->second, nb_allocated_resources);
            sched_alloc->mapping.push_back(mit->second);
        }

        xbt_assert(sched_alloc->mapping.size() == mapping_map.size());
    }

    // Everything has been parsed correctly, let's inject the message into the simulation.
    dsend_message(timestamp, "server", IPMessageType::SCHED_ALLOCATION, (void*) message);
}

void JsonProtocolReader::handle_call_me_later(int event_number, double timestamp, const Value &data_object)
{
    /* {
      "timestamp": 10.0,
      "type": "CALL_ME_LATER",
      "data": {"timestamp": 25.5}
    } */

    NOPMeLaterMessage * message = new NOPMeLaterMessage;

    xbt_assert(data_object.IsObject(), "Invalid JSON message: the 'data' value of event %d (CALL_ME_LATER) should be an object", event_number);
    xbt_assert(data_object.MemberCount() == 1, "Invalid JSON message: the 'data' value of event %d (CALL_ME_LATER) should be of size 1 (size=%d)", event_number, (int)data_object.MemberCount());

    xbt_assert(data_object.HasMember("timestamp"), "Invalid JSON message: the 'data' value of event %d (CALL_ME_LATER) should contain a 'timestamp' key.", event_number);
    const Value & timestamp_value = data_object["timestamp"];
    xbt_assert(timestamp_value.IsDouble(), "Invalid JSON message: the 'timestamp' value in the 'data' value of event %d (CALL_ME_LATER) should be a double.", event_number);
    message->target_time = timestamp_value.GetDouble();

    if (message->target_time < MSG_get_clock())
        XBT_WARN("Event %d (CALL_ME_LATER) asks to be called at time %g but it is already reached", event_number, message->target_time);

    dsend_message(timestamp, "server", IPMessageType::SCHED_NOP_ME_LATER, (void*) message);
}

void JsonProtocolReader::handle_set_resource_state(int event_number, double timestamp, const Value &data_object)
{
    /* {
      "timestamp": 10.0,
      "type": "SET_RESOURCE_STATE",
      "data": {"resources": "1 2 3-5", "state": "42"}
    } */
    PStateModificationMessage * message = new PStateModificationMessage;

    // ********************
    // Resources management
    // ********************
    // Let's read it from the JSON message
    xbt_assert(data_object.HasMember("resources"), "Invalid JSON message: the 'data' value of event %d (SET_RESOURCE_STATE) should contain a 'resources' key.", event_number);
    const Value & resources_value = data_object["resources"];
    xbt_assert(resources_value.IsString(), "Invalid JSON message: the 'resources' value in the 'data' value of event %d (SET_RESOURCE_STATE) should be a string.", event_number);
    string resources = resources_value.GetString();

    message->machine_ids = MachineRange::from_string_hyphen(resources, " ", "-", "Invalid JSON message: ");
    int nb_allocated_resources = message->machine_ids.size();
    xbt_assert(nb_allocated_resources > 0, "Invalid JSON message: in event %d (SET_RESOURCE_STATE): the number of allocated resources should be strictly positive (got %d).", event_number, nb_allocated_resources);

    // State management
    xbt_assert(data_object.HasMember("state"), "Invalid JSON message: the 'data' value of event %d (SET_RESOURCE_STATE) should contain a 'state' key.", event_number);
    const Value & state_value = data_object["state"];
    xbt_assert(state_value.IsString(), "Invalid JSON message: the 'state' value in the 'data' value of event %d (SET_RESOURCE_STATE) should be a string.", event_number);
    string state_value_string = state_value.GetString();
    try { message->new_pstate = std::stoi(state_value_string); }
    catch(const exception &)
    {
        xbt_assert(false, "Invalid JSON message: the 'state' value in the 'data' value of event %d (SET_RESOURCE_STATE) should be a string corresponding to an integer (got '%s')", event_number, state_value_string.c_str());
    }

    dsend_message(timestamp, "server", IPMessageType::PSTATE_MODIFICATION, (void*) message);
}

void JsonProtocolReader::handle_notify(int event_number, double timestamp, const Value &data_object)
{
    xbt_assert(false, "Unimplemented! TODO");
    (void) event_number;
    (void) timestamp;
    (void) data_object;
}

void JsonProtocolReader::handle_submit_job(int event_number, double timestamp, const Value &data_object)
{
    xbt_assert(false, "Unimplemented! TODO");
    (void) event_number;
    (void) timestamp;
    (void) data_object;
}

void JsonProtocolReader::handle_kill_job(int event_number, double timestamp, const Value &data_object)
{
    xbt_assert(false, "Unimplemented! TODO");
    (void) event_number;
    (void) timestamp;
    (void) data_object;
}

void JsonProtocolReader::dsend_message(double when,
                                      const string &destination_mailbox,
                                      IPMessageType type,
                                      void *data) const
{
    // Let's wait until "when" time is reached
    double current_time = MSG_get_clock();
    if (when > current_time)
        MSG_process_sleep(when - current_time);

    // Let's actually send the message
    ::dsend_message(destination_mailbox, type, data);
}

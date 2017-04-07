#include "protocol.hpp"

#include <xbt.h>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace rapidjson;
using namespace std;

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
    xbt_assert(std::find(accepted_statuses.begin(), accepted_statuses.end(), job_status) != accepted_statuses.end(),
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
    xbt_assert(data_object.IsObject(), "Invalid JSON message: the 'data' value of event %d (QUERY_REQUEST) should be an object", event_number);
    xbt_assert(data_object.MemberCount() > 0, "Invalid JSON message: the 'data' value of event %d (QUERY_REQUEST) cannot be empty (size=%d)", event_number, (int)data_object.MemberCount());

    for (auto it = data_object.MemberBegin(); it != data_object.MemberEnd(); ++it)
    {
        const auto & key_value = it->name;
        const auto & value_object = it->value;

        xbt_assert(key_value.IsString(), "Invalid JSON message: a key within the 'data' object of event %d (QUERY_REQUEST) is not a string", event_number);
        string key = key_value.GetString();
        xbt_assert(std::find(accepted_requests.begin(), accepted_requests.end(), key) != accepted_requests.end(), "Invalid JSON message: Unknown QUERY_REQUEST '%s' of event %d", key.c_str(), event_number);

        xbt_assert(value_object.IsObject(), "Invalid JSON message: the value of '%s' inside 'data' object of event %d (QUERY_REQUEST) is not an object", key.c_str(), event_number);

        if (key == "consumed_energy")
        {
            xbt_assert(value_object.ObjectEmpty(), "Invalid JSON message: the value of '%s' inside 'data' object of event %d (QUERY_REQUEST) should be empty", key.c_str(), event_number);
            send_message(timestamp, "server", IPMessageType::SCHED_TELL_ME_ENERGY);
        }
    }
}

void JsonProtocolReader::send_message(double when,
                                      const string &destination_mailbox,
                                      IPMessageType type,
                                      void *data) const
{
    double current_time = MSG_get_clock();
    if (when > current_time)
        MSG_process_sleep(when - current_time);

    ::send_message(destination_mailbox, type, data);
}

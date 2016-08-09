/**
 * @file jobs.cpp
 * @brief Contains job-related structures
 */

#include "jobs.hpp"

#include <string>
#include <fstream>
#include <streambuf>
#include <algorithm>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>

#include <simgrid/msg.h>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include "profiles.hpp"

using namespace std;
using namespace rapidjson;

XBT_LOG_NEW_DEFAULT_CATEGORY(jobs, "jobs"); //!< Logging

Jobs::Jobs()
{

}

Jobs::~Jobs()
{
    for (auto mit : _jobs)
    {
        delete mit.second;
    }
}

void Jobs::setProfiles(Profiles *profiles)
{
    _profiles = profiles;
}

void Jobs::setWorkload(Workload *workload)
{
    _workload = workload;
}

void Jobs::load_from_json(const Document &doc, const string &filename)
{
    (void) filename; // Avoids a warning if assertions are ignored
    xbt_assert(doc.IsObject());
    xbt_assert(doc.HasMember("jobs"), "Invalid JSON file '%s': the 'jobs' array is missing", filename.c_str());
    const Value & jobs = doc["jobs"];
    xbt_assert(jobs.IsArray(), "Invalid JSON file '%s': the 'jobs' member is not an array", filename.c_str());

    for (SizeType i = 0; i < jobs.Size(); i++) // Uses SizeType instead of size_t
    {
        Job * j = new Job;
        j->workload = _workload;
        j->starting_time = -1;
        j->runtime = -1;
        j->state = JobState::JOB_STATE_NOT_SUBMITTED;
        j->consumed_energy = -1;

        const Value & job = jobs[i];
        xbt_assert(job.IsObject(), "Invalid JSON file '%s': one job is not an object", filename.c_str());

        xbt_assert(job.HasMember("id"), "Invalid JSON file '%s': one job has no 'id' field", filename.c_str());
        xbt_assert(job["id"].IsInt(), "Invalid JSON file '%s': one job has a non-integral 'id' field ('%s')", filename.c_str(), job["id"].GetString());
        j->number = job["id"].GetInt();

        xbt_assert(job.HasMember("subtime"), "Invalid JSON file '%s': job %d has no 'subtime' field", filename.c_str(), j->number);
        xbt_assert(job["subtime"].IsNumber(), "Invalid JSON file '%s': job %d has a non-number 'subtime' field", filename.c_str(), j->number);
        j->submission_time = job["subtime"].GetDouble();

        xbt_assert(job.HasMember("walltime"), "Invalid JSON file '%s': job %d has no 'walltime' field", filename.c_str(), j->number);
        xbt_assert(job["walltime"].IsNumber(), "Invalid JSON file '%s': job %d has a non-number 'walltime' field", filename.c_str(), j->number);
        j->walltime = job["walltime"].GetDouble();

        xbt_assert(job.HasMember("res"), "Invalid JSON file '%s': job %d has no 'res' field", filename.c_str(), j->number);
        xbt_assert(job["res"].IsInt(), "Invalid JSON file '%s': job %d has a non-number 'res' field", filename.c_str(), j->number);
        j->required_nb_res = job["res"].GetInt();

        xbt_assert(job.HasMember("profile"), "Invalid JSON file '%s': job %d has no 'profile' field", filename.c_str(), j->number);
        xbt_assert(job["profile"].IsString(), "Invalid JSON file '%s': job %d has a non-string 'profile' field", filename.c_str(), j->number);
        j->profile = job["profile"].GetString();

        // Let's get the JSON string which describes the job
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        job.Accept(writer);
        j->json_description = buffer.GetString();

        xbt_assert(!exists(j->number), "Invalid JSON file '%s': duplication of job id %d", filename.c_str(), j->number);
        _jobs[j->number] = j;
    }
}

Job *Jobs::operator[](int job_id)
{
    auto it = _jobs.find(job_id);
    xbt_assert(it != _jobs.end(), "Cannot get job %d: it does not exist", job_id);
    return it->second;
}

const Job *Jobs::operator[](int job_id) const
{
    auto it = _jobs.find(job_id);
    xbt_assert(it != _jobs.end(), "Cannot get job %d: it does not exist", job_id);
    return it->second;
}

Job *Jobs::at(int job_id)
{
    return operator[](job_id);
}

const Job *Jobs::at(int job_id) const
{
    return operator[](job_id);
}

bool Jobs::exists(int job_id) const
{
    auto it = _jobs.find(job_id);
    return it != _jobs.end();
}

bool Jobs::containsSMPIJob() const
{
    xbt_assert(_profiles != nullptr, "Invalid Jobs::containsSMPIJob call: setProfiles had not been called yet");
    for (auto & mit : _jobs)
    {
        Job * job = mit.second;
        if ((*_profiles)[job->profile]->type == ProfileType::SMPI)
            return true;
    }
    return false;
}

void Jobs::displayDebug() const
{
    // Let us traverse jobs to display some information about them
    vector<string> jobsVector;
    for (auto & mit : _jobs)
    {
        jobsVector.push_back(std::to_string(mit.second->number));
    }

    // Let us create the string that will be sent to XBT_INFO
    string s = "Jobs debug information:\n";

    s += "There are " + to_string(_jobs.size()) + " jobs.\n";
    s += "Jobs : [" + boost::algorithm::join(jobsVector, ", ") + "]";

    // Let us display the string which has been built
    XBT_INFO("%s", s.c_str());
}

const std::map<int, Job* > &Jobs::jobs() const
{
    return _jobs;
}

bool job_comparator_subtime(const Job *a, const Job *b)
{
    return a->submission_time < b->submission_time;
}

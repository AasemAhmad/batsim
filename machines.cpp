#include "machines.hpp"

#include <algorithm>
#include <iterator>
#include <map>

#include "export.hpp"

using namespace std;

Machines::Machines()
{

}

Machines::~Machines()
{
    for (Machine * machine : _machines)
        delete machine;
    _machines.clear();
}

void Machines::createMachines(xbt_dynar_t hosts, const string &masterHostName)
{
    xbt_assert(_machines.size() == 0, "Bad call to Machines::createMachines(): machines already created");

    int nb_machines = xbt_dynar_length(hosts);
    _machines.reserve(nb_machines);

    msg_host_t host;
    unsigned int i;
    xbt_dynar_foreach(hosts, i, host)
    {
        Machine * machine = new Machine;

        machine->id = i;
        machine->name = MSG_host_get_name(host);
        machine->host = host;
        machine->jobs_being_computed = {};
        machine->state = MachineState::IDLE;

        if (machine->name != masterHostName)
            _machines.push_back(machine);
        else
        {
            xbt_assert(_masterMachine == nullptr);
            _masterMachine = machine;
        }
    }

    xbt_assert(_masterMachine != nullptr, "Cannot find the MasterHost '%s' in the platform file", masterHostName.c_str());
}

const Machine * Machines::operator[](int machineID) const
{
    return _machines[machineID];
}

Machine * Machines::operator[](int machineID)
{
    return _machines[machineID];
}

bool Machines::exists(int machineID) const
{
    return machineID >= 0 && machineID < (int)_machines.size();
}

const std::vector<Machine *> &Machines::machines() const
{
    return _machines;
}

const Machine *Machines::masterMachine() const
{
    return _masterMachine;
}

void Machines::updateMachinesOnJobRun(int jobID, const std::vector<int> & usedMachines)
{
    for (int machineID : usedMachines)
    {
        Machine * machine = _machines[machineID];
        machine->state = MachineState::COMPUTING;

        int previous_top_job = -1;
        if (!machine->jobs_being_computed.empty())
            previous_top_job = *machine->jobs_being_computed.begin();

        // cout << machine;
        machine->jobs_being_computed.insert(jobID);
        // cout << machine;

        if (previous_top_job == -1 || previous_top_job != *machine->jobs_being_computed.begin())
        {
            xbt_assert(_tracer != nullptr, "Invalid Machines::updateMachinesOnJobRun call: setTracer has not been called");
            _tracer->set_machine_as_computing_job(machine->id, *machine->jobs_being_computed.begin(), MSG_get_clock());
        }
    }
}

void Machines::updateMachinesOnJobEnd(int jobID, const std::vector<int> & usedMachines)
{
    for (int machineID : usedMachines)
    {
        Machine * machine = _machines[machineID];
        // cout << machine;

        xbt_assert(!machine->jobs_being_computed.empty());
        int previous_top_job = *machine->jobs_being_computed.begin();

        // Let's erase jobID in the jobs_being_computed data structure
        int ret = machine->jobs_being_computed.erase(jobID);
        xbt_assert(ret == 1);

        if (machine->jobs_being_computed.empty())
        {
            machine->state = MachineState::IDLE;
            xbt_assert(_tracer != nullptr, "Invalid Machines::updateMachinesOnJobRun call: setTracer has not been called");
            _tracer->set_machine_idle(machine->id, MSG_get_clock());
        }
        else if (*machine->jobs_being_computed.begin() != previous_top_job)
        {
            xbt_assert(_tracer != nullptr, "Invalid Machines::updateMachinesOnJobRun call: setTracer has not been called");
            _tracer->set_machine_as_computing_job(machine->id, *machine->jobs_being_computed.begin(), MSG_get_clock());
        }

        // cout << machine;
    }
}

void Machines::setTracer(PajeTracer *tracer)
{
    _tracer = tracer;
}

ostream & operator<<(ostream & out, const Machine & machine)
{
    out << "Machine " << machine.id << ", ";
    out << "state = " << machineStateToString(machine.state) << ", ";
    out << "jobs = [";
    std::copy(machine.jobs_being_computed.begin(), machine.jobs_being_computed.end(),
              std::ostream_iterator<char>(out, " "));
    out << "]" << endl;

    return out;
}

string machineStateToString(MachineState state)
{
    static const std::map<MachineState,std::string> conv =
    {
        {MachineState::SLEEPING, "sleeping"},
        {MachineState::IDLE, "idle"},
        {MachineState::COMPUTING, "computing"}
    };

    return conv.at(state);
}

/**
 * @file export.hpp
 * @brief Batsim's export classes and functions
 * @details Contains the classes and functions which are related to Batsim's exports.
 */

#pragma once

#include <stdio.h>
#include <sys/types.h> /* ssize_t, needed by xbt/str.h, included by msg/msg.h */
#include <vector>
#include <string>
#include <fstream>
#include <map>

#include <simgrid/msg.h>

#include "machines.hpp"

struct BatsimContext;

/**
 * @brief Buffered-write output file
 */
class WriteBuffer
{
public:
    /**
     * @brief Builds a WriteBuffer
     * @param[in] filename The file that will be written
     * @param[in] bufferSize The size of the buffer (in bytes).
     */
    WriteBuffer(const std::string & filename, int bufferSize = 64*1024);

    /**
     * @brief Destructor
     * @details This method flushes the buffer if it is not empty, destroys the buffer and closes the file.
     */
    ~WriteBuffer();

    /**
     * @brief Appends a text at the end of the buffer. If the buffer is full, it is automatically flushed into the disk.
     * @param[in] text The text to append
     */
    void appendText(const char * text);

    /**
     * @brief Write the current content of the buffer into the file
     */
    void flushBuffer();

private:
    std::ofstream f;            //!< The file stream on which the buffer is outputted
    const int bufferSize;       //!< The buffer maximum size
    char * buffer = nullptr;    //!< The buffer
    int bufferPos = 0;          //!< The current position of the buffer (previous positions are already written)
};

/**
 * @brief Exports the job execution to a CSV file
 * @param[in] filename The name of the output file used to write the CSV data
 * @param[in] context The BatsimContext
 */
void exportJobsToCSV(const std::string &filename, const BatsimContext * context);

/**
 * @brief Compute and exports some schedule criteria to a CSV file
 * @param[in] filename The name of the output file used to write the CSV data
 * @param[in] context The BatsimContext
 */
void exportScheduleToCSV(const std::string &filename, const BatsimContext * context);


/**
 * @brief Allows to handle a Pajé trace corresponding to a schedule
 */
class PajeTracer
{
private:
    /**
     * @brief Enumerates the different states of a PajeTracer
     */
    enum PajeTracerState
    {
        UNINITIALIZED   //!< The PajeTracer has not been initialized yet
        ,INITIALIZED    //!< The PajeTracer has been initialized
        ,FINALIZED      //!< The PajeTracer has been finalized
    };

    /**
     * @brief Enumerates the constants used in the output of the Paje trace
     */
    enum PajeTracerOutputConstants
    {
        DEFINE_CONTAINER_TYPE = 1   //!< Defines a container type
        ,CREATE_CONTAINER           //!< Creates a container
        ,DESTROY_CONTAINER          //!< Destroys a container
        ,DEFINE_STATE_TYPE          //!< Defines a state type
        ,DEFINE_ENTITY_VALUE        //!< Defines an entity value
        ,SET_STATE                  //!< Sets a state
        ,DEFINE_EVENT_TYPE          //!< Defines an event type
        ,NEW_EVENT                  //!< Tells an event occured
        ,DEFINE_VARIABLE_TYPE       //!< Defines a variable type
        ,SET_VARIABLE               //!< Sets a variable
    };

public:
    /**
     * @brief Builds a PajeTracer
     * @param[in] logLaunchings If set to true, job launching time will be written in the trace. This option leads to larger trace files
     */
    PajeTracer(bool logLaunchings = false);

    /**
     * @brief Sets the filename of a PajeTracer
     * @param[in] filename The name of the output file
     */
    void setFilename(const std::string & filename);

    /**
     * @brief PajeTracer destructor.
     */
    ~PajeTracer();

    /**
     * @brief Initializes a PajeTracer.
     * @details This function must be called once before adding job launchings, runnings or endings.
     * @param[in] context The BatsimContext
     * @param[in] time The beginning time
     */
    void initialize(const BatsimContext * context, double time);

    /**
     * @brief Finalizes a PajeTracer.
     * @details This function must be called before the PajeTracer's object destruction.
     * @param[in] context The BatsimContext
     * @param[in] time The simulation time at which the finalization is done
     */
    void finalize(const BatsimContext * context, double time);

    /**
     * @brief Adds a job launch in the file trace.
     * @details Please note that this method can only be called when the PajeTracer object has been initialized and had not been finalized yet.
     * @param[in] jobID The job unique number
     * @param[in] usedMachineIDs The machines which compute the job
     * @param[in] time The simulation time at which the addition is done
     */
    void addJobLaunching(int jobID, const std::vector<int> & usedMachineIDs, double time);

    /**
     * @brief Creates a job in the Pajé output file
     * @param[in] jobID The job unique number
     */
    void register_new_job(int jobID);

    /**
     * @brief Sets a machine in the idle state
     * @param[in] machineID The unique machine number
     * @param[in] time The time at which the machine should be marked as idle
     */
    void set_machine_idle(int machineID, double time);

    /**
     * @brief Sets a machine in the computing state
     * @param[in] machineID The unique machine number
     * @param[in] jobID The unique job number that the machine computes
     * @param[in] time The time at which the machine should be marked as computing the job
     */
    void set_machine_as_computing_job(int machineID, int jobID, double time);

    /**
     * @brief Adds a job kill in the file trace.
     * @details Please note that this method can only be called when the PajeTracer object has been initialized and had not been finalized yet.
     * @param[in] jobID The job unique number
     * @param[in] usedMachineIDs The machines which compute the job
     * @param[in] time The simulation time at which the kill is done
     * @param[in] associateKillToMachines By default (false), one event is added in the killer container. If set to true, one event is added for every machine on which the kill occurs.
     */
    void addJobKill(int jobID, const MachineRange & usedMachineIDs, double time, bool associateKillToMachines = false);

    /**
     * @brief Adds a global utilization value of the system.
     * @details Please note that this method can only be called when the PajeTracer object has been initialized and had not been finalized yet.
     * @param[in] utilization The global utilization of the system.
     * @param[in] time The simulation time at which the system has this utilization value
     */
    void addGlobalUtilization(double utilization, double time);

public:
    /**
     * @brief Give the RGB representation of a color represented in HSV
     * @details This function is greatly inspired by http://www.cs.rit.edu/~ncs/color/t_convert.html
     * @param[in] h The hue, whose value is in [0,360]
     * @param[in] s The saturation, whose value is in [0,1]
     * @param[in] v The value, whose value is in [0,1]
     * @param[out] r The red, whose value is in [0,1]
     * @param[out] g The green, whose value is in [0,1]
     * @param[out] b The blue, whose value is in [0,1]
     */
    static void hsvToRgb(double h, double s, double v, double & r, double & g, double & b);

private:
    /**
     * @brief Generate colors
     * @details The colors are fairly shared in the Hue color spectrum.
     * @param[in] colorCount colorCount
     */
    void generateColors(int colorCount = 8);

    /**
     * @brief Randomize the position of the colors in the colormap
     */
    void shuffleColors();

private:
    const char * rootType = "root_ct";                  //!< The root type output name
    const char * machineType = "machine_ct";            //!< The machine type output name
    const char * machineState = "machine_state";        //!< The machine state output name
    const char * schedulerType = "scheduler_ct";        //!< The scheduler type output name
    const char * killerType = "killer_ct";              //!< The killer type output name
    const char * killEventKiller = "kk";                //!< The kill event (on the Killer) output name
    const char * killEventMachine = "km";               //!< The kill event (on machines) output name
    const char * utilizationVarType = "vu_vt";          //!< The utilization variable type output name

    const char * mstateWaiting = "w";                   //!< The waiting state output name
    const char * mstateLaunching = "l";                 //!< The launching state output name

    const char * root = "root";                         //!< The Pajé root output name
    const char * scheduler = "sc";                      //!< The scheduler output name
    const char * killer = "k";                          //!< The killer output name
    const char * machinePrefix = "m";                   //!< The machine output prefix
    const char * jobPrefix = "j";                       //!< The job output prefix
    const char * waitingColor= "\"0.0 0.0 0.0\"";       //!< The color used for idle machines
    const char * launchingColor = "\"0.3 0.3 0.3\"";    //!< The color used for machines launching a job
    const char * utilizationColor = "\"0.0 0.5 0.0\"";  //!< The color used for the utilization graph

    const bool _logLaunchings; //!< If set to true, job launchings should be outputted (in addition to starting decisions)

    WriteBuffer * _wbuf = nullptr;  //!< The buffer class used to handle the output file

    std::map<int, std::string> _jobs; //!< Maps job numbers to their Pajé representation
    std::vector<std::string> _colors; //!< Strings associated with colors, used for the jobs

    PajeTracerState state = UNINITIALIZED; //!< The state of the PajeTracer
};


/**
 * @brief Traces how power states are changed over time
 */
class PStateChangeTracer
{
public:
    /**
     * @brief Constructs a PStateChangeTracer
     */
    PStateChangeTracer();

    /**
     * @brief Sets the output filename of the tracer
     * @param filename The name of the output file of the tracer
     */
    void setFilename(const std::string & filename);

    /**
     * @brief Destroys a PStateChangeTracer
     */
    ~PStateChangeTracer();

    /**
     * @brief Adds a power state change in the tracer
     * @param time The time at which the change occurs
     * @param machines The machines whose state has been changed
     * @param pstate_after The power state the machine will be in after the given time
     */
    void add_pstate_change(double time, MachineRange machines, int pstate_after);

private:
    WriteBuffer * _wbuf = nullptr; //!< The buffer used to handle the output file
};

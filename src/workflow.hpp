/**
 * @file workflow.hpp
 * @brief Contains workflow-related classes
 */

#pragma once

#include <string>
#include <map>

class Job;

/**
 * @brief A workflow is a DAG of tasks, with points to
 *        source tasks and sink tasks
 */
class Workflow
{
public:
    /**
     * @brief Builds an empty Workflow
     */
    Workflow();

    /**
     * @brief Destroys a Workflow
     */
    ~Workflow();

    /**
     * @brief Loads a complete workflow from an XML filename
     * @param[in] xml_filename The name of the XML file
     */
    void load_from_xml(const std::string & xml_filename)

    /**
     * @brief Checks whether a Workflow is valid (not needed since loading from XML?)
     */
    void check_validity();


public:
    std::string name; //!< The Workflow name
    Task * tasks = nullptr; //!< ALL tasks of the Workflow
    Task * sources = nullptr; //!< The source tasks of the workflow
    Task * sinks = nullptr; //!< The sink tasks of the workflow

};

/**
 * @brief A workflow Task is some attributes, pointers to parent tasks,
 *        and pointers to children tasks.
 */
class Task
{
public:
    /**
     * @brief Constructor
     */
    Task(int num_procs, double execution_time, Job batsim_job);

    /**
     * @brief Destructeur
     */
    ~Task();

    /**
     * @brief Add a parent to a task
     */
    void add_parent(Task parent_task);

    /**
     * @brief Add a child to a task
     */
    void add_child(Task child_task);


public:
    int num_procs; //!< The number of processors needed for the tas
    double execution_time; //!< The execution time of the task
    Job batsim_job; //!< The batsim job created for this task
    Task * parents = nullptr; //!< The parents
    Task * children = nullptr; //!< The children

};



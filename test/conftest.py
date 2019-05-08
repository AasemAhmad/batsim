#!/usr/bin/env python3
from collections import namedtuple
import glob
from os.path import abspath, basename, dirname, realpath

Workload = namedtuple('Workload', ['name', 'filename'])
Platform = namedtuple('Platform', ['name', 'filename'])
Algorithm = namedtuple('Platform', ['name', 'sched_implem', 'sched_algo_name'])

def generate_platforms(platform_dir, definition, accepted_names):
    return [Platform(
        name=name,
        filename=abspath(f'{platform_dir}/{filename}'))
        for name, filename in definition.items() if name in accepted_names]

def generate_workloads(workload_dir, definition, accepted_names):
    return [Workload(
        name=name,
        filename=abspath(f'{workload_dir}/{filename}'))
        for name, filename in definition.items() if name in accepted_names]

def generate_batsched_algorithms(definition, accepted_names):
    return [Algorithm(name=name, sched_implem='batsched', sched_algo_name=sched_algo_name)
        for name, sched_algo_name in definition.items() if name in accepted_names]

def pytest_generate_tests(metafunc):
    script_dir = dirname(realpath(__file__))
    batsim_dir = realpath(f'{script_dir}/..')
    platform_dir = realpath(f'{batsim_dir}/platforms')
    workload_dir = realpath(f'{batsim_dir}/workloads')

    #############################
    # Define the various inputs #
    #############################
    # Platforms
    platforms_def = {
        "small": "small_platform.xml",
        "cluster512": "cluster512.xml",
        "energy128notopo": "energy_platform_homogeneous_no_net_128.xml",
        "energy128cluster": "cluster_energy_128.xml",
    }
    energy_platforms = ["energy128notopo", "energy128cluster"]

    # Workloads
    workloads_def = {
        "delay1": "test_one_delay_job.json",
        "delays": "test_delays.json",
        "delaysequences": "test_sequence_delay.json",
        "energymini0": "test_energy_minimal_load0.json",
        "energymini50": "test_energy_minimal_load50.json",
        "energymini100": "test_energy_minimal_load100.json",
        "compute1": "test_one_computation_job.json",
        "computetot1": "test_one_computation_job_tot.json",
        "farfuture": "test_long_workload.json",
        "long": "test_batsim_paper_workload_seed1.json",
        "mixed": "test_various_profile_types.json",
        "samesubmittime": "test_same_submit_time.json",
        "smpicomp1": "test_smpi_compute_only.json",
        "smpicomp2": "test_smpi_compute_only_2_jobs.json",
        "smpimapping": "test_smpi_mapping.json",
        "smpimixed": "test_smpi_mixed_comp_comm.json",
        "smpicollectives": "test_smpi_collectives.json",
        "tuto1": "test_case_study1.json",
        "walltime": "test_walltime.json",
    }
    one_job_workloads = ["delay1", "compute1", "computetot1"]
    small_workloads = ["delays", "delaysequences", "mixed"]
    smpi_workloads = ["smpicomp1", "smpicomp2", "smpimapping", "smpimixed", "smpicollectives"]
    dynsub_workloads = ["delay1", "mixed"]

    # Algorithms
    algorithms_def = {
        "easy": "easy_bf",
        "easyfast": "easy_bf_fast",
        "energywatcher": "energy_watcher",
        "idlesleeper": "energy_bf_idle_sleeper",
        "fcfs": "fcfs_fast",
        "filler": "filler",
        "random": "random",
        "rejecter": "rejecter",
        "sequencer": "sequencer",
        "sleeper": "sleeper",
        "submitter": "submitter",
    }
    basic_algorithms = ["fcfs", "easyfast", "filler"]
    energy_algorithms = ["sleeper", "energywatcher"]

    ##########################################
    # Generate fixtures from the definitions #
    ##########################################
    # Platforms
    if 'platform' in metafunc.fixturenames:
        metafunc.parametrize('platform', generate_platforms(platform_dir, platforms_def, [key for key in platforms_def]))
    if 'small_platform' in metafunc.fixturenames:
        metafunc.parametrize('small_platform', generate_platforms(platform_dir, platforms_def, ['small']))
    if 'energy_platform' in metafunc.fixturenames:
        metafunc.parametrize('energy_platform', generate_platforms(platform_dir, platforms_def, energy_platforms))

    # Workloads
    if 'workload' in metafunc.fixturenames:
        metafunc.parametrize('workload', generate_workloads(workload_dir, workloads_def, [key for key in workload_def]))
    if 'one_job_workload' in metafunc.fixturenames:
        metafunc.parametrize('one_job_workload', generate_workloads(workload_dir, workloads_def, one_job_workloads))
    if 'small_workload' in metafunc.fixturenames:
        metafunc.parametrize('small_workload', generate_workloads(workload_dir, workloads_def, small_workloads))
    if 'smpi_workload' in metafunc.fixturenames:
        metafunc.parametrize('smpi_workload', generate_workloads(workload_dir, workloads_def, smpi_workloads))
    if 'dynsub_workload' in metafunc.fixturenames:
        metafunc.parametrize('dynsub_workload', generate_workloads(workload_dir, workloads_def, dynsub_workloads))

    # Algorithms
    if 'basic_algorithm' in metafunc.fixturenames:
        metafunc.parametrize('basic_algorithm', generate_batsched_algorithms(algorithms_def, basic_algorithms))
    if 'energy_algorithm' in metafunc.fixturenames:
        metafunc.parametrize('energy_algorithm', generate_batsched_algorithms(algorithms_def, energy_algorithms))
    if 'submitter_algorithm' in metafunc.fixturenames:
        metafunc.parametrize('submitter_algorithm', generate_batsched_algorithms(algorithms_def, ['submitter']))

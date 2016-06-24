#/usr/bin/python2

import json
import struct
import socket
import sys, re

class Batsim(object):

    def __init__(self, json_file, scheduler, validatingmachine=None, server_address = '/tmp/bat_socket', verbose=0):
        self.server_address = server_address
        self.verbose = verbose
        sys.setrecursionlimit(10000)

        if validatingmachine is None:
            self.scheduler = scheduler
        else:
            self.scheduler = validatingmachine(scheduler)

        #load json file
        self._load_json_workload_profile(json_file)

        #open connection
        self._connection = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        print("[BATSIM]: connecting to %r" % server_address)
        try:
            self._connection.connect(server_address)
            if self.verbose > 1: print('[BATSIM]: connected')
        except socket.error:
            print("[BATSIM]: socket error")
            raise

        #initialize some public attributes
        self.last_msg_recv_time = -1

        self.nb_jobs_recieved = 0
        self.nb_jobs_scheduled = 0
        self.nb_jobs_json = len(self.jobs)

        self.scheduler.bs = self
        self.scheduler.onAfterBatsimInit()


    def time(self):
        return self._current_time

    def consume_time(self, t):
        self._current_time += float(t)
        return self._current_time

    def wake_me_up_at(self, time):
        self._msgs_to_send.append( ( self.time(), "n:"+str(time) ) )

    def start_jobs_continuous(self, allocs):
        """
        allocs should have the followinf format:
        [ (job, (first res, last res)), (job, (first res, last res)), ...]
        """
        if len(allocs) > 0:
            msg = "J:"
            for (job, (first_res, last_res)) in allocs:
                self.nb_jobs_scheduled += 1
                msg += str(job.id)+ "=" + str(first_res) + "-" + str(last_res)+ ";"

            msg = msg[:-1] # remove last semicolon
            self._msgs_to_send.append( ( self.time(), msg ) )

    def start_jobs(self, jobs, res):
        if len(jobs) > 0:
            msg = "J:"
            for j in jobs:
                self.nb_jobs_scheduled += 1
                msg += str(j.id) + "="
                for r in res[j.id]:
                    msg += str(r) + ","
                msg = msg[:-1] + ";" # replace last comma by semicolon separtor between jobs
            msg = msg[:-1] # remove last semicolon
            self._msgs_to_send.append( ( self.time(), msg ) )

    def request_consumed_energy(self):
        self._msgs_to_send.append( ( self.time(), "E" ) )

    def change_pstates(self, pstates_to_change):
        """
        should be [ (new_pstate, (first_node, last_node)),  ...]
        """
        if len(pstates_to_change) > 0:
            parts = []
            for (new_pstate, (ps, pe)) in pstates_to_change:
                if ps == pe:
                    parts.append( str(ps) + "=" + str(new_pstate))
                else:
                    parts.append( str(ps)+"-"+str(pe) + "=" + str(new_pstate))
            for part in parts:
                self._msgs_to_send.append( ( self.time(), "P:" + part ) )

    def change_pstate_merge(self, new_pstate, first_node, last_node):
        """
        if the previous call of change_pstate_merge had the same new_pstate and old.last_node+1 == new.first_node,
        then we merge the requests.
        """
        part = None
        if len(self._msgs_to_send)>0:
            last_msg = self._msgs_to_send[-1]
            if last_msg[0] == self.time():
                resInter = re.split("P:([0-9]+)-([0-9]+)=([0-9]+)", last_msg[1])
                resUniq = re.split("P:([0-9]+)=([0-9]+)", last_msg[1])
                if len(resInter) == 5 and int(resInter[3]) == new_pstate and int(resInter[2])+1 == first_node:
                    self._msgs_to_send.pop(-1)
                    part = str(resInter[1])+"-"+str(last_node) + "=" + str(new_pstate)
                elif len(resUniq) == 4 and int(resUniq[2]) == new_pstate and int(resUniq[1])+1 == first_node:
                    self._msgs_to_send.pop(-1)
                    part = str(resUniq[1])+"-"+str(last_node) + "=" + str(new_pstate)
        if part is None:
            part = str(first_node)+"-"+str(last_node) + "=" + str(new_pstate)

        self._msgs_to_send.append( ( self.time(), "P:" + part ) )


    def do_next_event(self):
        return self._read_bat_msg()

    def start(self):
        cont = True
        while cont:
            cont = self.do_next_event()

    def _time_to_str(self,t):
        return('%.*f' % (6, t))

    def _read_bat_msg(self):
        lg_str = self._connection.recv(4)

        if not lg_str:
            print("[BATSIM]: connection is closed by batsim core")
            return False

        lg = struct.unpack("I",lg_str)[0]
        msg = self._connection.recv(lg).decode()
        if self.verbose > 0: print('[BATSIM]: from batsim (%r) : %r' % (lg, msg))
        sub_msgs = msg.split('|')
        data = sub_msgs[0].split(":")
        version = int(data[0])
        self.last_msg_recv_time = float(data[1])
        self._current_time = float(data[1])

        if self.verbose > 1: print("[BATSIM]: version: %r  now: %r" % (version, self.time()))

        # [ (timestamp, txtDATA), ...]
        self._msgs_to_send = []

        for i in range(1, len(sub_msgs)):
            data = sub_msgs[i].split(':')
            if data[1] == 'R':
                self.scheduler.onJobRejection()
            elif data[1] == 'N':
                self.scheduler.onNOP()
            elif data[1] == 'S':
                self.scheduler.onJobSubmission(self.jobs[int(data[2])])
                self.nb_jobs_recieved += 1
            elif data[1] == 'C':
                j = self.jobs[int(data[2])]
                j.finish_time = float(data[0])
                self.scheduler.onJobCompletion(j)
            elif data[1] == 'p':
                opts = data[2].split('=')
                nodes = opts[0].split("-")
                if len(nodes) == 1:
                    nodeInterval = (int(nodes[0]), int(nodes[0]))
                elif len(nodes) == 2:
                    nodeInterval = (int(nodes[0]), int(nodes[1]))
                else:
                    raise False, "Not supported"
                self.scheduler.onMachinePStateChanged(nodeInterval, int(opts[1]))
            elif data[1] == 'e':
                consumed_energy = float(data[2])
                self.scheduler.onReportEnergyConsumed(consumed_energy)
            elif data[1] == 'J' or data[1] == 'P' or data[1] == 'E':
                raise "Only the server can receive this kind of message"
            else:
                raise Exception("Unknow submessage type " + data[1] )

        msg = "0:" + self._time_to_str(self.last_msg_recv_time) + "|"
        if len(self._msgs_to_send) > 0:
            #sort msgs by timestamp
            self._msgs_to_send = sorted(self._msgs_to_send, key=lambda m: m[0])
            for m in self._msgs_to_send:
                msg += self._time_to_str(m[0])+":"+m[1]+"|"
            msg = msg[:-1]#remove the last "|"
        else:
            msg +=  self._time_to_str(self.time()) +":N"

        if self.verbose > 0: print("[BATSIM]:  to  batsim : %r" % msg)
        lg = struct.pack("i",int(len(msg)))
        self._connection.sendall(lg)
        self._connection.sendall(msg.encode())
        return True



    def _load_json_workload_profile(self, filename):
        wkp_file = open(filename)
        wkp = json.load(wkp_file)

        self.nb_res = wkp["nb_res"]
        self.jobs = {j["id"]: Job(j["id"], j["subtime"], j["walltime"], j["res"], j["profile"]) for j in wkp["jobs"]}
        #TODO: profiles


class Job(object):
    def __init__(self, id, subtime, walltime, res, profile):
        self.id = id
        self.submit_time = subtime
        self.requested_time = walltime
        self.requested_resources = res
        self.profile = profile
        self.finish_time = None#will be set on completion by batsim
    def __repr__(self):
        return("<Job {0}; sub:{1} res:{2} reqtime:{3} prof:{4}>".format(self.id, self.submit_time, self.requested_resources, self.requested_time, self.profile))
    #def __eq__(self, other):
        #return self.id == other.id
    #def __ne__(self, other):
        #return not self.__eq__(other)


class BatsimScheduler(object):
    def __init__(self, options):
        self.options = options

    def onAfterBatsimInit(self):
        #You now have access to self.bs and all other functions
        pass
    def onJobRejection(self):
        raise "not implemented"
    def onNOP(self):
        raise "not implemented"
    def onJobSubmission(self, job):
        raise "not implemented"
    def onJobCompletion(self, job):
        raise "not implemented"
    def onMachinePStateChanged(self, nodeid, pstate):
        raise "not implemented"
    def onReportEnergyConsumed(self, consumed_energy):
        raise "not implemented"

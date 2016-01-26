#/usr/bin/python3

from batsim.batsim import BatsimScheduler
from schedulers.common_pyss_adaptator import CpuSnapshot

import sys
import os
from random import sample
from sortedcontainers import SortedSet
from sortedcontainers import SortedListWithKey
import copy


"""
An Easy Backfill scheduler that care a little about topology.
This scheduler consider job as rectangle.


"""




INFINITY = float('inf')


class FreeSpace(object):
    def __init__(self, first_res, last_res, len, p, n):
        self.first_res = first_res
        self.last_res = last_res
        self.res = last_res-first_res+1
        self.length = len
        self.prev = p
        self.nextt = n

class FressSpaceContainer(object):
    """
    Developers, NEVER FORGET:
    - operations on the list can be done will iterating on it with the generator()
    - when on BF mode, 2 consecutives items can have the same first_res or last_res.
    """
    def __init__(self, total_processors):
        self.firstItem = FreeSpace(0, total_processors-1, INFINITY, None, None)

    def generator(self):
        curit = self.firstItem
        while curit is not None:
            yield curit
            curit = curit.nextt

    def remove(self, item):
        prev = item.prev
        nextt = item.nextt
        if item == self.firstItem:
            self.firstItem = nextt
        else:
            prev.nextt = nextt
        if nextt is not None:
            nextt.prev = prev
    
    def _assignJobBeginning(self, l, job):
            alloc = range(l.first_res, l.first_res+job.requested_resources)
            l.first_res = l.first_res+job.requested_resources
            l.res = l.last_res-l.first_res+1
            assert l.res>=0
            if l.res == 0:
                self.remove(l)
            return alloc
    
    def _assignJobEnding(self, l, job):
            alloc = range(l.last_res-job.requested_resources+1, l.last_res+1)
            l.last_res = l.last_res-job.requested_resources
            l.res = l.last_res-l.first_res+1
            assert l.res>=0
            if l.res == 0:
                self.remove(l)
            return alloc
    
    def assignJob(self, l, job, current_time):
        assert job.requested_resources <= l.res
        #we try to first alloc jobs on the side of the cluster to reduce fragmentation
        if l.prev is None:
            alloc = self._assignJobBeginning(l, job)
        elif l.nextt is None:
            alloc = self._assignJobEnding(l, job)
        #then, we try to alloc near other job to reduce fragmentation
        elif l.first_res -1 == l.prev.last_res:
            alloc = self._assignJobBeginning(l, job)
        elif l.last_res +1 == l.nextt.first_res:
            alloc = self._assignJobEnding(l, job)
        else:
            #alloc wherever (a good optimisation would be to alloc close to the job that finish close to the finish time of <job>)
            alloc = self._assignJobBeginning(l, job)
        job.finish_time = job.requested_time + current_time
        job.alloc = alloc
        return alloc

    def _findSurroundingFreeSpaces(self, job):
        prev_fspc = None
        for fspc in self.generator():
            if fspc.first_res > job.alloc[0]:
                return (prev_fspc, fspc)
            prev_fspc = fspc
        #prev_fspc = last fspc
        return (prev_fspc, None)
        
    def unassignJob(self, job):
        (l1, l2) = self._findSurroundingFreeSpaces(job)
        
        #merge with l1?
        mergel1 = ((l1 is not None) and (l1.last_res+1 == job.alloc[0]))
        mergel2 = ((l2 is not None) and (l2.first_res-1 == job.alloc[-1]))
        if mergel1 and mergel2:
            #merge l2 into l1
            l1.nextt = l2.nextt
            if l1.nextt is not None:
                l1.nextt.prev = l1
            l1.last_res = l2.last_res
            l1.res = l1.last_res-l1.first_res+1
            return l1
        elif mergel1:
            #increase l1 size
            l1.last_res = l1.last_res + job.requested_resources
            l1.res = l1.last_res-l1.first_res+1
            return l1
        elif mergel2:
            #increase l2 size
            l2.first_res = l2.first_res - job.requested_resources
            l2.res = l2.last_res-l2.first_res+1
            return l2
        else:
            #create a new freespace
            lnew = FreeSpace(job.alloc[0], job.alloc[-1], INFINITY, l1, l2)
            
            if l1 is None:
                self.firstItem = lnew
            else:
                l1.nextt = lnew
            if l2 is not None:
                l2.prev = lnew
            return lnew
        assert False
        
    def printme(self):
        print self
        print "-------------------"
        for l in self.generator():
            print "["+str(l.first_res)+"-"+str(l.last_res)+"] "+str(l.length)
        print "-------------------"


    def insertNewFreeSpaceBefore(first_res, last_res, len, l):
        newfs = FreeSpace(first_res, last_res, len, l.prev, l)
        
        if l.prev is None:
            self.firstItem = newfs
        else:
            l.prev.nextt = newfs
        l.prev = newfs
        
        return newfs





class EasyBackfill(BatsimScheduler):
    """
    An EASY backfill scheduler that schedule rectangles.
    """

    def __init__(self):
        pass

    def onAfterBatsimInit(self):
        self.listFreeSpace = FressSpaceContainer(self.bs.nb_res)

        self.listRunningJob = SortedListWithKey(key=lambda job: job.finish_time)
        self.listWaitingJob = []



    def onJobSubmission(self, just_submitted_job):
        current_time = self.bs.time()
        self.listWaitingJob.append(just_submitted_job)
        #if (self.cpu_snapshot.free_processors_available_at(current_time) >= just_submitted_job.requested_resources):
        self._schedule_jobs(current_time)

    def onJobCompletion(self, job):
        current_time = self.bs.time()
        
        self.listFreeSpace.unassignJob(job)
        self._removeAjobFromRunningJob(job)
        job.finish_time = current_time
        
        self._schedule_jobs(current_time)

    def _removeAjobFromRunningJob(self, job):
        #because we use "key", .remove does not work as intended
        for (i,j) in zip(range(0, len(self.listRunningJob)), self.listRunningJob):
            if j == job:
                del self.listRunningJob[i]
                return
        raise "Job not found!"
        

    def _schedule_jobs(self, current_time):
        
        allocs = self.allocHeadOfList(current_time)
        
        if len(self.listWaitingJob) > 1:
            first_job = self.listWaitingJob.pop(0)
            allocs += self.allocBackFill(first_job, current_time)
            self.listWaitingJob.insert(0, first_job)
        
        if len(allocs) > 0:
            res = {}
            scheduledJobs = []
            for (job, alloc) in allocs:
                res[job.id] = alloc
                job.start_time = current_time#just to be sure
                scheduledJobs.append(job)
            self.bs.start_jobs(scheduledJobs, res)


    def allocJobWithoutTime(self, job, current_time):
        for l in self.listFreeSpace.generator():
            if job.requested_resources <= l.res:
                alloc = self.listFreeSpace.assignJob(l, job, current_time)
                return alloc
        return None

    def allocJobWithTime(self, job, current_time):
        for l in self.listFreeSpace.generator():
            if job.requested_resources <= l.res and job.requested_time <= l.length:
                alloc = self.listFreeSpace.assignJob(l, job, current_time)
                return alloc
        return None


    def allocHeadOfList(self, current_time):
        allocs = []
        while len(self.listWaitingJob) > 0:
            alloc = self.allocJobWithoutTime(self.listWaitingJob[0], current_time)
            if alloc is None:
                break
            job = self.listWaitingJob.pop(0)
            self.listRunningJob.add(job)
            allocs.append( (job, alloc) )
        return allocs


    def findAllocFuture(self, job):
        #rjobs = sort(self.listRunningJob, by=finish_time) automaticly done by SortedList
        listFreeSpaceTemp = copy.deepcopy(self.listFreeSpace)
        for j in self.listRunningJob:
            new_free_space_created_by_this_unallocation = listFreeSpaceTemp.unassignJob(j)
            if job.requested_resources <= new_free_space_created_by_this_unallocation.res:
                alloc = listFreeSpaceTemp.assignJob(new_free_space_created_by_this_unallocation, job, j.finish_time)
                #we find a valid allocation
                return (alloc, j.finish_time)
        #if we are it means that the job will never fit in the cluster!
        assert False
        return None


    def allocFutureJob(self, first_job_res, first_job_starttime, current_time):
        """
        Update self.listFreeSpace to insert (if needed) 2 virtual free space. These freespaces need to be removes afterwards
        Example: 3 machine (A,B,C), 1 job running (1), the firstjob is 2
        A|
        B|     22
        C|1111122
        -'------------>
        A FreeSpace (A,B, INFINITY) exists, we replace it with 2 FreeSpaces (A,A, INFINITY) and (A,B,first_job_starttime)
        
        
        TODO: here we can optimise the code. The free sapces that we are looking for are already been found in findAllocFuture(). BUT, in this function we find the FreeSpaces of listFreeSpaceTemp not self.listFreeSpace; so a link should be made betweend these 2 things.
        """
        first_virtual_space = None
        first_shortened_space = None
        second_virtual_space = None
        second_shortened_space = None
        for l in self.listFreeSpace.generator():
            if l.first_res == first_job_res[0]:
                assert first_virtual_space is None and first_shortened_space is None
                
                first_shortened_space = l
                l.length = first_job_starttime-current_time
                
            elif l.first_res < first_job_res[0] and l.last_res >= first_job_res[0]:
                #we transform this free space as 2 free spaces, the wider rectangle and the longest rectangle
                assert first_virtual_space is None and first_shortened_space is None
                
                first_virtual_space = self.listFreeSpace.insertNewFreeSpaceBefore(l.first_res, first_job_res[0]-1, INFINITY, l)
                
                first_shortened_space = l
                l.length = first_job_starttime-current_time
            
            if l.last_res == first_job_res[-1]:
                assert second_virtual_space is None and second_shortened_space is None
                
                second_shortened_space = l
                l.length = first_job_starttime-current_time
            elif l.first_res <= first_job_res[-1] and l.last_res > first_job_res[-1]:
                #we transform this free space as 2 free spaces, the wider rectangle and the longest rectangle
                assert second_virtual_space is None and second_shortened_space is None
                
                second_virtual_space = self.listFreeSpace.insertNewFreeSpaceBefore(first_job_res[-1]+1, l.last_res, INFINITY, l)
                
                second_shortened_space = l
                l.length = first_job_starttime-current_time
                #no need to continue
                break
        return (first_virtual_space, first_shortened_space, second_virtual_space, second_shortened_space)
        


    def allocBackFill(self, first_job, current_time):
        
        allocs = []
        
        (first_job_res, first_job_starttime) = self.findAllocFuture(first_job)
        
        (first_virtual_space, first_shortened_space, second_virtual_space, second_shortened_space) = self.allocFutureJob(first_job_res, first_job_starttime, current_time)
        
        for j in self.listWaitingJob:
            alloc = self.allocJobWithTime(j, current_time)
            if alloc is not None:
                allocs.append( (j, alloc) )
                self.listWaitingJob.remove(j)
                self.listRunningJob.add(j)
        
        if first_virtual_space is not None:
            self.listFreeSpace.remove(first_virtual_space)
        if first_shortened_space is not None:
            first_shortened_space.length = INFINITY
        if second_virtual_space is not None:
            self.listFreeSpace.remove(second_virtual_space)
        if second_shortened_space is not None:
            second_shortened_space.length = INFINITY
        
        
        return allocs



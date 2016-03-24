


class Interval(object):
    def __init__(self, begin, end, prev, next):
        assert begin<=end
        self.begin = begin
        self.end = end
        self.prev = prev
        self.next = next
    def __repr__(self):
        return "<Interval ["+str(self.begin)+"-"+str(self.end)+"]>"


class IntervalContainer(object):
    def __init__(self):
        self.first_item = None
    
    def addInterval(self, rmFrom, rmTo):
        newInter = Interval(rmFrom,rmTo,None,None)
        prevnextitem = None
        nextitem = self.first_item
        while not(nextitem is None):
            if newInter.begin <= nextitem.begin:
                break
            prevnextitem = nextitem
            nextitem = nextitem.next

        if prevnextitem is None:
            self.first_item = newInter
            newInter.prev = None
        else:
            if prevnextitem.end+1 >= newInter.begin:
                prevnextitem.end = max(prevnextitem.end, newInter.end)
                newInter = prevnextitem
            else:
                newInter.prev = prevnextitem
                prevnextitem.next = newInter
        
        
        if nextitem is None:
            newInter.next = None
        else:
            assert not(nextitem.next is not None and nextitem.next.begin-1 <= newInter.end), "NOT IMPLEMENTED"

            if nextitem.begin-1 <= newInter.end:
                nextitem.begin = newInter.begin
                nextitem.end = max(nextitem.end, newInter.end)
                nextitem.prev = newInter.prev
                if not(nextitem.prev is None):
                    nextitem.prev.next = nextitem
                else:
                    self.first_item = nextitem
            else:
                nextitem.prev = newInter
                newInter.next = nextitem
        
    def removeInterval(self, rmFrom, rmTo):
        """
        remove from rmFrom to rmTo INCLUDED:
        removing (5,7) in (1,10) will produce (1,4),(8,10)
        """
        assert rmFrom <= rmTo
        item = self.first_item
        while not(item is None):
            if rmFrom <= item.begin and item.end <= rmTo:
                #remove item
                if item.prev is None:
                    self.first_item = item.next
                else:
                    item.prev.next = item.next
                if item.next is None:
                    pass
                else:
                    item.next.prev = item.prev
            elif  item.begin < rmFrom and rmTo < item.end:
                #split item
                newInter = Interval(rmTo+1, item.end, item, item.next)
                if not(newInter.next is None):
                    newInter.next.prev = newInter
                item.next = newInter
                item.end = rmFrom-1
                return
            elif rmFrom <= item.begin and item.begin <= rmTo:
                item.begin = rmTo+1
                return
            elif rmFrom <= item.end and item.end <= rmTo:
                item.end = rmFrom-1
            
            item = item.next
    
    
    
    def intersection(self, rmFrom, rmTo):
        """
        intersection from rmFrom to rmTo INCLUDED
        """
        assert rmFrom <= rmTo
        item = self.first_item
        ret = []
        while not(item is None):
            if rmFrom <= item.begin and item.end <= rmTo:
                ret.append( (item.begin, item.end) )
            elif  item.begin < rmFrom and rmTo < item.end:
                ret.append( (rmFrom, rmTo) )
                return ret
            elif rmFrom <= item.begin and item.begin <= rmTo:
                ret.append( (item.begin, rmTo) )
                return ret
            elif rmFrom <= item.end and item.end <= rmTo:
                ret.append( (rmFrom, item.end) )
            
            item = item.next
        return ret
    
    
    def difference(self, rmFrom, rmTo):
        """
        return the part of (rmFrom, rmTo) that is not in the container
        TODO: return a listof differences
        """
        assert rmFrom <= rmTo
        prevnextitem = None
        nextitem = self.first_item
        while not(nextitem is None):
            if rmFrom <= nextitem.begin:
                break
            prevnextitem = nextitem
            nextitem = nextitem.next

        if not(prevnextitem is None):
            assert rmFrom >= prevnextitem.begin, str(("notintersection across multiple intervals not supported", rmFrom, prevnextitem.begin))
            rmFrom = max(rmFrom, prevnextitem.end+1)
        if not(nextitem is None):
            assert rmTo <= nextitem.end, str(("notintersection across multiple intervals not supported", rmTo, nextitem.end))
            rmTo = min(rmTo, nextitem.begin-1)
        
        if rmTo < rmFrom:
            return None
        else:
            return (rmFrom, rmTo)
        



    def printme(self):
        item = self.first_item
        while not(item is None):
            print item
            item = item.next




if __name__ == '__main__':
    l = IntervalContainer()
    l.printme()
    print "---------------"
    l.addInterval(10,20)
    l.addInterval(30,40)
    l.addInterval(50,60)
    l.printme()
    print l.difference(15, 25)
    print "---------------"
    print l.intersection(10,30)
    print l.intersection(100,300)
    l.removeInterval(10,20)
    l.printme()
    print "---------------"

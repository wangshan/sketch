from threading import Thread
from threading import Event
 
# Based on the promise created here:
# https://weblogs.asp.net/wesleybakker/asynchronous-programming-with-python-using-promises
# Note this promise is not promise A+ standard conforming
# https://promisesaplus.com

class Future(object):
    def __init__(self):
        self._event = Event()
        self._rejected = False
        self._result = None

 
    def resolve(self, value):
        self._rejected = False
        self._result = value
        self._event.set()

 
    def reject(self, reason):
        self._rejected = True
        self._result = reason
        self._event.set()

 
    def promise(self, within=300):
        promise = Promise(self, within)
        return promise
 

 
class Promise(object):
    def __init__(self, future, within=300):
        self._future = future
        self._timeout = within
 

    def then(self, resolved_func=None, rejected_func=None):
        # this future is used for chaining promises
        future = Future()
 
        def task():
            try:
                self.wait(self._timeout)  # seconds
                self._future._event.wait()
                if self._future._rejected:
                    result = self._future._result
                    if rejected_func:
                        result = rejected_func(self._future._result)
                        
                    future.reject(result)
                else:
                    result = self._future._result
 
                    if resolved_func:
                        result = resolved_func(self._future._result)
 
                    future.resolve(result)
            except Exception as e:
                if rejected_func:
                    rejected_func(e)
                future.reject(e.message)

 
        #Thread(target=task).start()
        task()
 
        return future.promise()
 

    def wait(self, timeout=None):
        self._future._event.wait(timeout)

 
#    @staticmethod
#    def wait_all(*args):
#        for promise in args:
#            promise.wait()


# use the promise:

def do_promised():
    # if inherit from Future then don't need this
    future = Future()
    try:
        t = Thread(target=run, args=(future))
        t.start()
    except ThreadError as e:
        t.join(10)
        future.reject("failed")

    return future.promise()

def run(future):
    result = do_something()
    future.resolve(result)

p = do_promised()
# do other things...
p.then(lambda x: print x)


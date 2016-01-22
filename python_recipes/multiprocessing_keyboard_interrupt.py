import signal
import time
import multiprocessing


def handle_KeyboardInterrupt():
    signal.signal(signal.SIGINT, signal.SIG_IGN)

def doSomething(number):
    time.sleep(1)
    print "received {0}".format(number)

class Worker(object):
    def __init__(self, poolSize):
        self.pool = multiprocessing.Pool(poolSize, handle_KeyboardInterrupt)

    def clean(self):
        self.pool.close()
        self.pool.join()

    def work(self):
        try:
            self.pool.map_async(doSomething, range(8)).get(timeout=10)
        except multiprocessing.TimeoutError as e:
            print "Timeout"
        except KeyboardInterrupt:
            print "KeyboardInterrupt, user terminated the process"
            self.pool.terminate()
            self.pool.join()

        print "Done"


if __name__ == '__main__':
    worker = Worker(2)
    worker.work()
    worker.clean()

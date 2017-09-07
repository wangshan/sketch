import time
def latency_recorder(func):
    def wrapper(*args, **kwargs):
        start = time.time()
        res = func(*args, **kwargs)
        latency = time.time() - start
        self = args[0] if args[0] else object
        name = self.__calss__.__name__ + "." + func.__name__
        logging.info("latency in {0}: {1} seconds".format(name, latency))
        return res
    return wrapper


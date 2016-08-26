#!/usr/bin/env python

import zmq
import numpy as np
import time

cfg = {}
with open("common.py") as f:
    code = compile(f.read(), "common.py", 'exec')
    exec(code, cfg)

zmqContext = zmq.Context()
zmqSocket = zmqContext.socket(zmq.REQ)
zmqSocket.connect(cfg["to_connect"])

print("Connected to "+cfg["to_connect"])

array = np.random.randint(256, size=5*5*3, dtype="uint8").reshape((5,5,3))
array_d = np.random.rand(5,5,1).astype("float32")*50
nb_it = 100.0
i=0
start = time.time()
while i<nb_it:
    print(array.shape)
    md = dict(
        arrayname = "josette",
        dtype = str(array.dtype),
        shape = array.shape,
        timer = time.time()
    )
    zmqSocket.send_json(md, zmq.SNDMORE)
    zmqSocket.send(array, flags=zmq.SNDMORE, copy=False)
    zmqSocket.send(array_d, copy=False)
    print("sending done")

    md_res = zmqSocket.recv_json()
    print(md_res)
    dat = zmqSocket.recv()
    A = np.frombuffer(dat, dtype=md_res['dtype'])
    A = A.reshape(md_res['shape'])

    print("Received "+str(i))
    print(A.shape)
    i+=1
start = time.time() - start
print("Took: "+str(start/nb_it))

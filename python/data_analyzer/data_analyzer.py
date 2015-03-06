__author__ = 'jhammers'

import numpy as np
import scipy
import matplotlib.pyplot as plt
import json
import itertools
import zipfile
import os
# from timer import Timer

def load_json(path, is_zip = True):
    if is_zip:
        z = zipfile.ZipFile(path)
        f = z.open(z.infolist()[0])
        return json.loads(f.read().decode())
    else:
        f = open(path)
        return json.loads(f)

def speed_analyze():
    s = load_json("/Users/jhammers/test.json.zip")
    s = s["speed"]
    plt.plot(s)
    plt.show()
    #print(s)

def log_analyze(filename):
    #log = load_json("/Users/jhammers/EcoSonic/2015-02-06_13-58/59-12.json.zip")
    #log = load_json("/Users/jhammers/EcoSonic/2015-02-16_20-27/30-23.json.zip")
    log = load_json(filename)


    #print(log)
    items = log["items"]
    x = range(len(items))


    # throttle = [i["throttle"] for i in items]
    speed = [i["speed"] for i in items]
    pos = [i["position"] for i in items]
    dt = [i["dt"] for i in items]
    t = list(itertools.accumulate(dt))

    #plt.hist(dt, 100)
    plt.plot(pos,speed)

    s = load_json("/Users/jhammers/test.json.zip")
    s = s["speed"]
    plt.plot([2*x for x in range(0,len(s))], s)

    plt.show()

def log_analyze_all(folder):
    for file in os.listdir(folder):
        if file.endswith("json.zip"):
            print(file)
            log_analyze(folder+file)

log_analyze_all("/Users/jhammers/Dropbox/VP1001/")
#speed_analyze()
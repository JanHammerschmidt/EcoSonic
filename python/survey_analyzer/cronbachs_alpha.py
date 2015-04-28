import numpy as np

def CronbachAlpha(itemscores):
    itemscores = np.asarray(itemscores)
    itemvars = itemscores.var(axis=1, ddof=1)
    tscores = itemscores.sum(axis=0)
    nitems = len(itemscores)

    return nitems / (nitems-1.) * (1 - itemvars.sum() / tscores.var(ddof=1))

def svar2(X):
    n = float(len(X))
    svar=(sum([(x-np.mean(X))**2 for x in X]) / n)* n/(n-1.)
    return svar


def CronbachAlpha3(itemscores):
    itemvars = [svar2(item) for item in itemscores]
    tscores = [0] * len(itemscores[0])
    for item in itemscores:
       for i in range(len(item)):
          tscores[i]+= item[i]
    nitems = len(itemscores)
    print("total scores=", tscores, 'number of items=', nitems)

    Calpha=nitems/(nitems-1.) * (1-sum(itemvars)/ svar2(tscores))

    return Calpha

def mean(X):
    return sum(X)/float(len(X))

def pvar(X):
    xbar = mean(X)
    return sum([(x-xbar)**2 for x in X]) / float(len(X))

def svar(X):
    n = len(X)
    return pvar(X) * n/(n-1.0)


def CronbachAlpha2(itemscores):
    itemvars = [svar(item) for item in itemscores]
    sigmai2 = sum(itemvars)

    totscores = [0] * len(itemscores[0])
    for item in itemscores:
       for j in range(len(item)):
          totscores[j]+= item[j]
    print("totscores=", totscores)
    sigma2 = svar(totscores)

    nT = len(itemscores)

    return nT/(nT-1.0) * (1-sigmai2/ sigma2)


def Test():
    itemscores = [[ 3,4,3,3,3,4,2,3,3,3],
              [ 5,4,4,3,4,5,5,4,5,3],
              [ 1,3,4,5,5,5,5,4,4,2],
              [ 4,5,4,2,4,3,3,2,4,3],
              [ 1,3,4,1,3,2,4,4,3,2]]
    print("Cronbach alpha = ", CronbachAlpha(itemscores))
    print("Cronbach alpha 2 = ", CronbachAlpha2(itemscores))
    print("Cronbach alpha 3 = ", CronbachAlpha3(itemscores))
    itemscores = [[ 4,14,3,3,23,4,52,3,33,3],
                  [ 5,14,4,3,24,5,55,4,15,3]]
    print("Cronbach alpha = ", CronbachAlpha(itemscores))
    print("Cronbach alpha 2 = ", CronbachAlpha2(itemscores))
    print("Cronbach alpha 3 = ", CronbachAlpha3(itemscores))

if __name__== "__main__":
   Test()
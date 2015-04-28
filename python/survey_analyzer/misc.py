import matplotlib.pyplot as plt
import numpy as np

def all_identical(list):
    return len(set(list)) <= 1

def show_dependency_matrix(mat, row_labels, col_labels, show = True):
    #print(mat)
    im = plt.matshow(mat,cmap=plt.cm.seismic, vmin = -1, vmax = 1)
    nrows, ncols = len(row_labels), len(col_labels)
    plt.yticks(range(nrows), row_labels)
    plt.xticks(range(ncols), col_labels)
    for label in im.axes.xaxis.get_ticklabels():
        label.set_rotation(90)
    x, y = np.meshgrid(np.arange(ncols), np.arange(nrows))
    for i, (x_val, y_val) in enumerate(zip(x.flatten(), y.flatten())):
        row, col = i // ncols, i % ncols
        c = "{:.3f}".format(mat[row,col])
        plt.text(x_val, y_val, c, va='center', ha='center')
    if show:
        plt.show()

class lazy_property(object):
    '''
    meant to be used for lazy evaluation of an object attribute.
    property should represent non-mutable data, as it replaces itself.
    '''

    def __init__(self,fget):
        self.fget = fget
        self.func_name = fget.__name__


    def __get__(self,obj,cls):
        if obj is None:
            return None
        value = self.fget(obj)
        setattr(obj,self.func_name,value)
        return value

# code from http://stackoverflow.com/questions/7450957/how-to-implement-rs-p-adjust-in-python
# also see: http://de.wikipedia.org/wiki/Bonferroni-Methode
def correct_pvalues_for_multiple_testing(pvalues, correction_type = "Benjamini-Hochberg"):
    """
    consistent with R - print correct_pvalues_for_multiple_testing([0.0, 0.01, 0.029, 0.03, 0.031, 0.05, 0.069, 0.07, 0.071, 0.09, 0.1])
    """
    from numpy import array, empty
    pvalues = array(pvalues)
    n = float(pvalues.shape[0])
    new_pvalues = empty(n)
    if correction_type == "Bonferroni":
        new_pvalues = n * pvalues
    elif correction_type == "Bonferroni-Holm":
        values = [ (pvalue, i) for i, pvalue in enumerate(pvalues) ]
        values.sort()
        for rank, vals in enumerate(values):
            pvalue, i = vals
            new_pvalues[i] = (n-rank) * pvalue
    elif correction_type == "Benjamini-Hochberg":
        values = [ (pvalue, i) for i, pvalue in enumerate(pvalues) ]
        values.sort()
        values.reverse()
        new_values = []
        for i, vals in enumerate(values):
            rank = n - i
            pvalue, index = vals
            new_values.append((n/rank) * pvalue)
        for i in range(0, int(n)-1):
            if new_values[i] < new_values[i+1]:
                new_values[i+1] = new_values[i]
        for i, vals in enumerate(values):
            pvalue, index = vals
            new_pvalues[index] = new_values[i]
    return new_pvalues
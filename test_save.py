import sdftools as st
import numpy as np

#Generating 1D sdf file
shape = np.array([100])

def func1D(x):
    return np.exp(-0.5*x*x)

coords = np.linspace(0.0, 10.0, shape[0])
data = func1D(coords)

st.save_bbox_sdf('test1D.sdf', [coords], data)

#Generating 2D sdf file in two ways
#full function saves all coordinates
#bbox function only saves the bounding box coordinates
#i.e in 2D case for a box where x lies in [0, 1] and y lies in [0, 1] it will save the box [[0, 1], [0, 1]]

shape = np.array([30, 20])
coord_box = np.array([0.0, 10.0, 0.0, 5.0])

def func2D(x):
     return np.exp(-0.5*x[0]*x[0] - x[1]*x[1])

#coordinate arrays
coord_x = np.linspace(coord_box[0], coord_box[1], shape[0])
coord_y = np.linspace(coord_box[2], coord_box[3], shape[1])

coord_pairs = np.vstack([np.repeat(coord_x, shape[1]),np.tile(coord_y, shape[0])]).reshape(np.append([2], shape))
#data array with a shape (100, 75)
data = func2D(coord_pairs)

st.save_sdf('test2D_1.sdf', [coord_x, coord_y], data, time=1.0, cnames=['z','w'])
st.save_bbox_sdf('test2D_2.sdf', [coord_x, coord_y], data)
import sdftools as st
import numpy as np

import matplotlib.pyplot as plt

#Loading 1D sdf file
sdf = st.load_sdf('test1D.sdf')

print("SDF time point is: " + str(sdf.time))
print("SDF coordinates are: " + str(sdf.cnames))
plt.plot(sdf.coords, sdf.data)
plt.show()

#Loading 2D sdf file
sdf = st.load_sdf('test2D_1.sdf', level=1)

xfunc, yfunc = np.meshgrid(sdf.coords[0], sdf.coords[1], indexing='ij')

print("SDF time point is: " + str(sdf.time))
print("SDF coordinates are: " + str(sdf.cnames))
ax = plt.axes(projection='3d')
ax.plot_surface(yfunc, xfunc, sdf.data, rstride=1, cstride=1, cmap='jet', edgecolor='none')
ax.view_init(45, 45)
plt.show()
import ctypes
import numpy.ctypeslib as ctl
import numpy as np

#The functions loaded are directly bbhutil routines

lib_path = './'
lib = ctl.load_library('routines_lib.so', lib_path)

# Reads number of dimensions
# gft_read_rank(const char *gf_name, const int level, int *rank);

gft_read_rank = lib.gft_read_rank
gft_read_rank.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.POINTER(ctypes.c_int)]
gft_read_rank.restype = ctypes.c_int

# Reads dimension sizes
# gft_read_shape(const char *gf_name, const int level, int *shape);

gft_read_shape = lib.gft_read_shape
gft_read_shape.argtypes = [ctypes.c_char_p, ctypes.c_int, ctl.ndpointer(np.int32, flags='aligned, c_contiguous')]
gft_read_shape.restype = ctypes.c_int

# Reads full sdf file
# int gft_read_full(const char *gf_name, int level, int *shape, char *cnames, int rank, double *time, double *coords, double *data);

gft_read_full = lib.gft_read_full
gft_read_full.argtypes = [ctypes.c_char_p, ctypes.c_int, 
								ctl.ndpointer(np.int32, flags='aligned, c_contiguous'), 
								ctypes.POINTER(ctypes.ARRAY(ctypes.c_char, 10)), 
								ctypes.c_int, ctypes.POINTER(ctypes.c_double),
								ctl.ndpointer(np.float64, flags='aligned, c_contiguous'), 
								ctl.ndpointer(np.float64, flags='aligned, c_contiguous')]
gft_read_shape.restype = ctypes.c_int

# Writes data stored in a bounding box
# int gft_out_bbox(const char *func_name, double time, int *shape, int rank, double *coords, double *data);

gft_out_bbox = lib.gft_out_bbox
gft_out_bbox.argtypes = [ctypes.c_char_p, ctypes.c_double,
							ctl.ndpointer(np.int32, flags='aligned, c_contiguous'),
							ctypes.c_int,
							ctl.ndpointer(np.float64, flags='aligned, c_contiguous'), 
							ctl.ndpointer(np.float64, flags='aligned, c_contiguous')]
gft_out_bbox.restype = ctypes.c_int

# Writes data in the full format
# int gft_out_full(const char *func_name, double time, int *shape, char *cnames, int rank, double *coords, double *data);

gft_out_full = lib.gft_out_full
gft_out_full.argtypes = [ctypes.c_char_p, ctypes.c_double,
							ctl.ndpointer(np.int32, flags='aligned, c_contiguous'),
							ctypes.c_char_p, ctypes.c_int,
							ctl.ndpointer(np.float64, flags='aligned, c_contiguous'), 
							ctl.ndpointer(np.float64, flags='aligned, c_contiguous')]
gft_out_full.restype = ctypes.c_int

#Here we introduce some Python API

#This class is used for loading sdf files

#This class has optional arguments:
#							level is the level which is loaded from an sdf file
#This class has elements:
#							rank is the number of dimensions
#							shape is a numpy array with dimension lengths
#							time is the saved point in time
#							cnames are the coordinate names
#							coords is a list of numpy arrays with coordinate values along each dimension
#							data is n-dimensional numpy array with data values

class load_sdf(object):
    def __init__(self, name, **kwargs):
        if 'level' in kwargs:
            self.level = kwargs['level']
        else:
            self.level = 1
        
        rank = ctypes.c_int()
        if gft_read_rank(name.encode(), self.level, ctypes.byref(rank)) != 1:
            raise Exception("gft_read_rank failed")
        self.rank = rank.value
        
        self.shape = np.zeros(self.rank, dtype=np.int32)
        if gft_read_shape(name.encode(), self.level, self.shape) != 1:
            raise Exception("gft_read_shape failed")
        
        time = ctypes.c_double()
        self.coords = np.zeros(np.sum(self.shape), dtype=np.float64)
        self.data = np.zeros(np.prod(self.shape), dtype=np.float64)
        
        cnames = (ctypes.c_char*10)()
        if gft_read_full(name.encode(), self.level, self.shape, ctypes.byref(cnames), self.rank, ctypes.byref(time), self.coords, self.data) != 1:
            raise Exception("gft_read_full failed")
        self.cnames = cnames.value.decode().split('|')
        self.time = time.value
        
        if len(self.shape) > 1:
            self.coords = np.split(self.coords,self.shape[:-1])
        self.data = self.data.reshape(self.shape[::-1]).T

#These functions are used for saving sdf files

def save_sdf(name, coords, data, **kwargs):
    if 'time' in kwargs:
        time = kwargs['time']
    else:
        time = 0.0
    if 'cnames' in kwargs:
        cnames = kwargs['cnames']
    else:
        cnames = ['x', 'y', 'z']
    
    shape = np.array([len(el) for el in coords], dtype=np.int32) 
    rank = len(shape)
    cnames = '|'.join(cnames[:rank])
    
    coords = np.concatenate(coords)
    data = data.flatten().reshape(shape).T.flatten() #Converting to Fortran convention
    if gft_out_full(name.encode(), time, shape, cnames.encode(), rank, coords, data) != 1:
        raise Exception("gft_out_full failed")
        
def save_bbox_sdf(name, coords, data, **kwargs):
    if 'time' in kwargs:
        time = kwargs['time']
    else:
        time = 0.0
  
    shape = np.array([len(el) for el in coords], dtype=np.int32) 
    rank = len(shape)
    
    coords = np.concatenate([[el[0], el[-1]] for el in coords])
    data = data.flatten().reshape(shape).T.flatten() #Converting to Fortran convention
    if gft_out_bbox(name.encode(), time, shape, rank, coords, data) != 1:
        raise Exception("gft_out_bbox failed")

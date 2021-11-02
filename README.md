This is a simple python package that can load and save sdf (simple data file) files 
primarily used by pamr numerical relativity code
This package should be compiled by make in src folder.
This package has a standalone copy of bbhutil package used to work with sdf files.
You should be able to compile this package if you have compiled pamr.

One known issue of this package is that saving routines only save files when the
python script exits.
Compiling the package using make will produce a few warning messages while compiling bbhutil,
these messages can be ignored, it does not affect the package performance.

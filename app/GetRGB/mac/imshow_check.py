import matplotlib
import matplotlib.pyplot as plt

import numpy as np
import sys,os

from ROOT import larcv
larcv.load_pyutil

iom = larcv.IOManager(larcv.IOManager.kREAD)

iom.add_in_file(sys.argv[1])

iom.initialize()


iom.read_entry(0);
ev_image = iom.get_data(larcv.kProductImage2D,"fake_color");
imm = np.zeros([ 864, 864, 3 ])

img_v = ev_image.Image2DArray()
    
assert img_v.size() == 3

for j in xrange(3):
    imm[:,:,j]  = larcv.as_ndarray( img_v[j] )
    imm[:,:,j] = imm[:,:,j].T

imm = imm[::-1,:,:]
imm = imm[:, :, (2, 1, 0)] 

fig,ax = plt.subplots(figsize = (12,12))
plt.imshow(imm)

plt.axis('off')
plt.show()

iom.finalize()

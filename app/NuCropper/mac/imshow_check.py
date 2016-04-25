import matplotlib
import matplotlib.pyplot as plt

import numpy as np
def normalize_image(image):
    a = np.min(image)
    b = np.max(image)

    return (image - a) / (b - a)

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

#imm = imm[:, :, (2, 1, 0)] 

fig,ax = plt.subplots(figsize = (12,12))

ax = plt.subplot(2,2,1)

imm = imm.astype(np.uint8)
plt.imshow(imm[ : , : , (2,1,0) ])
           
plt.axis('off')

for i in xrange(3):
    i+=1
    ax = plt.subplot(2,2,i+1)
    plt.imshow(imm[:,:,i-1],cmap='gray')
    ax.set_title("Channel: {}".format(i))
    plt.axis('off')
    
plt.tight_layout()
plt.show()

iom.finalize()
